

#include "OgreException.h"

namespace Ogre
{
	CException::CException(int num, const CString& desc, const CString& src) :
		line( 0 ),
		number( num ),
		description( desc ),
		source( src )
	{
	}

	CException::CException(int num, const CString& desc, const CString& src, 
					 const char* typ, const char* fil, long lin) :
		line( lin ),
		number( num ),
		typeName(typ),
		description( desc ),
		source( src ),
		file( fil )
	{
		//// Log this error, mask it from debug though since it may be caught and ignored
		//if(LogManager::getSingletonPtr())
		//{
		//	LogManager::getSingleton().logMessage(
		//		this->getFullDescription(), 
		//		LML_CRITICAL, true);
		//}
	}

	CException::CException(const CException& rhs)
		: line( rhs.line ), 
		number( rhs.number ), 
		typeName( rhs.typeName ), 
		description( rhs.description ), 
		source( rhs.source ), 
		file( rhs.file )
	{
	}

	void CException::operator = ( const CException& rhs )
	{
		description = rhs.description;
		number = rhs.number;
		source = rhs.source;
		file = rhs.file;
		line = rhs.line;
		typeName = rhs.typeName;
	}

	const CString& CException::getFullDescription(void) const
	{
		//if (fullDesc.empty())
		//{

		//	StringUtil::StrStreamType desc;

		//	desc <<  "OGRE EXCEPTION(" << number << ":" << typeName << "): "
		//		<< description 
		//		<< " in " << source;

		//	if( line > 0 )
		//	{
		//		desc << " at " << file << " (line " << line << ")";
		//	}

		//	fullDesc = desc.str();
		//}

		return fullDesc;
	}

	int CException::getNumber(void) const throw()
	{
		return number;
	}
}