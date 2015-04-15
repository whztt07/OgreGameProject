

#include "OgreD3D9HardwareBuffer.h"
#include "OgreD3D9Mapping.h"

namespace Ogre
{
	//--------------------------
	// CD3D9HardwareVertexBuffer
	//--------------------------
	CD3D9HardwareVertexBuffer::CD3D9HardwareVertexBuffer(size_t vertexSize, 
		size_t numVertices, CHardwareBuffer::Usage usage, LPDIRECT3DDEVICE9 pDev)
		:CHardwareVertexBuffer(vertexSize, numVertices, usage),
		m_pVertexBuffer(NULL)
	{
		if (pDev == NULL) return;
		//mD3DPool = D3DPOOL_DEFAULT;
		mD3DPool = D3DPOOL_MANAGED;

		HRESULT hr = pDev->CreateVertexBuffer(
			static_cast<UINT>(m_nSizeInBytes), 
			D3D9Mappings::Get(usage),
			0,
			mD3DPool,
			&m_pVertexBuffer,
			NULL);
		
		if (FAILED(hr))
		{
			CString msg = (char*)DXGetErrorDescription(hr);
			OGRE_EXCEPT(CException::ERR_RENDERINGAPI_ERROR, 
				"Cannot create D3D9 vertex buffer: " + msg, 
				"CD3D9HardwareVertexBuffer::CD3D9HardwareVertexBuffer");
		}
	}
	
	CD3D9HardwareVertexBuffer::~CD3D9HardwareVertexBuffer()
	{
		SAFE_RELEASE(m_pVertexBuffer);
	}

	void CD3D9HardwareVertexBuffer::ReadData(size_t offset, size_t length, 
		void* pDest)
	{
		void* pSrc = this->Lock(offset, length, CHardwareBuffer::HBL_READ_ONLY);
		memcpy(pDest, pSrc, length);
		this->Unlock();

	}

	void CD3D9HardwareVertexBuffer::WriteData(size_t offset, size_t length, 
		const void* pSource,
		bool discardWholeBuffer)
	{
		void* pDst = this->Lock(offset, length, 
			discardWholeBuffer ? CHardwareBuffer::HBL_DISCARD : CHardwareBuffer::HBL_NORMAL);
		memcpy(pDst, pSource, length);
		this->Unlock();
	}

	void* CD3D9HardwareVertexBuffer::LockImpl(size_t offset, 
		size_t length, CHardwareBuffer::LockOptions options)
	{
		if (m_pVertexBuffer == NULL) return NULL;

		void* pBuf;
		HRESULT hr = m_pVertexBuffer->Lock(
			static_cast<UINT>(offset), 
			static_cast<UINT>(length), 
			&pBuf,
			D3D9Mappings::Get(options, m_eUsage));

		if (FAILED(hr))
		{
			CString msg = (char*)DXGetErrorDescription(hr);
			OGRE_EXCEPT(CException::ERR_RENDERINGAPI_ERROR, 
				"Cannot lock D3D9 vertex buffer: " + msg, 
				"CD3D9HardwareVertexBuffer::lock");
		}

		return pBuf;
	}

	void CD3D9HardwareVertexBuffer::UnlockImpl(void)
	{
		if (m_pVertexBuffer == NULL) return;
		HRESULT hr = m_pVertexBuffer->Unlock();
	}

