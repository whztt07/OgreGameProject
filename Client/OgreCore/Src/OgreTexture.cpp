

#include "OgreTexture.h"

namespace Ogre
{
	CTexture::CTexture(CString szGroup, CString szName)
		:CResource(szGroup, szName)
	{
		m_nWidth = 0;
		m_nHeight = 0;
	}

	CTexture::~CTexture()
	{

	}
}