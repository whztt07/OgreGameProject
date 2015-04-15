

#ifndef __ModelEditor_h__
#define __ModelEditor_h__

#include "OgreEdEditor.h"
#include "OgreSingleton.h"

class CModelEditor : public OgreEd::CEditor, public Ogre::CSingleton<CModelEditor>
{
public:
	CModelEditor();
	~CModelEditor();
private:
	virtual void Render();
};

#endif