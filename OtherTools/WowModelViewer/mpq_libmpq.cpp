

#include "mpq_libmpq.h"
#include "OgreUseful.h"
#include <QMessageBox>
#include <atlbase.h>

#define __STORMLIB_SELF__
#include "StormLib.h"

typedef std::vector<mpq_archive*> ArchiveSet;
typedef std::map<std::string, HANDLE> ArchiveMap;
ArchiveSet gOpenArchives;
ArchiveMap gOpenArchivesMap;
//------------------------------------
//------------CMPQArchive1-------------
//------------------------------------
CMPQArchive1::CMPQArchive1(const char* filename)
{
	int result = libmpq_archive_open(&mpq_a, (unsigned char*)filename);

	if(result) 
	{
		QMessageBox::Icon icon = QMessageBox::NoIcon;
		QMessageBox(icon, QString("MPQArchiveOpen"), QString("Error opening archive %s"));
		return;
	}

	gOpenArchives.push_back(&mpq_a);
}

//CMPQArchive1::CMPQArchive1(const char* filename)
//:hMpq(NULL)
//{
//	int nLen = strlen(filename) + 1;  
//	int nwLen = MultiByteToWideChar(CP_ACP, 0, filename, nLen, NULL, 0);  
//	TCHAR   lpszFile[256];  
//	MultiByteToWideChar(CP_ACP, 0, filename, nLen, lpszFile, nwLen);  
//
//	if (!SFileOpenArchive(lpszFile, 0, MPQ_OPEN_FORCE_MPQ_V1|MPQ_OPEN_READ_ONLY, &hMpq )) 
//	{
//		return;
//	}
//
//	gOpenArchivesMap[filename] = hMpq;
//}

CMPQArchive1::~CMPQArchive1()
{
}

void CMPQArchive1::Close()
{
	libmpq_archive_close(&mpq_a);

	for(ArchiveSet::iterator it=gOpenArchives.begin(); it!=gOpenArchives.end();++it)
	{
		mpq_archive &mpq_b = **it;
		if (&mpq_b == &mpq_a) 
		{
			gOpenArchives.erase(it);
			return;
		}
	}
}

//void CMPQArchive1::Close()
//{
//	SFileCloseArchive(hMpq);
//	for(ArchiveMap::iterator iter = gOpenArchivesMap.begin(); iter != gOpenArchivesMap.end(); ++iter)
//	{
//		HANDLE handle = iter->second;
//		if (handle == hMpq)
//		{
//			gOpenArchivesMap.erase(iter);
//			return;
//		}
//	}
//}

//------------------------------------
//------------------------------------
//------------------------------------
void GetFileLists2(std::set<CFileTreeItem> &dest, bool filterfunc(std::string))
{
	for(ArchiveMap::iterator i=gOpenArchivesMap.begin(); i!=gOpenArchivesMap.end();++i)
	{
		HANDLE &mpq_a = i->second;
		HANDLE fh;
		
		if( SFileOpenFileEx( mpq_a, "(listfile)", 0, &fh ) )
		{
			// Found!
			DWORD filesize = SFileGetFileSize( fh, NULL);
			size_t size = filesize;

			QString temp = i->first.c_str();
			temp = temp.toLower();
			int col = 0; // Black

			if ((temp.contains("wow-update-")) || (temp.contains("patch.mpq")))
				col = 1; // Blue
			else if (temp.contains("cache") || temp.contains("patch-2.mpq"))
				col = 2; // Red
			else if(temp.contains("expansion1.mpq") || temp.contains("expansion.mpq"))
				col = 3; // Outlands Purple
			else if (temp.contains("expansion2.mpq") || temp.contains("lichking.mpq"))
				col = 4; // Frozen Blue
			else if (temp.contains("expansion3.mpq") )
				col = 5; // Destruction Orange
			else if (temp.contains("expansion4.mpq")  || temp.contains("patch-3.mpq") )
				col = 6; // Bamboo Green
			else if (temp.contains("alternate.mpq") )
				col = 7; // Cyan

			if (size > 0 ) 
			{
				DWORD newSize;
				unsigned char *buffer = new unsigned char[size];
				SFileReadFile( fh, buffer, (DWORD)size, &newSize, 0);
				unsigned char *p = buffer, *end = buffer + size;

				while (p < end) 
				{ // if p = end here, no need to go into next loop !
					unsigned char *q=p;
					do 
					{
						if (*q=='\r' || *q=='\n') // carriage return or new line
							break;
					} 
					while (q++<=end);

					int length = q - p;
					char cTemp[MAX_PATH] = {0};
					memcpy(cTemp, p, length);
					QString line(cTemp);
					if (line.length()==0) 
						break;

					p = q;
					if (*p == '\r')
						p++;
					if (*p == '\n')
						p++;
					
					//line = line.Trim(); // delete \r\n
					//if (isPartial)
					//{
					//	if (line.Lower().StartsWith(wxT("base\\"))) // strip "base\\"
					//		line = line.Mid(5);
					//	else if (line.StartsWith(langName)) // strip "enus\\"
					//		line = line.Mid(5);
					//	else
					//		continue;
					//}

					if (filterfunc(line.toStdString())) 
					{
						line = line.toLower();

						// This is just to help cleanup Duplicates
						// Ideally I should tokenise the string and clean it up automatically
						//FileTreeItem tmp;

						//tmp.fileName = line;
						//line.MakeLower();
						//line[0] = toupper(line.GetChar(0));
						//int ret = line.Find('\\');
						//if (ret>-1)
						//	line[ret+1] = toupper(line.GetChar(ret+1));

						//tmp.displayName = line;
						//tmp.color = col;
						//dest.insert(tmp);

						CFileTreeItem tmp;
						tmp.fn = line.toStdString();
						tmp.col = col;
						dest.insert(tmp);
					}
				}

				//wxDELETEA(buffer);
				//p = NULL;
				//end = NULL;

				delete buffer;
				buffer = NULL;
				p = NULL;
				end = NULL;
			}

			SFileCloseFile( fh );
		}
	}
}

