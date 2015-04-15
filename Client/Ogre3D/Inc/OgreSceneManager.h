

#ifndef __OgreSceneManager_h__
#define __OgreSceneManager_h__

#include "OgrePrerequisites.h"
#include "OgreString.h"

namespace Ogre
{
	class CMdxEntity;

	class OgreExport CSceneManager
	{
	public:
		CSceneManager();
		~CSceneManager();

		static CSceneManager& Instance();

		CMdxEntity* CreateEntity(CString szEntityName, CString szMeshName);
		void ClearEntity();
		void RenderScene();
	private:
		static CSceneManager *s_Instance;
		typedef std::vector<CMdxEntity*> EntityList;
		EntityList m_vecEntity;
	};
}

#endif