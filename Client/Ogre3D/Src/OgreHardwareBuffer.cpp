
#include "OgreHardwareBuffer.h"
#include "OgreRenderPass.h"

namespace Ogre
{
	//----------------------
	// Global Param
	//----------------------
	CHardwareBufferManager* CHardwareBufferManager::s_Instance = NULL;

	//----------------------
	// CHardwareBuffer
	//----------------------
	CHardwareBuffer::CHardwareBuffer(Usage usage) 
	{
		m_eUsage = usage;
	}

	CHardwareBuffer::~CHardwareBuffer()
	{
	}

	void* CHardwareBuffer::Lock(size_t offset, size_t length, LockOptions options)
	{
		void* ret = LockImpl(offset, length, options);
		return ret;
	}

	void* CHardwareBuffer::Lock(LockOptions options)
	{
		return this->Lock(0, m_nSizeInBytes, options);
	}

	void CHardwareBuffer::Unlock(void)
	{
		UnlockImpl();
	}

	void* CHardwareBuffer::LockImpl(size_t offset, size_t length, LockOptions options)
	{
		return NULL;
	}

	void CHardwareBuffer::UnlockImpl(void)
	{
	}

	//-------------------------
	// CHardwareVertexBuffer
	//-------------------------
	CHardwareVertexBuffer::CHardwareVertexBuffer(size_t vertexSize, size_t numVertices, CHardwareBuffer::Usage usage) 
		: CHardwareBuffer(usage)
	{
		m_nNumVertices = numVertices;
		m_nVertexSize = vertexSize;
		m_nSizeInBytes = vertexSize * numVertices;
	}

	CHardwareVertexBuffer::~CHardwareVertexBuffer()
	{
	}

	size_t CHardwareVertexBuffer::GetVertexSize(void) const 
	{
		return m_nVertexSize;
	}

	size_t CHardwareVertexBuffer::GetNumVertices(void) const 
	{ 
		return m_nNumVertices;
	}

	//--------------------------
	// CHardwareIndexBuffer
	//--------------------------
	CHardwareIndexBuffer::CHardwareIndexBuffer(IndexType idxType, size_t numIndexes, CHardwareBuffer::Usage usage) 
		: CHardwareBuffer(usage)
	{
		m_IndexType = idxType;
		m_nNumIndexes = numIndexes;
		
		// Calculate the size of the indexes
		switch (m_IndexType)
		{
		case IT_16BIT:
			m_nIndexSize = sizeof(unsigned short);
			break;
		case IT_32BIT:
			m_nIndexSize = sizeof(unsigned int);
			break;
		}
		
		m_nSizeInBytes = m_nIndexSize * m_nNumIndexes;
	}

	CHardwareIndexBuffer::~CHardwareIndexBuffer()
	{
	}

	//--------------------------
	// CHardwareBufferManager
	//--------------------------
	CHardwareBufferManager::CHardwareBufferManager()
	{
		s_Instance = this;
		m_vecVertexBuf.clear();
		m_vecIndexBuf.clear();
		m_vecVertexDecl.clear();
		m_vecVertexBufBinding.clear();
	}

	CHardwareBufferManager::~CHardwareBufferManager()
	{

	}

	CHardwareBufferManager& CHardwareBufferManager::Instance()
	{
		return *s_Instance;
	}

	CVertexBufferBinding* CHardwareBufferManager::CreateVertexBufferBinding()
	{
		CVertexBufferBinding *pBinding = new CVertexBufferBinding;
		if (pBinding != NULL)
		{
			m_vecVertexBufBinding.push_back(pBinding);
		}

		return pBinding;
	}

	//--------------------------
	// CGpuProgram
	//--------------------------
	CGpuProgram::CGpuProgram(CString szGroup, CString szName)
		:CResource(szGroup, szName)
	{

	}

	CGpuProgram::~CGpuProgram()
	{

	}

	//--------------------------
	// CHighLevelGpuProgram
	//--------------------------
	CHighLevelGpuProgram::CHighLevelGpuProgram(CString szGroup, CString szName)
		:CGpuProgram(szGroup, szName)
	{

	}

	CHighLevelGpuProgram::~CHighLevelGpuProgram()
	{

	}
}