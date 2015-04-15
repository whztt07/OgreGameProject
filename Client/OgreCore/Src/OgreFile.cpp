
#include "../inc/OgreFile.h"

namespace Ogre
{
	//-----------------------------
	//----------CFile--------------
	//-----------------------------
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

	//-----------------------------
	//--------CFilePath------------
	//-----------------------------
	CFilePath::CFilePath()
	{

	}

	CFilePath::~CFilePath()
	{

	}

	void CFilePath::Split(char *filename)
	{
		_splitpath( filename, m_szDrive, m_szDir, m_szName, m_szExt );
	}

	char* CFilePath::GetDrive()
	{
		return m_szDrive;
	}

	char* CFilePath::GetDirectory()
	{
		return m_szDir;
	}

	char* CFilePath::GetFileName()
	{
		return m_szName;
	}

	void CFilePath::SetCurDirectory(char *directory)
	{
		::SetCurrentDirectoryA(directory);
	}

	bool CFilePath::IsDirectory(char *pathname)
	{
		HANDLE fhd;
		WIN32_FIND_DATAA fd;
		
		if((fhd = ::FindFirstFileA(pathname, &fd))==INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}

		if((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			::FindClose(fhd);
			return TRUE;
		}
		else
		{
			::FindClose(fhd);
			return FALSE;
		}
	}

	bool CFilePath::IsFileExist(char *filename)
	{
		HANDLE fhd;
		WIN32_FIND_DATAA fd;

		if((fhd = ::FindFirstFileA(filename, &fd)) == INVALID_HANDLE_VALUE)
		{
			return FALSE;
		}

		if((fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE))
		{
			::FindClose(fhd);
			return TRUE;
		}
		else
		{
			::FindClose(fhd);
			return FALSE;
		}
	}

	bool CFilePath::MakeDirectory(char *pathname)
	{
		if( !pathname )
		{
			return false;
		}

		char szPath[MAX_PATH];
		strncpy(szPath, pathname, sizeof(szPath) - 1);

		int nLength = strlen( szPath );
		std::vector<std::string> vectorDir;
		char* pDirNameStart = szPath;
		for( unsigned int i = 0; i < nLength; i++ )
		{
			if( szPath[i] == '\\' || szPath[i] == '/')
			{
				szPath[i] = 0;
				vectorDir.push_back( pDirNameStart );
				pDirNameStart = &szPath[i + 1];
			}
		}

		szPath[0] = 0;
		for( int i = 0; i < vectorDir.size(); i++ )
		{
			size_t freesize = sizeof(szPath) - strlen(szPath);
			strncat( szPath, vectorDir[i].c_str(), freesize - 1 );
			if( !IsDirectory( szPath ) )
			{
				::CreateDirectoryA( szPath, NULL );
			}

			freesize = sizeof(szPath) - strlen(szPath);
			strncat( szPath, "\\", freesize - 1 );
		}

		return true;
	}
}