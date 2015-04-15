
#ifndef __OgreMdx_h__
#define __OgreMdx_h__

#include "OgrePrerequisites.h"
#include "OgreMath.h"
#include "OgreUseful.h"
#include "OgreResource.h"

namespace Ogre
{
	class CMdxMesh;
	class CRenderOperation;
	class CTexture;

	typedef std::pair<size_t, size_t> MdxAnimRange;
	template <class T> T Interpolate(const float r, const T &v1, const T &v2)
	{
		return static_cast<T>(v1 * (1.0f - r) + v2 * r);
	}

	union OgreExport CMdxPointer
	{
		CMdxPointer(void *in);

		void		*p;
		byte		*byte;
		int			*i;
		float		*f;
		short		*int16;
		char		*c;
		DWORD		*dw;
		WORD		*word;
		CVector2D	*v2;
		CVector		*v3;
		CQuaternion *quat;
	};

	class OgreExport CMemoryBlock
	{
	public:
		CMemoryBlock();
		~CMemoryBlock();

		BOOL Create( BYTE* pbyBuffer, int nBufferSize );
		CMdxPointer GetPointer();
		int GetBufferSize();
	private:
		byte*	m_pBuffer;
		int		m_nSize;
	};

	class OgreExport CMdxMaterial
	{
	public:
		CMdxMaterial(CMdxMesh *pMdx);
		~CMdxMaterial();

		enum
		{
			MATERIAL_NAME_SIZE = 80,
			MATERIAL_BASE_INFO = 'mtbs',
		};

		enum
		{
			MATERIAL_ALPHA_TEST = 1 << 0,
		};

		void ReadMaterial(CMdxPointer inP, int size);
		char* GetTexName();
		char* GetNormal();
		char* GetSpecular();
		int GetColorId();
		CTexture* GetBaseTexture(){return m_pBaseTexture;}
		bool HasNormal(){return m_bHasNormal;}
		bool HasSpecular(){return m_bHasSpecular;}
		bool HasAlphaTest();
		void SetBaseTexture(CResource *pResource);
	private:
		void ReadBaseInfo(CMdxPointer inP, int iSize);

		CMdxMesh *m_pSrcMdx;
		CTexture *m_pBaseTexture;

		char *m_pszTexName;
		char *m_pszSpecularName;
		char *m_pszNormalName;
		bool m_bHasSpecular;
		bool m_bHasNormal;
		DWORD m_dwRenderFlag;
		int m_nColorId;
	};

	class OgreExport CMdxMaterials
	{
	public:
		CMdxMaterials(CMdxMesh *pMdx);
		~CMdxMaterials();

		enum
		{
			MATERIALS_INFO = 'mtrl',
		};

		void ReadMaterials(CMdxPointer inP, int iSize);
		CMdxMaterial* GetMaterial(int i);
	private:
		void ReadMaterial(CMdxPointer inP, int iSize);

		CMdxMesh *m_pSrcMdx;
		typedef std::vector<CMdxMaterial*> MaterialList;
		MaterialList m_vecMaterial;
	};

	class OgreExport CBaseBone
	{
	public:
		CBaseBone(CMdxMesh *pMdx);
		~CBaseBone();

		virtual void ReadBoneInfo(CMdxPointer inP, int nSize) = 0;
		virtual void CalcBone(int nAnimID, DWORD dwTime) = 0;
		virtual void Clear() = 0;
		virtual CMatrix GetMatrix() = 0;
	protected:
		CMdxMesh *m_pSrcMdx;
		int m_nParentID;
		char *m_pszName;
	};

	class OgreExport CWOWBone : public CBaseBone
	{
	public:
		CWOWBone(CMdxMesh *pMdx);
		~CWOWBone();

		enum
		{
			WOW_BONE_BASE_INFO = 'wowb',
		};

		virtual void ReadBoneInfo(CMdxPointer inP, int nSize);
		virtual void CalcBone(int nAnimID, DWORD dwTime);
		virtual void Clear();
		virtual CMatrix GetMatrix();
	private:
		void ReadWOWBase(CMdxPointer inP, int nSize);
		CVector GetTrans(int nAnimID, DWORD dwTime);
		CQuaternion GetQuat(int nAnimID, DWORD dwTime);
		CVector GetScale(int nAnimID, DWORD dwTime);
		
