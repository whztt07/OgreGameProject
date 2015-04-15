#include "MaxInterface.h"

static CMaxInterface g_MaxInterface;
CMaxInterface* GetMaxIP()
{
	return &g_MaxInterface;
}

void CMaxInterface::CBoneGroup::AddInfluence( CInfluence& infl )
{
	for( int i = 0; i < vectorInfl.size(); i++ )
	{
		if( strcmp( vectorInfl[i].szBoneName, infl.szBoneName ) == 0 )
		{
			vectorInfl[i].fWeight += infl.fWeight;
			return;
		}
	}

	vectorInfl.push_back( infl );
}

CMaxInterface::CMaxInterface(void)
{
}

CMaxInterface::~CMaxInterface(void)
{
}

void CMaxInterface::Create(ExpInterface *pExpInterface, Interface *pInterface)
{
	m_pExpInterface = pExpInterface;
	m_pInterface = pInterface;
}

INode* CMaxInterface::GetSelectedNode()
{
	if (m_pInterface->GetSelNodeCount() <= 0)
	{
		return NULL;
	}

	return m_pInterface->GetSelNode(0);
}

INode* CMaxInterface::GetRootNode()
{
	return m_pInterface->GetRootNode();
}

bool CMaxInterface::IsBone(INode *pNode)
{
	if (pNode == NULL)
	{
		return false;
	}

	// check for root node
	if(pNode->IsRootNode())
	{
		return false;
	}

	// check for bone node
	ObjectState os;
	os = pNode->EvalWorldState(0);
	if(os.obj->ClassID() == Class_ID(BONE_CLASS_ID, 0))
		return true;

#if MAX_RELEASE >= 4000
	if(os.obj->ClassID() == BONE_OBJ_CLASSID) return true;
#endif

	if(os.obj->ClassID() == Class_ID(DUMMY_CLASS_ID, 0)) return false;

	// check for biped node
	Control *pControl;
	pControl = pNode->GetTMController();
	if((pControl->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) || (pControl->ClassID() == BIPBODY_CONTROL_CLASS_ID))
	{
		return true;
	}

	return false;
}

bool CMaxInterface::IsBipedBone(INode *pNode)
{
	// check for invalid nodes
	if(pNode == 0) return false;

	// check for root node
	if(pNode->IsRootNode()) return false;

	// check for bone node
	ObjectState os;
	os = pNode->EvalWorldState(0);
	if(os.obj->ClassID() == Class_ID(DUMMY_CLASS_ID, 0)) return false;

	// check for biped node
	Control *pControl;
	pControl = pNode->GetTMController();
	if((pControl->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) || (pControl->ClassID() == BIPBODY_CONTROL_CLASS_ID)) return true;

	return false;
}

void CMaxInterface::ClearNodeTree()
{
	m_vecNode.clear();
}

int CMaxInterface::GetNodeCount()
{
	return m_vecNode.size();
}

INode* CMaxInterface::GetNode( int i )
{
	return m_vecNode[i];
}

void CMaxInterface::GetNodeTree( INode* pNode )
{
	if( pNode == NULL )
	{
		pNode = GetRootNode();
	}

	if( !pNode )
	{
		return;
	}

	if( pNode != GetRootNode() )
	{
		m_vecNode.push_back( pNode );
	}

	for (int i = 0; i < pNode->NumberOfChildren(); i++) 
	{
		GetNodeTree( pNode->GetChildNode( i ) );
	}
}

DWORD WINAPI CMaxInterface::ProgressFunction(LPVOID arg)
{
	return 0;
}

void CMaxInterface::StartProgressInfo(const std::string& strText)
{
	m_pInterface->ProgressStart(const_cast<char *>(strText.c_str()),
		true, ProgressFunction, 0);
}

void CMaxInterface::SetProgressInfo(int percentage)
{
	m_pInterface->ProgressUpdate(percentage);
}

void CMaxInterface::StopProgressInfo()
{
	m_pInterface->ProgressEnd();
}

bool CMaxInterface::IsMesh(INode *pNode)
{
	// check for root node
	if( !pNode ) 
		return false;

	if( pNode->IsRootNode() ) 
		return false;

	// check for mesh node
	ObjectState os;
	os = pNode->EvalWorldState(0);
	if(os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID) 
		return true;

	return false;
}

bool CMaxInterface::IsRenderable( INode* node )
{
	Object *obj = node->EvalWorldState ( 0 ).obj;	
	return obj->IsRenderable();
}