	//----------------------------
	// CD3D9HardwareIndexBuffer
	//----------------------------
	CD3D9HardwareIndexBuffer::CD3D9HardwareIndexBuffer(CHardwareIndexBuffer::IndexType idxType, 
		size_t numIndexes, CHardwareBuffer::Usage usage, LPDIRECT3DDEVICE9 pDev)
		: CHardwareIndexBuffer(idxType, numIndexes, usage),
		m_pIndexBuffer(NULL)
	{
		if (pDev == NULL) return;

		//mD3DPool = D3DPOOL_DEFAULT;
		mD3DPool = D3DPOOL_MANAGED;
		
		// Create the Index buffer
		HRESULT hr = pDev->CreateIndexBuffer(
			static_cast<UINT>(m_nSizeInBytes),
			D3D9Mappings::Get(m_eUsage),
			D3D9Mappings::Get(m_IndexType),
			mD3DPool,
			&m_pIndexBuffer,
			NULL
			);

		if (FAILED(hr))
		{
			CString msg = (char*)DXGetErrorDescription(hr);
			OGRE_EXCEPT(CException::ERR_RENDERINGAPI_ERROR, 
				"Cannot create D3D9 Index buffer: " + msg, 
				"CD3D9HardwareIndexBuffer::CD3D9HardwareIndexBuffer");
		}
	}

	CD3D9HardwareIndexBuffer::~CD3D9HardwareIndexBuffer()
	{
		SAFE_RELEASE(m_pIndexBuffer);
	}

	void CD3D9HardwareIndexBuffer::ReadData(size_t offset, size_t length, 
		void* pDest)
	{
		void* pSrc = this->Lock(offset, length, CHardwareBuffer::HBL_READ_ONLY);
		memcpy(pDest, pSrc, length);
		this->Unlock();
	}

	void CD3D9HardwareIndexBuffer::WriteData(size_t offset, size_t length, 
		const void* pSource,
		bool discardWholeBuffer)
	{
		void* pDst = this->Lock(offset, length, 
			discardWholeBuffer ? CHardwareBuffer::HBL_DISCARD : CHardwareBuffer::HBL_NORMAL);
		memcpy(pDst, pSource, length);
		this->Unlock();   
	}

	void* CD3D9HardwareIndexBuffer::LockImpl(size_t offset, 
		size_t length, LockOptions options)
	{
		if (m_pIndexBuffer == NULL) return NULL;
		void* pBuf;
		HRESULT hr = m_pIndexBuffer->Lock(
			static_cast<UINT>(offset), 
			static_cast<UINT>(length), 
			&pBuf,
			D3D9Mappings::Get(options, m_eUsage));

		if (FAILED(hr))
		{
			CString msg = (char*)DXGetErrorDescription(hr);
			OGRE_EXCEPT(CException::ERR_RENDERINGAPI_ERROR, 
				"Cannot lock D3D9 Index buffer: " + msg, 
				"CD3D9HardwareIndexBuffer::lock");
		}

		return pBuf;
	}

	void CD3D9HardwareIndexBuffer::UnlockImpl(void)
	{
		if (m_pIndexBuffer == NULL) return;
		HRESULT hr = m_pIndexBuffer->Unlock();
	}

	//-------------------------------
	// CD3D9HardwareBufferManager
	//-------------------------------
	CD3D9HardwareBufferManager::CD3D9HardwareBufferManager(IDirect3DDevice9 *pDevice)
		:CHardwareBufferManager()
	{
		m_pDevice = pDevice;
	}

	CD3D9HardwareBufferManager::~CD3D9HardwareBufferManager()
	{

	}

	CHardwareVertexBuffer* CD3D9HardwareBufferManager::CreateVertexBuffer(int nVertSize,
								int nVertNum, CHardwareBuffer::Usage nUsage)
	{
		CD3D9HardwareVertexBuffer *pBuf = new CD3D9HardwareVertexBuffer(nVertSize, nVertNum,
			nUsage, m_pDevice);
		if (pBuf != NULL)
		{
			m_vecVertexBuf.push_back(pBuf);
		}

		return pBuf;
	}

	CHardwareIndexBuffer* CD3D9HardwareBufferManager::CreateIndexBuffer(int nIndexNum, 
							CHardwareIndexBuffer::IndexType nType, CHardwareBuffer::Usage nUsage)
	{
		CD3D9HardwareIndexBuffer *pBuf = new CD3D9HardwareIndexBuffer(nType, nIndexNum, nUsage, m_pDevice);
		if (pBuf != NULL)
		{
			m_vecIndexBuf.push_back(pBuf);
		}

		return pBuf;
	}

