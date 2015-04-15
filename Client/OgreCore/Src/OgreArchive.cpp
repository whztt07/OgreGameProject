

#include "OgreArchive.h"
#include "OgreUseful.h"
#include "OgreException.h"

#include <sys/types.h>
#include <sys/stat.h>

#define __STORMLIB_SELF__
#include "StormLib.h"

namespace Ogre
{
	// -------------------------
	// Global Param
	// -------------------------
	CArchiveManager* CArchiveManager::s_Instance = NULL;

	// -------------------------
	// CArchive
	// -------------------------
	CArchive::CArchive()
	{

	}

	CArchive::CArchive(CString type, CString name)
	{
		m_szType = type;
		m_szName = name;
	}

	CArchive::~CArchive()
	{

	}

	CString& CArchive::GetName()
	{
		return m_szName;
	}

	CString& CArchive::GetType()
	{
		return m_szType;
	}

	void CArchive::Load()
	{

	}

	void CArchive::UnLoad()
	{

	}

	// -------------------------
	// CFileArchive
	// -------------------------
	CFileArchive::CFileArchive(CString name)
		:CArchive("File", name)
	{
	}

	CFileArchive::~CFileArchive()
	{

	}

	CDataStream* CFileArchive::Open(char* filename)
	{
		CDataStream *pDataStream = new CFileHandleDataStream;
		pDataStream->Open(filename);
		
		return pDataStream;
	}

	int CFileArchive::GetFileNum()
	{
		return 0;
	}

	CString CFileArchive::GetFileName(int nId)
	{
		return "";
	}

	// -------------------------
	// CMpqArchive
	// -------------------------
	CMpqArchive::CMpqArchive(CString name)
		:CArchive("Mpq", name)
	{
		m_hMpq = NULL;
	}

	CMpqArchive::~CMpqArchive()
	{

	}

	void CMpqArchive::Load()
	{
		int nLen = strlen(m_szName.c_str()) + 1;  
		int nwLen = MultiByteToWideChar(CP_ACP, 0, m_szName.c_str(), nLen, NULL, 0);  
		TCHAR lpszFile[MAX_PATH];  
		MultiByteToWideChar(CP_ACP, 0, m_szName.c_str(), nLen, lpszFile, nwLen);  
		SFileOpenArchive(lpszFile, 0, MPQ_OPEN_FORCE_MPQ_V1 | MPQ_OPEN_READ_ONLY, &m_hMpq);
		
		m_szDir = CStringUtil::Combine(m_szName, "", false);

		SFILE_FIND_DATA data; bool bFind = true;
		HANDLE hFindFile = SFileFindFirstFile(m_hMpq, "*.*", &data, NULL);
		while (hFindFile != NULL && bFind)
		{
			CFileInfo info;
			info.m_szName = data.cFileName;
			info.m_nSize = data.dwFileSize;
			info.m_nCompSize = data.dwCompSize;
			m_vecFile.push_back(info);

			bFind = SFileFindNextFile(hFindFile, &data);
		}
		SFileFindClose(hFindFile);
	}

	void CMpqArchive::UnLoad()
	{
		SFileCloseArchive(m_hMpq);
	}

	CDataStream* CMpqArchive::Open(char* filename)
	{
		CDataStream *pDataStream = new CMpqDataStream(m_hMpq);
		pDataStream->Open(filename);

		return pDataStream;
	}

	int CMpqArchive::GetFileNum()
	{
		return m_vecFile.size();
	}

	CString CMpqArchive::GetFileName(int nId)
	{
		int nNum = m_vecFile.size();
		if (nId < 0 || nId > (nNum - 1))
		{
			return "";
		}

		//CString szFile = m_szName + "\\" + m_vecFile[nId].m_szName;
		CString szFile = m_vecFile[nId].m_szName;
		return szFile;
	}

	// -------------------------
	// CZipArchive
	// -------------------------
	CZipArchive::CZipArchive(CString name)
	{

	}

	CZipArchive::~CZipArchive()
	{

	}

	void CZipArchive::Load()
	{

	}

	void CZipArchive::UnLoad()
	{

	}

	CDataStream* CZipArchive::Open(char* filename)
	{
		return NULL;
	}

	int CZipArchive::GetFileNum()
	{
		return 0;
	}

	CString CZipArchive::GetFileName(int nId)
	{
		return "";
	}

	// -------------------------
	// CArchiveFactory
	// -------------------------
	CArchiveFactory::CArchiveFactory()
	{

	}

	CArchiveFactory::~CArchiveFactory()
	{

	}

