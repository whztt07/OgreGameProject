

#ifndef __OgreMSMdx_h__
#define __OgreMSMdx_h__

#include "OgrePrerequisites.h"
#include "OgreFile.h"
#include "OgreMath.h"
#include "OgreUseful.h"

namespace Ogre
{
	#define MDX_MAX_NAME 80
	class CMSMdxSkeleton;
	struct CMSMdxUV;
	struct CMSMdxFace;
	class CMSMdx;

	union OgreExport CMSMdxPointer
	{
		CMSMdxPointer(void* in);

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
		CMSMdxUV*	uv;
		CMSMdxFace*		face;
	};

	class OgreExport CMSMemoryBlock
	{
	public:
		CMSMemoryBlock();
		~CMSMemoryBlock();
		
		BOOL Create( BYTE* pbyBuffer, int nBufferSize );
		CMSMdxPointer GetPointer();
		int GetBufferSize();
	private:
		BYTE*	m_pbyBuffer;
		int		m_nBufferSize;
	};

	class OgreExport CMSMdxTexture
	{
	public:
		CMSMdxTexture();
		~CMSMdxTexture();

		void SetName( const char* pszName );
		char* GetName();
	private:
		char m_szName[MAX_PATH];
	};

	class OgreExport CMSMdxTextures 
	{
	public:
		CMSMdxTextures();
		~CMSMdxTextures();

		BOOL Read( CMSMdxPointer inP, int nSize );
		int GetTextureCount();
		CMSMdxTexture* GetTexture( int i );
		CMSMdxTexture* AddTexture();
	private:
		std::vector<CMSMdxTexture> m_vecTexture;
		int m_nNum;
	};

	class OgreExport CMSMdxBone
	{
	public:
		CMSMdxBone();
		~CMSMdxBone();

		BOOL Read(CMSMdxPointer inP, int nSize);
		CMatrix GetFrame0Inv();
		CMatrix GetMatrices( int nFrameId );
	public:
		CMSMdxSkeleton* m_pSkeleton;
		int m_nBoneId;

		char m_szName[MDX_MAX_NAME];
		int m_nParentId;
		int m_nChildCount; 
		int* m_pnChildIds;
		int m_nFrameCount;
		CMatrix *m_pMatrices;
		CVector* m_pTrans;
		CQuaternion* m_pQuats;
		CMatrix m_matFrame0Inv;
		BOOL* m_pVisible;
	};

	class OgreExport CMSMdxSkeleton
	{
	public:
		CMSMdxSkeleton();
		~CMSMdxSkeleton();

		BOOL Read( CMSMdxPointer inP, int nSize );
		int GetBoneNum();
		CMSMdxBone* GetBone(int nBone);
	private:
		int m_nRootBoneCount;
		int* m_pnRootBoneIds;
		std::vector<CMSMdxBone*> m_vecBone;
	};

	class OgreExport CMSMdxBoneGroup
	{
	public:
		CMSMdxBoneGroup();
		~CMSMdxBoneGroup();

		BOOL Read(CMSMdxPointer inP, int nSize);
	private:
		int m_nBoneCount;
		int* m_pnBoneIds;
	};

	class OgreExport CMSMdxBoneGroups
	{
	public:
		CMSMdxBoneGroups();
		~CMSMdxBoneGroups();

		BOOL Read( CMSMdxPointer inP, int nSize );
	private:
		std::vector<CMSMdxBoneGroup> m_vecBoneGroup;
	};

	struct CMSMdxUV
	{
		float u, v;
	};

	struct CMSMdxFace
	{
		short nId[3];
	};

	class OgreExport CMSMdxGeoChunk
	{
	public:
		CMSMdxGeoChunk();
		~CMSMdxGeoChunk();

		enum
		{
			eMaxUVLayer = 2,
		};

		BOOL Read( CMSMdxPointer inP, int nSize );
	public:
		int m_nChunkId;
		CMSMdx* m_pSrcMdx;
	private:
		char* m_pszName;
		int m_nVertexCount;
		int m_nFaceCount;
		int m_nMtlId;
		CVector* m_pVertices;
		CVector* m_pNormals;
		DWORD* m_pColors;
		DWORD* m_pOriginColors;
		CMSMdxUV* m_pUVs[eMaxUVLayer];
		BYTE* m_pBoneGroupIndices;
		CMSMdxFace* m_pFaces;
		int m_nUVLayerCount;
	};

	class OgreExport CMSMdxGeometry
	{
	public:
		CMSMdxGeometry();
		~CMSMdxGeometry();

		int GetChunkCount();
		CMSMdxGeoChunk *GetChunk(int i);
		BOOL Read( CMSMdxPointer inP, int nSize );
	public:
		CMSMdx* m_pSrcMdx;
	private:
		std::vector<CMSMdxGeoChunk*> m_vecChunk;
	};

	class OgreExport CMSMdx
	{
	public:
		CMSMdx();
		~CMSMdx();
	public:
		void Destroy();
		BOOL LoadFromFile( const char* pszFilename, DWORD dwFlag = 0 );
		BOOL LoadFromMemory( BYTE* pbyBuffer, int nBufferSize );
		CMSMdxTextures* GetTextures();
		CMSMdxSkeleton* GetSkeleton();
		CMSMdxBoneGroups* GetBoneGroups();
		CMSMdxGeometry* GetGeometry();

		void SetFrameId( int nFrameId );
	private:
		char m_szFileName[MAX_PATH];

		CMSMemoryBlock m_TexturesMB;
		CMSMdxTextures *m_textures;
		CMSMemoryBlock m_SkeletonMB;
		CMSMdxSkeleton* m_pSkeleton;
		CMSMemoryBlock m_BoneGroupsMB;
		CMSMdxBoneGroups* m_pBoneGroups;
		CMSMemoryBlock m_GeometryMB;
		CMSMdxGeometry* m_pGeometry;

		int m_nFrameId;
	};
}

#endif