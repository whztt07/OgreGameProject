

#ifndef __OgreEdEditor_h__
#define __OgreEdEditor_h__

#include "OgreEdPrerequisites.h"

namespace Ogre
{
	class CRoot;
}

namespace OgreEd
{
	class CEdCamera;

	class OgreExport CEditor
	{
	public:
		CEditor();
		virtual ~CEditor();

		CEdCamera* GetCamera(){ return m_pCamera; }

		void Create(HWND hWnd);

		virtual void RenderEditor();

		virtual void ResetEditor(int width, int height);
	protected:
		void SetupResources();

		virtual void Render(){}

		Ogre::CRoot *m_pRoot;
		CEdCamera *m_pCamera;
	};
}

#endif