
#include "OgreFile.h"

namespace Ogre
{
	CFile::CFile()
	{
		m_fp = 0;
		m_pBuffer = 0;
		m_dwFileSize = 0;
		m_dwFileOffset = 0;
	}

	CFile::~CFile()
	{

	}

	int CFile::LoadFileToMemory( const char* pszFilename, const char *pszMode, BYTE** ppBuffer )
	{
		FILE* fp = ::fopen( pszFilename, pszMode );
		if( fp == 0 )
			return false;
		::fseek ( fp, 0, SEEK_END );
		DWORD dwFileSize = ::ftell( fp );
		if( dwFileSize == 0 )
		{
			::fclose( fp );
			return 0;
		}
		BYTE* pBuffer = new BYTE[dwFileSize];
		::rewind( fp );
		::fread( pBuffer, dwFileSize, 1, fp );
		::fclose( fp );
		*ppBuffer = pBuffer;
		return dwFileSize;
	}

	void CFile::ReleaseFileMemory( BYTE** ppBuffer )
	{
		if( *ppBuffer )
		{
			delete []*ppBuffer;
			*ppBuffer = 0;
		}
	}

	bool CFile::fopen( const char* pszFilename, const char* pszMode )
	{
		fclose();
		
		m_dwFileSize = LoadFileToMemory( pszFilename, pszMode, &m_pBuffer );
		m_dwFileOffset = 0;
		if( m_dwFileSize > 0 )
			return true;

		return false;
	}

	void CFile::fclose()
	{
		if( m_fp )
		{
			::fclose( m_fp );
			m_fp = 0;
		}
		
		if(m_pBuffer )
		{
			ReleaseFileMemory( &m_pBuffer );
			m_pBuffer = NULL;
		}
	}

	BYTE* CFile::GetBuffer()
	{ 
		return m_pBuffer;
	}

	DWORD CFile::GetBufferSize()
	{ 
		return m_dwFileSize;
	}
}