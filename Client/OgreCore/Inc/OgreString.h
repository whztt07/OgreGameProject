
#ifndef __OgreString_h__
#define __OgreString_h__

#include "OgrePrerequisites.h"

namespace Ogre
{
#if OGRE_WCHAR_T_STRINGS
	typedef std::wstring _StringBase;
#else
	typedef std::string CString;
#endif
	typedef std::vector<CString> StringVector;
	
	class OgreExport CStringUtil
	{
	public:
		CStringUtil();
		~CStringUtil();

		static void Trim(CString &str, bool bLeft = true, bool bRight = true);
		static CString Combine(CString szSrc, CString szSuffix, bool bUseName = false);
		static bool Find(CString src, CString dest);
	};
}

#endif