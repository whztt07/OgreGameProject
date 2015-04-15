

#include "OgreMdxEntity.h"
#include "OgreRoot.h"
#include "OgreRenderSystem.h"
#include "OgreHardwareBuffer.h"

namespace Ogre
{
	//D3DVERTEXELEMENT9 MdxVertexElements[] = 
	//{
	//	{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	//	{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,	  0},
	//	{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	//	D3DDECL_END()
	//};

	//struct sMdxFormat
	//{
	//	CVector vPos;
	//	CVector vNormal;
	//	CVector2D vTex;
	//};

	//--------------------------------
	//-----------CMdxAnim-------------
	//--------------------------------
	//CMdxAnim::CMdxAnim()
	//{
	//	m_pMdx = NULL;
	//	m_bSingleFrame = false;
	//	m_nCurAnimId = NULL;
	//	m_nCurAnimFrame = 0;
	//}

	//CMdxAnim::~CMdxAnim()
	//{

	//}

	//bool CMdxAnim::CreateMdxAnim(char *filename)
	//{
	//	//m_nCurAnimId = 0;

	//	//m_pDevice = pDevice;
	//	//m_pDevice->CreateVertexDeclaration(MdxVertexElements, &m_pVertexDecl);

	//	//LPD3DXBUFFER pError = NULL;
	//	//D3DXCreateEffectFromFileA(m_pDevice, "Resources/StaticMesh.fx", NULL, NULL,
	//	//	D3DXSHADER_DEBUG, 0, &m_pEffect, &pError);
	//	//if (pError)
	//	//{
	//	//	char *name = (char*)pError->GetBufferPointer();
	//	//}

	//	return true;
	//}

	//void CMdxAnim::SetRotate(float yaw, float roll, float pitch)
	//{
	//	m_vRotate.x += pitch;
	//	m_vRotate.z += yaw;
	//}

	//void CMdxAnim::SetCurAnimId(int nId)
	//{
	//	m_bSingleFrame = false;
	//	m_nCurAnimId = nId;
	//}

	//void CMdxAnim::SetCurAnimFrame(int nFrame)
	//{
	//	m_bSingleFrame = true;
	//	m_nCurAnimFrame = nFrame;
	//}

	//void CMdxAnim::SetSingleFrame(bool bSingleFrame)
	//{
	//	m_bSingleFrame = bSingleFrame;
	//}

	//CMdxMesh* CMdxAnim::GetMdxMesh()
	//{
	//	return m_pMdx;
	//}

	//CVector CMdxAnim::GetRotate()
	//{
	//	return m_vRotate;
	//}

	//int CMdxAnim::GetAnimNum()
	//{
	//	CMdxAnimSet *pAnimSet = m_pMdx->GetAnimSet();
	//	if (pAnimSet != NULL)
	//	{
	//		return pAnimSet->GetAnimNum();
	//	}

	//	return 0;
	//}

	//void CMdxAnim::UpdateSubset(int nSubset)
	//{

	//}

	//void CMdxAnim::RenderSubset(int nSubset)
	//{
	////	if (m_pMdx == NULL)
	////	{
	////		return;
	////	}

	////	CMdxGeometry *pGeometry = m_pMdx->GetGeometry();
	////	if (pGeometry == NULL)
	////	{
	////		return;
	////	}

	////	CMdxGeoChunk *pChunk = pGeometry->GetChunk(nSubset);
	////	if (pChunk == NULL)
	////	{
	////		return;
	////	}

	////	CMdxSkeleton *pSkeleton = m_pMdx->GetSkeleton();
	////	if (pChunk->HasBoneInfo())
	////	{
	////		if (pSkeleton == NULL)
	////		{
	////			return;
	////		}
	////	}

	////	int nMaterialID = pChunk->GetMaterialId();
	////	CMdxMaterial *pMaterial = m_pMdx->GetMaterials()->GetMaterial(nMaterialID);
	////	if (pMaterial == NULL)
	////	{
	////		return;
	////	}

	////	if (pMaterial->GetColorId() != -1)
	////	{
	////		CWOWColorGroup *pColorGroup = m_pMdx->GetWOWColorGroup();
	////		if (pColorGroup == NULL)
	////		{
	////			return;
	////		}