bool CMaxInterface::IsDummy(INode *pNode)
{
	// check for invalid nodes
	if(pNode == 0) return false;

	// check for root node
	if(pNode->IsRootNode()) return false;

	// check for dummy node
	ObjectState os;
	os = pNode->EvalWorldState(0);
	if(os.obj->ClassID() == Class_ID(DUMMY_CLASS_ID, 0)) return true;

	return false;
}

Mesh* CMaxInterface::GetMesh( INode* pNode )
{
	if( !IsMesh( pNode ) )
		return NULL;

	TimeValue time = 0;
	ObjectState os;
	os = pNode->EvalWorldState(time);

	Object* obj = os.obj;
	if( !os.obj )
	{
		assert( false );
		return NULL;
	}

	TriObject* triObj = (TriObject *)obj->ConvertToType( time, triObjectClassID );
	if( !triObj )
	{
		assert( false );
		return NULL;
	}

	Mesh* pMesh = &triObj->GetMesh();

	return pMesh;
}

Modifier* CMaxInterface::FindModifier(INode *pINode, Class_ID id)
{
	// get the object reference of the node
	Object *pObject;
	pObject = pINode->GetObjectRef();
	if(pObject == 0) 
		return 0;

	// loop through all derived objects
	while(pObject->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		IDerivedObject *pDerivedObject;
		pDerivedObject = static_cast<IDerivedObject *>(pObject);

		// loop through all modifiers
		int stackId;
		for(stackId = 0; stackId < pDerivedObject->NumModifiers(); stackId++)
		{
			// get the modifier
			Modifier *pModifier;
			pModifier = pDerivedObject->GetModifier(stackId);

			// check if we found the physique modifier
			if(pModifier->ClassID() == id)
				return pModifier;
		}

		// continue with next derived object
		pObject = pDerivedObject->GetObjRef();
	}

	return 0;

}

int CMaxInterface::GetFps()
{
	return GetFrameRate();
}

int CMaxInterface::GetStartTick()
{
	return m_pInterface->GetAnimRange().Start();
}
int CMaxInterface::GetEndTick()
{
	return m_pInterface->GetAnimRange().End();
}
int CMaxInterface::GetFrameCount()
{
	return (GetEndTick() - GetStartTick()) / GetFps() + 1;
}
int CMaxInterface::GetTickPerFrame()
{
	return GetTicksPerFrame();
}

bool CMaxInterface::GetMtlAnim( StdMat* pStdMtl, CColorTrack& track )
{
	if( pStdMtl == NULL )
	{
		assert( false && "std mtl is NULL" );
		return false;
	}

	int nFrameCount = 0;
	TimeValue nStartTick = GetStartTick();
	TimeValue nEndTick = GetEndTick();
	int nTickPerFrame = GetTickPerFrame();

	track.bTiling = FALSE;

	StdUVGen *uv = NULL;
	Texmap *tx = pStdMtl->GetSubTexmap(ID_OP);;
	if( tx )
	{
		if( tx->ClassID() == Class_ID( BMTEX_CLASS_ID, 0 ) )
		{
			BitmapTex *bmt = (BitmapTex*)tx;
			uv = bmt->GetUVGen();
			if( uv )
			{
				track.nUTile = (int)uv->GetUScl(0);
				track.nVTile = (int)uv->GetVScl(0);
				if( track.nUTile == 1 && track.nVTile == 1 )
					track.bTiling = FALSE;
				else
					track.bTiling = TRUE;
			}
		}
	}

	// ¼ÆËãÖ¡Êý
	TimeValue t;
	for( t = nStartTick; t <= nEndTick; t += nTickPerFrame )
		nFrameCount++;

	track.vectorColorKey.reserve( nFrameCount );
	track.vectorColorKey.resize( nFrameCount );

	t = nStartTick;
	for( int i = 0; i < nFrameCount; i++, t += nTickPerFrame )
	{
		CMdxColorKey key;
		memset( &key, 0x00, sizeof( key ) );
		Color diffuse	= pStdMtl->GetDiffuse( t );
		Color ambient	= pStdMtl->GetAmbient( t );
		Color specular	= pStdMtl->GetSpecular( t );
		Color filter	= pStdMtl->GetFilter( t );
		float alpha		= pStdMtl->GetOpacity( t );
		float shinstr	= pStdMtl->GetShinStr(t);
		float selfillum = pStdMtl->GetSelfIllum( t );

		key.dr = diffuse.r;
		key.dg = diffuse.g;
		key.db = diffuse.b;

		key.ar = ambient.r;
		key.ag = ambient.g;
		key.ab = ambient.b;

		key.sr = specular.r;
		key.sg = specular.g;
		key.sb = specular.b;

		key.alpha = alpha;
		key.shinstr = shinstr;
		key.selfillum = selfillum;


		if( uv )
		{
			key.uoffset = uv->GetUOffs( t );
			key.voffset = uv->GetVOffs( t );
		}
		else
		{
			key.uoffset = 0;
			key.voffset = 0;
		}

		track.vectorColorKey[i] = key;
	}

	return true;
}

