

#include "OgreMSMdx.h"

namespace Ogre
{
	//-----------------------------------
	//-----------CMSMdxPointer-------------
	//-----------------------------------
	CMSMdxPointer::CMSMdxPointer(void* in) 
	{
		p = in;
	}

	//-----------------------------------
	//-----------CMemoryBlock------------
	//-----------------------------------
	CMSMemoryBlock::CMSMemoryBlock()
	{
		m_pbyBuffer = NULL;
		m_nBufferSize = 0;
	}

	CMSMemoryBlock::~CMSMemoryBlock()
	{
		if( m_pbyBuffer )
		{
			delete[] m_pbyBuffer;
			m_pbyBuffer = NULL;
		}
		m_nBufferSize = 0;
	}

	BOOL CMSMemoryBlock::Create( BYTE* pbyBuffer, int nBufferSize )
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

	CMSMdxPointer CMSMemoryBlock::GetPointer()
	{
		CMSMdxPointer p( m_pbyBuffer );
		return p;
	}

	int CMSMemoryBlock::GetBufferSize()
	{ 
		return m_nBufferSize;
	}

	//-----------------------------------
	//-----------CMdxTexture-------------
	//-----------------------------------
	CMSMdxTexture::CMSMdxTexture()
	{
		m_szName[0] = 0;
	}

	CMSMdxTexture::~CMSMdxTexture()
	{

	}

	void CMSMdxTexture::SetName( const char* pszName )
	{
		strcpy(m_szName, pszName);
	}

	char* CMSMdxTexture::GetName()
	{ 
		return m_szName;
	}

	//-----------------------------------
	//-----------CMdxTextures------------
	//-----------------------------------
	CMSMdxTextures::CMSMdxTextures()
	{
		m_nNum = 0;
	}

	CMSMdxTextures::~CMSMdxTextures()
	{

	}

	BOOL CMSMdxTextures::Read( CMSMdxPointer inP, int nSize )
	{
		CMSMdxPointer p(inP.p);
		
		int nTextureCount = *p.i++;
		for( int i = 0; i < nTextureCount; i++ )
		{
			CMSMdxTexture texture;
			texture.SetName( (const char*)p.c );
			m_vecTexture.push_back( texture );
			m_nNum = m_vecTexture.size();
			p.c += MAX_PATH;
		}

		return TRUE;
	}

	int CMSMdxTextures::GetTextureCount()
	{
		return m_nNum;
	}

	CMSMdxTexture* CMSMdxTextures::GetTexture( int i )
	{
		if (i < 0 || i > m_nNum - 1)
		{
			assert(false);
			return NULL;
		}

		return &m_vecTexture[i]; 
	}

	CMSMdxTexture* CMSMdxTextures::AddTexture()
	{
		CMSMdxTexture texture;
		texture.SetName( "" );
		m_vecTexture.push_back( texture );
		m_nNum = m_vecTexture.size();
		
		return &m_vecTexture[m_nNum - 1];
	}

	//-----------------------------------
	//------------CMdxBone---------------
	//-----------------------------------
	CMSMdxBone::CMSMdxBone()
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

	CMSMdxBone::~CMSMdxBone()
	{

	}

	CMatrix CMSMdxBone::GetFrame0Inv()
	{
		return m_matFrame0Inv;
	}

	CMatrix CMSMdxBone::GetMatrices( int nFrameId )
	{
		if (m_pTrans)
		{
			CMatrix m;
			MakeMatrix(m_pTrans[nFrameId], m_pQuats[nFrameId], m);
			return m;
		}

		return m_pMatrices[nFrameId];
	}

