#pragma once

#include "MaxInterface.h"

enum
{	
	OGRE_MDX_WIREFRAME			=	1<<0,
	OGRE_MDX_TWOSIDED			=	1<<1,
	OGRE_MDX_ALPHABLEND			=	1<<2,
	OGRE_MDX_ALPHATEST			=	1<<3,
};

#define MDX_MAX_NAME 80

class CMdxCandidate
{
public:
	CMdxCandidate(void);
	~CMdxCandidate(void);

	struct CTexture
	{
		CTexture()
		{
			memset( szName, 0x00, sizeof( szName ) );
		}

		char szName[MAX_PATH];
	};

	struct CTextures
	{
	public:
		int AddTexture( CTexture& t );
		int GetTextureCount();
		CTexture* GetTexture( int i );
	protected:
		std::vector<CTexture> vectorTexture;
		std::vector<int> vectorTextureId;
	};

	struct CMaterial
	{
		CMaterial()
		{
		}

		std::vector<CMaxInterface::CColorTrack> vectorColorTrack;
	};

	struct CMaterials
	{
		std::vector<CMaterial> vectorMtl;
	};

	struct CMtlFace
	{
		short nMaxVertexId[3];
	};

	struct CSplitVertex
	{
		int nVertexId;
		int nParentVertID;
		Point3 pos;
		Point3 normal;
		DWORD color;
		float u, v;
		CMaxInterface::CBoneGroup bg;
	};

	struct CAttachment
	{
		CAttachment()
		{
			pNode = NULL;
			nAttachBoneID = -1;
			szName[0] = 0;
		}

		INode* pNode;
		int nAttachBoneID;
		char szName[MAX_PATH];
		CMatrix matInit;
	};

	struct CAttachments
	{
		std::vector<CAttachment> vectorAttachment;
	};

	struct CMdxUV
	{
		float u, v;
	};

	struct CBoneGroups
	{
		std::vector<CMaxInterface::CBoneGroup> vectorBoneGroup;
	};

	struct CBone
	{
		CBone()
		{
			memset( szName, 0x00, sizeof( szName ) );
			memset( szParentName, 0x00, sizeof( szParentName ) );
		}

		INode* pNode;
		char szName[MAX_PATH];
		char szParentName[MAX_PATH];
		int nParentId;
		std::vector<int> vectorChildId;
		CMaxInterface::CTrack track;
	};

	struct CSkeleton
	{	
		int FindBone( const char* pszBoneName );

		std::vector<CBone> vectorBone;
		std::vector<int> vectorRootBoneId;
	};

	struct CGeomChunk
	{	
		CGeomChunk()
		{
			szNodename[0] = 0;
			pStdMtl = NULL;
			nMtlId = -1;
			nModifierType = -1;
		};

		char szNodename[MAX_PATH];
		StdMat* pStdMtl;
		int nMtlId;
		int nModifierType;
		std::vector<CSplitVertex> vectorSplitVertex;
		std::vector<CMtlFace> vectorFace;
		std::vector<Point3> vectorVertex;
		std::vector<Point3> vectorNormal;
		std::vector<CMdxUV> vectorUV;
		std::vector<DWORD> vectorColor;
		std::vector<byte> vectorBGId;
	};

	struct CGeometry
	{
		std::vector<CGeomChunk> vectorChunk;
	};

	class CVertexNormal 
	{
	public:	
		CVertexNormal();
		CVertexNormal(Point3 &n,DWORD s);
		~CVertexNormal();
		
		void AddNormal(Point3 &n,DWORD s);
		Point3 &GetNormal(DWORD s);
		void Normalize();
		
		Point3 norm;
		DWORD smooth;
		CVertexNormal *next;
		BOOL init;
	};

	union CMdxPointer
	{
		CMdxPointer(void* in);

		BYTE*			byte;
		bool*			boolean;
		int*			i;
		DWORD*			dw;
		char*			c;
		void*			p;
		float*			f;
		CMatrix*		matrix;
		CVector*		v3;
		CQuaternion*	quat;
		CMdxUV*			uv;
		CMtlFace*		face;
	};

	void CreateMdx(INode *pSelNode);
	bool SaveMdx(char *szFileName);
private:
	void CreateGeometry(INode *pNode);
	void CollectBoneAndAttachment(INode *pNode);
	int FindTexture( CTexture* pTexture );
	int FindBoneGroup( CMaxInterface::CBoneGroup* infls );
	int FindBone( const char* pszBoneName );
	int FindMtl( CMaterial* pMtl );
	void ProjBoneGroups();

	CTextures	m_Textures;
	CMaterials	m_Mtls;
	CAttachments m_Attachments;
	CBoneGroups	m_BoneGroups;
	CGeometry	m_Geometry;
	CSkeleton	m_Skeleton;
	std::vector<INode*> m_vecBone;
};