bool CMaxInterface::GetStdMtlChannelBitmapFileName( StdMat* pStdMat, int nChannel, char *szFileName )
{
	if( !pStdMat )
	{
		assert( false );
		return FALSE;
	}

	Texmap *tx = pStdMat->GetSubTexmap(nChannel);
	if( !tx  )
		return FALSE;

	if(tx->ClassID() != Class_ID(BMTEX_CLASS_ID,0))
		return FALSE;

	BitmapTex *bmt = (BitmapTex*)tx;
	strcpy( szFileName, bmt->GetMapName() );
	
	return TRUE;
}

void CMaxInterface::GetBoneGroup( Modifier *pModifier,
									 int nModifierType, 
									 INode* pNode, 
									 Mesh* pMesh,
									 int nVertexId,
									 CMaxInterface::CBoneGroup& boneGroup )
{
	if( !pMesh )
	{
		assert( false );
		return;
	}

	if( nVertexId >= pMesh->numVerts )
	{
		assert( false );
		return;
	}

	// ¸ÕÌå
	if( nModifierType == MODIFIER_NONE )
	{
		INode* pParent = pNode->GetParentNode();
		if( pParent && ( IsBone( pParent ) || IsBipedBone( pParent ) ) )
		{
			CInfluence infl;
			infl.fWeight = 1.0f;
			strcpy( infl.szBoneName, pParent->GetName() );
			boneGroup.AddInfluence( infl );
		}
	}
	// check for physique modifier
	else if( nModifierType == MODIFIER_PHYSIQUE )
	{
		assert( pModifier && "get bone group error, modifier is null" );
		// create a physique export interface
		IPhysiqueExport *pPhysiqueExport;
		pPhysiqueExport = (IPhysiqueExport *)pModifier->GetInterface(I_PHYINTERFACE);
		if(pPhysiqueExport == 0)
		{
			return;
		}

		// create a context export interface
		IPhyContextExport *pContextExport;
		pContextExport = (IPhyContextExport *)pPhysiqueExport->GetContextInterface(pNode);
		if(pContextExport == 0)
		{
			pModifier->ReleaseInterface(I_PHYINTERFACE, pPhysiqueExport);
			return;
		}

		// set the flags in the context export interface
		pContextExport->ConvertToRigid(TRUE);
		pContextExport->AllowBlending(TRUE);

		// get the vertex export interface
		IPhyVertexExport *pVertexExport;
		pVertexExport = (IPhyVertexExport *)pContextExport->GetVertexInterface(nVertexId);
		if(pVertexExport == 0)
		{
			pPhysiqueExport->ReleaseContextInterface(pContextExport);
			pModifier->ReleaseInterface(I_PHYINTERFACE, pPhysiqueExport);
			return;
		}

		// get the vertex type
		int vertexType;
		vertexType = pVertexExport->GetVertexType();
		// handle the specific vertex type
		if(vertexType == RIGID_TYPE)
		{
			// typecast to rigid vertex
			IPhyRigidVertex *pTypeVertex;
			pTypeVertex = (IPhyRigidVertex *)pVertexExport;

			CInfluence infl;
			if( pTypeVertex->GetNode() )
			{
				strcpy( infl.szBoneName, pTypeVertex->GetNode()->GetName() );
				infl.fWeight = 1.0f;
				boneGroup.AddInfluence( infl );
			}
			else 
			{
				return;
			}
		}
		else if(vertexType == RIGID_BLENDED_TYPE)
		{
			// typecast to blended vertex
			IPhyBlendedRigidVertex *pTypeVertex;
			pTypeVertex = (IPhyBlendedRigidVertex *)pVertexExport;

			// loop through all influencing bones
			CInfluence infl;

			int nodeId;
			for(nodeId = 0; nodeId < pTypeVertex->GetNumberNodes(); nodeId++)
			{
				strcpy( infl.szBoneName, pTypeVertex->GetNode( nodeId )->GetName() );
				infl.fWeight = pTypeVertex->GetWeight( nodeId );
				boneGroup.AddInfluence( infl );
			}
		}

		// release all interfaces
		pPhysiqueExport->ReleaseContextInterface(pContextExport);
		pModifier->ReleaseInterface(I_PHYINTERFACE, pPhysiqueExport);
	}
	else if( nModifierType == MODIFIER_SKIN)
	{
		assert( pModifier && "get bone group error, modifier is null" );
		
		// create a skin interface
		ISkin *pSkin;
		pSkin = (ISkin*)pModifier->GetInterface(I_SKIN);
		if(pSkin == 0)
		{
			return;
		}

		// create a skin context data interface
		ISkinContextData *pSkinContextData;
		pSkinContextData = (ISkinContextData *)pSkin->GetContextInterface(pNode);
		if(pSkinContextData == 0)
		{
			pModifier->ReleaseInterface(I_SKIN, pSkin);
			return;
		}

		// loop through all influencing bones
		int nodeId;
		for(nodeId = 0; nodeId < pSkinContextData->GetNumAssignedBones(nVertexId); nodeId++)
		{
			// get the bone id
			int boneId;
			boneId = pSkinContextData->GetAssignedBone(nVertexId, nodeId);
			if(boneId < 0)
				continue;

			INode* pBone = pSkin->GetBone( boneId );
			CInfluence infl;
			strcpy( infl.szBoneName, pBone->GetName() );
			infl.fWeight = pSkinContextData->GetBoneWeight(nVertexId, nodeId);
			boneGroup.AddInfluence( infl );
		}

		// release all interfaces
		pModifier->ReleaseInterface(I_SKIN, pSkin);
	}
}

