

#include "OgreD3D9VideoMode.h"

namespace Ogre
{
	//-----------------------------------------
	//----------CD3D9VideoMode-----------------
	//-----------------------------------------
	CD3D9VideoMode::CD3D9VideoMode()
	{
		m_nModeNumber = g_nModeCount++;
		ZeroMemory(&m_DisplayMode, sizeof(D3DDISPLAYMODE));
	}

	CD3D9VideoMode::CD3D9VideoMode(D3DDISPLAYMODE d3ddm)
	{
		m_nModeNumber = g_nModeCount++; 
		m_DisplayMode = d3ddm; 
	}

	CD3D9VideoMode::~CD3D9VideoMode()
	{

	}

	int CD3D9VideoMode::GetWidth()
	{
		return m_DisplayMode.Width;
	}

	int CD3D9VideoMode::GetHeight()
	{
		return m_DisplayMode.Height;
	}

	D3DFORMAT CD3D9VideoMode::GetFormat()
	{
		return m_DisplayMode.Format;
	}

	int CD3D9VideoMode::GetRefreshRate()
	{
		return m_DisplayMode.RefreshRate;
	}

	int CD3D9VideoMode::GetColorDepth()
	{
		unsigned int colourDepth = 16;
		if( m_DisplayMode.Format == D3DFMT_X8R8G8B8 ||
			m_DisplayMode.Format == D3DFMT_A8R8G8B8 ||
			m_DisplayMode.Format == D3DFMT_R8G8B8 )
		{
			colourDepth = 32;
		}

		return colourDepth;
	}

	CString CD3D9VideoMode::GetDescription()
	{
		char tmp[MAX_PATH];
		unsigned int colourDepth = 16;
		if( m_DisplayMode.Format == D3DFMT_X8R8G8B8 ||
			m_DisplayMode.Format == D3DFMT_A8R8G8B8 ||
			m_DisplayMode.Format == D3DFMT_R8G8B8 )
		{
			colourDepth = 32;
		}

		sprintf( tmp, "%d x %d @ %d-bit colour",
			m_DisplayMode.Width, m_DisplayMode.Height, colourDepth );
		return CString(tmp);
	}

	void CD3D9VideoMode::IncreaseRefreshRate(unsigned int rr)
	{
		m_DisplayMode.RefreshRate = rr; 
	}

	D3DDISPLAYMODE CD3D9VideoMode::GetDisplayMode()
	{
		return m_DisplayMode;
	}

	//-----------------------------------------
	//----------CD3D9VideoModeList-------------
	//-----------------------------------------
	CD3D9VideoModeList::CD3D9VideoModeList(CD3D9Driver* pDriver)
	{
		m_pDriver = pDriver;

		Enumerate();
	}

	CD3D9VideoModeList::~CD3D9VideoModeList()
	{

	}

	int CD3D9VideoModeList::Count()
	{
		return m_vecVideoMode.size();
	}

	CD3D9VideoMode* CD3D9VideoModeList::Item( const CString &name )
	{
		VideoModeList::iterator iter = m_vecVideoMode.begin();
		if (iter == m_vecVideoMode.end())
			return NULL;

		for (; iter != m_vecVideoMode.end(); ++iter)
		{
			if (iter->GetDescription() == name)
			{
				return &(*iter);
			}
		}

		return NULL;
	}

	CD3D9VideoMode* CD3D9VideoModeList::Item(int nSel)
	{
		int nNum = m_vecVideoMode.size();
		if ( nSel < 0 || nSel > (nNum - 1) )
		{
			return NULL;
		}

		VideoModeList::iterator p = m_vecVideoMode.begin();

		return &p[nSel];
	}

	bool CD3D9VideoModeList::Enumerate()
	{
		UINT iMode;
		LPDIRECT3D9 pD3D = m_pDriver->GetD3D();
		UINT adapter = m_pDriver->GetAdapterNumber();

		int nAdapter = pD3D->GetAdapterModeCount(adapter, D3DFMT_R5G6B5);
		for( iMode = 0; iMode < nAdapter; iMode++ )
		{
			D3DDISPLAYMODE displayMode;
			pD3D->EnumAdapterModes( adapter, D3DFMT_R5G6B5, iMode, &displayMode );

			// Filter out low-resolutions
			if( displayMode.Width < 640 || displayMode.Height < 400 )
				continue;

			// Check to see if it is already in the list (to filter out refresh rates)
			BOOL found = FALSE;
			VideoModeList::iterator iter;
			for( iter = m_vecVideoMode.begin(); iter != m_vecVideoMode.end(); iter++ )
			{
				D3DDISPLAYMODE oldDisp = iter->GetDisplayMode();
				if( oldDisp.Width == displayMode.Width &&
					oldDisp.Height == displayMode.Height &&
					oldDisp.Format == displayMode.Format )
				{
					// Check refresh rate and favour higher if poss
					if (oldDisp.RefreshRate < displayMode.RefreshRate)
						iter->IncreaseRefreshRate(displayMode.RefreshRate);
					found = TRUE;
					break;
				}
			}

			if( !found )
			{
				m_vecVideoMode.push_back( CD3D9VideoMode( displayMode ) );
			}
		}

		nAdapter = pD3D->GetAdapterModeCount(adapter, D3DFMT_X8R8G8B8);
		for( iMode = 0; iMode < nAdapter; iMode++ )
		{
			D3DDISPLAYMODE displayMode;
			pD3D->EnumAdapterModes( adapter, D3DFMT_X8R8G8B8, iMode, &displayMode );

			// Filter out low-resolutions
			if( displayMode.Width < 640 || displayMode.Height < 400 )
				continue;

			// Check to see if it is already in the list (to filter out refresh rates)
			BOOL found = FALSE;
			VideoModeList::iterator it;
			for( it = m_vecVideoMode.begin(); it != m_vecVideoMode.end(); it++ )
			{
				D3DDISPLAYMODE oldDisp = it->GetDisplayMode();
				if( oldDisp.Width == displayMode.Width &&
					oldDisp.Height == displayMode.Height &&
					oldDisp.Format == displayMode.Format )
				{
					// Check refresh rate and favour higher if poss
					if (oldDisp.RefreshRate < displayMode.RefreshRate)
						it->IncreaseRefreshRate(displayMode.RefreshRate);
					found = TRUE;
					break;
				}
			}

			if( !found )
			{
				m_vecVideoMode.push_back( CD3D9VideoMode( displayMode ) );
			}
		}

		return TRUE;
	}
}