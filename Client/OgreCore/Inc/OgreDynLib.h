

#ifndef __OgreDynLib_h__
#define __OgreDynLib_h__

#include "OgrePrerequisites.h"
#include "OgreString.h"

#define DYNLIB_HANDLE HINSTANCE
#define DYNLIB_LOAD(a) LoadLibraryExA(a, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)
#define DYNLIB_GETSYM(a, b) GetProcAddress(a, b)
#define DYNLIB_UNLOAD(a) !FreeLibrary(a)

namespace Ogre
{
	class OgreExport CPlugin
	{
	public:
		CPlugin(){}
		virtual ~CPlugin(){}

		virtual CString GetName(){return m_szName;}
		virtual void Install() = 0;
		virtual void UnInstall() = 0;
		virtual void Initialise() = 0;
		virtual void ShutDown() = 0;
	protected:
		CString m_szName;
	};

	class OgreExport CDynLib
	{
	public:
		CDynLib(CString szName);
		~CDynLib();

		void Load();
		void UnLoad();
		void* GetSymbol(CString szSymbol);
		CString GetName(){return m_szName;}
	protected:
		CString DynLibError();

		CString m_szName;
		DYNLIB_HANDLE m_hInst;
	};

	class OgreExport CDynLibManager
	{
	public:
		CDynLibManager();
		virtual ~CDynLibManager();

		static CDynLibManager& Instance();
		CDynLib* Load(CString name);
		void UnLoad(CDynLib *pDynLib);
	protected:
		static CDynLibManager *s_Instance;
		typedef std::map<CString, CDynLib*> DynLibMap;
		DynLibMap m_mapDynLib;
	};
}

#endif