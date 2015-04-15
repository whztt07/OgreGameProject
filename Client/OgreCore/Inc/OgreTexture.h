

#ifndef __OgreTexture_h__
#define __OgreTexture_h__

#include "OgrePrerequisites.h"
#include "OgreResource.h"

namespace Ogre
{
	class OgreExport CTexture : public CResource
	{
	public:
		CTexture(CString szGroup, CString szName);
		virtual ~CTexture();

		virtual void* GetRealTex(){return NULL;}
	protected:
		int m_nWidth;
		int m_nHeight;
	};
}

#endif