

#ifndef _OgreLog_h__
#define _OgreLog_h__

#include "OgrePrerequisites.h"
#include "OgreSingleton.h"
#include "OgreString.h"

namespace Ogre
{
	enum LogMessageLevel
	{
		LML_TRIVIAL = 1,
		LML_NORMAL = 2,
		LML_CRITICAL = 3
	};

	class OgreExport CLogListener
	{
	public:
		CLogListener();
		virtual ~CLogListener();

	    virtual void Message( const CString& message, LogMessageLevel lml);
	};

	class OgreExport CLog
	{
	public:
		CLog(const CString& name);
		~CLog();

		void Log(std::string msg, LogMessageLevel lml = LML_NORMAL);

		void AddListener(CLogListener* listener);

		std::ofstream mfpLog;

		typedef std::vector<CLogListener*> mtLogListener;
		mtLogListener mListeners;
	};

	class OgreExport CLogManager
	{
	public:
		CLogManager();
		~CLogManager();

		static CLogManager& Instance();

		CLog* CreateLog( const CString& name, bool defaultLog = false);

		void Log( const CString& message, LogMessageLevel lml = LML_NORMAL);
	private:
		static CLogManager *s_Instance;
		
		typedef std::map<CString, CLog*> LogList;
		 LogList m_mapLog;
		 
		 CLog* m_pDefaultLog;
	};
}

#define OGRE_LOG(message, lml) {CLogManager::Instance().Log(message, lml);}

#endif