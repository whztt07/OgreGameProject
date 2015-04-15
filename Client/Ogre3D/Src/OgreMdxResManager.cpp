

#include "OgreMdxResManager.h"

namespace Ogre
{
	//--------------------------------
	//------------CRecord-------------
	//--------------------------------
	CRecord::CRecord()
	{

	}

	CRecord::~CRecord()
	{

	}

	//--------------------------------
	//--------CMdxResManager----------
	//--------------------------------
	CMdxResManager::CMdxResManager()
	{
		m_dwExternalBaseID = 10000;
	}
	
	CMdxResManager::~CMdxResManager()
	{

	}

	int	CMdxResManager::AddExternalFile( char *filename )
	{
		if( AddFile( filename, m_dwExternalBaseID ) )
		{
			return m_dwExternalBaseID++;
		}

		return -1;
	}

	bool CMdxResManager::AddFile(char *filename, int id)
	{
		CRecord* pRecord = new CRecord;
		strcpy( pRecord->m_szFileName, filename );
		pRecord->m_nID = id;

		m_mapRecord[id] = pRecord;
		m_vecRecord.push_back(pRecord);

		return true;
	}
}