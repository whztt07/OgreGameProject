

#include "OgreMdxMesh.h"
#include "OgreFile.h"
#include "OgreArchive.h"
#include "OgreResourceManager.h"
#include "OgreHardwareBuffer.h"
#include "OgreRenderPass.h"

namespace Ogre
{
	//-----------------------------------
	//-----------CMSMdxPointer-------------
	//-----------------------------------
	CMdxPointer::CMdxPointer(void* in) 
	{
		p = in;
	}

	//-----------------------------------
	//-----------CMemoryBlock------------
	//-----------------------------------
	CMemoryBlock::CMemoryBlock()
	{
		m_pBuffer = NULL;
		m_nSize = 0;
	}

	CMemoryBlock::~CMemoryBlock()
	{
		if( m_pBuffer )
		{
			delete[] m_pBuffer;
			m_pBuffer = NULL;
		}

		m_nSize = 0;
	}

	BOOL CMemoryBlock::Create( BYTE* pbyBuffer, int nBufferSize )
	{
		if( pbyBuffer == NULL || nBufferSize == 0 )
		{
			assert( false );
			return FALSE;
		}

		m_nSize = nBufferSize;
		m_pBuffer = new BYTE[m_nSize];
		memcpy( m_pBuffer, pbyBuffer, m_nSize );
		
		return TRUE;
	}

	CMdxPointer CMemoryBlock::GetPointer()
	{
		CMdxPointer p( m_pBuffer );
		return p;
	}

	int CMemoryBlock::GetBufferSize()
	{ 
		return m_nSize;
	}

	//--------------------------------
	//---------CMdxMaterial-----------
	//--------------------------------
	CMdxMaterial::CMdxMaterial(CMdxMesh *pMdx)
	{
		m_pSrcMdx = pMdx;
		m_bHasSpecular = false;
		m_bHasNormal = false;
		m_pszTexName = NULL;
		m_pszSpecularName = NULL;
		m_pszNormalName = NULL;
		m_dwRenderFlag = 0;
		m_nColorId = -1;
		m_pBaseTexture = NULL;
	}

	CMdxMaterial::~CMdxMaterial()
	{

	}

	void CMdxMaterial::ReadBaseInfo(CMdxPointer inP, int iSize)
	{
		CMdxPointer p(inP.p);

		m_pszTexName = p.c;
		p.c += MATERIAL_NAME_SIZE;
		
		m_bHasSpecular = *p.i++;
		if (m_bHasSpecular)
		{
			m_pszSpecularName = p.c;
			p.c += MATERIAL_NAME_SIZE;
		}

		m_bHasNormal = *p.i++;
		if (m_bHasNormal)
		{
			m_pszNormalName = p.c;
			p.c += MATERIAL_NAME_SIZE;
		}

		m_dwRenderFlag = *p.dw++;
		m_nColorId = *p.i++;
	}

	char* CMdxMaterial::GetTexName()
	{
		return m_pszTexName;
	}

	char* CMdxMaterial::GetNormal()
	{
		return m_pszNormalName;
	}

	char* CMdxMaterial::GetSpecular()
	{
		return m_pszSpecularName;
	}

	int CMdxMaterial::GetColorId()
	{
		return m_nColorId;
	}

	bool CMdxMaterial::HasAlphaTest()
	{
		return (m_dwRenderFlag & MATERIAL_ALPHA_TEST);
	}

	void CMdxMaterial::SetBaseTexture(CResource *pResource)
	{
		m_pBaseTexture = (CTexture*)pResource;
	}