void GetFileLists(std::set<CFileTreeItem> &dest, bool filterfunc(std::string))
{
	for(ArchiveSet::iterator i=gOpenArchives.begin(); i!=gOpenArchives.end();++i)
	{
		mpq_archive &mpq_a = **i;
		int fileno = libmpq_file_number(&mpq_a, "(listfile)");

		if(fileno != LIBMPQ_EFILE_NOT_FOUND) 
		{
			// Found!
			size_t size = libmpq_file_info(&mpq_a, LIBMPQ_FILE_UNCOMPRESSED_SIZE, fileno);
			int retVal = libmpq_file_info(&mpq_a, LIBMPQ_FILE_COMPRESSION_TYPE, fileno);
			// If retVal is 512, its compressed, if its 0 then its uncompressed

			QString temp = mpq_a.filename;
			temp = temp.toLower();
			int col = 0;

			if (temp.contains("patch.mpq"))
				col = 1;
			else if (temp.contains("patch-2.mpq"))
				col = 2;

			if (temp.contains("expansion"))
				col = 3;

			// TODO: Add handling for uncompressed files.
			// err.. it seems uncompressed files no longer cause crashes?
			if (size > 0 /*&& retVal != 0*/) 
			{
				unsigned char *buffer = new unsigned char[size];
				libmpq_file_getdata(&mpq_a, fileno, buffer);
				unsigned char *p = buffer, *end = buffer + size;

				while (p <= end) 
				{
					unsigned char *q=p;
					do 
					{
						if (*q==13) 
							break;
					} while (q++<=end);

					
					int length = q - p;
					char cTemp[MAX_PATH] = {0};
					memcpy(cTemp, p, length);
					QString line(cTemp);
					if (line.length()==0) 
						break;
					//p += line.length();
					p = q + 2;
					//line.erase(line.length()-2, 2); // delete \r\n

					if (filterfunc(line.toStdString())) 
					{

						// This is just to help cleanup Duplicates
						// Ideally I should tokenise the string and clean it up automatically
						line = line.toLower();
						//line[0] = char(line.at(0).toAscii() - 32);
						//int ret = line.count('\\', Qt::CaseInsensitive);
						//	line[ret+1] = char(line.at(ret + 1).toAscii() - 32);

						CFileTreeItem tmp;
						tmp.fn = line.toStdString();
						tmp.col = col;
						dest.insert(tmp);
					}
				}

				delete buffer;
				buffer = NULL;
				p = NULL;
				end = NULL;
			}
		}
	}
}

