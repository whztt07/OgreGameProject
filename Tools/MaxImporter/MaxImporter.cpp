//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Appwizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "MaxImporter.h"
#include "MaxInterface.h"
#include "MdxCandidate.h"

#define MaxImporter_CLASS_ID	Class_ID(0xb46b586c, 0x775a9e36)

extern CMdxCandidate *g_pMdxCandidate;

class MaxImporter : public SceneImport {
	public:

		static HWND hParams;
		
		int				ExtCount();					// Number of extensions supported
		const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
		const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
		const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
		const TCHAR *	AuthorName();				// ASCII Author name
		const TCHAR *	CopyrightMessage();			// ASCII Copyright message
		const TCHAR *	OtherMessage1();			// Other message #1
		const TCHAR *	OtherMessage2();			// Other message #2
		unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
		void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box
		int				DoImport(const TCHAR *name,ImpInterface *i,Interface *gi, BOOL suppressPrompts=FALSE);	// Import file
		
		//Constructor/Destructor
		MaxImporter();
		~MaxImporter();		

};



class MaxImporterClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 		{ return new MaxImporter(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return SCENE_IMPORT_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return MaxImporter_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("MaxImporter"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* GetMaxImporterDesc() { 
	static MaxImporterClassDesc MaxImporterDesc;
	return &MaxImporterDesc; 
}




INT_PTR CALLBACK MaxImporterOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	static MaxImporter *imp = NULL;

	switch(message) {
		case WM_INITDIALOG:
			imp = (MaxImporter *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return 1;
	}
	return 0;
}


//--- MaxImporter -------------------------------------------------------
MaxImporter::MaxImporter()
{

}

MaxImporter::~MaxImporter() 
{

}

int MaxImporter::ExtCount()
{
	#pragma message(TODO("Returns the number of file name extensions supported by the plug-in."))
	return 1;
}

const TCHAR *MaxImporter::Ext(int n)
{		
	#pragma message(TODO("Return the 'i-th' file name extension (i.e. \"3DS\")."))
	//return _T("");
	return _T("mdx2");
}

const TCHAR *MaxImporter::LongDesc()
{
	#pragma message(TODO("Return long ASCII description (i.e. \"Targa 2.0 Image File\")"))
	//return _T("");
	return _T("Ogre Mdx2 File");
}
	
const TCHAR *MaxImporter::ShortDesc() 
{			
	#pragma message(TODO("Return short ASCII description (i.e. \"Targa\")"))
	return _T("Ogre Mdx2 File");
}

const TCHAR *MaxImporter::AuthorName()
{			
	#pragma message(TODO("Return ASCII Author name"))
	return _T("");
}

const TCHAR *MaxImporter::CopyrightMessage() 
{	
	#pragma message(TODO("Return ASCII Copyright message"))
	return _T("");
}

const TCHAR *MaxImporter::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *MaxImporter::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int MaxImporter::Version()
{				
	#pragma message(TODO("Return Version number * 100 (i.e. v3.01 = 301)"))
	return 100;
}

void MaxImporter::ShowAbout(HWND hWnd)
{			
	// Optional
}

int MaxImporter::DoImport(const TCHAR *filename,ImpInterface *i,
						Interface *gi, BOOL suppressPrompts)
{
	#pragma message(TODO("Implement the actual file import here and"))	

	GetMaxIP()->Create(0, gi);

	suppressPrompts = true;
	if(!suppressPrompts)
		DialogBoxParam(hInstance, 
				MAKEINTRESOURCE(IDD_PANEL), 
				GetActiveWindow(), 
				MaxImporterOptionsDlgProc, (LPARAM)this);

	g_pMdxCandidate = new CMdxCandidate;
	if (g_pMdxCandidate)
	{
		g_pMdxCandidate->ImportMdx((char*)filename);
	}

	if(g_pMdxCandidate)
	{
		delete g_pMdxCandidate;
		g_pMdxCandidate = NULL;
	}

	#pragma message(TODO("return TRUE If the file is imported properly"))
	return FALSE;
}
	
