

#ifndef __OgreRenderSystem_h__
#define __OgreRenderSystem_h__

#include "OgrePrerequisites.h"
#include "OgreMath.h"

namespace Ogre
{
	class CTextureManager;
	class CHardwareBufferManager;
	class CGpuProgramManager;
	class CRenderOperation;
	class CCamera;

	class OgreExport CRenderSystem
	{
	public:
		CRenderSystem();
		virtual ~CRenderSystem();

		static CRenderSystem& Instance();
		virtual void Initialise(HWND hWnd, int nWidth, int nHeight) = 0;
		virtual void Reset(int nWidth, int nHeight) = 0;
		virtual void* GetDevice() = 0;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void Render(CRenderOperation *op) = 0;
		virtual void SetViewMatrix(CCamera *pCam) = 0;
		virtual void SetProjMatrix(CCamera *pCam) = 0;
		virtual void SetViewport(int nLeft, int nTop, int nWidth, int nHeight) = 0;
		void Destroy(){}
	protected:
		static CRenderSystem *s_Instance;
		CTextureManager *m_pTextureManager;
		CGpuProgramManager *m_pGpuProgramManager;
		CHardwareBufferManager *m_pHardwareBufferManager;
	};
}

#endif