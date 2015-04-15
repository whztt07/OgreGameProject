
#include "MdxCandidate.h"

CMdxCandidate *g_pMdxCandidate = NULL;

int FloatCmp( float a, float b, float e = 0.0001f )
{
	if( fabsf( a-b ) < e )
		return 0;
	
	if( a < b )
		return -1;
	
	return 1;
}

int CompareBone( const void *arg0, const void *arg1 )
{
	INode* pNode0 = *(INode**)(arg0);
	INode* pNode1 = *(INode**)(arg1);

	if( stricmp( pNode0->GetName(), pNode1->GetName() ) > 0 )
		return 1;

	if( stricmp( pNode0->GetName(), pNode1->GetName() ) < 0 )
		return -1;

	return 0;
}

void ComputeVertexNormals(Mesh *mesh, 
						  Matrix3& tm,
						  Tab<CMdxCandidate::CVertexNormal>& vnorms, 
						  Tab<Point3>& fnorms )
{
	CMaxInterface* ip = GetMaxIP();
	ip->StartProgressInfo( "Compute Vertex Normals" );

	Face *face; 
	Point3 *verts;
	Point3 v0, v1, v2;
	face = mesh->faces; 
	verts = mesh->verts;
	Matrix3 rot = tm;
	
	// 得到旋转
	rot.NoTrans();
	rot.NoScale();

	vnorms.SetCount(mesh->getNumVerts());
	fnorms.SetCount(mesh->getNumFaces());

	int i = 0;
	// Compute face and vertex surface normals
	for (i = 0; i < mesh->getNumVerts(); i++) 
	{
		vnorms[i] = CMdxCandidate::CVertexNormal();
	}
	
	for (i = 0; i < mesh->getNumFaces(); i++, face++) 
	{
		// Calculate the surface normal
		if( i % 10 == 0)
			ip->SetProgressInfo( 100.0f * i / mesh->getNumFaces() );

		v0 = verts[face->v[0]];
		v1 = verts[face->v[1]];
		v2 = verts[face->v[2]];

		fnorms[i] = (v1 - v0) ^ (v2 - v1);
		fnorms[i] = rot * fnorms[i];
		for (int j = 0; j < 3; j++) 
		{  
			vnorms[face->v[j]].AddNormal(fnorms[i], face->smGroup);
		}
		fnorms[i] = Normalize(fnorms[i]);
	}

	int n = vnorms.Count();
	for (i = 0; i < mesh->getNumVerts(); i++) 
	{
		vnorms[i].Normalize();
	}

	// Display the normals in the debug window of the VC++ IDE
}

//--------------------------------------
//--------------CTextures---------------
//--------------------------------------
int CMdxCandidate::CTextures::AddTexture(CTexture& t)
{
	int nRet = vectorTexture.size();
	vectorTexture.push_back(t);
	return nRet;
}

int CMdxCandidate::CTextures::GetTextureCount()
{ 
	return vectorTexture.size();
}

CMdxCandidate::CTexture* CMdxCandidate::CTextures::GetTexture( int i )
{ 
	return &vectorTexture[i];
}

//--------------------------------------
//--------------CSkeleton---------------
//--------------------------------------
int CMdxCandidate::CSkeleton::FindBone( const char* pszBoneName )
{
	for( int i = 0; i < vectorBone.size(); i++ )
	{
		if( stricmp( pszBoneName, vectorBone[i].szName ) == 0 )
			return i;
	}

	return -1;
}

//--------------------------------------
//----------CVertexNormal---------------
//--------------------------------------
CMdxCandidate::CVertexNormal::CVertexNormal()
{
	smooth=0;
	next=NULL;
	init=FALSE;
	norm=Point3(0,0,0);
}

CMdxCandidate::CVertexNormal::CVertexNormal(Point3 &n,DWORD s)
{
	next=NULL;
	init=TRUE;
	norm=n;
	smooth=s;
}

CMdxCandidate::CVertexNormal::~CVertexNormal() 
{
	if(next)
	{
		delete next;
		next = NULL;
	}
}

// Add a normal to the list if the smoothing group bits overlap, 

