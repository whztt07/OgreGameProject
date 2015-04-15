

#ifndef __OgreMdx_h__
#define __OgreMdx_h__

#include "OgrePrerequisites.h"
#include "OgreFile.h"
#include "OgreMath.h"
#include "OgreUseful.h"

namespace Ogre
{
	#define MDX_MAX_NAME 80
	class CMdxSkeleton;
	class CMdxUV;
	class CMdxFace;
	class CMdx;

	union OgreExport CMdxPointer
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
		CMdxFace*		face;
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
		BYTE*	m_pbyBuffer;
		int		m_nBufferSize;
	};

	class OgreExport CMdxTexture
	{
	public:
		CMdxTexture();
		~CMdxTexture();

		void SetName( const char* pszName );
		char* GetName();
	private:
		char m_szName[MAX_PATH];
	};

	class OgreExport CMdxTextures 
	{
	public:
		CMdxTextures();
		~CMdxTextures();

		BOOL Read( CMdxPointer inP, int nSize );
		int GetTextureCount();
		CMdxTexture* GetTexture( int i );
		CMdxTexture* AddTexture();
	private:
		std::vector<CMdxTexture> m_vecTexture;
		int m_nNum;
	};

	class OgreExport CMdxBone
	{
	public:
		CMdxBone();
		~CMdxBone();

		BOOL Read(CMdxPointer inP, int nSize);
		CMatrix GetFrame0Inv();
		CMatrix GetMatrices( int nFrameId );
	public:
		CMdxSkeleton* m_pSkeleton;
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

	class OgreExport CMdxSkeleton
	{
	public:
		CMdxSkeleton();
		~CMdxSkeleton();

		BOOL Read( CMdxPointer inP, int nSize );
		int GetBoneNum();
		CMdxBone* GetBone(int nBone);
	private:
		int m_nRootBoneCount;
		int* m_pnRootBoneIds;
		std::vector<CMdxBone*> m_vecBone;
	};

	class OgreExport CMdxBoneGroup
	{
	public:
		CMdxBoneGroup();
		~CMdxBoneGroup();

		BOOL Read(CMdxPointer inP, int nSize);
	private:
		int m_nBoneCount;
		int* m_pnBoneIds;
	};

	class OgreExport CMdxBoneGroups
	{
	public:
		CMdxBoneGroups();
		~CMdxBoneGroups();

		BOOL Read( CMdxPointer inP, int nSize );
	private:
		std::vector<CMdxBoneGroup> m_vecBoneGroup;
	};

	struct CMdxUV
	{
		float u, v;
	};

	struct CMdxFace
	{
		short nId[3];
	};

	class OgreExport CMdxGeoChunk
	{
	public:
		CMdxGeoChunk();
		~CMdxGeoChunk();

		char* GetName();
		int GetVertexNum();
		int GetFaceNum();
		int GetMtlID();
		CVector* GetVertex(int index);
		CMdxFace* GetFace(int index);
		CMdxUV* GetUV(int layer, int index);

		enum
		{
			eMaxUVLayer = 2,
		};

		BOOL Read( CMdxPointer inP, int nSize );
	public:
		int m_nChunkId;
		CMdx* m_pSrcMdx;
	private:
		char* m_pszName;
		int m_nVertexCount;
		int m_nFaceCount;
		int m_nMtlId;
		CVector* m_pVertices;
		CVector* m_pNormals;
		DWORD* m_pColors;
		DWORD* m_pOriginColors;
		CMdxUV* m_pUVs[eMaxUVLayer];
		BYTE* m_pBoneGroupIndices;
		CMdxFace* m_pFaces;
		int m_nUVLayerCount;
	};

	class OgreExport CMdxGeometry
	{
	public:
		CMdxGeometry();
		~CMdxGeometry();

		int GetChunkCount();
		CMdxGeoChunk *GetChunk(int i);
		BOOL Read( CMdxPointer inP, int nSize );
	public:
		CMdx* m_pSrcMdx;
	private:
		std::vector<CMdxGeoChunk*> m_vecChunk;
	};

	class OgreExport CMdx
	{
	public:
		CMdx();
		~CMdx();
	public:
		void Destroy();
		BOOL LoadFromFile( const char* pszFilename, DWORD dwFlag = 0 );
		BOOL LoadFromMemory( BYTE* pbyBuffer, int nBufferSize );
		CMdxTextures* GetTextures();
		CMdxSkeleton* GetSkeleton();
		CMdxBoneGroups* GetBoneGroups();
		CMdxGeometry* GetGeometry();
		char* GetFileName();

		void SetFrameId( int nFrameId );
	private:
		char m_szFileName[MAX_PATH];

		CMemoryBlock m_TexturesMB;
		CMdxTextures *m_textures;
		CMemoryBlock m_SkeletonMB;
		CMdxSkeleton* m_pSkeleton;
		CMemoryBlock m_BoneGroupsMB;
		CMdxBoneGroups* m_pBoneGroups;
		CMemoryBlock m_GeometryMB;
		CMdxGeometry* m_pGeometry;

		int m_nFrameId;
	};
}

#endif