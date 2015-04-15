
#ifndef __OgrePrerequisites_h__
#define __OgrePrerequisites_h__

#include <windows.h>
#include <vector>
#include <map>
#include <string>
#include <stack>
#include <assert.h>
#include <stdio.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <math.h>
using namespace std;
#pragma comment(lib,"winmm.lib")

#define  DLL_FILE

//// message setting...
#ifdef DLL_FILE
#define OgreExport __declspec(dllexport)
#else
#define OgreExport __declspec(dllimport)
#endif

namespace Ogre
{
	#define MDX_TAG(x) (DWORD)( (((DWORD)x&0x0000ff00)<<8) + (((DWORD)x&0x000000ff)<<24) + \
		(((DWORD)x&0x00ff0000)>>8) + (((DWORD)x&0xff000000)>>24) )

	#define DC_TAG(x)  (DWORD)( (((DWORD)x&0x0000ff00)<<8) + (((DWORD)x&0x000000ff)<<24) + (((DWORD)x&0x00ff0000)>>8) + (((DWORD)x&0xff000000)>>24) )

	#define FRAMEPERSEC 30.0f
	#define ONEFRAMEINTERVAL (1000.0f / FRAMEPERSEC)
}

#endif