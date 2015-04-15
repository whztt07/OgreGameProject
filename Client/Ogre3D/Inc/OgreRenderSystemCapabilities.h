

#ifndef __OgreRenderSystemCapabilities_H__
#define __OgreRenderSystemCapabilities_H__

#include "OgreString.h"

namespace Ogre
{
	struct OgreExport CDriverVersion 
	{
		CDriverVersion();

		int m_nMajor;
		int m_nMinor;
		int m_nRelease;
		int m_nBuild;
	};
}

#endif