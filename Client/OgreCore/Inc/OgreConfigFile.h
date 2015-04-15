

#ifndef __OgreConfigFile_H__
#define __OgreConfigFile_H__

#include "OgrePrerequisites.h"
#include "OgreString.h"
#include "OgreArchive.h"

namespace Ogre
{
	class OgreExport CConfigFile
	{
	public:
		CConfigFile();
		~CConfigFile();

		typedef std::multimap<CString, CString> SettingSection;
		typedef std::map<CString, SettingSection*> SettingSectionMap;

		void Load(char *filename, const CString& separators = "\t:=");
		SettingSectionMap& GetMap();
	private:
		void Load(CDataStream *pStream, const CString& separators = "\t:=");
		void Clear();

		SettingSectionMap m_mapSection;
	};
}

#endif