// otherwise create a new vertex normal in the list
void CMdxCandidate::CVertexNormal::AddNormal(Point3 &n, DWORD s) 
{
	if (!(s & smooth) && init)
	{
		if (next)
		{
			next->AddNormal(n, s);
		}
		else 
		{
			next = new CVertexNormal(n, s);
		}
	} 
	else 
	{
		norm += n;
		smooth |= s;
		init = TRUE;
	}
}

// Retrieves a normal if the smoothing groups overlap or there is 
// only one in the list
Point3& CMdxCandidate::CVertexNormal::GetNormal(DWORD s) 
{
	if ( (smooth & s) || !next)
	{
		return norm;
	}
	else 
	{
		return next->GetNormal(s);
	}
}

// Normalize each normal in the list
void CMdxCandidate::CVertexNormal::Normalize() 
{
	//CVertexNormal *ptr = next, *prev = this;
	//while (ptr) 
	//{
	//	if (ptr->smooth&smooth) 
	//	{
	//		norm += ptr->norm;
	//		prev->next = ptr->next;
	//		delete ptr;
	//		ptr = prev->next;
	//	} 
	//	else 
	//	{
	//		prev = ptr;
	//		ptr = ptr->next;
	//	}
	//}

	norm = ::Normalize(norm);
	if (next)
	{
		next->Normalize();
	}
}

//--------------------------------------
//--------------CMdxCandidate-----------
//--------------------------------------
CMdxCandidate::CMdxCandidate(void)
{
}

CMdxCandidate::~CMdxCandidate(void)
{
}

void CMdxCandidate::CollectBoneAndAttachment(INode *pNode)
{
	char szName[256];
	strcpy( szName, pNode->GetName() );
	int nLength = strlen( szName );

	// attachment物体，不计算为骨骼，辅助物体
	if( strnicmp( szName, "attc", 4 ) == 0 )
	{
		CAttachment attachment;
		attachment.pNode = pNode;
		strcpy( attachment.szName, pNode->GetName() );
		Matrix3 tm = pNode->GetObjTMAfterWSM ( 0 );
		for (int r =0; r < 4; r++ )
		{
			Point3 row = tm.GetRow(r);
			attachment.matInit.m[r][0] = row.x;
			attachment.matInit.m[r][1] = row.y;
			attachment.matInit.m[r][2] = row.z;
		}

		attachment.matInit.m[0][3] = 0;
		attachment.matInit.m[1][3] = 0;
		attachment.matInit.m[2][3] = 0;
		attachment.matInit.m[3][3] = 1;

		m_Attachments.vectorAttachment.push_back( attachment );
	}
	else
	{
		m_vecBone.push_back( pNode );
	}
}

