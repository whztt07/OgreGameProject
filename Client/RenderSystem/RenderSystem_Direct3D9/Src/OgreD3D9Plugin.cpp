

#include "OgreD3D9Plugin.h"

namespace Ogre
{
	CD3D9Plugin::CD3D9Plugin()
		:CPlugin()
	{
		m_szName = "D3D9Plugin";
		m_pRenderSystem = NULL;
	}

	CD3D9Plugin::~CD3D9Plugin()
	{

	}

	void CD3D9Plugin::Install()
	{
#if OGRE_DEBUG_MODE == 1
		HINSTANCE hInst = GetModuleHandleA( "RenderSystem_Direct3D9_d.dll" );
#else
		HINSTANCE hInst = GetModuleHandleA( "RenderSystem_Direct3D9.dll" );
#endif

		m_pRenderSystem = new CD3D9RenderSystem;
		CRoot::Instance().AddRenderSystem(m_pRenderSystem);
	}

	void CD3D9Plugin::UnInstall()
	{

	}

	void CD3D9Plugin::Initialise()
	{

	}

	void CD3D9Plugin::ShutDown()
	{
		m_pRenderSystem->Destroy();
		SAFE_DELETE(m_pRenderSystem);
	}
}