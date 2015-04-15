

#include "OgreHardwareBuffer.h"

namespace Ogre
{
	//--------------------------------------
	//--------CHardwareBuffer---------------
	//--------------------------------------
	CHardwareBuffer::CHardwareBuffer(Usage usage, bool systemMemory) 
		: m_eUsage(usage), m_bSystemMemory(systemMemory)
	{
	}

	CHardwareBuffer::~CHardwareBuffer()
	{
	}

	void* CHardwareBuffer::Lock(size_t offset, size_t length, LockOptions options)
	{
		void* ret = LockImpl(offset, length, options);
		return ret;
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

	//--------------------------------------
	//--------CHardwareVertexBuffer---------
	//--------------------------------------
	CHardwareVertexBuffer::CHardwareVertexBuffer(size_t vertexSize,  
		size_t numVertices, CHardwareBuffer::Usage usage, bool useSystemMemory) 
		: CHardwareBuffer(usage, useSystemMemory), 
		mNumVertices(numVertices),
		mVertexSize(vertexSize)
	{
		m_nSizeInBytes = mVertexSize * numVertices;
	}

	CHardwareVertexBuffer::~CHardwareVertexBuffer()
	{
		//HardwareBufferManager* mgr = HardwareBufferManager::getSingletonPtr();
		//if (mgr)
		//{
		//	mgr->_notifyVertexBufferDestroyed(this);
		//}
	}

	size_t CHardwareVertexBuffer::GetVertexSize(void) const 
	{
		return mVertexSize;
	}

	size_t CHardwareVertexBuffer::GetNumVertices(void) const 
	{ 
		return mNumVertices;
	}

	//--------------------------------------
	//--------CHardwareIndexBuffer----------
	//--------------------------------------
	//-----------------------------------------------------------------------------
	CHardwareIndexBuffer::CHardwareIndexBuffer(IndexType idxType, 
		size_t numIndexes, CHardwareBuffer::Usage usage, 
		bool useSystemMemory) 
		: CHardwareBuffer(usage, useSystemMemory), 
		m_IndexType(idxType), m_nNumIndexes(numIndexes)
	{
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
		//HardwareBufferManager* mgr = HardwareBufferManager::getSingletonPtr();
		//if (mgr)
		//{
		//	mgr->_notifyIndexBufferDestroyed(this);
		//}
	}
}