//------------------------------------
//------------CMPQArchive1-------------
//------------------------------------
CMPQFile::CMPQFile(const char* filename):
eof(false),
buffer(0),
pointer(0),
size(0)
{
	for(ArchiveSet::iterator i=gOpenArchives.begin(); i!=gOpenArchives.end(); ++i)
	{
		mpq_archive &mpq_a = **i;
		int fileno = libmpq_file_number(&mpq_a, filename);
		if(fileno == LIBMPQ_EFILE_NOT_FOUND)
			continue;

		// Found!
		size = libmpq_file_info(&mpq_a, LIBMPQ_FILE_UNCOMPRESSED_SIZE, fileno);

		// HACK: in patch.mpq some files don't want to open and give 1 for filesize
		if (size<=1) 
		{
			eof = true;
			buffer = 0;
			return;
		}

		buffer = new unsigned char[size];
		libmpq_file_getdata(&mpq_a, fileno, buffer);
		return;
	}

	//for(ArchiveMap::iterator i = gOpenArchivesMap.begin(); i != gOpenArchivesMap.end(); ++i)
	//{
	//	HANDLE mpq_a = i->second;

	//	HANDLE fh;
	//	if( !SFileOpenFileEx( mpq_a, filename, SFILE_OPEN_FROM_MPQ, &fh ) )
	//		continue;

	//	// Found!
	//	DWORD filesize = SFileGetFileSize( fh, NULL);
	//	size = filesize;

	//	// HACK: in patch.mpq some files don't want to open and give 1 for filesize
	//	if (size<=1) 
	//	{
	//		eof = true;
	//		buffer = 0;
	//		return;
	//	}

	//	DWORD newSize;
	//	buffer = new unsigned char[size];
	//	SFileReadFile( fh, buffer, (DWORD)size, &newSize, 0);
	//	SFileCloseFile( fh );

	//	return;
	//}

	eof = true;
	buffer = 0;
}

CMPQFile::~CMPQFile()
{
	close();
}

bool CMPQFile::exists(const char* filename)
{
	//if( useLocalFiles )
	//{
	//	wxString fn = gamePath;
	//	fn.Append(filename);
	//	if (wxFile::Exists(fn.fn_str()))
	//		return true;
	//}

	for(ArchiveSet::iterator i=gOpenArchives.begin(); i!=gOpenArchives.end();++i)
	{
		mpq_archive &mpq_a = **i;
		int fileno = libmpq_file_number(&mpq_a, filename);
		if (fileno != LIBMPQ_EFILE_NOT_FOUND) 
			return true;
	}

	return false;
}

size_t CMPQFile::read(void* dest, size_t bytes)
{
	if (eof) 
		return 0;

	size_t rpos = pointer + bytes;
	if (rpos > size) 
	{
		bytes = size - pointer;
		eof = true;
	}

	memcpy(dest, &(buffer[pointer]), bytes);

	pointer = rpos;

	return bytes;
}

bool CMPQFile::isEof()
{
	return eof;
}

void CMPQFile::seek(int offset)
{
	pointer = offset;
	eof = (pointer >= size);
}

void CMPQFile::seekRelative(int offset)
{
	pointer += offset;
	eof = (pointer >= size);
}

void CMPQFile::close()
{
	SAFE_DELETE_ARRAY(buffer);
	eof = true;
}

size_t CMPQFile::getSize()
{
	return size;
}

int CMPQFile::getSize(const char* filename)
{
	//if( useLocalFiles ) 
	//{
	//	wxString fn = gamePath;
	//	fn.Append(filename);
	//	if (wxFile::Exists(fn.fn_str())) 
	//	{
	//		wxFile file(fn);
	//		return file.Length();
	//	}
	//}

	for(ArchiveSet::iterator i=gOpenArchives.begin(); i!=gOpenArchives.end();++i)
	{
		mpq_archive &mpq_a = **i;
		int fileno = libmpq_file_number(&mpq_a, filename);
		if (fileno != LIBMPQ_EFILE_NOT_FOUND)
			return libmpq_file_info(&mpq_a, LIBMPQ_FILE_UNCOMPRESSED_SIZE, fileno);
	}

	return 0;
}

size_t CMPQFile::getPos()
{
	return pointer;
}

unsigned char* CMPQFile::getBuffer()
{
	return buffer;
}

unsigned char* CMPQFile::getPointer()
{
	return buffer + pointer;
}