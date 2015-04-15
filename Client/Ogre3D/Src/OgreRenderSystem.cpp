

#include "OgreRenderSystem.h"
#include "OgreResourceManager.h"
#include "OgreHardwareBuffer.h"

namespace Ogre
{
	CRenderSystem* CRenderSystem::s_Instance = NULL;

	CRenderSystem::CRenderSystem()
	{
		m_pTextureManager = NULL;
		m_pHardwareBufferManager = NULL;
		m_pGpuProgramManager = NULL;
	}

	CRenderSystem::~CRenderSystem()
	{

	}

	CRenderSystem& CRenderSystem::Instance()
	{
		return *s_Instance;
	}
}