void CMdxCandidate::CreateGeometry(INode *pNode)
{
	GetMaxIP()->StartProgressInfo( "Create Geometry" );
	
	if( !pNode )
		return;

	if( !GetMaxIP()->IsMesh( pNode ) )
		return;
	
	if( !GetMaxIP()->IsRenderable( pNode ) )
		return;
	
	if( GetMaxIP()->IsBone( pNode ) )
		return;
	
	if( GetMaxIP()->IsDummy( pNode ) )
		return;

	Mesh* mesh = GetMaxIP()->GetMesh( pNode );
	if( !mesh )
		return;

	// 判断是否为镜像物体
	Matrix3 matrix = pNode->GetObjTMAfterWSM ( 0 );
	BOOL bMirror = DotProd ( CrossProd ( matrix.GetRow ( 0 ), matrix.GetRow ( 1 ) ), matrix.GetRow ( 2 ) ) < 0.0 ? TRUE : FALSE;

	mesh->buildNormals();
	mesh->buildRenderNormals();
	Matrix3	matTrans = matrix;
	Modifier* pModifier = GetMaxIP()->FindModifier( pNode, Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B) );
	int nModifierType;
	if(	pModifier )
		nModifierType	= MODIFIER_PHYSIQUE;
	else
	{
		pModifier = GetMaxIP()->FindModifier( pNode, SKIN_CLASSID );
		if(	pModifier )
			nModifierType = MODIFIER_SKIN;
		else
			nModifierType = MODIFIER_NONE;
	}

	// 收集材质...
	Mtl* pMtl = pNode->GetMtl();
	std::vector<StdMat*> vectorStdMtl;
	GetMaxIP()->StartProgressInfo( "Get Mtl" );
	if( pMtl )
	{
		if( pMtl->IsMultiMtl() )
		{
			for( int i = 0; i < pMtl->NumSubMtls(); i++ )
			{
				Mtl* pSubMtl = pMtl->GetSubMtl( i );
				StdMat* pStdMtl = (StdMat*)pSubMtl;
				if( pStdMtl == NULL )
					return;
				vectorStdMtl.push_back( pStdMtl );
			}
		}
		else
		{
			StdMat* pStdMtl = (StdMat*)pMtl;
			vectorStdMtl.push_back( pStdMtl );
		}
	}

	// new Chunk...
	int nStdMtlCount = vectorStdMtl.size();
	std::vector<CGeomChunk> chunks;
	if( nStdMtlCount > 0 )
	{
		chunks.reserve( nStdMtlCount );
		chunks.resize( nStdMtlCount );
		for( int i = 0; i < chunks.size(); i++ )
		{
			strcpy( chunks[i].szNodename, pNode->GetName() );
			chunks[i].pStdMtl = vectorStdMtl[i];
		}
	}
	else
	{
		chunks.reserve( 1 );
		chunks.resize( 1 );
		strcpy( chunks[0].szNodename, pNode->GetName() );
	}

	// 赋予材质...
	for( int nStdMtlId = 0; nStdMtlId < vectorStdMtl.size(); nStdMtlId++ )
	{
		StdMat* pStdMtl = vectorStdMtl[nStdMtlId];

		if( pStdMtl )
		{
			CMaxInterface::CColorTrack colorTrack;
			GetMaxIP()->GetMtlAnim( pStdMtl, colorTrack );

			BOOL bDiffuseMap = FALSE;
			BOOL bTransMap = FALSE;
			BOOL bSpecularMap = FALSE;

			char szDiffuseMap[MAX_PATH] = "";
			char szTransMap[MAX_PATH] = "";
			char szSpecularMap[MAX_PATH] = "";
			if( GetMaxIP()->GetStdMtlChannelBitmapFileName( pStdMtl, ID_DI, szDiffuseMap ) )
			{
				bDiffuseMap = TRUE;
			}
			if( GetMaxIP()->GetStdMtlChannelBitmapFileName( pStdMtl, ID_OP, szTransMap ) )
			{
				bTransMap = TRUE;
			}
			if( GetMaxIP()->GetStdMtlChannelBitmapFileName( pStdMtl, ID_SP, szSpecularMap ) )
			{
				bSpecularMap = TRUE;
			}

			CTexture texture;
			if( bDiffuseMap )
			{
				strcpy( texture.szName, szDiffuseMap );
			}
			else if( bTransMap )
			{			
				strcpy( texture.szName, szTransMap );
			}

			int nTextureId = FindTexture( &texture );
			if( nTextureId == -1 )
			{
				nTextureId = m_Textures.AddTexture(texture);
			}

			DWORD dwFlag = 0;
			if( pStdMtl->GetTwoSided() )
				dwFlag |= OGRE_MDX_TWOSIDED;
			
			if( pStdMtl->GetWire() )
				dwFlag |= OGRE_MDX_WIREFRAME;
			
			if( bDiffuseMap )
			{
				dwFlag |= OGRE_MDX_ALPHATEST;
			}
			else if( bTransMap )
			{
				dwFlag |= OGRE_MDX_ALPHABLEND;
			}

			CMaterial mtl;
			mtl.vectorColorTrack.push_back( colorTrack );

			int nMtlId = FindMtl( &mtl );
			if( nMtlId == -1 )
			{
				nMtlId = m_Mtls.vectorMtl.size();
				m_Mtls.vectorMtl.push_back( mtl );
			}
			chunks[nStdMtlId].nMtlId = nMtlId;
		}
		else
		{
			chunks[nStdMtlId].nMtlId = -1;
		}

		chunks[nStdMtlId].nModifierType = nModifierType;
	}

	Tab<CVertexNormal> vnorms;
	Tab<Point3> fnorms;
	ComputeVertexNormals( mesh, matTrans, vnorms, fnorms );

	// get mesh...
	GetMaxIP()->StartProgressInfo( "Get Mesh" );
	for( int nFaceId = 0; nFaceId < mesh->numFaces; nFaceId++ )
	{
		if( nFaceId % 10 == 0 )
			GetMaxIP()->SetProgressInfo( 100.0f * nFaceId / mesh->numFaces );

		CMtlFace face;
		CGeomChunk* pChunk = NULL;
		if( nStdMtlCount > 0 )
		{
			int nStdMtlId = mesh->getFaceMtlIndex( nFaceId );
			if( nStdMtlCount > 0 )
			{
				nStdMtlId %= nStdMtlCount;
			}

			pChunk = &chunks[nStdMtlId];
		}
		else
		{
			pChunk = &chunks[0];
		}

		for( int nVertexId = 0; nVertexId < 3; nVertexId++ )
		{
			float u = 0.0f;
			float v = 0.0f;
			if( mesh->tvFace )
			{
				int nOffset = mesh->tvFace[nFaceId].t[nVertexId];
				u = mesh->tVerts[nOffset].x;
				v = 1 - mesh->tVerts[nOffset].y;
			}

			int nOldVertexId = mesh->faces[nFaceId].getVert(nVertexId);
			Point3 normal = vnorms[nOldVertexId].GetNormal( mesh->faces[nFaceId].smGroup );

			DWORD dwColor = 0xffffffff;
			if( mesh->vertCol )
			{
				int k = mesh->numCVerts;
				int nVCVertexID = mesh->vcFace[nFaceId].t[nVertexId];
				Point3 color = mesh->vertCol[nVCVertexID];
				DWORD r = ((DWORD)(color.x*255));
				DWORD g = ((DWORD)(color.y*255));
				DWORD b = ((DWORD)(color.z*255));
				dwColor = 0xff000000|r<<16|g<<8|b;
			}
			else if( mesh->vertColArray )
			{
				int k = mesh->numCVerts;
				int nVCVertexID = mesh->vcFaceData[nFaceId].t[nVertexId];
				Point3 color = mesh->vertColArray[nVCVertexID];
				DWORD r = ((DWORD)(color.x*255));
				DWORD g = ((DWORD)(color.y*255));
				DWORD b = ((DWORD)(color.z*255));
				dwColor = 0xff000000|r<<16|g<<8|b;
			}

			bool found = false;
			for( int i = 0; i < pChunk->vectorSplitVertex.size(); i++ )
			{
				if( nOldVertexId == pChunk->vectorSplitVertex[i].nVertexId &&
					dwColor == pChunk->vectorSplitVertex[i].color &&
					FloatCmp( u, pChunk->vectorSplitVertex[i].u ) == 0 &&
					FloatCmp( v, pChunk->vectorSplitVertex[i].v ) == 0 &&
					FloatCmp( normal.x, pChunk->vectorSplitVertex[i].normal.x ) == 0 && 
					FloatCmp( normal.y, pChunk->vectorSplitVertex[i].normal.y ) == 0 && 
					FloatCmp( normal.z, pChunk->vectorSplitVertex[i].normal.z ) == 0 )
				{
					found = true;
					face.nMaxVertexId[nVertexId] = i;
					break;
				}
			}

			if( !found )
			{
				face.nMaxVertexId[nVertexId] = pChunk->vectorSplitVertex.size();

				CSplitVertex s;
				s.nVertexId = nOldVertexId;
				s.pos = mesh->getVert( nOldVertexId ) * matTrans;
				s.normal = normal;
				s.color = dwColor;
				s.u = u;
				s.v = v;
				pChunk->vectorSplitVertex.push_back( s );
			}
		}

		if( bMirror )
		{
			int temp = face.nMaxVertexId[0];
			face.nMaxVertexId[0] = face.nMaxVertexId[1];
			face.nMaxVertexId[1] = temp;
		}

		pChunk->vectorFace.push_back( face );
	}

	// build chunk...
	GetMaxIP()->StartProgressInfo( "Build Chunk" );
	for( int nChunkId = 0; nChunkId < chunks.size(); nChunkId++ )
	{
		CGeomChunk* pChunk = &chunks[nChunkId];
		for( int i = 0; i < pChunk->vectorFace.size(); i++ )
		{
			CMtlFace* face = &pChunk->vectorFace[i];
			if( face->nMaxVertexId[0] >= pChunk->vectorSplitVertex.size() ||
				face->nMaxVertexId[1] >= pChunk->vectorSplitVertex.size() ||
				face->nMaxVertexId[2] >= pChunk->vectorSplitVertex.size() )
			{
				assert( false && "chunk error, face index > vertex count" );
			}
		}

		BOOL bAllVertexCountIsWhite = TRUE;
		
		GetMaxIP()->StartProgressInfo( "Process Split Vertex" );
		int nSplitVertexCount = pChunk->vectorSplitVertex.size();
		pChunk->vectorVertex.reserve( nSplitVertexCount );
		pChunk->vectorVertex.resize( nSplitVertexCount );
		pChunk->vectorNormal.reserve( nSplitVertexCount );
		pChunk->vectorNormal.resize( nSplitVertexCount );
		pChunk->vectorColor.reserve( nSplitVertexCount );
		pChunk->vectorColor.resize( nSplitVertexCount );
		pChunk->vectorUV.reserve( nSplitVertexCount );
		pChunk->vectorUV.resize( nSplitVertexCount );
		pChunk->vectorBGId.reserve( nSplitVertexCount );
		pChunk->vectorBGId.resize( nSplitVertexCount );

		for( int i = 0; i < pChunk->vectorSplitVertex.size(); i++ )
		{
			char s[256];
			sprintf( s, "Process Split Vertex %ld", i );
			if( i % 10 == 0 )
			{
				GetMaxIP()->StartProgressInfo( s );
				GetMaxIP()->SetProgressInfo( 100.0f * i / pChunk->vectorSplitVertex.size() );
			}

			pChunk->vectorVertex[i] = pChunk->vectorSplitVertex[i].pos;
			pChunk->vectorNormal[i] = pChunk->vectorSplitVertex[i].normal;
			pChunk->vectorColor[i] = pChunk->vectorSplitVertex[i].color;

			bAllVertexCountIsWhite = FALSE;

			CMdxUV uv;
			uv.u = pChunk->vectorSplitVertex[i].u;
			uv.v = pChunk->vectorSplitVertex[i].v;
			pChunk->vectorUV[i] = uv;

			CMaxInterface::CBoneGroup bg;
			GetMaxIP()->GetBoneGroup( 
				pModifier, 
				nModifierType, 
				pNode, 
				mesh,
				pChunk->vectorSplitVertex[i].nVertexId, 
				bg );
			
			if( bg.vectorInfl.size() != 0 )
			{
				int nBGId = FindBoneGroup( &bg );
				if( nBGId == -1 )
				{
					nBGId = m_BoneGroups.vectorBoneGroup.size();
					m_BoneGroups.vectorBoneGroup.push_back( bg );
				}
								
				pChunk->vectorBGId[i] = nBGId;
			}
			else
			{
				pChunk->vectorBGId[i] = -1;
			}

			pChunk->vectorSplitVertex[i].bg = bg;
		}
	}

	for( int nChunkId = 0; nChunkId < chunks.size(); nChunkId++ )
	{
		CGeomChunk* pChunk = &chunks[nChunkId];
		m_Geometry.vectorChunk.push_back( *pChunk );
	}
}