	CVertexDeclaration* CD3D9HardwareBufferManager::CreateVertexDeclaration()
	{
		CD3D9VertexDeclaration *pDecl = new CD3D9VertexDeclaration(m_pDevice);
		if (pDecl != NULL)
		{
			m_vecVertexDecl.push_back(pDecl);
		}

		return pDecl;
	}

	void CD3D9HardwareBufferManager::OnLostDevice()
	{

	}

	void CD3D9HardwareBufferManager::OnResetDevice()
	{

	}

	//-------------------------------
	// CD3D9VertexDeclaration
	//-------------------------------
	CD3D9VertexDeclaration::CD3D9VertexDeclaration(LPDIRECT3DDEVICE9 device)
		:CVertexDeclaration()
	{
		m_bCache = true;
		m_pDevice = device;
		m_pD3DDecl = NULL;
	}

	CD3D9VertexDeclaration::~CD3D9VertexDeclaration()
	{

	}

	LPDIRECT3DVERTEXDECLARATION9 CD3D9VertexDeclaration::GetD3DVertexDeclaration()
	{
		if (m_bCache)
		{
			SAFE_RELEASE(m_pD3DDecl);

			D3DVERTEXELEMENT9 *d3delem = new D3DVERTEXELEMENT9[m_vecElement.size() + 1];
			int nNum = GetElementCount();
			for (int i = 0; i < nNum; i++)
			{
				CVertexElement *pElement = GetElement(i);
				if (pElement == NULL)
				{
					continue;
				}

				d3delem[i].Method = D3DDECLMETHOD_DEFAULT;
				d3delem[i].Offset = pElement->GetOffset();
				d3delem[i].Stream = pElement->GetSource();
				d3delem[i].Type = D3D9Mappings::Get(pElement->GetType());
				d3delem[i].Usage = D3D9Mappings::Get(pElement->GetSemantic());

				if (pElement->GetSemantic() == VES_SPECULAR)
				{
					d3delem[i].UsageIndex = 1;
				}
				else if (pElement->GetSemantic() == VES_DIFFUSE)
				{
					d3delem[i].UsageIndex = 0;
				}
				else
				{
					d3delem[i].UsageIndex = pElement->GetIndex();
				}
			}

			// add terminator
			d3delem[nNum].Stream = 0xff;
			d3delem[nNum].Offset = 0;
			d3delem[nNum].Type = D3DDECLTYPE_UNUSED;
			d3delem[nNum].Method = 0;
			d3delem[nNum].Usage = 0;
			d3delem[nNum].UsageIndex = 0;

			HRESULT hr = m_pDevice->CreateVertexDeclaration(d3delem, &m_pD3DDecl);
			if (FAILED(hr))
			{
				OGRE_EXCEPT(CException::ERR_INTERNAL_ERROR, 
					"Cannot create D3D9 vertex declaration:",
					"Direct3D9VertexDeclaration::getD3DVertexDeclaration");
			}

			SAFE_DELETE_ARRAY(d3delem);
			m_bCache = false;
		}

		return m_pD3DDecl;
	}

	//-------------------------------
	// CD3D9HLSLProgram
	//-------------------------------
	CD3D9HLSLProgram::CD3D9HLSLProgram(IDirect3DDevice9 *pDevice, CString szGroup, CString szName)
		:CHighLevelGpuProgram(szGroup, szName)
	{
		m_pDevice = pDevice;
		m_pEffect = NULL;
	}

	CD3D9HLSLProgram::~CD3D9HLSLProgram()
	{

	}

	bool CD3D9HLSLProgram::LoadImpl()
	{
		LPD3DXBUFFER pError = NULL;
		D3DXCreateEffectFromFileA(m_pDevice, m_szName.c_str(), NULL, NULL,
			D3DXSHADER_DEBUG, 0, &m_pEffect, &pError);
		if (pError)
		{
			char *name = (char*)pError->GetBufferPointer();
		}

		return true;
	}

