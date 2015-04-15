

#ifndef __OGRE_GL_TEXTURE_H__
#define __OGRE_GL_TEXTURE_H__

#include "OgrePrerequisites.h"
#include "OgreTexture.h"

namespace Ogre
{
	class OgreExport CGLTexture : public CTexture
	{
	public:
		CGLTexture(CString szGroup, CString szName);
		~CGLTexture();
	};
}

#endif