

#ifndef __OgreArchive_h__
#define __OgreArchive_h__

#include "OgrePrerequisites.h"
#include "OgreString.h"

namespace Ogre
{
	class CArchive;
	class CFileArchive;
	class CMpqArchive;
	class CZipArchive;
	class CArchiveFactory;
	class CFileArchiveFactory;
	class CMpqArchiveFactory;
	class CZipArchiveFactory;
	class CDataStream;
	class CMemoryDataStream;
	class CFileStreamDataStream;
	class CFileHandleDataStream;
	class CMpqDataStream;
	class CZipDataStream;

	struct OgreExport CFileInfo
	{
		CString m_szName;
		int m_nSize;
		int m_nCompSize;
	};

	class OgreExport CArchive
	{
	public:
		CArchive();
		CArchive(CString type, CString name);
		virtual ~CArchive();

		CString& GetName();
		CString& GetType();

		virtual void Load();
		virtual void UnLoad();
		virtual int GetFileNum() = 0;
		virtual CString GetFileName(int nId) = 0;
		virtual CDataStream* Open(char* filename) = 0;
	protected:
		CString m_szName;
		CString m_szType;
	};

	class OgreExport CFileArchive : public CArchive
	{
	public:
		CFileArchive(CString name);
		~CFileArchive();

		virtual CDataStream* Open(char* filename);
		virtual int GetFileNum();
		virtual CString GetFileName(int nId);
	};

	class OgreExport CMpqArchive : public CArchive
	{
	public:
		CMpqArchive(CString name);
		~CMpqArchive();

		virtual void Load();
		virtual void UnLoad();
		virtual CDataStream* Open(char* filename);
		virtual int GetFileNum();
		virtual CString GetFileName(int nId);
	private:
		typedef std::vector<CFileInfo> FileList;
		FileList m_vecFile;
		CString m_szDir;
		HANDLE m_hMpq;
	};

	class OgreExport CZipArchive : public CArchive
	{
	public:
		CZipArchive(CString name);
		~CZipArchive();

		virtual void Load();
		virtual void UnLoad();
		virtual CDataStream* Open(char* filename);
		virtual int GetFileNum();
		virtual CString GetFileName(int nId);
	};

	class OgreExport CArchiveFactory
	{
	public:
		CArchiveFactory();
		~CArchiveFactory();

		CString GetType();
		void DestroyInstance(CArchive *pArch);
		virtual CArchive* CreateInstance(CString filename) = 0;
	protected:
		CString m_szType;
	};

	class OgreExport CFileArchiveFactory : public CArchiveFactory
	{
	public:
		CFileArchiveFactory();
		~CFileArchiveFactory();

		virtual CArchive* CreateInstance(CString filename);
	};

	class OgreExport CMpqArchiveFactory : public CArchiveFactory
	{
	public:
		CMpqArchiveFactory();
		~CMpqArchiveFactory();

		virtual CArchive* CreateInstance(CString filename);
	};

	class OgreExport CZipArchiveFactory : public CArchiveFactory
	{
	public:
		CZipArchiveFactory();
		~CZipArchiveFactory();

		virtual CArchive* CreateInstance(CString filename);
	};

	class OgreExport CDataStream
	{
	public:
		CDataStream();
		~CDataStream();

		virtual void Open(CString filename){}
		virtual int Read(void* pBuf, int nCount) = 0;
		virtual void Close() = 0;
		virtual byte* GetBuffer(){return NULL;}
		virtual bool Eof(){return true;}
		virtual void Skip(long nCount){}
		CString GetLine();
		int GetSize();
	protected:
		CString m_szName;
		int m_nSize;
	};

	class OgreExport CMemoryDataStream : public CDataStream
	{
	public:
		CMemoryDataStream();
		~CMemoryDataStream();

		virtual int Read(void* pBuf, int nCount){return true;}
		virtual void Close(){}
	};

	class OgreExport CFileStreamDataStream : public CDataStream
	{
	public:
		CFileStreamDataStream(CString name, std::ifstream *fp);
		~CFileStreamDataStream();

		virtual int Read(void* pBuf, int nCount);
		virtual void Close(){}
		virtual bool Eof();
		virtual void Skip(long nCount);
	private:
		std::ifstream *m_pStream;
	};

	class OgreExport CFileHandleDataStream : public CDataStream
	{
	public:
		CFileHandleDataStream();
		~CFileHandleDataStream();

		virtual void Open(CString filename);
		virtual int Read(void* pBuf, int nCount);
		virtual void Close();
	private:
		FILE *m_pFile;
	};

	class OgreExport CMpqDataStream : public CDataStream
	{
	public:
		CMpqDataStream(HANDLE hMpq);
		~CMpqDataStream();

		virtual void Open(CString filename);
		virtual int Read(void* pBuf, int nCount);
		virtual void Close();
		virtual byte* GetBuffer();
	private:
		HANDLE m_hMpq;
		HANDLE m_hFile;
		byte *m_pByte;
	};

	class OgreExport CZipDataStream : public CDataStream
	{
	public:
		CZipDataStream();
		~CZipDataStream();

		virtual int Read(void* pBuf, int nCount);
		virtual void Close();
	};

	class OgreExport CArchiveManager
	{
	public:
		CArchiveManager();
		~CArchiveManager();

		static CArchiveManager& Instance();

		void AddArchiveFactory(CArchiveFactory *pArchFactory);
		CArchive* Load(CString filename, CString szType);
		void UnLoad(char *filename);
		CArchive* Find(CString filename);
	private:
		static CArchiveManager *s_Instance;

		typedef std::map<CString, CArchiveFactory*> ArchiveFactoryMap;
		typedef std::map<CString, CArchive*> ArchiveMap;
		ArchiveFactoryMap m_mapFactory;
		ArchiveMap m_mapArchive;
	};
}

#endif