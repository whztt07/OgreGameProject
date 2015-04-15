

#ifndef __World_h__
#define __World_h__

#include "Ogre3d.h"
#include "mpq_libmpq.h"

#define TILE_MAX_X 64
#define TILE_MAX_Y 64
#define MAPTILECACHESIZE 16

#define TILESIZE (533.33333f)
#define CHUNKSIZE ((TILESIZE) / 16.0f)
#define UNITSIZE (CHUNKSIZE / 8.0f)
#define ZEROPOINT (32.0f * (TILESIZE))
const int mapbufsize = 9*9 + 8*8;

class MapTile;

typedef unsigned char uint8;
typedef char int8;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned int uint32;
typedef int int32;

struct MapChunkHeader 
{
	uint32 flags;
	uint32 ix;
	uint32 iy;
	uint32 nLayers;
	uint32 nDoodadRefs;
	uint32 ofsHeight;
	uint32 ofsNormal;
	uint32 ofsLayer;
	uint32 ofsRefs;
	uint32 ofsAlpha;
	uint32 sizeAlpha;
	uint32 ofsShadow;
	uint32 sizeShadow;
	uint32 areaid;
	uint32 nMapObjRefs;
	uint32 holes;
	uint16 s1;
	uint16 s2;
	uint32 d1;
	uint32 d2;
	uint32 d3;
	uint32 predTex;
	uint32 nEffectDoodad;
	uint32 ofsSndEmitters;
	uint32 nSndEmitters;
	uint32 ofsLiquid;
	uint32 sizeLiquid;
	float  zpos;
	float  xpos;
	float  ypos;
	uint32 textureId;
	uint32 props;
	uint32 effectId;
};

struct sTexCoords
{
	Ogre::CVector2D detailTexCoord;
	Ogre::CVector2D alphaTexCoord;
};

class MapChunk
{
public:
	MapChunk();
	~MapChunk();

	void Init(MapTile* mt, MPQFile &f);
	void InitStrip(int holes);
	void InitStrip2(int holes);
	void RenderChunk();

	Ogre::CVector vmin, vmax, vcenter;

	int nTextures;

	float xbase, ybase, zbase;
	float r;

	unsigned int areaID;

	bool haswater;
	bool visible;
	bool hasholes;
	float waterlevel;

	int textures[4];
	int alphamaps[3];
	int shadow, blend;

	int animated[4];

	unsigned int vertices, normals;
	int m_nNumIndex;

	short *strip;
	int striplen;

	struct sNormal
	{
		std::vector<Ogre::CVector> vecNormal;
	};
	sNormal m_vecNormalGroup[mapbufsize];
	Ogre::CVector m_vecNormal[mapbufsize];

	LPDIRECT3DVERTEXBUFFER9 m_pVertexBuffer;
	LPDIRECT3DINDEXBUFFER9 m_pIndexBuffer;
	LPDIRECT3DVERTEXDECLARATION9 m_pVertexDecl;
	LPDIRECT3DTEXTURE9 m_pBlendTex;
};

class MapTile 
{
public:
	MapTile(int x0, int z0, char* filename);
	~MapTile();

	void RenderTile();

	std::vector<std::string> textures;
	MapChunk m_pChunks[16][16];
	float xbase, zbase;
	int x, z;
	bool ok;
};

enum
{
	COMPRESSED_DXT1,
	COMPRESSED_DXT3,
	COMPRESSED_DXT5,
	UNCOMPRESSED_RGBA8,
};

class CWOWTexture
{
public:
	CWOWTexture(std::string name);
	~CWOWTexture();

	std::string m_strName;
	int m_nWidth;
	int m_nHeight;
	bool m_bCompressed;
};

class CWOWTextureManager
{
public:
	CWOWTextureManager();
	~CWOWTextureManager();

	int Add(std::string name);
	void Del(int nTexID);
	LPDIRECT3DTEXTURE9 FindD3D9Texture(int nID);
	CWOWTexture* FindTexture(int nID);
	int FindTextureID(std::string name);
	void GetPixel(unsigned char *buf, int nTexID);
	void ComposePixel(unsigned char *buf, int width, int height, int& nTexID);
private:
	bool LoadBLP(CWOWTexture *tex);
	void CreateD3D9Texture(int width, int height, int numMip, int format, std::vector<int> , std::vector<unsigned char*> vecBuf);
	void CreateD3D9Texture2(int width, int height, int numMip, int format, std::vector<int> , std::vector<unsigned int*> vecBuf);

