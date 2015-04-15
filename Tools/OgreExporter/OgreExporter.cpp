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

#include "OgreExporter.h"
#include "MaxInterface.h"
#include "MdxCandidate.h"

#define OgreExporter_CLASS_ID	Class_ID(0x7d427e65, 0x5333cd39)

extern CMaxInterface* GetMaxIP();
#define EXP_MDX_FILE "C:\\MyExptmp.mdx"

class OgreExporter : public UtilityObj 
{
public:
		
	//Constructor/Destructor
	OgreExporter();
	virtual ~OgreExporter();

	virtual void DeleteThis() { }		
	
	virtual void BeginEditParams(Interface *ip,IUtil *iu);
	virtual void EndEditParams(Interface *ip,IUtil *iu);

	virtual void Init(HWND hWnd);
	virtual void Destroy(HWND hWnd);

	void Export();
	
	// Singleton access
	static OgreExporter* GetInstance() { 
		static OgreExporter theOgreExporter;
		return &theOgreExporter; 
	}

private:

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND			hPanel;
	IUtil			*iu;
	Interface		*ip;
};


class OgreExporterClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 	{ return OgreExporter::GetInstance(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return UTILITY_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return OgreExporter_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("OgreExporter"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* GetOgreExporterDesc() { 
	static OgreExporterClassDesc OgreExporterDesc;
	return &OgreExporterDesc; 
}




//--- OgreExporter -------------------------------------------------------
OgreExporter::OgreExporter()
{
	iu = NULL;
	ip = NULL;	
	hPanel = NULL;
}

OgreExporter::~OgreExporter()
{

}

void OgreExporter::BeginEditParams(Interface* ip,IUtil* iu) 
{
	this->iu = iu;
	this->ip = ip;
	hPanel = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_PANEL),
		DlgProc,
		GetString(IDS_PARAMS),
		0);

	GetMaxIP()->Create(0, this->ip);
}
	
void OgreExporter::EndEditParams(Interface* ip,IUtil* iu) 
{
	this->iu = NULL;
	this->ip = NULL;
	ip->DeleteRollupPage(hPanel);
	hPanel = NULL;
}

void OgreExporter::Init(HWND hWnd)
{

}

void OgreExporter::Destroy(HWND hWnd)
{

}

extern CMdxCandidate *g_pMdxCandidate;
void OgreExporter::Export()
{
	// ...
	INode* pNode = GetMaxIP()->GetSelectedNode();
	if( pNode )
	{
		if( !GetMaxIP()->IsBone( pNode ) &&
			!GetMaxIP()->IsBipedBone( pNode ) )
		{
			MessageBox( hPanel, "Cannot select none bone node", "failed", MB_OK );
			return;
		}
	}

	// ...
	INode* pSelNode = NULL;
	if( GetMaxIP()->GetSelectedNode() )
	{
		pSelNode = GetMaxIP()->GetSelectedNode();
	}
	else
	{
		pSelNode = GetMaxIP()->GetRootNode();
	}

	g_pMdxCandidate = new CMdxCandidate;
	if (g_pMdxCandidate)
	{
		g_pMdxCandidate->CreateMdx(pSelNode);

		g_pMdxCandidate->SaveMdx(EXP_MDX_FILE);
	}

	SAFE_DELETE(g_pMdxCandidate);
}

INT_PTR CALLBACK OgreExporter::DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
		case WM_INITDIALOG:
			OgreExporter::GetInstance()->Init(hWnd);
			break;

		case WM_DESTROY:
			OgreExporter::GetInstance()->Destroy(hWnd);
			break;

		case WM_COMMAND:
			{
				switch(LOWORD(wParam))
				{
				case IDC_Button_Export:
					{
						OgreExporter::GetInstance()->Export();
					}
					break;
				}
			}
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			OgreExporter::GetInstance()->ip->RollupMouseMessage(hWnd,msg,wParam,lParam); 
			break;

		default:
			return 0;
	}
	return 1;
}