		bool m_bCalc;
		bool m_bUseTrans;
		bool m_bUseScale;
		bool m_bUseQuat;
		CMatrix m_matBone;
		
		int m_nTransTimeNum;
		int m_nScaleTimeNum;
		int m_nQuatTimeNum;
		int m_nTransRangeNum;
		int m_nScaleRangeNum;
		int m_nQuatRangeNum;
		int m_nTransNum;
		int m_nScaleNum;
		int m_nQuatNum;

		CVector m_vPivot;
		DWORD *m_puTransTime;
		DWORD *m_puScaleTime;
		DWORD *m_puQuatTime;
		MdxAnimRange *m_pTransRange;
		MdxAnimRange *m_pScaleRange;
		MdxAnimRange *m_pQuatRange;
		CVector *m_pTrans;
		CVector *m_pScale;
		CQuaternion *m_pQuat;
	};

	class OgreExport CMdxSkeleton
	{
	public:
		CMdxSkeleton(CMdxMesh *pMdx);
		~CMdxSkeleton();

		enum
		{
			SKELETONTYPE_NONE = -1,
			SKELETONTYPE_WOW = 0,
			SKELETON_BONE_WOW = 'wowb',
		};

		void ReadSkeleton(CMdxPointer inP, int nSize);
		void CalcAllBone(int nAnimID, DWORD dwTime);
		CBaseBone* GetBone(int nID);
		CMatrix GetMatrix(int nID);
	private:
		void ReadBone(CMdxPointer inP, int nSize);

		CMdxMesh *m_pSrcMdx;
		int m_nSketType;
		
		typedef std::vector<CBaseBone*> BoneList;
		BoneList m_vecBone;
	};

	class OgreExport CBaseAnim
	{
	public:
		CBaseAnim();
		~CBaseAnim();

		enum
		{
			ANIM_NAME_SIZE = 50,
		};

		virtual void ReadAnim(CMdxPointer inP, int iSize) = 0;
		char* GetName();
	protected:
		char *m_pszName;
	};

	class OgreExport CWOWAnim : public CBaseAnim
	{
	public:
		CWOWAnim();
		~CWOWAnim();

		virtual void ReadAnim(CMdxPointer inP, int iSize);
		DWORD GetStartTime();
		DWORD GetEndTime();
	private:
		DWORD m_nStartTime;
		DWORD m_nEndTime;
	};

	class OgreExport CMdxAnimSet
	{
	public:
		CMdxAnimSet();
		~CMdxAnimSet();

		enum
		{
			ANIMSET_ANIM = 'anim',
			ANIMSET_TYPE_NONE = 0,
			ANIMSET_TYPE_WOW = 1,
		};

		void ReadAnimSet(CMdxPointer inP, int iSize);
		CBaseAnim* FindAnim(int nID);
		int GetAnimNum();
		int GetType();
	private:
		void ReadAnim(CMdxPointer inP, int iSize);

		int m_nAnimType;
		typedef std::vector<CBaseAnim*> AnimList;
		typedef std::map<char*, CBaseAnim*> AnimMap;
		typedef std::map<int, CBaseAnim*> AnimIDMap;
		AnimList m_vecAnim;
		AnimMap m_mapAnim;
		AnimIDMap m_mapAnimID;
	};

	class OgreExport CMdxGeoChunk
	{
	public:		
		CMdxGeoChunk(int id, CMdxMesh *pMdx);
		~CMdxGeoChunk();
		
		enum
		{
			CHUNK_NAME_SIZE = 80,
			CHUNK_BASE_INFO	= 'base',
			CHUNK_BONE_INFO = 'bone',
		};
		
		void ReadChunk(CMdxPointer inP, int size);
		int GetVertexNum();
		int GetIndexNum();
		int GetMaterialId();
		CVector* GetVertices();
		CVector* GetNormals();
		CVector2D* GetUVs();
		WORD* GetIndices();
		byte* GetBoneWeight();
		byte* GetBoneIndex();
		bool HasBoneInfo();
		CRenderOperation* GetRenderOp(){return m_pRenderOp;}
	private:
		void ReadBaseInfo(CMdxPointer inP, int size);
		void ReadBoneInfo(CMdxPointer inP, int size);

		CRenderOperation *m_pRenderOp;

