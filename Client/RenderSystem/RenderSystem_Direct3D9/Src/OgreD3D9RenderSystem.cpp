

#include "OgreD3D9RenderSystem.h"
#include "OgreD3D9Texture.h"
#include "OgreD3D9HardwareBuffer.h"
#include "OgreCamera.h"
#include "OgreLog.h"

namespace Ogre
{
	CD3D9RenderSystem::CD3D9RenderSystem()
		:CRenderSystem()
	{
		m_pD3D = NULL;
		m_pDevice = NULL;
		m_pDriverList = NULL;
		m_pActiveDriver = NULL;
		m_bReset = false;

		// Create out Direct3D objet
		m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);

		// set config options defaults
		InitConfigOption();
	}

	CD3D9RenderSystem::~CD3D9RenderSystem()
	{

	}

	void CD3D9RenderSystem::InitConfigOption()
	{
		CD3D9DriverList* driverList;
		CD3D9Driver* driver;

		driverList = GetDirect3DDrivers();
		D3D9ConfigOption optDevice;
		D3D9ConfigOption optVideoMode;

		optDevice.szName = "Rendering Device";
		optDevice.possibleValues.clear();
		optDevice.bImmutable = false;

		optVideoMode.szName = "Video Mode";
		optVideoMode.szCurValue = "1024 x 768 @ 32-bit colour";
		optVideoMode.bImmutable = false;

		for( unsigned nDriver = 0; nDriver < driverList->Count(); nDriver++ )
		{
			driver = driverList->Item(nDriver);
			optDevice.possibleValues.push_back( driver->DriverDescription() );
			// Make first one default
			if( nDriver == 0 )
				optDevice.szCurValue = driver->DriverDescription();
		}

		m_mapOption[optDevice.szName] = optDevice;
		m_mapOption[optVideoMode.szName] = optVideoMode;

		RefreshConfigOption();
	}

	void CD3D9RenderSystem::RefreshConfigOption()
	{
		D3D9ConfigOption* optVideoMode = NULL;
		CD3D9Driver* pDriver = NULL;
		CD3D9VideoMode *pVideoMode = NULL;

		D3D9ConfigOptionMap::iterator opt = m_mapOption.find( "Rendering Device" );
		if( opt != m_mapOption.end() )
		{
			int nNumDriver = GetDirect3DDrivers()->Count();
			for( unsigned nDriver = 0; nDriver < nNumDriver; nDriver++ )
			{
				pDriver = GetDirect3DDrivers()->Item(nDriver);

				if( !stricmp(pDriver->DriverDescription(), opt->second.szCurValue.c_str()) )
				{
					break;
				}
			}

			if (pDriver)
			{
				opt = m_mapOption.find("Video Mode");
				optVideoMode = &opt->second;
				optVideoMode->possibleValues.clear();

				//Get video mode for this device...
				CD3D9VideoModeList *pVideoList = pDriver->GetD3D9VideoModeList();
				int nVideoNum = pVideoList->Count();
				for (int nVideoMode = 0; nVideoMode < nVideoNum; nVideoMode++)
				{
					pVideoMode = pVideoList->Item(nVideoMode);
					optVideoMode->possibleValues.push_back(pVideoMode->GetDescription());
				}
			}
		}
	}

	CD3D9DriverList* CD3D9RenderSystem::GetDirect3DDrivers()
	{
		if ( (m_pDriverList == NULL) && m_pD3D )
		{
			m_pDriverList = new CD3D9DriverList(m_pD3D);
			m_pDriverList->Enumerate();
		}

		return m_pDriverList;
	}

	void CD3D9RenderSystem::Initialise(HWND hWnd, int nWidth, int nHeight)
	{
		// Init using current settings
		m_pActiveDriver = NULL;
		D3D9ConfigOptionMap::iterator opt = m_mapOption.find( "Rendering Device" );
		for( int nDriver = 0; nDriver < GetDirect3DDrivers()->Count(); nDriver++ )
		{
			CD3D9Driver *pDriver = GetDirect3DDrivers()->Item(nDriver);
			if (pDriver)
			{
				if( !stricmp(pDriver->DriverDescription(), opt->second.szCurValue.c_str()) )
				{
					m_pActiveDriver = GetDirect3DDrivers()->Item(nDriver);
					break;
				}
			}
		}

		// get driver version
		m_DriverVersion.m_nMajor = HIWORD(m_pActiveDriver->GetAdapterIdentifier().DriverVersion.HighPart);
		m_DriverVersion.m_nMinor = LOWORD(m_pActiveDriver->GetAdapterIdentifier().DriverVersion.HighPart);
		m_DriverVersion.m_nRelease = HIWORD(m_pActiveDriver->GetAdapterIdentifier().DriverVersion.LowPart);
		m_DriverVersion.m_nBuild = LOWORD(m_pActiveDriver->GetAdapterIdentifier().DriverVersion.LowPart);
	
		InitDevice(hWnd, nWidth, nHeight);
		SetViewport(0, 0, nWidth, nHeight);
	
		m_pTextureManager = new CD3D9TextureManager(m_pDevice);
		m_pHardwareBufferManager = new CD3D9HardwareBufferManager(m_pDevice);
		m_pGpuProgramManager = new CD3D9GpuProgramManager(m_pDevice);
	}

	void CD3D9RenderSystem::Reset(int nWidth, int nHeight)
	{
		if (m_pDevice != NULL)
		{
			m_bReset = true;
			m_D3Dpp.BackBufferWidth = nWidth;
			m_D3Dpp.BackBufferHeight = nHeight;
		}
	}

	void CD3D9RenderSystem::CreateRenderWindow(CString &name, int nWidth, int nHeight)
	{

	}

	void CD3D9RenderSystem::InitDevice(HWND hWnd, int nWidth, int nHeight)
	{
		D3DPRESENT_PARAMETERS param;
		ZeroMemory( &param, sizeof(param) );
		param.Windowed = TRUE;
		param.SwapEffect = D3DSWAPEFFECT_DISCARD;
		param.BackBufferFormat = D3DFMT_X8R8G8B8;
		param.BackBufferWidth = nWidth;
		param.BackBufferHeight = nHeight;
		param.hDeviceWindow = hWnd;
		param.BackBufferCount = 1;
		param.MultiSampleType = D3DMULTISAMPLE_NONE;
		param.MultiSampleQuality = 0;
		param.EnableAutoDepthStencil = TRUE;
		param.AutoDepthStencilFormat = D3DFMT_D24S8;
		param.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		//m_D3Dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; 用这个移动的时候屏幕上会有条线
		// 而且帧率足够的情况下也只有60帧
		param.FullScreen_RefreshRateInHz = 0;
		m_D3Dpp = param;

		DWORD BehaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

		OGRE_LOG("开始创建device", LML_NORMAL);

		// Hardware Device
		HRESULT hr = m_pD3D->CreateDevice(
			D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
			BehaviorFlags, &param, &m_pDevice );

		OGRE_LOG("结束创建device", LML_NORMAL);
	}

	void CD3D9RenderSystem::BeginFrame()
	{
		if (m_pDevice == NULL) return;

		HRESULT hr;
		if (FAILED(hr = m_pDevice->BeginScene()))
		{
			CString msg = DXGetErrorDescriptionA(hr);
			OGRE_EXCEPT(CException::ERR_RENDERINGAPI_ERROR, "Error beginning frame :" + msg, "D3D9RenderSystem::_beginFrame" );
		}

		m_pDevice->Clear( 0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_COLORVALUE(0.5f, 0.5f, 0.5f, 0.0f), 1.0f, 0);
		m_pDevice->SetViewport(&m_Vp);

		m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		m_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
		m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	}

	void CD3D9RenderSystem::EndFrame()
	{
		if (m_pDevice == NULL) return;

		HRESULT hr;
		if( FAILED( hr = m_pDevice->EndScene() ) )
		{
			OGRE_EXCEPT(CException::ERR_RENDERINGAPI_ERROR, "Error ending frame", "D3D9RenderSystem::_endFrame" );
		}

		m_pDevice->Present(0, 0, 0, 0);

		if (m_bReset)
		{
			m_pGpuProgramManager->OnLostDevice();
			m_pTextureManager->OnLostDevice();
			m_pHardwareBufferManager->OnLostDevice();
			m_pDevice->Reset(&m_D3Dpp);
			m_pGpuProgramManager->OnResetDevice();
			m_pTextureManager->OnLostDevice();
			m_pHardwareBufferManager->OnResetDevice();
			m_bReset = false;
		}
	}

	void CD3D9RenderSystem::Render(CRenderOperation *op)
	{
		if (m_pDevice == NULL) return;

		int primCount = 0;
		D3DPRIMITIVETYPE primType;
		switch (op->m_OpType)
		{
		case CRenderOperation::OT_TRIANGLE_LIST:
			{
				primType = D3DPT_TRIANGLELIST;
				primCount = ( op->m_bUseIndex ? op->m_pIndexData->GetIndexCount() : op->m_pVertexData->GetVertexCount() ) / 3;
				break;
			}
			break;
		}

		SetVertexDeclaration(op->m_pVertexData->GetVertexDeclaration());
		SetVertexBufferBinding(op->m_pVertexData->GetVertexBufferBinding());
		if (op->m_bUseIndex)
		{
			SetIndices(op->m_pIndexData->GetIndexBuffer());

			m_pDevice->DrawIndexedPrimitive(
					primType,
					op->m_pVertexData->GetVertexStart(),
					0,
					op->m_pVertexData->GetVertexCount(),
					op->m_pIndexData->GetIndexStart(),
					primCount);
		}
	}

	void CD3D9RenderSystem::SetVertexDeclaration(CVertexDeclaration *decl)
	{
		CD3D9VertexDeclaration *pD3DDecl = (CD3D9VertexDeclaration*)decl;
		m_pDevice->SetVertexDeclaration(pD3DDecl->GetD3DVertexDeclaration());
	}

	void CD3D9RenderSystem::SetVertexBufferBinding(CVertexBufferBinding *pBinding)
	{
		CVertexBufferBinding::VertexBufMap &bind = pBinding->GetBindingMap();
		CVertexBufferBinding::VertexBufMap::iterator iter = bind.begin();
		for (; iter != bind.end(); iter++)
		{
			int nStream = iter->first;
			CD3D9HardwareVertexBuffer *pBuf = (CD3D9HardwareVertexBuffer*)iter->second;
			IDirect3DVertexBuffer9 *pVertexBuf = pBuf->GetD3D9VertexBuffer();
			int nVertexSize = pBuf->GetVertexSize();
			m_pDevice->SetStreamSource(nStream, pVertexBuf, 0, nVertexSize);
		}
	}

	void CD3D9RenderSystem::SetIndices(CHardwareIndexBuffer *pIndexBuf)
	{
		CD3D9HardwareIndexBuffer *pD3DIndexBuf = (CD3D9HardwareIndexBuffer*)pIndexBuf;
		m_pDevice->SetIndices(pD3DIndexBuf->GetD3DIndexBuffer());
	}

	void CD3D9RenderSystem::SetViewMatrix(CCamera *pCam)
	{
		if (m_pDevice == NULL) return;

		D3DXMATRIX mView;
		D3DXMatrixLookAtLH(&mView, 
			(D3DXVECTOR3*)&pCam->GetEye(),
			(D3DXVECTOR3*)&pCam->GetLookat(),
			&D3DXVECTOR3(0.0f, 1.0f, 0.0f));

		pCam->SetViewMatrix((CMatrix*)&mView);

		HRESULT hr;
		if( FAILED( hr = m_pDevice->SetTransform( D3DTS_VIEW, &mView ) ) )
		{
			OGRE_EXCEPT(CException::ERR_RENDERINGAPI_ERROR,
				"Cannot set D3D9 view matrix", "D3D9RenderSystem::_setViewMatrix" );
		}
	}

	void CD3D9RenderSystem::SetProjMatrix(CCamera *pCam)
	{
		if (m_pDevice == NULL) return;

		D3DXMATRIX mProj;
		D3DXMatrixPerspectiveFovLH(&mProj, 
			pCam->GetProjFovy(), pCam->GetProjAspect(),
			pCam->GetProjNearDist(), pCam->GetProjFarDist());

		pCam->SetProjMatrix((CMatrix*)&mProj);

		HRESULT hr;
		if( FAILED( hr = m_pDevice->SetTransform( D3DTS_PROJECTION, &mProj ) ) )
		{
			OGRE_EXCEPT(CException::ERR_RENDERINGAPI_ERROR, 
				"Cannot set D3D9 projection matrix", "D3D9RenderSystem::_setProjectionMatrix" );
		}
	}

	void CD3D9RenderSystem::SetViewport(int nLeft, int nTop, int nWidth, int nHeight)
	{
		if (m_pDevice != NULL)
		{
			m_Vp.X = nLeft;
			m_Vp.Y = nTop;
			m_Vp.Width = nWidth;
			m_Vp.Height = nHeight;
			m_Vp.MinZ = 0.0f;
			m_Vp.MaxZ = 1.0f;
		}
	}
}