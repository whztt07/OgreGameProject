#include "dbcfile.h"
#include "mpq.h"
#include "mpq_libmpq.h"
#include <QMessageBox>

DBCFile::DBCFile(const std::string &filename) : filename(filename)
{
	data = NULL;
}

bool DBCFile::open()
{
	CMPQFile f(filename.c_str());

	// Need some error checking, otherwise an unhandled exception error occurs
	// if people screw with the data path.
	if (f.isEof() == true)
		return false;

	unsigned char header[4];
	unsigned int na,nb,es,ss;

	f.read(header,4); // File Header

	if (header[0]!='W' || header[1]!='D' || header[2]!='B' || header[3] != 'C') 
	{

		f.close();
		data = NULL;
		QMessageBox::Icon icon = QMessageBox::NoIcon;
		QMessageBox(icon, QString("MPQArchiveOpen"), 
			QString("Critical Error: An error occured while trying to read the DBCFile %s.").arg(filename.c_str()) );
		return false;
	}

	//assert(header[0]=='W' && header[1]=='D' && header[2]=='B' && header[3] == 'C');

	f.read(&na,4); // Number of records
	f.read(&nb,4); // Number of fields
	f.read(&es,4); // Size of a record
	f.read(&ss,4); // String size
	
	recordSize = es;
	recordCount = na;
	fieldCount = nb;
	stringSize = ss;
	//assert(fieldCount*4 == recordSize);
	assert(fieldCount*4 >= recordSize);

	data = new unsigned char[recordSize*recordCount+stringSize];
	stringTable = data + recordSize*recordCount;
	f.read(data,recordSize*recordCount+stringSize);
	f.close();
	return true;
}

DBCFile::~DBCFile()
{
	delete [] data;
}

DBCFile::Record DBCFile::getRecord(size_t id)
{
	assert(data);
	return Record(*this, data + id*recordSize);
}

DBCFile::Iterator DBCFile::begin()
{
	assert(data);
	return Iterator(*this, data);
}
DBCFile::Iterator DBCFile::end()
{
	assert(data);
	return Iterator(*this, stringTable);
}
