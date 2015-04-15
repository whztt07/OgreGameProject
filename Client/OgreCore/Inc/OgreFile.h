
#ifndef __OgreFile_h__
#define __OgreFile_h__

#include "OgrePrerequisites.h"

namespace Ogre
{
	class OgreExport CFile
	{
	public:
		CFile();
		~CFile();

		int LoadFileToMemory( const char* pszFilename, const char *pszMode, BYTE** ppBuffer );
		void ReleaseFileMemory( BYTE** ppBuffer );
		bool fopen( const char* pszFilename, const char* pszMode );
		void fclose();
		BYTE* GetBuffer();
		DWORD GetBufferSize();
	private:
		FILE* m_fp;
		BYTE* m_pBuffer;
		DWORD m_dwFileSize;
		DWORD m_dwFileOffset;
	};

	class OgreExport CFilePath
	{
	public:
		CFilePath();
		~CFilePath();

		static void SetCurDirectory(char *directory);
		static bool IsFileExist(char *filename);
		static bool IsDirectory(char *pathname);
		static bool MakeDirectory(char *pathname);
		void Split(char *filename);
		char* GetDrive();
		char* GetDirectory();
		char* GetFileName();
	private:
		char m_szDrive[MAX_PATH];
		char m_szDir[MAX_PATH];
		char m_szName[MAX_PATH];
		char m_szExt[MAX_PATH];
	};
}

#endif