	CString CArchiveFactory::GetType()
	{
		return m_szType;
	}

	void CArchiveFactory::DestroyInstance(CArchive *pArch)
	{
		SAFE_DELETE(pArch);
	}

	// -------------------------
	// CFileArchiveFactory
	// -------------------------
	CFileArchiveFactory::CFileArchiveFactory()
	{
		m_szType = "File";
	}

	CFileArchiveFactory::~CFileArchiveFactory()
	{

	}

	CArchive* CFileArchiveFactory::CreateInstance(CString filename)
	{
		return new CFileArchive(filename);
	}

	// -------------------------
	// CMpqArchiveFactory
	// -------------------------
	CMpqArchiveFactory::CMpqArchiveFactory()
	{
		m_szType = "Mpq";
	}

	CMpqArchiveFactory::~CMpqArchiveFactory()
	{

	}

	CArchive* CMpqArchiveFactory::CreateInstance(CString filename)
	{
		return new CMpqArchive(filename);
	}

	// -------------------------
	// CZipArchiveFactory
	// -------------------------
	CZipArchiveFactory::CZipArchiveFactory()
	{
		m_szType = "Zip";
	}

	CZipArchiveFactory::~CZipArchiveFactory()
	{

	}

	CArchive* CZipArchiveFactory::CreateInstance(CString filename)
	{
		return new CZipArchive(filename);
	}

	// -------------------------
	// CDataStream
	// -------------------------
	CDataStream::CDataStream()
	{
		m_nSize = 0;
	}

	CDataStream::~CDataStream()
	{

	}

	int CDataStream::GetSize()
	{
		return m_nSize;
	}

	CString CDataStream::GetLine()
	{
		// keep looping while not hitting delimiter
		int nReadCount;
		char szTmpBuf[MAX_PATH];
		CString szRet;
		while ( (nReadCount = Read(szTmpBuf, MAX_PATH - 1)) != 0)
		{
			// Terminate string
			szTmpBuf[nReadCount] = '\0';

			char *p = strchr(szTmpBuf, '\n');
			if (p != 0)
			{
				// reposition backwards
				Skip((long)(p + 1 - szTmpBuf - nReadCount));
				*p = '\0';
			}

			szRet = szTmpBuf;

			if (p != 0)
			{
				// trim off trailing cr if this was a cr/lf entry
				if (szRet.length() && szRet[szRet.length() - 1] == '\r')
				{
					szRet.erase(szRet.length() - 1, 1);
				}

				// found terminator, break out
				break;
			}
		}

		CStringUtil::Trim(szRet);

		return szRet;
	}

	// -------------------------
	// CMemoryDataStream
	// -------------------------
	CMemoryDataStream::CMemoryDataStream()
	{

	}

	CMemoryDataStream::~CMemoryDataStream()
	{

	}

	// -------------------------
	// CFileStreamDataStream
	// -------------------------
	CFileStreamDataStream::CFileStreamDataStream(CString name, std::ifstream *fp)
		:CDataStream()
	{
		m_pStream = fp;
		
		// calculate the size
		m_pStream->seekg(0, std::ios_base::end);
		m_nSize = m_pStream->tellg();
		m_pStream->seekg(0, std::ios_base::beg);
	}

	CFileStreamDataStream::~CFileStreamDataStream()
	{

	}

	bool CFileStreamDataStream::Eof()
	{
		if (m_pStream != NULL)
		{
			return m_pStream->eof();
		}

		return true;
	}

	int CFileStreamDataStream::Read(void* pBuf, int nCount)
	{
		m_pStream->read(static_cast<char*>(pBuf), static_cast<std::streamsize>(nCount));
		return m_pStream->gcount();
	}

	void CFileStreamDataStream::Skip(long nCount)
	{
		if (m_pStream != NULL)
		{
			m_pStream->clear();
			m_pStream->seekg(static_cast<std::ifstream::pos_type>(nCount), std::ios::cur);
		}
	}

	// -------------------------
	// CFileHandleDataStream
	// -------------------------
	CFileHandleDataStream::CFileHandleDataStream()
	{
		m_pFile = NULL;
	}

	CFileHandleDataStream::~CFileHandleDataStream()
	{

	}

	void CFileHandleDataStream::Open(CString filename)
	{
		m_pFile = fopen(filename.c_str(), "rb");
		if (m_pFile == NULL)
		{
			OGRE_EXCEPT(CException::ERR_FILE_NOT_FOUND,
				"Cannot open file: " + filename,
				"CFileHandleDataStream::Open");
		}

		struct stat tagStat;
		int ret = stat(filename.c_str(), &tagStat);
		m_nSize = tagStat.st_size;
	}