	void CMdxMaterial::ReadMaterial(CMdxPointer inP, int iSize)
	{
		CMdxPointer p(inP.p);

		while(p.c < inP.c + iSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case MATERIAL_BASE_INFO:
				{
					p.dw++;
					int size = *p.i++;
					ReadBaseInfo(p.p, size);
					p.c += size;
				}
				break;
			default:
				{
					p.dw++;
					int size = *p.i++;
					p.c += size;
				}
				break;
			}
		}
	}

	//--------------------------------
	//---------CMdxMaterials----------
	//--------------------------------
	CMdxMaterials::CMdxMaterials(CMdxMesh *pMdx)
	{
		m_pSrcMdx = pMdx;
	}

	CMdxMaterials::~CMdxMaterials()
	{

	}

	CMdxMaterial* CMdxMaterials::GetMaterial(int i)
	{
		int size = m_vecMaterial.size();
		if (i < 0 || i > size - 1)
		{
			return NULL;
		}

		return m_vecMaterial[i];
	}

	void CMdxMaterials::ReadMaterials(CMdxPointer inP, int iSize)
	{
		CMdxPointer p(inP.p);

		while (p.c < inP.c + iSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case MATERIALS_INFO:
				{
					p.dw++;
					int size = *p.i++;
					ReadMaterial(p.p, size);
					p.c += size;
				}
				break;
			default:
				{
					p.dw++;
					int size = *p.i++;
					p.c += size;
				}
				break;
			}
		}
	}

	void CMdxMaterials::ReadMaterial(CMdxPointer inP, int iSize)
	{
		CMdxMaterial *pMaterial = new CMdxMaterial(m_pSrcMdx);
		if (pMaterial)
		{
			pMaterial->ReadMaterial(inP, iSize);
			m_vecMaterial.push_back(pMaterial);
		}
	}

	//--------------------------------
	//------------CMdxBone------------
	//--------------------------------
	CBaseBone::CBaseBone(CMdxMesh *pMdx)
	{
		m_pSrcMdx = pMdx;
		m_nParentID = -1;
		m_pszName = NULL;
	}

	CBaseBone::~CBaseBone()
	{
	}

	//--------------------------------
	//-------------CWOWBone-----------
	//--------------------------------
	CWOWBone::CWOWBone(CMdxMesh *pMdx)
		:CBaseBone(pMdx)
	{
		m_bCalc = false;
		m_bUseTrans = false;
		m_bUseScale = false;
		m_bUseQuat = false;
		
		m_nTransTimeNum = 0;
		m_nScaleTimeNum = 0;
		m_nQuatTimeNum = 0;
		m_nTransRangeNum = 0;
		m_nScaleRangeNum = 0;
		m_nQuatRangeNum = 0;
		m_nTransNum = 0;
		m_nScaleNum = 0;
		m_nQuatNum = 0;

		m_puTransTime = NULL;
		m_puScaleTime = NULL;
		m_puQuatTime = NULL;
		m_pTransRange = NULL;
		m_pScaleRange = NULL;
		m_pQuatRange = NULL;
		m_pTrans = NULL;
		m_pScale = NULL;
		m_pQuat = NULL;
	}

	CWOWBone::~CWOWBone()
	{
	}

	void CWOWBone::ReadBoneInfo(CMdxPointer inP, int nSize)
	{
		CMdxPointer p(inP.p);

		while (p.c < inP.c + nSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case WOW_BONE_BASE_INFO:
				{
					p.dw++;
					int size = *p.i++;
					ReadWOWBase(p.p, size);
					p.c += size;
				}
				break;
			default:
				{
					p.dw++;
					int size = *p.i++;
					p.c += size;
				}
				break;
			}
		}
	}

	void CWOWBone::ReadWOWBase(CMdxPointer inP, int nSize)
	{
		CMdxPointer p(inP.p);
		
		m_nParentID = *p.i++;
		m_bUseTrans = *p.i++;
		m_bUseScale = *p.i++;
		m_bUseQuat = *p.i++;
		m_nTransTimeNum = *p.i++;
		m_nScaleTimeNum = *p.i++;
		m_nQuatTimeNum = *p.i++;
		m_nTransRangeNum = *p.i++;
		m_nScaleRangeNum = *p.i++;
		m_nQuatRangeNum = *p.i++;
		m_nTransNum = *p.i++;
		m_nScaleNum = *p.i++;
		m_nQuatNum = *p.i++;

		m_vPivot = *p.v3++;
		if (m_nTransTimeNum > 0)
		{
			m_puTransTime = p.dw;
			p.dw += m_nTransTimeNum;
		}

		if (m_nScaleTimeNum > 0)
		{
			m_puScaleTime = p.dw;
			p.dw += m_nScaleTimeNum;
		}

		if (m_nQuatTimeNum > 0)
		{
			m_puQuatTime = p.dw;
			p.dw += m_nQuatTimeNum;
		}

		if (m_nTransRangeNum > 0)
		{
			m_pTransRange = (MdxAnimRange*)p.c;
			p.c += m_nTransRangeNum * sizeof(MdxAnimRange);
		}

		if (m_nScaleRangeNum > 0)
		{
			m_pScaleRange = (MdxAnimRange*)p.c;
			p.c += m_nScaleRangeNum * sizeof(MdxAnimRange);
		}

		if (m_nQuatRangeNum > 0)
		{
			m_pQuatRange = (MdxAnimRange*)p.c;
			p.c += m_nQuatRangeNum * sizeof(MdxAnimRange);
		}

		if (m_nTransNum > 0)
		{
			m_pTrans = p.v3;
			p.v3 += m_nTransNum;
		}

		if (m_nScaleNum > 0)
		{
			m_pScale = p.v3;
			p.v3 += m_nScaleNum;
		}

		if (m_nQuatNum > 0)
		{
			m_pQuat = p.quat;
			p.quat += m_nQuatNum;
		}
	}

	void CWOWBone::Clear()
	{
		m_bCalc = false;
	}

	CMatrix CWOWBone::GetMatrix()
	{
		return m_matBone;
	}

	CVector CWOWBone::GetTrans(int nAnimID, DWORD dwTime)
	{
		if (m_nTransNum == 0)
		{
			return CVector(0.0f, 0.0f, 0.0f);
		}

		if (m_nTransNum == 1)
		{
			return m_pTrans[0];
		}

		if (m_nTransRangeNum == 0)
		{
			return CVector(0.0f, 0.0f, 0.0f);
		}

		MdxAnimRange range = m_pTransRange[nAnimID];
		if (range.first != range.second)
		{
			size_t t1, t2;
			size_t pos = 0;
			for (size_t i = range.first; i < range.second; i++) 
			{
				if (dwTime >= m_puTransTime[i] && dwTime < m_puTransTime[i + 1]) 
				{
					pos = i;
					break;
				}
			}

			t1 = m_puTransTime[pos];
			t2 = m_puTransTime[pos + 1];
			float r = (dwTime - t1) / (float)(t2 - t1);
			CVector v1 = m_pTrans[pos];
			CVector v2 = m_pTrans[pos + 1];
			return Interpolate(r, v1, v2);
		}

		return CVector(0.0f, 0.0f, 0.0f);
	}

	CQuaternion CWOWBone::GetQuat(int nAnimID, DWORD dwTime)
	{
		if (m_nQuatNum == 0)
		{
			return CQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
		}

		if (m_nQuatNum == 1)
		{
			return m_pQuat[0];
		}

		if (m_nQuatRangeNum == 0)
		{
			return CQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
		}

		MdxAnimRange range = m_pQuatRange[nAnimID];
		if (range.first != range.second)
		{
			size_t t1, t2;
			size_t pos = 0;
			for (size_t i = range.first; i < range.second; i++) 
			{
				if (dwTime >= m_puQuatTime[i] && dwTime < m_puQuatTime[i + 1]) 
				{
					pos = i;
					break;
				}
			}

			t1 = m_puQuatTime[pos];
			t2 = m_puQuatTime[pos + 1];

			float r = (dwTime - t1) / (float)(t2 - t1);
			CQuaternion v1 = m_pQuat[pos];
			CQuaternion v2 = m_pQuat[pos + 1];
			return Interpolate(r, v1, v2);
		}

		return CQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
	}

	CVector CWOWBone::GetScale(int nAnimID, DWORD dwTime)
	{
		if (m_nScaleNum == 0)
		{
			return CVector(1.0f, 1.0f, 1.0f);
		}

		if (m_nScaleNum == 1)
		{
			return m_pScale[0];
		}

		if (m_nScaleRangeNum == 0)
		{
			return CVector(1.0f, 1.0f, 1.0f);
		}

		MdxAnimRange range = m_pScaleRange[nAnimID];
		if (range.first != range.second)
		{
			size_t t1, t2;
			size_t pos = 0;
			for (size_t i = range.first; i < range.second; i++) 
			{
				if (dwTime >= m_puScaleTime[i] && dwTime < m_puScaleTime[i + 1]) 
				{
					pos = i;
					break;
				}
			}

			t1 = m_puScaleTime[pos];
			t2 = m_puScaleTime[pos + 1];
			float r = (dwTime - t1) / (float)(t2 - t1);
			CVector v1 = m_pScale[pos];
			CVector v2 = m_pScale[pos + 1];
			return Interpolate(r, v1, v2);
		}

		return CVector(1.0f, 1.0f, 1.0f);
	}

	void CWOWBone::CalcBone(int nAnimID, DWORD dwTime)
	{
		if (m_bCalc)
		{
			return;
		}

		CMatrix m;
		bool bUse = m_bUseTrans || m_bUseScale || m_bUseQuat;
		if (bUse)
		{
			m.Translation(m_vPivot);

			if (m_bUseTrans) 
			{
				Ogre::CVector tr = GetTrans(nAnimID, dwTime);
				m *= Ogre::CMatrix::NewTranslation(tr);
			}

			if (m_bUseQuat) 
			{
				Ogre::CQuaternion q = GetQuat(nAnimID, dwTime);
				m *= Ogre::CMatrix::NewQuatRotate(q);
			}

			if (m_bUseScale)
			{
				Ogre::CVector sc = GetScale(nAnimID, dwTime);
				m *= Ogre::CMatrix::NewScale(sc);
			}

			m *= Ogre::CMatrix::NewTranslation(m_vPivot * -1.0f);
		}
		else
		{
			m.Unit();
		}

		if (m_nParentID > -1) 
		{
			CBaseBone *pBone = m_pSrcMdx->GetSkeleton()->GetBone(m_nParentID);
			if (pBone)
			{
				pBone->CalcBone(nAnimID, dwTime);
				m_matBone = pBone->GetMatrix() * m;
			}
			else
			{
				m_matBone = m;
			}
		} 
		else
		{
			m_matBone = m;
		}

		m_bCalc = true;
	}

	//--------------------------------
	//----------CMdxSkeleton----------
	//--------------------------------
	CMdxSkeleton::CMdxSkeleton(CMdxMesh *pMdx)
	{
		m_pSrcMdx = pMdx;
		m_nSketType = SKELETONTYPE_NONE;
	}

	CMdxSkeleton::~CMdxSkeleton()
	{

	}

	void CMdxSkeleton::ReadSkeleton(CMdxPointer inP, int nSize)
	{
		CMdxPointer p(inP.p);

		m_nSketType = *p.i++;

		while (p.c < inP.c + nSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case SKELETON_BONE_WOW:
				{
					p.dw++;
					int size = *p.i++;
					ReadBone(p.p, size);
					p.c += size;
				}
				break;
			default:
				{
					p.dw++;
					int size = *p.i++;
					p.c += size;
				}
				break;
			}
		}
	}

	void CMdxSkeleton::ReadBone(CMdxPointer inP, int nSize)
	{
		CBaseBone *pBone = NULL;
		switch(m_nSketType)
		{
		case SKELETONTYPE_WOW:
			{
				pBone = new CWOWBone(m_pSrcMdx);
			}
			break;
		default:
			break;
		}
		
		if (pBone)
		{
			pBone->ReadBoneInfo(inP.p, nSize);
			m_vecBone.push_back(pBone);
		}
	}

	CBaseBone* CMdxSkeleton::GetBone(int nID)
	{
		int nNum = m_vecBone.size();
		if (nID < 0 || nID > nNum - 1)
		{
			return NULL;
		}

		return m_vecBone[nID];
	}

	CMatrix CMdxSkeleton::GetMatrix(int nID)
	{
		CMatrix mat;
		mat.Unit();

		CBaseBone *pBone = GetBone(nID);
		if (pBone)
		{
			mat = pBone->GetMatrix();
		}

		return mat;
	}

	void CMdxSkeleton::CalcAllBone(int nAnimID, DWORD dwTime)
	{
		int nNum = m_vecBone.size();
		for (int i = 0; i < nNum; i++)
		{
			CBaseBone *pBone = m_vecBone[i];
			if (pBone)
			{
				pBone->Clear();
			}
		}

		for (int i = 0; i < nNum; i++)
		{
			CBaseBone *pBone = m_vecBone[i];
			if (pBone)
			{
				pBone->CalcBone(nAnimID, dwTime);
			}
		}
	}

	//--------------------------------
	//---------CMdxGeoChunk-----------
	//--------------------------------
	CMdxGeoChunk::CMdxGeoChunk(int id, CMdxMesh *pMdx)
	{
		m_pSrcMdx = pMdx;
		m_nChunkID = id;
		m_nMaterialId = -1;
		
		m_pszName = NULL;
		m_nVertexNum = 0;
		m_nIndexNum = 0;
		m_pVertices = NULL;
		m_pNormals = NULL;
		m_pUVs = NULL;
		m_pDiffuse = NULL;
		m_pIndices = NULL;
		m_pLIndices = NULL;
		m_pBoneWeight = NULL;
		m_pBoneIndex = NULL;

		m_bUseLIndex = false;
		m_bHasBoneInfo = false;
		m_pRenderOp = new CRenderOperation;
	}

	CMdxGeoChunk::~CMdxGeoChunk()
	{

	}

	int CMdxGeoChunk::GetVertexNum()
	{
		return m_nVertexNum;
	}

	int CMdxGeoChunk::GetIndexNum()
	{
		return m_nIndexNum;
	}

	int CMdxGeoChunk::GetMaterialId()
	{
		return m_nMaterialId;
	}

	CVector* CMdxGeoChunk::GetVertices()
	{
		return m_pVertices;
	}

	CVector* CMdxGeoChunk::GetNormals()
	{
		return m_pNormals;
	}

	CVector2D* CMdxGeoChunk::GetUVs()
	{
		return m_pUVs;
	}

	WORD* CMdxGeoChunk::GetIndices()
	{
		return m_pIndices;
	}

	byte* CMdxGeoChunk::GetBoneWeight()
	{
		return m_pBoneWeight;
	}

	byte* CMdxGeoChunk::GetBoneIndex()
	{
		return m_pBoneIndex;
	}

	bool CMdxGeoChunk::HasBoneInfo()
	{
		return m_bHasBoneInfo;
	}

	void CMdxGeoChunk::ReadChunk(CMdxPointer inP, int size)
	{
		CMdxPointer p(inP.p);

		while (p.c < inP.c + size)
		{
			switch(MDX_TAG(*p.dw))
			{
			case CHUNK_BASE_INFO:
				{
					p.dw++;
					int size = *p.i++;
					ReadBaseInfo(p.p, size);
					p.c += size;
				}
				break;
			case CHUNK_BONE_INFO:
				{
					p.dw++;
					int size = *p.i++;
					ReadBoneInfo(p.p, size);
					p.c += size;
				}
				break;
			default:
				{
					p.dw++;
					int size = *p.i++;
					p.c += size;
				}
				break;
			}
		}
	}

	void CMdxGeoChunk::ReadBoneInfo(CMdxPointer inP, int size)
	{
		CMdxPointer p(inP.p);

		m_pBoneWeight = p.byte;
		p.byte += m_nVertexNum * 4;

		m_pBoneIndex = p.byte;
		p.byte += m_nVertexNum * 4;

		m_bHasBoneInfo = true;
	}

	void CMdxGeoChunk::ReadBaseInfo(CMdxPointer inP, int size)
	{
		CMdxPointer p(inP.p);

		m_pszName = p.c;
		p.c += CHUNK_NAME_SIZE;

		m_nVertexNum = *p.i++;
		m_nIndexNum = *p.i++;
		m_nMaterialId = *p.i++;
		m_bUseLIndex = *p.i++;
		
		m_pVertices = p.v3;
		p.v3 += m_nVertexNum;
		
		m_pNormals = p.v3;
		p.v3 += m_nVertexNum;
		
		m_pUVs = p.v2;
		p.v2 += m_nVertexNum;

		if (m_bUseLIndex)
		{
			m_pLIndices = p.dw;
			p.dw += m_nIndexNum;
		}
		else
		{
			m_pIndices = p.word;
			p.word += m_nIndexNum;

		}
	}

	//--------------------------------
	//---------CMdxGeometry-----------
	//--------------------------------
	CMdxGeometry::CMdxGeometry(CMdxMesh *pMdx)
	{
		m_pSrcMdx = pMdx;
	}

	CMdxGeometry::~CMdxGeometry()
	{

	}

	void CMdxGeometry::ReadGeometry(CMdxPointer inP, int size)
	{
		CMdxPointer p(inP.p);

		while(p.c < inP.c + size)
		{
			switch(MDX_TAG(*p.dw))
			{
			case GEOMETRY_CHUNK:
				{
					p.dw++;
					int size = *p.i++;
					ReadChunk(p.p, size);
					p.c += size;
				}
				break;
			default:
				{
					p.dw++;
					int size = *p.i++;
					p.c += size;
				}
				break;
			}
		}
	}

	CMdxGeoChunk* CMdxGeometry::GetChunk(int id)
	{
		int nSize = m_vecChunk.size();
		if (id < 0 || id > nSize - 1)
		{
			return NULL;
		}

		return m_vecChunk[id];
	}

	int CMdxGeometry::GetChunkNum()
	{
		int nSize = m_vecChunk.size();
		return nSize;
	}

	void CMdxGeometry::ReadChunk(CMdxPointer inP, int size)
	{
		int nID = m_vecChunk.size();
		CMdxGeoChunk *pChunk = new CMdxGeoChunk(nID, m_pSrcMdx);
		if (pChunk)
		{
			pChunk->ReadChunk(inP, size);
			m_vecChunk.push_back(pChunk);
		}
	}

	//--------------------------------
	//-----------CBaseAnim------------
	//--------------------------------
	CBaseAnim::CBaseAnim()
	{
		m_pszName = NULL;
	}

	CBaseAnim::~CBaseAnim()
	{

	}

	char* CBaseAnim::GetName()
	{
		return m_pszName;
	}

	//--------------------------------
	//------------CWOWAnim------------
	//--------------------------------
	CWOWAnim::CWOWAnim()
	{

	}

	CWOWAnim::~CWOWAnim()
	{

	}

	void CWOWAnim::ReadAnim(CMdxPointer inP, int iSize)
	{
		CMdxPointer p(inP.p);
		m_pszName = p.c;
		p.c += ANIM_NAME_SIZE;

		m_nStartTime = *p.dw++;
		m_nEndTime = *p.dw++;
	}

	DWORD CWOWAnim::GetStartTime()
	{
		return m_nStartTime;
	}

	DWORD CWOWAnim::GetEndTime()
	{
		return m_nEndTime;
	}

	//--------------------------------
	//----------CMdxAnimSet-----------
	//--------------------------------
	CMdxAnimSet::CMdxAnimSet()
	{
		m_nAnimType = ANIMSET_TYPE_NONE;
	}

	CMdxAnimSet::~CMdxAnimSet()
	{

	}

	void CMdxAnimSet::ReadAnimSet(CMdxPointer inP, int iSize)
	{
		CMdxPointer p(inP.p);

		m_nAnimType = *p.i++;

		while (p.c < inP.c + iSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case ANIMSET_ANIM:
				{
					p.dw++;
					int size = *p.i++;
					ReadAnim(p.p, size);
					p.c += size;
				}
				break;
			default:
				{
					p.dw++;
					int size = *p.i++;
					p.c += size;
				}
				break;
			}
		}
	}

	void CMdxAnimSet::ReadAnim(CMdxPointer inP, int iSize)
	{
		CBaseAnim *pAnim = new CWOWAnim;
		if (pAnim)
		{
			pAnim->ReadAnim(inP.p, iSize);
			m_vecAnim.push_back(pAnim);
			m_mapAnim[pAnim->GetName()] = pAnim;
			m_mapAnimID[m_vecAnim.size() - 1] = pAnim;
		}
	}

	CBaseAnim* CMdxAnimSet::FindAnim(int nID)
	{
		AnimIDMap::iterator iter = m_mapAnimID.find(nID);
		if (iter != m_mapAnimID.end())
		{
			return iter->second;
		}

		return NULL;
	}

	int CMdxAnimSet::GetAnimNum()
	{
		return m_vecAnim.size();
	}

	int CMdxAnimSet::GetType()
	{
		return m_nAnimType;
	}

	//--------------------------------
	//--------CWOWColorGroup----------
	//--------------------------------
	CWOWColorGroup::CWOWColorGroup(CMdxMesh *pMdx)
	{
		m_pSrcMdx = pMdx;
	}

	CWOWColorGroup::~CWOWColorGroup()
	{

	}

	CWOWColorGroup::CWOWColor::CWOWColor()
	{
		nOpacityTimeNum = 0;
		nOpacityRangeNum = 0;
		nOpacityNum = 0;
		puOpacityTime = NULL;
		pOpacityRange = NULL;
		pOpacity = NULL;
	}

	CWOWColorGroup::CWOWColor::~CWOWColor()
	{

	}

	void CWOWColorGroup::ReadWOWColor(CMdxPointer inP, int iSize)
	{
		CMdxPointer p(inP.p);

		while (p.c < inP.c + iSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case WOWCOLOR_COLOR:
				{
					p.dw++;
					int size = *p.i++;

					CMdxPointer s(p.p);
					{
						CWOWColor *pColor = new CWOWColor;
						pColor->nOpacityTimeNum = *s.i++;
						pColor->nOpacityRangeNum = *s.i++;
						pColor->nOpacityNum = *s.i++;
						
						if (pColor->nOpacityTimeNum > 0)
						{
							pColor->puOpacityTime = s.dw;
							s.dw += pColor->nOpacityTimeNum;
						}
						
						if(pColor->nOpacityRangeNum)
						{
							pColor->pOpacityRange = (MdxAnimRange*)s.c;
							s.c += pColor->nOpacityRangeNum * sizeof(MdxAnimRange);
						}
						
						if (pColor->nOpacityNum)
						{
							pColor->pOpacity = s.f;
							s.f += pColor->nOpacityNum;
						}
						
						m_vecColor.push_back(pColor);
					}

					p.c += size;
				}
				break;
			default:
				{
					p.dw++;
					int size = *p.i++;
					p.c += size;
				}
				break;
			}
		}
	}

	float CWOWColorGroup::GetColor(int nID, int nAnimId)
	{
		if (nID < 0 || nID > m_vecColor.size() - 1)
		{
			return 0.0f;
		}

		CWOWColor *pColor = m_vecColor[nID];
		if (pColor == NULL)
		{
			return 0.0f;
		}

		DWORD dwTime = m_pSrcMdx->GetFrame();

		if (pColor->nOpacityNum == 0)
		{
			return 0.0f;
		}

		if (pColor->nOpacityNum == 1)
		{
			return pColor->pOpacity[0];
		}

		if (pColor->nOpacityRangeNum == 0)
		{
			return 0.0f;
		}

		MdxAnimRange range = pColor->pOpacityRange[nAnimId];
		if (range.first != range.second)
		{
			size_t t1, t2;
			size_t pos = 0;
			for (size_t i = range.first; i < range.second; i++) 
			{
				if (dwTime >= pColor->puOpacityTime[i] && dwTime < pColor->puOpacityTime[i + 1]) 
				{
					pos = i;
					break;
				}
			}

			t1 = pColor->puOpacityTime[pos];
			t2 = pColor->puOpacityTime[pos + 1];
			float r = (dwTime - t1) / (float)(t2 - t1);
			float v1 = pColor->pOpacity[pos];
			float v2 = pColor->pOpacity[pos + 1];
			
			return Interpolate(r, v1, v2);
		}

		return 0.0f;
	}

	//--------------------------------
	//--------------CMdxMesh--------------
	//--------------------------------
	CMdxMesh::CMdxMesh(CString szGroup, CString szName)
		:CResource(szGroup, szName)
	{
		m_szName = szName;
		m_pGeometry = NULL;
		m_pMaterials = NULL;
		m_pSkeleton = NULL;
		m_pAnimSet = NULL;
		m_pWOWColorGroup = NULL;
		m_nFrame = 0;
	}

	CMdxMesh::~CMdxMesh()
	{

	}

	bool CMdxMesh::LoadImpl()
	{
		// load mdx mesh ...
		if (m_pDataStream == NULL) return false;
		int nSize = m_pDataStream->GetSize();
		byte *pBuf = m_pDataStream->GetBuffer();
		LoadFromMdxFile(pBuf, nSize);

		// load mdx mesh skeleton ...
		CString szSket = CStringUtil::Combine(m_szName, ".skeleton", true);
		CDataStream *pSketStream = CResourceGroupManager::Instance().OpenResource(m_szGroup, szSket);
		if (pSketStream != NULL)
		{
			nSize = pSketStream->GetSize();
			pBuf = pSketStream->GetBuffer();
			LoadFromSketFile(pBuf, nSize);
			SAFE_DELETE(pSketStream);
		}

		// load texture ...
		int nChunkNum = m_pGeometry->GetChunkNum();
		for (int nChunk = 0; nChunk < nChunkNum; nChunk++)
		{
			CMdxGeoChunk *pChunk = m_pGeometry->GetChunk(nChunk);
			int nMatId = pChunk->GetMaterialId();
			CMdxMaterial *pMaterial = m_pMaterials->GetMaterial(nMatId);
			CString szMat = CStringUtil::Combine(m_szName, pMaterial->GetTexName(), false);
			pMaterial->SetBaseTexture(CTextureManager::Instance().Load(m_szGroup, szMat));

			if (pMaterial->HasSpecular())
			{
				szMat = CStringUtil::Combine(m_szName, pMaterial->GetSpecular(), false);
				CTextureManager::Instance().Load(m_szGroup, szMat);
			}
		}

		// vertex buffer and index buffer
		for (int nChunk = 0; nChunk < nChunkNum; nChunk++)
		{
			CMdxGeoChunk *pChunk = m_pGeometry->GetChunk(nChunk);
			if (pChunk == NULL)
			{
				continue;
			}

			CVector *pVertices = pChunk->GetVertices();
			CVector *pNormals = pChunk->GetNormals();
			CVector2D *pUVs = pChunk->GetUVs();
			WORD *pIndices = pChunk->GetIndices();
			int nVertexNum = pChunk->GetVertexNum();
			int nIndexNum = pChunk->GetIndexNum();

			CHardwareVertexBuffer *pVertBuf = CHardwareBufferManager::Instance().CreateVertexBuffer(
						32, nVertexNum, CHardwareBuffer::HBU_WRITE_ONLY);
			if (pVertBuf != NULL)
			{		
				CMdxMeshFormat *pBuf = (CMdxMeshFormat*)pVertBuf->Lock(CHardwareBuffer::HBL_DISCARD);
				if (pBuf != NULL)
				{
					for (int nVertex = 0; nVertex < nVertexNum; nVertex++, pBuf++)
					{
						pBuf->vPos = pVertices[nVertex];
						pBuf->vNormal = pNormals[nVertex];
						pBuf->vTex = pUVs[nVertex];
					}
				}
				pVertBuf->Unlock();
			}

			CHardwareIndexBuffer *pIndexBuf = CHardwareBufferManager::Instance().CreateIndexBuffer(
				nIndexNum, CHardwareIndexBuffer::IT_16BIT, CHardwareBuffer::HBU_WRITE_ONLY);
			if (pIndexBuf != NULL)
			{
				void *pBuf = pIndexBuf->Lock(CHardwareBuffer::HBL_DISCARD);
				if (pBuf != NULL)
					memcpy(pBuf, pIndices, nIndexNum * sizeof(WORD));
				pIndexBuf->Unlock();
			}

			// add render op ...
			CRenderOperation *pRenderOp = pChunk->GetRenderOp();
			if (pRenderOp != NULL)
			{
				CVertexData *pVertexData = new CVertexData;
				CIndexData *pIndexData = new CIndexData;
				CVertexBufferBinding *pBing = pVertexData->GetVertexBufferBinding();
				CVertexDeclaration *pDecl = pVertexData->GetVertexDeclaration();	
				pVertexData->SetVertexRange(0, pChunk->GetVertexNum());
				pIndexData->SetIndexRange(0, pChunk->GetIndexNum());
				pIndexData->SetIndexBuffer(pIndexBuf);
				pBing->AddBinding(0, pVertBuf);
				pDecl->AddElement(0, 0, VET_SHORT3, VES_POSITION, 0);
				pDecl->AddElement(0, 12, VET_FLOAT3, VES_NORMAL, 0);
				pDecl->AddElement(0, 24, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);

				pRenderOp->m_bUseIndex = true;
				pRenderOp->m_pVertexData = pVertexData;
				pRenderOp->m_pIndexData = pIndexData;
				pRenderOp->m_OpType = CRenderOperation::OT_TRIANGLE_LIST;
			}
		}

		return true;
	}

	bool CMdxMesh::LoadFromMdxFile(BYTE* pbyBuffer, int nBufferSize)
	{
		CMdxPointer p(pbyBuffer);
		while (p.byte < pbyBuffer + nBufferSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case GEOMETRY:
				{
					p.dw++;
					int size = *p.i++;
					ReadGeometry(p.p, size);
					p.c += size;
				}
				break;
			case MATERIALS:
				{
					p.dw++;
					int size = *p.i++;
					ReadMaterials(p.p, size);
					p.c += size;
				}
				break;
			case WOWCOLOR:
				{
					p.dw++;
					int size = *p.i++;
					ReadWOWColor(p.p, size);
					p.c += size;
				}
				break;
			default:
				{
					p.dw++;
					int size = *p.i++;
					p.c += size;
				}
				break;
			}
		}

		return true;
	}

	bool CMdxMesh::LoadFromSketFile(byte *pbyBuffer, int nBufSize)
	{
		CMdxPointer p(pbyBuffer);

		while(p.byte < pbyBuffer + nBufSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case SKELETON:
				{
					p.dw++;
					int size = *p.i++;
					ReadSkeleton(p.p, size);
					p.c += size;
				}
				break;
			case ANIMSET:
				{
					p.dw++;
					int size = *p.i++;
					ReadAnimSet(p.p, size);
					p.c += size;
				}
				break;
			default:
				{
					p.dw++;
					int size = *p.i++;
					p.c += size;
				}
				break;
			}
		}

		return true;
	}

	void CMdxMesh::UpdateSkeleton(int nAnimID, bool bSingleFrame, int nFrame)
	{
		int t = 0;
		if (m_pAnimSet)
		{
			CBaseAnim *pAnim = m_pAnimSet->FindAnim(nAnimID);
			if (pAnim)
			{
				if (m_pAnimSet->GetType() == CMdxAnimSet::ANIMSET_TYPE_WOW)
				{
					CWOWAnim *pWOWAnim = (CWOWAnim*)pAnim;

					DWORD dwStartTime = pWOWAnim->GetStartTime();
					DWORD dwEndTime = pWOWAnim->GetEndTime();
					
					if (bSingleFrame)
					{
						t = nFrame;
						if (t < dwStartTime || t >= dwEndTime)
						{
							t = dwStartTime;
						}
					}
					else
					{
						static DWORD dwLastTime = HQ_TimeGetTime();
						DWORD dwCurTime = HQ_TimeGetTime();
						t = dwStartTime + (dwCurTime - dwLastTime);
						while (t >= dwEndTime)
						{
							t -= (dwEndTime - dwStartTime);
							dwLastTime = HQ_TimeGetTime();
						}
					}
				}
			}
		}

		m_nFrame = t;

		if (m_pSkeleton)
		{
			m_pSkeleton->CalcAllBone(nAnimID, m_nFrame);
		}	
	}

	DWORD CMdxMesh::GetCurAnimStartTime(int nAnimId)
	{
		if (m_pAnimSet)
		{
			CBaseAnim *pAnim = m_pAnimSet->FindAnim(nAnimId);
			if (pAnim)
			{
				if (m_pAnimSet->GetType() == CMdxAnimSet::ANIMSET_TYPE_WOW)
				{
					CWOWAnim *pWOWAnim = (CWOWAnim*)pAnim;

					DWORD dwStartTime = pWOWAnim->GetStartTime();
					return dwStartTime;
				}
			}
		}

		return 0;
	}

	DWORD CMdxMesh::GetCurAnimEndTime(int nAnimId)
	{
		if (m_pAnimSet)
		{
			CBaseAnim *pAnim = m_pAnimSet->FindAnim(nAnimId);
			if (pAnim)
			{
				if (m_pAnimSet->GetType() == CMdxAnimSet::ANIMSET_TYPE_WOW)
				{
					CWOWAnim *pWOWAnim = (CWOWAnim*)pAnim;

					DWORD dwEndTime = pWOWAnim->GetEndTime();
					return dwEndTime;
				}
			}
		}

		return 0;
	}

	void CMdxMesh::ReadGeometry(CMdxPointer inP, int size)
	{
		CMdxPointer p(inP.p);

		m_GeometryMB.Create(p.byte, size);
		m_pGeometry = new CMdxGeometry(this);
		m_pGeometry->ReadGeometry(m_GeometryMB.GetPointer(), m_GeometryMB.GetBufferSize());
	}

	void CMdxMesh::ReadMaterials(CMdxPointer inP, int iSize)
	{
		CMdxPointer p(inP.p);

		m_MaterialsMB.Create(p.byte, iSize);
		m_pMaterials = new CMdxMaterials(this);
		m_pMaterials->ReadMaterials(m_MaterialsMB.GetPointer(), m_MaterialsMB.GetBufferSize());
	}

	void CMdxMesh::ReadWOWColor(CMdxPointer inP, int iSize)
	{
		CMdxPointer p(inP.p);

		m_WOWColorMB.Create(p.byte, iSize);
		m_pWOWColorGroup = new CWOWColorGroup(this);
		m_pWOWColorGroup->ReadWOWColor(m_WOWColorMB.GetPointer(), m_WOWColorMB.GetBufferSize());
	}

	void CMdxMesh::ReadSkeleton(CMdxPointer inP, int iSize)
	{
		m_SkeletonMB.Create(inP.byte, iSize);
		m_pSkeleton = new CMdxSkeleton(this);
		m_pSkeleton->ReadSkeleton(m_SkeletonMB.GetPointer(), m_SkeletonMB.GetBufferSize());
	}

	void CMdxMesh::ReadAnimSet(CMdxPointer inP, int iSize)
	{
		m_AnimSetMB.Create(inP.byte, iSize);
		m_pAnimSet = new CMdxAnimSet;
		m_pAnimSet->ReadAnimSet(m_AnimSetMB.GetPointer(), m_AnimSetMB.GetBufferSize());
	}
}