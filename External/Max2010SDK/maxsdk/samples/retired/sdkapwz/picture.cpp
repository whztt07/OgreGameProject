// Machine generated IDispatch wrapper class(es) created by Microsoft Visual C++

// NOTE: Do not modify the contents of this file.  If this class is regenerated by
//  Microsoft Visual C++, your modifications will be overwritten.


#include "stdafx.h"
#include "picture.h"


/////////////////////////////////////////////////////////////////////////////
// CPicture properties

LONG_PTR CPicture::GetHandle()
{
	LONG_PTR result;
	GetProperty(0x0, VT_INT_PTR, (void*)&result);
	return result;
}

LONG_PTR CPicture::GetHPal()
{
	LONG_PTR result;
	GetProperty(0x2, VT_INT_PTR, (void*)&result);
	return result;
}

void CPicture::SetHPal(LONG_PTR propVal)
{
	SetProperty(0x2, VT_INT_PTR, propVal);
}

short CPicture::GetType()
{
	short result;
	GetProperty(0x3, VT_I2, (void*)&result);
	return result;
}

long CPicture::GetWidth()
{
	long result;
	GetProperty(0x4, VT_I4, (void*)&result);
	return result;
}

long CPicture::GetHeight()
{
	long result;
	GetProperty(0x5, VT_I4, (void*)&result);
	return result;
}

/////////////////////////////////////////////////////////////////////////////
// CPicture operations