	void CD3D9HLSLProgram::BeginRender()
	{
		if (m_pEffect == NULL) return;
		m_pEffect->SetTechnique("CommonTech");
		UINT nPassNum;
		m_pEffect->Begin(&nPassNum, 0);
		m_pEffect->BeginPass(0);
	}

	void CD3D9HLSLProgram::EndRender()
	{
		if (m_pEffect == NULL) return;
		m_pEffect->EndPass();
		m_pEffect->End();
	}

	void CD3D9HLSLProgram::SetMatrix(std::string segment, CMatrix *pMatrix)
	{
		if (m_pEffect == NULL) return;
		m_pEffect->SetMatrix(segment.c_str(), (D3DXMATRIX*)pMatrix);
	}

	void CD3D9HLSLProgram::SetTexture(std::string segment, CTexture *pTexture)
	{
		if (m_pEffect == NULL) return;
		IDirect3DTexture9 *pD3D9Tex = (IDirect3DTexture9*)pTexture->GetRealTex();
		m_pEffect->SetTexture(segment.c_str(), pD3D9Tex);
	}

	void CD3D9HLSLProgram::Commit()
	{
		if (m_pEffect == NULL) return;
		m_pEffect->CommitChanges();
	}

	//-------------------------------
	// CD3D9GpuProgramManager
	//-------------------------------
	CD3D9GpuProgramManager::CD3D9GpuProgramManager(IDirect3DDevice9 *pDevice)
		:CGpuProgramManager()
	{
		m_pDevice = pDevice;
		m_vecHLSLProgram.clear();
		m_mapHLSLProgram.clear();
	}

	CD3D9GpuProgramManager::~CD3D9GpuProgramManager()
	{

	}

	CResource* CD3D9GpuProgramManager::Load(CString szGroup, CString szName)
	{
		CResource *pResource = Create(szName, szGroup);
		pResource->Load();
		return pResource;
	}

	CHighLevelGpuProgram* CD3D9GpuProgramManager::GetHLSLProgram(std::string szName)
	{
		HLSLProgramMap::iterator iter = m_mapHLSLProgram.find(szName);
		if (iter != m_mapHLSLProgram.end())
		{
			return iter->second;
		}

		return NULL;
	}

	CResource* CD3D9GpuProgramManager::CreateImpl(CString szGroup, CString szName)
	{
		HLSLProgramMap::iterator iter = m_mapHLSLProgram.find(szName);
		if (iter != m_mapHLSLProgram.end())
		{
			return iter->second;
		}

		CD3D9HLSLProgram *pProgram = new CD3D9HLSLProgram(m_pDevice, szGroup, szName);
		m_vecHLSLProgram.push_back(pProgram);
		m_mapHLSLProgram[szName] = pProgram;
		return pProgram;
	}

	void CD3D9GpuProgramManager::OnLostDevice()
	{
		int nNum = m_vecHLSLProgram.size();
		for (int i = 0; i < nNum; i++)
		{
			CD3D9HLSLProgram *pProgram = m_vecHLSLProgram[i];
			if (pProgram == NULL)
			{
				continue;
			}

			ID3DXEffect *pEffect = pProgram->GetD3DXEffect();
			if (pEffect == NULL)
			{
				continue;
			}

			pEffect->OnLostDevice();
		}
	}

	void CD3D9GpuProgramManager::OnResetDevice()
	{
		int nNum = m_vecHLSLProgram.size();
		for (int i = 0; i < nNum; i++)
		{
			CD3D9HLSLProgram *pProgram = m_vecHLSLProgram[i];
			if (pProgram == NULL)
			{
				continue;
			}

			ID3DXEffect *pEffect = pProgram->GetD3DXEffect();
			if (pEffect == NULL)
			{
				continue;
			}

			pEffect->OnResetDevice();
		}
	}
}
