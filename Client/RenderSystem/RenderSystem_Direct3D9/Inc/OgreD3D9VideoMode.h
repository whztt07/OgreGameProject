

#ifndef __OgreD3D9VideoMode_h__
#define __OgreD3D9VideoMode_h__

#include "OgreD3D9Prerequisites.h"
#include "Ogre3d.h"
#include "OgreD3D9Driver.h"

namespace Ogre
{
	static unsigned int g_nModeCount = 0;

	class CD3D9VideoMode
	{
	public:
		CD3D9VideoMode();
		CD3D9VideoMode(D3DDISPLAYMODE d3ddm);
		~CD3D9VideoMode();

		int GetWidth();
		int GetHeight();
		D3DFORMAT GetFormat();
		int GetRefreshRate();
		int GetColorDepth();
		CString GetDescription();
		void IncreaseRefreshRate(unsigned int rr);
		D3DDISPLAYMODE GetDisplayMode();
	private:
		D3DDISPLAYMODE m_DisplayMode;
		unsigned int m_nModeNumber;
	};

	class CD3D9VideoModeList
	{
	public:
		CD3D9VideoModeList(CD3D9Driver* pDriver);
		~CD3D9VideoModeList();

		bool Enumerate();
		int Count();
		CD3D9VideoMode* Item( const CString &name );
		CD3D9VideoMode* Item(int nSel);
	private:
		CD3D9Driver *m_pDriver;
		typedef std::vector<CD3D9VideoMode> VideoModeList;
		VideoModeList m_vecVideoMode;
	};
}

#endif