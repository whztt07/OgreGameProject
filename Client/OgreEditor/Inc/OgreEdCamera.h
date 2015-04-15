

#ifndef __OgreEdCamera_h__
#define __OgreEdCamera_h__

#include "OgreEdPrerequisites.h"
#include "OgreSingleton.h"

namespace Ogre
{
	class CCamera;
}

namespace OgreEd
{
	class OgreExport CEdCamera //: public CEdSingleton<CEdCamera>
	{
	public:
		CEdCamera(Ogre::CCamera *cam);
		~CEdCamera();

		Ogre::CCamera* GetCamera(){ return m_pCamera; }
	private:
		Ogre::CCamera *m_pCamera;
	};
}

#endif