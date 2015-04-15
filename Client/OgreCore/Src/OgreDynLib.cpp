

#include "OgreDynLib.h"
#include "OgreException.h"
#include "OgreUseful.h"

namespace Ogre
{
	// -------------------------
	// Global Param
	// -------------------------
	CDynLibManager* CDynLibManager::s_Instance = NULL;

	// -------------------------
	// CDynLib
	// -------------------------
	CDynLib::CDynLib(CString szName)
	{
		m_szName = szName;
		m_hInst = NULL;
	}

	CDynLib::~CDynLib()
	{

	}

	CString CDynLib::DynLibError()
	{
		LPVOID lpMsgBuf; 
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS, 
			NULL, 
			GetLastError(), 
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
			(LPTSTR) &lpMsgBuf, 
			0, 
			NULL 
			); 
		CString ret = (char*)lpMsgBuf;
		// Free the buffer.
		LocalFree( lpMsgBuf );
		return ret;
	}

	void CDynLib::Load()
	{
#ifdef _DEBUG
		m_szName += "_d";
		m_hInst = (DYNLIB_HANDLE)DYNLIB_LOAD( m_szName.c_str() );
#else
		m_hInst = (DYNLIB_HANDLE)DYNLIB_LOAD( m_szName.c_str() );
#endif

		if( !m_hInst )
			OGRE_EXCEPT(
			CException::ERR_INTERNAL_ERROR, 
			"Could not load dynamic library " + m_szName + 
			".  System Error: " + DynLibError(),
			"DynLib::load" );
	}

	void CDynLib::UnLoad()
	{
		bool bUnLoad = DYNLIB_UNLOAD( m_hInst );
		if(!bUnLoad)
		{
			OGRE_EXCEPT(
				CException::ERR_INTERNAL_ERROR, 
				"Could not unload dynamic library " + m_szName +
				".  System Error: " + DynLibError(),
				"DynLib::unload");
		}
	}
	
	void* CDynLib::GetSymbol(CString szSymbol)
	{
		return (void*)DYNLIB_GETSYM( m_hInst, szSymbol.c_str() );
	}

	// -------------------------
	// CDynLibManager
	// -------------------------
	CDynLibManager::CDynLibManager()
	{
		s_Instance = this;
		m_mapDynLib.clear();
	}

	CDynLibManager::~CDynLibManager()
	{
		DynLibMap::iterator iter = m_mapDynLib.begin();
		for (; iter != m_mapDynLib.end(); iter++)
		{
			CDynLib *pDynLib = iter->second;
			pDynLib->UnLoad();
			SAFE_DELETE(pDynLib);
		}

		m_mapDynLib.clear();
	}

	CDynLibManager& CDynLibManager::Instance()
	{
		return *s_Instance;
	}

	CDynLib* CDynLibManager::Load(CString name)
	{
		DynLibMap::iterator iter = m_mapDynLib.find(name);
		if (iter != m_mapDynLib.end())
		{
			return iter->second;
		}

		CDynLib *pDynLib = new CDynLib(name);
		pDynLib->Load();
		m_mapDynLib[name] = pDynLib;
		return pDynLib;
	}

	void CDynLibManager::UnLoad(CDynLib *pDynLib)
	{
		CString szName = pDynLib->GetName();
		DynLibMap::iterator iter = m_mapDynLib.find(szName);
		if (iter != m_mapDynLib.end())
		{
			m_mapDynLib.erase(iter);
		}

		pDynLib->UnLoad();
		SAFE_DELETE(pDynLib);
	}
}