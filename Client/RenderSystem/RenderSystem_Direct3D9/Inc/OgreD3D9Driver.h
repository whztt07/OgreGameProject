

#ifndef __OgreD3D9Driver_h__
#define __OgreD3D9Driver_h__

#include "OgreD3D9Prerequisites.h"
#include "OgreString.h"

namespace Ogre
{
	class CD3D9VideoModeList;

	class CD3D9Driver
	{
	public:
		CD3D9Driver();
		CD3D9Driver( LPDIRECT3D9 pD3D, unsigned int adapterNumber, 
			const D3DADAPTER_IDENTIFIER9& adapterIdentifer, const D3DDISPLAYMODE& desktopDisplayMode );
		~CD3D9Driver();

		char* DriverDescription();
		LPDIRECT3D9 GetD3D();
		int GetAdapterNumber();
		D3DADAPTER_IDENTIFIER9 GetAdapterIdentifier();
		CD3D9VideoModeList* GetD3D9VideoModeList();
	private:
		LPDIRECT3D9 m_pD3D;
		LPDIRECT3DDEVICE9 m_pD3DDevice;
		unsigned int m_nAdapterNumber;
		D3DADAPTER_IDENTIFIER9 m_AdapterIdentifier;
		D3DDISPLAYMODE m_DesktopDisplayMode;
		CD3D9VideoModeList *m_pVideoModeList;
	};

	class CD3D9DriverList
	{
	public:
		CD3D9DriverList();
		CD3D9DriverList(LPDIRECT3D9 pD3D);
		~CD3D9DriverList();

		BOOL Enumerate();
		CD3D9Driver* Item(int nDriver);
		int Count();
	private:
		typedef std::vector<CD3D9Driver> DriverList;
		DriverList m_vecDriver;
		LPDIRECT3D9 m_pD3D;
	};
}

#endif