void CMaxInterface::GetTracks( int nNodeCount, INode** nodes, CTrack** tracks, BOOL bOnlyFirstFrame )
{
	StartProgressInfo("Get node track...");
	
	TimeValue nStartTick = GetStartTick();
	TimeValue nEndTick = GetEndTick();
	int nTickPerFrame = GetTickPerFrame();
	
	int nFrameCount = 0;
	if( bOnlyFirstFrame )
	{
		nFrameCount = 1;
	}
	else
	{
		for( TimeValue t = nStartTick; t <= nEndTick; t += nTickPerFrame )
			nFrameCount++;
	}

	for( int i = 0; i < nNodeCount; i++ )
	{
		tracks[i]->vectorMatrix.reserve( nFrameCount );
		tracks[i]->vectorMatrix.resize( nFrameCount );
		tracks[i]->vectorVisible.reserve( nFrameCount );
		tracks[i]->vectorVisible.resize( nFrameCount );

		Matrix3 matrix = nodes[i]->GetObjTMAfterWSM ( 0 );
		BOOL bMirror = DotProd ( CrossProd ( matrix.GetRow ( 0 ), matrix.GetRow ( 1 ) ), matrix.GetRow ( 2 ) ) < 0.0 ? TRUE : FALSE;
		tracks[i]->bMirror = bMirror;
	}

	TimeValue t = nStartTick;
	for( int nFrameId = 0; nFrameId < nFrameCount; nFrameId++, t += nTickPerFrame )
	{
		SetProgressInfo( 100.0f * nFrameId / nFrameCount );

		for( int nNodeId = 0; nNodeId < nNodeCount; nNodeId++ )
		{
			INode* pNode = nodes[nNodeId];
			CTrack* pTrack = tracks[nNodeId];

			Matrix3 tm = pNode->GetNodeTM(t);

			float fv = pNode->GetVisibility( t );
			if( fv == 0 )
				pTrack->vectorVisible[nFrameId] = false;
			else
				pTrack->vectorVisible[nFrameId] = true;

			CMatrix* matrix = &pTrack->vectorMatrix[nFrameId];
			for (int i = 0; i < 4; i++ )
			{
				Point3 row = tm.GetRow(i);
				matrix->m[i][0] = row.x;
				matrix->m[i][1] = row.y;
				matrix->m[i][2] = row.z;

			}

			matrix->m[0][3] = 0;
			matrix->m[1][3] = 0;
			matrix->m[2][3] = 0;
			matrix->m[3][3] = 1;
		}
	}
}