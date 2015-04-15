

#ifndef __OgreException_H_
#define __OgreException_H_

// Precompiler options
#include "OgrePrerequisites.h"
#include "OgreString.h"
#include <exception>

namespace Ogre 
{
	class OgreExport CException : public std::exception
    {
	public:
		enum ExceptionCodes
		{
			ERR_CANNOT_WRITE_TO_FILE,
			ERR_INVALID_STATE,
			ERR_INVALIDPARAMS,
			ERR_RENDERINGAPI_ERROR,
			ERR_DUPLICATE_ITEM,
			ERR_ITEM_NOT_FOUND,
			ERR_FILE_NOT_FOUND,
			ERR_INTERNAL_ERROR,
			ERR_RT_ASSERTION_FAILED, 
			ERR_NOT_IMPLEMENTED
		};
		CException( int number, const CString& description, const CString& source );
		CException( int number, const CString& description, const CString& source, const char* type, const char* file, long line );
		CException(const CException& rhs);
		~CException() throw() {}

		void operator = (const CException& rhs);
		virtual const CString& getFullDescription(void) const;
		virtual int getNumber(void) const throw();
		virtual const CString &getSource() const { return source; }
		virtual const CString &getFile() const { return file; }
		virtual long getLine() const { return line; }
		virtual const CString &getDescription(void) const { return description; }
		const char* what() const throw() { return getFullDescription().c_str(); }
    protected:
        long line;
        int number;
		CString typeName;
        CString description;
        CString source;
        CString file;
		mutable CString fullDesc;
    };

	template <int num> struct ExceptionCodeType
	{
		enum { number = num };
	};

	class OgreExport UnimplementedException : public CException 
	{
	public:
		UnimplementedException(int number, const CString& description, const CString& source, const char* file, long line)
			: CException(number, description, source, "UnimplementedException", file, line) {}
	};
	class OgreExport FileNotFoundException : public CException
	{
	public:
		FileNotFoundException(int number, const CString& description, const CString& source, const char* file, long line)
			: CException(number, description, source, "FileNotFoundException", file, line) {}
	};
	class OgreExport IOException : public CException
	{
	public:
		IOException(int number, const CString& description, const CString& source, const char* file, long line)
			: CException(number, description, source, "IOException", file, line) {}
	};
	class OgreExport InvalidStateException : public CException
	{
	public:
		InvalidStateException(int number, const CString& description, const CString& source, const char* file, long line)
			: CException(number, description, source, "InvalidStateException", file, line) {}
	};
	class OgreExport InvalidParametersException : public CException
	{
	public:
		InvalidParametersException(int number, const CString& description, const CString& source, const char* file, long line)
			: CException(number, description, source, "InvalidParametersException", file, line) {}
	};
	class OgreExport ItemIdentityException : public CException
	{
	public:
		ItemIdentityException(int number, const CString& description, const CString& source, const char* file, long line)
			: CException(number, description, source, "ItemIdentityException", file, line) {}
	};
	class OgreExport InternalErrorException : public CException
	{
	public:
		InternalErrorException(int number, const CString& description, const CString& source, const char* file, long line)
			: CException(number, description, source, "InternalErrorException", file, line) {}
	};
	class OgreExport RenderingAPIException : public CException
	{
	public:
		RenderingAPIException(int number, const CString& description, const CString& source, const char* file, long line)
			: CException(number, description, source, "RenderingAPIException", file, line) {}
	};
	class OgreExport RuntimeAssertionException : public CException
	{
	public:
		RuntimeAssertionException(int number, const CString& description, const CString& source, const char* file, long line)
			: CException(number, description, source, "RuntimeAssertionException", file, line) {}
	};

	class ExceptionFactory
	{
	private:
		ExceptionFactory() {}
	public:
		static UnimplementedException create(
			ExceptionCodeType<CException::ERR_NOT_IMPLEMENTED> code, 
			const CString& desc, 
			const CString& src, const char* file, long line)
		{
			return UnimplementedException(code.number, desc, src, file, line);
		}
		static FileNotFoundException create(
			ExceptionCodeType<CException::ERR_FILE_NOT_FOUND> code, 
			const CString& desc, 
			const CString& src, const char* file, long line)
		{
			return FileNotFoundException(code.number, desc, src, file, line);
		}
		static IOException create(
			ExceptionCodeType<CException::ERR_CANNOT_WRITE_TO_FILE> code, 
			const CString& desc, 
			const CString& src, const char* file, long line)
		{
			return IOException(code.number, desc, src, file, line);
		}
		static InvalidStateException create(
			ExceptionCodeType<CException::ERR_INVALID_STATE> code, 
			const CString& desc, 
			const CString& src, const char* file, long line)
		{
			return InvalidStateException(code.number, desc, src, file, line);
		}
		static InvalidParametersException create(
			ExceptionCodeType<CException::ERR_INVALIDPARAMS> code, 
			const CString& desc, 
			const CString& src, const char* file, long line)
		{
			return InvalidParametersException(code.number, desc, src, file, line);
		}
		static ItemIdentityException create(
			ExceptionCodeType<CException::ERR_ITEM_NOT_FOUND> code, 
			const CString& desc, 
			const CString& src, const char* file, long line)
		{
			return ItemIdentityException(code.number, desc, src, file, line);
		}
		static ItemIdentityException create(
			ExceptionCodeType<CException::ERR_DUPLICATE_ITEM> code, 
			const CString& desc, 
			const CString& src, const char* file, long line)
		{
			return ItemIdentityException(code.number, desc, src, file, line);
		}
		static InternalErrorException create(
			ExceptionCodeType<CException::ERR_INTERNAL_ERROR> code, 
			const CString& desc, 
			const CString& src, const char* file, long line)
		{
			return InternalErrorException(code.number, desc, src, file, line);
		}
		static RenderingAPIException create(
			ExceptionCodeType<CException::ERR_RENDERINGAPI_ERROR> code, 
			const CString& desc, 
			const CString& src, const char* file, long line)
		{
			return RenderingAPIException(code.number, desc, src, file, line);
		}
		static RuntimeAssertionException create(
			ExceptionCodeType<CException::ERR_RT_ASSERTION_FAILED> code, 
			const CString& desc, 
			const CString& src, const char* file, long line)
		{
			return RuntimeAssertionException(code.number, desc, src, file, line);
		}
	};
	
#ifndef OGRE_EXCEPT
#define OGRE_EXCEPT(num, desc, src) throw Ogre::ExceptionFactory::create( \
	Ogre::ExceptionCodeType<num>(), desc, src, __FILE__, __LINE__ );
#endif
}

#endif
