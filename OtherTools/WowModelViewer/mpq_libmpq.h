
#ifndef __mpq_libmpq_h_
#define __mpq_libmpq_h_

#include "mpq.h"
#include <set>
#include <windows.h>
#include <string>

class CMPQArchive1
{
	mpq_archive mpq_a;
	HANDLE hMpq;
public:
	CMPQArchive1(const char* filename);
	~CMPQArchive1();

	void Close();
};

struct CFileTreeItem 
{
	std::string fn;
	int col;

	/// Comparison
	bool operator < (const CFileTreeItem &i) const
	{
		return fn < i.fn;
	}

	bool operator > (const CFileTreeItem &i) const
	{
		return fn < i.fn;
	}
};

class CMPQFile
{
	//MPQHANDLE handle;
	bool eof;
	unsigned char *buffer;
	size_t pointer, size;

	// disable copying
	CMPQFile(const CMPQFile &f) {}
	void operator=(const CMPQFile &f) {}

public:
	CMPQFile(const char* filename);	// filenames are not case sensitive
	~CMPQFile();
	size_t read(void* dest, size_t bytes);
	size_t getSize();
	size_t getPos();
	unsigned char* getBuffer();
	unsigned char* getPointer();
	bool isEof();
	void seek(int offset);
	void seekRelative(int offset);
	void close();

	static bool exists(const char* filename);
	static int getSize(const char* filename); // Used to do a quick check to see if a file is corrupted
};

inline bool defaultFilterFunc(std::string) { return true; }
void GetFileLists2(std::set<CFileTreeItem> &dest, bool filterfunc(std::string) = defaultFilterFunc);
void GetFileLists(std::set<CFileTreeItem> &dest, bool filterfunc(std::string) = defaultFilterFunc);


#endif