	BOOL CMSMdxBone::Read(CMSMdxPointer inP, int nSize)
	{
		CMSMdxPointer p(inP.p);
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
	CMSMdxSkeleton::CMSMdxSkeleton()
	{
		m_nRootBoneCount = 0;
		m_pnRootBoneIds = NULL;
	}

	CMSMdxSkeleton::~CMSMdxSkeleton()
	{

	}

	BOOL CMSMdxSkeleton::Read( CMSMdxPointer inP, int nSize )
	{
		CMSMdxPointer p(inP.p);

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
						CMSMdxBone *pBone = new CMSMdxBone;
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

	int CMSMdxSkeleton::GetBoneNum()
	{
		return m_vecBone.size();
	}

	CMSMdxBone* CMSMdxSkeleton::GetBone(int nBone)
	{
		if (nBone < 0 || nBone >= m_vecBone.size())
		{
			return NULL;
		}

		return m_vecBone[nBone];
	}

	//-----------------------------------
	//------------CMSMdxBoneGroup----------
	//-----------------------------------
	CMSMdxBoneGroup::CMSMdxBoneGroup()
	{
		m_nBoneCount = 0;
		m_pnBoneIds = NULL;
	}

	CMSMdxBoneGroup::~CMSMdxBoneGroup()
	{

	}

	BOOL CMSMdxBoneGroup::Read(CMSMdxPointer inP, int nSize)
	{
		CMSMdxPointer p(inP.p);
		m_nBoneCount = *p.i++;
		m_pnBoneIds = p.i;
		p.i += m_nBoneCount;

		return TRUE;
	}

	//-----------------------------------
	//------------CMdxBoneGroups---------
	//-----------------------------------
	CMSMdxBoneGroups::CMSMdxBoneGroups()
	{

	}

	CMSMdxBoneGroups::~CMSMdxBoneGroups()
	{

	}

	BOOL CMSMdxBoneGroups::Read( CMSMdxPointer inP, int nSize )
	{
		CMSMdxPointer p(inP.p);

		while (p.c < inP.c + nSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case 'bgrp':
				{
					p.dw++;
					int size = *p.i++;
					{
						CMSMdxBoneGroup bg;
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
	//------------CMSMdxGeoChunk-----------
	//-----------------------------------
	CMSMdxGeoChunk::CMSMdxGeoChunk()
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

	CMSMdxGeoChunk::~CMSMdxGeoChunk()
	{

	}

	BOOL CMSMdxGeoChunk::Read( CMSMdxPointer inP, int nSize )
	{
		CMSMdxPointer p(inP.p);
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
					CMSMdxPointer s(p.c);
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
	CMSMdxGeometry::CMSMdxGeometry()
	{
		m_pSrcMdx = NULL;
	}

	CMSMdxGeometry::~CMSMdxGeometry()
	{

	}

	int CMSMdxGeometry::GetChunkCount()
	{
		return m_vecChunk.size();
	}

	CMSMdxGeoChunk* CMSMdxGeometry::GetChunk(int i)
	{
		if (i < 0 || i >= m_vecChunk.size())
		{
			return NULL;
		}

		return m_vecChunk[i];
	}

	BOOL CMSMdxGeometry::Read( CMSMdxPointer inP, int nSize )
	{
		CMSMdxPointer p(inP.p);

		while (p.c < inP.c + nSize)
		{
			switch(MDX_TAG(*p.dw))
			{
			case 'chks':
				{
					p.dw++;
					int size = *p.i++;
					{
						CMSMdxGeoChunk *pChunk = new CMSMdxGeoChunk;
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
	CMSMdx::CMSMdx()
	{
		m_szFileName[0] = 0;
		m_textures = NULL;
		m_pSkeleton = NULL;
		m_pBoneGroups = NULL;
		m_pGeometry = NULL;

		m_nFrameId = -1;
	}

	CMSMdx::~CMSMdx()
	{

	}

	void CMSMdx::Destroy()
	{
		SAFE_DELETE(m_textures);
	}

	BOOL CMSMdx::LoadFromFile( const char* pszFilename, DWORD dwFlag )
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

	CMSMdxTextures* CMSMdx::GetTextures()
	{
		return m_textures;
	}

	CMSMdxSkeleton* CMSMdx::GetSkeleton()
	{
		return m_pSkeleton;
	}

	CMSMdxBoneGroups* CMSMdx::GetBoneGroups()
	{
		return m_pBoneGroups;
	}

	CMSMdxGeometry* CMSMdx::GetGeometry()
	{
		return m_pGeometry;
	}

	void CMSMdx::SetFrameId( int nFrameId )
	{
		m_nFrameId = nFrameId;
	}

	BOOL CMSMdx::LoadFromMemory( BYTE* pbyBuffer, int nBufferSize )
	{
		CMSMdxPointer p(pbyBuffer);

		while( p.c < (char*)pbyBuffer + nBufferSize )
		{
			switch( MDX_TAG( *p.dw ) )
			{
			case 'texs':
				{
					p.dw++;
					int size = *p.i++;
					{
						m_textures = new CMSMdxTextures;
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
						m_pGeometry = new CMSMdxGeometry;
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
						m_pSkeleton = new CMSMdxSkeleton;
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
						m_pBoneGroups = new CMSMdxBoneGroups;
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