	int CFileHandleDataStream::Read(void* pBuf, int nCount)
	{
		fread(pBuf, nCount, 1, m_pFile);
		return 0;
	}

	void CFileHandleDataStream::Close()
	{
		if (m_pFile != NULL)
		{
			fclose(m_pFile);
			m_pFile = NULL;
		}
	}

	// -------------------------
	// CMpqDataStream
	// -------------------------
	CMpqDataStream::CMpqDataStream(HANDLE hMpq)
	{
		m_hMpq = hMpq;
		m_hFile = NULL;
	}

	CMpqDataStream::~CMpqDataStream()
	{
		SAFE_DELETE_ARRAY(m_pByte);
		this->Close();
	}

	void CMpqDataStream::Open(CString filename)
	{
		SFileOpenFileEx(m_hMpq, filename.c_str(), SFILE_OPEN_FROM_MPQ, &m_hFile);
		if (m_hFile == NULL)
		{
			// do something ...
		}

		m_nSize = SFileGetFileSize(m_hFile, 0);
		m_pByte = new byte[m_nSize];
		Read(m_pByte, m_nSize);
	}

	int CMpqDataStream::Read(void* pBuf, int nCount)
	{
		SFileReadFile(m_hFile, pBuf, m_nSize, (LPDWORD)&m_nSize, NULL);

		return 0;
	}

	void CMpqDataStream::Close()
	{
		SFileCloseFile(m_hFile);
	}

	byte* CMpqDataStream::GetBuffer()
	{
		return m_pByte;
	}

	// -------------------------
	// CZipDataStream
	// -------------------------
	CZipDataStream::CZipDataStream()
	{

	}

	CZipDataStream::~CZipDataStream()
	{

	}

	int CZipDataStream::Read(void* pBuf, int nCount)
	{
		return 0;
	}

	void CZipDataStream::Close()
	{

	}

	// -------------------------
	// CArchiveManager
	// -------------------------
	CArchiveManager::CArchiveManager()
	{
		s_Instance = this;
		m_mapFactory.clear();
		m_mapArchive.clear();
	}

	CArchiveManager::~CArchiveManager()
	{

	}

	CArchiveManager& CArchiveManager::Instance()
	{
		return *s_Instance;
	}

	void CArchiveManager::AddArchiveFactory(CArchiveFactory *pArchFactory)
	{
		CString &type = pArchFactory->GetType();
		m_mapFactory[type] = pArchFactory;
	}

	CArchive* CArchiveManager::Load(CString filename, CString szType)
	{
		ArchiveMap::iterator iter = m_mapArchive.find(filename);
		if (iter != m_mapArchive.end())
		{
			return iter->second;
		}

		ArchiveFactoryMap::iterator iterFac = m_mapFactory.find(szType);
		if (iterFac == m_mapFactory.end())
		{	
			// factory not found
			OGRE_EXCEPT(CException::ERR_ITEM_NOT_FOUND, 
					"Cannot find an archive factory to deal with archive of type " + szType,
					"ArchiveManager::load");
		}

		CArchive *pArchive = NULL;
		CArchiveFactory *pFactory = iterFac->second;
		if (pFactory != NULL)
		{
			pArchive = pFactory->CreateInstance(filename);
			pArchive->Load();
			m_mapArchive[filename] = pArchive;
		}

		return pArchive;
	}

	void CArchiveManager::UnLoad(char *filename)
	{
		ArchiveMap::iterator iter = m_mapArchive.find(filename);
		if (iter == m_mapArchive.end())
		{
			return;
		}

		CArchive *pArchive = iter->second;
		if (pArchive != NULL)
		{
			pArchive->UnLoad();

			ArchiveFactoryMap::iterator iterFac = m_mapFactory.find(pArchive->GetType());
			if (iterFac == m_mapFactory.end())
			{
				// Factory not found
				OGRE_EXCEPT(CException::ERR_ITEM_NOT_FOUND,
					"Cannot find an archive factory to deal with archive of type " + pArchive->GetType(),
					"ArchiveManager::UnLoad");
			}

			iterFac->second->DestroyInstance(pArchive);
			m_mapArchive.erase(iter);
		}
	}

	CArchive* CArchiveManager::Find(CString filename)
	{
		ArchiveMap::iterator iter = m_mapArchive.find(filename);
		if (iter != m_mapArchive.end())
		{
			return iter->second;
		}

		return NULL;
	}
}
