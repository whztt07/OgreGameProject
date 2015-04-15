

#ifndef __OgreD3D9HardwareBuffer_h__
#define __OgreD3D9HardwareBuffer_h__

#include "OgreD3D9Prerequisites.h"
#include "OgreHardwareBuffer.h"
#include "OgreException.h"
#include "OgreD3D9Mapping.h"
#include "OgreUseful.h"
#include "OgreRenderPass.h"
#include "OgreResourceManager.h"

namespace Ogre
{
	class CD3D9HardwareVertexBuffer : public CHardwareVertexBuffer
	{
	public:
		CD3D9HardwareVertexBuffer(size_t vertexSize, size_t numVertices, 
			CHardwareBuffer::Usage usage, LPDIRECT3DDEVICE9 pDev);
		~CD3D9HardwareVertexBuffer();
		
		void ReadData(size_t offset, size_t length, void* pDest);
		void WriteData(size_t offset, size_t length, const void* pSource,
			bool discardWholeBuffer = false);

		LPDIRECT3DVERTEXBUFFER9 GetD3D9VertexBuffer(void) const { return m_pVertexBuffer; }
	protected:	
		void* LockImpl(size_t offset, size_t length, CHardwareBuffer::LockOptions options);
		void UnlockImpl(void);

		LPDIRECT3DVERTEXBUFFER9 m_pVertexBuffer;
		D3DPOOL mD3DPool;
	};

	class CD3D9HardwareIndexBuffer : public CHardwareIndexBuffer
	{
	public:
		CD3D9HardwareIndexBuffer(IndexType idxType, size_t numIndexes, CHardwareBuffer::Usage usage, LPDIRECT3DDEVICE9 pDev);
		~CD3D9HardwareIndexBuffer();

		void ReadData(size_t offset, size_t length, void* pDest);
		void WriteData(size_t offset, size_t length, const void* pSource,
			bool discardWholeBuffer = false);

		LPDIRECT3DINDEXBUFFER9 GetD3DIndexBuffer(void) { return m_pIndexBuffer; }
	protected:	
		void* LockImpl(size_t offset, size_t length, LockOptions options);
		void UnlockImpl(void);

		LPDIRECT3DINDEXBUFFER9 m_pIndexBuffer;
		D3DPOOL mD3DPool;
	};

	class OgreExport CD3D9HardwareBufferManager : public CHardwareBufferManager
	{
	public:
		CD3D9HardwareBufferManager(IDirect3DDevice9 *pDevice);
		~CD3D9HardwareBufferManager();

		virtual CHardwareVertexBuffer* CreateVertexBuffer(int nVertSize, int nVertNum, CHardwareBuffer::Usage nUsage);
		virtual CHardwareIndexBuffer* CreateIndexBuffer(int nIndexNum, CHardwareIndexBuffer::IndexType nType, CHardwareBuffer::Usage nUsage);
		virtual CVertexDeclaration* CreateVertexDeclaration();
		virtual void OnLostDevice();
		virtual void OnResetDevice();
	private:
		IDirect3DDevice9 *m_pDevice;
	};

	class OgreExport CD3D9VertexDeclaration : public CVertexDeclaration
	{
	public:
		CD3D9VertexDeclaration(LPDIRECT3DDEVICE9 device);
		~CD3D9VertexDeclaration();

		LPDIRECT3DVERTEXDECLARATION9 GetD3DVertexDeclaration();
	private:
		bool m_bCache;
		IDirect3DDevice9 *m_pDevice;
		LPDIRECT3DVERTEXDECLARATION9 m_pD3DDecl;
	};

	class OgreExport CD3D9HLSLProgram : public CHighLevelGpuProgram
	{
	public:
		CD3D9HLSLProgram(IDirect3DDevice9 *pDevice, CString szGroup, CString szName);
		~CD3D9HLSLProgram();
	
		virtual void BeginRender();
		virtual void EndRender();
		virtual void SetMatrix(std::string segment, CMatrix *pMatrix);
		virtual void SetTexture(std::string segment, CTexture *pTexture);
		virtual void Commit();
		ID3DXEffect* GetD3DXEffect(){return m_pEffect;}
	private:
		virtual bool LoadImpl();
		
		IDirect3DDevice9 *m_pDevice;
		ID3DXEffect *m_pEffect;
	};

	class OgreExport CD3D9GpuProgramManager : public CGpuProgramManager
	{
	public:
		CD3D9GpuProgramManager(IDirect3DDevice9 *pDevice);
		~CD3D9GpuProgramManager();

		virtual CResource* Load(CString szGroup, CString szName);
		virtual CHighLevelGpuProgram* GetHLSLProgram(std::string szName);

		virtual void OnLostDevice();
		virtual void OnResetDevice();
	private:
		virtual CResource* CreateImpl(CString szGroup, CString szName);
		IDirect3DDevice9 *m_pDevice;
		typedef std::vector<CD3D9HLSLProgram*> HLSLProgramList;
		typedef std::map<std::string, CD3D9HLSLProgram*> HLSLProgramMap;
		HLSLProgramList m_vecHLSLProgram;
		HLSLProgramMap m_mapHLSLProgram;
	};
}

#endif