void CMdxCandidate::CreateMdx(INode *pSelNode)
{
	m_vecBone.clear();
	GetMaxIP()->ClearNodeTree();
	GetMaxIP()->GetNodeTree(pSelNode);
	// ...
	GetMaxIP()->StartProgressInfo("Get All Node...");
	
	int nNodeCount = GetMaxIP()->GetNodeCount();
	for (int i = 0; i < nNodeCount; i++)
	{
		GetMaxIP()->SetProgressInfo( 100.0f * i / nNodeCount );
		INode* pNode = GetMaxIP()->GetNode( i );

		// 如果是骨骼，或者是特定的辅助物体
		if( GetMaxIP()->IsBone( pNode ) || 
			GetMaxIP()->IsBipedBone( pNode ) )
		{
			CollectBoneAndAttachment(pNode);
		}
		else
		{
			CreateGeometry(pNode);
		}
	}

	// bone serialize...
	if( m_vecBone.size() == 0 )
	{
		MessageBox( NULL, "Create Skeleton Failed, Bone Count = 0", "Failed", MB_OK );
	}
	qsort( (void*)&m_vecBone[0], m_vecBone.size(), sizeof(INode*) , CompareBone );

	// create bone...
	for( int i = 0; i < m_vecBone.size(); i++ )
	{
		INode* pNode = m_vecBone[i];

		CBone bone;
		bone.pNode = pNode;
		strcpy( bone.szName, pNode->GetName() );
		INode* pParent = pNode->GetParentNode();

		if( pParent == GetMaxIP()->GetRootNode() )
			strcpy( bone.szParentName, "" );
		else 
			strcpy( bone.szParentName, pNode->GetParentNode()->GetName() );

		bone.nParentId = -1;
		m_Skeleton.vectorBone.push_back( bone );
	}

	// create track...
	long const MAX_NODES = 1024;
	INode* nodes[MAX_NODES];
	CMaxInterface::CTrack* tracks[MAX_NODES];
	int i = 0;
	for( i = 0; i < m_Skeleton.vectorBone.size(); i++ )
	{
		nodes[i] = m_Skeleton.vectorBone[i].pNode;
		tracks[i] = &m_Skeleton.vectorBone[i].track;
	}
	GetMaxIP()->GetTracks( m_vecBone.size(), nodes, tracks );

	// create skeleton...
	for( i = 0; i < m_Skeleton.vectorBone.size(); i++ )
	{
		CBone* pBone = &m_Skeleton.vectorBone[i];
		if( pBone->szParentName[0] == 0 ||
			pBone->pNode->GetParentNode() == GetMaxIP()->GetRootNode() ||
			pBone->pNode->GetParentNode() == NULL )
		{
			pBone->nParentId = -1;
			m_Skeleton.vectorRootBoneId.push_back( i );
		}
		else
		{
			for( int nBoneId = 0; nBoneId < m_Skeleton.vectorBone.size(); nBoneId++ )
			{
				if( nBoneId == i )
					continue;

				if( strcmp( pBone->szParentName,
					m_Skeleton.vectorBone[nBoneId].szName ) == 0 )
				{
					pBone->nParentId = nBoneId;
					m_Skeleton.vectorBone[nBoneId].vectorChildId.push_back( i );
				}
			}
		}
	}

	ProjBoneGroups();
	for( int nAtt = 0; nAtt < m_Attachments.vectorAttachment.size(); nAtt++ )
	{
		CAttachment* pAttachment = &m_Attachments.vectorAttachment[nAtt];
		
		if( pAttachment->pNode->GetParentNode() )
		{
			pAttachment->nAttachBoneID = m_Skeleton.FindBone( pAttachment->pNode->GetParentNode()->GetName() );
		}
	}	

	GetMaxIP()->StopProgressInfo();
}