	////		float fOpacity = pColorGroup->GetColor(pMaterial->GetColorId(), m_nCurAnimId);
	////		if (fOpacity == 0.0f)
	////		{
	////			return;
	////		}
	////	}
	////	
	////	m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	////	m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	////	m_pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	////	m_pDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	////	m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	////	
	////	int nVertexNum = pChunk->GetVertexNum();
	////	int nIndexNum = pChunk->GetIndexNum();
	////	
	////	if (pChunk->HasBoneInfo())
	////	{
	////		CVector *pVertices = pChunk->GetVertices();
	////		CVector *pNormals = pChunk->GetNormals();
	////		byte *pBoneWeight = pChunk->GetBoneWeight();
	////		byte *pBoneIndex = pChunk->GetBoneIndex();
	////		sMdxFormat *pMdxFormat = NULL;
	////		HRESULT hr = m_pVertexBuffer[nSubset]->Lock(0, 0, (void**)&pMdxFormat, 0);
	////		if (SUCCEEDED(hr))
	////		{
	////			for (int nVertex = 0; nVertex < nVertexNum; nVertex++, pMdxFormat++)
	////			{
	////				Ogre::CVector v, n;
	////				for (int b = 0; b < 4; b++)
	////				{
	////					byte bWeight = pBoneWeight[4 * nVertex + b];
	////					byte bIndex = pBoneIndex[4 * nVertex + b];
	////					if (bWeight > 0)
	////					{
	////						CMatrix mat = pSkeleton->GetMatrix(bIndex);
	////						Ogre::CVector tv = mat * pVertices[nVertex];
	////						Ogre::CVector tn = mat * pNormals[nVertex];
	////						v += tv * ((float)bWeight / 255.0f);
	////						n += tn * ((float)bWeight / 255.0f);
	////					}
	////				}
	////				pMdxFormat->vPos = CVector(-v.x, v.y, v.z);
	////				pMdxFormat->vNormal = CVector(-n.x, n.y, n.z);
	////			}
	////			m_pVertexBuffer[nSubset]->Unlock();
	////		}
	////	}

	////	m_pDevice->SetVertexDeclaration(m_pVertexDecl);
	////	m_pDevice->SetStreamSource(0, m_pVertexBuffer[nSubset], 0, sizeof(sMdxFormat));
	////	m_pDevice->SetIndices(m_pIndexBuffer[nSubset]);
	////	
	////	m_pEffect->SetTexture("tBase", m_pTexture[nSubset]);
	////	m_pEffect->SetFloat("bHasSpecular", pMaterial->HasSpecular());
	////	if (pMaterial->HasSpecular())
	////	{
	////		m_pEffect->SetTexture("tSpecular", m_pTexture[nSubset + 30]);
	////	}
	////	else
	////	{
	////		m_pEffect->SetTexture("tSpecular", NULL);
	////	}
	////	m_pEffect->SetFloat("bAlphaTest", pMaterial->HasAlphaTest());
	////	m_pEffect->CommitChanges();
	////	
	////	m_pDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST,
	////		0,
	////		0,
	////		nVertexNum,
	////		0,
	////		nIndexNum / 3 );
	//}

	CMdxEntity::CMdxEntity(CString szName, CMdxMesh *pMesh)
	{
		m_szName = szName;
		m_pMdxMesh = pMesh;
		m_mWorld.Unit();
	}

	CMdxEntity::~CMdxEntity()
	{

	}

	void CMdxEntity::SetRotate(float yaw, float roll, float pitch)
	{
		m_vRotate.x += pitch;
		m_vRotate.z += yaw;

		CMatrix mYaw, mPitch;
		CMatrix::RotationAxis(&mYaw, &Ogre::CVector(0.0f, 1.0f, 0.0f), m_vRotate.z);
		CMatrix::RotationAxis(&mPitch, &Ogre::CVector(1.0f, 0.0f, 0.0f), m_vRotate.x);
		m_mWorld = mYaw * mPitch;
	}

	void CMdxEntity::Update(float fElapsedTime)
	{
		//m_pMdx->UpdateSkeleton(m_nCurAnimId, m_bSingleFrame, m_nCurAnimFrame);
	}

	void CMdxEntity::Render()
	{
		if (m_pMdxMesh == NULL) return;

		CMdxGeometry *pGeometry = m_pMdxMesh->GetGeometry();
		if (pGeometry == NULL) return;

		int nChunk = pGeometry->GetChunkNum();
		for (int i = 0; i < nChunk; i++)
		{
			RenderSubset(i);
		}
	}

	void CMdxEntity::RenderSubset(int nChunk)
	{
		CMdxGeometry *pGeometry = m_pMdxMesh->GetGeometry();
		if (pGeometry == NULL) return;

		CMdxGeoChunk *pChunk = pGeometry->GetChunk(nChunk);
		if (pChunk == NULL) return;

		int nMaterialID = pChunk->GetMaterialId();
		CMdxMaterial *pMaterial = m_pMdxMesh->GetMaterials()->GetMaterial(nMaterialID);
		if (pMaterial == NULL) return;

		CHighLevelGpuProgram *pHLSLProgram = CGpuProgramManager::Instance().GetHLSLProgram("./Resources/StaticMesh.fx");
		if (pHLSLProgram == NULL) return;

		pHLSLProgram->SetTexture("tBase", pMaterial->GetBaseTexture());
		pHLSLProgram->Commit();

		CRenderOperation *pOp = pChunk->GetRenderOp();
		CRoot::Instance().GetActiveRenderer()->Render(pOp);
	}
}