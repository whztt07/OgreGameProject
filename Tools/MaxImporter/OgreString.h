
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
}

#endif