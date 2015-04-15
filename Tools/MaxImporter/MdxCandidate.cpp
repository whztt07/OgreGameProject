

#include "MdxCandidate.h"

CMdxCandidate *g_pMdxCandidate;
CMdxCandidate::CMdxCandidate()
{
	m_pMdx = NULL;
}

CMdxCandidate::~CMdxCandidate()
{
	SAFE_DELETE(m_pMdx);
}

void CMdxCandidate::ImportMdx(char *filename)
{
	m_pMdx = new CMdx;
	if (m_pMdx)
	{
		m_pMdx->LoadFromFile(filename, 0);
		CMdxGeometry* pGeometry = m_pMdx->GetGeometry();

		if (pGeometry)
		{
			int nChunkNum = pGeometry->GetChunkCount();
			for (int nChunk = 0; nChunk < nChunkNum; nChunk++)
			{
				CMdxGeoChunk *pChunk = pGeometry->GetChunk(nChunk);
				if (pChunk)
				{
					TriObject *triObj = new TriObject; 
					INode *pNode = GetMaxIP()->GetIP()->CreateObjectNode((Object *)triObj);
					GetMaxIP()->GetRootNode()->AttachChild(pNode);
					pNode->SetName(pChunk->GetName());

					Mesh &mesh = ((TriObject*)pNode->GetObjectRef())->GetMesh();
					OnImportVertex(pChunk, mesh);
					OnImportTVert(pChunk, mesh);
					OnImportFace(pChunk, mesh);
					OnImportTexture(filename, m_pMdx, pChunk, mesh, pNode);
				}
			}
		}
	}
}

void CMdxCandidate::OnImportTexture(char *filename, CMdx *pMdx, CMdxGeoChunk *pChunk, Mesh &mesh, INode *pNode)
{
	// Create a material to the node
	StdMat2 *mtl = NewDefaultStdMat();
	Color ambient(1.0f, 0.0f, 0.0f);
	mtl->SetAmbient(ambient, 0); 
	Color diffuse(1.0f, 0.0f, 0.0f);
	mtl->SetDiffuse(diffuse, 0);

	// Set The Texture
	BitmapTex *bmtex = (BitmapTex*)NewDefaultBitmapTex();
	
	char *name = pChunk->GetName();
	bmtex->SetName(name);

	CMdxTexture *pTex = pMdx->GetTextures()->GetTexture(pChunk->GetMtlID());
	std::string texpath = filename;
	int nPos = texpath.find_last_of('\\');
	texpath = texpath.substr(0, nPos);
	std::string texname = pTex->GetName();
	nPos = texname.find_last_of('.');
	texname = texname.substr(0, nPos);
	texname = texname + ".dds";
	texname = texpath + "\\" + texname;
	bmtex->SetMapName(texname.c_str());
	
	mtl->SetSubTexmap(ID_DI, bmtex);
	//mtl->SetActiveTexmap(bmtex);
	//mtl->SetMtlFlag(MTL_TEX_DISPLAY_ENABLED);
	mtl->SetMtlFlag(MTL_DISPLAY_ENABLE_FLAGS);
	pNode->SetMtl(mtl);
	GetMaxIP()->GetIP()->RedrawViews(GetMaxIP()->GetIP()->GetTime());
	
	int nMtlId = pChunk->GetMtlID();
	GetMaxIP()->GetIP()->PutMtlToMtlEditor(mtl, nMtlId);
}

void CMdxCandidate::OnImportFace(CMdxGeoChunk *pChunk, Mesh &mesh)
{
	int nFaceNum = pChunk->GetFaceNum();
	mesh.setNumFaces(nFaceNum);
	mesh.setNumTVFaces(nFaceNum);

	for (int i = 0; i < nFaceNum; i++)
	{
		CMdxFace *pFace = pChunk->GetFace(i);
		if (pFace)
		{
			int x = pFace->nId[0];
			int y = pFace->nId[1];
			int z = pFace->nId[2];
			mesh.faces[i].setVerts(x, y, z);
			mesh.faces[i].setSmGroup(1);
			mesh.tvFace[i].setTVerts(x, y, z);
		}
	}
}

void CMdxCandidate::OnImportTVert(CMdxGeoChunk *pChunk, Mesh &mesh)
{
	int nVertNum = pChunk->GetVertexNum();
	mesh.setNumMaps(1);
	mesh.setNumTVerts(nVertNum);

	for (int i = 0; i < nVertNum; i++)
	{
		CMdxUV *pUV = pChunk->GetUV(0, i);
		if (pUV)
		{
			float u = pUV->u;
			float v = 1.0f - pUV->v;
			mesh.setTVert(i, u, v, 0);
		}
	}
}

void CMdxCandidate::OnImportVertex(CMdxGeoChunk *pChunk, Mesh &mesh)
{
	int nVertNum = pChunk->GetVertexNum();
	mesh.setNumVerts(nVertNum);

	for (int i = 0; i < nVertNum; i++)
	{
		CVector *pVertex = pChunk->GetVertex(i);
		if (pVertex)
		{
			float x = pVertex->x;
			float y = pVertex->y;
			float z = pVertex->z;
			mesh.setVert(i, x, y, z);
		}
	}
}