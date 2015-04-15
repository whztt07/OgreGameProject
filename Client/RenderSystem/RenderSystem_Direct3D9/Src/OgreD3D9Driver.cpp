

#include "OgreD3D9Driver.h"
#include "OgreD3D9VideoMode.h"

namespace Ogre
{
	//-----------------------------------------
	//----------CD3D9Driver--------------------
	//-----------------------------------------
	CD3D9Driver::CD3D9Driver()
	{
		m_pD3D = NULL;
		m_pD3DDevice = NULL;
		m_pVideoModeList = NULL;

		m_nAdapterNumber = 0;
		ZeroMemory( &m_AdapterIdentifier, sizeof(m_AdapterIdentifier) );
		ZeroMemory( &m_DesktopDisplayMode, sizeof(m_DesktopDisplayMode) );
	}

	CD3D9Driver::CD3D9Driver( LPDIRECT3D9 pD3D, unsigned int adapterNumber, 
		const D3DADAPTER_IDENTIFIER9& adapterIdentifer, const D3DDISPLAYMODE& desktopDisplayMode )
	{
		m_pD3D = pD3D;
		// initialise device member
		m_pD3DDevice = NULL;
		m_pVideoModeList = NULL;
		m_nAdapterNumber = adapterNumber;
		m_AdapterIdentifier = adapterIdentifer;
		m_DesktopDisplayMode = desktopDisplayMode;
	}

	CD3D9Driver::~CD3D9Driver()
	{
		m_pD3D = NULL;
		m_pD3DDevice = NULL;
	}

	char* CD3D9Driver::DriverDescription()
	{
		return m_AdapterIdentifier.Description;
	}

	LPDIRECT3D9 CD3D9Driver::GetD3D()
	{
		return m_pD3D;
	}

	int CD3D9Driver::GetAdapterNumber()
	{
		return m_nAdapterNumber;
	}

	D3DADAPTER_IDENTIFIER9 CD3D9Driver::GetAdapterIdentifier()
	{
		return m_AdapterIdentifier;
	}

	CD3D9VideoModeList* CD3D9Driver::GetD3D9VideoModeList()
	{
		if (m_pVideoModeList == NULL)
		{
			m_pVideoModeList = new CD3D9VideoModeList(this);
		}

		return m_pVideoModeList;
	}

	//-----------------------------------------
	//----------CD3D9DriverList----------------
	//-----------------------------------------
	CD3D9DriverList::CD3D9DriverList()
	{
		m_pD3D = NULL;
	}

	CD3D9DriverList::CD3D9DriverList(LPDIRECT3D9 pD3D)
	{
		m_pD3D = pD3D;
	}

	CD3D9DriverList::~CD3D9DriverList()
	{

	}

	BOOL CD3D9DriverList::Enumerate()
	{
		for( UINT iAdapter = 0; iAdapter < m_pD3D->GetAdapterCount(); ++iAdapter )
		{
			D3DADAPTER_IDENTIFIER9 adapterIdentifier;
			D3DDISPLAYMODE d3ddm;
			m_pD3D->GetAdapterIdentifier( iAdapter, 0, &adapterIdentifier );
			m_pD3D->GetAdapterDisplayMode( iAdapter, &d3ddm );

			m_vecDriver.push_back( CD3D9Driver( m_pD3D, iAdapter, adapterIdentifier, d3ddm ) );
		}

		return TRUE;
	}

	int CD3D9DriverList::Count()
	{
		return m_vecDriver.size();
	}

	CD3D9Driver* CD3D9DriverList::Item(int nDriver)
	{
		if (nDriver < 0 || nDriver > (m_vecDriver.size() - 1) )
		{
			return NULL;
		}

		return &m_vecDriver[nDriver];
	}
}