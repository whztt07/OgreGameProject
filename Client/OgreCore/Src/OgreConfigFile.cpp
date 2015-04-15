

#include "OgreConfigFile.h"
#include "OgreUseful.h"

namespace Ogre
{
	CConfigFile::CConfigFile()
	{
		m_mapSection.clear();
	}

	CConfigFile::~CConfigFile()
	{

	}

	void CConfigFile::Clear()
	{
		SettingSectionMap::iterator iter = m_mapSection.begin();
		for (; iter != m_mapSection.end(); iter++)
		{
			SettingSection *pSection = iter->second;
			SAFE_DELETE(pSection);
		}

		m_mapSection.clear();
	}

	CConfigFile::SettingSectionMap& CConfigFile::GetMap()
	{
		return m_mapSection;
	}

	void CConfigFile::Load(char *filename, const CString& separators)
	{
		std::ifstream fp;

		fp.open(filename, std::ios::in | std::ios::binary);

		CDataStream *pDataStream = new CFileStreamDataStream(filename, &fp);
		if (pDataStream != NULL)
		{
			Load(pDataStream);
		}

		fp.close();
	}

	void CConfigFile::Load(CDataStream *pStream, const CString& separators)
	{
		Clear();

		CString line, currentSection, optName, optVal;
		SettingSection *pCurSection = NULL;
		while (!pStream->Eof())
		{
			line = pStream->GetLine();

			if ( line.length() > 0 && (line.at(0) != '#') && (line.at(0) != '@') )
			{
				if (line.at(0) == '[' && line.at(line.length() - 1) == ']')
				{
					currentSection = line.substr(1, line.length() - 2);
					SettingSectionMap::iterator iter = m_mapSection.find(currentSection);
					if (iter == m_mapSection.end())
					{
						pCurSection = new SettingSection;
						m_mapSection[currentSection] = pCurSection;
					}
					else
					{
						pCurSection = iter->second;
					}
				}
				else
				{
					// find the first seperator character and split the string there
					std::string::size_type separator_pos = line.find_first_of(separators, 0);
					if (separator_pos != std::string::npos)
					{
						optName = line.substr(0, separator_pos);

						// find the first non-seperator character following the name
						std::string::size_type nonseparator_pos = line.find_first_not_of(separators, separator_pos);

						// ... and extract the value
						// make sure we don't crash on an empty setting (it might be a valid value)
						optVal = (nonseparator_pos == std::string::npos) ? "" : line.substr(nonseparator_pos);
						CStringUtil::Trim(optVal);
						CStringUtil::Trim(optName);

						//pCurSection[optVal] = optName;
						pCurSection->insert(SettingSection::value_type(optName, optVal));
					}
				}
			}
		}
	}
}