

#ifndef __OgreGui_h__
#define __OgreGui_h__

#include "OgrePrerequisites.h"

namespace Ogre
{
	//---------------------------------------------
	// Enums for pre-defined control types
	//---------------------------------------------
	enum OGRE_GUI_CONTROL_TYPE
	{
		OGRE_GUI_CONTROL_STATIC,
		OGRE_GUI_CONTROL_BUTTON,
		OGRE_GUI_CONTROL_CHECKBOX,
		OGRE_GUI_CONTROL_RADIOBUTTON,
		OGRE_GUI_CONTROL_SLIDER,
		OGRE_GUI_CONTROL_SCROLLBAR,
		OGRE_GUI_CONTROL_COMBOBOX,
		OGRE_GUI_CONTROL_LISTBOX,
	};

	//---------------------------------------------
	// Enum for per-defined control states
	//---------------------------------------------
	enum OGRE_GUI_CONTROL_STATE
	{
		OGRE_GUI_CONTROL_NORMAL = 0,
		OGRE_GUI_CONTROL_DISABLED,
		OGRE_GUI_CONTROL_HIDDEN,
		OGRE_GUI_CONTROL_FOCUS,
		OGRE_GUI_CONTROL_MOUSEOVER,
		OGRE_GUI_CONTROL_PRESSED,
	};

	class OgreExport CGui
	{
	public:
		CGui();
		~CGui();
	};
}

#endif