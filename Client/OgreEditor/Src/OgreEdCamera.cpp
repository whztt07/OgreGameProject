

#include "OgreEdCamera.h"
#include "OgreCamera.h"

//template <> CEdCamera* Ogre::CSingleton<CEdCamera>::msInstance = 0;
//template <> const char* Ogre::CSingleton<CEdCamera>::mClassTypeName("EdCamera");

namespace OgreEd
{
	CEdCamera::CEdCamera(Ogre::CCamera *cam)
		: m_pCamera(cam)
	{
	}

	CEdCamera::~CEdCamera()
	{

	}
}