int CMdxCandidate::FindTexture( CMdxCandidate::CTexture* pTexture )
{
	int nNum = m_Textures.GetTextureCount();
	for( int i = 0; i < nNum; i++ )
	{
		CTexture *pTex = m_Textures.GetTexture(i);
		if (pTex)
		{
			if( stricmp( pTex->szName, pTexture->szName ) == 0 )
				return i;
		}
	}

	return -1;
}

void CMdxCandidate::ProjBoneGroups()
{
	for( int i = 0; i < m_BoneGroups.vectorBoneGroup.size(); i++ )
	{
		CMaxInterface::CBoneGroup* bg = &m_BoneGroups.vectorBoneGroup[i];
		for( int j = 0; j < bg->vectorInfl.size(); j++ )
		{
			bg->vectorInfl[j].nBoneId = FindBone( bg->vectorInfl[j].szBoneName );
		}
	}
}

int CMdxCandidate::FindBone( const char* pszBoneName )
{
	for( int i = 0; i < m_Skeleton.vectorBone.size(); i++ )
	{
		if( strcmp( m_Skeleton.vectorBone[i].szName, pszBoneName ) == 0 )
			return i;
	}

	return -1;
}

int CMdxCandidate::FindMtl( CMdxCandidate::CMaterial* pMtl )
{
	for( int i = 0; i < m_Mtls.vectorMtl.size(); i++ )
	{
		CMdxCandidate::CMaterial* pMtlSrc = &m_Mtls.vectorMtl[i];
		if( pMtlSrc->vectorColorTrack.size() != pMtl->vectorColorTrack.size() )
			continue;

		BOOL bSame = TRUE;
		for( int j = 0; j < pMtlSrc->vectorColorTrack.size(); j++ )
		{
			CMaxInterface::CColorTrack* pTrackSrc = &pMtlSrc->vectorColorTrack[j];
			CMaxInterface::CColorTrack* pTrackDst = &pMtl->vectorColorTrack[j];
			if( pTrackSrc->bTiling != pTrackDst->bTiling ||
				pTrackSrc->nUTile != pTrackDst->nUTile ||
				pTrackSrc->nVTile != pTrackDst->nVTile ||
				pTrackSrc->vectorColorKey.size() != pTrackDst->vectorColorKey.size() )
			{
				bSame = FALSE;
				break;
			}

			if( pTrackSrc->vectorColorKey.size() == pTrackDst->vectorColorKey.size() )
			{
				if( memcmp( &pTrackSrc->vectorColorKey[0], 
					&pTrackDst->vectorColorKey[0], 
					sizeof(CMaxInterface::CMdxColorKey) * pTrackDst->vectorColorKey.size() ) != 0 )
				{
					bSame = FALSE;
					break;
				}
			}
		}

		if( bSame )
			return i;
	}

	return -1;
}

