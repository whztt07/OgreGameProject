

#include "OgreD3D9Texture.h"
#include "OgreUseful.h"

namespace Ogre
{
	// -----------------------
	// GlobalParam
	// -----------------------

	// -----------------------
	// CD3D9Texture
	// -----------------------
	CD3D9Texture::CD3D9Texture(CString szGroup, CString szName, IDirect3DDevice9 *pDevice)
		:CTexture(szGroup, szName)
	{
		m_pDevice = pDevice;
		m_pTexture = NULL;
	}

	CD3D9Texture::~CD3D9Texture()
	{

	}

	bool CD3D9Texture::LoadImpl()
	{
		int nSize = m_pDataStream->GetSize();
		byte *pBuf = m_pDataStream->GetBuffer();

		D3DXCreateTextureFromFileInMemory(m_pDevice, pBuf, nSize, &m_pTexture);

		SAFE_DELETE_ARRAY(pBuf);
		return true;
	}

	void CD3D9Texture::PostLoadImpl()
	{
	}

	int CD3D9Texture::CalcSize()
	{
		return 0;
	}

	// -----------------------
	// CD3D9TextureManager
	// -----------------------
	CD3D9TextureManager::CD3D9TextureManager(IDirect3DDevice9 *pDevice)
	{
		m_pDevice = pDevice;
	}

	CD3D9TextureManager::~CD3D9TextureManager()
	{

	}

	CResource* CD3D9TextureManager::Load(CString szGroup, CString szName)
	{
		CResource *pResource = Create(szName, szGroup);
		pResource->Load();
		return pResource;
	}

	CResource* CD3D9TextureManager::CreateImpl(CString szGroup, CString szName)
	{
		TextureMap::iterator iter = m_mapTexture.find(szName);
		if (iter != m_mapTexture.end())
		{
			return iter->second;
		}

		CD3D9Texture *pTexture = new CD3D9Texture(szGroup, szName, m_pDevice);
		m_vecTexture.push_back(pTexture);
		m_mapTexture[szName] = pTexture;
		return pTexture;
	}

	void CD3D9TextureManager::OnLostDevice()
	{
		int nNum = m_vecTexture.size();
		for (int i = 0; i < nNum; i++)
		{
			CD3D9Texture *pTexture = m_vecTexture[i];
			if (pTexture == NULL)
			{
				continue;
			}

			IDirect3DTexture9 *pD3D9Texture = (IDirect3DTexture9*)pTexture->GetRealTex();
			if (pD3D9Texture == NULL)
			{
				continue;
			}

			//SAFE_RELEASE(pD3D9Texture);
		}
	}

	void CD3D9TextureManager::OnResetDevice()
	{
		int nNum = m_vecTexture.size();
		for (int i = 0; i < nNum; i++)
		{
			CD3D9Texture *pTexture = m_vecTexture[i];
			if (pTexture == NULL)
			{
				continue;
			}

			//pTexture->LoadImpl();
		}
	}
}