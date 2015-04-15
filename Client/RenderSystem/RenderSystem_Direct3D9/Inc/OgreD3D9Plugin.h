

#ifndef __OgreD3D9Plugin_h__
#define __OgreD3D9Plugin_h__

#include "OgreDynLib.h"
#include "OgreD3D9RenderSystem.h"

namespace Ogre
{
	class OgreExport CD3D9Plugin : public CPlugin
	{
	public:
		CD3D9Plugin();
		~CD3D9Plugin();

		virtual void Install();
		virtual void UnInstall();
		virtual void Initialise();
		virtual void ShutDown();
	private:
		CD3D9RenderSystem *m_pRenderSystem;
	};

	CD3D9Plugin *g_pPlugin = NULL;
	extern "C" void OgreExport dllStartPlugin() throw()
	{
		g_pPlugin = new CD3D9Plugin();
		CRoot::Instance().InstallPlugin(g_pPlugin);
	}

	extern "C" void OgreExport dllStopPlugin()
	{
		CRoot::Instance().UnInstallPlugin(g_pPlugin);
		SAFE_DELETE(g_pPlugin);
	}
}

#endif