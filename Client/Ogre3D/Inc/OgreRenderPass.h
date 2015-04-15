

#ifndef __OgreRenderPass_h__
#define __OgreRenderPass_h__

#include "OgrePrerequisites.h"

namespace Ogre
{
	class CHardwareVertexBuffer;
	class CHardwareIndexBuffer;

	enum VertexElementSemantic
	{
		VES_POSITION = 1,
		VES_BLEND_WEIGHTS,
		VES_BLEND_INDICES,
		VES_NORMAL,
		VES_DIFFUSE,
		VES_SPECULAR,
		VES_TEXTURE_COORDINATES,
		VES_BINORMAL,
		VES_TANGENT
	};

	enum VertexElementType
	{
		VET_FLOAT1 = 0,
		VET_FLOAT2 = 1,
		VET_FLOAT3 = 2,
		VET_FLOAT4 = 3,
		/// alias to more specific colour type - use the current rendersystem's colour packing
		VET_COLOUR = 4,
		VET_SHORT1 = 5,
		VET_SHORT2 = 6,
		VET_SHORT3 = 7,
		VET_SHORT4 = 8,
		VET_UBYTE4 = 9,
		/// D3D style compact colour
		VET_COLOUR_ARGB = 10,
		/// GL style compact colour
		VET_COLOUR_ABGR = 11
	};

	class OgreExport CVertexElement
	{
	public:
		CVertexElement(int source, int offset, VertexElementType theType,
			VertexElementSemantic semantic, unsigned short index);
		~CVertexElement();

		int GetSource(){return m_nSource;}
		int GetOffset(){return m_nOffset;}
		VertexElementType GetType(){return m_nType;}
		VertexElementSemantic GetSemantic(){return m_nSemantic;}
		int GetIndex(){return m_nIndex;}
		int GetTypeSize(VertexElementType type);
		int GetTypeCount(VertexElementType type);
	protected:
		int m_nSource;
		int m_nOffset;
		VertexElementType m_nType;
		VertexElementSemantic m_nSemantic;
		int m_nIndex;
	};

	class OgreExport CVertexDeclaration
	{
	public:
		CVertexDeclaration();
		virtual ~CVertexDeclaration();

		void AddElement(int source, int offset, VertexElementType theType, 
			VertexElementSemantic semantic, unsigned short index);
		int GetElementCount();
		CVertexElement* GetElement(int i);
	protected:
		typedef std::vector<CVertexElement*> ElementList;
		ElementList m_vecElement;
	};

	class OgreExport CVertexBufferBinding
	{
	public:
		CVertexBufferBinding();
		~CVertexBufferBinding();
		typedef std::map<int, CHardwareVertexBuffer*> VertexBufMap;

		CHardwareVertexBuffer* AddBinding(int nIndex, CHardwareVertexBuffer *pBuf);
		VertexBufMap GetBindingMap(){return m_mapVertexBuf;}
	private:
		VertexBufMap m_mapVertexBuf;
	};

	class OgreExport CVertexData
	{
	public:
		CVertexData();
		~CVertexData();

		void SetVertexDeclaration(CVertexDeclaration *pDecl){m_pVertexDeclaration = pDecl;}
		void SetVertexBufBinding(CVertexBufferBinding *pBind){m_pVertexBufBinding = pBind;}
		void SetVertexRange(int nStart, int nCount){m_nVertexStart = nStart; m_nVertexCount = nCount;}
		CVertexDeclaration* GetVertexDeclaration(){return m_pVertexDeclaration;}
		CVertexBufferBinding* GetVertexBufferBinding(){return m_pVertexBufBinding;}
		int GetVertexStart(){return m_nVertexStart;}
		int GetVertexCount(){return m_nVertexCount;}
	private:
		CVertexDeclaration *m_pVertexDeclaration;
		CVertexBufferBinding *m_pVertexBufBinding;
		int m_nVertexStart;
		int m_nVertexCount;
	};

	class OgreExport CIndexData
	{
	public:
		CIndexData();
		~CIndexData();

		void SetIndexBuffer(CHardwareIndexBuffer *pBuffer){m_pIndexBuffer = pBuffer;}
		void SetIndexRange(int nStart, int nCount){m_nIndexStart = nStart; m_nIndexCount = nCount;}
		CHardwareIndexBuffer* GetIndexBuffer(){return m_pIndexBuffer;}
		int GetIndexStart(){return m_nIndexStart;}
		int GetIndexCount(){return m_nIndexCount;}
	private:
		CHardwareIndexBuffer *m_pIndexBuffer;
		int m_nIndexStart;
		int m_nIndexCount;
	};

	class OgreExport CRenderOperation
	{
	public:
		CRenderOperation();
		~CRenderOperation();

		enum OperationType
		{
			OT_POINT_LIST,
			OT_LINE_LIST,
			OT_LINE_STRIP,
			OT_TRIANGLE_LIST,
			OT_TRIANGLE_STRIP,
			OT_TRIANGLE_FAN,
		};

		OperationType m_OpType;
		CVertexData *m_pVertexData;
		CIndexData *m_pIndexData;
		bool m_bUseIndex;
	};
}

#endif