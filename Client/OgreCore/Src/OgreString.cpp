

#include "OgreString.h"

namespace Ogre
{
	CStringUtil::CStringUtil()
	{

	}

	CStringUtil::~CStringUtil()
	{

	}

	void CStringUtil::Trim(CString &str, bool bLeft, bool bRight)
	{
		static const CString delims = " \t\r";
		if (bRight)
		{
			str.erase(str.find_last_not_of(delims) + 1); // trim right
		}

		if (bLeft)
		{
			str.erase(0, str.find_first_not_of(delims)); // trim left
		}
	}

	CString CStringUtil::Combine(CString szSrc, CString szSuffix, bool bUseName)
	{
		char szDrive[MAX_PATH];
		char szDir[MAX_PATH];
		char szName[MAX_PATH];
		char szExt[MAX_PATH];
		_splitpath( (char*)szSrc.c_str(), szDrive, szDir, szName, szExt );

		char szSkeleton[MAX_PATH];

		if (bUseName)
		{
			sprintf(szSkeleton, "%s%s%s%s", szDrive, szDir, szName, szSuffix.c_str());
		}
		else
		{
			sprintf(szSkeleton, "%s%s%s", szDrive, szDir, szSuffix.c_str());
		}
		
		return szSkeleton;
	}

	bool CStringUtil::Find(CString src, CString dest)
	{
		bool bFind = (src.find(dest) != -1);
		if (bFind)
		{
			char szDrive[MAX_PATH];
			char szDir[MAX_PATH];
			char szName[MAX_PATH];
			char szExt[MAX_PATH];
			_splitpath( (char*)src.c_str(), szDrive, szDir, szName, szExt );
			if (!stricmp(szExt, dest.c_str()))
			{
				return true;
			}
		}

		return false;
	}
}