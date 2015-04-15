
#ifndef __OgreD3D9RenderSystem_h__
#define __OgreD3D9RenderSystem_h__

#include "OgreD3D9Prerequisites.h"
#include "OgreD3D9Driver.h"
#include "OgreD3D9VideoMode.h"
#include "OgreRenderSystem.h"
#include "OgreRenderSystemCapabilities.h"

namespace Ogre
{
	class CVertexDeclaration;
	class CVertexBufferBinding;
	class CHardwareIndexBuffer;
	struct CDriverVersion;

	struct D3D9ConfigOption
	{
		CString szName;
		CString szCurValue;
		std::vector<CString> possibleValues;
		bool bImmutable;
	};
	typedef std::map<std::string, D3D9ConfigOption> D3D9ConfigOptionMap;

	class OgreExport CD3D9RenderSystem : public CRenderSystem
	{
	public:
		CD3D9RenderSystem();
		~CD3D9RenderSystem();

		virtual void Initialise(HWND hWnd, int nWidth, int nHeight);
		virtual void* GetDevice(){return m_pDevice;}
		virtual void BeginFrame();
		virtual void EndFrame();
		virtual void Render(CRenderOperation *op);
		virtual void SetViewMatrix(CCamera *pCam);
		virtual void SetProjMatrix(CCamera *pCam);
		virtual void SetViewport(int nLeft, int nTop, int nWidth, int nHeight);
		CD3D9DriverList* GetDirect3DDrivers();
		
		void InitConfigOption();
		void RefreshConfigOption();
		void CreateRenderWindow(CString &name, int nWidth, int nHeight);
		void InitDevice(HWND hWnd, int nWidth, int nHeight);
		virtual void Reset(int nWidth, int nHeight);
	private:
		void SetVertexDeclaration(CVertexDeclaration *decl);
		void SetVertexBufferBinding(CVertexBufferBinding *pBinding);
		void SetIndices(CHardwareIndexBuffer *pIndexBuf);

		LPDIRECT3D9 m_pD3D;
		IDirect3DDevice9 *m_pDevice;
		CD3D9DriverList *m_pDriverList;
		CD3D9Driver *m_pActiveDriver;
		D3D9ConfigOptionMap m_mapOption;
		CDriverVersion m_DriverVersion;
		D3DPRESENT_PARAMETERS m_D3Dpp;
		D3DVIEWPORT9 m_Vp;
		bool m_bReset;
	};
}

#endif