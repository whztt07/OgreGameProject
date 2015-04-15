
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
	public:
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
}

#endif