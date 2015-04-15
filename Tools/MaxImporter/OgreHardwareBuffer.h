

#ifndef __OgreHardwareBuffer_h__
#define __OgreHardwareBuffer_h__

#include "OgrePrerequisites.h"

namespace Ogre
{
	class OgreExport CHardwareBuffer
    {
	public:
	    enum Usage 
	    {
            HBU_STATIC = 1,
		    HBU_DYNAMIC = 2,
		    HBU_WRITE_ONLY = 4,
            HBU_DISCARDABLE = 8,
			HBU_STATIC_WRITE_ONLY = 5, 
			HBU_DYNAMIC_WRITE_ONLY = 6,
            HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE = 14
	    };

	    enum LockOptions
	    {
            HBL_NORMAL,
		    HBL_DISCARD,
		    HBL_READ_ONLY,
            HBL_NO_OVERWRITE
	    };

		CHardwareBuffer(Usage usage, bool systemMemory);
		virtual ~CHardwareBuffer();

		void* Lock(size_t offset, size_t length, LockOptions options);
		void Unlock(void);
	protected:
		virtual void* LockImpl(size_t offset, size_t length, LockOptions options);
		virtual void UnlockImpl(void);

		int m_nSizeInBytes;
		Usage m_eUsage;
		bool m_bSystemMemory;
    };

	class OgreExport CHardwareVertexBuffer : public CHardwareBuffer
	{
	public:
		CHardwareVertexBuffer(size_t vertexSize, size_t numVertices,
			CHardwareBuffer::Usage usage, bool useSystemMemory);
		virtual ~CHardwareVertexBuffer();

		size_t GetVertexSize(void) const;
		size_t GetNumVertices(void) const;
	protected:
		size_t mNumVertices;
		size_t mVertexSize;
	};

	class OgreExport CHardwareIndexBuffer : public CHardwareBuffer
	{
	public:
		enum IndexType
		{
			IT_16BIT,
			IT_32BIT
		};

	public:
		CHardwareIndexBuffer(IndexType idxType, size_t numIndexes, CHardwareBuffer::Usage usage,
			bool useSystemMemory);
		~CHardwareIndexBuffer();

		IndexType GetType(void) const { return m_IndexType; }
		size_t GetNumIndexes(void) const { return m_nNumIndexes; }
		size_t GetIndexSize(void) const { return m_nIndexSize; }
	protected:
		IndexType m_IndexType;
		size_t m_nNumIndexes;
		size_t m_nIndexSize;
	};
}

#endif