	typedef std::map<std::string, int> NameMap;
	typedef std::map<int, LPDIRECT3DTEXTURE9> TextureIDMap;
	typedef std::map<int, CWOWTexture*> TextureMap;
	NameMap m_mapName;
	TextureIDMap m_mapTextureID;
	TextureMap m_mapTexture;
	int nIDCount;
};

class CShaderMgr
{
public:
	CShaderMgr();
	~CShaderMgr();

	void Init();
	
	LPD3DXEFFECT m_pTerrainShader;
};

class CWorld
{
public:
	CWorld(char *name);
	~CWorld();

	void LoadRes();
	bool Oktile(int i, int j);
	void EnterTile(int x, int z);
	MapTile *LoadTile(int x, int z);
	void RenderWorld();
	void InitGlobalVBOs();
	
	short *mapstrip, *mapstrip2;
	LPDIRECT3DVERTEXBUFFER9 m_pDetailTexCoord;
private:
	Ogre::CString m_szName;
	
	Ogre::CVector m_vEye;
	Ogre::CVector m_vLookat;
	int m_nNumWMO, m_nNumMap;
	int cx,cz;
	bool oob;

	bool m_bMaps[TILE_MAX_X][TILE_MAX_Y];
	MapTile *maptilecache[MAPTILECACHESIZE];
	MapTile *m_pCurrent[3][3];
};

int indexMapBuf(int x, int y);
// 8x8x2 version with triangle strips, size = 8*18 + 7*2
const int stripsize = 8*18 + 7*2;
template <class V> void stripify(V *in, V *out)
{
	for (int row=0; row<8; row++) 
	{
		V *thisrow = &in[indexMapBuf(0,row*2)];
		V *nextrow = &in[indexMapBuf(0,(row+1)*2)];

		if (row>0) *out++ = thisrow[0];
		for (int col=0; col<9; col++)
		{
			*out++ = thisrow[col];
			*out++ = nextrow[col];
		}

		if (row<7) 
			*out++ = nextrow[8];
	}
}

const int stripsize3 = 4 * 8 * 8 * 3;
template <class V> void stripify3(V *out)
{
	for (int row = 0; row < 8; row++)
	{
		for (int col = 0; col < 8; col++)
		{
			//---------------------
			//----	i--------------(i+1)
			//----	|				|
			//----	|------9+i------|
			//----	|				|
			//----(9+8)+i---------(9+8)+i+1
			//---------------------
			int r1 = row * (9 + 8) + col;
			int r2 = r1 + 1;
			int r3 = r1 + (9 + 8);
			int r4 = r2 + (9 + 8);
			int middle = r1 + 9;
			
			*out++ = middle;
			*out++ = r1;
			*out++ = r3;

			*out++ = middle;
			*out++ = r3;
			*out++ = r4;

			*out++ = middle;
			*out++ = r4;
			*out++ = r2;

			*out++ = middle;
			*out++ = r2;
			*out++ = r1;
		}
	}
}

// high res version, size = 16*18 + 7*2 + 8*2
const int stripsize2 = 16*18 + 7*2 + 8*2;
template <class V> void stripify2(V *in, V *out)
{
	for (int row=0; row<8; row++) 
	{ 
		V *thisrow = &in[indexMapBuf(0,row*2)];
		V *nextrow = &in[indexMapBuf(0,row*2+1)];
		V *overrow = &in[indexMapBuf(0,(row+1)*2)];

		if (row>0) 
			*out++ = thisrow[0];// jump end
		
		for (int col=0; col<8; col++) 
		{
			*out++ = thisrow[col];
			*out++ = nextrow[col];
		}
		
		*out++ = thisrow[8];
		*out++ = overrow[8];
		*out++ = overrow[8];// jump start
		*out++ = thisrow[0];// jump end
		*out++ = thisrow[0];
		
		for (int col=0; col<8; col++) 
		{
			*out++ = overrow[col];
			*out++ = nextrow[col];
		}

		if (row<8)
			*out++ = overrow[8];
		
		if (row<7) 
			*out++ = overrow[8];// jump start
	}
}

extern CWorld *g_pWorld;

#endif