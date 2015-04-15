

#include "OgreRoot.h"
#include "OgreRenderSystem.h"
#include "OgreSceneManager.h"
#include "OgreUseful.h"
#include "OgreException.h"
#include "OgreCamera.h"
#include "OgreLog.h"

namespace Ogre
{
	CRoot* CRoot::s_Instance = NULL;

	CRoot::CRoot(char *pluginName, char* logFileName)
	{
		s_Instance = this;

		m_pLogManager = new CLogManager;
		m_pLogManager->CreateLog(logFileName, true);

		// Archive Manager
		m_pArchiveManager = new CArchiveManager;
		m_pFileArchiveFactory = new CFileArchiveFactory;
		m_pArchiveManager->AddArchiveFactory(m_pFileArchiveFactory);
		m_pMpqArchiveFactory = new CMpqArchiveFactory;
		m_pArchiveManager->AddArchiveFactory(m_pMpqArchiveFactory);
		m_pZipArchiveFactory = new CZipArchiveFactory;
		m_pArchiveManager->AddArchiveFactory(m_pZipArchiveFactory);

		m_pResourceGroupManager = new CResourceGroupManager;
		m_pDynLibManager = new CDynLibManager;
		m_pSceneManager = new CSceneManager;
		m_pMdxMeshManager = new CMdxMeshManager;
		m_pCamera = new CCamera;
		m_pActiveRenderer = NULL;

		LoadPlugins(pluginName);
	}

	CRoot::~CRoot()
	{
		UnLoadPlugins();
		SAFE_DELETE(m_pFileArchiveFactory);
		SAFE_DELETE(m_pMpqArchiveFactory);
		SAFE_DELETE(m_pZipArchiveFactory);
		SAFE_DELETE(m_pArchiveManager);
		
		SAFE_DELETE(m_pResourceGroupManager);
		SAFE_DELETE(m_pDynLibManager);
	}

	CRoot& CRoot::Instance()
	{
		return *s_Instance;
	}

	void CRoot::LoadPlugins(CString szPluginFile)
	{
		//CConfigFile file;
		//file.Load((char*)szPluginFile.c_str());

		//CConfigFile::SettingSectionMap &mapSec = file.GetMap();
		//CConfigFile::SettingSectionMap::iterator iter = mapSec.begin();
		//for (; iter != mapSec.end(); iter++)
		//{
		//	CConfigFile::SettingSection *pSec = iter->second;
		//	if (pSec != NULL)
		//	{
		//		CConfigFile::SettingSection::iterator iterSec = pSec->begin();
		//		for (; iterSec != pSec->end(); iterSec++)
		//		{
		//			LoadPlugin(iterSec->second);
		//		}
		//	}
		//}

		LoadPlugin("RenderSystem_Direct3D9");
	}

	void CRoot::UnLoadPlugins()
	{
		DynLibList::iterator iter = m_vecDynLib.begin();
		for (; iter != m_vecDynLib.end(); iter++)
		{
			DLL_STOP_PLUGIN pFunc = (DLL_STOP_PLUGIN)(*iter)->GetSymbol("dllStopPlugin");
			pFunc();
			CDynLibManager::Instance().UnLoad(*iter);
		}
		m_vecDynLib.clear();
		m_vecPlugin.clear();
	}

	void CRoot::InstallPlugin(CPlugin *pPlugin)
	{
		m_vecPlugin.push_back(pPlugin);
		pPlugin->Install();
		pPlugin->Initialise();
	}

	void CRoot::UnInstallPlugin(CPlugin *pPlugin)
	{
		PluginList::iterator iter = std::find(m_vecPlugin.begin(), m_vecPlugin.end(), pPlugin);
		if (iter != m_vecPlugin.end())
		{
			pPlugin->ShutDown();
			pPlugin->UnInstall();
			m_vecPlugin.erase(iter);
		}
	}

	void CRoot::InitialisePlugins()
	{
		PluginList::iterator iter = m_vecPlugin.begin();
		for (; iter != m_vecPlugin.end(); iter++)
		{
			(*iter)->Initialise();
		}
	}

	void CRoot::ShutDownPlugins()
	{
		PluginList::iterator iter = m_vecPlugin.begin();
		for (; iter != m_vecPlugin.end(); iter++)
		{
			(*iter)->ShutDown();
		}
	}

	void CRoot::LoadPlugin(CString szPluginName)
	{
		CDynLib *pLib = CDynLibManager::Instance().Load(szPluginName);

		if (std::find(m_vecDynLib.begin(), m_vecDynLib.end(), pLib) == m_vecDynLib.end())
		{
			m_vecDynLib.push_back(pLib);

			DLL_START_PLUGIN pFunc = (DLL_START_PLUGIN)pLib->GetSymbol("dllStartPlugin");

			if (NULL == pFunc)
			{
				OGRE_EXCEPT(CException::ERR_ITEM_NOT_FOUND, 
				 "Cannot find symbol dllStartPlugin in library " + szPluginName, "Root::loadPlugin");
			}

			pFunc();
		}
	}

	void CRoot::UnLoadPlugin(CPlugin* plugin)
	{
		PluginList::iterator iter = std::find(m_vecPlugin.begin(), m_vecPlugin.end(), plugin);
		if (iter != m_vecPlugin.end())
		{
			plugin->ShutDown();
			plugin->UnInstall();
			m_vecPlugin.erase(iter);
		}
	}

	void CRoot::AddRenderSystem(CRenderSystem *s)
	{
		m_vecRenderer.push_back(s);

		if (m_pActiveRenderer == NULL)
		{
			m_pActiveRenderer = s;
		}
	}

	void CRoot::Initialize(HWND hWnd, int nWidth, int nHeight)
	{
		if (m_pActiveRenderer != NULL)
		{
			m_pActiveRenderer->Initialise(hWnd, nWidth, nHeight);
		}
	}

	void CRoot::Reset(int nWidth, int nHeight)
	{
		if (m_pActiveRenderer != NULL)
		{
			m_pActiveRenderer->Reset(nWidth, nHeight);
		}
	}

	void CRoot::RenderOneFrame()
	{
		m_pActiveRenderer->SetViewMatrix(m_pCamera);
		m_pActiveRenderer->SetProjMatrix(m_pCamera);
		m_pSceneManager->RenderScene();
	}
}