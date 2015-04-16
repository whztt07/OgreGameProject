/*===========================================================================*\
 | 
 |  FILE:	Plugin.cpp
 |			MorpherView Utility - demonstrates use of MorpherAPI access
 |			3D Studio MAX R3.0
 | 
 |  AUTH:   Harry Denholm
 |			Developer Consulting Group
 |			Copyright(c) Discreet 1999
 |
 |  HIST:	Started 4-4-99
 | 
\*===========================================================================*/

#include "MorpherView.h"

HINSTANCE hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) {
   if( fdwReason == DLL_PROCESS_ATTACH )
   {
      hInstance = hinstDLL;
      DisableThreadLibraryCalls(hInstance);
   }
	return(TRUE);
}



__declspec( dllexport ) const TCHAR *
LibDescription() { return GetString(IDS_LIBDESC); }


__declspec( dllexport ) int LibNumberClasses() 
{
	return 1;
}


__declspec( dllexport ) ClassDesc* LibClassDesc(int i) 
{
	switch(i) {
		case 0: return GetMorpherViewDesc();
		default: return 0;
	}
}


__declspec( dllexport ) ULONG LibVersion() { return VERSION_3DSMAX; }



TCHAR *GetString(int id)
{
	static TCHAR buf[256];
	if(hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;

	return NULL;
}
