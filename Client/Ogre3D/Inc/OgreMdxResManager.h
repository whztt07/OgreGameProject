

#ifndef __OgreMdxResManager_h__
#define __OgreMdxResManager_h__

#include "OgrePrerequisites.h"
#include "OgreMath.h"

namespace Ogre
{
	class CRecord
	{
	public:
		CRecord();
		~CRecord();
		
		char	m_szFileName[MAX_PATH];
		int		m_nID;
	};

	class CMdxResManager
	{
	public:
		CMdxResManager();
		~CMdxResManager();

		int	AddExternalFile( char *filename );
	private:
		bool AddFile(char *filename, int id);

		typedef std::map<int, CRecord*> RecordMap;
		typedef std::vector<CRecord*> RecordList;
		RecordMap m_mapRecord;
		RecordList m_vecRecord;
		DWORD m_dwExternalBaseID;
	};
}

#endif