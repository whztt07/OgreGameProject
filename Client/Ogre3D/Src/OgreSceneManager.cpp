

#include "OgreSceneManager.h"
#include "OgreResourceManager.h"
#include "OgreMdxMesh.h"
#include "OgreMdxEntity.h"
#include "OgreHardwareBuffer.h"
#include "OgreRoot.h"
#include "OgreCamera.h"

namespace Ogre
{
	CSceneManager* CSceneManager::s_Instance = NULL;

	CSceneManager::CSceneManager()
	{
		s_Instance = this;
		m_vecEntity.clear();
	}

	CSceneManager::~CSceneManager()
	{

	}

	CSceneManager& CSceneManager::Instance()
	{
		return *s_Instance;
	}

	CMdxEntity* CSceneManager::CreateEntity(CString szEntityName, CString szMeshName)
	{
		CResource *pResource = CMdxMeshManager::Instance().Load(szMeshName, "Auto");
		CMdxMesh *pMesh = (CMdxMesh*)pResource;
		CMdxEntity *pEntity = new CMdxEntity(szEntityName, pMesh);
		m_vecEntity.push_back(pEntity);
		return pEntity;
	}

	void CSceneManager::ClearEntity()
	{
		int nNum = m_vecEntity.size();
		for (int i = 0; i < nNum; i++)
		{
			CMdxEntity *pEntity = m_vecEntity[i];
			if (pEntity != NULL)
			{
				pEntity->Destroy();
				SAFE_DELETE(pEntity);
			}
		}

		m_vecEntity.clear();
	}

	void CSceneManager::RenderScene()
	{
		CHighLevelGpuProgram *pHLSLProgram = CGpuProgramManager::Instance().GetHLSLProgram("./Resources/StaticMesh.fx");
		if (pHLSLProgram == NULL) return;

		CCamera *pCamera = CRoot::Instance().GetCamera();
		if (pCamera == NULL) return;

		CMatrix mViewProj = pCamera->GetViewMatrix() * pCamera->GetProjMatrix();
		pHLSLProgram->SetMatrix("mViewProj", &mViewProj);

		pHLSLProgram->BeginRender();
		int nNum = m_vecEntity.size();
		for (int i = 0; i < nNum; i++)
		{
			CMdxEntity *pEntity = m_vecEntity[i];
			if (pEntity != NULL)
			{
				pHLSLProgram->SetMatrix("mWorld", &pEntity->GetWorldMatrix());

				pEntity->Update(0.0f);
				pEntity->Render();
			}
		}
		pHLSLProgram->EndRender();
	}
}