		CMdxMesh *m_pSrcMdx;
		int m_nChunkID;
		int m_nMaterialId;

		char *m_pszName;
		int m_nVertexNum;
		int m_nIndexNum;
		CVector *m_pVertices;
		CVector *m_pNormals;
		CVector2D *m_pUVs;
		DWORD *m_pDiffuse;
		WORD *m_pIndices;
		DWORD *m_pLIndices;
		byte *m_pBoneWeight;
		byte *m_pBoneIndex;

		bool m_bUseLIndex;
		bool m_bHasBoneInfo;
	};

	class OgreExport CMdxGeometry
	{
	public:	
		CMdxGeometry(CMdxMesh *pMdx);
		~CMdxGeometry();
		
		enum
		{
			GEOMETRY_CHUNK = 'chks',
		};
		
		void ReadGeometry(CMdxPointer inP, int size);
		CMdxGeoChunk* GetChunk(int id);
		int GetChunkNum();
	private:
		void ReadChunk(CMdxPointer inP, int size);

		CMdxMesh *m_pSrcMdx;
		typedef std::vector<CMdxGeoChunk*> ChunkList;
		ChunkList m_vecChunk;
	};

	class OgreExport CWOWColorGroup
	{
	public:
		CWOWColorGroup(CMdxMesh *pMdx);
		~CWOWColorGroup();

		enum
		{
			WOWCOLOR_COLOR = 'colr',
		};

		struct CWOWColor
		{
			CWOWColor();
			~CWOWColor();

			int nOpacityTimeNum;
			int nOpacityRangeNum;
			int nOpacityNum;
			DWORD *puOpacityTime;
			MdxAnimRange *pOpacityRange;
			float *pOpacity;
		};

		void ReadWOWColor(CMdxPointer inP, int size);
		float GetColor(int nID, int nAnimId);
	private:
		CMdxMesh *m_pSrcMdx;

		typedef std::vector<CWOWColor*> ColorList;
		ColorList m_vecColor;
	};

	struct CMdxMeshFormat
	{
		CVector vPos;
		CVector vNormal;
		CVector2D vTex;
	};

	class OgreExport CMdxMesh : public CResource
	{
	public:	
		CMdxMesh(CString szGroup, CString szName);
		~CMdxMesh();	
		
		enum
		{
			GEOMETRY = 'geom',
			MATERIALS = 'mtrl',
			SKELETON = 'sket',
			ANIMSET = 'anim',
			WOWCOLOR = 'wowc',
		};

		virtual bool LoadImpl();
		
		void UpdateSkeleton(int nAnimID, bool bSingleFrame, int nFrame);
		int GetFrame(){return m_nFrame;}
		CMdxGeometry* GetGeometry(){return m_pGeometry;}
		CMdxMaterials* GetMaterials(){return m_pMaterials;}
		CMdxSkeleton* GetSkeleton(){return m_pSkeleton;}
		CMdxAnimSet* GetAnimSet(){return m_pAnimSet;}
		CWOWColorGroup* GetWOWColorGroup(){return m_pWOWColorGroup;}
		DWORD GetCurAnimStartTime(int nAnimId);
		DWORD GetCurAnimEndTime(int nAnimId);
	private:
		bool LoadFromMdxFile(BYTE* pbyBuffer, int nBufferSize);
		bool LoadFromSketFile(byte *pbyBuffer, int nBufSize);
		void ReadGeometry(CMdxPointer inP, int size);
		void ReadMaterials(CMdxPointer inP, int iSize);
		void ReadWOWColor(CMdxPointer inP, int iSize);
		void ReadSkeleton(CMdxPointer inP, int iSize);
		void ReadAnimSet(CMdxPointer inP, int iSize);

		int m_nFrame;
		CMemoryBlock m_GeometryMB;
		CMdxGeometry *m_pGeometry;
		CMemoryBlock m_MaterialsMB;
		CMdxMaterials *m_pMaterials;
		CMemoryBlock m_SkeletonMB;
		CMdxSkeleton *m_pSkeleton;
		CMemoryBlock m_AnimSetMB;
		CMdxAnimSet *m_pAnimSet;
		CMemoryBlock m_WOWColorMB;
		CWOWColorGroup *m_pWOWColorGroup;
	};
}
#endif