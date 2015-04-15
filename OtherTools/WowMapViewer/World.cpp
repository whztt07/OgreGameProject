

#include "World.h"
#include "mpq_libmpq.h"
#include "wowmapviewer.h"
#include "OgreCamera.h"

int holetab_h[4] = {0x1111, 0x2222, 0x4444, 0x8888};
int holetab_v[4] = {0x000F, 0x00F0, 0x0F00, 0xF000};

bool isHole(int holes, int i, int j)
{
	return (holes & holetab_h[i] & holetab_v[j]) != 0;
}

int indexMapBuf(int x, int y)
{
	return ((y + 1) / 2) * 9 + (y / 2) * 8 + x;
}

void fixname(std::string &name)
{
	for (size_t i=0; i<name.length(); i++) 
	{
		if (i>0 && name[i]>='A' && name[i]<='Z' && isalpha(name[i-1])) 
		{
			name[i] |= 0x20;
		} 
		else if ((i==0 || !isalpha(name[i-1])) && name[i]>='a' && name[i]<='z') 
		{
			name[i] &= ~0x20;
		}
	}
}

D3DVERTEXELEMENT9 ModelVertexElements[] = 
{
	{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	//{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
	{1, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	{1, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
	//{0, 32, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0 },
	//{0, 36, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	D3DDECL_END()
};

CWorld *g_pWorld = NULL;
Ogre::CCamera gCamera;
extern float g_fAspect;
CWOWTextureManager g_TextureManager;
CShaderMgr g_ShaderMgr;

//------------------------------------
//-------------CWorld-----------------
//------------------------------------
CWorld::CWorld(char *name)
{
	m_szName = name;
	oob = false;
	cx = 0;
	cz = 0;

	for (int i = 0; i < MAPTILECACHESIZE; i++) 
	{
		maptilecache[i] = 0;
	}

	// default strip indices
	short *defstrip = new short[stripsize];
	for (int i=0; i<stripsize; i++) 
		defstrip[i] = i; // note: this is ugly and should be handled in stripify
	mapstrip = new short[stripsize];
	stripify<short>(defstrip, mapstrip);

	//defstrip = new short[stripsize2];
	//for (int i=0; i<stripsize2; i++)
	//	defstrip[i] = i; // note: this is ugly and should be handled in stripify
	//mapstrip2 = new short[stripsize2];
	//stripify2<short>(defstrip, mapstrip2);

	mapstrip2 = new short[stripsize3];
	stripify3<short>(mapstrip2);

	delete[] defstrip;
	defstrip = NULL;

	LoadRes();
	InitGlobalVBOs();
}

CWorld::~CWorld()
{

}

void CWorld::LoadRes()
{
	char fn[256];
	sprintf(fn,"World\\Maps\\%s\\%s.wdt", m_szName.c_str(), m_szName.c_str());

	memset(m_bMaps, 0, sizeof(m_bMaps));

	MPQFile f(fn);
	char fourcc[5];
	size_t size;
	m_nNumMap = 0;

	while (!f.isEof()) 
	{
		f.read(fourcc,4);
		f.read(&size, 4);

		flipcc(fourcc);
		fourcc[4] = 0;

		size_t nextpos = f.getPos() + size;

		if (!strcmp(fourcc,"MAIN")) 
		{
			for (int j = 0; j < TILE_MAX_Y; j++) 
			{
				for (int i = 0; i < TILE_MAX_X; i++)
				{
					int d;
					f.read(&d, 4);
					if (d) 
					{
						m_bMaps[j][i] = true;
						m_nNumMap++;
					} 
					else
						m_bMaps[j][i] = false;

					f.read(&d, 4);
				}
			}
		}
		else if (!strcmp(fourcc,"MODF")) 
		{
			// global wmo instance data
			m_nNumWMO = (int)size / 64;
		}

		f.seek((int)nextpos);
	}

	f.close();
}

bool CWorld::Oktile(int i, int j)
{
	return i>=0 && j >= 0 && i<64 && j<64;
}

void CWorld::EnterTile(int x, int z)
{
	if (!Oktile(x,z)) 
	{
		oob = true;
		return;
	} 
	else 
		oob = !m_bMaps[z][x];

	cx = x;
	cz = z;
	
	//for (int j=0; j<3; j++)
	//{
	//	for (int i=0; i<3; i++)
	//	{
	//		m_pCurrent[j][i] = LoadTile(x - 1 + i, z - 1 + j);
	//	}
	//}

	m_pCurrent[0][0] = LoadTile(x, z);
	//m_pCurrent[1][1] = LoadTile(x, z + 1);

	m_vEye = Ogre::CVector(x * TILESIZE, z * TILESIZE, 0.0f)
		+ Ogre::CVector(TILESIZE * 0.5f, TILESIZE * 0.5f, 0.0f);
	Ogre::CVector vc = m_pCurrent[0][0]->m_pChunks[0][0].vmax;
	m_vEye.z = vc.z + 100.0f;
	m_vLookat = m_vEye + Ogre::CVector(0.0f, -1.0f, -1.0f);
	gCamera.SetViewParam(m_vEye, m_vLookat, Ogre::CVector(0.0f, 0.0f, 1.0f));
}

MapTile *CWorld::LoadTile(int x, int z)
{
	if (!Oktile(x,z) || !m_bMaps[z][x]) 
	{
		return 0;
	}

	int firstnull = MAPTILECACHESIZE;
	for (int i=0; i<MAPTILECACHESIZE; i++)
	{
		if ((maptilecache[i] != 0)  &&
			(maptilecache[i]->x == x) &&
			(maptilecache[i]->z == z)) 
		{
			return maptilecache[i];
		}

		if (maptilecache[i] == 0 && i < firstnull) 
			firstnull = i;
	}
	
	// ok we need to find a place in the cache
	if (firstnull == MAPTILECACHESIZE) 
	{
		int score, maxscore = 0, maxidx = 0;
		// oh shit we need to throw away a tile
		for (int i=0; i<MAPTILECACHESIZE; i++) 
		{
			score = abs(maptilecache[i]->x - cx) + abs(maptilecache[i]->z - cz);
			if (score>maxscore)
			{
				maxscore = score;
				maxidx = i;
			}
		}

		// maxidx is the winner (loser)
		delete maptilecache[maxidx];
		firstnull = maxidx;
	}

	// TODO: make a loader thread  or something :(

	char name[256];
	sprintf(name,"World\\Maps\\%s\\%s_%d_%d.adt", m_szName.c_str(), m_szName.c_str(), x, z);

	maptilecache[firstnull] = new MapTile(x,z,name);
	return maptilecache[firstnull];
}

void CWorld::RenderWorld()
{
	D3DXMATRIX mView;
	D3DXMATRIX mProj;

	D3DXVECTOR3 vEye = gCamera.GetEye();
	D3DXVECTOR3 vDir = gCamera.GetViewDir();
	D3DXVECTOR3 vLookat = vEye + vDir;
	D3DXVECTOR3 vUp = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
	D3DXMatrixLookAtLH(&mView, &vEye, &vLookat, &vUp);
	D3DXMatrixPerspectiveFovLH(&mProj, D3DX_PI / 4, g_fAspect, 0.1f, 10000.0f);
	g_pDevice->SetTransform(D3DTS_VIEW, &mView);
	g_pDevice->SetTransform(D3DTS_PROJECTION, &mProj);

	if (m_pCurrent[0][0])
	{
		m_pCurrent[0][0]->RenderTile();
	}

	//if (m_pCurrent[1][1])
	//{
	//	m_pCurrent[1][1]->RenderTile();
	//}
}

const float detail_size = 8.0f;
void CWorld::InitGlobalVBOs()
{
	Ogre::CVector2D temp1[mapbufsize], temp2[mapbufsize], *vt;
	float tx,ty;

	// init texture coordinates for detail map:
	vt = temp1;
	const float detail_half = 0.5f * detail_size / 8.0f;
	for (int j = 0; j < 17; j++) 
	{
		for (int i = 0; i < ((j % 2) ? 8 : 9); i++)
		{
			tx = detail_size / 8.0f * i;
			ty = detail_size / 8.0f * j * 0.5f;
			if ( j %2 ) 
			{
				tx += detail_half;
			}
			
			*vt++ = Ogre::CVector2D(tx, ty);
		}
	}

	// init texture coordinates for alpha map:
	vt = temp2;
	const float alpha_half = 0.5f * 1.0f / 8.0f;
	for (int j=0; j<17; j++) 
	{
		for (int i=0; i<((j%2)?8:9); i++) 
		{
			tx = 1.0f / 8.0f * i;
			ty = 1.0f / 8.0f * j * 0.5f;
			if (j%2)
			{
				tx += alpha_half;
			}
			const int divs = 32;
			const float inv = 1.0f / divs;
			const float mul = (divs-1.0f);
			*vt++ = Ogre::CVector2D(tx*(mul*inv), ty*(mul*inv));
			//*vt++ = Ogre::CVector2D(tx, ty);
		}
	}

	g_pDevice->CreateVertexBuffer(mapbufsize * sizeof(sTexCoords),
		D3DUSAGE_WRITEONLY,
		0,
		D3DPOOL_DEFAULT,
		&m_pDetailTexCoord,
		NULL);

	sTexCoords *detailtexcoord;
	m_pDetailTexCoord->Lock(0, 0, (void**)&detailtexcoord, 0);
	for (int i = 0; i < mapbufsize; i++)
	{
		detailtexcoord[i].detailTexCoord = temp1[i];
		detailtexcoord[i].alphaTexCoord = temp2[i];
	}
	m_pDetailTexCoord->Unlock();
}

//------------------------------------
//-------------MapTile----------------
//------------------------------------
MapTile::MapTile(int x0, int z0, char* filename)
{
	xbase = x0 * TILESIZE;
	zbase = z0 * TILESIZE;

	MPQFile f(filename);
	ok = !f.isEof();
	if (!ok) 
	{
		return;
	}

	char fourcc[5];
	size_t size;

	size_t mcnk_offsets[256], mcnk_sizes[256];

	while (!f.isEof())
	{
		f.read(fourcc,4);
		f.read(&size, 4);

		flipcc(fourcc);
		fourcc[4] = 0;

		size_t nextpos = f.getPos() + size;

		if (!strcmp(fourcc,"MCIN")) 
		{
			// mapchunk offsets/sizes
			for (int i=0; i<256; i++) 
			{
				f.read(&mcnk_offsets[i],4);
				f.read(&mcnk_sizes[i],4);
				f.seekRelative(8);
			}
		}
		else if (!strcmp(fourcc,"MTEX"))
		{
			char *buf = new char[size];
			f.read(buf, size);
			char *p=buf;
			int t=0;
			while (p<buf+size) 
			{
				string texpath(p);
				p+=strlen(p)+1;
				fixname(texpath);

				// load the specular texture instead
				texpath.insert(texpath.length()-4, "_s");
				textures.push_back(texpath);
				
				g_TextureManager.Add(texpath);
				int i = 0;
			}

			delete[] buf;
		}

		f.seek((int)nextpos);
	}

	// read individual map chunks
	for (int j=0; j<16; j++) 
	{
		for (int i=0; i<16; i++) 
		{
			f.seek((int)mcnk_offsets[j*16+i]);
			m_pChunks[j][i].Init(this, f);
		}
	}

	f.close();
}

MapTile::~MapTile()
{

}

void MapTile::RenderTile()
{
	g_ShaderMgr.m_pTerrainShader->SetTechnique("RenderTerrain");

	UINT passes;
	g_ShaderMgr.m_pTerrainShader->Begin(&passes, 0);
	g_ShaderMgr.m_pTerrainShader->BeginPass(0);

	for (int y = 0; y < 16; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			m_pChunks[x][y].RenderChunk();
		}
	}

	g_ShaderMgr.m_pTerrainShader->EndPass();
	g_ShaderMgr.m_pTerrainShader->End();
}

//------------------------------------
//-------------MapChunk---------------
//------------------------------------
MapChunk::MapChunk()
{
	m_pVertexBuffer = NULL;
	m_pIndexBuffer = NULL;

	for (int i = 0; i < 4; i++)
	{
		textures[i] = -1;
	}
}

MapChunk::~MapChunk()
{

}

void MapChunk::Init(MapTile* mt, MPQFile &f)
{
	Ogre::CVector tn[mapbufsize], tv[mapbufsize];

	f.seekRelative(4);
	char fcc[5];
	size_t size;
	f.read(&size, 4);

	// okay here we go ^_^
	size_t lastpos = f.getPos() + size;

	MapChunkHeader header;
	f.read(&header, 0x80);

	areaID = header.areaid;

	zbase = header.zpos;
	xbase = header.xpos;
	ybase = header.ypos;

	int holes = header.holes;
	int chunkflags = header.flags;

	hasholes = (holes != 0);

	// correct the x and z values ^_^
	zbase = zbase*-1.0f + ZEROPOINT;
	xbase = xbase*-1.0f + ZEROPOINT;

	vmin = Ogre::CVector( 9999999.0f, 9999999.0f, 9999999.0f);
	vmax = Ogre::CVector(-9999999.0f,-9999999.0f,-9999999.0f);

	unsigned char *blendbuf = new unsigned char[64*64*4];
	memset(blendbuf, 0, 64*64*4);

	while (f.getPos() < lastpos) 
	{
		f.read(fcc,4);
		f.read(&size, 4);

		flipcc(fcc);
		fcc[4] = 0;

		size_t nextpos = f.getPos() + size;

		if (!strcmp(fcc,"MCNR"))
		{
			nextpos = f.getPos() + 0x1C0; // size fix
			// normal vectors
			char nor[3];
			for (int j = 0; j < 17; j++)
			{
				for (int i=0; i<((j % 2) ? 8 :9); i++)
				{
					f.read(nor,3);
				}
			}
		}
		else if (!strcmp(fcc,"MCVT")) 
		{
			Ogre::CVector *ttv = tv;

			// vertices
			for (int j = 0; j < 17; j++) 
			{
				for (int i = 0; i < ((j % 2) ? 8 : 9); i++) 
				{
					float h,xpos,zpos;
					f.read(&h,4);
					xpos = i * UNITSIZE;
					zpos = j * 0.5f * UNITSIZE;
					if (j % 2)
					{
						xpos += UNITSIZE*0.5f;
					}

					Ogre::CVector v = Ogre::CVector(xbase+xpos, zbase+zpos, ybase+h);
					*ttv++ = v;
					if (v.z < vmin.z) 
						vmin.z = v.z;
					
					if (v.z > vmax.z) 
						vmax.z = v.z;
				}
			}

			vmin.x = xbase;
			vmin.y = zbase;
			vmax.x = xbase + 8 * UNITSIZE;
			vmax.y = zbase + 8 * UNITSIZE;
			r = (vmax - vmin).Length() * 0.5f;
		}
		else if (!strcmp(fcc,"MCLY")) 
		{
			// texture info
			nTextures = (int)size / 16;
			for (int i=0; i<nTextures; i++)
			{
				int tex, flags;
				f.read(&tex,4);
				f.read(&flags, 4);

				f.seekRelative(8);

				flags &= ~0x100;

				if (flags & 0x80) 
				{
                    animated[i] = flags;
				} 
				else 
				{
					animated[i] = 0;
				}

				textures[i] = g_TextureManager.FindTextureID(mt->textures[tex]);
			}
		}
		else if (!strcmp(fcc,"MCSH")) 
		{
			// shadow map 64 x 64
			unsigned char sbuf[64*64], *p, c[8];
			p = sbuf;
			for (int j=0; j<64; j++)
			{
				f.read(c,8);
				for (int i=0; i<8; i++) 
				{
					for (int b=0x01; b!=0x100; b<<=1) 
					{
						*p++ = (c[i] & b) ? 0 : 255;
					}
				}
			}

			for (int p=0; p<64*64; p++)
			{
				blendbuf[p*4+3] = sbuf[p];
			}
		}
		else if (!strcmp(fcc,"MCAL")) 
		{
			// alpha maps  64 x 64
			if (nTextures>0) 
			{
				for (int i=0; i<nTextures-1; i++) 
				{
					unsigned char amap[64*64], *p;
					char *abuf = f.getPointer();
					p = amap;
					for (int j=0; j<64; j++) 
					{
						for (int i=0; i<32; i++) 
						{
							unsigned char c = *abuf++;
							*p++ = (c & 0x0f) << 4;
							*p++ = (c & 0xf0);
						}

					}

					for (int p=0; p<64*64; p++)
					{
						blendbuf[p*4 + 2 - i] = amap[p];
					}

					f.seekRelative(0x800);
				}

			} 
			else 
			{
				continue;
			}
		}
		else if (!strcmp(fcc,"MCLQ")) 
		{
			char fcc1[5];
			f.read(fcc1,4);
			flipcc(fcc1);
			fcc1[4]=0;
			if (!strcmp(fcc1,"MCSE"))
			{
				haswater = false;
			}
			else 
			{
				haswater = true;
				f.seekRelative(-4);
				f.read(&waterlevel,4);

				if (waterlevel > vmax.y) 
					vmax.z = waterlevel;
				if (waterlevel < vmin.y) 
					haswater = false;

				f.seekRelative(4);
			}
			break;
		}

		f.seek((int)nextpos);
	}

	if (hasholes)
		InitStrip2(holes);

	vcenter = (vmin + vmax) * 0.5f;

	// ....................
	HRESULT hr = g_pDevice->CreateTexture(64, 64, 1, 0, 
		D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pBlendTex, NULL);

	D3DLOCKED_RECT lockedrect;
	m_pBlendTex->LockRect(0, &lockedrect, NULL, 0);
	memcpy((void*)lockedrect.pBits, (void*)blendbuf, 64 * 64 * 4);
	m_pBlendTex->UnlockRect(0);

	g_pDevice->CreateVertexBuffer(mapbufsize * sizeof(Ogre::CVector),
		D3DUSAGE_WRITEONLY,
		0,
		D3DPOOL_DEFAULT,
		&m_pVertexBuffer,
		NULL);

	if (hasholes)
	{
		if (striplen > 0)
		{
			g_pDevice->CreateIndexBuffer(striplen * sizeof(int16),
				D3DUSAGE_WRITEONLY,
				D3DFMT_INDEX16,
				D3DPOOL_DEFAULT,
				&m_pIndexBuffer,
				NULL);		
		}
		m_nNumIndex = striplen;
	}
	else
	{
		g_pDevice->CreateIndexBuffer(stripsize3 * sizeof(int16),
			D3DUSAGE_WRITEONLY,
			D3DFMT_INDEX16,
			D3DPOOL_DEFAULT,
			&m_pIndexBuffer,
			NULL);
		m_nNumIndex = stripsize3;
	}


	Ogre::CVector *pModelVertex = NULL;
	m_pVertexBuffer->Lock(0, 0, (void**)&pModelVertex, 0);
	for (int nVertex = 0; nVertex < mapbufsize; nVertex++, pModelVertex++)
	{
		*pModelVertex = tv[nVertex];
	}
	m_pVertexBuffer->Unlock();

	if (m_nNumIndex > 0)
	{
		int16 *pIndexData = NULL;
		m_pIndexBuffer->Lock(0, 0, (void**)&pIndexData, 0);
		if (hasholes)
		{
			memcpy(pIndexData, strip, striplen * sizeof(int16));
		}
		else
		{
			memcpy(pIndexData, g_pWorld->mapstrip2, stripsize3 * sizeof(int16));
		}
		m_pIndexBuffer->Unlock();
	}

	g_pDevice->CreateVertexDeclaration(ModelVertexElements, &m_pVertexDecl);

	for (int nTri = 0; nTri < m_nNumIndex / 3; nTri++)
	{
		int n1 = g_pWorld->mapstrip2[nTri * 3 + 0];
		int n2 = g_pWorld->mapstrip2[nTri * 3 + 1];
		int n3 = g_pWorld->mapstrip2[nTri * 3 + 2];
		Ogre::CVector v1 = tv[n1];
		Ogre::CVector v2 = tv[n2];
		Ogre::CVector v3 = tv[n3];

		Ogre::CVector nor1 = v1 - v2;
		Ogre::CVector nor2 = v3 - v2;
		Ogre::CVector normal = nor1.Cross(nor2);
		normal.Normalize();

		m_vecNormalGroup[n1].vecNormal.push_back(normal);
		m_vecNormalGroup[n2].vecNormal.push_back(normal);
		m_vecNormalGroup[n3].vecNormal.push_back(normal);
	}

	for (int i = 0; i < mapbufsize; i++)
	{
		int size = m_vecNormalGroup[i].vecNormal.size();
		if (size > 0)
		{
			Ogre::CVector numNormal;
			for (int j = 0; j < size; j++)
			{
				Ogre::CVector normal = m_vecNormalGroup[i].vecNormal[j];
				numNormal += normal;
			}

			Ogre::CVector newNormal = numNormal / size;
			newNormal.Normalize();
			m_vecNormal[i] = newNormal;
		}
		else
		{
			m_vecNormal[i] = Ogre::CVector(0.0f, 0.0f, 1.0f);
		}
	}
}

void MapChunk::RenderChunk()
{
	if (g_ShaderMgr.m_pTerrainShader)
	{
		if (textures[0] != -1)
		{
			LPDIRECT3DTEXTURE9 pTex = g_TextureManager.FindD3D9Texture(textures[0]);
			g_ShaderMgr.m_pTerrainShader->SetTexture("Base", pTex);
		}

		if (textures[1] != -1)
		{
			LPDIRECT3DTEXTURE9 pTex = g_TextureManager.FindD3D9Texture(textures[1]);
			g_ShaderMgr.m_pTerrainShader->SetTexture("Layer1", pTex);
		}
		else
		{
			g_ShaderMgr.m_pTerrainShader->SetTexture("Layer1", NULL);
		}

		if (textures[2] != -1)
		{
			LPDIRECT3DTEXTURE9 pTex = g_TextureManager.FindD3D9Texture(textures[2]);
			g_ShaderMgr.m_pTerrainShader->SetTexture("Layer2", pTex);
		}
		else
		{
			g_ShaderMgr.m_pTerrainShader->SetTexture("Layer2", NULL);
		}

		if (textures[3] != -1)
		{
			LPDIRECT3DTEXTURE9 pTex = g_TextureManager.FindD3D9Texture(textures[3]);
			g_ShaderMgr.m_pTerrainShader->SetTexture("Layer3", pTex);
		}
		else
		{
			g_ShaderMgr.m_pTerrainShader->SetTexture("Layer3", NULL);
		}


		g_ShaderMgr.m_pTerrainShader->SetTexture("Blend", m_pBlendTex);
		g_ShaderMgr.m_pTerrainShader->CommitChanges();
	}

	g_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, true);
	g_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
	g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	g_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	
	g_pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	
	g_pDevice->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	
	g_pDevice->SetSamplerState(3, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(3, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

	g_pDevice->SetSamplerState(4, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(4, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

	// Í¼Æ¬ÓÐmipmap »áÉÁË¸£¨¿ªÆômipmap£©
	g_pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	g_pDevice->SetSamplerState(3, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	g_pDevice->SetVertexDeclaration(m_pVertexDecl);
	g_pDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(Ogre::CVector));
	g_pDevice->SetStreamSource(1, g_pWorld->m_pDetailTexCoord, 0, sizeof(sTexCoords));
	g_pDevice->SetIndices(m_pIndexBuffer);
	g_pDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST,
		0,
		0,
		mapbufsize,
		0,
		m_nNumIndex / 3 );
}

void MapChunk::InitStrip2(int holes)
{
	strip = new short[4 * 4 * 4 * (4 * 3)]; 
	short *s = strip;
	for (int y=0; y<4; y++)
	{
		for (int x=0; x<4; x++) 
		{
			int start = x * 2 + (9 + 8) * 2 * y;
			if (!isHole(holes, x, y))
			{
				for (int i = 0; i < 2; i++)
				{
					for (int j = 0; j < 2; j++)
					{
						int r1 = start + j + i * (9 + 8);
						int r2 = r1 + 1;
						int r3 = r1 + (9 + 8);
						int r4 = r2 + (9 + 8);
						int middle = r1 + 9;

						*s++ = middle;
						*s++ = r1;
						*s++ = r3;

						*s++ = middle;
						*s++ = r3;
						*s++ = r4;

						*s++ = middle;
						*s++ = r4;
						*s++ = r2;

						*s++ = middle;
						*s++ = r2;
						*s++ = r1;
					}
				}
			}
		}
	}

	striplen = (int)(s - strip);
}

void MapChunk::InitStrip(int holes)
{
	strip = new short[256]; // TODO: figure out exact length of strip needed
	short *s = strip;
	bool first = true;
	for (int y=0; y<4; y++)
	{
		for (int x=0; x<4; x++) 
		{
			if (!isHole(holes, x, y))
			{
				// draw tile here
				// this is ugly but sort of works
				int i = x*2;
				int j = y*4;
				for (int k=0; k<2; k++) 
				{
					if (!first) 
					{
						*s++ = indexMapBuf(i,j+k*2);
					} 
					else
						first = false;
					
					for (int l=0; l<3; l++) 
					{
						*s++ = indexMapBuf(i+l,j+k*2);
						*s++ = indexMapBuf(i+l,j+k*2+2);
					}

					*s++ = indexMapBuf(i+2,j+k*2+2);
				}
			}
		}
	}

	striplen = (int)(s - strip);
}

//--------------------------------
//----------CWOWTexture--------------
//--------------------------------
CWOWTexture::CWOWTexture(std::string name)
{
	m_strName = name;
	m_nWidth = 0;
	m_nHeight = 0;
	m_bCompressed = false;
}

CWOWTexture::~CWOWTexture()
{

}

//--------------------------------
//-------CWOWTextureManager----------
//--------------------------------
CWOWTextureManager::CWOWTextureManager()
{
	nIDCount = 0;
}

CWOWTextureManager::~CWOWTextureManager()
{

}

LPDIRECT3DTEXTURE9 CWOWTextureManager::FindD3D9Texture(int nID)
{
	if (m_mapTextureID.find(nID) != m_mapTextureID.end())
	{
		return m_mapTextureID[nID];
	}

	return NULL;
}

CWOWTexture* CWOWTextureManager::FindTexture(int nID)
{
	if (m_mapTexture.find(nID) != m_mapTexture.end())
	{
		return m_mapTexture[nID];
	}

	return NULL;
}

int CWOWTextureManager::Add(std::string name)
{
	int id = -1;

	// if the item already exists, return the existing ID
	if (m_mapName.find(name) != m_mapName.end())
	{
		id = m_mapName[name];
		return id;
	}

	CWOWTexture *tex = new CWOWTexture(name);
	if (tex)
	{
		bool bLoad = LoadBLP(tex);
		if (bLoad)
		{
			m_mapTexture[nIDCount] = tex;
			m_mapName[name] = nIDCount;
			return nIDCount;
		}
	}

	return id;
}

int CWOWTextureManager::FindTextureID(std::string name)
{
	NameMap::iterator iter = m_mapName.find(name);
	if (iter != m_mapName.end())
	{
		return iter->second;
	}

	return -1;
}

void CWOWTextureManager::Del(int nTexID)
{
	TextureIDMap::iterator iterTexID = m_mapTextureID.find(nTexID);
	if (iterTexID != m_mapTextureID.end())
	{
		LPDIRECT3DTEXTURE9 pD3DTex = iterTexID->second;
		SAFE_RELEASE(pD3DTex);
	}

	TextureMap::iterator iterTex = m_mapTexture.find(nTexID);
	if (iterTex != m_mapTexture.end())
	{
		CWOWTexture *pTex = iterTex->second;
		SAFE_DELETE(pTex);
	}

	NameMap::iterator iterName = m_mapName.begin();
	for (; iterName != m_mapName.end(); iterName++)
	{
		if(nTexID == iterName->second)
		{
			break;
		}
	}

	m_mapName.erase(iterName);
	m_mapTexture.erase(iterTex);
	m_mapTextureID.erase(iterTexID);
}

bool CWOWTextureManager::LoadBLP(CWOWTexture *tex)
{
	int offsets[16], sizes[16], w=0, h=0, type=0;
	int format = 0;
	char attr[4];

	MPQFile f(tex->m_strName.c_str());
	if (f.isEof())
	{
		qCritical("Error: Could not load the texture '%s'", tex->m_strName.c_str());
		return false;
	} 
	else 
	{
		qDebug("Loading texture: %s", tex->m_strName.c_str());
	}

	f.seek(4);
	f.read(&type,4);
	f.read(attr,4);
	f.read(&w,4);
	f.read(&h,4);
	f.read(offsets,4*16);
	f.read(sizes,4*16);

	tex->m_nWidth = w;
	tex->m_nHeight = h;

	bool hasmipmaps = true;
	int mipmax = hasmipmaps ? 16 : 1;

	if (attr[0] == 2)
	{
		format = COMPRESSED_DXT1;
		int blocksize = 8;

		if (attr[1] == 8)
		{
			format = COMPRESSED_DXT3;
			blocksize = 16;
		}

		if (attr[1] == 8 && attr[2] == 7)
		{
			format = COMPRESSED_DXT5;
			blocksize = 16;
		}

		std::vector<unsigned char*> vecBuf;
		std::vector<int> vecSize;
		int nRealMipNum = 0;

		tex->m_bCompressed = true;
		unsigned char *buf = new unsigned char[sizes[0]];
		for (int i=0; i < mipmax; i++)
		{
			if (w==0) 
				w = 1;

			if (h==0) 
				h = 1;

			if (offsets[i] && sizes[i]) 
			{
				f.seek(offsets[i]);
				f.read(buf,sizes[i]);

				int size = ((w+3)/4) * ((h+3)/4) * blocksize;

				unsigned char *tempBuf = new unsigned char[size];
				memcpy((void*)tempBuf, (void*)buf, size);
				vecBuf.push_back(tempBuf);
				vecSize.push_back(size);
				nRealMipNum++;
			} 
			else 
				break;

			w >>= 1;
			h >>= 1;
		}

		CreateD3D9Texture(tex->m_nWidth, tex->m_nHeight, nRealMipNum, format, vecSize, vecBuf);

		for (int nBuf = 0; nBuf < vecBuf.size(); nBuf++)
		{
			SAFE_DELETE_ARRAY(vecBuf[nBuf]);
		}
		vecBuf.clear();
		vecSize.clear();
		SAFE_DELETE_ARRAY(buf);
	}
	else if (attr[0] == 1)
	{
		unsigned int pal[256];
		f.read(pal,1024);

		unsigned char *buf = new unsigned char[sizes[0]];
		unsigned int *buf2 = new unsigned int[w * h];
		unsigned int *p = NULL;
		unsigned char *c = NULL, *a = NULL;

		int alphabits = attr[1];
		bool hasalpha = alphabits!=0;

		tex->m_bCompressed = false;

		std::vector<unsigned int*> vecBuf;
		std::vector<int> vecSize;
		int nRealMipNum = 0;
		for (int i=0; i<mipmax; i++) 
		{
			if (w==0) 
				w = 1;

			if (h==0) 
				h = 1;

			if (offsets[i] && sizes[i]) 
			{
				f.seek(offsets[i]);
				f.read(buf,sizes[i]);

				int cnt = 0;
				int alpha = 0;

				p = buf2;
				c = buf;
				a = buf + w*h;
				for (int y=0; y<h; y++) 
				{
					for (int x=0; x<w; x++)
					{
						unsigned int k = pal[*c++];

						k = ((k&0x00FF0000)>>16) | ((k&0x0000FF00)) | ((k& 0x000000FF)<<16);

						if (hasalpha)
						{
							if (alphabits == 8) 
							{
								alpha = (*a++);
							} 
							else if (alphabits == 1) 
							{
								alpha = (*a & (1 << cnt++)) ? 0xff : 0;
								if (cnt == 8) 
								{
									cnt = 0;
									a++;
								}
							}
						}
						else 
						{
							alpha = 0xff;
						}

						k |= alpha << 24;
						*p++ = k;
					}
				}

				// gen texture...
				//unsigned int *tempBuf = new unsigned int[sizes[i]];
				//memcpy((void*)tempBuf, (void*)buf2, sizes[i] * 4);
				unsigned int *tempBuf = new unsigned int[w * h * 4];
				memcpy((void*)tempBuf, (void*)buf2, w * h * 4);
				vecBuf.push_back(tempBuf);
				//vecSize.push_back(sizes[i] * 4);
				vecSize.push_back( w * h * 4);
				nRealMipNum++;
			}
			else
				break;

			w >>= 1;
			h >>= 1;
		}

		CreateD3D9Texture2(tex->m_nWidth, tex->m_nHeight, nRealMipNum, UNCOMPRESSED_RGBA8, vecSize, vecBuf);

		for (int nBuf = 0; nBuf < vecBuf.size(); nBuf++)
		{
			SAFE_DELETE_ARRAY(vecBuf[nBuf]);
		}
		vecBuf.clear();
		vecSize.clear();

		SAFE_DELETE_ARRAY(buf2);
		SAFE_DELETE_ARRAY(buf);
	}

	f.close();

	return true;
}

void CWOWTextureManager::CreateD3D9Texture2(int width, 
										 int height,
										 int numMip,
										 int format,
										 std::vector<int> vecSize, 
										 std::vector<unsigned int*> vecBuf)
{
	D3DFORMAT d3dFormat = D3DFMT_A8R8G8B8;
	if (format == UNCOMPRESSED_RGBA8)
	{
		d3dFormat = D3DFMT_A8R8G8B8;
	}

	LPDIRECT3DTEXTURE9 pD3D9Texture = NULL;
	HRESULT hr = g_pDevice->CreateTexture(width, height, numMip, 0, 
		d3dFormat, D3DPOOL_MANAGED, &pD3D9Texture, NULL);
	if (FAILED(hr))
	{
		qCritical("CWOWTextureManager CreateD3D9Texture CreateTexture");
		return;
	}

	for (int i = 0; i < numMip; i++)
	{
		D3DLOCKED_RECT lockedrect;
		hr = pD3D9Texture->LockRect(i, &lockedrect, NULL, 0);
		if (FAILED(hr))
		{
			qCritical("CWOWTextureManager CreateD3D9Texture LockRect");
			return;
		}

		memcpy((void*)lockedrect.pBits, (void*)vecBuf[i], vecSize[i]);
		hr = pD3D9Texture->UnlockRect(i);
		if (FAILED(hr))
		{
			qCritical("CWOWTextureManager CreateD3D9Texture UnlockRect");
			return;
		}
	}

	nIDCount++;
	m_mapTextureID[nIDCount] = pD3D9Texture;
}

void CWOWTextureManager::CreateD3D9Texture(int width, int height,
										int numMip,
										int format, 
										std::vector<int> vecSize, 
										std::vector<unsigned char*> vecBuf)
{
	D3DFORMAT d3dFormat = D3DFMT_DXT1;
	if (format == COMPRESSED_DXT1)
	{
		d3dFormat = D3DFMT_DXT1;
	}
	else if (format == COMPRESSED_DXT3)
	{
		d3dFormat = D3DFMT_DXT3;
	}
	else if (format == COMPRESSED_DXT5)
	{
		d3dFormat = D3DFMT_DXT5;
	}

	LPDIRECT3DTEXTURE9 pD3D9Texture = NULL;
	HRESULT hr = g_pDevice->CreateTexture(width, height, numMip, 0, 
		d3dFormat, D3DPOOL_MANAGED, &pD3D9Texture, NULL);
	if (FAILED(hr))
	{
		qCritical("CWOWTextureManager CreateD3D9Texture CreateTexture");
		return;
	}

	for (int i = 0; i < numMip; i++)
	{
		D3DLOCKED_RECT lockedrect;
		hr = pD3D9Texture->LockRect(i, &lockedrect, NULL, 0);
		if (FAILED(hr))
		{
			qCritical("CWOWTextureManager CreateD3D9Texture LockRect");
			return;
		}

		memcpy((void*)lockedrect.pBits, (void*)vecBuf[i], vecSize[i]);
		hr = pD3D9Texture->UnlockRect(i);
		if (FAILED(hr))
		{
			qCritical("CWOWTextureManager CreateD3D9Texture UnlockRect");
			return;
		}
	}

	nIDCount++;
	m_mapTextureID[nIDCount] = pD3D9Texture;
}

void CWOWTextureManager::GetPixel(unsigned char *buf, int nTexID)
{
	LPDIRECT3DTEXTURE9 pD3D9Texture = FindD3D9Texture(nTexID);
	CWOWTexture *pTexture = FindTexture(nTexID);
	if (pD3D9Texture && pTexture)
	{
		D3DLOCKED_RECT lockedrect;
		HRESULT hr = pD3D9Texture->LockRect(0, &lockedrect, NULL, 0);
		if (FAILED(hr))
		{
			qCritical("CWOWTextureManager GetPixel LockRect");
			return;
		}

		int size = pTexture->m_nWidth * pTexture->m_nHeight * 4;
		memcpy((void*)buf, (void*)lockedrect.pBits, size);
		hr = pD3D9Texture->UnlockRect(0);
		if (FAILED(hr))
		{
			qCritical("CWOWTextureManager GetPixel UnlockRect");
			return;
		}
	}
}

void CWOWTextureManager::ComposePixel(unsigned char *buf, int width, int height, int& nTexID)
{
	HRESULT hr;
	LPDIRECT3DTEXTURE9 pD3D9Texture = pD3D9Texture = FindD3D9Texture(nTexID);
	if (pD3D9Texture == NULL)
	{
		hr = g_pDevice->CreateTexture(width, height, 1, 0, 
			D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pD3D9Texture, NULL);

		nIDCount++;
		m_mapTextureID[nIDCount] = pD3D9Texture;
		nTexID = nIDCount;
	}

	if (FAILED(hr))
	{
		qCritical("CWOWTextureManager CreateD3D9Texture CreateTexture");
		return;
	}

	if (pD3D9Texture)
	{
		D3DLOCKED_RECT lockedrect;
		HRESULT hr = pD3D9Texture->LockRect(0, &lockedrect, NULL, 0);
		if (FAILED(hr))
		{
			qCritical("CWOWTextureManager GetPixel LockRect");
			return;
		}

		int size = width * height * 4;
		memcpy((void*)lockedrect.pBits, (void*)buf, size);
		hr = pD3D9Texture->UnlockRect(0);
		if (FAILED(hr))
		{
			qCritical("CWOWTextureManager GetPixel UnlockRect");
			return;
		}
	}
}

//--------------------------------
//----------CShaderMgr------------
//--------------------------------
CShaderMgr::CShaderMgr()
{
	m_pTerrainShader = NULL;
}

CShaderMgr::~CShaderMgr()
{

}

void CShaderMgr::Init()
{
	LPD3DXBUFFER pError = NULL;
	D3DXCreateEffectFromFileA(g_pDevice, "Resources\\Terrain.fx", NULL, NULL,
		D3DXSHADER_DEBUG, 0, &m_pTerrainShader, &pError);
	if (pError)
	{
		char *name = (char*)pError->GetBufferPointer();
		MessageBoxA(NULL, name, NULL, MB_OK);
	}

	int i = 0;
}
