

#include "OgreRenderPass.h"
#include "OgreException.h"
#include "OgreHardwareBuffer.h"

namespace Ogre
{
	// -------------------------
	// CVertexElement
	// -------------------------
	CVertexElement::CVertexElement(int source, int offset, VertexElementType theType,
		VertexElementSemantic semantic, unsigned short index)
	{
		m_nSource = source;
		m_nOffset = offset;
		m_nType = theType;
		m_nSemantic = semantic;
		m_nIndex = index;
	}

	CVertexElement::~CVertexElement()
	{

	}

	int CVertexElement::GetTypeSize(VertexElementType type)
	{
		switch(type)
		{
		case VET_COLOUR:
		case VET_COLOUR_ABGR:
		case VET_COLOUR_ARGB:
			return sizeof(DWORD);
		case VET_FLOAT1:
			return sizeof(float);
		case VET_FLOAT2:
			return sizeof(float)*2;
		case VET_FLOAT3:
			return sizeof(float)*3;
		case VET_FLOAT4:
			return sizeof(float)*4;
		case VET_SHORT1:
			return sizeof(short);
		case VET_SHORT2:
			return sizeof(short)*2;
		case VET_SHORT3:
			return sizeof(short)*3;
		case VET_SHORT4:
			return sizeof(short)*4;
		case VET_UBYTE4:
			return sizeof(unsigned char)*4;
		}

		return 0;
	}

	int CVertexElement::GetTypeCount(VertexElementType type)
	{
		switch (type)
		{
		case VET_COLOUR:
		case VET_COLOUR_ABGR:
		case VET_COLOUR_ARGB:
			return 1;
		case VET_FLOAT1:
			return 1;
		case VET_FLOAT2:
			return 2;
		case VET_FLOAT3:
			return 3;
		case VET_FLOAT4:
			return 4;
		case VET_SHORT1:
			return 1;
		case VET_SHORT2:
			return 2;
		case VET_SHORT3:
			return 3;
		case VET_SHORT4:
			return 4;
		case VET_UBYTE4:
			return 4;
		}

		OGRE_EXCEPT(CException::ERR_INVALIDPARAMS, "Invalid type", 
			"VertexElement::getTypeCount");
	}

	// -------------------------
	// CVertexDeclaration
	// -------------------------
	CVertexDeclaration::CVertexDeclaration()
	{

	}

	CVertexDeclaration::~CVertexDeclaration()
	{

	}

	int CVertexDeclaration::GetElementCount()
	{
		return m_vecElement.size();
	}

	CVertexElement* CVertexDeclaration::GetElement(int i)
	{
		int nNum = m_vecElement.size();
		if (nNum == 0)
		{
			return NULL;
		}

		if (i < 0 || i > nNum - 1)
		{
			return NULL;
		}

		return m_vecElement[i];
	}

	void CVertexDeclaration::AddElement(int source, int offset, VertexElementType theType, 
			VertexElementSemantic semantic, unsigned short index)
	{
		CVertexElement *pElement = new CVertexElement(source, offset, theType, semantic, index);
		m_vecElement.push_back(pElement);
	}

	// -------------------------
	// CVertexBufferBinding
	// -------------------------
	CVertexBufferBinding::CVertexBufferBinding()
	{

	}

	CVertexBufferBinding::~CVertexBufferBinding()
	{

	}

	CHardwareVertexBuffer* CVertexBufferBinding::AddBinding(int nIndex, CHardwareVertexBuffer *pBuf)
	{
		VertexBufMap::iterator iter = m_mapVertexBuf.find(nIndex);
		if (iter != m_mapVertexBuf.end())
		{
			return iter->second;
		}

		m_mapVertexBuf[nIndex] = pBuf;

		iter = m_mapVertexBuf.find(nIndex);
		if (iter != m_mapVertexBuf.end())
		{
			return iter->second;
		}

		return NULL;
	}

	// -------------------------
	// CVertexData
	// -------------------------
	CVertexData::CVertexData()
	{
		m_pVertexDeclaration = CHardwareBufferManager::Instance().CreateVertexDeclaration();
		m_pVertexBufBinding = CHardwareBufferManager::Instance().CreateVertexBufferBinding();
		m_nVertexStart = 0;
		m_nVertexCount = 0;
	}

	CVertexData::~CVertexData()
	{

	}

	// -------------------------
	// CIndexData
	// -------------------------
	CIndexData::CIndexData()
	{
		m_pIndexBuffer = NULL;
		m_nIndexStart = 0;
		m_nIndexCount = 0;
	}

	CIndexData::~CIndexData()
	{

	}

	// -------------------------
	// CRenderOperation
	// -------------------------
	CRenderOperation::CRenderOperation()
	{
		m_OpType = OT_TRIANGLE_LIST;
		m_pVertexData = NULL;
		m_pIndexData = NULL;
		m_bUseIndex = true;
	}

	CRenderOperation::~CRenderOperation()
	{

	}
}