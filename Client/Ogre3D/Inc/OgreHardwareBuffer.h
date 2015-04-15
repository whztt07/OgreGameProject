

#ifndef __OgreHardwareBuffer_h__
#define __OgreHardwareBuffer_h__

#include "OgrePrerequisites.h"
#include "OgreResource.h"
#include "OgreMath.h"

namespace Ogre
{
	class CVertexDeclaration;
	class CVertexBufferBinding;
	class CTexture;

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

		CHardwareBuffer(Usage usage);
		virtual ~CHardwareBuffer();

		void* Lock(size_t offset, size_t length, LockOptions options);
		void* Lock(LockOptions options);
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
		CHardwareVertexBuffer(size_t vertexSize, size_t numVertices, CHardwareBuffer::Usage usage);
		virtual ~CHardwareVertexBuffer();

		size_t GetVertexSize(void) const;
		size_t GetNumVertices(void) const;
	protected:
		size_t m_nNumVertices;
		size_t m_nVertexSize;
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
		CHardwareIndexBuffer(IndexType idxType, size_t numIndexes, CHardwareBuffer::Usage usage);
		~CHardwareIndexBuffer();

		IndexType GetType(void) const { return m_IndexType; }
		size_t GetNumIndexes(void) const { return m_nNumIndexes; }
		size_t GetIndexSize(void) const { return m_nIndexSize; }
	protected:
		IndexType m_IndexType;
		size_t m_nNumIndexes;
		size_t m_nIndexSize;
	};

	class OgreExport CHardwareBufferManager
	{
	public:
		CHardwareBufferManager();
		~CHardwareBufferManager();

		static CHardwareBufferManager& Instance();
		virtual CHardwareVertexBuffer* CreateVertexBuffer(int nVertSize, int nVertNum, CHardwareBuffer::Usage nUsage) = 0;
		virtual CHardwareIndexBuffer* CreateIndexBuffer(int nIndexNum, CHardwareIndexBuffer::IndexType nType, CHardwareBuffer::Usage nUsage) = 0;
		virtual CVertexDeclaration* CreateVertexDeclaration() = 0;
		virtual void OnLostDevice(){}
		virtual void OnResetDevice(){}
		CVertexBufferBinding* CreateVertexBufferBinding();
	protected:
		static CHardwareBufferManager *s_Instance;
		typedef std::vector<CHardwareVertexBuffer*> VertexBufferList;
		typedef std::vector<CHardwareIndexBuffer*> IndexBufferList;
		typedef std::vector<CVertexDeclaration*> VertexDeclarationList;
		typedef std::vector<CVertexBufferBinding*> VertexBufferBindingList;
		VertexBufferList m_vecVertexBuf;
		IndexBufferList m_vecIndexBuf;
		VertexDeclarationList m_vecVertexDecl;
		VertexBufferBindingList m_vecVertexBufBinding;
	};

	class OgreExport CGpuProgram : public CResource
	{
	public:
		CGpuProgram(CString szGroup, CString szName);
		virtual ~CGpuProgram();
	};

	class OgreExport CHighLevelGpuProgram : public CGpuProgram
	{
	public:
		CHighLevelGpuProgram(CString szGroup, CString szName);
		virtual ~CHighLevelGpuProgram();

		virtual void BeginRender(){}
		virtual void EndRender(){}
		virtual void SetMatrix(std::string segment, CMatrix *pMatrix){}
		virtual void SetTexture(std::string segment, CTexture *pTexture){}
		virtual void Commit(){}
	};
}

#endif