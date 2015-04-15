

#ifndef __OgreRoot_h__
#define __OgreRoot_h__

#include "OgreArchive.h"
#include "OgreResourceManager.h"
#include "OgreDynLib.h"
#include "OgreConfigFile.h"

namespace Ogre
{
	class CRenderSystem;
	class CSceneManager;
	class CLogManager;
	class CCamera;

	class OgreExport CRoot
	{
	public:
		CRoot(char *pluginName = "Plugins.cfg", char* logFileName = "Ogre.log");
		~CRoot();

		static CRoot& Instance();

		CRenderSystem* GetActiveRenderer(){return m_pActiveRenderer;}
		CCamera* GetCamera(){return m_pCamera;}
		void RenderOneFrame();

		void LoadPlugins(CString szPluginFile);
		void UnLoadPlugins();
		void LoadPlugin(CString szPluginName);
		void UnLoadPlugin(CPlugin* plugin);
		void InstallPlugin(CPlugin *pPlugin);
		void UnInstallPlugin(CPlugin *pPlugin);
		void InitialisePlugins();
		void ShutDownPlugins();

		void AddRenderSystem(CRenderSystem *s);
		void Initialize(HWND hWnd, int nWidth, int nHeight);
		void Reset(int nWidth, int nHeight);
	private:
		typedef std::vector<CDynLib*> DynLibList;
		typedef std::vector<CPlugin*> PluginList;
		typedef std::vector<CRenderSystem*> RenderSystemList;
		typedef void (*DLL_START_PLUGIN)(void);
		typedef void (*DLL_STOP_PLUGIN)(void);

		static CRoot *s_Instance;
		CFileArchiveFactory *m_pFileArchiveFactory;
		CMpqArchiveFactory *m_pMpqArchiveFactory;
		CZipArchiveFactory *m_pZipArchiveFactory;
		CArchiveManager *m_pArchiveManager;
		CResourceGroupManager *m_pResourceGroupManager;
		CDynLibManager *m_pDynLibManager;
		CSceneManager *m_pSceneManager;
		CMdxMeshManager *m_pMdxMeshManager;
		CLogManager *m_pLogManager;
		CRenderSystem *m_pActiveRenderSystem;

		RenderSystemList m_vecRenderer;
		CRenderSystem *m_pActiveRenderer;
		DynLibList m_vecDynLib;
		PluginList m_vecPlugin;
		CCamera *m_pCamera;
	};
}

#endif