

#ifndef __OgreRenderSystemCapabilities_H__
#define __OgreRenderSystemCapabilities_H__

#include "Ogre3DInclude.h"
#include "OgreString.h"

struct OgreExport CDriverVersion 
{
	CDriverVersion();

	int m_nMajor;
	int m_nMinor;
	int m_nRelease;
	int m_nBuild;
};

#endif