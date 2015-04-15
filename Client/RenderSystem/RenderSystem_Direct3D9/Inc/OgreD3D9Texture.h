

#ifndef __OgreD3D9Texture_h__
#define __OgreD3D9Texture_h__

#include "OgreD3D9Prerequisites.h"
#include "OgreResourceManager.h"

namespace Ogre
{
	class CD3D9TextureManager;

	class OgreExport CD3D9Texture : public CTexture
	{
		friend class CD3D9TextureManager;
	public:
		CD3D9Texture(CString szGroup, CString szName, IDirect3DDevice9 *pDevice);
		~CD3D9Texture();

		virtual void* GetRealTex(){return m_pTexture;}
	private:
		bool LoadImpl();
		void PostLoadImpl();
		int CalcSize();

		IDirect3DDevice9 *m_pDevice;
		IDirect3DTexture9 *m_pTexture;
	};

	class OgreExport CD3D9TextureManager : public CTextureManager
	{
	public:
		CD3D9TextureManager(IDirect3DDevice9 *pDevice);
		~CD3D9TextureManager();

		virtual CResource* Load(CString szGroup, CString szName);
		virtual void OnLostDevice();
		virtual void OnResetDevice();
	private:
		virtual CResource* CreateImpl(CString szGroup, CString szName);

		IDirect3DDevice9 *m_pDevice;
		typedef std::vector<CD3D9Texture*> TextureList;
		typedef std::map<CString, CD3D9Texture*> TextureMap;
		TextureList m_vecTexture;
		TextureMap m_mapTexture;
	};
}

#endif