

#include "ModelEditor.h"
#include "OgreEdCamera.h"

template <> CModelEditor* Ogre::CSingleton<CModelEditor>::msInstance = NULL;
template <> const char* Ogre::CSingleton<CModelEditor>::mClassTypeName("ModelEditor");

CModelEditor::CModelEditor()
:OgreEd::CEditor()
{

}

CModelEditor::~CModelEditor()
{

}

void CModelEditor::Render()
{
}