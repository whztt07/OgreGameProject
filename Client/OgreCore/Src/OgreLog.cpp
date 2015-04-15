

#include "OgreLog.h"
#include <time.h>
#include <iostream>
#include <iomanip>

namespace Ogre
{
	// -----------------
	// CLogListener
	// -----------------
	CLogListener::CLogListener()
	{

	}

	CLogListener::~CLogListener()
	{

	}

	void CLogListener::Message( const CString& message, LogMessageLevel lml)
	{
	}

	// -----------------
	// CLog
	// -----------------
	CLog::CLog(const CString& name)
	{
		mfpLog.open(name.c_str());
	}

	CLog::~CLog()
	{
		mfpLog.close();
	}

	void CLog::Log(std::string message, LogMessageLevel lml)
	{
		for( mtLogListener::iterator i = mListeners.begin(); i != mListeners.end(); ++i )
			(*i)->Message( message, lml);

		std::string desc;

		switch (lml)
		{
		case LML_TRIVIAL:
			desc = "Trivial";
			break;
		case LML_NORMAL:
			desc = "Normal";
			break;
		case LML_CRITICAL:
			desc = "Critical";
			break;
		}

		// Write time into log
		struct tm *pTime;
		time_t ctTime; time(&ctTime);
		pTime = localtime( &ctTime );
		mfpLog << desc << " " << std::setw(2) << std::setfill('0') << pTime->tm_hour
			<< ":" << std::setw(2) << std::setfill('0') << pTime->tm_min
			<< ":" << std::setw(2) << std::setfill('0') << pTime->tm_sec 
			<< ": " << message << std::endl;

		// Flush stcmdream to ensure it is written (incase of a crash, we need log to be up to date)
		mfpLog.flush();
	}

	void CLog::AddListener(CLogListener* listener)
	{
		mListeners.push_back(listener);
	}

	// -----------------
	// CLogManager
	// -----------------
	//template <> CLogManager* CSingleton<CLogManager>::msInstance = NULL;
	//template <> const char* CSingleton<CLogManager>::mClassTypeName("LogManager");

	CLogManager* CLogManager::s_Instance = NULL;

	CLogManager::CLogManager()
		:m_pDefaultLog(NULL)
	{
		s_Instance = this;
		m_mapLog.clear();
	}

	CLogManager::~CLogManager()
	{
		LogList::iterator i;
		for (i = m_mapLog.begin(); i != m_mapLog.end(); ++i)
		{
			SAFE_DELETE(i->second);
		}
	}

	CLogManager& CLogManager::Instance()
	{
		return *s_Instance;
	}

	CLog* CLogManager::CreateLog( const CString& name, bool defaultLog)
	{
		CLog* newLog = new CLog(name);

		if( !m_pDefaultLog || defaultLog )
		{
			m_pDefaultLog = newLog;
		}

		m_mapLog.insert( LogList::value_type( name, newLog ) );

		return newLog;
	}

	void CLogManager::Log( const CString& message, LogMessageLevel lml)
	{
		if (m_pDefaultLog)
			m_pDefaultLog->Log(message, lml);
	}
}