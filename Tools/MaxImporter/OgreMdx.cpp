

#include "OgreMdx.h"

namespace Ogre
{
	//-----------------------------------
	//-----------CMdxPointer-------------
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
		m_pbyBuffer = NULL;
		m_nBufferSize = 0;
	}

	CMemoryBlock::~CMemoryBlock()
	{
		if( m_pbyBuffer )
		{
			delete[] m_pbyBuffer;
			m_pbyBuffer = NULL;
		}
		m_nBufferSize = 0;
	}

	BOOL CMemoryBlock::Create( BYTE* pbyBuffer, int nBufferSize )
	{
		if( !pbyBuffer || nBufferSize == 0 )
		{
			assert( false );
			return FALSE;
		}

		m_nBufferSize = nBufferSize;
		m_pbyBuffer = new BYTE[m_nBufferSize];
		memcpy( m_pbyBuffer, pbyBuffer, m_nBufferSize );
		return TRUE;
	}

	CMdxPointer CMemoryBlock::GetPointer()
	{
		CMdxPointer p( m_pbyBuffer );
		return p;
	}

	int CMemoryBlock::GetBufferSize()
	{ 
		return m_nBufferSize;
	}

	//-----------------------------------
	//-----------CMdxTexture-------------
	//-----------------------------------
	CMdxTexture::CMdxTexture()
	{
		m_szName[0] = 0;
	}

	CMdxTexture::~CMdxTexture()
	{

	}

	void CMdxTexture::SetName( const char* pszName )
	{
		strcpy(m_szName, pszName);
	}

	char* CMdxTexture::GetName()
	{ 
		return m_szName;
	}

	//-----------------------------------
	//-----------CMdxTextures------------
	//-----------------------------------
	CMdxTextures::CMdxTextures()
	{
		m_nNum = 0;
	}

	CMdxTextures::~CMdxTextures()
	{

	}

	BOOL CMdxTextures::Read( CMdxPointer inP, int nSize )
	{
		CMdxPointer p(inP.p);
		
		int nTextureCount = *p.i++;
		for( int i = 0; i < nTextureCount; i++ )
		{
			CMdxTexture texture;
			texture.SetName( (const char*)p.c );
			m_vecTexture.push_back( texture );
			m_nNum = m_vecTexture.size();
			p.c += MAX_PATH;
		}

		return TRUE;
	}

	int CMdxTextures::GetTextureCount()
	{
		return m_nNum;
	}

	CMdxTexture* CMdxTextures::GetTexture( int i )
	{
		if (i < 0 || i > m_nNum - 1)
		{
			assert(false);
			return NULL;
		}

		return &m_vecTexture[i]; 
	}

	CMdxTexture* CMdxTextures::AddTexture()
	{
		CMdxTexture texture;
		texture.SetName( "" );
		m_vecTexture.push_back( texture );
		m_nNum = m_vecTexture.size();
		
		return &m_vecTexture[m_nNum - 1];
	}

	//-----------------------------------
	//------------CMdxBone---------------
	//-----------------------------------
	CMdxBone::CMdxBone()
	{
		m_pSkeleton = NULL;
		m_nBoneId = -1;

		m_szName[0] = 0;
		m_nParentId = -1;
		m_nChildCount = 0;
		m_pnChildIds = NULL;
		m_nFrameCount = 0;
		m_pTrans = NULL;
		m_pQuats = NULL;
		m_pMatrices = NULL;
		m_pVisible = NULL;
	}

	CMdxBone::~CMdxBone()
	{

	}

	CMatrix CMdxBone::GetFrame0Inv()
	{
		return m_matFrame0Inv;
	}

	CMatrix CMdxBone::GetMatrices( int nFrameId )
	{
		if (m_pTrans)
		{
			CMatrix m;
			MakeMatrix(m_pTrans[nFrameId], m_pQuats[nFrameId], m);
			return m;
		}

		return m_pMatrices[nFrameId];
	}

	BOOL CMdxBone::Read(CMdxPointer inP, int nSize)
	{
		CMdxPointer p(inP.p);
		memcpy(m_szName, p.c, MDX_MAX_NAME);
		p.c += MDX_MAX_NAME;
		m_nParentId = *p.i++;
		m_nChildCount = *p.i++;
		if (m_nChildCount > 0)
		{
			m_pnChildIds = new int[m_nChildCount];
			memcpy(m_pnChildIds, p.c, sizeof(int) * m_nChildCount);
		}
		p.i += m_nChildCount;

		while(p.c < inP.c + nSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case 'trck':
				{
					p.dw++;
					int size = *p.i++;
					{
						m_nFrameCount = *p.i++;
						m_pMatrices = p.matrix;
						p.matrix += m_nFrameCount;
						
						m_matFrame0Inv = m_pMatrices[0];
						m_matFrame0Inv.Inverse();
					}
					p.c += size;
				}
				break;
			case 'trk2':
				{
					p.dw++;
					int size = *p.i++;
					{
						m_nFrameCount = *p.i++;
						m_pTrans = p.v3;
						p.v3 += m_nFrameCount;
						m_pQuats = p.quat;
						p.quat += m_nFrameCount;
					}
					p.c += size;
				}
				break;
			case 'vis2':
				{
					p.dw++;
					int size = *p.i++;
					{
						m_pVisible = p.i;
						p.i += m_nFrameCount;
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

		return TRUE;
	}

	//-----------------------------------
	//------------CMdxSkeleton-----------
	//-----------------------------------
	CMdxSkeleton::CMdxSkeleton()
	{
		m_nRootBoneCount = 0;
		m_pnRootBoneIds = NULL;
	}

	CMdxSkeleton::~CMdxSkeleton()
	{

	}

	BOOL CMdxSkeleton::Read( CMdxPointer inP, int nSize )
	{
		CMdxPointer p(inP.p);

		int nBoneCount = *p.i++;
		m_nRootBoneCount = *p.i++;

		if( m_nRootBoneCount > 0 )
		{
			m_pnRootBoneIds = new int[m_nRootBoneCount];
			memcpy( m_pnRootBoneIds, p.i, sizeof(int) * m_nRootBoneCount );
		}
		p.i += m_nRootBoneCount;

		while(p.c < inP.c + nSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case 'bone':
				{
					p.dw++;
					int size = *p.i++;
					{
						CMdxBone *pBone = new CMdxBone;
						pBone->m_pSkeleton = this;
						pBone->m_nBoneId = m_vecBone.size();
						pBone->Read(p, size);
						m_vecBone.push_back(pBone);
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

		return FALSE;
	}

	int CMdxSkeleton::GetBoneNum()
	{
		return m_vecBone.size();
	}

	CMdxBone* CMdxSkeleton::GetBone(int nBone)
	{
		if (nBone < 0 || nBone >= m_vecBone.size())
		{
			return NULL;
		}

		return m_vecBone[nBone];
	}

	//-----------------------------------
	//------------CMdxBoneGroup----------
	//-----------------------------------
	CMdxBoneGroup::CMdxBoneGroup()
	{
		m_nBoneCount = 0;
		m_pnBoneIds = NULL;
	}

	CMdxBoneGroup::~CMdxBoneGroup()
	{

	}

	BOOL CMdxBoneGroup::Read(CMdxPointer inP, int nSize)
	{
		CMdxPointer p(inP.p);
		m_nBoneCount = *p.i++;
		m_pnBoneIds = p.i;
		p.i += m_nBoneCount;

		return TRUE;
	}

	//-----------------------------------
	//------------CMdxBoneGroups---------
	//-----------------------------------
	CMdxBoneGroups::CMdxBoneGroups()
	{

	}

	CMdxBoneGroups::~CMdxBoneGroups()
	{

	}

	BOOL CMdxBoneGroups::Read( CMdxPointer inP, int nSize )
	{
		CMdxPointer p(inP.p);

		while (p.c < inP.c + nSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case 'bgrp':
				{
					p.dw++;
					int size = *p.i++;
					{
						CMdxBoneGroup bg;
						bg.Read(p, size);
						m_vecBoneGroup.push_back(bg);
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

		return TRUE;
	}

	//-----------------------------------
	//------------CMdxGeoChunk-----------
	//-----------------------------------
	CMdxGeoChunk::CMdxGeoChunk()
	{
		m_nChunkId = -1;
		m_pSrcMdx = NULL;

		m_pszName = NULL;
		m_nVertexCount = 0;
		m_nFaceCount = 0;
		m_nMtlId = -1;
		m_pVertices = NULL;
		m_pNormals = NULL;
		m_pColors = NULL;
		m_pOriginColors = NULL;
		m_pBoneGroupIndices = NULL;
		m_pFaces = NULL;
		m_nUVLayerCount = 1;

		for (int i = 0; i < eMaxUVLayer; i++)
		{
			m_pUVs[i] = NULL;
		}
	}

	CMdxGeoChunk::~CMdxGeoChunk()
	{

	}

	char* CMdxGeoChunk::GetName()
	{
		return m_pszName;
	}

	int CMdxGeoChunk::GetVertexNum()
	{
		return m_nVertexCount;
	}

	int CMdxGeoChunk::GetFaceNum()
	{
		return m_nFaceCount;
	}

	int CMdxGeoChunk::GetMtlID()
	{
		return m_nMtlId;
	}

	CMdxFace* CMdxGeoChunk::GetFace(int index)
	{
		if (index < 0 || index > m_nFaceCount - 1)
		{
			return NULL;
		}

		return &m_pFaces[index];
	}

	CMdxUV* CMdxGeoChunk::GetUV(int layer, int index)
	{
		if (layer < 0 || layer > eMaxUVLayer - 1)
		{
			return NULL;
		}

		if (index < 0 || index > m_nVertexCount - 1)
		{
			return NULL;
		}

		return &m_pUVs[layer][index];
	}

	CVector* CMdxGeoChunk::GetVertex(int index)
	{
		if (index < 0 || index > m_nVertexCount - 1)
		{
			return NULL;
		}

		return &m_pVertices[index];
	}

	BOOL CMdxGeoChunk::Read( CMdxPointer inP, int nSize )
	{
		CMdxPointer p(inP.p);
		m_nVertexCount = *p.i++;
		m_nFaceCount = *p.i++;
		m_nMtlId = *p.i++;

		m_pVertices = p.v3;
		p.v3 += m_nVertexCount;
		m_pNormals = p.v3;
		p.v3 += m_nVertexCount;
		m_pUVs[0] = p.uv;
		p.uv += m_nVertexCount;
		m_pBoneGroupIndices = p.byte;
		p.byte += m_nVertexCount;
		m_pFaces = p.face;
		p.face += m_nFaceCount;

		while (p.c < inP.c + nSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case 'name':
				{
					p.dw++;
					int size = *p.i++;
					{
						m_pszName = p.c;
					}
					p.c += size;
				}
				break;
			case 'vcol':
				{
					p.dw++;
					int size = *p.i++;
					{
						m_pColors = p.dw;
						m_pOriginColors = p.dw;
					}
					p.c += size;
				}
				break;
			case 'mtuv':
				{
					p.dw++;
					int size = *p.i++;
					CMdxPointer s(p.c);
					{
						m_nUVLayerCount = *s.i++;
						for (int nLayer = 1; nLayer < m_nUVLayerCount; nLayer++)
						{
							m_pUVs[nLayer] = s.uv;
							s.uv += m_nVertexCount;
						}
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

		return TRUE;
	}

	//-----------------------------------
	//------------CMdxGeometry-----------
	//-----------------------------------
	CMdxGeometry::CMdxGeometry()
	{
		m_pSrcMdx = NULL;
	}

	CMdxGeometry::~CMdxGeometry()
	{

	}

	int CMdxGeometry::GetChunkCount()
	{
		return m_vecChunk.size();
	}

	CMdxGeoChunk* CMdxGeometry::GetChunk(int i)
	{
		if (i < 0 || i >= m_vecChunk.size())
		{
			return NULL;
		}

		return m_vecChunk[i];
	}

	BOOL CMdxGeometry::Read( CMdxPointer inP, int nSize )
	{
		CMdxPointer p(inP.p);

		while (p.c < inP.c + nSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case 'chks':
				{
					p.dw++;
					int size = *p.i++;
					{
						CMdxGeoChunk *pChunk = new CMdxGeoChunk;
						pChunk->m_nChunkId = m_vecChunk.size();
						pChunk->m_pSrcMdx = m_pSrcMdx;
						pChunk->Read(p, size);
						m_vecChunk.push_back(pChunk);
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

		return TRUE;
	}

	//-----------------------------------
	//---------------CMdx----------------
	//-----------------------------------
	CMdx::CMdx()
	{
		m_szFileName[0] = 0;
		m_textures = NULL;
		m_pSkeleton = NULL;
		m_pBoneGroups = NULL;
		m_pGeometry = NULL;

		m_nFrameId = -1;
	}

	CMdx::~CMdx()
	{

	}

	void CMdx::Destroy()
	{
		SAFE_DELETE(m_textures);
	}

	char* CMdx::GetFileName()
	{
		return m_szFileName;
	}

	BOOL CMdx::LoadFromFile( const char* pszFilename, DWORD dwFlag )
	{
		strcpy(m_szFileName, pszFilename);

		CFile fp;
		if (fp.fopen(pszFilename, "rb"))
		{
			BYTE* byMemory = fp.GetBuffer();
			int nSize = fp.GetBufferSize();
			LoadFromMemory(byMemory, nSize);

			fp.fclose();
			return TRUE;
		}

		return FALSE;
	}

	CMdxTextures* CMdx::GetTextures()
	{
		return m_textures;
	}

	CMdxSkeleton* CMdx::GetSkeleton()
	{
		return m_pSkeleton;
	}

	CMdxBoneGroups* CMdx::GetBoneGroups()
	{
		return m_pBoneGroups;
	}

	CMdxGeometry* CMdx::GetGeometry()
	{
		return m_pGeometry;
	}

	void CMdx::SetFrameId( int nFrameId )
	{
		m_nFrameId = nFrameId;
	}

	BOOL CMdx::LoadFromMemory( BYTE* pbyBuffer, int nBufferSize )
	{
		CMdxPointer p(pbyBuffer);

		while( p.c < (char*)pbyBuffer + nBufferSize )
		{
			switch( MDX_TAG( *p.dw ) )
			{
			case 'texs':
				{
					p.dw++;
					int size = *p.i++;
					{
						m_textures = new CMdxTextures;
						m_TexturesMB.Create( p.byte, size );
						m_textures->Read( m_TexturesMB.GetPointer(), m_TexturesMB.GetBufferSize() );
					}
					p.c += size;
				}
				break;
			case 'geom':
				{
					p.dw++;
					int size = *p.i++;
					{
						m_pGeometry = new CMdxGeometry;
						m_pGeometry->m_pSrcMdx = this;
						m_GeometryMB.Create(p.byte, size);
						m_pGeometry->Read(m_GeometryMB.GetPointer(), m_GeometryMB.GetBufferSize());
					}
					p.c += size;
				}
				break;
			case 'sklt':
				{
					p.dw++;
					int size = *p.i++;
					{
						m_pSkeleton = new CMdxSkeleton;
						m_SkeletonMB.Create( p.byte, size );
						m_pSkeleton->Read( m_SkeletonMB.GetPointer(), m_SkeletonMB.GetBufferSize() );
					}
					p.c += size;
				}
				break;
			case 'bgps':
				{
					p.dw++;
					int size = *p.i++;
					{
						m_pBoneGroups = new CMdxBoneGroups;
						m_BoneGroupsMB.Create(p.byte, size);
						m_pBoneGroups->Read(m_BoneGroupsMB.GetPointer(), m_BoneGroupsMB.GetBufferSize());
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

		return TRUE;
	}
}