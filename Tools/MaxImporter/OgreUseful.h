
#ifndef __OgreUseful_h__
#define __OgreUseful_h__

#include "OgrePrerequisites.h"

#define SAFE_DELETE_ARRAY(p) {if(p){delete []p; p = NULL;}}
#define SAFE_DELETE(p) {if(p){delete p; p = NULL;}}
#define SAFE_RELEASE(p) {if(p){p->Release(); p = NULL;}}

DWORD OgreExport HQ_TimeGetTime();

#endif