int CMdxCandidate::FindBoneGroup( CMaxInterface::CBoneGroup* infls )
{

	for( int i = 0; i < m_BoneGroups.vectorBoneGroup.size(); i++ )
	{
		if( m_BoneGroups.vectorBoneGroup[i].vectorInfl.size() != infls->vectorInfl.size() )
			continue;
		
		BOOL bFound = TRUE;
		for( int id = 0; id < m_BoneGroups.vectorBoneGroup[i].vectorInfl.size(); id++ )
		{
			if( strcmp( m_BoneGroups.vectorBoneGroup[i].vectorInfl[id].szBoneName, infls->vectorInfl[id].szBoneName ) != 0  )
			{
				bFound = FALSE;
				break;
			}
		}

		if( bFound )
			return i;
	}

	return -1;
}

bool CMdxCandidate::SaveMdx(char *szFileName)
{
	CDataChunkWrite w(1024 * 1024 * 6);

	// tex...
	w.StartChunk( DC_TAG( 'texs' ) );
	{
		w.WriteInt( m_Textures.GetTextureCount() );
		if( m_Textures.GetTextureCount() > 0 )
		{
			int nSize = sizeof(m_Textures.GetTexture(0)->szName);
			int nCount = m_Textures.GetTextureCount();
			w.Write( m_Textures.GetTexture(0), nSize, nCount );
		}
	}
	w.EndChunk( DC_TAG( 'texs' ) );

	// geo..
	w.StartChunk( DC_TAG( 'geom' ) );
	{
		for( int i = 0; i < m_Geometry.vectorChunk.size(); i++ )
		{
			CGeomChunk* pChunk = &m_Geometry.vectorChunk[i];
			if( pChunk->vectorVertex.size() == 0 ||
				pChunk->vectorFace.size() == 0 )
			{
				continue;
			}

			w.StartChunk( DC_TAG( 'chks' ) );
			{
				w.WriteInt( pChunk->vectorVertex.size() );
				w.WriteInt( pChunk->vectorFace.size() );
				w.WriteInt( pChunk->nMtlId );

				int nVertexCount = pChunk->vectorVertex.size();
				int nFaceCount = pChunk->vectorFace.size();
				if( nVertexCount > 0 )
				{
					w.Write( &pChunk->vectorVertex[0], sizeof( CVector ), nVertexCount );
					w.Write( &pChunk->vectorNormal[0], sizeof( CVector ), nVertexCount );
					w.Write( &pChunk->vectorUV[0], sizeof( float )*2, nVertexCount );
					w.Write( &pChunk->vectorBGId[0], sizeof( byte ), nVertexCount );
				}
				
				if( nFaceCount > 0 )
				{
					w.Write( &pChunk->vectorFace[0], sizeof( CMtlFace ), nFaceCount );
				}

				if( pChunk->vectorColor.size() > 0 )
				{
					w.StartChunk( DC_TAG( 'vcol' ) );
					{
						BOOL bAllVertexColorIsWhite = TRUE;
						for( int i = 0; i < nVertexCount; i++ )
						{
							if( pChunk->vectorColor[i] != 0xffffffff )
							{
								bAllVertexColorIsWhite = FALSE;
								break;
							}
						}

						if( !bAllVertexColorIsWhite )
						{
							w.Write( &pChunk->vectorColor[0], sizeof( DWORD ), pChunk->vectorColor.size() );
						}
					}
					w.EndChunk( DC_TAG( 'vcol' ) );
				}
				
				w.StartChunk( DC_TAG( 'name' ) );
				{
					char szChunkName[MAX_PATH];
					ZeroMemory( szChunkName, sizeof( szChunkName ) );
					strcpy( szChunkName, pChunk->szNodename );
					w.Write( szChunkName, MAX_PATH, 1 );

				}
				w.EndChunk( DC_TAG( 'name' ) );
			}
			w.EndChunk( DC_TAG( 'chks' ) );
		}
	}
	w.EndChunk( DC_TAG( 'geom' ) );

	// skeleton...
	w.StartChunk( DC_TAG( 'sklt' ) );
	{
		w.WriteInt( m_Skeleton.vectorBone.size() );
		w.WriteInt( m_Skeleton.vectorRootBoneId.size() );
		int nNumRootBone = m_Skeleton.vectorRootBoneId.size();
		if( nNumRootBone > 0 )
		{
			w.Write( &m_Skeleton.vectorRootBoneId[0],
				sizeof( int ), m_Skeleton.vectorRootBoneId.size() );
		}

		for( int i = 0; i < m_Skeleton.vectorBone.size(); i++ )
		{
			CBone* pBone = &m_Skeleton.vectorBone[i];
			w.StartChunk( DC_TAG( 'bone' ) );
			{
				w.Write( pBone->szName, MDX_MAX_NAME, 1 );
				w.WriteInt( pBone->nParentId );
				w.WriteInt( pBone->vectorChildId.size() );
				for( int b = 0; b < pBone->vectorChildId.size(); b++ )
				{
					w.WriteInt( pBone->vectorChildId[b] );
				}

				int nNumFrame = pBone->track.vectorMatrix.size();
				w.StartChunk( DC_TAG( 'trck' ) );
				{
					w.WriteInt( pBone->track.vectorMatrix.size() );
					w.Write( &pBone->track.vectorMatrix[0], 
						sizeof( CMatrix ), pBone->track.vectorMatrix.size() );
				}
				w.EndChunk( DC_TAG( 'trck' ) );

				w.StartChunk( DC_TAG( 'vis2' ) );
				{
					if( pBone->track.vectorVisible.size() > 0 )
					{
						BOOL bHasUnvisibleFrame = FALSE;
						for( int i = 0; i < pBone->track.vectorVisible.size(); i++ )
						{
							if( !pBone->track.vectorVisible[i] )
							{
								bHasUnvisibleFrame = TRUE;
								break;
							}
						}

						if( bHasUnvisibleFrame )
						{
							w.Write( &pBone->track.vectorVisible[0], 
								sizeof( BOOL ), pBone->track.vectorVisible.size() );
						}

					}
				}
				w.EndChunk( DC_TAG( 'vis2' ) );
			}
			w.EndChunk( DC_TAG( 'bone' ) );
		}
	}
	w.EndChunk( DC_TAG( 'sklt' ) );

	// bonegroups...
	w.StartChunk( DC_TAG( 'bgps' ) );
	{
		for( int i = 0; i < m_BoneGroups.vectorBoneGroup.size(); i++ )
		{
			w.StartChunk( DC_TAG( 'bgrp' ) );
			{
				int nInflCount = m_BoneGroups.vectorBoneGroup[i].vectorInfl.size();
				w.WriteInt( nInflCount );
				for( int b = 0; b < nInflCount; b++ )
				{
					int nBoneId = m_BoneGroups.vectorBoneGroup[i].vectorInfl[b].nBoneId;

					w.WriteInt( nBoneId );
				}
			}
			w.EndChunk( DC_TAG( 'bgrp' ) );
		}
	}
	w.EndChunk( DC_TAG( 'bgps' ) );

	BOOL bOK = w.SaveToFile( szFileName );
	w.Destroy();

	return bOK;
}
