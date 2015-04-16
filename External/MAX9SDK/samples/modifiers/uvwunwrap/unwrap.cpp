/********************************************************************** *<
FILE: unwrap.cpp

DESCRIPTION: A UVW map modifier unwraps the UVWs onto the image

HISTORY: 12/31/96
CREATED BY: Rolf Berteig
UPDATED Sept. 16, 1998 Peter Watje




*>	Copyright (c) 1998, All Rights Reserved.
**********************************************************************/

/**

5.1.01 fixes a stitch bug on edges that align perfectly
5.1.02 expands the bitmap and filter id field drop downs
changes the material drop down to load only the first bitmap of each sub material
changes th mat filter id to automatically switch and load the bitmap appropriate for that id
5.1.03 fixes a crash bug when no material was assigned to the mesh caused by 5.1.02
5.1.04 fixes a stitch bug stiching when only one edge was selected.
for instance create 1x1x1 box flatten it then try to sticth one edge
Makes the unwrap matid filter drop down wider
5.1.05 Adds a switch to turn on/off the automatic background reload for matids
5.1.06 adds a better relax method
5.1.07 fixes a bug where it is not getting the textures from a non standard materials right
**/

/***************************************************************


absolute/relative typein
add maxscript access
hook them up
add defaults
add to  global defaults


Fix gizmo so the move/rotate/scales are percentage hits



Pivot and Gizmo snaps



Sometimes and open does not display right, especially after a break edge
Sometimes the edge display is wrong will show as selected or maybe not as open
Sometimes hidden selected edges will show up in face mode
Copy paste rotate is brokenthe 3rd time you paste with a trimesh

Survive a topochange option 
Symetttry Tool 
Fit to box command
Relax

Something is broken when packing 1 dimensional objects



fit to square

Texture Map Drop Down not scriptable




Maxscript access to U V W  (move()) is really slow on update.  Don't know if you can fix this?

Macro record actions


fix the spacing algorithm so we can define a pixel distance
put in % percent threshold for when to collapse a cluster

put in render size or some better way to compute one

look at planarmap no scale speed






*****************************************************************/




#include "unwrap.h"
#include "buildver.h"
#include "modstack.h"
#include "IDxMaterial.h"

#include "3dsmaxport.h"

//these are just debug globals so I can stuff data into to draw
#ifdef DEBUGMODE
//just some pos tabs to draw bounding volumes
Tab<float> jointClusterBoxX;
Tab<float> jointClusterBoxY;
Tab<float> jointClusterBoxW;
Tab<float> jointClusterBoxH;


float hitClusterBoxX,hitClusterBoxY,hitClusterBoxW,hitClusterBoxH;

int currentCluster, subCluster;

//used to turn off the regular display and only show debug display
BOOL drawOnlyBounds;
#endif

PeltPointToPointMode* UnwrapMod::peltPointToPointMode   = NULL;

class MyEnumProc : public DependentEnumProc 
{
public :
	virtual int proc(ReferenceMaker *rmaker); 
	INodeTab Nodes;              
};

int MyEnumProc::proc(ReferenceMaker *rmaker) 
{ 
	if (rmaker->SuperClassID()==BASENODE_CLASS_ID)    
	{
		Nodes.Append(1, (INode **)&rmaker);  
		return DEP_ENUM_SKIP;
	}

	return DEP_ENUM_CONTINUE;
}


static void PreSave (void *param, NotifyInfo *info)
{
	UnwrapMod *mod = (UnwrapMod*) param;
	if (mod->CurrentMap == 0)
		mod->ShowCheckerMaterial(FALSE);
}

static void PostSave (void *param, NotifyInfo *info)
{
	UnwrapMod *mod = (UnwrapMod*) param;
	if ((mod->CurrentMap == 0) && (mod->checkerWasShowing))
		mod->ShowCheckerMaterial(TRUE);
}





static GenSubObjType SOT_SelFace(19);
static GenSubObjType SOT_SelVerts(1);
static GenSubObjType SOT_SelEdges(2);
static GenSubObjType SOT_SelGizmo(14);

void UnwrapMatrixFromNormal(Point3& normal, Matrix3& mat);









HCURSOR PaintSelectMode::GetXFormCur() 
{
	return mod->zoomRegionCur;
}


HCURSOR ZoomRegMode::GetXFormCur() 
{
	return mod->zoomRegionCur;
}


HCURSOR ZoomMode::GetXFormCur() 
{
	return mod->zoomCur;
}


HCURSOR PanMode::GetXFormCur() 
{
	return mod->panCur;
}


HCURSOR RotateMode::GetXFormCur() 
{
	return mod->rotCur;
}


HCURSOR WeldMode::GetXFormCur() 
{
	return mod->weldCur;
}

HCURSOR MoveMode::GetXFormCur()
{		
	if (mod->move==1)
		return mod->moveXCur;
	else if (mod->move==2) return mod->moveYCur;
	return mod->moveCur;
}

HCURSOR ScaleMode::GetXFormCur()
{		
	if (mod->scale==1)
		return mod->scaleXCur;
	else if (mod->scale==2) return mod->scaleYCur;
	return mod->scaleCur;

}

HCURSOR FreeFormMode::GetXFormCur()	
{
	if (mod->freeFormSubMode == ID_TOOL_MOVE)
		return mod->moveCur;
	else if (mod->freeFormSubMode == ID_TOOL_SCALE)
		return mod->scaleCur;
	else if (mod->freeFormSubMode == ID_TOOL_ROTATE)
		return mod->rotCur;
	else if (mod->freeFormSubMode == ID_TOOL_MOVEPIVOT)
		return LoadCursor(NULL, IDC_SIZEALL);

	else return mod->selCur;
}




class UnwrapRightMenu : public RightClickMenu {
private:
	UnwrapMod *ep;
public:
	void Init(RightClickMenuManager* manager, HWND hWnd, IPoint2 m);
	void Selected(UINT id);
	void SetMod(UnwrapMod *ep) { this->ep = ep; }
};



void UnwrapRightMenu::Init(RightClickMenuManager* manager, HWND hWnd, IPoint2 m) {

	int flags1,flags2,flags3,flags4,flags5,flags6;

	flags1 = flags2 = flags3 = flags4 = flags5 = flags6 = MF_STRING | MF_UNCHECKED;



	manager->AddMenu(this, MF_SEPARATOR, 0, NULL);
	manager->AddMenu(this, flags1, 0, GetString(IDS_PW_FACEMODE));
	manager->AddMenu(this, flags6, 5, GetString(IDS_PW_APPLYPLANAR));

}


void UnwrapRightMenu::Selected(UINT id) {
	//  Add Cross Section
	if (id ==  0)
		ep->ip->SetSubObjectLevel(1);	
	else if (id == 5)
	{
		SendMessage(ep->hParams,WM_COMMAND,IDC_UNWRAP_APPLY,0);

	}

}

//UnwrapRightMenu rMenu;




class UVWUnwrapDeleteEvent : public EventUser {
public:
	UnwrapMod *m;

	void Notify() {if (m) 
	{
		m->DeleteSelected();
		m->InvalidateView();

	}
	}
	void SetEditMeshMod(UnwrapMod *im) {m=im;}
};

UVWUnwrapDeleteEvent delEvent;


LocalModData *MeshTopoData::Clone() {
	MeshTopoData *d = new MeshTopoData;
	d->mesh = NULL;
	d->mnMesh = NULL;
	d->patch = NULL;
	return d;
}

MeshTopoData::MeshTopoData(Mesh &mesh) {
	this->mesh = new Mesh(mesh);
	this->patch = NULL;
	this->mnMesh = NULL;
}

MeshTopoData::MeshTopoData(PatchMesh &patch) {
	this->mesh = NULL;
	this->mnMesh = NULL;
	this->patch = new PatchMesh(patch);
}

MeshTopoData::MeshTopoData(MNMesh &mesh) {
	this->mesh = NULL;
	this->mnMesh =new MNMesh(mesh);
	this->patch =  NULL;
}

void MeshTopoData::SetCache(Mesh &mesh)
{
	FreeCache();
	this->mesh = new Mesh(mesh);
}

void MeshTopoData::SetCache(MNMesh &mesh)
{
	FreeCache();
	this->mnMesh = new MNMesh(mesh);
}

void MeshTopoData::SetCache(PatchMesh &patch)
{
	FreeCache();
	this->patch = new PatchMesh(patch);
}

void MeshTopoData::FreeCache() {
	if (mesh) delete mesh;
	mesh = NULL;
	if (patch) delete patch;
	patch = NULL;
	if (mnMesh) delete mnMesh;
	mnMesh = NULL;

}

void MeshTopoData::SetFaceSel(BitArray &set, UnwrapMod *imod, TimeValue t) {
	UnwrapMod *mod = (UnwrapMod *) imod;
	faceSel = set;
	if (mesh) mesh->faceSel = set;
	if (mnMesh) mnMesh->FaceSelect(set);
	if (patch) patch->patchSel = set;
}




BOOL			UnwrapMod::executedStartUIScript = FALSE;

HWND            UnwrapMod::hOptionshWnd = NULL;
HWND            UnwrapMod::hSelRollup = NULL;
HWND            UnwrapMod::hParams = NULL;
HWND            UnwrapMod::hWnd = NULL;
HWND            UnwrapMod::hView = NULL;
IObjParam      *UnwrapMod::ip = NULL;
ICustToolbar   *UnwrapMod::iTool = NULL;
ICustToolbar   *UnwrapMod::iVertex = NULL;
ICustToolbar   *UnwrapMod::iView = NULL;
ICustToolbar   *UnwrapMod::iOption = NULL;
ICustToolbar   *UnwrapMod::iFilter = NULL;
ICustButton    *UnwrapMod::iMove = NULL;
ICustButton    *UnwrapMod::iRot = NULL;
ICustButton    *UnwrapMod::iScale = NULL;
ICustButton    *UnwrapMod::iFalloff = NULL;
ICustButton    *UnwrapMod::iFalloffSpace = NULL;
ICustButton    *UnwrapMod::iFreeForm = NULL;


ICustButton    *UnwrapMod::iMirror = NULL;
ICustButton    *UnwrapMod::iWeld = NULL;
ICustButton    *UnwrapMod::iPan = NULL;
ICustButton    *UnwrapMod::iZoom = NULL;
ICustButton    *UnwrapMod::iUpdate = NULL;
ISpinnerControl *UnwrapMod::iU = NULL;
ISpinnerControl *UnwrapMod::iV = NULL;
ISpinnerControl *UnwrapMod::iW = NULL;
ISpinnerControl *UnwrapMod::iStr = NULL;
ISpinnerControl *UnwrapMod::iMapID = NULL;
ISpinnerControl *UnwrapMod::iPlanarThreshold = NULL;
ISpinnerControl *UnwrapMod::iMatID = NULL;
ISpinnerControl *UnwrapMod::iSG = NULL;

ICustToolbar   *UnwrapMod::iUVWSpinBar = NULL;
ICustButton    *UnwrapMod::iUVWSpinAbsoluteButton = NULL;

MouseManager    UnwrapMod::mouseMan;
CopyPasteBuffer UnwrapMod::copyPasteBuffer;
IOffScreenBuf  *UnwrapMod::iBuf = NULL;
int             UnwrapMod::mode = ID_FREEFORMMODE;
int             UnwrapMod::oldMode = ID_FREEFORMMODE;

MoveMode       *UnwrapMod::moveMode = NULL;
RotateMode     *UnwrapMod::rotMode = NULL;
ScaleMode      *UnwrapMod::scaleMode = NULL;
PanMode        *UnwrapMod::panMode = NULL;
ZoomMode       *UnwrapMod::zoomMode = NULL;
ZoomRegMode    *UnwrapMod::zoomRegMode = NULL;
WeldMode       *UnwrapMod::weldMode = NULL;
//PELT
PeltStraightenMode       *UnwrapMod::peltStraightenMode = NULL;
RightMouseMode *UnwrapMod::rightMode = NULL;
MiddleMouseMode *UnwrapMod::middleMode = NULL;
FreeFormMode   *UnwrapMod::freeFormMode = NULL;
SketchMode		*UnwrapMod::sketchMode = NULL;

BOOL            UnwrapMod::viewValid = FALSE;
BOOL            UnwrapMod::typeInsValid = FALSE;
UnwrapMod      *UnwrapMod::editMod = NULL;
ICustButton    *UnwrapMod::iZoomReg = NULL;
ICustButton    *UnwrapMod::iZoomExt = NULL;
ICustButton    *UnwrapMod::iUVW = NULL;
ICustButton    *UnwrapMod::iProp = NULL;
ICustButton    *UnwrapMod::iShowMap = NULL;
ICustButton    *UnwrapMod::iLockSelected = NULL;
ICustButton    *UnwrapMod::iFilterSelected = NULL;
ICustButton    *UnwrapMod::iHide = NULL;
ICustButton    *UnwrapMod::iFreeze = NULL;
ICustButton    *UnwrapMod::iIncSelected = NULL;
ICustButton    *UnwrapMod::iDecSelected = NULL;
ICustButton    *UnwrapMod::iSnap = NULL;

ICustButton    *UnwrapMod::iBreak = NULL;
ICustButton    *UnwrapMod::iWeldSelected = NULL;



SelectModBoxCMode *UnwrapMod::selectMode      = NULL;
MoveModBoxCMode   *UnwrapMod::moveGizmoMode      = NULL;
RotateModBoxCMode *UnwrapMod::rotGizmoMode       = NULL;
UScaleModBoxCMode *UnwrapMod::uscaleGizmoMode    = NULL;
NUScaleModBoxCMode *UnwrapMod::nuscaleGizmoMode   = NULL;
SquashModBoxCMode *UnwrapMod::squashGizmoMode    = NULL;
	

PaintSelectMode *UnwrapMod::paintSelectMode      = NULL;



COLORREF UnwrapMod::lineColor = RGB(255,255,255);
COLORREF UnwrapMod::selColor  = RGB(255,0,0);
COLORREF UnwrapMod::openEdgeColor = RGB(0,255,0);
COLORREF UnwrapMod::handleColor = RGB(255,255,0);
COLORREF UnwrapMod::freeFormColor = RGB(255,100,50);
COLORREF UnwrapMod::sharedColor = RGB(0,0,255);

COLORREF UnwrapMod::backgroundColor = RGB(60,60,60);

float UnwrapMod::weldThreshold = 0.01f;
BOOL UnwrapMod::update = FALSE;
int UnwrapMod::showVerts = 0;
int UnwrapMod::midPixelSnap = 0;

//watje tile
IOffScreenBuf  *UnwrapMod::iTileBuf = NULL;
BOOL		   UnwrapMod::tileValid = FALSE;


HCURSOR UnwrapMod::selCur   = NULL;
HCURSOR UnwrapMod::moveCur  = NULL;
HCURSOR UnwrapMod::moveXCur  = NULL;
HCURSOR UnwrapMod::moveYCur  = NULL;
HCURSOR UnwrapMod::rotCur   = NULL;
HCURSOR UnwrapMod::scaleCur = NULL;
HCURSOR UnwrapMod::scaleXCur = NULL;
HCURSOR UnwrapMod::scaleYCur = NULL;

HCURSOR UnwrapMod::zoomCur = NULL;
HCURSOR UnwrapMod::zoomRegionCur = NULL;
HCURSOR UnwrapMod::panCur = NULL;
HCURSOR UnwrapMod::weldCur = NULL;
HCURSOR UnwrapMod::weldCurHit = NULL;
HCURSOR UnwrapMod::sketchCur = NULL;
HCURSOR UnwrapMod::sketchPickCur = NULL;
HCURSOR UnwrapMod::sketchPickHitCur = NULL;
HWND UnwrapMod::hRelaxDialog = NULL;

/*
HCURSOR UnwrapMod::circleCur[0] = NULL;
HCURSOR UnwrapMod::circleCur[1] = NULL;
HCURSOR UnwrapMod::circleCur[2] = NULL;
HCURSOR UnwrapMod::circleCur[3] = NULL;
HCURSOR UnwrapMod::circleCur[4] = NULL;
HCURSOR UnwrapMod::circleCur[5] = NULL;
HCURSOR UnwrapMod::circleCur[6] = NULL;
HCURSOR UnwrapMod::circleCur[7] = NULL;
HCURSOR UnwrapMod::circleCur[8] = NULL;
HCURSOR UnwrapMod::circleCur[9] = NULL;
HCURSOR UnwrapMod::circleCur[10] = NULL;
HCURSOR UnwrapMod::circleCur[11] = NULL;
HCURSOR UnwrapMod::circleCur[12] = NULL;
HCURSOR UnwrapMod::circleCur[13] = NULL;
HCURSOR UnwrapMod::circleCur[14] = NULL;
HCURSOR UnwrapMod::circleCur[15] = NULL;

*/


//--- UnwrapMod methods -----------------------------------------------




UnwrapMod::~UnwrapMod()
{
	int ct = gfaces.Count();
	for (int i =0; i < ct; i++)
	{
		if (gfaces[i]->vecs) delete gfaces[i]->vecs;
		gfaces[i]->vecs = NULL;

		delete gfaces[i];
		gfaces[i] = NULL;
	}
	TVMaps.FreeFaces();
	TVMaps.FreeEdges();

	DeleteAllRefsFromMe();
}

static INT_PTR CALLBACK UnwrapSelRollupWndProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	static BOOL inEnter = FALSE;

	switch (msg) {
		case WM_INITDIALOG:
			{
			mod = (UnwrapMod*)lParam;
         DLSetWindowLongPtr(hWnd, lParam);
			mod->hSelRollup = hWnd;
			//setup element check box
			CheckDlgButton(hWnd,IDC_SELECTELEMENT_CHECK,mod->fnGetGeomElemMode());
			


			//setup element check box
			CheckDlgButton(hWnd,IDC_PLANARANGLE_CHECK,mod->fnGetGeomPlanarMode());

			mod->iPlanarThreshold = GetISpinner(GetDlgItem(hWnd,IDC_PLANARANGLE_SPIN));
			mod->iPlanarThreshold->LinkToEdit(GetDlgItem(hWnd,IDC_PLANARANGLE),EDITTYPE_FLOAT);
			mod->iPlanarThreshold->SetLimits(0.0f, 180.0f, FALSE);
			mod->iPlanarThreshold->SetAutoScale();	
			mod->iPlanarThreshold->SetValue(mod->fnGetGeomPlanarModeThreshold(),TRUE);

			mod->iMatID = GetISpinner(GetDlgItem(hWnd,IDC_MATID_SPIN));
			mod->iMatID->LinkToEdit(GetDlgItem(hWnd,IDC_MATID),EDITTYPE_INT);
			mod->iMatID->SetLimits(1, 255, FALSE);
			mod->iMatID->SetScale(0.1f);	
			mod->iMatID->SetValue(0,TRUE);

			mod->iSG = GetISpinner(GetDlgItem(hWnd,IDC_SG_SPIN2));
			mod->iSG->LinkToEdit(GetDlgItem(hWnd,IDC_SG),EDITTYPE_INT);
			mod->iSG->SetLimits(1, 32, FALSE);
			mod->iSG->SetScale(0.1f);	
			mod->iSG->SetValue(0,TRUE);

			ICustButton *iEditSeamsByPointButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_SEAMPOINTTOPOINT));
			iEditSeamsByPointButton->SetType(CBT_CHECK);
			iEditSeamsByPointButton->SetHighlightColor(GREEN_WASH);
			ReleaseICustButton(iEditSeamsByPointButton);

			mod->EnableFaceSelectionUI(FALSE);
			mod->EnableEdgeSelectionUI(FALSE);
			mod->EnableSubSelectionUI(FALSE);
			


			//setup back face cull check box
			CheckDlgButton(hWnd,IDC_IGNOREBACKFACING_CHECK,mod->fnGetBackFaceCull());			
			break;
			}
		case WM_CUSTEDIT_ENTER:	
		case CC_SPINNER_BUTTONUP:
			{
				if (LOWORD(wParam))
				{
					float angle = mod->iPlanarThreshold->GetFVal();
					mod->fnSetGeomPlanarModeThreshold(angle);
					//send macro message
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setGeomPlanarThreshold"), 1, 0,
						mr_float,mod->fnGetGeomPlanarModeThreshold());
					macroRecorder->EmitScript();
				}
				break;
			}

		case WM_COMMAND:
			switch (LOWORD(wParam)) 
			{

			case IDC_UNWRAP_LOOP:
				{
					mod->fnGeomLoopSelect();
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.geomEdgeLoopSelection"), 0, 0);
					break;
				}
			case IDC_UNWRAP_RING:
				{
					mod->fnGeomRingSelect();
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.geomEdgeRingSelection"), 0, 0);
					break;
				}


			case IDC_UNWRAP_SELECTEXPAND:
				{
					if (mod->ip->GetSubObjectLevel() == 1)
					{
						mod->fnGeomExpandVertexSel();
						//send macro message
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.expandGeomVertexSelection"), 0, 0);
						macroRecorder->EmitScript();
					}
					else if (mod->ip->GetSubObjectLevel() == 2)
					{
						mod->fnGeomExpandEdgeSel();
						//send macro message
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.expandGeomEdgeSelection"), 0, 0);
						macroRecorder->EmitScript();
					}

					else if (mod->ip->GetSubObjectLevel() == 3)
					{
						mod->fnGeomExpandFaceSel();
						//send macro message
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.expandGeomFaceSelection"), 0, 0);
						macroRecorder->EmitScript();
					}
					break;
				}
			case IDC_UNWRAP_SELECTCONTRACT:
				{
					if (mod->ip->GetSubObjectLevel() == 1)
					{
						mod->fnGeomContractVertexSel();
						//send macro message
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.contractGeomVertexSelection"), 0, 0);
						macroRecorder->EmitScript();
					}
					else if (mod->ip->GetSubObjectLevel() == 2)
					{
						mod->fnGeomContractEdgeSel();
						//send macro message
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.contractGeomEdgeSelection"), 0, 0);
						macroRecorder->EmitScript();
					}

					else if (mod->ip->GetSubObjectLevel() == 3)
					{
						mod->fnGeomContractFaceSel();
					//send macro message
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.contractGeomFaceSelection"), 0, 0);
						macroRecorder->EmitScript();
					}
					break;
				}
			case IDC_UNWRAP_EXPANDTOSEAMS:
				{
					mod->WtExecute(ID_PELT_EXPANDSELTOSEAM);
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltExpandSelectionToSeams"), 0, 0);						

					break;
				}
			case IDC_UNWRAP_SEAMPOINTTOPOINT:
				{			
					mod->WtExecute(ID_PELT_POINTTOPOINTSEAMS);
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltPointToPointSeamsMode"), 1, 0,	mr_bool,mod->fnGetPeltPointToPointSeamsMode());						
					break;
				}
			case IDC_UNWRAP_EDGESELTOSEAMS:
				{
					SHORT iret = GetAsyncKeyState (VK_CONTROL);
					if (iret==-32767)
					{
						mod->WtExecute(ID_PW_SELTOSEAM2);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltEdgeSelToSeam"), 1, 0,	mr_bool,FALSE);												
					}
					else
					{
						mod->WtExecute(ID_PW_SELTOSEAM);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltEdgeSelToSeam"), 1, 0,	mr_bool,TRUE);						
					}

					break;
				}
			case IDC_UNWRAP_SEAMSTOEDGESEL:
				{
					SHORT iret = GetAsyncKeyState (VK_CONTROL);
					if (iret==-32767)
					{
						mod->WtExecute(ID_PW_SEAMTOSEL2);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltSeamToEdgeSel"), 1, 0,	mr_bool,FALSE);						
						
					}
					else
					{
						mod->WtExecute(ID_PW_SEAMTOSEL);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltSeamToEdgeSel"), 1, 0,	mr_bool,TRUE);						
					}

					break;
				}


			case IDC_UNWRAP_SELECTSG:
				{
					int id = mod->iSG->GetIVal();
					mod->fnSelectBySG(id);
					//send macro message
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.selectBySG"), 1, 0,
						mr_int,id);
					macroRecorder->EmitScript();
					break;
				}
			case IDC_UNWRAP_SELECTMATID:
				{
					int id = mod->iMatID->GetIVal();
					mod->fnSelectByMatID(id);
					//send macro message
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.selectByMatID"), 1, 0,
						mr_int,id);
					macroRecorder->EmitScript();
					break;
				}
							



			case IDC_SELECTELEMENT_CHECK:
				{
					//set element mode swtich 
					//					CheckDlgButton(hWnd,IDC_SELECTELEMENT_CHECK,mod->fnGetGeomElemMode());
					mod->fnSetGeomElemMode(IsDlgButtonChecked(hWnd,IDC_SELECTELEMENT_CHECK));
					//send macro message
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setGeomSelectElementMode"), 1, 0,
						mr_bool,mod->fnGetGeomElemMode());
					macroRecorder->EmitScript();

					if (mod->hWnd)
					{
						IMenuBarContext* pContext = (IMenuBarContext*) GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
						if (pContext)
							pContext->UpdateWindowsMenu();
					}


					break;

				}
			case IDC_PLANARANGLE_CHECK:
				{
					//set element mode swtich 
					//					CheckDlgButton(hWnd,IDC_SELECTELEMENT_CHECK,mod->fnGetGeomElemMode());
					mod->fnSetGeomPlanarMode(IsDlgButtonChecked(hWnd,IDC_PLANARANGLE_CHECK));
					//send macro message
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setGeomPlanarThresholdMode"), 1, 0,
						mr_bool,mod->fnGetGeomPlanarMode());
					macroRecorder->EmitScript();

					if (mod->hWnd)
					{
						IMenuBarContext* pContext = (IMenuBarContext*) GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
						if (pContext)
							pContext->UpdateWindowsMenu();
					}


					break;
				}
			case IDC_IGNOREBACKFACING_CHECK:
				{
					//set element mode swtich 
					//					CheckDlgButton(hWnd,IDC_SELECTELEMENT_CHECK,mod->fnGetGeomElemMode());
					mod->fnSetBackFaceCull(IsDlgButtonChecked(hWnd,IDC_IGNOREBACKFACING_CHECK));
					//send macro message
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap2.setIgnoreBackFaceCull"), 1, 0,
						mr_bool,mod->fnGetBackFaceCull());
					macroRecorder->EmitScript();

					if (mod->hWnd)
					{
						IMenuBarContext* pContext = (IMenuBarContext*) GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
						if (pContext)
							pContext->UpdateWindowsMenu();
					}


					break;
				}


			}


		default:
			return FALSE;
			}
	return TRUE;
}


static INT_PTR CALLBACK UnwrapRollupWndProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	static BOOL inEnter = FALSE;

	switch (msg) {
case WM_INITDIALOG:
	{
		
		mod = (UnwrapMod*)lParam;
         DLSetWindowLongPtr(hWnd, lParam);
		mod->hParams = hWnd;
/*
		mod->iApplyButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_APPLY));
		mod->iApplyButton->SetType(CBT_PUSH);
		mod->iApplyButton->SetHighlightColor(GREEN_WASH);
		mod->iApplyButton->Enable(TRUE);
*/
		mod->iMapID = GetISpinner(GetDlgItem(hWnd,IDC_MAP_CHAN_SPIN));
		mod->iMapID->LinkToEdit(GetDlgItem(hWnd,IDC_MAP_CHAN),EDITTYPE_INT);
		mod->iMapID->SetLimits(1, 99, FALSE);
		mod->iMapID->SetScale(0.1f);	

		mod->SetupChannelButtons();
		//mod->ip->GetRightClickMenuManager()->Register(&rMenu);


		CheckDlgButton(hWnd,IDC_RADIO4,TRUE);



		CheckDlgButton(hWnd,IDC_DONOTREFLATTEN_CHECK,mod->fnGetPreventFlattening());

		BOOL thickSeams = mod->fnGetThickOpenEdges();
		BOOL showSeams = mod->fnGetViewportOpenEdges();

		CheckDlgButton(hWnd,IDC_SHOWMAPSEAMS_CHECK,FALSE);
		CheckDlgButton(hWnd,IDC_THINSEAM,FALSE);
		CheckDlgButton(hWnd,IDC_THICKSEAM,FALSE);


		if (thickSeams)
			CheckDlgButton(hWnd,IDC_THICKSEAM,TRUE);
		else CheckDlgButton(hWnd,IDC_THINSEAM,TRUE);
		
		if (showSeams)
		{
			CheckDlgButton(hWnd,IDC_SHOWMAPSEAMS_CHECK,TRUE);
		}



		CheckDlgButton(hWnd,IDC_ALWAYSSHOWPELTSEAMS_CHECK,mod->fnGetAlwayShowPeltSeams());

//		mod->alignDir = 3;


		break;
	}


case WM_CUSTEDIT_ENTER:
	{
		if (!inEnter)
		{
			inEnter = TRUE;
			TSTR buf1 = GetString(IDS_RB_SHOULDRESET);
			TSTR buf2 = GetString(IDS_RB_UNWRAPMOD);
			int tempChannel = mod->iMapID->GetIVal();
			if (tempChannel == 1) tempChannel = 0;
			if (tempChannel != mod->channel)
			{
				int res = MessageBox(mod->ip->GetMAXHWnd(),buf1,buf2,MB_YESNO|MB_ICONQUESTION|MB_TASKMODAL);
				if (res==IDYES)
				{
					theHold.Begin();
					mod->Reset();
					mod->channel = mod->iMapID->GetIVal();
					if (mod->channel == 1) mod->channel = 0;
					theHold.Accept(GetString(IDS_RB_SETCHANNEL));					

					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setMapChannel"), 1, 0,
						mr_int,mod->channel);
					macroRecorder->EmitScript();

					mod->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
					mod->ip->RedrawViews(mod->ip->GetTime());
					mod->InvalidateView();
				}
				else mod->SetupChannelButtons();
			}
			inEnter = FALSE;
		}

	}

	break;
case CC_SPINNER_BUTTONUP:
	{
		TSTR buf1 = GetString(IDS_RB_SHOULDRESET);
		TSTR buf2 = GetString(IDS_RB_UNWRAPMOD);
		int tempChannel = mod->iMapID->GetIVal();
		if (tempChannel == 1) tempChannel = 0;
		if (tempChannel != mod->channel)
		{
			int res = MessageBox(mod->ip->GetMAXHWnd(),buf1,buf2,MB_YESNO|MB_ICONQUESTION|MB_TASKMODAL);
			if (res==IDYES)
			{
				theHold.Begin();
				mod->Reset();
				mod->channel = mod->iMapID->GetIVal();
				if (mod->channel == 1) mod->channel = 0;
				theHold.Accept(GetString(IDS_RB_SETCHANNEL));					
				mod->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);

				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setMapChannel"), 1, 0,
					mr_int,mod->channel);
				macroRecorder->EmitScript();

				mod->SetCheckerMapChannel();

				mod->ip->RedrawViews(mod->ip->GetTime());
				mod->InvalidateView();
			}
			else mod->SetupChannelButtons();
		}

	}

	break;


case WM_COMMAND:
	switch (LOWORD(wParam)) {
case IDC_UNWRAP_SAVE:
	mod->fnSave();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.save"), 0, 0 );
	macroRecorder->EmitScript();

	break;
case IDC_UNWRAP_LOAD:
	mod->fnLoad();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.load"), 0, 0 );
	macroRecorder->EmitScript();
	break;
/*
case IDC_RADIO1: 
	if (IsDlgButtonChecked(hWnd,IDC_RADIO1)) mod->alignDir = 0;
	mod->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	mod->ip->RedrawViews(mod->ip->GetTime());
	mod->InvalidateView();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setProjectionType"), 1, 0,
		mr_int,1);
	macroRecorder->EmitScript();

	break;
case IDC_RADIO2:
	if (IsDlgButtonChecked(hWnd,IDC_RADIO2)) mod->alignDir = 1;
	mod->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	mod->ip->RedrawViews(mod->ip->GetTime());
	mod->InvalidateView();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setProjectionType"), 1, 0,
		mr_int,2);
	macroRecorder->EmitScript();
	break;
case IDC_RADIO3:
	if (IsDlgButtonChecked(hWnd,IDC_RADIO3)) mod->alignDir = 2;
	mod->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	mod->ip->RedrawViews(mod->ip->GetTime());
	mod->InvalidateView();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setProjectionType"), 1, 0,
		mr_int,3);
	macroRecorder->EmitScript();
	break;

case IDC_RADIO4:
	if (IsDlgButtonChecked(hWnd,IDC_RADIO4)) mod->alignDir = 3;
	mod->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	mod->ip->RedrawViews(mod->ip->GetTime());
	mod->InvalidateView();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setProjectionType"), 1, 0,
		mr_int,4);
	macroRecorder->EmitScript();

	break;
*/
case IDC_MAP_CHAN1:
case IDC_MAP_CHAN2: {
	TSTR buf1 = GetString(IDS_RB_SHOULDRESET);
	TSTR buf2 = GetString(IDS_RB_UNWRAPMOD);
   int res = IDYES;
   if (!mod->suppressWarning)
      res = MessageBox(mod->ip->GetMAXHWnd(),buf1,buf2,MB_YESNO|MB_ICONQUESTION|MB_TASKMODAL);
	if (res==IDYES)
	{
		theHold.Begin();
		mod->Reset();
		mod->channel = IsDlgButtonChecked(hWnd,IDC_MAP_CHAN2);
		if (mod->channel == 1)
			mod->iMapID->Enable(FALSE);
		else 
		{
			int ival = mod->iMapID->GetIVal();
			if (ival == 1) mod->channel = 0;
			else mod->channel = ival;
			mod->iMapID->Enable(TRUE);
		}
		mod->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
		mod->ip->RedrawViews(mod->ip->GetTime());
		mod->InvalidateView();
		if (mod->channel == 1)
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setVC"), 1, 0,
			mr_bool,TRUE);
		else macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setVC"), 1, 0,
			mr_bool,FALSE);
		macroRecorder->EmitScript();

		theHold.Accept(GetString(IDS_RB_SETCHANNEL));					
	}
	else mod->SetupChannelButtons();

	break;
					}

case IDC_UNWRAP_RESET: {
	mod->fnReset();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.reset"), 0, 0 );
	macroRecorder->EmitScript();
	break;
					   }

case IDC_UNWRAP_APPLY:
	{
		mod->fnPlanarMap();
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.planarmap"), 0, 0 );
		macroRecorder->EmitScript();
		break;
	}
case IDC_UNWRAP_EDIT:
	mod->fnEdit();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.edit"), 0, 0 );
	macroRecorder->EmitScript();

	break;
case IDC_ALWAYSSHOWPELTSEAMS_CHECK:
	
	mod->fnSetAlwayShowPeltSeams(IsDlgButtonChecked(hWnd,IDC_ALWAYSSHOWPELTSEAMS_CHECK));
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltAlwaysShowSeams"), 1, 0,
			mr_bool,mod->fnGetAlwayShowPeltSeams());
	macroRecorder->EmitScript();

	break;
case IDC_DONOTREFLATTEN_CHECK:
	{
		//set element mode swtich 
		//					CheckDlgButton(hWnd,IDC_SELECTELEMENT_CHECK,mod->fnGetGeomElemMode());
		mod->fnSetPreventFlattening(IsDlgButtonChecked(hWnd,IDC_DONOTREFLATTEN_CHECK));
		//send macro message
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setPreventFlattening"), 1, 0,
			mr_bool,mod->fnGetPreventFlattening());
		macroRecorder->EmitScript();

		if (mod->hWnd)
		{
			IMenuBarContext* pContext = (IMenuBarContext*) GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
			if (pContext)
				pContext->UpdateWindowsMenu();
		}


		break;
	}
case IDC_SHOWMAPSEAMS_CHECK:
	{
		BOOL showMapSeams = IsDlgButtonChecked(hWnd,IDC_SHOWMAPSEAMS_CHECK);
		if (showMapSeams)
		{
			mod->fnSetViewportOpenEdges(TRUE);
		}
		else
		{
			mod->fnSetViewportOpenEdges(FALSE);
		}

		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setShowMapSeams"), 1, 0,
						mr_bool,mod->fnGetViewportOpenEdges());
		mod->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
		mod->ip->RedrawViews(mod->ip->GetTime());

		break;
	}

case IDC_THINSEAM:
case IDC_THICKSEAM:
	{
		BOOL thickSeams = IsDlgButtonChecked(hWnd,IDC_THICKSEAM);

		if (thickSeams)
			mod->fnSetThickOpenEdges(TRUE);
		else mod->fnSetThickOpenEdges(FALSE);

		mod->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
		mod->ip->RedrawViews(mod->ip->GetTime());

		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap4.setThickOpenEdges"), 1, 0,
						mr_bool,mod->fnGetThickOpenEdges());



		break;
	}
	}
	break;

default:
	return FALSE;
	}
	return TRUE;
}

void UnwrapMod::SetupChannelButtons()
{
	if (hParams && editMod==this) {		
		if (channel == 0)
		{
			iMapID->Enable(TRUE);
			iMapID->SetValue(1,TRUE);
			CheckDlgButton(hParams,IDC_MAP_CHAN1,TRUE);
			CheckDlgButton(hParams,IDC_MAP_CHAN2,FALSE);

		}
		else if (channel == 1)
		{
			CheckDlgButton(hParams,IDC_MAP_CHAN1,FALSE);
			CheckDlgButton(hParams,IDC_MAP_CHAN2,TRUE);
			iMapID->Enable(FALSE);
			//			iMapID->SetValue(0,TRUE);
		}
		else
		{
			CheckDlgButton(hParams,IDC_MAP_CHAN1,TRUE);
			CheckDlgButton(hParams,IDC_MAP_CHAN2,FALSE);
			iMapID->Enable(TRUE);
			iMapID->SetValue(channel,TRUE);
		}
	}
}

void UnwrapMod::Reset()
{
	if (theHold.Holding()) theHold.Put(new ResetRestore(this));

	for (int i=0; i<TVMaps.cont.Count(); i++) 
		if (TVMaps.cont[i]) DeleteReference(i+11+100);
	TVMaps.v.Resize(0);
	TVMaps.FreeFaces();
	TVMaps.f.Resize(0);
	TVMaps.cont.Resize(0);
	TVMaps.edgesValid = FALSE;
	vsel.SetSize(0);
	updateCache = TRUE;
	firstPass = TRUE;

	NotifyDependents(FOREVER,0,REFMSG_CONTROLREF_CHANGE,TREE_VIEW_CLASS_ID,FALSE);	
	NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
}

BOOL UnwrapMod::AssignController(Animatable *control,int subAnim)
{
	ReplaceReference(subAnim+11+100,(RefTargetHandle)control);	
	NotifyDependents(FOREVER,0,REFMSG_CONTROLREF_CHANGE,TREE_VIEW_CLASS_ID,FALSE);	
	NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	return TRUE;
}


Object *GetBaseObject(Object *obj)
{
	if (obj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		IDerivedObject* dobj;
		dobj = (IDerivedObject*)obj;
		while (dobj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
		{
			dobj = (IDerivedObject*)dobj->GetObjRef();
		}

		return (Object *) dobj;

	}
	return obj;

}


BOOL UnwrapMod::IsInStack(INode *node)
{
	int				i;
	SClass_ID		sc;
	IDerivedObject* dobj;


	// then osm stack
	Object* obj = node->GetObjectRef();
	int ct = 0;

	if ((dobj = node->GetWSMDerivedObject()) != NULL)
	{
		for (i = 0; i < dobj->NumModifiers(); i++)
		{
			Modifier *m = dobj->GetModifier(i);
			if (m==this) return TRUE;

		}
	}

	if ((sc = obj->SuperClassID()) == GEN_DERIVOB_CLASS_ID)
	{
		dobj = (IDerivedObject*)obj;

		while (sc == GEN_DERIVOB_CLASS_ID)
		{
			for (i = 0; i < dobj->NumModifiers(); i++)
			{
				TSTR name;

				Modifier *m = dobj->GetModifier(i);

				if (m == this)
					return TRUE;


			}
			dobj = (IDerivedObject*)dobj->GetObjRef();
			sc = dobj->SuperClassID();
		}
	}
	return FALSE;



}

void UnwrapMod::BeginEditParams(
								IObjParam  *ip, ULONG flags,Animatable *prev)
{
	if (loadDefaults)
	{
		fnLoadDefaults();
		loadDefaults = FALSE;
	}

	selCur   = ip->GetSysCursor(SYSCUR_SELECT);
	moveCur	 = ip->GetSysCursor(SYSCUR_MOVE);

	if (moveXCur == NULL)
		moveXCur	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_MOVEX));
	if (moveYCur == NULL)
		moveYCur	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_MOVEY));

	if (scaleXCur == NULL)
		scaleXCur	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_SCALEX));
	if (scaleYCur == NULL)
		scaleYCur	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_SCALEY));

	if (zoomCur == NULL)
		zoomCur	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_ZOOM));
	if (zoomRegionCur == NULL)
		zoomRegionCur	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_ZOOMREG));
	if (panCur == NULL)
		panCur	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_PANHAND));
	if (weldCur == NULL)
		weldCur	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_WELDCUR));

	if (weldCurHit == NULL)
		weldCurHit	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_WELDCUR1));

	if (sketchCur == NULL)
		sketchCur	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_SKETCHCUR));

	if (sketchPickCur == NULL)
		sketchPickCur	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_SKETCHPICKCUR));

	if (sketchPickHitCur == NULL)
		sketchPickHitCur	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_SKETCHPICKCUR1));


	if (circleCur[0] == NULL)
		circleCur[0]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR1));
	if (circleCur[1] == NULL)
		circleCur[1]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR2));
	if (circleCur[2] == NULL)
		circleCur[2]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR3));
	if (circleCur[3] == NULL)
		circleCur[3]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR4));
	if (circleCur[4] == NULL)
		circleCur[4]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR5));
	if (circleCur[5] == NULL)
		circleCur[5]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR6));
	if (circleCur[6] == NULL)
		circleCur[6]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR7));
	if (circleCur[7] == NULL)
		circleCur[7]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR8));
	if (circleCur[8] == NULL)
		circleCur[8]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR9));
	if (circleCur[9] == NULL)
		circleCur[9]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR10));
	if (circleCur[10] == NULL)
		circleCur[10]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR11));
	if (circleCur[11] == NULL)
		circleCur[11]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR12));
	if (circleCur[12] == NULL)
		circleCur[12]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR13));
	if (circleCur[13] == NULL)
		circleCur[13]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR14));
	if (circleCur[14] == NULL)
		circleCur[14]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR15));
	if (circleCur[15] == NULL)
		circleCur[15]	 = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR16));



	rotCur   = ip->GetSysCursor(SYSCUR_ROTATE);
	scaleCur = ip->GetSysCursor(SYSCUR_USCALE);


	// Add our sub object type
	// TSTR type1(GetString(IDS_PW_SELECTFACE));
	// TSTR type2 (GetString(IDS_PW_FACEMAP));
	// TSTR type3 (GetString(IDS_PW_PLANAR));
	// const TCHAR *ptype[] = {type1};
	// This call is obsolete. Please see BaseObject::NumSubObjTypes() and BaseObject::GetSubObjType()
	// ip->RegisterSubObjectTypes(ptype, 1);

	selectMode = new SelectModBoxCMode(this,ip);
	moveGizmoMode = new MoveModBoxCMode(this,ip);
	rotGizmoMode       = new RotateModBoxCMode(this,ip);
	uscaleGizmoMode    = new UScaleModBoxCMode(this,ip);
	nuscaleGizmoMode   = new NUScaleModBoxCMode(this,ip);
	squashGizmoMode    = new SquashModBoxCMode(this,ip);	

	offsetControl = NewDefaultPoint3Controller();
	Point3 p(0.5f,0.5f,0.0f);
	offsetControl->SetValue(0,&p,CTRL_ABSOLUTE);

	scaleControl = NewDefaultPoint3Controller();
	Point3 sp(0.25f,0.25f,0.0f);
	scaleControl->SetValue(0,&sp,CTRL_ABSOLUTE);

	rotateControl = NewDefaultFloatController();
	float a = 0.0f;
	rotateControl->SetValue(0,&a,CTRL_ABSOLUTE);


	this->ip = ip;
	editMod  = this;
	TimeValue t = ip->GetTime();
	//aszabo|feb.05.02
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);

	hParams  = ip->AddRollupPage( 
		hInstance, 
		MAKEINTRESOURCE(IDD_UNWRAP_SELPARAMS),
		UnwrapSelRollupWndProc,
		GetString(IDS_PW_SELPARAMS),
		(LPARAM)this);

	hParams  = ip->AddRollupPage( 
		hInstance, 
		MAKEINTRESOURCE(IDD_UNWRAP_PARAMS),
		UnwrapRollupWndProc,
		GetString(IDS_RB_PARAMETERS),
		(LPARAM)this);
	//PELT
	hMapParams  = ip->AddRollupPage( 
		hInstance, 
		MAKEINTRESOURCE(IDD_UNWRAP_MAPPARAMS),
		UnwrapMapRollupWndProc,
		GetString(IDS_RB_MAPPARAMETERS),
		(LPARAM)this);
	peltData.SetRollupHandle(hMapParams);
	peltData.SetSelRollupHandle(hSelRollup);
	peltData.SetParamRollupHandle(hParams);


	ip->RegisterTimeChangeCallback(this);

	SetNumSelLabel();
	firstPass = TRUE;

	
	//	actionTable = new UnwrapActionCB(this);
	//	ip->GetActionManager()->ActivateActionTable(actionTable, kUnwrapActions);

	if ((alwaysEdit) && (!popUpDialog))
		fnEdit();
	RebuildEdges();

	MyEnumProc dep;              
	DoEnumDependents(&dep);

	modifierInstanced = FALSE;
	if (dep.Nodes.Count() > 1)
	{


		Object *obj = NULL;
		BOOL first = TRUE;
		for (int i = 0; i < dep.Nodes.Count(); i++)
		{
			if (first)
			{
				if (IsInStack(dep.Nodes[i]))
				{
					obj = GetBaseObject(dep.Nodes[i]->GetObjectRef());
					first = FALSE;
				}
			}
			else if (obj)
			{

				Object *tobj = GetBaseObject(dep.Nodes[i]->GetObjectRef());
				if (IsInStack(dep.Nodes[i]))
				{
					if (obj != tobj) modifierInstanced = TRUE;
				}
			}
		}

		if (modifierInstanced )
		{
			fnSetPreventFlattening(TRUE);
			EnableWindow(GetDlgItem(hParams,IDC_RADIO1),FALSE);
			EnableWindow(GetDlgItem(hParams,IDC_RADIO2),FALSE);
			EnableWindow(GetDlgItem(hParams,IDC_RADIO3),FALSE);
			EnableWindow(GetDlgItem(hParams,IDC_RADIO4),FALSE);
			EnableWindow(GetDlgItem(hParams,IDC_UNWRAP_EDIT),FALSE);
			EnableWindow(GetDlgItem(hParams,IDC_UNWRAP_RESET),FALSE);
			EnableWindow(GetDlgItem(hParams,IDC_UNWRAP_SAVE),FALSE);
			EnableWindow(GetDlgItem(hParams,IDC_UNWRAP_LOAD),FALSE);
			EnableWindow(GetDlgItem(hParams,IDC_MAP_CHAN1),FALSE);
			EnableWindow(GetDlgItem(hParams,IDC_MAP_CHAN2),FALSE);
			EnableWindow(GetDlgItem(hParams,IDC_DONOTREFLATTEN_CHECK),FALSE);

			if (iMapID) iMapID->Enable(FALSE);


			EnableWindow(GetDlgItem(hSelRollup,IDC_IGNOREBACKFACING_CHECK),FALSE);
			EnableWindow(GetDlgItem(hSelRollup,IDC_SELECTELEMENT_CHECK),FALSE);
			EnableWindow(GetDlgItem(hSelRollup,IDC_PLANARANGLE_CHECK),FALSE);

			if (iPlanarThreshold) iPlanarThreshold->Enable(FALSE);
			if (iMatID) iMatID->Enable(FALSE);
			if (iSG) iSG->Enable(FALSE);

			ICustButton *iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_SELECTEXPAND));
			iTempButton->Enable(FALSE);
			ReleaseICustButton(iTempButton);

			iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_SELECTCONTRACT));
			iTempButton->Enable(FALSE);
			ReleaseICustButton(iTempButton);

			iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_SELECTMATID));
			iTempButton->Enable(FALSE);
			ReleaseICustButton(iTempButton);

			iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_SELECTSG));
			iTempButton->Enable(FALSE);
			ReleaseICustButton(iTempButton);

			subObjCount = 0;
//			NotifyDependents(FOREVER,PART_ALL,REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
			mUpdateStackUI = TRUE;

		}
	}

	peltPointToPointMode = new PeltPointToPointMode(this,ip);

	RegisterNotification(PreSave, this,NOTIFY_FILE_PRE_SAVE);
	RegisterNotification(PostSave, this,NOTIFY_FILE_POST_SAVE);
	ActivateActionTable();
	
}


void UnwrapMod::EndEditParams(
							  IObjParam *ip,ULONG flags,Animatable *next)
{	
	
	peltData.SubObjectUpdateSuspend();
	fnSetMapMode(0);
	peltData.SubObjectUpdateResume();

	ClearAFlag(A_MOD_BEING_EDITED);

	//	ip->GetActionManager()->DeactivateActionTable(actionTable, kUnwrapActions);
	//	delete actionTable;
	
   if (hWnd) DestroyWindow(hWnd);


	ip->UnRegisterTimeChangeCallback(this);

	if (hSelRollup) ip->DeleteRollupPage(hSelRollup);
	hSelRollup  = NULL;	

	if (hParams) ip->DeleteRollupPage(hParams);
	hParams  = NULL;	

	//PELT
	if (hMapParams) ip->DeleteRollupPage(hMapParams);
	hMapParams  = NULL;	
	peltData.SetRollupHandle(NULL);
	peltData.SetSelRollupHandle(NULL);
	peltData.SetParamRollupHandle(NULL);



	ip->DeleteMode(selectMode);

	if (selectMode) delete selectMode;
	selectMode = NULL;

	ip->DeleteMode(moveGizmoMode);
	ip->DeleteMode(rotGizmoMode);
	ip->DeleteMode(uscaleGizmoMode);
	ip->DeleteMode(nuscaleGizmoMode);
	ip->DeleteMode(squashGizmoMode);

	if (moveGizmoMode) delete moveGizmoMode;
	moveGizmoMode = NULL;
	if (rotGizmoMode) delete rotGizmoMode;
	rotGizmoMode = NULL;
	if (uscaleGizmoMode) delete uscaleGizmoMode;
	uscaleGizmoMode = NULL;
	if (nuscaleGizmoMode) delete nuscaleGizmoMode;
	nuscaleGizmoMode = NULL;
	if (squashGizmoMode) delete squashGizmoMode;
	squashGizmoMode = NULL;


	
		
	if (peltData.GetPointToPointSeamsMode())
		peltData.SetPointToPointSeamsMode(this,FALSE);

	ip->DeleteMode(peltPointToPointMode);	
	if (peltPointToPointMode) delete peltPointToPointMode;
	peltPointToPointMode = NULL;

    peltData.SetPeltMapMode(this,FALSE);
	mapMapMode = NOMAP;



	ReleaseISpinner(iMapID); iMapID = NULL;
	ReleaseISpinner(iPlanarThreshold); iPlanarThreshold = NULL;
	ReleaseISpinner(iMatID); iMatID = NULL;
	ReleaseISpinner(iSG); iSG = NULL;


	//	ip->GetRightClickMenuManager()->Unregister(&rMenu);


	TimeValue t =ip->GetTime();
	//aszabo|feb.05.02
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
	this->ip = NULL;

	editMod  = NULL;

	//PUTBACK	ip->EnableSubObjectSelection(TRUE);
	TVMaps.FreeEdges();
	TVMaps.edgesValid = FALSE;

	if (CurrentMap == 0)
		ShowCheckerMaterial(FALSE);

	UnRegisterNotification(PreSave, this,NOTIFY_FILE_PRE_SAVE);
	UnRegisterNotification(PostSave, this,NOTIFY_FILE_POST_SAVE);
	DeActivateActionTable();

}




class NullView: public View {
public:
	Point2 ViewToScreen(Point3 p) { return Point2(p.x,p.y); }
	NullView() { worldToView.IdentityMatrix(); screenW=640.0f; screenH = 480.0f; }
};

static AdjFaceList *BuildAdjFaceList(Mesh &mesh)
{
	AdjEdgeList ae(mesh);
	return new AdjFaceList(mesh,ae);
}


Box3 UnwrapMod::BuildBoundVolume(Object *obj)

{
	Box3 b;
	b.Init();
	if (objType == IS_PATCH)
	{
		PatchObject *pobj = (PatchObject*)obj;
		for (int i = 0; i < pobj->patch.patchSel.GetSize(); i++)
		{
			if (pobj->patch.patchSel[i])
			{
				int pcount = 3;
				if (pobj->patch.patches[i].type == PATCH_QUAD) pcount = 4;
				for (int j = 0; j < pcount; j++)
				{
					int index = pobj->patch.patches[i].v[j];

					b+= pobj->patch.verts[index].p;
				}
			}	
		}

	}	
	else if (objType == IS_MESH)
	{
		TriObject *tobj = (TriObject*)obj;
		for (int i = 0; i < tobj->GetMesh().faceSel.GetSize(); i++)
		{
			if (tobj->GetMesh().faceSel[i])
			{
				for (int j = 0; j < 3; j++)
				{
					int index = tobj->GetMesh().faces[i].v[j];

					b+= tobj->GetMesh().verts[index];
				}
			}	
		}
	}
	else if (objType == IS_MNMESH)
	{
		PolyObject *tobj = (PolyObject*)obj;
		BitArray sel;
		tobj->GetMesh().getFaceSel (sel);
		for (int i = 0; i < sel.GetSize(); i++)
		{
			if (sel[i])
			{
				int ct;
				ct = tobj->GetMesh().f[i].deg;
				for (int j = 0; j < ct; j++)
				{
					int index = tobj->GetMesh().f[i].vtx[j];

					b+= tobj->GetMesh().v[index].p;
				}
			}	
		}
	}
	return b;
}


void UnwrapMod::InitControl(TimeValue t)
{
	Box3 box;
	Matrix3 tm;

	if (tmControl==NULL) {
		ReplaceReference(0,NewDefaultMatrix3Controller()); 
		NotifyDependents(FOREVER,0,REFMSG_CONTROLREF_CHANGE);

	}		

	if (flags&CONTROL_INIT) {
		SuspendAnimate();
		AnimateOff();	



		// get our bounding box
		if (TVMaps.geomPoints.Count() > 0)
		{
			Box3 bounds;
			bounds.Init();

			// get our center
			for (int i = 0; i < TVMaps.geomPoints.Count(); i++)
			{
				bounds += TVMaps.geomPoints[i];
			}
			Point3 center = bounds.Center();
			// build the scale
			Point3 scale(bounds.Width().x ,bounds.Width().y , bounds.Width().z);
			tm.SetRow(0,Point3(scale.x,0.0f,0.0f));
			tm.SetRow(1,Point3(0.0f,scale.y,0.0f));
			tm.SetRow(2,Point3(0.0f,0.0f,scale.z));
			tm.SetRow(3,center);
			

			Matrix3 ptm(1), id(1);
			SetXFormPacket tmpck(tm,ptm);
			tmControl->SetValue(t,&tmpck,TRUE,CTRL_RELATIVE);

		}
		ResumeAnimate();


	}
	flags = 0;
}



Matrix3 UnwrapMod::CompMatrix(
							  TimeValue t,ModContext *mc, Matrix3 *ntm,BOOL applySize, BOOL applyAxis)
{
	Matrix3 tm(1);
	Interval valid;

	//	int type = GetMapType();

	if (tmControl) {
		tmControl->GetValue(t,&tm,valid,CTRL_RELATIVE);
	}

	if (mc && mc->tm) {
		tm = tm * Inverse(*mc->tm);
	}

	if (ntm) {
		tm = tm * *ntm;
	}
	return tm;
}

static int lStart[12] = {0,1,3,2,4,5,7,6,0,1,2,3};
static int lEnd[12]   = {1,3,2,0,5,7,6,4,4,5,6,7};

static void DoBoxIcon(BOOL sel,float length, PolyLineProc& lp)
{
	Point3 pt[3];

	length *= 0.5f;
	Box3 box;
	box.pmin = Point3(-length,-length,-length);
	box.pmax = Point3( length, length, length);

	if (sel) //lp.SetLineColor(1.0f,1.0f,0.0f);
		lp.SetLineColor(GetUIColor(COLOR_SEL_GIZMOS));
	else //lp.SetLineColor(0.85f,0.5f,0.0f);		
		lp.SetLineColor(GetUIColor(COLOR_GIZMOS));

	for (int i=0; i<12; i++) {
		pt[0] = box[lStart[i]];
		pt[1] = box[lEnd[i]];
		lp.proc(pt,2);
	}
}


void UnwrapMod::ComputeSelectedFaceData()
{
	Point3 pnorm(0.0f,0.0f,0.0f);
	int ct = 0;
	gCenter.x = 0.0f;
	gCenter.y = 0.0f;
	gCenter.z = 0.0f;
	int gCt = 0;
	int dir = GetQMapAlign();
	if (dir == 0) //x
	{
		gNormal.x = 1.0f; 
		gNormal.y = 0.0f; 
		gNormal.z = 0.0f; 
	}
	else if (dir == 1) //y
	{	
		gNormal.y = 1.0f; 
		gNormal.x = 0.0f; 
		gNormal.z = 0.0f; 
	}	
	else if (dir == 2) //z
	{
		gNormal.z = 1.0f; 
		gNormal.x = 0.0f; 
		gNormal.y = 0.0f; 
	}
	else
	{
		for (int k=0; k<gfaces.Count(); k++) {
			// Grap the three points, xformed
			int pcount = 3;
			//		if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
			pcount = gfaces[k]->count;

			Point3 temp_point[4];
			for (int j=0; j<pcount; j++) {
				int index = gfaces[k]->t[j];
				//					Point3 temp_point;
				if (j < 4)
					temp_point[j] = gverts.d[index].p;
				gCenter += gverts.d[index].p;

			}
			pnorm += Normalize(temp_point[1]-temp_point[0]^temp_point[2]-temp_point[1]);
			ct++;
		}

	  // Skip divide by zero situation, make it equal to zero, which happens when gfaces is empty
	  if(gfaces.Count() > 0)
	  {
		gNormal = pnorm/(float)ct;
		gNormal = Normalize(gNormal);
	}
   }
	gCenter.x = 0.0f;
	gCenter.y = 0.0f;
	gCenter.z = 0.0f;

	gCt = 0;
	float minx = 99999999999.9f,miny = 99999999999.9f,maxx= -999999999.0f,maxy= -9999999999.0f;
	float minz = 99999999999.9f,maxz = -99999999999.9f;

	for (int k=0; k<gverts.d.Count(); k++) {
		// Grap the three points, xformed
		if (gverts.sel[k])
		{
			Point3 pd2 = gverts.d[k].p;
			if (pd2.x < minx) minx = pd2.x;
			if (pd2.y < miny) miny = pd2.y;
			if (pd2.z < minz) minz = pd2.z;
			if (pd2.x > maxx) maxx = pd2.x;
			if (pd2.y > maxy) maxy = pd2.y;
			if (pd2.z > maxz) maxz = pd2.z;
		}
	}

	gCenter.x = (float) (maxx+minx)/2.0f ;
	gCenter.y = (float) (maxy+miny)/2.0f ;
	gCenter.z = (float) (maxz+minz)/2.0f ;


	Matrix3 tm;
	UnwrapMatrixFromNormal(gNormal,tm);
	tm.SetTrans(gCenter);
	tm = Inverse(tm);
	minx = 99999999999.9f;
	miny = 99999999999.9f;
	maxx= -999999999.0f;
	maxy= -9999999999.0f;
	minz = 99999999999.9f;
	maxz = -99999999999.9f;

for (int k=0; k<gfaces.Count(); k++) {
		// Grap the three points, xformed
		int pcount = 3;
		//	if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
		pcount = gfaces[k]->count;

		for (int j=0; j<pcount; j++) {
			int index = gfaces[k]->t[j];
			Point3 pd2 = gverts.d[index].p*tm;
			if (pd2.x < minx) minx = pd2.x;
			if (pd2.y < miny) miny = pd2.y;
			if (pd2.z < minz) minz = pd2.z;
			if (pd2.x > maxx) maxx = pd2.x;
			if (pd2.y > maxy) maxy = pd2.y;
			if (pd2.z > maxz) maxz = pd2.z;

		}
	}
	gXScale = (float) fabs(maxx-minx)/2.0f ;
	gYScale = (float) fabs(maxy-miny)/2.0f ;
	gZScale = (float) fabs(maxz-minz)/2.0f ;

	if (gXScale == 0.0f) gXScale = 1.0f;
	if (gYScale == 0.0f) gYScale = 1.0f;
	if (gZScale == 0.0f) gZScale = 1.0f;


}

void UnwrapMod::DoIcon(PolyLineProc& lp,BOOL sel)
{
	//	int type = GetMapType();	
	type = MAP_PLANAR;
	switch (type) {
case MAP_BOX: DoBoxIcon(sel,2.0f,lp); break;
case MAP_PLANAR: DoPlanarMapIcon(sel,2.0f,2.0f,lp); break;
case MAP_BALL:
case MAP_SPHERICAL: DoSphericalMapIcon(sel,1.0f,lp); break;
case MAP_CYLINDRICAL: DoCylindricalMapIcon(sel,1.0f,1.0f,lp); break;
	}
}

static void Draw3dEdge(GraphicsWindow *gw, float size, Point3 a, Point3 b, Color c)

{
	Matrix3 tm;
	Point3 vec = Normalize(a-b);
	MatrixFromNormal(vec, tm);
	Point3 vecA,vecB,vecC;
	vecA = tm.GetRow(0)*size;
	vecB = tm.GetRow(1)*size;
	Point3 p[3];
	Point3 color[3];
	Point3 ca = Point3(c);
	color[0] = ca;
	color[1] = ca;
	color[2] = ca;

	p[0] = a + vecA + vecB;
	p[1] = b + vecA + vecB;
	p[2] = a - vecA + vecB;
	gw->triangle(p,color);

	p[0] = b - vecA + vecB;
	p[1] = a - vecA + vecB;
	p[2] = b + vecA + vecB;
	gw->triangle(p,color);

	p[2] = a + vecA - vecB;
	p[1] = b + vecA - vecB;
	p[0] = a - vecA - vecB;
	gw->triangle(p,color);

	p[2] = b - vecA - vecB;
	p[1] = a - vecA - vecB;
	p[0] = b + vecA - vecB;
	gw->triangle(p,color);


	p[2] = a + vecB + vecA;
	p[1] = b + vecB + vecA;
	p[0] = a - vecB + vecA;
	gw->triangle(p,color);

	p[2] = b - vecB + vecA;
	p[1] = a - vecB + vecA;
	p[0] = b + vecB + vecA;
	gw->triangle(p,color);

	p[0] = a + vecB - vecA;
	p[1] = b + vecB - vecA;
	p[2] = a - vecB - vecA;
	gw->triangle(p,color);

	p[0] = b - vecB - vecA;
	p[1] = a - vecB - vecA;
	p[2] = b + vecB - vecA;
	gw->triangle(p,color);
}

//PELT
void UnwrapMod::BuildVisibleFaces(GraphicsWindow *gw, ModContext *mc, BitArray &visibleFaces)
{
	if (mc->localData == NULL) return;

	visibleFaces.SetSize(TVMaps.f.Count());
	visibleFaces.ClearAll();

	gw->clearHitCode();

	HitRegion hr;

	hr.type = RECT_RGN;
	hr.crossing  = true;
	int w = gw->getWinSizeX();
	int h = gw->getWinSizeY();
	hr.rect.left = 0;
	hr.rect.top = 0;
	hr.rect.right = w;
	hr.rect.bottom = h;




	if (objType == IS_MESH)
	{
		gw->setHitRegion(&hr);


		SubObjHitList hitList;
		MeshSubHitRec *rec;	

		if (!((MeshTopoData*)mc->localData)->GetMesh()) return;

		Mesh &mesh = *((MeshTopoData*)mc->localData)->GetMesh();
		//				mesh.faceSel = ((MeshTopoData*)mc->localData)->faceSel;
		int res = mesh.SubObjectHitTest(gw, gw->getMaterial(), &hr,
			flags|SUBHIT_FACES|SUBHIT_SELSOLID, hitList);

		rec = hitList.First();
		while (rec) {
			int hitface = rec->index;
			if (hitface < visibleFaces.GetSize())
			visibleFaces.Set(hitface,true);
			rec = rec->Next();
		}
	}
	else if (objType == IS_PATCH)
	{
		SubPatchHitList hitList;

		if (!((MeshTopoData*)mc->localData)->GetPatch()) return;

		PatchMesh &patch = *((MeshTopoData*)mc->localData)->GetPatch();
		//				patch.patchSel = ((MeshTopoData*)mc->localData)->faceSel;

		//				if (ignoreBackFaceCull)
		int res = patch.SubObjectHitTest(gw, gw->getMaterial(), &hr,
			flags|SUBHIT_PATCH_PATCHES|SUBHIT_SELSOLID|SUBHIT_PATCH_IGNORE_BACKFACING, hitList);
		//				else res = patch.SubObjectHitTest(gw, gw->getMaterial(), &hr,
		//					flags|SUBHIT_PATCH_PATCHES|SUBHIT_SELSOLID, hitList);

		PatchSubHitRec *rec = hitList.First();
		while (rec) {
			int hitface = rec->index;
			if (hitface < visibleFaces.GetSize())
			visibleFaces.Set(hitface,true);
			rec = rec->Next();
		}

	}
	else if (objType == IS_MNMESH)
	{
		SubObjHitList hitList;
		MeshSubHitRec *rec;	

		if (!((MeshTopoData*)mc->localData)->GetMNMesh()) return;

		MNMesh &mesh = *((MeshTopoData*)mc->localData)->GetMNMesh();
		//				mesh.FaceSelect(((MeshTopoData*)mc->localData)->faceSel);
		//			mesh.faceSel = ((MeshTopoData*)mc->localData)->faceSel;
		int res = mesh.SubObjectHitTest(gw, gw->getMaterial(), &hr,
			flags|SUBHIT_MNFACES|SUBHIT_SELSOLID, hitList);

		rec = hitList.First();
		while (rec) {
			int hitface = rec->index;
			if (hitface < visibleFaces.GetSize())
			visibleFaces.Set(hitface,true);
			rec = rec->Next();
		}

	}

}

int UnwrapMod::Display(
					   TimeValue t, INode* inode, ViewExp *vpt, int flags, 
					   ModContext *mc) 
{	

	if (mc->localData == NULL) return 1;

	if (ip && mUpdateStackUI)
	{
		mUpdateStackUI = FALSE;
		NotifyDependents(FOREVER,PART_ALL,REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
	}

	int iret = 0;
	Matrix3 modmat, ntm = inode->GetObjectTM(t);

 	GraphicsWindow *gw = vpt->getGW();
	Matrix3 viewTM;
	vpt->GetAffineTM(viewTM);
 	Point3 nodeCenter = ntm.GetRow(3);
   	nodeCenter = nodeCenter ;

   	float thickLineSize = vpt->GetVPWorldWidth(nodeCenter)/gw->getWinSizeX() *0.5f;
	Point3 sizeVec(thickLineSize,thickLineSize,thickLineSize);
	sizeVec = VectorTransform(Inverse(ntm),sizeVec);
	thickLineSize = Length(sizeVec);

//	DebugPrint("nodeCenter %f %f %f size %f\n",nodeCenter.x,nodeCenter.y,nodeCenter.z,thickLineSize);

	if  (ip && (ip->GetSubObjectLevel() == 1) )
	{
		
		COLORREF vertColor = ColorMan()->GetColor(GEOMVERTCOLORID);
		int limits = gw->getRndLimits();

		Matrix3 vtm(1);
		Interval iv;
		if (inode) 
			vtm = inode->GetObjectTM(t,&iv);



		

		gw->setTransform(vtm);	

		//			gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		gw->setRndLimits(gw->getRndLimits() |  GW_BACKCULL);

		BitArray visibleFaces;
//		BuildVisibleFaces(gw, mc,visibleFaces);
		visibleFaces.SetSize(TVMaps.f.Count());
		visibleFaces.SetAll();


		gw->setRndLimits(gw->getRndLimits() |  GW_TWO_SIDED);
		gw->setRndLimits(gw->getRndLimits() |  GW_FLAT);
		gw->setRndLimits(gw->getRndLimits() &  ~GW_WIREFRAME);
		gw->setRndLimits(gw->getRndLimits() |  GW_ILLUM);


		Color c(vertColor);
		gw->setColor(LINE_COLOR,c);

		Material m;
		m.Kd = c;
		m.dblSided = 1;
		m.opacity = 1.0f;
		m.selfIllum = 1.0f;
		gw->setMaterial(m) ;





		
		gw->startMarkers();

		//draw are open edge seams
		//loop through the edges looking for ones with only one selection
		//or are sleected and have only one face

		BOOL useDot = getUseVertexDots();
		int mType = getVertexDotType();

		for (int i = 0; i < TVMaps.geomPoints.Count(); i++)
		{
			if ((i < gvsel.GetSize()) && gvsel[i])
			{
				Point3 p  = TVMaps.geomPoints[i];
				if (useDot)
					gw->marker(&p,VERTEX_DOT_MARKER(mType));								
				else gw->marker(&p,PLUS_SIGN_MRKR);								
			}

		}

		gw->endMarkers();

		iret =  1;
		gw->setRndLimits(limits);
	}
	if  (ip && (ip->GetSubObjectLevel() == 2) )
	{
		
		COLORREF egdeColor = ColorMan()->GetColor(GEOMEDGECOLORID);
		int limits = gw->getRndLimits();

		Matrix3 vtm(1);
		Interval iv;
		if (inode) 
			vtm = inode->GetObjectTM(t,&iv);



		

		gw->setTransform(vtm);	

		//			gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		gw->setRndLimits(gw->getRndLimits() |  GW_BACKCULL);

		BitArray visibleFaces;
//		BuildVisibleFaces(gw, mc,visibleFaces);
		visibleFaces.SetSize(TVMaps.f.Count());
		visibleFaces.SetAll();


		gw->setRndLimits(gw->getRndLimits() |  GW_TWO_SIDED);
		gw->setRndLimits(gw->getRndLimits() |  GW_FLAT);
		gw->setRndLimits(gw->getRndLimits() &  ~GW_WIREFRAME);
		gw->setRndLimits(gw->getRndLimits() |  GW_ILLUM);


		Color c(egdeColor);
		gw->setColor(LINE_COLOR,c);

		Material m;
		m.Kd = c;
		m.dblSided = 1;
		m.opacity = 1.0f;
		m.selfIllum = 1.0f;
		gw->setMaterial(m) ;


		float size = 0.5f;
		Box3 b;
		b.Init();
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			int e = TVMaps.f[i]->count;
			for (int j =0; j < e; j++)
			{
				int index = TVMaps.f[i]->v[j];
				b+= TVMaps.geomPoints[index];

			}
		}
		size = thickLineSize;//Length(b.Max()-b.Min())/1500.0f;



		if (thickOpenEdges)
			gw->startTriangles();
		else gw->startSegments();

		//draw are open edge seams
		//loop through the edges looking for ones with only one selection
		//or are sleected and have only one face
		Point3 plist[3];

		for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
		{
			if ((i < gesel.GetSize()) && gesel[i])
			{
				
				BOOL drawEdge = TRUE;



				if (drawEdge)
				{
					int ga = TVMaps.gePtrList[i]->a;
					int gb = TVMaps.gePtrList[i]->b;
					plist[0] = TVMaps.geomPoints[gb];
					plist[1] = TVMaps.geomPoints[ga];

					if (TVMaps.f[ TVMaps.gePtrList[i]->faceList[0]]->vecs == NULL)
					{
						if (thickOpenEdges)
							Draw3dEdge(gw,size, plist[0], plist[1], c);
						else gw->segment(plist,1);
					}
					else
					{
						int va = TVMaps.gePtrList[i]->avec;
						int vb = TVMaps.gePtrList[i]->bvec;
						Point3 avec,bvec;
						avec = TVMaps.geomPoints[vb];
						bvec = TVMaps.geomPoints[va];
						Spline3D sp;
						SplineKnot ka(KTYPE_BEZIER_CORNER,LTYPE_CURVE,plist[0],avec,avec);
						SplineKnot kb(KTYPE_BEZIER_CORNER,LTYPE_CURVE,plist[1],bvec,bvec);
						sp.NewSpline();
						sp.AddKnot(ka);
						sp.AddKnot(kb);
						sp.SetClosed(0);
						sp.InvalidateGeomCache();
						Point3 ip1,ip2;

						//										Draw3dEdge(gw,size, plist[0], plist[1], c);
						for (int k = 0; k < 8; k++)
						{
							float per = k/7.0f;
							ip1 = sp.InterpBezier3D(0, per);
							if ( k > 0)
							{
								if (thickOpenEdges)
									Draw3dEdge(gw,size, ip1, ip2, c);
								else
								{
									plist[0] = ip1;
									plist[1] = ip2;
									gw->segment(plist,1);
								}
							}
							ip2 = ip1;

						}
					}
				}
			}

		}
		//draw our user defined seams
		//loop through our faces
		//
		if (thickOpenEdges)
			gw->endTriangles();
		else gw->endSegments();

		iret =  1;
		gw->setRndLimits(limits);
	}

	if ( (ip && (ip->GetSubObjectLevel() == 3) && (fnGetMapMode() != NOMAP)) )
	{
		DrawGizmo(t,inode,mc,gw);
		//		DoIcon(lp, ip&&ip->GetSubObjectLevel()==1);
		iret = 1;
		
	}
	else if ( (ip && (ip->GetSubObjectLevel() == 3) && (GetQMapPreview()) ))
	{
		ComputeSelectedFaceData();

		Matrix3 vtm(1);
		Interval iv;
		if (inode) 
			vtm = inode->GetObjectTM(t,&iv);

		Point3 off;
		off = vtm.GetTrans();
		Matrix3 tm;
		UnwrapMatrixFromNormal(gNormal,tm);

		Point3 a(-0.5f,-0.5f,0.0f),b(0.5f,-0.5f,0.0f),c(0.5f,0.5f,0.0f),d(-0.5f,0.5f,0.0f);
		
		Point3 xvec,yvec;
		xvec = tm.GetRow(0)*(gXScale *(1.0f+mapScale));//1.5f);		
		yvec = tm.GetRow(1)*(gYScale *(1.0f+mapScale));//1.5f);		
		a = -xvec+yvec;
		b = xvec+yvec;
		c = xvec-yvec;
		d = -xvec-yvec;

		GraphicsWindow *gw = vpt->getGW();
		Matrix3 modmat, ntm = inode->GetObjectTM(t);

//		DrawLineProc lp(gw);

//		modmat = CompMatrix(t,mc,&ntm);	
		gw->setTransform(vtm);	
		Point3 line[5];
		line[0] = gCenter + a;
		line[1] = gCenter + b;
		line[2] = gCenter + c;
		line[3] = gCenter + d;
		line[4] = line[0];
		gw->setColor(LINE_COLOR,GetUIColor(COLOR_SEL_GIZMOS));
		gw->polyline(5, line, NULL, NULL, 0);
		iret = 1;

		

	}


	
	//PELT
	if ( ((peltData.GetPeltMapMode()) && (!(vpt->getGW()->getRndLimits() & GW_BOX_MODE))) ||
		fnGetAlwayShowPeltSeams())
	{

		COLORREF seamColor = ColorMan()->GetColor(PELTSEAMCOLORID);

		
		int limits = gw->getRndLimits();

		Matrix3 vtm(1);
		Interval iv;
		if (inode) 
			vtm = inode->GetObjectTM(t,&iv);



		Matrix3 modmat, ntm = inode->GetObjectTM(t);

		gw->setTransform(vtm);	

		//			gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		gw->setRndLimits(gw->getRndLimits() |  GW_BACKCULL);

		BitArray visibleFaces;
//		BuildVisibleFaces(gw, mc,visibleFaces);
		visibleFaces.SetSize(TVMaps.f.Count());
		visibleFaces.SetAll();


		gw->setRndLimits(gw->getRndLimits() |  GW_TWO_SIDED);
		gw->setRndLimits(gw->getRndLimits() |  GW_FLAT);
		gw->setRndLimits(gw->getRndLimits() &  ~GW_WIREFRAME);
		gw->setRndLimits(gw->getRndLimits() |  GW_ILLUM);


		Color c(seamColor);
		gw->setColor(LINE_COLOR,c);

		Material m;
		m.Kd = c;
		m.dblSided = 1;
		m.opacity = 1.0f;
		m.selfIllum = 1.0f;
		gw->setMaterial(m) ;


		float size = 0.5f;

		size = thickLineSize;

		Tab<int> uvToGeom;
		uvToGeom.SetCount(TVMaps.v.Count());
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			int deg = TVMaps.f[i]->count;
			for (int j = 0; j < deg; j++)
			{
				int tvA = TVMaps.f[i]->t[j];
				int geoA = TVMaps.f[i]->v[j];
				uvToGeom[tvA] = geoA;
			}
		}

		for (int i = 0; i < peltData.tempPoints.GetSize(); i++)
		{
			if (peltData.tempPoints[i])
			{
				Point3 p = TVMaps.geomPoints[i];
				gw->marker(&p,HOLLOW_BOX_MRKR);
			}
		}
		if (thickOpenEdges)
			gw->startTriangles();
		else gw->startSegments();

		//draw are open edge seams
		//loop through the edges looking for ones with only one selection
		//or are sleected and have only one face
		Point3 plist[3];

		for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
		{
			int fct = TVMaps.gePtrList[i]->faceList.Count();
			BOOL drawEdge = FALSE;
			if (TVMaps.gePtrList[i]->faceList.Count() == 1)
			{
				int faceIndex = TVMaps.gePtrList[i]->faceList[0];
				if (visibleFaces[faceIndex] && (TVMaps.f[faceIndex]->flags & FLAG_SELECTED))
					drawEdge = TRUE;
			}
			else if ((i < peltData.seamEdges.GetSize()) && peltData.seamEdges[i])
			{
				drawEdge = TRUE;
			}
			else
			{
				int numberSelected = 0;
				int ct = TVMaps.gePtrList[i]->faceList.Count();
				BOOL visible = FALSE;
				for (int j = 0; j < ct; j++)
				{
					int faceIndex = TVMaps.gePtrList[i]->faceList[j];
					//draw this edge
					if (visibleFaces[faceIndex])
						visible = TRUE;
					if (TVMaps.f[faceIndex]->flags & FLAG_SELECTED)
					{
						numberSelected++;
					}
				}
				if ((numberSelected==1) && (visible))
				{
					if (peltData.GetPeltMapMode())
						drawEdge = TRUE;
				}
			}

			if (drawEdge)
			{
				int ga = TVMaps.gePtrList[i]->a;
				int gb = TVMaps.gePtrList[i]->b;
				plist[0] = TVMaps.geomPoints[gb];
				plist[1] = TVMaps.geomPoints[ga];

				if (TVMaps.f[ TVMaps.gePtrList[i]->faceList[0]]->vecs == NULL)
				{
					if (thickOpenEdges)
						Draw3dEdge(gw,size, plist[0], plist[1], c);
					else gw->segment(plist,1);
				}
				else
				{
					int va = TVMaps.gePtrList[i]->avec;
					int vb = TVMaps.gePtrList[i]->bvec;
					Point3 avec,bvec;
					avec = TVMaps.geomPoints[vb];
					bvec = TVMaps.geomPoints[va];
					Spline3D sp;
					SplineKnot ka(KTYPE_BEZIER_CORNER,LTYPE_CURVE,plist[0],avec,avec);
					SplineKnot kb(KTYPE_BEZIER_CORNER,LTYPE_CURVE,plist[1],bvec,bvec);
					sp.NewSpline();
					sp.AddKnot(ka);
					sp.AddKnot(kb);
					sp.SetClosed(0);
					sp.InvalidateGeomCache();
					Point3 ip1,ip2;

					//										Draw3dEdge(gw,size, plist[0], plist[1], c);
					for (int k = 0; k < 8; k++)
					{
						float per = k/7.0f;
						ip1 = sp.InterpBezier3D(0, per);
						if ( k > 0)
						{
							if (thickOpenEdges)
								Draw3dEdge(gw,size, ip1, ip2, c);
							else
							{
								plist[0] = ip1;
								plist[1] = ip2;
								gw->segment(plist,1);
							}
						}
						ip2 = ip1;

					}
				}
			}

		}
		//draw our user defined seams
		//loop through our faces
		//
		if (thickOpenEdges)
			gw->endTriangles();
		else gw->endSegments();

		iret =  1;
		gw->setRndLimits(limits);

	}
	if ((viewportOpenEdges) && (!(vpt->getGW()->getRndLimits() & GW_BOX_MODE)))
	{
		
		int limits = gw->getRndLimits();

		Matrix3 vtm(1);
		Interval iv;
		if (inode) 
			vtm = inode->GetObjectTM(t,&iv);



		Matrix3 modmat, ntm = inode->GetObjectTM(t);

		gw->setTransform(vtm);	

		//			gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		gw->setRndLimits(gw->getRndLimits() |  GW_BACKCULL);

		BitArray visibleFaces;
		BuildVisibleFaces(gw, mc,visibleFaces);


		gw->setRndLimits(gw->getRndLimits() |  GW_TWO_SIDED);
		gw->setRndLimits(gw->getRndLimits() |  GW_FLAT);
		gw->setRndLimits(gw->getRndLimits() &  ~GW_WIREFRAME);
		gw->setRndLimits(gw->getRndLimits() |  GW_ILLUM);


		Color c(openEdgeColor);
		gw->setColor(LINE_COLOR,c);

		Material m;
		m.Kd = c;
		m.dblSided = 1;
		m.opacity = 1.0f;
		m.selfIllum = 1.0f;
		gw->setMaterial(m) ;


		float size = 0.5f;

		size = thickLineSize;

		if (thickOpenEdges)
			gw->startTriangles();
		else gw->startSegments();
		for (int i=0; i<TVMaps.ePtrList.Count(); i++) 
		{
			int ea = TVMaps.ePtrList[i]->a;
			int eb = TVMaps.ePtrList[i]->b;
			//draw open edges
			if (TVMaps.ePtrList[i]->faceList.Count() == 1)
			{
				int faceIndex = TVMaps.ePtrList[i]->faceList[0];
				if ((visibleFaces[faceIndex]) && (faceIndex >= 0) && (faceIndex<TVMaps.f.Count()))
				{
					int eCount = TVMaps.f[faceIndex]->count;
					for (int j = 0; j < eCount; j++)
					{
						int a = TVMaps.f[faceIndex]->t[j];
						int b;
						if (j == 0)
							b = TVMaps.f[faceIndex]->t[eCount-1];
						else b = TVMaps.f[faceIndex]->t[j-1];

						if ( ((ea == a) && (eb == b)) ||
							((ea == b) && (eb == a)) )
						{
							int ga,gb;
							ga = TVMaps.f[faceIndex]->v[j];
							gb;
							if (j == 0)
								gb = TVMaps.f[faceIndex]->v[eCount-1];
							else gb = TVMaps.f[faceIndex]->v[j-1];
							Point3 plist[3];

							//								if ((ea == a) && (eb == b)) 
							{
								plist[0] = TVMaps.geomPoints[gb];
								plist[1] = TVMaps.geomPoints[ga];
								if (TVMaps.f[faceIndex]->vecs == NULL)
								{
									if (thickOpenEdges)
										Draw3dEdge(gw,size, plist[0], plist[1], c);
									else gw->segment(plist,1);
								}
								else
								{
									int va,vb;


									if (j == 0)
									{
										va = TVMaps.f[faceIndex]->vecs->vhandles[eCount*2-1];
										vb = TVMaps.f[faceIndex]->vecs->vhandles[eCount*2-2];
									}
									else
									{
										va = TVMaps.f[faceIndex]->vecs->vhandles[j*2-1];
										vb = TVMaps.f[faceIndex]->vecs->vhandles[j*2-2];
									}

									Point3 avec,bvec;
									avec = TVMaps.geomPoints[vb];
									bvec = TVMaps.geomPoints[va];
									Spline3D sp;
									SplineKnot ka(KTYPE_BEZIER_CORNER,LTYPE_CURVE,plist[0],avec,avec);
									SplineKnot kb(KTYPE_BEZIER_CORNER,LTYPE_CURVE,plist[1],bvec,bvec);
									sp.NewSpline();
									sp.AddKnot(ka);
									sp.AddKnot(kb);
									sp.SetClosed(0);
									sp.InvalidateGeomCache();
									Point3 ip1,ip2;
									//										Draw3dEdge(gw,size, plist[0], plist[1], c);
									for (int k = 0; k < 8; k++)
									{
										float per = k/7.0f;
										ip1 = sp.InterpBezier3D(0, per);
										if ( k > 0)
										{
											if (thickOpenEdges)
												Draw3dEdge(gw,size, ip1, ip2, c);
											else gw->segment(plist,1);
										}
										ip2 = ip1;
									}
								}

							}

						}

					}

				}
			}
		}
		if (thickOpenEdges)
			gw->endTriangles();
		else gw->endSegments();
		iret =  1;
		gw->setRndLimits(limits);
	}


	return iret;	
}



void UnwrapMod::GetWorldBoundBox(
								 TimeValue t,INode* inode, ViewExp *vpt, Box3& box, 
								 ModContext *mc) 
{
	gizmoBounds.Init();
	if ( (ip && (ip->GetSubObjectLevel() == 3) && (fnGetMapMode() != NOMAP) ))
	{
		Matrix3 ntm = inode->GetObjectTM(t), modmat;
 		modmat = CompMatrix(t,mc,&ntm);
		Box3 bounds;
		bounds.Init();
		bounds += Point3(1.0f,1.0f,1.0f);
		bounds += Point3(-1.0f,-1.0f,-1.0f);
		box.Init();
		for (int i = 0; i < 8; i++)
		{
			box += bounds[i] * modmat;
		}
		

//		cb->TM(modmat,0);
	}
	else if ( (ip && (ip->GetSubObjectLevel() > 0) )&& (gfaces.Count() > 0))
	{
		ComputeSelectedFaceData();

		Matrix3 tm;
		UnwrapMatrixFromNormal(gNormal,tm);

		Point3 a(-0.5f,-0.5f,0.0f),b(0.5f,-0.5f,0.0f),c(0.5f,0.5f,0.0f),d(-0.5f,0.5f,0.0f);

		Point3 xvec,yvec;
		xvec = tm.GetRow(0)*(gXScale * 1.5f);		
		yvec = tm.GetRow(1)*(gYScale * 1.5f);		
		a = -xvec+yvec;
		b = xvec+yvec;
		c = xvec-yvec;
		d = -xvec-yvec;

		Matrix3 modmat = inode->GetObjTMBeforeWSM(t);	

		Point3 line[5];
		line[0] = (gCenter + a)*modmat;
		line[1] = (gCenter + b)*modmat;
		line[2] = (gCenter + c)*modmat;
		line[3] = (gCenter + d)*modmat;
		line[4] = line[0];
		box.Init();

		box += line[0];
		box += line[1];
		box += line[2];
		box += line[3];
		gizmoBounds = box;
	}

}

void UnwrapMod::GetSubObjectCenters(
									SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc)
{	
	if ( (ip && (ip->GetSubObjectLevel() == 3) && (fnGetMapMode() != NOMAP) ))
	{
		Matrix3 ntm = node->GetObjectTM(t), modmat;
		modmat = CompMatrix(t,mc,&ntm);
		cb->Center(modmat.GetTrans(),0);
	}
	else
	{
		Matrix3 tm = node->GetObjectTM(t);	

		int ct = 0;
		Box3 box;
		if (ip && (ip->GetSubObjectLevel() == 1))
		{
			for (int i=0; i<TVMaps.v.Count(); i++) 
			{
				if ((i < gvsel.GetSize()) && gvsel[i])
				{
					box += TVMaps.geomPoints[i] * tm;
					ct++;
				}
			}
		}
		else if (ip && (ip->GetSubObjectLevel() == 2))
		{
			for (int i=0; i<TVMaps.gePtrList.Count(); i++) 
			{
				if ((i < gesel.GetSize()) && gesel[i])
				{
					int a = TVMaps.gePtrList[i]->a;
					box += TVMaps.geomPoints[a] * tm;
					a = TVMaps.gePtrList[i]->b;
					box += TVMaps.geomPoints[a] * tm;
					ct++;
				}
			}
		}
		else if (ip && (ip->GetSubObjectLevel() == 3))
		{
			for (int i=0; i<TVMaps.f.Count(); i++) 
			{
				if (TVMaps.f[i]->flags & FLAG_SELECTED)
				{
					for (int j=0; j<TVMaps.f[i]->count; j++) 
					{
						int id = TVMaps.f[i]->v[j];
						box += TVMaps.geomPoints[id] * tm;
						ct++;
					}
				}
			}
		}

		if (ct ==0) return;

		cb->Center (box.Center(),0);
	}

}

void UnwrapMod::GetSubObjectTMs(
								SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc)
{
	if ( (ip && (ip->GetSubObjectLevel() == 3)  && (fnGetMapMode() != NOMAP) ))
	{
		Matrix3 ntm = node->GetObjectTM(t), modmat;
		modmat = CompMatrix(t,mc,&ntm);
		cb->TM(modmat,0);
	}
}



int UnwrapMod::HitTest (TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc) {
	Interval valid;
	int savedLimits, res = 0;
	GraphicsWindow *gw = vpt->getGW();
	HitRegion hr;


	// Face slection and Face Map selection
//	if ((peltData.GetPeltMapMode()) && (peltData.GetPointToPointSeamsMode()))
	if (FALSE)

	{
		MakeHitRegion(hr,type, crossing,4,p);
		gw->setHitRegion(&hr);
		Matrix3 mat = inode->GetObjectTM(t);
		gw->setTransform(mat);	
		gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);

		if (oldSelMethod)
		{
			if (type == HITTYPE_POINT)
				gw->setRndLimits(gw->getRndLimits() & GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}
		else
		{
			if (ignoreBackFaceCull) gw->setRndLimits(gw->getRndLimits() |  GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}

		res = peltData.HitTestPointToPointMode(this,vpt,gw,p,hr,inode,mc);
	}
	else if (/*(peltData.GetPeltMapMode()) && */ (peltData.GetEditSeamsMode()))
	{
		MakeHitRegion(hr,type, crossing,4,p);
		gw->setHitRegion(&hr);
		Matrix3 mat = inode->GetObjectTM(t);
		gw->setTransform(mat);	
		gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);

		if (oldSelMethod)
		{
			if (type == HITTYPE_POINT)
				gw->setRndLimits(gw->getRndLimits() & GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}
		else
		{
			if (ignoreBackFaceCull) gw->setRndLimits(gw->getRndLimits() |  GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}

		res = peltData.HitTestEdgeMode(this, vpt,gw,p,hr,inode,mc, flags,type);
		gw->setRndLimits(savedLimits);	
		return res;	
	}
 	else if (ip && (ip->GetSubObjectLevel() ==1) )
	{

		MakeHitRegion(hr,type, crossing,4,p);
		gw->setHitRegion(&hr);
		Matrix3 mat = inode->GetObjectTM(t);
		gw->setTransform(mat);	
		gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);

		if (oldSelMethod)
		{
			if (type == HITTYPE_POINT)
				gw->setRndLimits(gw->getRndLimits() & GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}
		else
		{
			if (ignoreBackFaceCull) gw->setRndLimits(gw->getRndLimits() |  GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}

		gw->clearHitCode();

		BOOL add = GetKeyState(VK_CONTROL)<0;
		BOOL sub = GetKeyState(VK_MENU)<0;
		BOOL polySelect = !(GetKeyState(VK_SHIFT)<0);

		if (add)
			ip->ReplacePrompt( GetString(IDS_PW_MOUSE_ADD));
		else if (sub)
			ip->ReplacePrompt( GetString(IDS_PW_MOUSE_SUBTRACT));
		else if (!polySelect)
			ip->ReplacePrompt( GetString(IDS_PW_MOUSE_SELECTTRI));
		else ip->ReplacePrompt( GetString(IDS_PW_MOUSE_SELECTFACE));

		//build our visible face
		BitArray visibleFaces;
		BuildVisibleFaces(gw, mc, visibleFaces);

		Tab<UVWHitData> hitVerts;
		HitGeomVertData(hitVerts,gw,  hr);
		
		if (gvsel.GetSize() != TVMaps.geomPoints.Count())
		{
			gvsel.SetSize(TVMaps.geomPoints.Count());
			gvsel.ClearAll();
		}

		BitArray visibleVerts;
		visibleVerts.SetSize(TVMaps.geomPoints.Count());


		if (fnGetBackFaceCull())
		{
			visibleVerts.ClearAll();
			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				if (visibleFaces[i])
				{
					int deg = TVMaps.f[i]->count;
					for (int j = 0; j < deg; j++)
					{
						visibleVerts.Set(TVMaps.f[i]->v[j],TRUE);
					}
				}
			}
		}
		else visibleVerts.SetAll();
		for (int hi = 0; hi < hitVerts.Count(); hi++)
		{
			int i = hitVerts[hi].index;
			//						if (hitEdges[i])
			{
				

				BOOL selOnly = flags&HIT_SELONLY;
				BOOL unselOnly = flags&HIT_UNSELONLY;

				if (fnGetBackFaceCull())
				{
					if (visibleVerts[i])
					{
						if ( (gvsel[i] && (flags&HIT_SELONLY)) ||
							!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
							vpt->LogHit(inode,mc,0.0f,i,NULL);
						else if ( (!gvsel[i] && (flags&HIT_UNSELONLY)) ||
							!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
							vpt->LogHit(inode,mc,0.0f,i,NULL);

					}
				}
				else
				{

						if ( (gvsel[i] && (flags&HIT_SELONLY)) ||
							!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
							vpt->LogHit(inode,mc,0.0f,i,NULL);
						else if ( (!gvsel[i] && (flags&HIT_UNSELONLY)) ||
							!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
							vpt->LogHit(inode,mc,0.0f,i,NULL);

				}
			}
		}

	}

	if (ip && (ip->GetSubObjectLevel() ==2) )
	{

		MakeHitRegion(hr,type, crossing,4,p);
		gw->setHitRegion(&hr);
		Matrix3 mat = inode->GetObjectTM(t);
		gw->setTransform(mat);	
		gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);

		if (oldSelMethod)
		{
			if (type == HITTYPE_POINT)
				gw->setRndLimits(gw->getRndLimits() & GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}
		else
		{
			if (ignoreBackFaceCull) gw->setRndLimits(gw->getRndLimits() |  GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}

		gw->clearHitCode();

		BOOL add = GetKeyState(VK_CONTROL)<0;
		BOOL sub = GetKeyState(VK_MENU)<0;
		BOOL polySelect = !(GetKeyState(VK_SHIFT)<0);

		if (add)
			ip->ReplacePrompt( GetString(IDS_PW_MOUSE_ADD));
		else if (sub)
			ip->ReplacePrompt( GetString(IDS_PW_MOUSE_SUBTRACT));
		else if (!polySelect)
			ip->ReplacePrompt( GetString(IDS_PW_MOUSE_SELECTTRI));
		else ip->ReplacePrompt( GetString(IDS_PW_MOUSE_SELECTFACE));

		//build our visible face


 		if (peltData.GetPointToPointSeamsMode())
		{
			res = peltData.HitTestPointToPointMode(this,vpt,gw,p,hr,inode,mc);
		}
		else
		{
			BitArray visibleFaces;

			BuildVisibleFaces(gw, mc, visibleFaces);

			//hit test our geom edges
			Tab<UVWHitData> hitEdges;
  			HitGeomEdgeData(hitEdges,gw,  hr);		
			res = hitEdges.Count();
			if (type == HITTYPE_POINT)
			{
 				int closestIndex = -1;
				float closest=  0.0f;
				Matrix3 toView(1);
 				vpt->GetAffineTM(toView);
 				toView = mat * toView;

 //DebugPrint(" ct %d\n",hitEdges.Count());				
 				for (int hi = 0; hi < hitEdges.Count(); hi++)
				{
					int eindex = hitEdges[hi].index;
					int a = TVMaps.gePtrList[eindex]->a;
					
					Point3 p = TVMaps.geomPoints[a] * toView;
//DebugPrint(" eindex %d z %f\n",eindex,p.z);
					if ((p.z > closest) || (closestIndex==-1))
					{
						closest = p.z ;
						closestIndex = hi;
					}
				}
 				if (closestIndex != -1)
				{
//DebugPrint(" closestIndex %d\n",hitEdges[closestIndex]);	
					Tab<UVWHitData> tempHitEdge;
					tempHitEdge.Append(1,&hitEdges[closestIndex],1);
					hitEdges = tempHitEdge;
				}
			}

			

			if (gesel.GetSize() != TVMaps.gePtrList.Count())
			{
				gesel.SetSize(TVMaps.gePtrList.Count());
				gesel.ClearAll();
			}
			for (int hi = 0; hi < hitEdges.Count(); hi++)
			{
				int i = hitEdges[hi].index;
				//						if (hitEdges[i])
				{
					BOOL visibleFace = FALSE;
					BOOL selFace = FALSE;
					int ct = TVMaps.gePtrList[i]->faceList.Count();
					for (int j = 0; j < ct; j++)
					{
						int faceIndex =TVMaps.gePtrList[i]->faceList[j];
						if ((faceIndex < TVMaps.f.Count())/* && (peltData.peltFaces[faceIndex])*/)
							selFace = TRUE;
						if ((faceIndex < visibleFaces.GetSize()) && (visibleFaces[faceIndex]))
							visibleFace = TRUE;

					}
					BOOL selOnly = flags&HIT_SELONLY;
					BOOL unselOnly = flags&HIT_UNSELONLY;

					if (fnGetBackFaceCull())
					{
						if (selFace && visibleFace)
						{
							if ( (gesel[i] && (flags&HIT_SELONLY)) ||
								!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
								vpt->LogHit(inode,mc,0.0f,i,NULL);
							else if ( (!gesel[i] && (flags&HIT_UNSELONLY)) ||
								!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
								vpt->LogHit(inode,mc,0.0f,i,NULL);

						}
					}
					else
					{
						if (selFace )
						{
							if ( (gesel[i] && (flags&HIT_SELONLY)) ||
								!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
								vpt->LogHit(inode,mc,0.0f,i,NULL);
							else if ( (!gesel[i] && (flags&HIT_UNSELONLY)) ||
								!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
								vpt->LogHit(inode,mc,0.0f,i,NULL);

						}
					}
				}
			}
		}


	}

	else if (ip && (ip->GetSubObjectLevel() == 3) )
	{
		MakeHitRegion(hr,type, crossing,4,p);
		gw->setHitRegion(&hr);
		Matrix3 mat = inode->GetObjectTM(t);
		gw->setTransform(mat);	
		gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);

		if (oldSelMethod)
		{
			if (type == HITTYPE_POINT)
				gw->setRndLimits(gw->getRndLimits() & GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}
		else
		{
			if (ignoreBackFaceCull) gw->setRndLimits(gw->getRndLimits() |  GW_BACKCULL);
			else gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
		}

		gw->clearHitCode();

		BOOL add = GetKeyState(VK_CONTROL)<0;
		BOOL sub = GetKeyState(VK_MENU)<0;
		BOOL polySelect = !(GetKeyState(VK_SHIFT)<0);

		if (add)
			ip->ReplacePrompt( GetString(IDS_PW_MOUSE_ADD));
		else if (sub)
			ip->ReplacePrompt( GetString(IDS_PW_MOUSE_SUBTRACT));
		else if (!polySelect)
			ip->ReplacePrompt( GetString(IDS_PW_MOUSE_SELECTTRI));
		else ip->ReplacePrompt( GetString(IDS_PW_MOUSE_SELECTFACE));


		if ( fnGetMapMode() != NOMAP)
		{
			if ( peltData.GetPeltMapMode() 
				&& (peltData.GetPointToPointSeamsMode() || peltData.GetEditSeamsMode())
				)
			{
			}
			else
			{
				DrawGizmo(t, inode,mc, gw);
				if (gw->checkHitCode()) 
				{
					vpt->LogHit(inode, mc, gw->getHitDistance(), 0, NULL); 
					res = 1;
				}
			}
		}
		else if (objType == IS_MESH)
		{
			SubObjHitList hitList;
			MeshSubHitRec *rec;	

			if (!((MeshTopoData*)mc->localData)->GetMesh()) return 0;

			Mesh &mesh = *((MeshTopoData*)mc->localData)->GetMesh();
			mesh.faceSel = ((MeshTopoData*)mc->localData)->faceSel;

			//PELT
/*
			if ((peltData.GetPeltMapMode()) && (peltData.GetPointToPointSeamsMode()))
			{

				res = peltData.HitTestPointToPointMode(this,vpt,gw,p,hr,inode,mc);
			}
			// we want to pick edges
			else if ((peltData.GetPeltMapMode()) && (peltData.GetEditSeamsMode()))
			{
				res = peltData.HitTestEdgeMode(this, vpt,gw,p,hr,inode,mc, flags);
			}
			else 
*/
			if  (fnGetMapMode() == NOMAP)
			{
				res = mesh.SubObjectHitTest(gw, gw->getMaterial(), &hr,
					flags|SUBHIT_FACES|SUBHIT_SELSOLID, hitList);

				rec = hitList.First();
				while (rec) {
					vpt->LogHit(inode,mc,rec->dist,rec->index,NULL);
					rec = rec->Next();
				}
			}

		}
		else if (objType == IS_PATCH)
		{
			SubPatchHitList hitList;

			if (!((MeshTopoData*)mc->localData)->GetPatch()) return 0;

			PatchMesh &patch = *((MeshTopoData*)mc->localData)->GetPatch();
			patch.patchSel = ((MeshTopoData*)mc->localData)->faceSel;

/*			if ((peltData.GetPeltMapMode()) && (peltData.GetPointToPointSeamsMode()))
			{

				res = peltData.HitTestPointToPointMode(this,vpt,gw,p,hr,inode,mc);
			}
			// we want to pick edges
			else if ((peltData.GetPeltMapMode()) && (peltData.GetEditSeamsMode()))
			{
				res = peltData.HitTestEdgeMode(this, vpt,gw,p,hr,inode,mc, flags);
			}
			else */
			if  (fnGetMapMode() == NOMAP)
			{
				if (ignoreBackFaceCull)
					res = patch.SubObjectHitTest(gw, gw->getMaterial(), &hr,
					flags|SUBHIT_PATCH_PATCHES|SUBHIT_SELSOLID|SUBHIT_PATCH_IGNORE_BACKFACING, hitList);
				else res = patch.SubObjectHitTest(gw, gw->getMaterial(), &hr,
					flags|SUBHIT_PATCH_PATCHES|SUBHIT_SELSOLID, hitList);

				PatchSubHitRec *rec = hitList.First();
				while (rec) {
					vpt->LogHit(inode,mc,rec->dist,rec->index,NULL);
					rec = rec->Next();
				}
			}

		}
		else if (objType == IS_MNMESH)
		{
			SubObjHitList hitList;
			MeshSubHitRec *rec;	

			if (!((MeshTopoData*)mc->localData)->GetMNMesh()) return 0;

			MNMesh &mesh = *((MeshTopoData*)mc->localData)->GetMNMesh();
			mesh.FaceSelect(((MeshTopoData*)mc->localData)->faceSel);
			//			mesh.faceSel = ((MeshTopoData*)mc->localData)->faceSel;

/*			if ((peltData.GetPeltMapMode()) && (peltData.GetPointToPointSeamsMode()))
			{

				res = peltData.HitTestPointToPointMode(this,vpt,gw,p,hr,inode,mc);
			}
			else if ((peltData.GetPeltMapMode()) && (peltData.GetEditSeamsMode()))
			{
				res = peltData.HitTestEdgeMode(this, vpt,gw,p,hr,inode,mc, flags);
			}
			else */
			if  (fnGetMapMode() == NOMAP)
			{
				res = mesh.SubObjectHitTest(gw, gw->getMaterial(), &hr,
					flags|SUBHIT_MNFACES|SUBHIT_SELSOLID, hitList);

				rec = hitList.First();
				while (rec) {
					vpt->LogHit(inode,mc,rec->dist,rec->index,NULL);
					rec = rec->Next();
				}
			}

		}


	}

	gw->setRndLimits(savedLimits);	
	return res;	
}


void UnwrapMod::DeleteVertsFromFace(Tab<UVW_TVFaceClass*> f)

{
	Tab<int> fcount;
	Tab<int> fcount_original;
	fcount.SetCount(TVMaps.v.Count());
	fcount_original.SetCount(TVMaps.v.Count());

	for (int i = 0; i < fcount.Count(); i++)
	{
		fcount[i] = 0;
		fcount_original[i] = 0;
	}

for (int i = 0; i < f.Count(); i++)
	{
		if (!(TVMaps.f[f[i]->FaceIndex]->flags & FLAG_DEAD))
		{
			int pcount =3;
			//		if (TVMaps.f[f[i].FaceIndex].flags & FLAG_QUAD) pcount = 4;
			pcount = TVMaps.f[f[i]->FaceIndex]->count;

			for (int j = 0; j < pcount; j++)
			{
				int index = TVMaps.f[f[i]->FaceIndex]->t[j];
				fcount[index] += 1;
			}
		}
	}

for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		if (!(TVMaps.f[i]->flags & FLAG_DEAD))
		{
			int pcount =3;
			//		if (TVMaps.f[i].flags & FLAG_QUAD) pcount = 4;
			pcount = TVMaps.f[i]->count;

			for (int j = 0; j < pcount; j++)
			{
				int index = TVMaps.f[i]->t[j];
				fcount_original[index] += 1;
			}

		}
	}
for (int i = 0; i < f.Count(); i++)
	{
		if (!(TVMaps.f[f[i]->FaceIndex]->flags & FLAG_DEAD))
		{
			int pcount =3;
			//		if (TVMaps.f[f[i].FaceIndex].flags & FLAG_QUAD) pcount = 4;
			pcount = TVMaps.f[f[i]->FaceIndex]->count;

			for (int j = 0; j < pcount; j++)
			{
				int index = TVMaps.f[f[i]->FaceIndex]->t[j];
				if (fcount[index] == fcount_original[index])
					TVMaps.v[index].flags = FLAG_DEAD;

			}
		}
	}


}


void UnwrapMod::DeleteVertsFromFace(BitArray f)

{
	Tab<int> fcount;
	Tab<int> fcount_original;
	fcount.SetCount(TVMaps.v.Count());
	fcount_original.SetCount(TVMaps.v.Count());

	for (int i = 0; i < fcount.Count(); i++)
	{
		fcount[i] = 0;
		fcount_original[i] = 0;
	}

for (int i = 0; i < f.GetSize(); i++)
	{
		if (f[i] ==1)
		{
			if (!(TVMaps.f[i]->flags & FLAG_DEAD))
			{
				for (int j = 0; j < 3; j++)
				{
					int index = TVMaps.f[i]->t[j];
					fcount[index] += 1;
				}
			}
		}
	}

for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		if (!(TVMaps.f[i]->flags & FLAG_DEAD))
		{
			for (int j = 0; j < 3; j++)
			{
				int index = TVMaps.f[i]->t[j];
				fcount_original[index] += 1;
			}

		}
	}
for (int i = 0; i < f.GetSize(); i++)
	{
		if (f[i] ==1)
		{
			if (!(TVMaps.f[i]->flags & FLAG_DEAD))
			{
				for (int j = 0; j < 3; j++)
				{
					int index = TVMaps.f[i]->t[j];
					if (fcount[index] == fcount_original[index])
						TVMaps.v[index].flags = FLAG_DEAD;

				}
			}
		}
	}


}


void UnwrapMod::UpdateFaceSelection(BitArray f)
{
	int ct = 0;
	for (int i = 0; i < TVMaps.f.Count();i++)
	{
		if (!(TVMaps.f[i]->flags & FLAG_DEAD))
		{
			if (f[i] == 1)
			{
				TVMaps.f[i]->flags |= FLAG_SELECTED;
				ct++;
			}
			else TVMaps.f[i]->flags &= (~FLAG_SELECTED);
		}
	}
	//if (ct > 0) AlignMap();

}

int UnwrapMod::IsSelected(int index)
{
	if (index < isSelected.GetSize())
		return isSelected[index];
	else return 0;
	int sel = 0;
	for (int i = 0; i < TVMaps.f.Count();i++)
	{
		int pcount = 3;
		//	if (TVMaps.f[i].flags & FLAG_QUAD) pcount = 4;
		pcount = TVMaps.f[i]->count;

		for (int j = 0;  j < pcount;j++)
		{
			if ( (TVMaps.f[i]->t[j] == index) && ((TVMaps.f[i]->flags & FLAG_SELECTED)))
			{
				sel = 1;
				return sel;
			}
		}
	}
	return sel;
}

int UnwrapMod::IsSelectedSetup()
{
	int sel = 0;
	isSelected.SetSize(TVMaps.v.Count());
	isSelected.ClearAll();

	wasSelected = vsel;

	for (int i = 0; i < TVMaps.f.Count();i++)
	{
		int pcount = 3;
		//	if (TVMaps.f[i].flags & FLAG_QUAD) pcount = 4;
		pcount = TVMaps.f[i]->count;
		for (int j = 0;  j < pcount;j++)
		{
			if ( (TVMaps.f[i]->flags & FLAG_SELECTED))
			{
				isSelected.Set(TVMaps.f[i]->t[j]);
				if ((TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[i]->vecs))
				{
					isSelected.Set(TVMaps.f[i]->vecs->handles[j*2]);
					isSelected.Set(TVMaps.f[i]->vecs->handles[j*2+1]);
					if (TVMaps.f[i]->flags & FLAG_INTERIOR)
						isSelected.Set(TVMaps.f[i]->vecs->interiors[j]);
				}
			}

		}
	}
	return 1;
}





void UnwrapMod::SelectSubComponent (HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert) {

	if (TVMaps.v.Count()==0) return;

	MeshTopoData *d = NULL, *od = NULL;

	BitArray set;
	//BitArray tempset;
	AdjFaceList *al = NULL;
	facehit.ZeroCount();
	BOOL add = GetKeyState(VK_CONTROL)<0;
	BOOL sub = GetKeyState(VK_MENU)<0;
	BOOL polySelect = !(GetKeyState(VK_SHIFT)<0);

	ip->ClearCurNamedSelSet();
	MeshTopoData* tmd = NULL;
	//PELT
	if ( FALSE/*(peltData.GetPointToPointSeamsMode())*/)
	{
/*
		//get the hit point

		if (peltData.seamEdges.GetSize() != TVMaps.gePtrList.Count())
		{
			peltData.seamEdges.SetSize(TVMaps.gePtrList.Count());
			peltData.seamEdges.ClearAll();
		}

		int index = hitRec->hitInfo;

		if (index != peltData.currentPointHit)
		{
			//assign the current to previous
			peltData.previousPointHit = peltData.currentPointHit;
			//assign hit point to current
			peltData.currentPointHit = index;
			Point3 vec = peltData.viewZVec;
			//			static int lastPT = -1;
			//			if (add)
			//				peltData.previousPointHit = lastPT;




			//if we have both a current and previous create seam
			if ( ((peltData.previousPointHit!=-1) && (peltData.currentPointHit!=-1)) && 
				((peltData.previousPointHit!=peltData.currentPointHit)) )
			{
				//				HoldSelection();


				if (theHold.Holding()) theHold.Put (new UnwrapPeltSeamRestore (this));
				//				if (theHold.Holding()) theHold.Put (new UnwrapPeltSeamRestore (this));

				Tab<int> newSeamEdges;
				TVMaps.EdgeListFromPoints(newSeamEdges, peltData.previousPointHit, peltData.currentPointHit,vec);
				for (int i = 0; i < newSeamEdges.Count(); i++)
				{
					int edgeIndex = newSeamEdges[i];
					peltData.seamEdges.Set(edgeIndex,TRUE);

				}

				NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
				peltData.previousPointHit = -1;
				peltData.currentPointHit = -1;
				//				lastPT = peltData.currentPointHit;
			}
		}
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltSelectedSeams"), 1, 0,
			mr_bitarray,&(peltData.seamEdges));
*/
		//		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltSelectedSeams"), 1, 0,
		//			mr_bitarray,&(peltData.seamEdges));
	}
	else if (/*(peltData.GetPeltMapMode()) && */(peltData.GetEditSeamsMode()))
	{
		tmd = (MeshTopoData*)hitRec->modContext->localData;
		if (peltData.seamEdges.GetSize() != TVMaps.gePtrList.Count())
		{
			peltData.seamEdges.SetSize(TVMaps.gePtrList.Count());
			peltData.seamEdges.ClearAll();
		}

		if (theHold.Holding()) theHold.Put (new UnwrapPeltSeamRestore (this));

		//		if (objType == IS_MESH)
		{
			while (hitRec) 
			{					
				//				d = (MeshTopoData*)hitRec->modContext->localData;	


				set.ClearAll();
				int index = hitRec->hitInfo;
				if ((index >= 0) && (index < peltData.seamEdges.GetSize()))
				{

					BOOL state = selected;

					if (invert) state = !peltData.seamEdges[hitRec->hitInfo];
					if (state) peltData.seamEdges.Set(hitRec->hitInfo);
					else       peltData.seamEdges.Clear(hitRec->hitInfo);
					if (!all) break;
				}

				hitRec = hitRec->Next();

			}
		}
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltSelectedSeams"), 1, 0,
			mr_bitarray,&(peltData.seamEdges));
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		if (ip) ip->RedrawViews(ip->GetTime());
	}
	else
	{
		if ( (ip && (ip->GetSubObjectLevel() == 1)  ))
		{
			BitArray holdSel = gvsel;
			while (hitRec) 
			{					
				set.ClearAll();

				BOOL state = selected;
				//6-29--99 watje
				if (hitRec->hitInfo < gvsel.GetSize())
				{
					if (invert) 
						state = !gvsel[hitRec->hitInfo];
					if (state) 
						gvsel.Set(hitRec->hitInfo);
					else       
						gvsel.Clear(hitRec->hitInfo);
				}
				
				if (!all) break;

				hitRec = hitRec->Next();
			}
			if (geomElemMode) SelectGeomElement(!sub,&holdSel);

			
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setSelectedGeomVerts"), 1, 0,
			mr_bitarray,&(gvsel));
		}
		else if ( (ip && (ip->GetSubObjectLevel() == 2)  ))
		{

			{
				BitArray holdSel = gesel;
				while (hitRec) 
				{					
					set.ClearAll();

					BOOL state = selected;
					//6-29--99 watje
					if (hitRec->hitInfo < gesel.GetSize())
					{
						if (invert) 
							state = !gesel[hitRec->hitInfo];
						if (state) 
							gesel.Set(hitRec->hitInfo);
						else       
							gesel.Clear(hitRec->hitInfo);
					}
					
					if (!all) break;

					hitRec = hitRec->Next();
				}
				if (geomElemMode) SelectGeomElement(!sub,&holdSel);
				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setSelectedGeomEdges"), 1, 0,
				mr_bitarray,&(gesel));
			}
		}
		else if ( (ip && (ip->GetSubObjectLevel() == 3)  ) && ( fnGetMapMode() == NOMAP) )
		{
			tmd = (MeshTopoData*)hitRec->modContext->localData;
			BitArray holdFaceSel = fsel;
			
			if (objType == IS_MESH)
			{
				Mesh &mesh = *(((MeshTopoData*)hitRec->modContext->localData)->GetMesh());
				set.SetSize(mesh.getNumFaces());


				AdjFaceList *al = NULL;
				if (polySelect)
					al = BuildAdjFaceList(mesh);

				if (theHold.Holding()) theHold.Put (new UnwrapSelRestore (this,(MeshTopoData*)hitRec->modContext->localData));


				BitArray saveSel;
				if ((planarMode) && (sub))  //hmm hack alert since we are doing a planar selection removal we need to trick the system
				{
					saveSel.SetSize(tmd->faceSel.GetSize());  //save our current sel
					saveSel = tmd->faceSel;					  //then clear it ans set the system to normal select
					tmd->faceSel.ClearAll();				  //we will then fix the selection after the hitrecs
					selected = TRUE;
				}


				while (hitRec) {					
					d = (MeshTopoData*)hitRec->modContext->localData;					
					//			if (set.GetSize()!=d->faceSel.GetSize()) d->faceSel.SetSize(set.GetSize(),TRUE);
					set.ClearAll();
					if (polySelect)
					{
						//6-29--99 watje
						if ((hitRec->hitInfo < mesh.numFaces) &&
							(d->faceSel.GetSize() == set.GetSize()))
						{

							mesh.PolyFromFace (hitRec->hitInfo, set, 45.0, FALSE, al);
							if (invert) d->faceSel ^= set;
							else if (selected) d->faceSel |= set;
							else d->faceSel &= ~set;
						}


					}
					else
					{
						BOOL state = selected;
						//6-29--99 watje
						if (hitRec->hitInfo < d->faceSel.GetSize())
						{
							if (invert) state = !d->faceSel[hitRec->hitInfo];
							if (state) d->faceSel.Set(hitRec->hitInfo);
							else       d->faceSel.Clear(hitRec->hitInfo);
						}
					}
					if (!all) break;

					hitRec = hitRec->Next();
				}

				if (planarMode) SelectGeomFacesByAngle(d);

				if ((planarMode) && (sub))
				{
					saveSel &= ~d->faceSel;
					d->faceSel = saveSel;
				}


				if (geomElemMode) SelectGeomElement(d,!sub,&holdFaceSel);
				UpdateFaceSelection(d->faceSel);
				//PELT
				//				if (peltData.inPeltMode)
				peltData.peltFaces = d->faceSel;

				if (al) delete al;
			}
			else if (objType == IS_MNMESH)
			{
				MNMesh &mesh = *(((MeshTopoData*)hitRec->modContext->localData)->GetMNMesh());



				if (theHold.Holding()) theHold.Put (new UnwrapSelRestore (this,(MeshTopoData*)hitRec->modContext->localData));

				BitArray saveSel;
				if ((planarMode) && (sub))  //hmm hack alert since we are doing a planar selection removal we need to trick the system
				{
					saveSel.SetSize(tmd->faceSel.GetSize());  //save our current sel
					saveSel = tmd->faceSel;					  //then clear it ans set the system to normal select
					tmd->faceSel.ClearAll();				  //we will then fix the selection after the hitrecs
					selected = TRUE;
				}


				while (hitRec) {					
					d = (MeshTopoData*)hitRec->modContext->localData;					
					//			if (set.GetSize()!=d->faceSel.GetSize()) d->faceSel.SetSize(set.GetSize(),TRUE);
					set.ClearAll();
					BOOL state = selected;
					//6-29--99 watje
					if (hitRec->hitInfo < d->faceSel.GetSize())
					{
						if (invert) state = !d->faceSel[hitRec->hitInfo];
						if (state) d->faceSel.Set(hitRec->hitInfo);
						else       d->faceSel.Clear(hitRec->hitInfo);
					}
					if (!all) break;

					hitRec = hitRec->Next();
				}
				if (planarMode) SelectGeomFacesByAngle(d);

				if ((planarMode) && (sub))
				{
					saveSel &= ~d->faceSel;
					d->faceSel = saveSel;
				}


				if (geomElemMode) SelectGeomElement(d,!sub,&holdFaceSel);
				UpdateFaceSelection(d->faceSel);
				//PELT
				//				if (peltData.inPeltMode)
				peltData.peltFaces = d->faceSel;
			}

			else if (objType == IS_PATCH)
			{
				if (theHold.Holding()) theHold.Put (new UnwrapSelRestore (this,(MeshTopoData*)hitRec->modContext->localData));

				BitArray saveSel;
				if ((planarMode) && (sub))  //hmm hack alert since we are doing a planar selection removal we need to trick the system
				{
					saveSel.SetSize(tmd->faceSel.GetSize());  //save our current sel
					saveSel = tmd->faceSel;					  //then clear it ans set the system to normal select
					tmd->faceSel.ClearAll();				  //we will then fix the selection after the hitrecs
					selected = TRUE;
				}

				while (hitRec) {
					PatchMesh &patch = *(((MeshTopoData*)hitRec->modContext->localData)->GetPatch());
					//			if (patch.numPatches!=d->faceSel.GetSize()) d->faceSel.SetSize(patch.numPatches,TRUE);
					od = d;
					d  = (MeshTopoData*)hitRec->modContext->localData;

					// Build it the first time
					BOOL state = selected;
					//6-29--99 watje
					if (hitRec->hitInfo < d->faceSel.GetSize())
					{

						if (invert) state = !d->faceSel[hitRec->hitInfo];
						if (state) d->faceSel.Set(hitRec->hitInfo);
						else       d->faceSel.Clear(hitRec->hitInfo);
					}
					if (!all) break;
					hitRec = hitRec->Next();

				}
				if (planarMode) SelectGeomFacesByAngle(d);

				if ((planarMode) && (sub))
				{
					saveSel &= ~d->faceSel;
					d->faceSel = saveSel;
				}

				if (geomElemMode) SelectGeomElement(d,!sub,&holdFaceSel);
				UpdateFaceSelection(d->faceSel);
				peltData.peltFaces = d->faceSel;
			}
			if (filterSelectedFaces == 1) InvalidateView();

		}

		ComputeSelectedFaceData();

		if (fnGetSyncSelectionMode()) 
			fnSyncTVSelection();
		RebuildDistCache();
		theHold.Accept (GetString (IDS_DS_SELECT));
		if (tmd)
		{
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.selectPolygons"), 1, 0,
				mr_bitarray,&(tmd->faceSel));
		}

		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		SetNumSelLabel();
	}

}

void UnwrapMod::ClearSelection(int selLevel)
{
	//PELT

	if ((peltData.GetPeltMapMode()) && (peltData.GetEditSeamsMode()))
	{
/*
		peltData.seamEdges.ClearAll();
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		//update our views to show new faces
		InvalidateView();
		if (theHold.Holding()) 
		{
			theHold.Put (new UnwrapPeltSeamRestore (this));
			theHold.Accept (GetString (IDS_DS_SELECT));
		}
*/
		return;
	}
	if ( (peltData.GetPointToPointSeamsMode()))
	{		
		return;
	}

	ModContextList list;
	INodeTab nodes;	
	ip->GetModContexts(list,nodes);
	MeshTopoData *d;
	if (theHold.Holding()) 
		theHold.Put (new TSelRestore (this));

	for (int i=0; i<list.Count(); i++) {
		d = (MeshTopoData*)list[i]->localData;
		if (!d) continue;

		switch (selLevel) 
		{
		case 1:
			gvsel.ClearAll();
			break;
		case 2:
			gesel.ClearAll();
			break;
		case 3:
			if (fnGetMapMode() == NOMAP)
			{
				d->faceSel.ClearAll();
				UpdateFaceSelection(d->faceSel);
			}
			break;
		}
	}
//	theHold.Accept (GetString (IDS_DS_SELECT));

	theHold.Suspend();
	if (fnGetSyncSelectionMode()) fnSyncTVSelection();
	theHold.Resume();


	ip->ClearCurNamedSelSet();
	nodes.DisposeTemporary();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	SetNumSelLabel();
	//update our views to show new faces
	InvalidateView();


}
void UnwrapMod::SelectAll(int selLevel)
{
	//PELT


	if ((peltData.GetPeltMapMode()) && (peltData.GetEditSeamsMode()))
	{
		peltData.seamEdges.SetAll();
		return;
	}
	if ((peltData.GetPeltMapMode()) && (peltData.GetPointToPointSeamsMode()))
	{		
		return;
	}


	ModContextList list;
	INodeTab nodes;	
	ip->GetModContexts(list,nodes);
	MeshTopoData *d;
	for (int i=0; i<list.Count(); i++) {
		d = (MeshTopoData*)list[i]->localData;
		if (!d) continue;
		if (theHold.Holding()) theHold.Put (new UnwrapSelRestore (this,d));

		switch (selLevel) 
		{
		case 1:
			gvsel.SetAll();
			
			break;
		case 2:
			gesel.SetAll();
			
			break;
		case 3:
			d->faceSel.SetAll();
			UpdateFaceSelection(d->faceSel);
			break;
		}
	}
	theHold.Accept (GetString (IDS_DS_SELECT));

	ip->ClearCurNamedSelSet();
	nodes.DisposeTemporary();
	if (fnGetSyncSelectionMode()) 
		fnSyncTVSelection();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	SetNumSelLabel();
	//update our views to show new faces

	InvalidateView();
}

void UnwrapMod::InvertSelection(int selLevel)
{
	//PELT
	if ((peltData.GetPeltMapMode()) && (peltData.GetEditSeamsMode()))
		return;

	ModContextList list;
	INodeTab nodes;	
	ip->GetModContexts(list,nodes);
	MeshTopoData *d;
	for (int i=0; i<list.Count(); i++) {
		d = (MeshTopoData*)list[i]->localData;
		if (!d) continue;
		if (theHold.Holding()) theHold.Put (new UnwrapSelRestore (this,d));

		switch (selLevel) 
		{
			case 1:
				{
				BitArray temp = gvsel;
				gvsel = ~temp;				
				break;
				}
			case 2:
				{
				BitArray temp = gesel;
				gesel = ~temp;				
				break;
				}
			case 3:
				{
				BitArray temp = d->faceSel;
				d->faceSel = ~temp;
				UpdateFaceSelection(d->faceSel);
				break;
				}
		}
	}
	theHold.Accept (GetString (IDS_DS_SELECT));

	ip->ClearCurNamedSelSet();
	nodes.DisposeTemporary();
	if (fnGetSyncSelectionMode()) fnSyncTVSelection();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	SetNumSelLabel();
	//update our views to show new faces
	InvalidateView();


}




void UnwrapMod::ActivateSubobjSel(int level, XFormModes& modes) {
	// Fill in modes with our sub-object modes

	if (level ==0)
	{

		
		if (fnGetMapMode() != PELTMAP)
			fnSetMapMode(NOMAP);
		if (peltData.GetPointToPointSeamsMode())
			peltData.SetPointToPointSeamsMode(this,FALSE);

		modes = XFormModes(NULL,NULL,NULL,NULL,NULL,selectMode);
		EnableMapButtons(FALSE);
		EnableFaceSelectionUI(FALSE);
		EnableEdgeSelectionUI(FALSE);
		EnableSubSelectionUI(FALSE);
		if (fnGetMapMode() == PELTMAP)
			EnableAlignButtons(FALSE);
		peltData.EnablePeltButtons(hMapParams, FALSE);
		peltData.SetPointToPointSeamsMode(this,FALSE);
		peltData.SetEditSeamsMode(this,FALSE);

		
	}
	else if (level == 1)
	{		
	
		fnSetTVSubMode(TVVERTMODE);
		if (peltData.GetPointToPointSeamsMode())
			peltData.SetPointToPointSeamsMode(this,FALSE);

		SetupNamedSelDropDown();
		if (fnGetMapMode() != PELTMAP)
			fnSetMapMode(NOMAP);
		modes = XFormModes(NULL,NULL,NULL,NULL,NULL,selectMode);
		EnableMapButtons(FALSE);
		EnableFaceSelectionUI(FALSE);
		EnableEdgeSelectionUI(FALSE);
		EnableSubSelectionUI(TRUE); 	
		if (fnGetMapMode() == PELTMAP)
			EnableAlignButtons(FALSE);
		peltData.EnablePeltButtons(hMapParams, TRUE);

		peltData.SetPointToPointSeamsMode(this,FALSE);
		peltData.SetEditSeamsMode(this,FALSE);

	}
	else if (level == 2)
	{
		
		fnSetTVSubMode(TVEDGEMODE);
		SetupNamedSelDropDown();
		if (fnGetMapMode() != PELTMAP)
			fnSetMapMode(NOMAP);
		modes = XFormModes(NULL,NULL,NULL,NULL,NULL,selectMode);
		EnableMapButtons(FALSE);
		EnableFaceSelectionUI(FALSE);
		EnableEdgeSelectionUI(TRUE);
		EnableSubSelectionUI(TRUE);				
		if (fnGetMapMode() == PELTMAP)
			EnableAlignButtons(FALSE);
		peltData.EnablePeltButtons(hMapParams, TRUE);

		peltData.SetPointToPointSeamsMode(this,FALSE);
		peltData.SetEditSeamsMode(this,FALSE);

	}
	else if (level==3) 
	{
		if (peltData.GetPointToPointSeamsMode())
			peltData.SetPointToPointSeamsMode(this,FALSE);
		
		fnSetTVSubMode(TVFACEMODE);
		SetupNamedSelDropDown();
		//face select
		if (fnGetMapMode() == NOMAP)
			modes = XFormModes(NULL,NULL,NULL,NULL,NULL,selectMode);
		else modes = XFormModes(moveGizmoMode,rotGizmoMode,nuscaleGizmoMode,uscaleGizmoMode,squashGizmoMode,NULL);
		EnableMapButtons(TRUE);		
		EnableFaceSelectionUI(TRUE);
		EnableEdgeSelectionUI(FALSE);
		EnableSubSelectionUI(TRUE);	
		if (fnGetMapMode() == PELTMAP)
		{
			if (peltData.peltDialog.hWnd)
				EnableAlignButtons(FALSE);				
			else EnableAlignButtons(TRUE); 
		}
		peltData.EnablePeltButtons(hMapParams, TRUE);

		peltData.SetPointToPointSeamsMode(this,FALSE);
		peltData.SetEditSeamsMode(this,FALSE);

	}
/*
	if (level==4) 
	{
		SetupNamedSelDropDown();
		//face select
		modes = XFormModes(moveGizmoMode,rotGizmoMode,nuscaleGizmoMode,uscaleGizmoMode,squashGizmoMode,NULL);
		
		EnableMapButtons(TRUE);
	}
*/
	SetNumSelLabel ();
	MoveScriptUI();

	InvalidateView();
	NotifyDependents(FOREVER,PART_DISPLAY,REFMSG_CHANGE);

}


Mtl* UnwrapMod::GetMultiMaterials(Mtl *base)
{
	Tab<Mtl*> materialStack;
	materialStack.Append(1,&base);
	while (materialStack.Count() != 0)
	{
		Mtl* topMaterial = materialStack[0];
		materialStack.Delete(0,1);
		//append any mtl
		for (int i = 0; i < topMaterial->NumSubMtls(); i++)
		{
			Mtl* addMat = topMaterial->GetSubMtl(i);
			if (addMat)
				materialStack.Append(1,&addMat,100);
		}
		

		if  (topMaterial->ClassID() == Class_ID(MULTI_CLASS_ID,0))
			return topMaterial;
	}
	return NULL;
}

Texmap*  UnwrapMod::GetActiveMap()
{
	if ((CurrentMap >= 0) && (CurrentMap < pblock->Count(unwrap_texmaplist)))
	{
		Texmap *map;
		pblock->GetValue(unwrap_texmaplist,0,map,FOREVER,CurrentMap);
		return map;
	}
	else return NULL;
}

Mtl* UnwrapMod::GetCheckerMap()
{
	Mtl *mtl = NULL;
	pblock->GetValue(unwrap_checkmtl,0,mtl,FOREVER);
	return mtl;
}
Mtl* UnwrapMod::GetOriginalMap()
{
	Mtl *mtl = NULL;
	pblock->GetValue(unwrap_checkmtl,0,mtl,FOREVER);
	return mtl;
}

void UnwrapMod::ResetMaterialList()
{
	//set the param block to 1
	pblock->SetCount(unwrap_texmaplist,0);
	pblock->SetCount(unwrap_texmapidlist,0);
	//get our mat list
	Mtl *baseMtl = GetBaseMtl();
   Mtl* checkerMat = GetCheckerMap();
   if (baseMtl == checkerMat)
   {
      pblock->GetValue(unwrap_originalmtl,0,baseMtl,FOREVER);
   }
   else
   {
	pblock->SetValue(unwrap_originalmtl,0,baseMtl);
   }



	//add it to the pblock

	Tab<Texmap*> tmaps;
	Tab<int> matIDs;
	ParseMaterials(baseMtl, tmaps, matIDs);

	pblock->SetCount(unwrap_texmaplist,tmaps.Count());
	pblock->SetCount(unwrap_texmapidlist,tmaps.Count());


   
	if (checkerMat)
	{
		pblock->SetCount(unwrap_texmaplist,1+tmaps.Count());
		pblock->SetCount(unwrap_texmapidlist,1+tmaps.Count());

		Texmap *checkMap = NULL;
		checkMap = (Texmap *) checkerMat->GetSubTexmap(1);
		if (checkMap)
		{
			pblock->SetValue(unwrap_texmaplist,0,checkMap,0);
			int id = -1;
			pblock->SetValue(unwrap_texmapidlist,0,id,0);
		}
	}
	if ((baseMtl != NULL))
	{
		for (int i = 0; i < tmaps.Count(); i++)
		{
			pblock->SetValue(unwrap_texmaplist,0,tmaps[i],i+1);
			pblock->SetValue(unwrap_texmapidlist,0,matIDs[i],i+1);
		}
	}

}
void UnwrapMod::AddToMaterialList(Texmap *map, int id)
{
	pblock->Append(unwrap_texmaplist,1,&map);
	pblock->Append(unwrap_texmapidlist,1,&id);
}
void UnwrapMod::DeleteFromMaterialList(int index)
{
	pblock->Delete(unwrap_texmaplist,index,1);
	pblock->Delete(unwrap_texmapidlist,index,1);
}

void UnwrapMod::ParseMaterials(Mtl *base, Tab<Texmap*> &tmaps, Tab<int> &matIDs)
{
	if (base==NULL) return;
	Tab<Mtl*> materialStack;
	materialStack.Append(1,&base);
	while (materialStack.Count() != 0)
	{
		Mtl* topMaterial = materialStack[0];
		materialStack.Delete(0,1);
		//append any mtl
		for (int i = 0; i < topMaterial->NumSubMtls(); i++)
		{
			Mtl* addMat = topMaterial->GetSubMtl(i);
			if (addMat &&  (topMaterial->ClassID() != Class_ID(MULTI_CLASS_ID,0)) )
				materialStack.Append(1,&addMat,100);
		}
		
		IDxMaterial *dxMaterial = (IDxMaterial *) topMaterial->GetInterface(IDXMATERIAL_INTERFACE);
		if (dxMaterial != NULL)
		{
			int numberBitmaps = dxMaterial->GetNumberOfEffectBitmaps();
			for (int i = 0; i < numberBitmaps; i++)
			{
				PBBitmap *pmap= dxMaterial->GetEffectBitmap(i);
				if (pmap)
				{
					//create new bitmap texture
					BitmapTex *bmtex = (BitmapTex *) CreateInstance(TEXMAP_CLASS_ID, Class_ID(BMTEX_CLASS_ID,0));
					//add it to the list			
					TSTR mapName;
					mapName.printf("%s",pmap->bi.Name());
					bmtex->SetMapName(mapName);
					Texmap *map = (Texmap *) bmtex;
					tmaps.Append(1,&map,100);
					int id = -1;
					matIDs.Append(1,&id,100);
				}
			}
		}
		else if  (topMaterial->ClassID() == Class_ID(MULTI_CLASS_ID,0))
		{
			MultiMtl *mtl = (MultiMtl*) topMaterial;
			IParamBlock2 *pb = mtl->GetParamBlock(0);
			if (pb)
			{	
				int numMaterials = pb->Count(multi_mtls);
            for (int i = 0; i < numMaterials; i++)
				{
					int id;
					Mtl *mat;
					pb->GetValue(multi_mtls,0,mat,FOREVER,i);
					pb->GetValue(multi_ids,0,id,FOREVER,i);

					if (mat)
					{
					int tex_count = mat->NumSubTexmaps();
					for (int j = 0; j < tex_count; j++)
					{
						Texmap *tmap;
						tmap = mat->GetSubTexmap(j);
						
						if (tmap != NULL)
						{
							tmaps.Append(1,&tmap,100);								
							matIDs.Append(1,&id,100);
						}
					}
				}
			}
		}
		}
		else
		{
			int tex_count = topMaterial->NumSubTexmaps();
			for (int i = 0; i < tex_count; i++)
			{
				Texmap *tmap = topMaterial->GetSubTexmap(i);
				if (tmap != NULL)
				{
					tmaps.Append(1,&tmap,100);	
					int id = -1;
					matIDs.Append(1,&id,100);
				}
			}
		}

	}
}
Mtl *UnwrapMod::GetBaseMtl()
{
	MyEnumProc dep;              
	DoEnumDependents(&dep);
	if (dep.Nodes.Count() > 0)
		return dep.Nodes[0]->GetMtl();
	return NULL;
}

void UnwrapMod::LoadMaterials()
{
	//no entries on our material list lets add some based on the current assigned material in MtlBase
	int ct =0;
/*
	//5.1.02 adds new bitmap bg management
	hTextures = iOption->GetItemHwnd(ID_TOOL_TEXTURE_COMBO);
	SendMessage(hTextures, CB_RESETCONTENT,0,0);

	HFONT hFont;			// Add for Japanese version
	hFont = CreateFont(12,0,0,0,FW_LIGHT,0,0,0,SHIFTJIS_CHARSET,0,0,0, VARIABLE_PITCH | FF_SWISS, _T(""));
	SendMessage(hTextures, WM_SETFONT, (WPARAM)hFont, MAKELONG(0, 0));
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)_T("---------------------"));	
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_PW_PICK));	
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)_T("---------------------"));
*/
	/*
	for (int i = 0; i < 90 ;i++)
	{
		ReplaceReference(i+1,NULL);
		map[i] = NULL;
	}
*/



//	MyEnumProc dep;              
//	DoEnumDependents(&dep);
//	INode *SelfNode = dep.Nodes[0];
	Mtl *baseMtl = GetBaseMtl();
/*
	Mtl* checkerMat = GetCheckerMap();
	if (checkerMat)
	{
		Texmap *checkMap = NULL;
		checkMap = (Texmap *) checkerMat->GetSubTexmap(1);
		if (checkMap)
		{
			AddMaterial(checkMap);
			ct++;
		}
	}
*/
	//if ((map[0] == NULL) && (BaseMtl != NULL))
	if ((baseMtl != NULL))
	{
		Tab<Texmap*> tmaps;
		Tab<int> matIDs;
		ParseMaterials(baseMtl, tmaps, matIDs);
		int ct = 90;
		if (tmaps.Count() < ct) ct = tmaps.Count();
		for (int i = 0; i < ct; i++)
		{
			if (matid !=-1)
			{
//				if (filterMatID[matid] == matIDs[i])
//					AddMaterial(tmaps[i]);
			}
//			else AddMaterial(tmaps[i],FALSE);
		}
		//check is standard material
/*
		if (baseMtl->ClassID() == Class_ID(DMTL_CLASS_ID,0))
		{
			StdMat *stdmat = (StdMat*) baseMtl;
			int tex_count = stdmat->NumSubTexmaps();
			for (int i = 0; i < tex_count; i++)
			{
				Texmap *tmap;
				MtlBase *m;
				tmap = stdmat->GetSubTexmap(i);
				m = (MtlBase*) tmap;
				//			m = (MtlBase*) stdmat->GetSubMtl(i);
				if (m != NULL)
				{
					//add to our list
					AddMaterial(m);
					ct++;
					if (ct ==90) return;
				}
			}
		}
		//check multi sub material
		//5.1.02 adds new bitmap bg management
		else if (baseMtl->IsMultiMtl() && (matPairList.Count()>0))
		{			
			if ((matid !=-1) && (matPairList[filterMatID[matid]]))
			{


				Mtl *m = matPairList[filterMatID[matid]];
				int tex_count = m->NumSubTexmaps();
         for (int i = (tex_count-1); i >= 0 ; i--)
				{

					Texmap *tmap;
					MtlBase *mt;
					tmap = m->GetSubTexmap(i);
					mt = (MtlBase*) tmap;
					if (mt != NULL)
					{
						//add to our list
						AddMaterial(mt);
						ct++;
						if (ct ==90) return;
					}
				}
			}
			if ((matid == -1) || (ct == 0))
			{
				int mat_count = 0;
				mat_count = baseMtl->NumSubMtls();
				for (int j=0; j < mat_count; j++)
				{

					MtlBase *m;
					m = (MtlBase*) baseMtl->GetSubMtl(j);
					if (m != NULL)
					{
						int tex_count = m->NumSubTexmaps();
						for (int i = 0; i < tex_count; i++)
						{

							Texmap *tmap;
							MtlBase *mt;
							tmap = m->GetSubTexmap(i);
							mt = (MtlBase*) tmap;
							if (mt != NULL)
							{
								//add to our list
								AddMaterial(mt);
								ct++;
								if (ct ==90) return;
								i = tex_count;

							}
						}
					}
				}
				if (ct < 90)
				{
					for (int j=0; j < mat_count; j++)
					{

						MtlBase *m;
						m = (MtlBase*) baseMtl->GetSubMtl(j);
						if (m != NULL)
						{
							int tex_count = m->NumSubTexmaps();
							BOOL first = TRUE;
							for (int i = 0; i < tex_count; i++)
							{

								Texmap *tmap;
								MtlBase *mt;
								tmap = m->GetSubTexmap(i);
								mt = (MtlBase*) tmap;
								if (mt != NULL)
								{
									//add to our list
									if (!first)
									{
										AddMaterial(mt);
										ct++;
										if (ct ==90) return;
									}
									first = FALSE;
									//								i = tex_count;
								}

							}
						}
					}
				}


			}
		}
		else
		{
			int tex_count = baseMtl->NumSubTexmaps();
			for (int i = 0; i < tex_count; i++)
			{
				Texmap *tmap;
				MtlBase *m;
				tmap = baseMtl->GetSubTexmap(i);
				m = (MtlBase*) tmap;
				if (m != NULL)
				{
					//add to our list
					AddMaterial(m);
					ct++;
					if (ct ==90) i = tex_count;
				}
			}

			int mat_count = 0;
			mat_count = baseMtl->NumSubMtls();
			if (ct < 90)
			{
				for (int j=0; j < mat_count; j++)
				{
					MtlBase *m;
					m = (MtlBase*) baseMtl->GetSubMtl(j);
					if (m != NULL)				
					{
						int tex_count = m->NumSubTexmaps();
						for (int i = 0; i < tex_count; i++)
						{
							Texmap *tmap;
							MtlBase *m2;
							tmap = m->GetSubTexmap(i);
							m2 = (MtlBase*) tmap;
							if (m2 != NULL)
							{
								//add to our list
								AddMaterial(m2);
								ct++;
								if (ct ==90) i = tex_count;
							}
						}
					}
				}
			}

		}
*/
	}

//	SetupImage();
//	SendMessage(hTextures, CB_SETCURSEL, CurrentMap, 0 );

}

void UnwrapMod::BuildMatIDList()
{

	filterMatID.ZeroCount();
	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		int found = 0;
		if (!(TVMaps.f[i]->flags & FLAG_DEAD))
		{
			for (int j = 0; j < filterMatID.Count(); j++)
			{
				if (filterMatID[j] == TVMaps.f[i]->MatID) 
				{
					found = 1;
					j = filterMatID.Count();
				}
			}
		}
		else found = 1;
		if (found == 0)
			filterMatID.Append(1,&TVMaps.f[i]->MatID,1);
	}

	vertMatIDList.SetSize(TVMaps.v.Count());
	vertMatIDList.SetAll();
	matid = -1;
}


void UnwrapMod::ComputeFalloff(float &u, int ftype)

{
	if (u<= 0.0f) u = 0.0f;
	else if (u>= 1.0f) u = 1.0f;
	else switch (ftype)
	{
case (3) : u = u*u*u; break;
	//	case (BONE_FALLOFF_X2_FLAG) : u = u*u; break;
case (0) : u = u; break;
case (1) : u = 1.0f-((float)cos(u*PI) + 1.0f)*0.5f; break;
	//	case (BONE_FALLOFF_2X_FLAG) : u = (float) sqrt(u); break;
   case (2) : u = (float) pow(u,0.3f); break;

	}

}




void UnwrapMod::ModifyObject(
							 TimeValue t, ModContext &mc, ObjectState *os, INode *node)
{
 
	isMesh = FALSE;
	currentTime = t;

	MyEnumProc dep;              
	DoEnumDependents(&dep);

	// Prepare the controller and set up mats
	Object *obj = os->obj;

	if (oldDataPresent)
	{
		oldDataPresent = FALSE;

		Tab<UVW_TVFaceClass*> tempStorage;
		int ct = TVMaps.f.Count();
		tempStorage.SetCount(ct);
		for (int i =0; i < ct; i++)
			tempStorage[i] = TVMaps.f[i]->Clone();


		Mesh mesh;
		TriObject *convertedTri = NULL;
		if (obj->IsSubClassOf(triObjectClassID))
		{
			TriObject *tobj = (TriObject*)os->obj;
			mesh = tobj->GetMesh();
		}
		else if (obj->CanConvertToType(triObjectClassID))
		{
			convertedTri = (TriObject *) obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
			mesh = convertedTri->GetMesh();
		}

		if (mesh.selLevel==MESH_FACE) 
		{


			TVMaps.SetCountFaces(mesh.getNumFaces());
			int current = 0;
			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				if (mesh.faceSel[i])
				{
					TVMaps.f[i]->DeleteVec();
					if (TVMaps.f[i]) delete TVMaps.f[i];
					TVMaps.f[i] = tempStorage[current]->Clone();
					current++;
				}
				else 
				{
					TVMaps.f[i]->flags = 0;
					TVMaps.f[i]->t = new int[3];
					TVMaps.f[i]->v = new int[3];

					TVMaps.f[i]->t[0] = 0;
					TVMaps.f[i]->t[1] = 0;
					TVMaps.f[i]->t[2] = 0;
					TVMaps.f[i]->FaceIndex = i;
					TVMaps.f[i]->MatID = mesh.faces[i].getMatID();
					TVMaps.f[i]->count = 3;

					TVMaps.f[i]->flags |= FLAG_DEAD;
				}
			}	
		}
		TVMaps.geomPoints.SetCount(mesh.numVerts);

		for (int j=0; j<TVMaps.f.Count(); j++) 
		{
			if (j < mesh.numFaces)
			{
				for (int k = 0; k < 3; k++)
				{
					int index = mesh.faces[j].v[k];
					TVMaps.f[j]->v[k] = index;
					TVMaps.geomPoints[index] = mesh.verts[index];
				}
			}
		}
		if (convertedTri) 
			convertedTri->DeleteThis();

	}


   BOOL collapsed = FALSE;
   
	if (
#ifndef NO_PATCHES // orb 02-05-03
		(!os->obj->IsSubClassOf(patchObjectClassID)) && 
#endif // NO_PATCHES		
		(!os->obj->IsSubClassOf(triObjectClassID)) &&
		(!os->obj->IsSubClassOf(polyObjectClassID)) )
	{
      //neither patch or poly convert to a mesh
		if (os->obj->CanConvertToType(triObjectClassID))
		{
			TriObject *tri = (TriObject *) os->obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
			os->obj = tri;
			os->obj->UnlockObject();
         collapsed = TRUE;
		}

	}

	//poll for material on mesh
	int CurrentChannel = 0;

	if (channel == 0)
	{
		CurrentChannel = 1;
		//should be from scroller;

	}
	else if (channel == 1)
	{
		CurrentChannel = 0;
	}
	else CurrentChannel = channel;
	
	BOOL reset = FALSE;
   if (!collapsed)
   {
	if (obj->NumPoints() != TVMaps.geomPoints.Count())
	{
		int ct = obj->NumPoints();
		gvsel.SetSize(ct);
		gvsel.ClearAll();
		TVMaps.geomPoints.SetCount(ct);
         TVMaps.edgesValid = FALSE;    
		 reset =TRUE;
		 updateCache = TRUE;
	}

	
	for (int i = 0; i < TVMaps.geomPoints.Count(); i++)
	{
		TVMaps.geomPoints[i] = obj->GetPoint(i);
	}
   }
   else
   {
      TriObject *tobj = (TriObject *) os->obj;
      
      if (tobj->GetMesh().numVerts != TVMaps.geomPoints.Count())
      {
         int ct = tobj->GetMesh().numVerts;
         gvsel.SetSize(ct);
         gvsel.ClearAll();
         TVMaps.geomPoints.SetCount(ct);
         TVMaps.edgesValid = FALSE;   
		 reset =TRUE;
		 updateCache = TRUE;
      }

      for (int i = 0; i < TVMaps.geomPoints.Count(); i++)
      {
         TVMaps.geomPoints[i] = tobj->GetMesh().verts[i];
      }
   }

	

	if (dep.Nodes.Count() > 1)
	{
		BOOL first = TRUE;
		for (int i = 0; i < dep.Nodes.Count(); i++)
		{
			if (first)
			{
				if (IsInStack(dep.Nodes[i]))
				{
					obj = GetBaseObject(dep.Nodes[i]->GetObjectRef());
					first = FALSE;
				}
			}
			else if (obj)
			{

				Object *tobj = GetBaseObject(dep.Nodes[i]->GetObjectRef());
				if (IsInStack(dep.Nodes[i]))
				{
					if (obj != tobj) modifierInstanced = TRUE;
				}
			}
		}
		/*
		Object *obj = GetBaseObject(dep.Nodes[0]->GetObjectRef());

		for (int i = 1; i < dep.Nodes.Count(); i++)
		{
		Object *tobj = GetBaseObject(dep.Nodes[i]->GetObjectRef());
		if (IsInStack(dep.Nodes[i]))
		{
		if (obj != tobj) 
		modifierInstanced = TRUE;
		}
		}
		*/

		if ((modifierInstanced) && (ip))
		{
			subObjCount = 0;
		}
		if (modifierInstanced) return;
	}



	if (applyToWholeObject)
	{
#ifndef NO_PATCHES // orb 02-05-03
		if (os->obj->IsSubClassOf(patchObjectClassID))
		{
			PatchObject *pobj = (PatchObject*)os->obj;
			pobj->patch.selLevel = PATCH_OBJECT;
			pobj->patch.patchSel.ClearAll();
		}
		else 
#endif // NO_PATCHES
			if (os->obj->IsSubClassOf(triObjectClassID))
			{
				isMesh = TRUE;
				TriObject *tobj = (TriObject*)os->obj;
				tobj->GetMesh().selLevel = MESH_OBJECT;
				tobj->GetMesh().faceSel.ClearAll();
			}
			else if (os->obj->IsSubClassOf(polyObjectClassID))
			{
				PolyObject *pobj = (PolyObject*)os->obj;
				pobj->GetMesh().selLevel = MNM_SL_OBJECT; 
				BitArray s;
				pobj->GetMesh().getFaceSel(s);
				s.ClearAll();
				pobj->GetMesh().FaceSelect(s);
			}
	}


#ifndef NO_PATCHES // orb 02-05-03
	if (os->obj->IsSubClassOf(patchObjectClassID))
	{
		PatchObject *pobj = (PatchObject*)os->obj;
		if (ip) 
		{
			if ((pobj->patch.selLevel==PATCH_PATCH ) && (subObjCount>=1))
			{
				firstPass= TRUE;
			}
			else if (subObjCount==0)
			{
				firstPass= TRUE;
			}
		}
	}
	else

#endif
		if (os->obj->IsSubClassOf(triObjectClassID)) 
		{
			TriObject *tobj = (TriObject*)os->obj;
			isMesh = TRUE;
			if (ip) 
			{
				if ((tobj->GetMesh().selLevel==MESH_FACE) && (subObjCount>=1))
				{
					firstPass= TRUE;
				}
				else if (subObjCount==0)
				{
					firstPass= TRUE;
				}
			}
		}
		else if (os->obj->IsSubClassOf(polyObjectClassID) )
		{
			PolyObject *pobj = (PolyObject*)os->obj;
			if (ip) 
			{
				if ((pobj->GetMesh().selLevel==MNM_SL_FACE)  && (subObjCount>=1))
				{
					firstPass= TRUE;
				}
				else if (subObjCount==0)
				{
					firstPass= TRUE;
				}
			}
		}


#ifndef NO_PATCHES // orb 02-05-03

		if ((os->obj->IsSubClassOf(patchObjectClassID)) &  (firstPass))
		{
			PatchObject *pobj = (PatchObject*)os->obj;
			if (ip) 
			{
				firstPass = FALSE;
				if (pobj->patch.selLevel==PATCH_PATCH ) 
				{	
					//sendmessage(REFMSG_NUM_SUBOBJECTTYPES_CHANGED)
					//PUTBACK				ip->EnableSubObjectSelection(FALSE);

					if (subObjCount != 0)
					{
						subObjCount = 0;
						//NotifyDependents(FOREVER,PART_ALL,REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
						mUpdateStackUI = TRUE;


						EnableWindow(GetDlgItem(hParams,IDC_RADIO1),FALSE);
						EnableWindow(GetDlgItem(hParams,IDC_RADIO2),FALSE);
						EnableWindow(GetDlgItem(hParams,IDC_RADIO3),FALSE);
						EnableWindow(GetDlgItem(hParams,IDC_RADIO4),FALSE);
					}
				}
				else
				{
					//PUTBACK				ip->EnableSubObjectSelection(TRUE);
					if (subObjCount != 1)
					{
						subObjCount = 3;
						//NotifyDependents(FOREVER,PART_ALL,REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
						mUpdateStackUI = TRUE;
						EnableWindow(GetDlgItem(hParams,IDC_RADIO1),TRUE);
						EnableWindow(GetDlgItem(hParams,IDC_RADIO2),TRUE);
						EnableWindow(GetDlgItem(hParams,IDC_RADIO3),TRUE);
						EnableWindow(GetDlgItem(hParams,IDC_RADIO4),TRUE);
					}

				}


			}

		}
		else 
#endif // NO_PATCHES
			if ((os->obj->IsSubClassOf(triObjectClassID)) && (firstPass))
			{
				isMesh = TRUE;
				TriObject *tobj = (TriObject*)os->obj;
				if (ip) 
				{
					firstPass = FALSE;
					if (tobj->GetMesh().selLevel==MESH_FACE) 
					{
						//PUTBACK				ip->EnableSubObjectSelection(FALSE);
						if (subObjCount != 0)
						{
							subObjCount = 0;
							//NotifyDependents(FOREVER,PART_ALL,REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
							mUpdateStackUI = TRUE;

							EnableWindow(GetDlgItem(hParams,IDC_RADIO1),FALSE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO2),FALSE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO3),FALSE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO4),FALSE);
						}

					}
					else
					{
						//PUTBACK				ip->EnableSubObjectSelection(TRUE);
						if (subObjCount != 1)
						{
							subObjCount = 3;
							//NotifyDependents(FOREVER,PART_ALL,REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
							mUpdateStackUI = TRUE;


							EnableWindow(GetDlgItem(hParams,IDC_RADIO1),TRUE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO2),TRUE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO3),TRUE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO4),TRUE);
						}

					}


				}
			}
			else if ((os->obj->IsSubClassOf(polyObjectClassID)) && (firstPass))
			{
				PolyObject *pobj = (PolyObject*)os->obj;
				if (ip) 
				{
					firstPass = FALSE;
					if (pobj->GetMesh().selLevel==MNM_SL_FACE) 
					{
						if (subObjCount != 0)
						{
							subObjCount = 0;
							//NotifyDependents(FOREVER,PART_ALL,REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
							mUpdateStackUI = TRUE;

							EnableWindow(GetDlgItem(hParams,IDC_RADIO1),FALSE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO2),FALSE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO3),FALSE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO4),FALSE);
						}

					}
					else
					{
						if (subObjCount != 1)
						{
							subObjCount = 3;
							//NotifyDependents(FOREVER,PART_ALL,REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
							mUpdateStackUI = TRUE;

							EnableWindow(GetDlgItem(hParams,IDC_RADIO1),TRUE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO2),TRUE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO3),TRUE);
							EnableWindow(GetDlgItem(hParams,IDC_RADIO4),TRUE);
						}

					}


				}
			}




 			
#ifndef NO_PATCHES // orb 02-05-03
			if (os->obj->IsSubClassOf(patchObjectClassID))
			{
				// is whole mesh
				PatchObject *pobj = (PatchObject*)os->obj;
				PatchMesh &patch = pobj->patch;
				if ( ((TVMaps.f.Count() != patch.getNumPatches()) ) &&
					(TVMaps.v.Count() != 0) )	
				{
					reset =TRUE;
					updateCache = TRUE;
					//			os->obj->UpdateValidity(TEXMAP_CHAN_NUM,LocalValidity(t));	
					//			return;
				}

			}
			else 
#endif // NO_PATCHES
				if (os->obj->IsSubClassOf(triObjectClassID)) 
				{
					isMesh = TRUE;
					TriObject *tobj = (TriObject*)os->obj;
					// Apply our mapping
					Mesh &mesh = tobj->GetMesh();
					if ( ( (TVMaps.f.Count() != mesh.getNumFaces()) )&&
						(TVMaps.v.Count() != 0) )
					{
						reset = TRUE;
						updateCache = TRUE;
						//			os->obj->UpdateValidity(TEXMAP_CHAN_NUM,LocalValidity(t));	
						//			return;
					}
				}
				else if (os->obj->IsSubClassOf(polyObjectClassID)) 
				{
					PolyObject *tobj = (PolyObject*)os->obj;
					// Apply our mapping
					MNMesh &mesh = tobj->GetMesh();
					if ( ((TVMaps.f.Count() != mesh.FNum()) )  &&
						(TVMaps.v.Count() != 0) )
					{
						reset = TRUE;
						updateCache = TRUE;
						//			os->obj->UpdateValidity(TEXMAP_CHAN_NUM,LocalValidity(t));	
						//			return;
					}
				}
				//if the topochanges and the user has forceUpdate off do nothing
				//this is for cases when they apply unwrap to an object that has changed topology but
				//will come back at render time	
				if ((reset) && (!forceUpdate))
				{
					os->obj->UpdateValidity(TEXMAP_CHAN_NUM,GetValidity(t));	
					return;
				}
//if our edges are invalid chances are that our geo faces might also be so check them also
            if ((TVMaps.edgesValid == FALSE) && (!reset))
            {
               //if we have geo change need to reset
#ifndef NO_PATCHES // orb 02-05-03
               if (os->obj->IsSubClassOf(patchObjectClassID))
               {
                  PatchObject *pobj = (PatchObject*)os->obj;
                  UpdateGeoFaceData(&pobj->patch);
               }           
#endif // NO_PATCHES
               if (os->obj->IsSubClassOf(triObjectClassID)) 
               {
                  TriObject *tobj = (TriObject*)os->obj;
                  UpdateGeoFaceData(&tobj->GetMesh());
               }
               if (os->obj->IsSubClassOf(polyObjectClassID)) 
               {
                  PolyObject *tobj = (PolyObject*)os->obj;
                  UpdateGeoFaceData(&tobj->GetMesh());
               }

               
            }

				//check is TVMap == 0 then get data from mesh, patch, or nurbs and copy to our data
				if ((TVMaps.v.Count() == 0) || (reset))
				{
#ifndef NO_PATCHES // orb 02-05-03
					///is patch
					if (os->obj->IsSubClassOf(patchObjectClassID))
					{
						if (!InitializePatchData(os, CurrentChannel)) return;
					}
					///else it is something else convert to a mesh
					else 
#endif // NO_PATCHES
						if (os->obj->IsSubClassOf(triObjectClassID)) 
						{
							if (!InitializeMeshData(os, CurrentChannel)) return;
						}
						else if (os->obj->IsSubClassOf(polyObjectClassID)) 
						{
							if (!InitializeMNMeshData(os, CurrentChannel)) return;
						}

						BuildMatIDList();
						TVMaps.edgesValid = FALSE;

				}

				if (fsel.GetSize() != TVMaps.f.Count())
				{
					fsel.SetSize(TVMaps.f.Count());
					fsel.ClearAll();
				}
				if (gvsel.GetSize() != TVMaps.geomPoints.Count())
				{
					gvsel.SetSize(TVMaps.geomPoints.Count());
					gvsel.ClearAll();
				}

				



				if ((ip) && (TVMaps.edgesValid == FALSE))
				{
					RebuildEdges();
					theHold.Suspend();
					if (fnGetSyncSelectionMode()) 
					{
						MeshTopoData *md = (MeshTopoData *) mc.localData;
						if ((md) && (!reset))
							SyncGeomToTVSelection(md);
					}
					theHold.Resume();
				}


				if (getFaceSelectionFromStack)
				{
#ifndef NO_PATCHES // orb 02-05-03
					if (os->obj->IsSubClassOf(patchObjectClassID))
					{
						GetFaceSelectionFromPatch(os,mc, t);
					}
					else 
#endif // NO_PATCHES
						if (os->obj->IsSubClassOf(triObjectClassID))
						{
							GetFaceSelectionFromMesh(os,mc, t);
						}
						else if (os->obj->IsSubClassOf(polyObjectClassID))
						{
							GetFaceSelectionFromMNMesh(os,mc, t);
						}

						getFaceSelectionFromStack = FALSE;
				}

#ifndef NO_PATCHES // orb 02-05-03
				if (os->obj->IsSubClassOf(patchObjectClassID))
				{

					CopySelectionPatch(os,mc, CurrentChannel,t);
				}
				else 
#endif // NO_PATCHES
					if (os->obj->IsSubClassOf(triObjectClassID))
					{
						isMesh = TRUE;
						CopySelectionMesh(os,mc, CurrentChannel,t);
					}
					else if (os->obj->IsSubClassOf(polyObjectClassID))
					{
						CopySelectionMNMesh(os,mc, CurrentChannel,t);
					}


					if (TVMaps.geomPoints.Count() == 0)
					{
						TVMaps.geomPoints.SetCount(os->obj->NumPoints());
						for (int i = 0; i < os->obj->NumPoints(); i++)
						{
							TVMaps.geomPoints[i] = os->obj->GetPoint(i);
						}

					}

					//we already have edits so just copy them into the  tv faces/verts
					if (TVMaps.v.Count() != 0)
					{
						//else copy our data into the mesh,patch or nurbs tv data
						///is patch
#ifndef NO_PATCHES // orb 02-05-03
						if (os->obj->IsSubClassOf(patchObjectClassID))
						{
							ApplyPatchMapping(os, CurrentChannel,t);
						}
						///else convert to a mesh
						else 
#endif // NO_PATCHES
							if (os->obj->IsSubClassOf(triObjectClassID)) 
							{
								isMesh = TRUE;
								ApplyMeshMapping(os, CurrentChannel,t);
							}
							///else convert to a mesh
							else if (os->obj->IsSubClassOf(polyObjectClassID)) 
							{
								ApplyMNMeshMapping(os, CurrentChannel,t);
							}

					}

					Interval iv = GetValidity(t);
					iv = iv & os->obj->ChannelValidity(t,GEOM_CHAN_NUM);
					iv = iv & os->obj->ChannelValidity(t,TOPO_CHAN_NUM);
					os->obj->UpdateValidity(TEXMAP_CHAN_NUM,iv);

					if ((popUpDialog) && (alwaysEdit))
						fnEdit();
					popUpDialog = FALSE;
}

Interval UnwrapMod::LocalValidity(TimeValue t)
{
	// aszabo|feb.05.02 If we are being edited, 
	// return NEVER to forces a cache to be built after previous modifier.
	if (TestAFlag(A_MOD_BEING_EDITED))
		return NEVER;

	return GetValidity(t);
}

//aszabo|feb.06.02 - When LocalValidity is called by ModifyObject,
// it returns NEVER and thus the object channels are marked non valid
// As a result, the mod stack enters and infinite evaluation of the modifier
// ModifyObject now calls GetValidity and CORE calls LocalValidity to
// allow for building a cache on the input of this modifier when it's 
// being edited 
Interval UnwrapMod::GetValidity(TimeValue t)
{
	Interval iv = FOREVER;
	for (int i=0; i<TVMaps.cont.Count(); i++) {
		if (TVMaps.cont[i]) {
			if (i < TVMaps.v.Count())
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,iv);
		}
	}
	return iv;
}

		
		
RefTargetHandle UnwrapMod::GetReference(int i)
{
	if (i==0) return tmControl;
	else if (i<91) return  map[i-1];
	else if (i==95) return pblock;
	else if (i > 110) return TVMaps.cont[i-11-100];
	else if (i == 100) return checkerMat;
	else if (i == 101) return originalMat;
	return NULL;
}



void UnwrapMod::SetReference(int i, RefTargetHandle rtarg)
{
	if (i==0) tmControl = (Control*)rtarg;
	else if (i<91) map[i-1] = (Texmap*)rtarg;
	else if (i == 95)  pblock=(IParamBlock2*)rtarg;
	else if (i == 100) checkerMat = (StdMat*) rtarg;
	else if (i == 101) originalMat = (ReferenceTarget*) rtarg;
	else if ((i-11-100) < TVMaps.cont.Count())
		TVMaps.cont[i-11-100] = (Control*)rtarg;


}
// ref 0 - the tm control for gizmo
// ref 1-90 - map references
// ref 100 the checker texture
// ref 111+ the vertex controllers

int UnwrapMod::RemapRefOnLoad(int iref) 
{
	if (version == 1)
	{
		if (iref == 0)
			return 1;
		else if (iref > 0)
			return iref + 10 + 100;

	}
	else if (version < 8)
	{
		if (iref > 10)
			return iref + 100;
	}
	return iref;
}

Animatable* UnwrapMod::SubAnim(int i)
{
	return TVMaps.cont[i];
}

TSTR UnwrapMod::SubAnimName(int i)
{
	TSTR buf;
	//	buf.printf(_T("Point %d"),i+1);
	buf.printf(_T("%s %d"),GetString(IDS_PW_POINT),i+1);
	return buf;
}


RefTargetHandle UnwrapMod::Clone(RemapDir& remap)
{
	UnwrapMod *mod = new UnwrapMod;
	mod->TVMaps    = TVMaps;
	//	mod->tvert     = tvert;
	//	mod->tvFace    = tvFace;
	//	mod->cont      = cont;
	mod->vsel      = vsel;

	mod->esel = esel;
	mod->fsel = fsel;
	mod->gesel = gesel;
	mod->gvsel = gvsel;

	mod->zoom      = zoom;
	mod->aspect    = aspect;
	mod->xscroll   = xscroll;
	mod->yscroll   = yscroll;
	mod->uvw       = uvw;
	mod->showMap   = showMap;
	mod->update    = update;
	mod->lineColor = lineColor;
	mod->openEdgeColor = openEdgeColor;
	mod->selColor  =	selColor;
	mod->rendW     = rendW;
	mod->rendH     = rendH;
	mod->isBitmap =  isBitmap;
	mod->isBitmap =  pixelSnap;
	mod->useBitmapRes = useBitmapRes;
	mod->channel = channel;
   mod->ReplaceReference(PBLOCK_REF,remap.CloneRef(pblock));
	mod->ReplaceReference(0,remap.CloneRef(tmControl));
	

	for (int i=0; i<TVMaps.cont.Count(); i++) {
		mod->TVMaps.cont[i] = NULL;		
		if (TVMaps.cont[i]) mod->ReplaceReference(i+11+100,remap.CloneRef(TVMaps.cont[i]));
	}

	mod->TVMaps.channel = TVMaps.channel;
	mod->TVMaps.v = TVMaps.v;
	mod->TVMaps.geomPoints = TVMaps.geomPoints;
	mod->TVMaps.CloneFaces(TVMaps.f);

	mod->TVMaps.e.Resize(0); // LAM - 8/23/04 - in case mod is currently active in modify panel
	mod->TVMaps.ge.Resize(0);
	mod->TVMaps.ePtrList.Resize(0); // LAM - 8/23/04 - in case mod is currently active in modify panel
	mod->TVMaps.gePtrList.Resize(0);

	if (instanced)
	{
      for (int i=0; i<mod->TVMaps.cont.Count(); i++) mod->DeleteReference(i+11+100);
		mod->TVMaps.v.Resize(0);
		mod->TVMaps.f.Resize(0);
		mod->TVMaps.cont.Resize(0);
		mod->vsel.SetSize(0);
		mod->updateCache = TRUE;
		mod->instanced = FALSE;
	}
	mod->loadDefaults = FALSE;
	mod->showIconList = showIconList;

	BaseClone(this, mod, remap);
	return mod;
}

#define NAMEDSEL_STRING_CHUNK	0x2809
#define NAMEDSEL_ID_CHUNK		0x2810

#define NAMEDVSEL_STRING_CHUNK	0x2811
#define NAMEDVSEL_ID_CHUNK		0x2812

#define NAMEDESEL_STRING_CHUNK	0x2813
#define NAMEDESEL_ID_CHUNK		0x2814


RefResult UnwrapMod::NotifyRefChanged(
									  Interval changeInt, 
									  RefTargetHandle hTarget, 
									  PartID& partID, 
									  RefMessage message)
{

	if (suspendNotify) return REF_STOP;

	switch (message) 
	{
		case REFMSG_CHANGE:
			if (editMod==this && hView) 
			{
				InvalidateView();
			}
			break;



	}
	return REF_SUCCEED;
}

#define VERTCOUNT_CHUNK	0x0100
#define VERTS_CHUNK		0x0110
#define VERTSEL_CHUNK	0x0120
#define ZOOM_CHUNK		0x0130
#define ASPECT_CHUNK	0x0140
#define XSCROLL_CHUNK	0x0150
#define YSCROLL_CHUNK	0x0160
#define IWIDTH_CHUNK	0x0170
#define IHEIGHT_CHUNK	0x0180
#define SHOWMAP_CHUNK	0x0190
#define UPDATE_CHUNK	0x0200
#define LINECOLOR_CHUNK	0x0210
#define SELCOLOR_CHUNK	0x0220
#define FACECOUNT_CHUNK	0x0230
#define FACE_CHUNK		0x0240
#define UVW_CHUNK		0x0250
#define CHANNEL_CHUNK	0x0260
#define VERTS2_CHUNK	0x0270
#define FACE2_CHUNK		0x0280
#define PREFS_CHUNK		0x0290
#define USEBITMAPRES_CHUNK		0x0300

#define GEOMPOINTSCOUNT_CHUNK		0x320
#define GEOMPOINTS_CHUNK		0x330
#define LOCKASPECT_CHUNK		0x340
#define MAPSCALE_CHUNK		0x350
#define WINDOWPOS_CHUNK     0x360
#define FORCEUPDATE_CHUNK     0x370

#define TILE_CHUNK			0x380
#define TILECONTRAST_CHUNK  0x390
#define TILELIMIT_CHUNK     0x400

#define SOFTSELLIMIT_CHUNK     0x410

#define FLATTENMAP_CHUNK    0x420
#define NORMALMAP_CHUNK     0x430
#define UNFOLDMAP_CHUNK     0x440
#define STITCH_CHUNK		0x450

#define GEOMELEM_CHUNK		0x460
#define PLANARTHRESHOLD_CHUNK		0x470
#define BACKFACECULL_CHUNK		0x480
#define TVELEMENTMODE_CHUNK		0x490
#define ALWAYSEDIT_CHUNK		0x500
#define SHOWCONNECTION_CHUNK		0x510
#define PACK_CHUNK		0x520
#define TVSUBOBJECTMODE_CHUNK		0x530
#define FILLMODE_CHUNK		0x540
#define OPENEDGECOLOR_CHUNK	0x550
#define UVEDGEMODE_CHUNK	0x560
#define MISCCOLOR_CHUNK	0x570
#define HITSIZE_CHUNK	0x580
#define PIVOT_CHUNK	0x590
#define GIZMOSEL_CHUNK	0x600
#define SHARED_CHUNK	0x610
#define SHOWICON_CHUNK	0x620
#define SYNCSELECTION_CHUNK	0x630
#define BRIGHTCENTER_CHUNK	0x640

#define CURSORSIZE_CHUNK	0x650
#define TICKSIZE_CHUNK		0x660
//new
#define GRID_CHUNK		0x670

#define PREVENTFLATTENING_CHUNK		0x680

#define ENABLESOFTSELECTION_CHUNK		0x690
#define CONSTANTUPDATE_CHUNK			0x700
#define APPLYTOWHOLEOBJECT_CHUNK			0x710

//5.1.05
#define AUTOBACKGROUND_CHUNK			0x720

#define THICKOPENEDGE_CHUNK			0x730
#define VIEWPORTOPENEDGE_CHUNK			0x740

#define ABSOLUTETYPEIN_CHUNK			0x750

#define STITCHSCALE_CHUNK			0x760
#define SEAM_CHUNK			0x770
#define VERSION_CHUNK			0x780
#define CURRENTMAP_CHUNK			0x790
#define GEDGESELECTION_CHUNK			0x800
#define UEDGESELECTION_CHUNK			0x810
#define FACESELECTION_CHUNK			0x820
#define RELAX_CHUNK			0x830
#define FALLOFFSPACE_CHUNK			0x840
#define SHOWPELTSEAMS_CHUNK			0x850


IOResult UnwrapMod::Save(ISave *isave)
{
	ULONG nb;
	Modifier::Save(isave);
	version = CURRENTVERSION;
	

	int vct = TVMaps.v.Count(), fct = TVMaps.f.Count();

	isave->BeginChunk(VERTCOUNT_CHUNK);
	isave->Write(&vct, sizeof(vct), &nb);
	isave->EndChunk();

	if (vct) {
		isave->BeginChunk(VERTS2_CHUNK);
		isave->Write(TVMaps.v.Addr(0), sizeof(UVW_TVVertClass)*vct, &nb);
		isave->EndChunk();
	}

	isave->BeginChunk(FACECOUNT_CHUNK);
	isave->Write(&fct, sizeof(fct), &nb);
	isave->EndChunk();

	if (fct) {
		isave->BeginChunk(FACE4_CHUNK);
		TVMaps.SaveFaces(isave);
		isave->EndChunk();
		//		isave->BeginChunk(FACE4_CHUNK);
		//		isave->Write(TVMaps.f.Addr(0), sizeof(UVW_TVFaceClass)*fct, &nb);
		//		isave->EndChunk();
	}

	isave->BeginChunk(VERTSEL_CHUNK);
	vsel.Save(isave);
	isave->EndChunk();

	isave->BeginChunk(ZOOM_CHUNK);
	isave->Write(&zoom, sizeof(zoom), &nb);
	isave->EndChunk();

	isave->BeginChunk(ASPECT_CHUNK);
	isave->Write(&aspect, sizeof(aspect), &nb);
	isave->EndChunk();

	isave->BeginChunk(XSCROLL_CHUNK);
	isave->Write(&xscroll, sizeof(xscroll), &nb);
	isave->EndChunk();

	isave->BeginChunk(YSCROLL_CHUNK);
	isave->Write(&yscroll, sizeof(yscroll), &nb);
	isave->EndChunk();

	isave->BeginChunk(IWIDTH_CHUNK);
	isave->Write(&rendW, sizeof(rendW), &nb);
	isave->EndChunk();

	isave->BeginChunk(IHEIGHT_CHUNK);
	isave->Write(&rendH, sizeof(rendH), &nb);
	isave->EndChunk();

	isave->BeginChunk(UVW_CHUNK);
	isave->Write(&uvw, sizeof(uvw), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHOWMAP_CHUNK);
	isave->Write(&showMap, sizeof(showMap), &nb);
	isave->EndChunk();

	isave->BeginChunk(UPDATE_CHUNK);
	isave->Write(&update, sizeof(update), &nb);
	isave->EndChunk();

	isave->BeginChunk(LINECOLOR_CHUNK);
	isave->Write(&lineColor, sizeof(lineColor), &nb);
	isave->EndChunk();

	isave->BeginChunk(SELCOLOR_CHUNK);
	isave->Write(&selColor, sizeof(selColor), &nb);
	isave->EndChunk();

	isave->BeginChunk(CHANNEL_CHUNK);
	isave->Write(&channel, sizeof(channel), &nb);
	isave->EndChunk();

	isave->BeginChunk(PREFS_CHUNK);
	isave->Write(&lineColor, sizeof(lineColor), &nb);
	isave->Write(&selColor, sizeof(selColor), &nb);
	isave->Write(&weldThreshold, sizeof(weldThreshold), &nb);
	isave->Write(&update, sizeof(update), &nb);
	isave->Write(&showVerts, sizeof(showVerts), &nb);
	isave->Write(&midPixelSnap, sizeof(midPixelSnap), &nb);
	isave->EndChunk();

	if (namedSel.Count()) {
		isave->BeginChunk(0x2806);			
		for (int i=0; i<namedSel.Count(); i++) 
		{
			isave->BeginChunk(NAMEDSEL_STRING_CHUNK);
			isave->WriteWString(*namedSel[i]);
			isave->EndChunk();

			isave->BeginChunk(NAMEDSEL_ID_CHUNK);
			isave->Write(&ids[i],sizeof(DWORD),&nb);
			isave->EndChunk();
		}

		for (int i=0; i<namedVSel.Count(); i++) 
		{
			isave->BeginChunk(NAMEDVSEL_STRING_CHUNK);
			isave->WriteWString(*namedVSel[i]);
			isave->EndChunk();

			isave->BeginChunk(NAMEDVSEL_ID_CHUNK);
			isave->Write(&idsV[i],sizeof(DWORD),&nb);
			isave->EndChunk();
		}

		for (int i=0; i<namedESel.Count(); i++) 
		{
			isave->BeginChunk(NAMEDESEL_STRING_CHUNK);
			isave->WriteWString(*namedESel[i]);
			isave->EndChunk();

			isave->BeginChunk(NAMEDESEL_ID_CHUNK);
			isave->Write(&idsE[i],sizeof(DWORD),&nb);
			isave->EndChunk();
		}

		isave->EndChunk();
	}
	if (useBitmapRes)
	{
		isave->BeginChunk(USEBITMAPRES_CHUNK);
		isave->EndChunk();
	}

	fct = TVMaps.geomPoints.Count();
	isave->BeginChunk(GEOMPOINTSCOUNT_CHUNK);
	isave->Write(&fct, sizeof(fct), &nb);
	isave->EndChunk();

	if (fct) {
		isave->BeginChunk(GEOMPOINTS_CHUNK);
		isave->Write(TVMaps.geomPoints.Addr(0), sizeof(Point3)*fct, &nb);
		isave->EndChunk();
	}

	isave->BeginChunk(LOCKASPECT_CHUNK);
	isave->Write(&lockAspect, sizeof(lockAspect), &nb);
	isave->EndChunk();

	isave->BeginChunk(MAPSCALE_CHUNK);
	isave->Write(&mapScale, sizeof(mapScale), &nb);
	isave->EndChunk();

	isave->BeginChunk(WINDOWPOS_CHUNK);
	isave->Write(&windowPos, sizeof(WINDOWPLACEMENT), &nb);
	isave->EndChunk();

	isave->BeginChunk(FORCEUPDATE_CHUNK);
	isave->Write(&forceUpdate, sizeof(forceUpdate), &nb);
	isave->EndChunk();



	if (bTile)
	{
		isave->BeginChunk(TILE_CHUNK);
		isave->EndChunk();
	}

	isave->BeginChunk(TILECONTRAST_CHUNK);
	isave->Write(&fContrast, sizeof(fContrast), &nb);
	isave->EndChunk();

	isave->BeginChunk(TILELIMIT_CHUNK);
	isave->Write(&iTileLimit, sizeof(iTileLimit), &nb);
	isave->EndChunk();

	isave->BeginChunk(SOFTSELLIMIT_CHUNK);
	isave->Write(&limitSoftSel, sizeof(limitSoftSel), &nb);
	isave->Write(&limitSoftSelRange, sizeof(limitSoftSelRange), &nb);
	isave->EndChunk();


	isave->BeginChunk(FLATTENMAP_CHUNK);
	isave->Write(&flattenAngleThreshold, sizeof(flattenAngleThreshold), &nb);
	isave->Write(&flattenSpacing, sizeof(flattenSpacing), &nb);
	isave->Write(&flattenNormalize, sizeof(flattenNormalize), &nb);
	isave->Write(&flattenRotate, sizeof(flattenRotate), &nb);
	isave->Write(&flattenCollapse, sizeof(flattenCollapse), &nb);
	isave->EndChunk();

	isave->BeginChunk(NORMALMAP_CHUNK);
	isave->Write(&normalMethod, sizeof(normalMethod), &nb);
	isave->Write(&normalSpacing, sizeof(normalSpacing), &nb);
	isave->Write(&normalNormalize, sizeof(normalNormalize), &nb);
	isave->Write(&normalRotate, sizeof(normalRotate), &nb);
	isave->Write(&normalAlignWidth, sizeof(normalAlignWidth), &nb);
	isave->EndChunk();


	isave->BeginChunk(UNFOLDMAP_CHUNK);
	isave->Write(&unfoldMethod, sizeof(unfoldMethod), &nb);
	isave->EndChunk();


	isave->BeginChunk(STITCH_CHUNK);
	isave->Write(&bStitchAlign, sizeof(bStitchAlign), &nb);
	isave->Write(&fStitchBias, sizeof(fStitchBias), &nb);
	isave->EndChunk();


	isave->BeginChunk(GEOMELEM_CHUNK);
	isave->Write(&geomElemMode, sizeof(geomElemMode), &nb);
	isave->EndChunk();


	isave->BeginChunk(PLANARTHRESHOLD_CHUNK);
	isave->Write(&planarThreshold, sizeof(planarThreshold), &nb);
	isave->Write(&planarMode, sizeof(planarMode), &nb);
	isave->EndChunk();

	isave->BeginChunk(BACKFACECULL_CHUNK);
	isave->Write(&ignoreBackFaceCull, sizeof(ignoreBackFaceCull), &nb);
	isave->Write(&oldSelMethod, sizeof(oldSelMethod), &nb);
	isave->EndChunk();

	isave->BeginChunk(TVELEMENTMODE_CHUNK);
	isave->Write(&tvElementMode, sizeof(tvElementMode), &nb);
	isave->EndChunk();

	isave->BeginChunk(ALWAYSEDIT_CHUNK);
	isave->Write(&alwaysEdit, sizeof(alwaysEdit), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHOWCONNECTION_CHUNK);
	isave->Write(&showVertexClusterList, sizeof(showVertexClusterList), &nb);
	isave->EndChunk();

	isave->BeginChunk(PACK_CHUNK);
	isave->Write(&packMethod, sizeof(packMethod), &nb);
	isave->Write(&packSpacing, sizeof(packSpacing), &nb);
	isave->Write(&packNormalize, sizeof(packNormalize), &nb);
	isave->Write(&packRotate, sizeof(packRotate), &nb);
	isave->Write(&packFillHoles, sizeof(packFillHoles), &nb);
	isave->EndChunk();


	isave->BeginChunk(TVSUBOBJECTMODE_CHUNK);
	isave->Write(&TVSubObjectMode, sizeof(TVSubObjectMode), &nb);
	isave->EndChunk();


	isave->BeginChunk(FILLMODE_CHUNK);
	isave->Write(&fillMode, sizeof(fillMode), &nb);
	isave->EndChunk();

	isave->BeginChunk(OPENEDGECOLOR_CHUNK);
	isave->Write(&openEdgeColor, sizeof(openEdgeColor), &nb);
	isave->Write(&displayOpenEdges, sizeof(displayOpenEdges), &nb);
	isave->EndChunk();

	isave->BeginChunk(UVEDGEMODE_CHUNK);
	isave->Write(&uvEdgeMode, sizeof(uvEdgeMode), &nb);
	isave->Write(&openEdgeMode, sizeof(openEdgeMode), &nb);
	isave->Write(&displayHiddenEdges, sizeof(displayHiddenEdges), &nb);

	isave->EndChunk();

	isave->BeginChunk(MISCCOLOR_CHUNK);
	isave->Write(&handleColor, sizeof(handleColor), &nb);
	isave->Write(&freeFormColor, sizeof(freeFormColor), &nb);
	isave->EndChunk();

	isave->BeginChunk(HITSIZE_CHUNK);
	isave->Write(&hitSize, sizeof(hitSize), &nb);
	isave->EndChunk();

	isave->BeginChunk(PIVOT_CHUNK);
	isave->Write(&resetPivotOnSel, sizeof(resetPivotOnSel), &nb);
	isave->EndChunk();

	isave->BeginChunk(GIZMOSEL_CHUNK);
	isave->Write(&allowSelectionInsideGizmo, sizeof(allowSelectionInsideGizmo), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHARED_CHUNK);
	isave->Write(&showShared, sizeof(showShared), &nb);
	isave->Write(&sharedColor, sizeof(sharedColor), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHOWICON_CHUNK);
	showIconList.Save(isave);
	isave->EndChunk();

	isave->BeginChunk(SYNCSELECTION_CHUNK);
	isave->Write(&syncSelection, sizeof(syncSelection), &nb);
	isave->EndChunk();

	isave->BeginChunk(BRIGHTCENTER_CHUNK);
	isave->Write(&brightCenterTile, sizeof(brightCenterTile), &nb);
	isave->Write(&blendTileToBackGround, sizeof(blendTileToBackGround), &nb);
	isave->EndChunk();

	isave->BeginChunk(CURSORSIZE_CHUNK);
	isave->Write(&sketchCursorSize, sizeof(sketchCursorSize), &nb);
	isave->Write(&paintSize, sizeof(paintSize), &nb);
	isave->EndChunk();


	isave->BeginChunk(TICKSIZE_CHUNK);
	isave->Write(&tickSize, sizeof(tickSize), &nb);
	isave->EndChunk();

	//new
	isave->BeginChunk(GRID_CHUNK);
	isave->Write(&gridSize, sizeof(gridSize), &nb);
	isave->Write(&gridSnap, sizeof(gridSnap), &nb);
	isave->Write(&gridVisible, sizeof(gridVisible), &nb);
	isave->Write(&gridColor, sizeof(gridColor), &nb);
	isave->Write(&gridStr, sizeof(gridStr), &nb);
	isave->Write(&autoMap, sizeof(autoMap), &nb);
	isave->EndChunk();

	isave->BeginChunk(PREVENTFLATTENING_CHUNK);
	isave->Write(&preventFlattening, sizeof(preventFlattening), &nb);
	isave->EndChunk();

	isave->BeginChunk(ENABLESOFTSELECTION_CHUNK);
	isave->Write(&enableSoftSelection, sizeof(enableSoftSelection), &nb);
	isave->EndChunk();


	isave->BeginChunk(CONSTANTUPDATE_CHUNK);
	isave->Write(&update, sizeof(update), &nb);
	isave->Write(&loadDefaults, sizeof(loadDefaults), &nb);	
	isave->EndChunk();

	isave->BeginChunk(APPLYTOWHOLEOBJECT_CHUNK);
	isave->Write(&applyToWholeObject, sizeof(applyToWholeObject), &nb);
	isave->EndChunk();

	isave->BeginChunk(THICKOPENEDGE_CHUNK);
	isave->Write(&thickOpenEdges, sizeof(thickOpenEdges), &nb);
	isave->EndChunk();	

	isave->BeginChunk(VIEWPORTOPENEDGE_CHUNK);
	isave->Write(&viewportOpenEdges, sizeof(viewportOpenEdges), &nb);
	isave->EndChunk();	

	isave->BeginChunk(ABSOLUTETYPEIN_CHUNK);
	isave->Write(&absoluteTypeIn, sizeof(absoluteTypeIn), &nb);
	isave->EndChunk();	

	isave->BeginChunk(STITCHSCALE_CHUNK);
	isave->Write(&bStitchScale, sizeof(bStitchScale), &nb);
	isave->EndChunk();	

	isave->BeginChunk(SEAM_CHUNK);
	peltData.seamEdges.Save(isave);
	isave->EndChunk();	

	isave->BeginChunk(VERSION_CHUNK);
	isave->Write(&version, sizeof(version), &nb);
	isave->EndChunk();

	isave->BeginChunk(CURRENTMAP_CHUNK);
	isave->Write(&CurrentMap, sizeof(CurrentMap), &nb);
	isave->EndChunk();

	isave->BeginChunk(FACESELECTION_CHUNK);
	fsel.Save(isave);
	isave->EndChunk();

	isave->BeginChunk(UEDGESELECTION_CHUNK);
	esel.Save(isave);
	isave->EndChunk();

	isave->BeginChunk(GEDGESELECTION_CHUNK);
	gesel.Save(isave);
	isave->EndChunk();


	isave->BeginChunk(RELAX_CHUNK);
	isave->Write(&relaxAmount, sizeof(relaxAmount), &nb);
	isave->Write(&relaxStretch, sizeof(relaxStretch), &nb);
	isave->Write(&relaxIteration, sizeof(relaxIteration), &nb);
	isave->Write(&relaxType, sizeof(relaxType), &nb);
	isave->Write(&relaxBoundary, sizeof(relaxBoundary), &nb);
	isave->Write(&relaxSaddle, sizeof(relaxSaddle), &nb);
	isave->EndChunk();

	isave->BeginChunk(FALLOFFSPACE_CHUNK);
	isave->Write(&falloffSpace, sizeof(falloffSpace), &nb);
	isave->Write(&falloffStr, sizeof(falloffStr), &nb);
	isave->EndChunk();

	isave->BeginChunk(SHOWPELTSEAMS_CHUNK);
	isave->Write(&alwaysShowSeams, sizeof(alwaysShowSeams), &nb);
	isave->EndChunk();	

	return IO_OK;
}

#define UVWVER	4

void UnwrapMod::LoadUVW(HWND hWnd)
{
	static TCHAR fname[256] = {'\0'};
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	FilterList fl;
	fl.Append( GetString(IDS_PW_UVWFILES));
	fl.Append( _T("*.uvw"));		
	TSTR title = GetString(IDS_PW_LOADOBJECT);

	ofn.lStructSize     = sizeof(OPENFILENAME);  // No OFN_ENABLEHOOK
	ofn.hwndOwner       = GetCOREInterface()->GetMAXHWnd();
	ofn.lpstrFilter     = fl;
	ofn.lpstrFile       = fname;
	ofn.nMaxFile        = 256;    
	//ofn.lpstrInitialDir = ip->GetDir(APP_EXPORT_DIR);
	ofn.Flags           = OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
	ofn.FlagsEx         = OFN_EX_NOPLACESBAR;
	ofn.lpstrDefExt     = _T("uvw");
	ofn.lpstrTitle      = title;


	if (GetOpenFileName(&ofn)) {

		theHold.SuperBegin();
		theHold.Begin();
		HoldPointsAndFaces();	



		//load stuff here  stuff here
		FILE *file = fopen(fname,_T("rb"));

		for (int i = 0; i < TVMaps.cont.Count(); i++)
		{
			if (TVMaps.cont[i]) 
			{
				for (int i=0; i<TVMaps.cont.Count(); i++) DeleteReference(i+11+100);
				//			TVMaps.cont[i]->DeleteThis();
			}
		}

		int vct = TVMaps.v.Count(), fct = TVMaps.f.Count();

		//check if oldversion
		int ver = 3;

		fread(&ver, sizeof(ver), 1,file);
		if (ver == -1)
		{
			fread(&ver, sizeof(ver), 1,file);
			fread(&vct, sizeof(vct), 1,file);
		}
		else
		{
			vct = ver;
			ver = 3;
		}
		//	fread(&vct, sizeof(vct), 1,file);

		TVMaps.v.SetCount(vct);
		vsel.SetSize(vct);

		TVMaps.cont.SetCount(vct);
   for (int i = 0; i < vct; i++)
			TVMaps.cont[i] = NULL;

		if (vct) {
			fread(TVMaps.v.Addr(0), sizeof(UVW_TVVertClass)*vct, 1,file);
		}

		fread(&fct, sizeof(fct), 1,file);
		TVMaps.SetCountFaces(fct);
		if ((fct) && (ver < 4)) {
			//fix me old data
			Tab<UVW_TVFaceOldClass> oldData;
			oldData.SetCount(fct);
			fread(oldData.Addr(0), sizeof(UVW_TVFaceOldClass)*fct, 1,file);
			for (int i = 0; i < fct; i++)
			{
				TVMaps.f[i]->t[0] = oldData[i].t[0];
				TVMaps.f[i]->t[1] = oldData[i].t[1];
				TVMaps.f[i]->t[2] = oldData[i].t[2];
				TVMaps.f[i]->t[3] = oldData[i].t[3];
				TVMaps.f[i]->FaceIndex = oldData[i].FaceIndex;
				TVMaps.f[i]->MatID = oldData[i].MatID;
				TVMaps.f[i]->flags = oldData[i].flags;
				TVMaps.f[i]->vecs = NULL;
				if (TVMaps.f[i]->flags & 8)  // not this was FLAG_QUAD but this define got removed
					TVMaps.f[i]->count=4;
				else TVMaps.f[i]->count=3;

			}
			//now compute the geom points
      for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				int pcount = 3;
				pcount = TVMaps.f[i]->count;

				for (int j =0; j < pcount; j++)
				{
					BOOL found = FALSE;
					int index;
					for (int k =0; k < TVMaps.geomPoints.Count();k++)
					{
						if (oldData[i].pt[j] == TVMaps.geomPoints[k])
						{
							found = TRUE;
							index = k;
							k = TVMaps.geomPoints.Count();
						}
					}
					if (found)
					{
						TVMaps.f[i]->v[j] = index;
					}
					else
					{
						TVMaps.f[i]->v[j] = TVMaps.geomPoints.Count();
						TVMaps.geomPoints.Append(1,&oldData[i].pt[j],1);
					}
				}

			}

		}
		else
		{
			TVMaps.LoadFaces(file);
		}

		fclose(file);

		theHold.Accept(_T(GetString(IDS_PW_LOADOBJECT)));
		theHold.SuperAccept(_T(GetString(IDS_PW_LOADOBJECT)));


		TVMaps.edgesValid = FALSE;


	}

}
void UnwrapMod::SaveUVW(HWND hWnd)
{
	static TCHAR fname[256] = {'\0'};
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	FilterList fl;
	fl.Append( GetString(IDS_PW_UVWFILES));
	fl.Append( _T("*.uvw"));		
	TSTR title = GetString(IDS_PW_SAVEOBJECT);

	ofn.lStructSize     = sizeof(OPENFILENAME);  // No OFN_ENABLEHOOK
	ofn.hwndOwner       = GetCOREInterface()->GetMAXHWnd();
	ofn.lpstrFilter     = fl;
	ofn.lpstrFile       = fname;
	ofn.nMaxFile        = 256;    
	//ofn.lpstrInitialDir = ip->GetDir(APP_EXPORT_DIR);
	ofn.Flags           = OFN_HIDEREADONLY|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
	ofn.FlagsEx         = OFN_EX_NOPLACESBAR;
	ofn.lpstrDefExt     = _T("uvw");
	ofn.lpstrTitle      = title;

tryAgain:
	if (GetSaveFileName(&ofn)) {
		if (DoesFileExist(fname)) {
			TSTR buf1;
			TSTR buf2 = GetString(IDS_PW_SAVEOBJECT);
			buf1.printf(GetString(IDS_PW_FILEEXISTS),fname);
			if (IDYES!=MessageBox(
				hParams,
				buf1,buf2,MB_YESNO|MB_ICONQUESTION)) {
					goto tryAgain;
				}
		}
		//save stuff here
		// this is timed slice so it will not save animation not sure how to save controller info but will neeed to later on in other plugs

		FILE *file = fopen(fname,_T("wb"));

		int ver = -1;
		fwrite(&ver, sizeof(ver), 1,file);
		ver = UVWVER;
		fwrite(&ver, sizeof(ver), 1,file);

		int vct = TVMaps.v.Count(), fct = TVMaps.f.Count();

		fwrite(&vct, sizeof(vct), 1,file);

		if (vct) {
			fwrite(TVMaps.v.Addr(0), sizeof(UVW_TVVertClass)*vct, 1,file);
		}

		fwrite(&fct, sizeof(fct), 1,file);

		if (fct) {
			//		fwrite(TVMaps.f.Addr(0), sizeof(UVW_TVFaceClass)*fct, 1,file);
			TVMaps.SaveFaces(file);
		}

		fclose(file);
	}

}


IOResult UnwrapMod::LoadNamedSelChunk(ILoad *iload) {	
	IOResult res;
	DWORD ix=0;
	ULONG nb;

	while (IO_OK==(res=iload->OpenChunk())) 
	{
		switch(iload->CurChunkID())  
		{
			case NAMEDSEL_STRING_CHUNK: 
				{
					TCHAR *name;
					res = iload->ReadWStringChunk(&name);
					//AddSet(TSTR(name),level+1);
					TSTR *newName = new TSTR(name);
					namedSel.Append(1,&newName);				
					ids.Append(1,&ix);
					ix++;
					break;
				}
			case NAMEDSEL_ID_CHUNK:
				iload->Read(&ids[ids.Count()-1],sizeof(DWORD), &nb);
				break;

			case NAMEDVSEL_STRING_CHUNK: 
				{
					TCHAR *name;
					res = iload->ReadWStringChunk(&name);
					//AddSet(TSTR(name),level+1);
					TSTR *newName = new TSTR(name);
					namedVSel.Append(1,&newName);				
					idsV.Append(1,&ix);
					ix++;
					break;
				}
			case NAMEDVSEL_ID_CHUNK:
				iload->Read(&idsV[ids.Count()-1],sizeof(DWORD), &nb);
				break;

			case NAMEDESEL_STRING_CHUNK: 
				{
					TCHAR *name;
					res = iload->ReadWStringChunk(&name);
					//AddSet(TSTR(name),level+1);
					TSTR *newName = new TSTR(name);
					namedESel.Append(1,&newName);				
					idsE.Append(1,&ix);
					ix++;
					break;
				}
			case NAMEDESEL_ID_CHUNK:
				iload->Read(&idsE[ids.Count()-1],sizeof(DWORD), &nb);
				break;

		}
		iload->CloseChunk();
		if (res!=IO_OK) return res;
	}
	return IO_OK;
}


class UnwrapPostLoadCallback:public  PostLoadCallback
{
public:
	UnwrapMod      *s;

	int oldData;
	UnwrapPostLoadCallback(UnwrapMod *r, BOOL b) {s=r;oldData = b;}
	void proc(ILoad *iload);
};

void UnwrapPostLoadCallback::proc(ILoad *iload)
{
	if (!oldData)
	{
		//		for (int i=0; i<10; i++) 
		//			s->ReplaceReference(i+1,NULL);
	}
	delete this;
}



IOResult UnwrapMod::Load(ILoad *iload)
{
	popUpDialog = FALSE;
	version = 2;
	IOResult res;
	ULONG nb;
	Modifier::Load(iload);
   int ct;
	//check for backwards compatibility
	useBitmapRes = FALSE;

	Tab<UVW_TVFaceOldClass> oldData;


	bTile = FALSE;

	TVMaps.edgesValid = FALSE;

	//5.1.05
	this->autoBackground = FALSE;


	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {

			case CURRENTMAP_CHUNK:
				iload->Read(&CurrentMap, sizeof(CurrentMap), &nb);				
				break;

			case VERSION_CHUNK:
				iload->Read(&version, sizeof(version), &nb);				
				break;
			//5.1.05
			case AUTOBACKGROUND_CHUNK:
				this->autoBackground = TRUE;
				break;

			case 0x2806:
				res = LoadNamedSelChunk(iload);
				break;

			case VERTCOUNT_CHUNK:
				iload->Read(&ct, sizeof(ct), &nb);
				TVMaps.v.SetCount(ct);
				TVMaps.cont.SetCount(ct);
				vsel.SetSize(ct);
            for (int i=0; i<ct; i++) TVMaps.cont[i] = NULL;          
				break;
			case FACECOUNT_CHUNK:
				iload->Read(&ct, sizeof(ct), &nb);
				TVMaps.SetCountFaces(ct);
				break;
			case GEOMPOINTSCOUNT_CHUNK:
				iload->Read(&ct, sizeof(ct), &nb);
				TVMaps.geomPoints.SetCount(ct);
				break;

				//old way here for legacy reason only
			case FACE_CHUNK:
				{
					version = 1;
					oldDataPresent = TRUE;
					Tab<TVFace> f;
					f.SetCount(TVMaps.f.Count());
					iload->Read(f.Addr(0), sizeof(TVFace)*TVMaps.f.Count(), &nb);
            for (int i=0;i<TVMaps.f.Count();i++)
					{

						TVMaps.f[i]->count  = 3;
						TVMaps.f[i]->t = new int[TVMaps.f[i]->count];
						TVMaps.f[i]->v = new int[TVMaps.f[i]->count];

						TVMaps.f[i]->t[0] = f[i].t[0];
						TVMaps.f[i]->t[1] = f[i].t[1];
						TVMaps.f[i]->t[2] = f[i].t[2];
						TVMaps.f[i]->flags = 0;
						TVMaps.f[i]->vecs = NULL;
					}

					break;
				}
				//old way here for legacy reason only
			case VERTS_CHUNK:
				{
					Tab<Point3> p;
					p.SetCount(TVMaps.v.Count());
					oldDataPresent = TRUE;

					iload->Read(p.Addr(0), sizeof(Point3)*TVMaps.v.Count(), &nb);

            for (int i=0;i<TVMaps.v.Count();i++)
					{
						TVMaps.v[i].p = p[i];
						TVMaps.v[i].flags = 0;
						TVMaps.v[i].influence = 0.0f;
					}
					break;
				}
			case FACE2_CHUNK:

				oldData.SetCount(TVMaps.f.Count());
				iload->Read(oldData.Addr(0), sizeof(UVW_TVFaceOldClass)*oldData.Count(), &nb);
            for (int i = 0; i < TVMaps.f.Count(); i++)
				{
					//fix for bug 281118 it was checking an uniitiliazed flag
					if (oldData[i].flags & 8)  // not this was FLAG_QUAD but this define got removed
						TVMaps.f[i]->count=4;
					else TVMaps.f[i]->count=3;

					TVMaps.f[i]->t = new int[TVMaps.f[i]->count];
					TVMaps.f[i]->v = new int[TVMaps.f[i]->count];

					for (int j = 0; j < TVMaps.f[i]->count; j++)
						TVMaps.f[i]->t[j] = oldData[i].t[j];
					/*					TVMaps.f[i]->t[1] = oldData[i].t[1];
					TVMaps.f[i]->t[2] = oldData[i].t[2];
					TVMaps.f[i]->t[3] = oldData[i].t[3];
					*/
					TVMaps.f[i]->FaceIndex = oldData[i].FaceIndex;
					TVMaps.f[i]->MatID = oldData[i].MatID;
					TVMaps.f[i]->flags = oldData[i].flags;
					TVMaps.f[i]->vecs = NULL;


				}
				//now compute the geom points
            for (int i = 0; i < TVMaps.f.Count(); i++)
				{
					int pcount = 3;
					//					if (TVMaps.f[i].flags & FLAG_QUAD) pcount = 4;
					pcount = TVMaps.f[i]->count;

					for (int j =0; j < pcount; j++)
					{
						BOOL found = FALSE;
						int index;
						for (int k =0; k < TVMaps.geomPoints.Count();k++)
						{
							if (oldData[i].pt[j] == TVMaps.geomPoints[k])
							{
								found = TRUE;
								index = k;
								k = TVMaps.geomPoints.Count();
							}
						}
						if (found)
						{
							TVMaps.f[i]->v[j] = index;
						}
						else
						{
							TVMaps.f[i]->v[j] = TVMaps.geomPoints.Count();
							TVMaps.geomPoints.Append(1,&oldData[i].pt[j],1);
						}
					}

				}

				break;

			case FACE4_CHUNK:
				TVMaps.LoadFaces(iload);	
				//				iload->Read(TVMaps.f.Addr(0), sizeof(UVW_TVFaceClass)*TVMaps.f.Count(), &nb);

				break;

			case VERTS2_CHUNK:
				iload->Read(TVMaps.v.Addr(0), sizeof(UVW_TVVertClass)*TVMaps.v.Count(), &nb);
				break;
			case GEOMPOINTS_CHUNK:
				iload->Read(TVMaps.geomPoints.Addr(0), sizeof(Point3)*TVMaps.geomPoints.Count(), &nb);
				break;
			case VERTSEL_CHUNK:
				vsel.Load(iload);
				break;
			case ZOOM_CHUNK:
				iload->Read(&zoom, sizeof(zoom), &nb);
				break;
			case ASPECT_CHUNK:
				iload->Read(&aspect, sizeof(aspect), &nb);
				break;
			case LOCKASPECT_CHUNK:
				iload->Read(&lockAspect, sizeof(lockAspect), &nb);
				break;
			case MAPSCALE_CHUNK:
				iload->Read(&mapScale, sizeof(mapScale), &nb);
				break;

			case XSCROLL_CHUNK:
				iload->Read(&xscroll, sizeof(xscroll), &nb);
				break;
			case YSCROLL_CHUNK:
				iload->Read(&yscroll, sizeof(yscroll), &nb);
				break;
			case IWIDTH_CHUNK:
				iload->Read(&rendW, sizeof(rendW), &nb);
				break;
			case IHEIGHT_CHUNK:
				iload->Read(&rendH, sizeof(rendH), &nb);
				break;
			case SHOWMAP_CHUNK:
				iload->Read(&showMap, sizeof(showMap), &nb);
				break;
			case UPDATE_CHUNK:
				iload->Read(&update, sizeof(update), &nb);
				break;
			case LINECOLOR_CHUNK:
				iload->Read(&lineColor, sizeof(lineColor), &nb);
				break;
			case SELCOLOR_CHUNK:
				iload->Read(&selColor, sizeof(selColor), &nb);
				break;			
			case UVW_CHUNK:
				iload->Read(&uvw, sizeof(uvw), &nb);
				break;
			case CHANNEL_CHUNK:
				iload->Read(&channel, sizeof(channel), &nb);
				break;			
			case WINDOWPOS_CHUNK:
				iload->Read(&windowPos, sizeof(WINDOWPLACEMENT), &nb);
				break;			
			case PREFS_CHUNK:
				iload->Read(&lineColor, sizeof(lineColor), &nb);
				iload->Read(&selColor, sizeof(selColor), &nb);
				iload->Read(&weldThreshold, sizeof(weldThreshold), &nb);
				iload->Read(&update, sizeof(update), &nb);
				iload->Read(&showVerts, sizeof(showVerts), &nb);
				iload->Read(&midPixelSnap, sizeof(midPixelSnap), &nb);
				break;
			case USEBITMAPRES_CHUNK:
				useBitmapRes = TRUE;
				break;			

			case FORCEUPDATE_CHUNK:
				break;
				//tile stuff
			case TILE_CHUNK:
				bTile = TRUE;
				break;			
			case TILECONTRAST_CHUNK:
				iload->Read(&fContrast, sizeof(fContrast), &nb);
				break;			
			case TILELIMIT_CHUNK:
				iload->Read(&iTileLimit, sizeof(iTileLimit), &nb);
				break;			
			case SOFTSELLIMIT_CHUNK:
				iload->Read(&limitSoftSel, sizeof(limitSoftSel), &nb);
				iload->Read(&limitSoftSelRange, sizeof(limitSoftSelRange), &nb);
				break;			
			case FLATTENMAP_CHUNK:
				iload->Read(&flattenAngleThreshold, sizeof(flattenAngleThreshold), &nb);
				iload->Read(&flattenSpacing, sizeof(flattenSpacing), &nb);
				iload->Read(&flattenNormalize, sizeof(flattenNormalize), &nb);
				iload->Read(&flattenRotate, sizeof(flattenRotate), &nb);
				iload->Read(&flattenCollapse, sizeof(flattenCollapse), &nb);
				break;			
			case NORMALMAP_CHUNK:
				iload->Read(&normalMethod, sizeof(normalMethod), &nb);
				iload->Read(&normalSpacing, sizeof(normalSpacing), &nb);
				iload->Read(&normalNormalize, sizeof(normalNormalize), &nb);
				iload->Read(&normalRotate, sizeof(normalRotate), &nb);
				iload->Read(&normalAlignWidth, sizeof(normalAlignWidth), &nb);
				break;			

			case UNFOLDMAP_CHUNK:
				iload->Read(&unfoldMethod, sizeof(unfoldMethod), &nb);
				break;			

			case STITCH_CHUNK:
				iload->Read(&bStitchAlign, sizeof(bStitchAlign), &nb);
				iload->Read(&fStitchBias, sizeof(fStitchBias), &nb);
				break;	

			case STITCHSCALE_CHUNK:
				iload->Read(&bStitchScale, sizeof(bStitchScale), &nb);
				break;

			case GEOMELEM_CHUNK:
				iload->Read(&geomElemMode, sizeof(geomElemMode), &nb);
				break;			
			case PLANARTHRESHOLD_CHUNK:
				iload->Read(&planarThreshold, sizeof(planarThreshold), &nb);
				iload->Read(&planarMode, sizeof(planarMode), &nb);
				break;			
			case BACKFACECULL_CHUNK:
				iload->Read(&ignoreBackFaceCull, sizeof(ignoreBackFaceCull), &nb);
				iload->Read(&oldSelMethod, sizeof(oldSelMethod), &nb);
				break;			
			case TVELEMENTMODE_CHUNK:
				iload->Read(&tvElementMode, sizeof(tvElementMode), &nb);
				break;			
			case ALWAYSEDIT_CHUNK:
				iload->Read(&alwaysEdit, sizeof(alwaysEdit), &nb);
				break;			

			case SHOWCONNECTION_CHUNK:
				iload->Read(&showVertexClusterList, sizeof(showVertexClusterList), &nb);
				break;			
			case PACK_CHUNK:
				iload->Read(&packMethod, sizeof(packMethod), &nb);
				iload->Read(&packSpacing, sizeof(packSpacing), &nb);
				iload->Read(&packNormalize, sizeof(packNormalize), &nb);
				iload->Read(&packRotate, sizeof(packRotate), &nb);
				iload->Read(&packFillHoles, sizeof(packFillHoles), &nb);
				break;			
			case TVSUBOBJECTMODE_CHUNK:
				iload->Read(&TVSubObjectMode, sizeof(TVSubObjectMode), &nb);
				break;			
			case FILLMODE_CHUNK:
				iload->Read(&fillMode, sizeof(fillMode), &nb);
				break;			

			case OPENEDGECOLOR_CHUNK:
				iload->Read(&openEdgeColor, sizeof(openEdgeColor), &nb);
				iload->Read(&displayOpenEdges, sizeof(displayOpenEdges), &nb);
				break;
			case THICKOPENEDGE_CHUNK:
				iload->Read(&thickOpenEdges, sizeof(thickOpenEdges), &nb);
				break;
			case VIEWPORTOPENEDGE_CHUNK:
				iload->Read(&viewportOpenEdges, sizeof(viewportOpenEdges), &nb);
				break;
			case UVEDGEMODE_CHUNK:
				iload->Read(&uvEdgeMode, sizeof(uvEdgeMode), &nb);
				iload->Read(&openEdgeMode, sizeof(openEdgeMode), &nb);
				iload->Read(&displayHiddenEdges, sizeof(displayHiddenEdges), &nb);
				break;
			case MISCCOLOR_CHUNK:
				iload->Read(&handleColor, sizeof(handleColor), &nb);
				iload->Read(&freeFormColor, sizeof(freeFormColor), &nb);
				break;

			case HITSIZE_CHUNK:
				iload->Read(&hitSize, sizeof(hitSize), &nb);
				break;
			case PIVOT_CHUNK:
				iload->Read(&resetPivotOnSel, sizeof(resetPivotOnSel), &nb);
				break;
			case GIZMOSEL_CHUNK:
				iload->Read(&allowSelectionInsideGizmo, sizeof(allowSelectionInsideGizmo), &nb);
				break;
			case SHARED_CHUNK:
				iload->Read(&showShared, sizeof(showShared), &nb);
				iload->Read(&sharedColor, sizeof(sharedColor), &nb);
				break;
			case SHOWICON_CHUNK:
				showIconList.Load(iload);

				break;
			case SYNCSELECTION_CHUNK:
				iload->Read(&syncSelection, sizeof(syncSelection), &nb);
				break;
			case BRIGHTCENTER_CHUNK:
				iload->Read(&brightCenterTile, sizeof(brightCenterTile), &nb);
				iload->Read(&blendTileToBackGround, sizeof(blendTileToBackGround), &nb);
				break;

			case CURSORSIZE_CHUNK:
				iload->Read(&sketchCursorSize, sizeof(sketchCursorSize), &nb);
				iload->Read(&paintSize, sizeof(paintSize), &nb);
				break;


			case TICKSIZE_CHUNK:
				iload->Read(&tickSize, sizeof(tickSize), &nb);
				break;
				//new
			case GRID_CHUNK:
				iload->Read(&gridSize, sizeof(gridSize), &nb);
				iload->Read(&gridSnap, sizeof(gridSnap), &nb);
				iload->Read(&gridVisible, sizeof(gridVisible), &nb);
				iload->Read(&gridColor, sizeof(gridColor), &nb);
				iload->Read(&gridStr, sizeof(gridStr), &nb);
				iload->Read(&autoMap, sizeof(autoMap), &nb);
				break;

			case PREVENTFLATTENING_CHUNK:
				iload->Read(&preventFlattening, sizeof(preventFlattening), &nb);
				break;

			case ENABLESOFTSELECTION_CHUNK:
				iload->Read(&enableSoftSelection, sizeof(enableSoftSelection), &nb);
				break;

			case CONSTANTUPDATE_CHUNK:
				iload->Read(&update, sizeof(update), &nb);
				iload->Read(&loadDefaults, sizeof(loadDefaults), &nb);
				break;
			case APPLYTOWHOLEOBJECT_CHUNK:
				iload->Read(&applyToWholeObject, sizeof(applyToWholeObject), &nb);
				break;

			case ABSOLUTETYPEIN_CHUNK:
				iload->Read(&absoluteTypeIn, sizeof(absoluteTypeIn), &nb);
				break;

			case SEAM_CHUNK:
				peltData.seamEdges.Load(iload);
				break;
			case FACESELECTION_CHUNK:

				fsel.Load(iload);
				break;
			case GEDGESELECTION_CHUNK:
				gesel.Load(iload);
				break;
			case UEDGESELECTION_CHUNK:
				esel.Load(iload);
				break;
			case RELAX_CHUNK:
				iload->Read(&relaxAmount, sizeof(relaxAmount), &nb);
				iload->Read(&relaxStretch, sizeof(relaxStretch), &nb);
				iload->Read(&relaxIteration, sizeof(relaxIteration), &nb);
				iload->Read(&relaxType, sizeof(relaxType), &nb);
				iload->Read(&relaxBoundary, sizeof(relaxBoundary), &nb);
				iload->Read(&relaxSaddle, sizeof(relaxSaddle), &nb);
				break;
			case FALLOFFSPACE_CHUNK:
				iload->Read(&falloffSpace, sizeof(falloffSpace), &nb);
				iload->Read(&falloffStr, sizeof(falloffStr), &nb);
				break;

			case SHOWPELTSEAMS_CHUNK:
				iload->Read(&alwaysShowSeams, sizeof(alwaysShowSeams), &nb);
				break;






		}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
	}

	BuildMatIDList();

	UnwrapPostLoadCallback* unwrapplcb = new UnwrapPostLoadCallback(this,oldDataPresent);
	iload->RegisterPostLoadCallback(unwrapplcb);

	//	TVMaps.Dump();
	TVMaps.MarkDeadVertices();
	return IO_OK;
}

#define FACESEL_CHUNKID			0x0210
#define FSELSET_CHUNK			0x2846


#define ESELSET_CHUNK			0x2847


#define VSELSET_CHUNK			0x2848

IOResult UnwrapMod::SaveLocalData(ISave *isave, LocalModData *ld) {	
	MeshTopoData *d = (MeshTopoData*)ld;

	if (d == NULL)
		return IO_OK;

	isave->BeginChunk(FACESEL_CHUNKID);
	d->faceSel.Save(isave);
	isave->EndChunk();

	if (d->fselSet.Count()) {
		isave->BeginChunk(FSELSET_CHUNK);
		d->fselSet.Save(isave);
		isave->EndChunk();
	}



	if (d->vselSet.Count()) {
		isave->BeginChunk(VSELSET_CHUNK);
		d->vselSet.Save(isave);
		isave->EndChunk();
	}

	if (d->eselSet.Count()) {
		isave->BeginChunk(ESELSET_CHUNK);
		d->eselSet.Save(isave);
		isave->EndChunk();
	}


	return IO_OK;
}

IOResult UnwrapMod::LoadLocalData(ILoad *iload, LocalModData **pld) {
	MeshTopoData *d = new MeshTopoData;
	*pld = d;
	IOResult res;	
	while (IO_OK==(res=iload->OpenChunk())) 
	{
		switch(iload->CurChunkID())  
		{
			case FACESEL_CHUNKID:
			d->faceSel.Load(iload);
			break;
			case FSELSET_CHUNK:
			res = d->fselSet.Load(iload);
			break;
			case VSELSET_CHUNK:
			res = d->vselSet.Load(iload);
			break;
			case ESELSET_CHUNK:
			res = d->eselSet.Load(iload);
			break;
		}
		iload->CloseChunk();
		if (res!=IO_OK) return res;
	}
	return IO_OK;
}


/*
void UnwrapMod::SynchWithMesh(Mesh &mesh)
{
int ct=0;
if (mesh.selLevel==MESH_FACE) {
for (int i=0; i<mesh.getNumFaces(); i++) {
if (mesh.faceSel[i]) ct++;
}
} else {
ct = mesh.getNumFaces();
}
if (ct != tvFace.Count()) {
DeleteAllRefsFromMe();
tvert.Resize(0);
cont.Resize(0);
tvFace.SetCount(ct);

TVFace *tvFaceM = mesh.tvFace;
Point3 *tVertsM = mesh.tVerts;
int numTV = channel ? mesh.getNumVertCol() : mesh.getNumTVerts();
if (channel) {
tvFaceM = mesh.vcFace;
tVertsM = mesh.vertCol;
}

if (mesh.selLevel==MESH_FACE) {
// Mark tverts that are used by selected faces
BitArray used;
if (tvFaceM) used.SetSize(numTV);
else used.SetSize(mesh.getNumVerts());
for (int i=0; i<mesh.getNumFaces(); i++) {
if (mesh.faceSel[i]) {
if (tvFaceM) {
for (int j=0; j<3; j++) 
used.Set(tvFaceM[i].t[j]);
} else {
for (int j=0; j<3; j++) 
used.Set(mesh.faces[i].v[j]);
}
}
}

// Now build a vmap
Tab<DWORD> vmap;
vmap.SetCount(used.GetSize());
int ix=0;
         for (int i=0; i<used.GetSize(); i++) {
if (used[i]) vmap[i] = ix++;
else vmap[i] = UNDEFINED;				
}

// Copy in tverts
tvert.SetCount(ix);
cont.SetCount(ix);
vsel.SetSize(ix);
ix = 0;
Box3 box = mesh.getBoundingBox();
         for (int i=0; i<used.GetSize(); i++) {
if (used[i]) {
cont[ix] = NULL;
if (tvFaceM) tvert[ix++] = tVertsM[i];
else {
// Do a planar mapping if there are no tverts
tvert[ix].x = mesh.verts[i].x/box.Width().x + 0.5f;
tvert[ix].y = mesh.verts[i].y/box.Width().y + 0.5f;
tvert[ix].z = mesh.verts[i].z/box.Width().z + 0.5f;
ix++;
}
}
}

// Copy in face and remap indices		
ix = 0;
         for (int i=0; i<mesh.getNumFaces(); i++) {
if (mesh.faceSel[i]) {
if (tvFaceM) tvFace[ix] = tvFaceM[i];
else {
for (int j=0; j<3; j++) 
tvFace[ix].t[j] = mesh.faces[i].v[j];
}

for (int j=0; j<3; j++) {
tvFace[ix].t[j] = vmap[tvFace[ix].t[j]];
}
ix++;
}
}
} else {
// Just copy all the tverts and faces
if (tvFaceM) {
tvert.SetCount(numTV);
cont.SetCount(numTV);
vsel.SetSize(numTV);
for (int i=0; i<numTV; i++) {
tvert[i] = tVertsM[i];
cont[i]  = NULL;
}
            for (int i=0; i<mesh.getNumFaces(); i++) {
tvFace[i] = tvFaceM[i];
}
} else {
Box3 box = mesh.getBoundingBox();
tvert.SetCount(mesh.getNumVerts());
cont.SetCount(mesh.getNumVerts());
vsel.SetSize(mesh.getNumVerts());
for (int i=0; i<mesh.getNumVerts(); i++) {
// Do a planar mapping if there are no tverts
tvert[i].x = mesh.verts[i].x/box.Width().x + 0.5f;
tvert[i].y = mesh.verts[i].y/box.Width().y + 0.5f;
tvert[i].z = mesh.verts[i].z/box.Width().z + 0.5f;
cont[i]  = NULL;
}
            for (int i=0; i<mesh.getNumFaces(); i++) {
for (int j=0; j<3; j++) 
tvFace[i].t[j] = mesh.faces[i].v[j];
}
}
}
if (hView && editMod==this) {
InvalidateView();
}
}
}
*/
void UnwrapMod::GetUVWIndices(int &i1, int &i2)
{
	switch (uvw) {
case 0: i1 = 0; i2 = 1; break;
case 1: i1 = 1; i2 = 2; break;
case 2: i1 = 0; i2 = 2; break;
	}
}


//--- Floater Dialog -------------------------------------------------


#define TOOL_HEIGHT		30
#define SPINNER_HEIGHT	30

#define WM_SETUPMOD	WM_USER+0x18de

static HIMAGELIST hToolImages = NULL;
static HIMAGELIST hOptionImages = NULL;
static HIMAGELIST hViewImages = NULL;
static HIMAGELIST hVertexImages = NULL;

class DeleteResources {
public:
	~DeleteResources() {
		if (hToolImages) ImageList_Destroy(hToolImages);			
		if (hOptionImages) ImageList_Destroy(hOptionImages);			
		if (hViewImages) ImageList_Destroy(hViewImages);			
		if (hVertexImages) ImageList_Destroy(hVertexImages);			
	}
};
static DeleteResources	theDelete;

void UnwrapMod::TrackRBMenu(HWND hwnd, int x, int y) {
	//	hPopMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_POPUP_MENU));
	//	HMENU hMenu = getResMgr().getMenu(IDR_UNWRAP_RIGHT_MENU);
	HMENU hMenu = LoadMenu(hInstance,MAKEINTRESOURCE(IDR_UNWRAP_RIGHT_MENU));
	HMENU subMenu = GetSubMenu(hMenu, 0);
	RECT rect;

	/*	CheckMenuItem(subMenu, ID_DRAG_COPY,  dragMode == DRAG_COPY   ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(subMenu, ID_DRAG_ROT, dragMode == DRAG_ROTATE ? MF_CHECKED : MF_UNCHECKED);
	EnableMenuItem(subMenu, ID_RENDERMAP, IsTex(CurMtl(activeSlot))?MF_ENABLED:MF_GRAYED );

	CheckMenuItem(subMenu, ID_MEDIT_ZOOM0,  zoomLevel == 0  ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(subMenu, ID_MEDIT_ZOOM1,  zoomLevel == 1  ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(subMenu, ID_MEDIT_ZOOM2,  zoomLevel == 2  ? MF_CHECKED : MF_UNCHECKED);

	EnableMenuItem(subMenu, ID_MEDIT_MAG, (CanMagnify(activeSlot)&&!isMag)?MF_ENABLED:MF_GRAYED );
	*/

	GetWindowRect(hwnd, &rect);
	TrackPopupMenu(subMenu, TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, 
		rect.left+x-2, rect.top+y, 0, hWnd, NULL);
	DestroyMenu(subMenu);
	DestroyMenu(hMenu);		

}




INT_PTR CALLBACK UnwrapFloaterDlgProc(
									  HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);
	//POINTS p = MAKEPOINTS(lParam);	commented out by sca 10/7/98 -- causing warning since unused.
	switch (msg) {
case WM_INITDIALOG:

	mod = (UnwrapMod*)lParam;
         DLSetWindowLongPtr(hWnd, lParam);
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, GetClassLongPtr(mod->ip->GetMAXHWnd(), GCLP_HICONSM)); // mjm - 3.12.99
	mod->ip->RegisterDlgWnd(hWnd);
	mod->SetupDlg(hWnd);

	delEvent.SetEditMeshMod(mod);
	mod->ip->RegisterDeleteUser(&delEvent);
	
	//			SendMessage( mod->hTextures, CB_SETCURSEL, mod->CurrentMap, 0 );
	//			mod->SetupImage();
	mod->UpdateListBox();
	SendMessage( mod->hMatIDs, CB_SETCURSEL, mod->matid+1, 0 );

	if (mod->iFalloff) mod->iFalloff->SetCurFlyOff(mod->falloff,FALSE);
	if (mod->iFalloffSpace) mod->iFalloffSpace->SetCurFlyOff(mod->falloffSpace,FALSE);
	//			mod->iIncSelected->SetCurFlyOff(mod->incSelected,FALSE);
	if (mod->iMirror) mod->iMirror->SetCurFlyOff(mod->mirror,FALSE);
	if (mod->iUVW) mod->iUVW->SetCurFlyOff(mod->uvw,FALSE);

	if (mod->iHide) mod->iHide->SetCurFlyOff(mod->hide,FALSE);
	if (mod->iFreeze) mod->iFreeze->SetCurFlyOff(mod->freeze,FALSE);

	if (mod->iFilterSelected)
	{
		if (mod->filterSelectedFaces)
			mod->iFilterSelected->SetCheck(TRUE);
		else mod->iFilterSelected->SetCheck(FALSE);
	}


	if (mod->iSnap)
	{
		mod->iSnap->SetCurFlyOff(mod->initialSnapState);
		if (mod->pixelSnap)
		{
			mod->iSnap->SetCurFlyOff(1);
			mod->iSnap->SetCheck(TRUE);
		}
		if (mod->fnGetGridSnap())
		{
			mod->iSnap->SetCurFlyOff(0);
			mod->iSnap->SetCheck(TRUE);
		}
		else mod->iSnap->SetCheck(FALSE);
	}

	if (mod->iShowMap)
	{
		if (mod->showMap)
			mod->iShowMap->SetCheck(TRUE);
		else mod->iShowMap->SetCheck(FALSE);
	}

	if (mod->windowPos.length != 0) 
		SetWindowPlacement(hWnd,&mod->windowPos);
#ifdef DESIGN_VER
    mod->ZoomExtents();
#endif
	break;
case WM_SHOWWINDOW:
	{
		if (!wParam)
		{
			if (mod->peltData.peltDialog.hWnd)
				ShowWindow(mod->peltData.peltDialog.hWnd,SW_HIDE);
			if (mod->hOptionshWnd)
				ShowWindow(mod->hOptionshWnd,SW_HIDE);
			if (mod->hRelaxDialog)
				ShowWindow(mod->hRelaxDialog,SW_HIDE);
			if (mod->renderUVWindow)
				ShowWindow(mod->renderUVWindow,SW_HIDE);


			
		}
		else 
		{
			if (mod->peltData.peltDialog.hWnd)
				ShowWindow(mod->peltData.peltDialog.hWnd,SW_SHOW);
			if (mod->hOptionshWnd)
				ShowWindow(mod->hOptionshWnd,SW_SHOW);
			if (mod->hRelaxDialog)
				ShowWindow(mod->hRelaxDialog,SW_SHOW);
			if (mod->renderUVWindow)
				ShowWindow(mod->renderUVWindow,SW_SHOW);


		}
		return FALSE;
		break;
	}
case WM_SIZE:
  	if (mod)
	{


		mod->minimized = FALSE;
 		if (wParam == SIZE_MAXIMIZED)
		{
			WINDOWPLACEMENT floaterPos;
			floaterPos.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hWnd,&floaterPos);
	


			Rect rect;
			GetWindowRect(hWnd,&rect);
			int w,h;
			w = rect.right - rect.left;
			h = rect.bottom - rect.top;
			if (w > mod->maximizeWidth)
				mod->maximizeWidth = w;
			if (h > mod->maximizeHeight)
				mod->maximizeHeight = h;

//			mod->maximizeHeight = 958;
//			DebugPrint("Heigth %d\n",height);
			SetWindowPos(hWnd,NULL,0,0,mod->maximizeWidth-2-mod->xWindowOffset,mod->maximizeHeight-mod->yWindowOffset,SWP_NOZORDER );
//			SetWindowPos(hWnd,HWND_TOP,0,0,500,500,SWP_SHOWWINDOW);
			mod->SizeDlg();
			mod->MoveScriptUI();
			mod->bringUpPanel = TRUE;
			return 0;
		}
		mod->SizeDlg();

	 	if (wParam == SIZE_MINIMIZED)
		{
			if (mod->peltData.peltDialog.hWnd)
				ShowWindow(mod->peltData.peltDialog.hWnd,SW_HIDE);
			if (mod->hOptionshWnd)
				ShowWindow(mod->hOptionshWnd,SW_HIDE);
			if (mod->hRelaxDialog)
				ShowWindow(mod->hRelaxDialog,SW_HIDE);
			if (mod->renderUVWindow)
				ShowWindow(mod->renderUVWindow,SW_HIDE);

			mod->minimized = TRUE;
			mod->MoveScriptUI();
			
			return 0;
			
		}
		else 
		{
			if (mod->peltData.peltDialog.hWnd)
				ShowWindow(mod->peltData.peltDialog.hWnd,SW_SHOW);
			if (mod->hOptionshWnd)
				ShowWindow(mod->hOptionshWnd,SW_SHOW);
			if (mod->hRelaxDialog)
				ShowWindow(mod->hRelaxDialog,SW_SHOW);
			if (mod->renderUVWindow)
				ShowWindow(mod->renderUVWindow,SW_SHOW);


			mod->bringUpPanel = TRUE;
		}
	}
	break;
case WM_MOVE:
 	if (mod) 
	{
 		mod->MoveScriptUI();

	}
	break;

case WM_ACTIVATE:
		
	
	if (LOWORD(wParam) == WA_INACTIVE) 
	{
		mod->floaterWindowActive = FALSE;
		mod->enableActionItems = FALSE;
//		mod->DeActivateActionTable();
//		DebugPrint("Not Actve\n");
		
	}
	else 
	{
//		DebugPrint("Actve\n");
//		mod->ActivateActionTable();
		mod->enableActionItems = TRUE;
		if ((LOWORD(wParam) == WA_CLICKACTIVE) &&  (!mod->floaterWindowActive))
		{
			mod->bringUpPanel = TRUE;
		}

		mod->floaterWindowActive = TRUE;
		
	}

	break;

case WM_PAINT: {
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd,&ps);
	Rect rect;
	GetClientRect(hWnd,&rect);			
	rect.top += TOOL_HEIGHT-2;
	SelectObject(hdc,GetStockObject(WHITE_BRUSH));			
	WhiteRect3D(hdc,rect,TRUE);
	EndPaint(hWnd,&ps);
	break;
			   }
case CC_SPINNER_BUTTONDOWN:
	if (LOWORD(wParam) != IDC_UNWRAP_STRSPIN) 
	{
		theHold.SuperBegin();
		mod->PlugControllers();			
		theHold.Begin();

	}
	break;
	/*
	case WM_ACTIVATE:
	{
	switch ( LOWORD(wParam) )
	{
	case WA_ACTIVE:
	case WA_CLICKACTIVE:
	{
	GetCOREInterface()->GetActionManager()->ActivateActionTable(mod->actionTable, kIUVWUnwrapQuad);
	break;
	}
	case WA_INACTIVE:
	{
	GetCOREInterface()->GetActionManager()->DeactivateActionTable(mod->actionTable, kIUVWUnwrapQuad);
	break;
	}
	}
	break;
	}
	*/			
	/*
	case WM_RBUTTONDOWN:
	{
	// Get the current vieport quad menu from the menu manager.
	// SCM 4/12/00
	//				IMenuContext *pContext = GetCOREInterface()->GetMenuManager()->GetMenuMan().GetContext(kIUVWUnwrapQuad);
	IMenuContext *pContext = GetCOREInterface()->GetMenuManager()->GetContext(kIUVWUnwrapQuad);
	DbgAssert(pContext);
	DbgAssert(pContext->GetType() == kMenuContextQuadMenu);
	IQuadMenuContext *pQMContext = (IQuadMenuContext *)pContext;
	IQuadMenu *pMenu = pQMContext->GetMenu( pQMContext->GetCurrentMenuIndex() );
	DbgAssert(pMenu);
	pMenu->TrackMenu(hWnd);
	return TRUE;
	break;
	}
	*/


case CC_SPINNER_CHANGE:
	if (LOWORD(wParam) == IDC_UNWRAP_STRSPIN) 
	{
		mod->RebuildDistCache();
		UpdateWindow(hWnd);
		mod->InvalidateView();
	}
	else
	{
		if (!theHold.Holding()) {
			theHold.SuperBegin();
			mod->PlugControllers();			
			theHold.Begin();
		}


		switch (LOWORD(wParam)) {
case IDC_UNWRAP_USPIN:
	mod->tempWhich = 0;
	mod->TypeInChanged(0);
	break;
case IDC_UNWRAP_VSPIN:
	mod->tempWhich = 1;
	mod->TypeInChanged(1);
	break;
case IDC_UNWRAP_WSPIN:
	mod->tempWhich = 2;
	mod->TypeInChanged(2);
	break;
		}

		UpdateWindow(hWnd);
	}
	break;

case WM_CUSTEDIT_ENTER:
case CC_SPINNER_BUTTONUP:
	if ( (LOWORD(wParam) == IDC_UNWRAP_STR) || (LOWORD(wParam) == IDC_UNWRAP_STRSPIN) )
	{
		float str = mod->iStr->GetFVal();
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setFalloffDist"), 1, 0,
			mr_float,str);
		macroRecorder->EmitScript();
		mod->RebuildDistCache();
		mod->InvalidateView();
		UpdateWindow(hWnd);
	}
	else
	{
		if (HIWORD(wParam) || msg==WM_CUSTEDIT_ENTER) {

			if (theHold.Holding())
			{
				theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));
				theHold.SuperAccept(_T(GetString(IDS_PW_MOVE_UVW)));
			}


			if (mod->tempWhich ==0)
				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.moveX"), 1, 0,
				mr_float,mod->tempAmount);
			else if (mod->tempWhich ==1)
				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.moveY"), 1, 0,
				mr_float,mod->tempAmount);
			else if (mod->tempWhich ==2)
				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.moveZ"), 1, 0,
				mr_float,mod->tempAmount);

			if (mod->fnGetRelativeTypeInMode())
				mod->SetupTypeins();


		} else {
			theHold.Cancel();
			theHold.SuperCancel();

			mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
			mod->InvalidateView();
			UpdateWindow(hWnd);
			mod->ip->RedrawViews(mod->ip->GetTime());
			if (mod->fnGetRelativeTypeInMode())
				mod->SetupTypeins();

		}
	}
	break;

case 0x020A:
	{
		int delta = (short) HIWORD(wParam);

		int xPos = GET_X_LPARAM(lParam); 
		int yPos = GET_Y_LPARAM(lParam); 


		RECT rect;
		GetWindowRect(mod->hView,&rect);
		xPos = xPos - rect.left;
		yPos = yPos - rect.top;

		float z;
		if (delta<0)
			z = (1.0f/(1.0f-0.0025f*delta));
		else z = (1.0f+0.0025f*delta);
		mod->zoom = mod->zoom * z;

		if (mod->middleMode->inDrag)
		{
			mod->xscroll = mod->xscroll*z;
			mod->yscroll = mod->yscroll*z;
		}
		else
		{

			Rect rect;
			GetClientRect(mod->hView,&rect);	
			int centerX = (rect.w()-1)/2-xPos;
			int centerY = (rect.h()-1)/2-yPos;


			mod->xscroll = (mod->xscroll + centerX)*z;
			mod->yscroll = (mod->yscroll + centerY)*z;


			mod->xscroll -= centerX;
			mod->yscroll -= centerY;


		}
		//watje tile
		mod->tileValid = FALSE;

		mod->middleMode->ResetInitialParams();

		mod->InvalidateView();

		//			DebugPrint("WHeel\n");
		break;
	}

case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
case ID_TOOL_MOVE:

case ID_TOOL_ROTATE:
case ID_TOOL_SCALE:
case ID_TOOL_WELD:
case ID_TOOL_PAN:
case ID_TOOL_ZOOM:
case ID_TOOL_ZOOMREG:
case ID_UNWRAP_MOVE:
case ID_UNWRAP_ROTATE:
case ID_UNWRAP_SCALE:
case ID_UNWRAP_PAN:
case ID_UNWRAP_WELD:
case ID_UNWRAP_ZOOM:
case ID_UNWRAP_ZOOMREGION:
case ID_FREEFORMMODE:
case ID_SKETCHMODE:
case ID_PAINTSELECTMODE:

	if (mod->iMove)
	{
		mod->move = mod->iMove->GetCurFlyOff();
		if (mod->move == 0)
			mod->iMove->SetTooltip(TRUE,GetString(IDS_RB_MOVE));
		else if (mod->move == 1)
			mod->iMove->SetTooltip(TRUE,GetString(IDS_PW_MOVEH));
		else if (mod->move == 2)
			mod->iMove->SetTooltip(TRUE,GetString(IDS_PW_MOVEV));
	}

	if (mod->iScale)
	{
		mod->scale = mod->iScale->GetCurFlyOff();
		if (mod->scale == 0)
			mod->iScale->SetTooltip(TRUE,GetString(IDS_RB_SCALE));
		else if (mod->scale == 1)
			mod->iScale->SetTooltip(TRUE,GetString(IDS_PW_SCALEH));
		else if (mod->scale == 2)
			mod->iScale->SetTooltip(TRUE,GetString(IDS_PW_SCALEV));
	}

	mod->SetMode(LOWORD(wParam));
	break;				

case ID_TOOL_FALLOFF:
	if (mod->iFalloff) mod->falloff = mod->iFalloff->GetCurFlyOff();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setFalloffType"), 1, 0,
		mr_int,mod->falloff+1);
	macroRecorder->EmitScript();

	mod->RebuildDistCache();
	mod->InvalidateView();
	break;
case ID_TOOL_FALLOFF_SPACE:
	if (mod->iFalloffSpace) 
	{
		mod->falloffSpace = mod->iFalloffSpace->GetCurFlyOff();
		if (mod->falloffSpace)
			mod->iFalloffSpace->SetTooltip(TRUE,GetString(IDS_PW_FALLOFFSPACEUVW));
		else mod->iFalloffSpace->SetTooltip(TRUE,GetString(IDS_PW_FALLOFFSPACE));
	}

	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setFalloffSpace"), 1, 0,
		mr_int,mod->falloffSpace+1);
	macroRecorder->EmitScript();


	mod->RebuildDistCache();
	mod->InvalidateView();
	break;

case ID_TOOL_DECSELECTED:
	if (mod->iDecSelected) mod->iDecSelected->SetTooltip(TRUE,GetString(IDS_PW_CONTRACTSELECTION));
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.contractSelection"), 0, 0);
	macroRecorder->EmitScript();
	mod->fnContractSelection();
	break;

case ID_TOOL_INCSELECTED:
	if (mod->iIncSelected) mod->iIncSelected->SetTooltip(TRUE,GetString(IDS_PW_EXPANDSELECTION)); 
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.expandSelection"), 0, 0);
	macroRecorder->EmitScript();
	mod->fnExpandSelection();
	break;
case ID_UNWRAP_MIRROR:
case ID_TOOL_MIRROR:
	if (mod->iMirror)
	{
		mod->mirror = mod->iMirror->GetCurFlyOff();

		if (mod->mirror ==0)
		{
			mod->iMirror->SetTooltip(TRUE,GetString(IDS_PW_MIRRORH));
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.mirrorh"), 0, 0);
			macroRecorder->EmitScript();
			mod->MirrorPoints(hWnd, mod->mirror);
		}
		else if (mod->mirror ==1)
		{
			mod->iMirror->SetTooltip(TRUE,GetString(IDS_PW_MIRRORV));
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.mirrorv"), 0, 0);
			macroRecorder->EmitScript();
			mod->MirrorPoints(hWnd, mod->mirror);
		}
		else if (mod->mirror ==2)
		{
			mod->iMirror->SetTooltip(TRUE,GetString(IDS_PW_FLIPH));
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.fliph"), 0, 0);
			macroRecorder->EmitScript();
			mod->FlipPoints(mod->mirror-2);
		}
		else if (mod->mirror ==3)
		{
			mod->iMirror->SetTooltip(TRUE,GetString(IDS_PW_FLIPV));
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.flipv"), 0, 0);
			macroRecorder->EmitScript();
			mod->FlipPoints(mod->mirror-2);

		}
	}



	break;
case ID_TOOL_LOCKSELECTED:
	if (mod->iLockSelected) mod->lockSelected = mod->iLockSelected->IsChecked();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.lock"), 0, 0);
	macroRecorder->EmitScript();
	break;
case ID_TOOL_FILTER_SELECTEDFACES:
	if (mod->iFilterSelected) mod->filterSelectedFaces = mod->iFilterSelected->IsChecked();
	mod->InvalidateView();
	UpdateWindow(hWnd);
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.filterselected"), 0, 0);
	macroRecorder->EmitScript();

	break;
case ID_UNWRAP_EXTENT:
case ID_TOOL_ZOOMEXT:
	if (mod->iZoomExt) mod->zoomext = mod->iZoomExt->GetCurFlyOff();
	//watje tile
	mod->tileValid = FALSE;

	if (mod->zoomext == 0)
	{
		mod->ZoomExtents();
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.fit"), 0, 0);
	}
	else if (mod->zoomext == 1)
	{
		mod->ZoomSelected();
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.fitselected"), 0, 0);
	}
	else if (mod->zoomext == 2)
	{
		mod->FrameSelectedElement();
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.fitSelectedElement"), 0, 0);
	}


	macroRecorder->EmitScript();

	break;

case ID_TOOL_FILTER_MATID:
	if ( HIWORD(wParam) == CBN_SELCHANGE ) {
		//get count
		mod->matid = SendMessage( mod->hMatIDs, CB_GETCURSEL, 0, 0 )-1;
		mod->SetMatFilters();
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setMatID"), 1, 0,
			mr_int, mod->matid+2);
		macroRecorder->EmitScript();

		mod->UpdateListBox();
		if (mod->dropDownListIDs.Count() >= 2)
		{
			mod->CurrentMap = mod->dropDownListIDs[1];
			SendMessage( mod->hTextures, CB_SETCURSEL, 1, 0 );
		}
		else
		{
			SendMessage( mod->hTextures, CB_SETCURSEL, 0, 0 );
			mod->CurrentMap = mod->dropDownListIDs[0];
		}

		
		mod->SetupImage();


		mod->InvalidateView();
		SetFocus(hWnd);  //kinda hack, once the user has selected something we immediatetly change focus so he middle mouse scroll does not cycle the drop list
	}
	break;
case ID_TOOL_TEXTURE_COMBO:
	if ( HIWORD(wParam) == CBN_SELCHANGE ) {
		//get count
		SetFocus(hWnd); //kinda hack, once the user has selected something we immediatetly change focus so he middle mouse scroll does not cycle the drop list
		int ct = SendMessage( mod->hTextures, CB_GETCOUNT, 0, 0 );
		int res = SendMessage( mod->hTextures, CB_GETCURSEL, 0, 0 );
		//pick a new map
		if (res == (ct -4))
		{
			mod->PickMap();
			mod->SetupImage();
//			mod->UpdateListBox();			
			SendMessage( mod->hTextures, CB_SETCURSEL, mod->CurrentMap, 0 );
		}
		if (res == (ct -3))
		{
			mod->DeleteFromMaterialList(mod->CurrentMap);
			mod->SetupImage();
			mod->UpdateListBox();
			SendMessage( mod->hTextures, CB_SETCURSEL, mod->CurrentMap, 0 );
		}
		if (res == (ct -2))
		{
			mod->ResetMaterialList();
			mod->UpdateListBox();
			mod->SetupImage();
			SendMessage( mod->hTextures, CB_SETCURSEL, mod->CurrentMap, 0 );
		}
		else if (res < (ct-4))
			//select a current
		{
//			mod->CurrentMap = res;
			if ((res >= 0) && (res < mod->dropDownListIDs.Count()))
			{
				mod->CurrentMap = mod->dropDownListIDs[res];

				if (mod->CurrentMap == 0)
					mod->ShowCheckerMaterial(TRUE);
				else mod->ShowCheckerMaterial(FALSE);

				mod->SetupImage();
			}
			

			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setCurrentMap"), 1, 0,
				mr_int, mod->CurrentMap+1);
			macroRecorder->EmitScript();

		}

	}
	break;



case ID_TOOL_UVW:
	if (mod->iUVW) mod->uvw = mod->iUVW->GetCurFlyOff();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setUVSpace"), 1, 0,
		mr_int, mod->uvw+1);
	macroRecorder->EmitScript();

	mod->InvalidateView();
	break;

case ID_TOOL_PROP:
	SetFocus(hWnd);
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.options"), 0, 0);
	macroRecorder->EmitScript();

	mod->PropDialog();

	break;

case ID_TOOL_SHOWMAP:
	if (mod->iShowMap)
		mod->showMap = mod->iShowMap->IsChecked();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.DisplayMap"), 1, 0,
		mr_bool, mod->showMap);
	macroRecorder->EmitScript();

	mod->InvalidateView();
	break;

case ID_TOOL_SNAP:
	if (mod->iSnap->GetCurFlyOff() == 1)
	{
		if (mod->iSnap) mod->pixelSnap = mod->iSnap->IsChecked();
		mod->gridSnap = FALSE;
	}
	else
	{
		if (mod->iSnap) mod->fnSetGridSnap(mod->iSnap->IsChecked());
	}

	//					if (mod->iSnap) mod->pixelSnap = mod->iSnap->IsChecked();
	//					mod->InvalidateView();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.snap"), 0, 0);
	macroRecorder->EmitScript();
	break;

case ID_UNWRAP_BREAK:
case ID_TOOL_BREAK:
	mod->BreakSelected();
	mod->InvalidateView();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.breakSelected"), 0, 0);
	macroRecorder->EmitScript();

	break;
case ID_UNWRAP_WELDSELECTED:
case ID_TOOL_WELD_SEL:
	mod->WeldSelected(TRUE,TRUE);					
	//					mod->InvalidateView();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.weldSelected"), 0, 0);
	macroRecorder->EmitScript();
	break;
case ID_TOOL_UPDATE:
	mod->SetupImage();
	mod->UpdateListBox();
	mod->InvalidateView();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.updateMap"), 0, 0);
	macroRecorder->EmitScript();

	break;
case ID_TOOL_HIDE:
	if (mod->iHide) mod->hide = mod->iHide->GetCurFlyOff();
	if (mod->hide == 0)
	{
		if (mod->iHide) mod->iHide->SetTooltip(TRUE,GetString(IDS_PW_HIDE));
		mod->HideSelected();
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.hide"), 0, 0);
		macroRecorder->EmitScript();

	}
	else{
		if (mod->iHide) mod->iHide->SetTooltip(TRUE,GetString(IDS_PW_UNHIDE));
		mod->UnHideAll();
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.unhide"), 0, 0);
		macroRecorder->EmitScript();

	}
	mod->InvalidateView();
	break;
	/*
	case ID_TOOL_UNHIDE:
	mod->UnHideAll();
	mod->InvalidateView();
	break;
	*/
case ID_TOOL_FREEZE:
	if (mod->iFreeze) mod->freeze = mod->iFreeze->GetCurFlyOff();
	if (mod->freeze == 0)
	{
		if (mod->iFreeze) mod->iFreeze->SetTooltip(TRUE,GetString(IDS_PW_FREEZE));
		mod->FreezeSelected();
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.freeze"), 0, 0);
		macroRecorder->EmitScript();
	}
	else
	{
		if (mod->iFreeze) mod->iFreeze->SetTooltip(TRUE,GetString(IDS_PW_UNFREEZE));
		mod->UnFreezeAll();
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.unfreeze"), 0, 0);
		macroRecorder->EmitScript();

	}
	mod->InvalidateView();
	break;
case ID_ABSOLUTETYPEIN:
	{
		BOOL abs = FALSE;
		if (mod->iUVWSpinAbsoluteButton)
		{
			if (mod->iUVWSpinAbsoluteButton->IsChecked())
				abs = FALSE;
			else abs = TRUE;
			mod->SetAbsoluteTypeInMode(abs);
		}
		if (abs)
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap4.SetRelativeTypeIn"), 1, 0,
			mr_bool, FALSE);
		else macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap4.SetRelativeTypeIn"), 1, 0,
			mr_bool, TRUE);
		macroRecorder->EmitScript();
		break;
	}
case IDOK:
case IDCANCEL:
	break;
default:
	IMenuBarContext* pContext = (IMenuBarContext*) GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
	if (pContext)
	{
		int id = LOWORD(wParam);
		int hid = HIWORD(wParam);
		if (hid == 0)
			pContext->ExecuteAction(id);


	}
	return TRUE;

	/*
	case ID_TOOL_UNFREEZE:
	mod->UnFreezeAll();
	mod->InvalidateView();
	break;
	*/

		}

		IMenuBarContext* pContext = (IMenuBarContext*) GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
		if (pContext)
			pContext->UpdateWindowsMenu();
		break;
	}

case WM_CLOSE:
	{
		if ((mod->mode == ID_UNWRAP_WELD)||(mod->mode == ID_TOOL_WELD))
			mod->SetMode(mod->oldMode);
		HWND maxHwnd = mod->ip->GetMAXHWnd();
		mod->fnSetMapMode(NOMAP);
		SetFocus(maxHwnd);
 		DestroyWindow(hWnd);
		if (mod->hRelaxDialog)
		{
			DestroyWindow(mod->hRelaxDialog);
			mod->hRelaxDialog = NULL;
		}
		break;
	}

case WM_DESTROY:
	{
		mod->DestroyDlg();
		mod->ip->UnRegisterDeleteUser(&delEvent);
		if (mod->hRelaxDialog)
		{
			DestroyWindow(mod->hRelaxDialog);
			mod->hRelaxDialog = NULL;
		}
		break;
	}

default:
	return FALSE;
	}

	return TRUE;
}



static LRESULT CALLBACK UnwrapViewProc(
									   HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);
	switch (msg) {
case WM_CREATE:
	break;

case WM_SIZE:			
	if (mod) 
	{
		mod->iBuf->Resize();
		//watje tile
		mod->tileValid = FALSE;
		mod->iTileBuf->Resize();

		mod->InvalidateView();
		mod->MoveScriptUI();
	}
	break;


case WM_PAINT:
	if (mod) mod->PaintView();
	break;

case 0x020A:
	DebugPrint("WHeel\n");
	break;

case WM_LBUTTONDOWN:
case WM_LBUTTONDBLCLK:
case WM_LBUTTONUP:		
case WM_RBUTTONDOWN:
case WM_RBUTTONUP:
case WM_RBUTTONDBLCLK:
case WM_MBUTTONDOWN:
case WM_MBUTTONDBLCLK:
case WM_MBUTTONUP:		

case WM_MOUSEMOVE:

	return mod->mouseMan.MouseWinProc(hWnd,msg,wParam,lParam);

default:
	return DefWindowProc(hWnd,msg,wParam,lParam);
	}	
	return 0;
}

void UnwrapMod::DestroyDlg()
{
	if (renderUVWindow != NULL)
	{
		DestroyWindow(renderUVWindow);
	}
	renderUVWindow = NULL;

	ColorMan()->SetColor(LINECOLORID,  lineColor);
	ColorMan()->SetColor(SELCOLORID, selColor);
	ColorMan()->SetColor(OPENEDGECOLORID,  openEdgeColor);
	ColorMan()->SetColor(HANDLECOLORID,  handleColor);
	ColorMan()->SetColor(FREEFORMCOLORID,  freeFormColor);
	ColorMan()->SetColor(SHAREDCOLORID,  sharedColor);
	ColorMan()->SetColor(BACKGROUNDCOLORID,  backgroundColor);

	//new
	ColorMan()->SetColor(GRIDCOLORID,  gridColor);

	//get windowpos
	windowPos.length = sizeof(WINDOWPLACEMENT); 
	GetWindowPlacement(hWnd,&windowPos);
	EndScriptUI();

	ReleaseICustToolbar(iTool);
	iTool = NULL;
	ReleaseICustToolbar(iView);
	iView = NULL;
	ReleaseICustToolbar(iOption);
	iOption = NULL;
	ReleaseICustToolbar(iFilter);
	iFilter = NULL;
	ReleaseICustToolbar(iVertex);
	iVertex = NULL;

	ReleaseICustToolbar(iUVWSpinBar);
	iUVWSpinBar = NULL;
	if (iUVWSpinAbsoluteButton) ReleaseICustButton(iUVWSpinAbsoluteButton   ); 
	iUVWSpinAbsoluteButton    = NULL;


	if (iMove) ReleaseICustButton(iMove   ); 
	iMove    = NULL;


	if (iFreeForm) ReleaseICustButton(iFreeForm   ); iFreeForm    = NULL;

	if (iRot) ReleaseICustButton(iRot    ); 
	iRot     = NULL;
	if (iScale) ReleaseICustButton(iScale  ); 
	iScale   = NULL;

	if (iFalloff) ReleaseICustButton(iFalloff   ); 
	iFalloff    = NULL;
	if (iFalloffSpace) ReleaseICustButton(iFalloffSpace   ); 
	iFalloffSpace    = NULL;

	if (iMirror) ReleaseICustButton(iMirror  ); 
	iMirror   = NULL;
	if (iWeld) ReleaseICustButton(iWeld   ); 
	iWeld    = NULL;

	if (iPan) ReleaseICustButton(iPan); 
	iPan     = NULL;

	if (iZoom) ReleaseICustButton(iZoom   ); 
	iZoom    = NULL;
	if (iUpdate) ReleaseICustButton(iUpdate ); 
	iUpdate  = NULL;
	if (iZoomReg) ReleaseICustButton(iZoomReg); 
	iZoomReg = NULL;
	if (iZoomExt) ReleaseICustButton(iZoomExt); 
	iZoomExt = NULL;
	if (iUVW) ReleaseICustButton(iUVW	   ); 
	iUVW	   = NULL;
	if (iProp) ReleaseICustButton(iProp   ); 
	iProp    = NULL;
	if (iShowMap) ReleaseICustButton(iShowMap); 
	iShowMap = NULL;
	if (lockSelected) ReleaseICustButton(iLockSelected); 
	iLockSelected = NULL;

	if (iFilterSelected) ReleaseICustButton(iFilterSelected); 
	iFilterSelected = NULL;

	if (iHide) ReleaseICustButton(iHide); 
	iHide = NULL;
	if (iFreeze) ReleaseICustButton(iFreeze); 
	iFreeze = NULL;
	if (iIncSelected) ReleaseICustButton(iIncSelected); 
	iIncSelected = NULL;

	if (iDecSelected) 
		ReleaseICustButton(iDecSelected); 
	iDecSelected = NULL;

	if (iSnap) ReleaseICustButton(iSnap); iSnap = NULL;
	if (iU) ReleaseISpinner(iU); 
	iU = NULL;
	if (iV) ReleaseISpinner(iV); 
	iV = NULL;
	if (iW) ReleaseISpinner(iW); 
	iW = NULL;

	ReleaseISpinner(iStr); iStr = NULL;

	if (iWeldSelected) ReleaseICustButton(iWeldSelected); 
	iWeldSelected = NULL;

	if (iBreak) 
		ReleaseICustButton(iBreak); 
	iBreak = NULL;

	DestroyIOffScreenBuf(iBuf); iBuf   = NULL;

	//watje tile
	tileValid = FALSE;
	DestroyIOffScreenBuf(iTileBuf); iTileBuf   = NULL;


	hWnd = NULL;
	delete moveMode; moveMode = NULL;
	delete freeFormMode; freeFormMode = NULL;
	delete sketchMode; sketchMode = NULL;

	delete paintSelectMode; paintSelectMode = NULL;

	delete rotMode; rotMode = NULL;
	delete scaleMode; scaleMode = NULL;
	delete panMode; panMode = NULL;
	delete zoomMode; zoomMode = NULL;
	delete zoomRegMode; zoomRegMode = NULL;
	delete weldMode; weldMode = NULL;
	//PELT
	delete peltStraightenMode; peltStraightenMode = NULL;

	delete rightMode; rightMode = NULL;	
	delete middleMode; middleMode = NULL;	
	mouseMan.SetMouseProc(NULL,LEFT_BUTTON,0);
	mouseMan.SetMouseProc(NULL,RIGHT_BUTTON,0);
	mouseMan.SetMouseProc(NULL,MIDDLE_BUTTON,0);
	ip->UnRegisterDlgWnd(hWnd);


	if (mode == ID_SKETCHMODE)
		SetMode(ID_UNWRAP_MOVE);

	this->hWnd = NULL;

	if (CurrentMap == 0)
		ShowCheckerMaterial(FALSE);


}



void UnwrapMod::SetupDlg(HWND hWnd)
{
	if ((CurrentMap < 0) || (CurrentMap >= pblock->Count(unwrap_texmaplist)))
		CurrentMap = 0;

	lineColor = ColorMan()->GetColor(LINECOLORID);
	selColor = ColorMan()->GetColor(SELCOLORID );
	openEdgeColor = ColorMan()->GetColor(OPENEDGECOLORID  );
	handleColor = ColorMan()->GetColor(HANDLECOLORID  );
	freeFormColor = ColorMan()->GetColor(FREEFORMCOLORID  );
	sharedColor = ColorMan()->GetColor(SHAREDCOLORID  );
	backgroundColor = ColorMan()->GetColor(BACKGROUNDCOLORID  );

	//new
	gridColor = ColorMan()->GetColor(GRIDCOLORID  );

	this->hWnd = hWnd;

	hView = GetDlgItem(hWnd,IDC_UNWRAP_VIEW);
   DLSetWindowLongPtr(hView, this);
	iBuf = CreateIOffScreenBuf(hView);
	iBuf->SetBkColor(backgroundColor);
	viewValid    = FALSE;
	typeInsValid = FALSE;

	//watje tile
	iTileBuf = CreateIOffScreenBuf(hView);
	iTileBuf->SetBkColor(backgroundColor);
	tileValid = FALSE;

	moveMode = new MoveMode(this);
	rotMode = new RotateMode(this);
	scaleMode = new ScaleMode(this);
	panMode = new PanMode(this);
	zoomMode = new ZoomMode(this);
	zoomRegMode = new ZoomRegMode(this);
	weldMode = new WeldMode(this);
	//PELT
	peltStraightenMode = new PeltStraightenMode(this);

	rightMode = new RightMouseMode(this);
	middleMode = new MiddleMouseMode(this);
	freeFormMode = new FreeFormMode(this);
	sketchMode = new SketchMode(this);
	paintSelectMode = new PaintSelectMode(this);

	mouseMan.SetMouseProc(rightMode,RIGHT_BUTTON,1);
	mouseMan.SetMouseProc(middleMode,MIDDLE_BUTTON,2);

	iU = GetISpinner(GetDlgItem(hWnd,IDC_UNWRAP_USPIN));
	iU->LinkToEdit(GetDlgItem(hWnd,IDC_UNWRAP_U),EDITTYPE_FLOAT);
	iU->SetLimits(-9999999, 9999999, FALSE);
	iU->SetAutoScale();

	iV = GetISpinner(GetDlgItem(hWnd,IDC_UNWRAP_VSPIN));
	iV->LinkToEdit(GetDlgItem(hWnd,IDC_UNWRAP_V),EDITTYPE_FLOAT);
	iV->SetLimits(-9999999, 9999999, FALSE);
	iV->SetAutoScale();

	iW = GetISpinner(GetDlgItem(hWnd,IDC_UNWRAP_WSPIN));
	iW->LinkToEdit(GetDlgItem(hWnd,IDC_UNWRAP_W),EDITTYPE_FLOAT);
	iW->SetLimits(-9999999, 9999999, FALSE);
	iW->SetAutoScale();	

	if (showIconList[19]) 		
	{
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_USPIN),SW_SHOW);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_U),SW_SHOW);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_ULABEL),SW_SHOW);
	}
	else
	{
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_USPIN),SW_HIDE);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_U),SW_HIDE);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_ULABEL),SW_HIDE);
	}


	if (showIconList[20]) 		
	{
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_VSPIN),SW_SHOW);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_V),SW_SHOW);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_VLABEL),SW_SHOW);

	}
	else
	{
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_VSPIN),SW_HIDE);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_V),SW_HIDE);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_VLABEL),SW_HIDE);
	}

	if (showIconList[21]) 		
	{
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_WSPIN),SW_SHOW);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_W),SW_SHOW);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_WLABEL),SW_SHOW);
	}
	else
	{
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_WSPIN),SW_HIDE);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_W),SW_HIDE);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_WLABEL),SW_HIDE);
	}

	iStr = GetISpinner(GetDlgItem(hWnd,IDC_UNWRAP_STRSPIN));
	iStr->LinkToEdit(GetDlgItem(hWnd,IDC_UNWRAP_STR),EDITTYPE_FLOAT);
	iStr->SetLimits(0, 9999999, FALSE);
	iStr->SetAutoScale();	
	iStr->SetValue(falloffStr, FALSE);


	iTool = GetICustToolbar(GetDlgItem(hWnd,IDC_UNWARP_TOOLBAR));
	iTool->SetBottomBorder(TRUE);	
	iTool->SetImage(hToolImages);

	toolSize = 5;

	iTool->AddTool(ToolSeparatorItem(5));
	int toolCt = 0;

	if (showIconList[1])
	{
		iTool->AddTool(ToolButtonItem(CTB_CHECKBUTTON,0, 0, 1, 1, 16, 15, 23, 22, ID_TOOL_MOVE));
		toolSize += 23;
		toolCt++;
	}
	if (showIconList[2])
	{
		iTool->AddTool(ToolButtonItem(CTB_CHECKBUTTON,2, 2, 3, 3, 16, 15, 23, 22, ID_TOOL_ROTATE));
		toolSize += 23;
		toolCt++;
	}
	if (showIconList[3])
	{
		iTool->AddTool(ToolButtonItem(CTB_CHECKBUTTON,4, 4, 5, 5, 16, 15, 23, 22, ID_TOOL_SCALE));
		toolSize += 23;
		toolCt++;
	}
	if (showIconList[4])
	{
		iTool->AddTool(ToolButtonItem(CTB_CHECKBUTTON,38, 38, 39, 39, 16, 15, 23, 22, ID_FREEFORMMODE));
		toolSize += 23;
		toolCt++;
	}

	if (toolCt > 0)
	{
		iTool->AddTool(ToolSeparatorItem(5));
		toolSize += 5;
	}
	toolCt = 0;
	if (showIconList[5])
	{
		iTool->AddTool(ToolButtonItem(CTB_PUSHBUTTON,14, 14, 15, 15, 16, 15, 23, 22, ID_TOOL_MIRROR));
		toolSize += 23;
		toolCt++;
	}
	if (showIconList[6])
	{
		iTool->AddTool(	ToolButtonItem(CTB_PUSHBUTTON,18, 18, 19, 19, 16, 15, 23, 22, ID_TOOL_INCSELECTED));
		toolSize += 23;
		toolCt++;
	}
	if (showIconList[7])
	{
		iTool->AddTool(ToolButtonItem(CTB_PUSHBUTTON,20, 20, 21, 21, 16, 15, 23, 22, ID_TOOL_DECSELECTED));
		toolSize += 23;
		toolCt++;
	}
	if (toolCt > 0)
	{
		iTool->AddTool(ToolSeparatorItem(5));
		toolSize += 5;
	}
	toolCt = 0;
	if (showIconList[8])
	{
		iTool->AddTool(	ToolButtonItem(CTB_PUSHBUTTON,	24, 24, 25, 25, 16, 15, 23, 22, ID_TOOL_FALLOFF));
		toolSize += 23;
		toolCt++;
	}

	if (showIconList[9])
	{
		iTool->AddTool(ToolButtonItem(CTB_PUSHBUTTON,32, 32, 33, 33, 16, 15, 23, 22, ID_TOOL_FALLOFF_SPACE));
		toolSize += 23;
		toolCt++;
	}

	if (showIconList[10])
	{
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_STR),SW_SHOW);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_STRSPIN),SW_SHOW);
	}
	else
	{
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_STR),SW_HIDE);
		ShowWindow(GetDlgItem(hWnd,IDC_UNWRAP_STRSPIN),SW_HIDE);
	}

	vertSize = 0;

	iVertex = GetICustToolbar(GetDlgItem(hWnd,IDC_UNWRAP_VERTS_TOOLBAR));
	iVertex->SetBottomBorder(TRUE);	
	iVertex->SetImage(hVertexImages);

	if ((showIconList[11]) || (showIconList[12]) || (showIconList[13]) )
	{
		iVertex->AddTool(ToolSeparatorItem(5));
		vertSize += 5;
	}

	if (showIconList[11])
	{
		iVertex->AddTool(ToolButtonItem(CTB_PUSHBUTTON,	0, 1, 1, 1, 16, 15, 23, 22, ID_TOOL_BREAK));
		vertSize += 23;
	}
	if (showIconList[12])
	{
		iVertex->AddTool(ToolButtonItem(CTB_CHECKBUTTON,2, 2, 3, 3, 16, 15, 23, 22, ID_TOOL_WELD));
		vertSize += 23;
	}
	if (showIconList[13])
	{
		iVertex->AddTool(ToolButtonItem(CTB_PUSHBUTTON, 4,  4,  5,  5, 16, 15, 23, 22, ID_TOOL_WELD_SEL));
		vertSize += 23;
	}


	iOption = GetICustToolbar(GetDlgItem(hWnd,IDC_UNWRAP_OPTION_TOOLBAR));
	iOption->SetBottomBorder(TRUE);	
	iOption->SetImage(hOptionImages);
	optionSize = 0;
	if ((showIconList[14]) || (showIconList[15]) || (showIconList[16]) || (showIconList[17]) )
	{
		iOption->AddTool(ToolSeparatorItem(5));
		optionSize += 5;
	}
	if (showIconList[14]) 
	{
		iOption->AddTool(ToolButtonItem(CTB_PUSHBUTTON,0, 0, 0, 0, 16, 15, 70, 22, ID_TOOL_UPDATE));
		optionSize += 70;
	}
	if (showIconList[15]) 
	{
		iOption->AddTool(ToolButtonItem(CTB_CHECKBUTTON,0, 0, 1, 1, 16, 15, 23, 22, ID_TOOL_SHOWMAP));
		optionSize += 23;
	}
	if (showIconList[16]) 
	{
		iOption->AddTool(ToolButtonItem(CTB_PUSHBUTTON,2, 2, 2, 2, 16, 15, 23, 22, ID_TOOL_UVW));
		optionSize += 23;
	}
	if (showIconList[17]) 
	{
		iOption->AddTool(ToolButtonItem(CTB_PUSHBUTTON,5, 5, 5, 5, 16, 15, 23, 22, ID_TOOL_PROP));
		optionSize += 23;
	}

	//	iOption->AddTool(ToolSeparatorItem(5));

	// get the objects materail texture list

	if (showIconList[18]) 
	{
		//5.1.01
		iOption->AddTool(ToolOtherItem(_T("combobox"), 170,	430, ID_TOOL_TEXTURE_COMBO,
			CBS, 2, NULL, 0));
		hTextures = iOption->GetItemHwnd(ID_TOOL_TEXTURE_COMBO);
		//	SendMessage(hTextures, WM_SETFONT, (WPARAM)GetAppHFont(), MAKELONG(0, 0));

		HFONT hFont;
		hFont = CreateFont(GetFixedFontHeight(),0,0,0,FW_LIGHT,0,0,0,GetFixedFontCharset(),0,0,0, VARIABLE_PITCH | FF_SWISS, _T(""));
		SendMessage(hTextures, WM_SETFONT, (WPARAM)hFont, MAKELONG(0, 0));
	}



	iUVWSpinBar = GetICustToolbar(GetDlgItem(hWnd,IDC_UNWRAP_ABSOLUTE_TOOLBAR));
	iUVWSpinBar->SetBottomBorder(FALSE);	
	iUVWSpinBar->SetImage(hVertexImages);
	iUVWSpinBar->AddTool(ToolSeparatorItem(5));
	iUVWSpinBar->AddTool(ToolButtonItem(CTB_CHECKBUTTON,26, 27, 26, 27, 16, 15, 23, 22, ID_ABSOLUTETYPEIN));
	iUVWSpinAbsoluteButton = iUVWSpinBar->GetICustButton(ID_ABSOLUTETYPEIN);
	if (fnGetRelativeTypeInMode())
		iUVWSpinAbsoluteButton->SetCheck(TRUE);
	else iUVWSpinAbsoluteButton->SetCheck(FALSE);



	iView = GetICustToolbar(GetDlgItem(hWnd,IDC_UNWARP_VIEW_TOOLBAR));
	iView->SetBottomBorder(FALSE);	
	iView->SetImage(hViewImages);
	iView->AddTool(ToolSeparatorItem(5));

	viewSize = 5;

	if (showIconList[27]) 
	{
		iView->AddTool(ToolButtonItem(CTB_CHECKBUTTON,0, 0, 1, 1, 16, 15, 23, 22, ID_TOOL_PAN));
		viewSize += 23;
	}

	if (showIconList[28]) 
	{
		iView->AddTool(ToolButtonItem(CTB_CHECKBUTTON,2, 2, 3, 3, 16, 15, 23, 22, ID_TOOL_ZOOM));
		viewSize += 23;
	}	
	if (showIconList[29]) 
	{
		iView->AddTool(ToolButtonItem(CTB_CHECKBUTTON, 4,  4,  5,  5, 16, 15, 23, 22, ID_TOOL_ZOOMREG));
		viewSize += 23;
	}	
	if (showIconList[30]) 
	{
		iView->AddTool(ToolButtonItem(CTB_PUSHBUTTON, 6,  6,  7,  7, 16, 15, 23, 22, ID_TOOL_ZOOMEXT));
		viewSize += 23;
	}
	if (showIconList[31]) 
	{
		iView->AddTool(ToolButtonItem(CTB_CHECKBUTTON, 10, 10,  10,  10, 16, 15, 23, 22, ID_TOOL_SNAP));
		viewSize += 23;
	}

	/*View->AddTool(
	ToolButtonItem(CTB_CHECKBUTTON,
	11, 11,  11,  11, 16, 15, 23, 22, ID_TOOL_LOCKSELECTED));
	*/


	//	iVertex->AddTool(ToolSeparatorItem(10));

	iFilter = GetICustToolbar(GetDlgItem(hWnd,IDC_UNWRAP_FILTER_TOOLBAR));
	iFilter->SetBottomBorder(FALSE);	
	iFilter->SetImage(hVertexImages);
	iFilter->AddTool(ToolSeparatorItem(5));

	filterSize = 5;
	int filterCount = 0;
	if (showIconList[22]) 
	{
		iFilter->AddTool(ToolButtonItem(CTB_CHECKBUTTON, 14, 16,  15,  17, 16, 15, 23, 22, ID_TOOL_LOCKSELECTED));
		filterSize += 23;
		filterCount++;
	}

	if (showIconList[23]) 
	{
		iFilter->AddTool(ToolButtonItem(CTB_PUSHBUTTON, 6,  6,  7,  7, 16, 15, 23, 22, ID_TOOL_HIDE));
		filterSize += 23;
		filterCount++;
	}

	if (showIconList[24]) 
	{
		iFilter->AddTool(ToolButtonItem(CTB_PUSHBUTTON, 10,  10,  11,  11, 16, 15, 23, 22, ID_TOOL_FREEZE));
		filterSize += 23;
		filterCount++;
	}

	if (showIconList[25]) 
	{
		iFilter->AddTool(ToolButtonItem(CTB_CHECKBUTTON,18, 19, 26, 26, 16, 15, 23, 22, ID_TOOL_FILTER_SELECTEDFACES));
		filterSize += 23;
		filterCount++;
	}

	if ( showIconList[26] && (filterCount >0))
	{
		iFilter->AddTool(ToolSeparatorItem(10));
		filterSize += 10;
	}	

	if (showIconList[26])
	{
		//5.1.01
#ifdef	LOCALIZED_JPN	// --hide.
			iFilter->AddTool(ToolOtherItem(_T("combobox"), 100,	280, ID_TOOL_FILTER_MATID,
		                           CBS, 2, NULL, 0));
#elif defined(LOCALIZED_CHS)	// Chinese
			iFilter->AddTool(ToolOtherItem(_T("combobox"), 130,	280, ID_TOOL_FILTER_MATID,
		                           CBS, 2, NULL, 0));
#else
			iFilter->AddTool(ToolOtherItem(_T("combobox"), 140,	280, ID_TOOL_FILTER_MATID,
		                           CBS, 2, NULL, 0));
#endif // LOCALIZED_XXX
		hMatIDs = iFilter->GetItemHwnd(ID_TOOL_FILTER_MATID);
		//	SendMessage(hTextures, WM_SETFONT, (WPARAM)GetAppHFont(), MAKELONG(0, 0));
		//FIX THIS make res id
		//5.1.01
		{
			HFONT hFont;			// Add for Japanese version
			hFont = CreateFont(GetFixedFontHeight(),0,0,0,FW_LIGHT,0,0,0,GetFixedFontCharset(),0,0,0, VARIABLE_PITCH | FF_SWISS, _T(""));
			SendMessage(hMatIDs, WM_SETFONT, (WPARAM)hFont, MAKELONG(0, 0));
		}
		SendMessage(hMatIDs, CB_ADDSTRING, 0, (LPARAM)_T(GetString(IDS_PW_ID_ALLID)));	
		SendMessage(hMatIDs,CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
		filterSize += 140;
		}
	


	iWeld    = iVertex->GetICustButton(ID_TOOL_WELD);
	iLockSelected = iFilter->GetICustButton(ID_TOOL_LOCKSELECTED);
	iHide	 = iFilter->GetICustButton(ID_TOOL_HIDE);
	iFreeze	 = iFilter->GetICustButton(ID_TOOL_FREEZE);
	iFilterSelected = iFilter->GetICustButton(ID_TOOL_FILTER_SELECTEDFACES);

	iMove    = iTool->GetICustButton(ID_TOOL_MOVE);
	iRot     = iTool->GetICustButton(ID_TOOL_ROTATE);
	iScale   = iTool->GetICustButton(ID_TOOL_SCALE);	
	iMirror   = iTool->GetICustButton(ID_TOOL_MIRROR);	
	iIncSelected = iTool->GetICustButton(ID_TOOL_INCSELECTED);	
	iDecSelected = iTool->GetICustButton(ID_TOOL_DECSELECTED);	
	iFalloff = iTool->GetICustButton(ID_TOOL_FALLOFF);	
	iFalloffSpace = iTool->GetICustButton(ID_TOOL_FALLOFF_SPACE);

	iPan     = iView->GetICustButton(ID_TOOL_PAN);	
	iZoom    = iView->GetICustButton(ID_TOOL_ZOOM);
	iZoomExt = iView->GetICustButton(ID_TOOL_ZOOMEXT);
	iZoomReg = iView->GetICustButton(ID_TOOL_ZOOMREG);
	iSnap = iView->GetICustButton(ID_TOOL_SNAP);

	iBreak = iVertex->GetICustButton(ID_TOOL_BREAK);
	iWeldSelected = iVertex->GetICustButton(ID_TOOL_WELD_SEL);

	iFreeForm    = iTool->GetICustButton(ID_FREEFORMMODE);


	iUpdate  = iOption->GetICustButton(ID_TOOL_UPDATE);
	iUVW	 = iOption->GetICustButton(ID_TOOL_UVW);
	iProp	 = iOption->GetICustButton(ID_TOOL_PROP);
	iShowMap = iOption->GetICustButton(ID_TOOL_SHOWMAP);

	if (iMove) iMove->SetTooltip(TRUE,GetString(IDS_RB_MOVE));
	if (iRot) iRot->SetTooltip(TRUE,GetString(IDS_RB_ROTATE));
	if (iScale) iScale->SetTooltip(TRUE,GetString(IDS_RB_SCALE));
	if (iPan) iPan->SetTooltip(TRUE,GetString(IDS_RB_PAN));
	if (iZoom) iZoom->SetTooltip(TRUE,GetString(IDS_RB_ZOOM));
	if (iUpdate) iUpdate->SetTooltip(TRUE,GetString(IDS_RB_UPDATE));
	if (iZoomExt) iZoomExt->SetTooltip(TRUE,GetString(IDS_RB_ZOOMEXT));
	if (iZoomReg) iZoomReg->SetTooltip(TRUE,GetString(IDS_RB_ZOOMREG));
	if (iUVW) iUVW->SetTooltip(TRUE,GetString(IDS_RB_UVW));
	if (iProp) iProp->SetTooltip(TRUE,GetString(IDS_RB_PROP));
	if (iShowMap) iShowMap->SetTooltip(TRUE,GetString(IDS_RB_SHOWMAP));
	if (iSnap) iSnap->SetTooltip(TRUE,GetString(IDS_PW_SNAP));
	if (iWeld) iWeld->SetTooltip(TRUE,GetString(IDS_PW_WELD));

	if (iWeldSelected) iWeldSelected->SetTooltip(TRUE,GetString(IDS_PW_WELDSELECTED));
	if (iBreak) iBreak->SetTooltip(TRUE,GetString(IDS_PW_BREAK));

	if (iFreeForm) iFreeForm->SetTooltip(TRUE,GetString(IDS_PW_FREEFORMMODE));


	if (iMirror) iMirror->SetTooltip(TRUE,GetString(IDS_PW_MIRRORH));
	if (iIncSelected) iIncSelected->SetTooltip(TRUE,GetString(IDS_PW_EXPANDSELECTION));
	if (iDecSelected) iDecSelected->SetTooltip(TRUE,GetString(IDS_PW_CONTRACTSELECTION));
	if (iFalloff) iFalloff->SetTooltip(TRUE,GetString(IDS_PW_FALLOFF)); 
	if (iFalloffSpace) iFalloffSpace->SetTooltip(TRUE,GetString(IDS_PW_FALLOFFSPACE)); 
	//need break tool tip	iBreak->SetTooltip(TRUE,GetString(IDS_PW_BREAK)); 
	//need weld selecetd	iBreak->SetTooltip(TRUE,GetString(IDS_PW_BREAK)); 
	if (iLockSelected) iLockSelected->SetTooltip(TRUE,GetString(IDS_PW_LOCKSELECTED));
	if (iHide) iHide->SetTooltip(TRUE,GetString(IDS_PW_HIDE));
	if (iFreeze) iFreeze->SetTooltip(TRUE,GetString(IDS_PW_FREEZE));
	if (iFilterSelected) iFilterSelected->SetTooltip(TRUE,GetString(IDS_PW_FACEFILTER));

	if (iMove) iMove->SetHighlightColor(GREEN_WASH);
	if (iRot) iRot->SetHighlightColor(GREEN_WASH);
	if (iScale) iScale->SetHighlightColor(GREEN_WASH);
	if (iWeld) iWeld->SetHighlightColor(GREEN_WASH);
	if (iPan) iPan->SetHighlightColor(GREEN_WASH);
	if (iZoom) iZoom->SetHighlightColor(GREEN_WASH);	
	if (iZoomReg) iZoomReg->SetHighlightColor(GREEN_WASH);	
	if (iMirror) iMirror->SetHighlightColor(GREEN_WASH);	
	if (iFalloff) iFalloff->SetHighlightColor(GREEN_WASH);	
	if (iFalloffSpace) iFalloffSpace->SetHighlightColor(GREEN_WASH);	

	if (iFreeForm) iFreeForm->SetHighlightColor(GREEN_WASH);

	if (iUpdate) iUpdate->SetImage(NULL,0,0,0,0,0,0);
	if (iUpdate) iUpdate->SetText(GetString(IDS_RB_UPDATE));

	FlyOffData fdata1[] = {
		{ 2, 2 ,  2,  2},
		{ 3,  3,  3,  3},
		{ 4,  4,  4,  4}};
	if (iUVW) iUVW->SetFlyOff(3,fdata1,ip->GetFlyOffTime(),uvw,FLY_DOWN);

	FlyOffData fdata2[] = {
			{ 0,  0,  1,  1},
			{10, 10, 11, 11},
			{12, 12, 13, 13}};
	if (iMove) iMove->SetFlyOff(3,fdata2,ip->GetFlyOffTime(),move,FLY_DOWN);

	FlyOffData fdata2a[] = {
				{24, 24, 25, 25},
				{26, 26, 27, 27},
				{28, 28, 29, 29},
				{30, 30, 31, 31}
			};
	if (iFalloff) iFalloff->SetFlyOff(4,fdata2a,ip->GetFlyOffTime(),falloff,FLY_DOWN);


	FlyOffData fdata2b[] = {
				{32, 32, 32, 32},
				{33, 33, 33, 33},
			};
	if (iFalloffSpace) iFalloffSpace->SetFlyOff(2,fdata2b,ip->GetFlyOffTime(),falloffSpace,FLY_DOWN);



	FlyOffData fdata3[] = {
				{ 4,  4,  5,  5},
				{ 6,  6,  7,  7},
				{ 8,  8,  9,  9}};
	if (iScale) iScale->SetFlyOff(3,fdata3,ip->GetFlyOffTime(),scale,FLY_DOWN);

	FlyOffData fdata5[] = {
					{ 14,  14,  15,  15},
					{ 16,  16,  17,  17},
					{ 34,  34,  35,  35},
					{ 36,  36,  37,  37}
				};
	if (iMirror) iMirror->SetFlyOff(4,fdata5,ip->GetFlyOffTime(),mirror,FLY_DOWN);

	FlyOffData fdata4[] = {
					{ 6,  6,  7,  7},
					{ 8,  8,  9,  9},
					{ 30,  30,  31,  31}};
	if (iZoomExt) iZoomExt->SetFlyOff(3,fdata4,ip->GetFlyOffTime(),zoomext,FLY_UP);

	FlyOffData fdata6[] = {
						{ 6,  6,  7,  7},
						{ 8,  8,  9,  9}};
	if (iHide) iHide->SetFlyOff(2,fdata6,ip->GetFlyOffTime(),hide,FLY_UP);

	FlyOffData fdata7[] = {
							{ 10,  11,  11,  11},
							{ 12,  13,  13,  13}};
	if (iFreeze) iFreeze->SetFlyOff(2,fdata7,ip->GetFlyOffTime(),hide,FLY_UP);

							/*
							FlyOffData fdata8[] = {
							{ 18,  18,  19,  19},
							{ 20,  20,  21,  21}};
							iIncSelected->SetFlyOff(2,fdata8,ip->GetFlyOffTime(),hide,FLY_DOWN);
							*/

	FlyOffData fdata8[] = {
								{ 11,  11,  11,  11},
								{ 10,  10,  10,  10}
							};
	if (iSnap) iSnap->SetFlyOff(2,fdata8,ip->GetFlyOffTime(),hide,FLY_UP);

	if (iShowMap)
	{
		iShowMap->SetCheck(showMap);
		if (image) iShowMap->Enable();
		else	iShowMap->Disable();
	}


	if (iSnap) 
	{
		if (pixelSnap)
		{
			iSnap->SetCurFlyOff(1);
			iSnap->SetCheck(TRUE);
		}
		if (gridSnap)
		{
			iSnap->SetCurFlyOff(0);
			iSnap->SetCheck(TRUE);
		}
		else iSnap->SetCheck(FALSE);
	}

	if (iFilterSelected) iFilterSelected->SetCheck(filterSelectedFaces);

	if (iLockSelected) iLockSelected->SetCheck(lockSelected);

	SizeDlg();
	SetMode(mode,FALSE);

	//5.1.02 adds new bitmap bg management
	//							matPairList.ZeroCount();

	Mtl *baseMtl = GetBaseMtl();

	Tab<int> matIDs;
	Tab<Mtl*> matList;
	MultiMtl *mtl;
	if (baseMtl)
	{
		mtl = (MultiMtl*) GetMultiMaterials(baseMtl);
		if (mtl)
		{
			IParamBlock2 *pb = mtl->GetParamBlock(0);
			if (pb)
			{	

				int numMaterials = pb->Count(multi_mtls);
				matIDs.SetCount(numMaterials);
				matList.SetCount(numMaterials);
				for (int i = 0; i < numMaterials; i++)
				{
					int id;
					Mtl *mat;
					pb->GetValue(multi_mtls,0,mat,FOREVER,i);
					pb->GetValue(multi_ids,0,id,FOREVER,i);
					matIDs[i] = id;											
					matList[i] = mat;																						
				}
			}
		}

	}


	for (int i = 0; i<filterMatID.Count();i++)
	{
		char st[200];
		if (mtl) 
		{
			int id = filterMatID[i];

			int matchMat = -1;
			for (int j = 0; j < matIDs.Count(); j++)
			{
				if (id == matIDs[j])
					matchMat = j;
			}
			if ((matchMat == -1) || (matList[matchMat] == NULL))
				sprintf(st,"%d",filterMatID[i]+1);
			else sprintf(st,"%d:%s",filterMatID[i]+1,matList[matchMat]->GetFullName());

		}
		else sprintf(st,"%d",filterMatID[i]+1);
		SendMessage(hMatIDs, CB_ADDSTRING , 0, (LPARAM) (TCHAR*) st);
	}


	matid = -1;
	SendMessage(hMatIDs, CB_SETCURSEL, matid+1, 0 );



	if ((ip) && (!ip->IsSubObjectSelectionEnabled())&& (iFilterSelected))
		iFilterSelected->Enable(FALSE);

	if (pblock->Count(unwrap_texmaplist) == 0)
		ResetMaterialList();

}

static void SetWindowYPos(HWND hWnd,int y)
{
	Rect rect;
	GetClientRectP(hWnd,&rect);
	SetWindowPos(hWnd,NULL,rect.left,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
}

static void SetWindowXPos(HWND hWnd,int x)
{
	Rect rect;
	GetClientRectP(hWnd,&rect);
	SetWindowPos(hWnd,NULL,x,rect.top,0,0,SWP_NOSIZE|SWP_NOZORDER);
}

void UnwrapMod::SizeDlg()
{
	Rect rect;
	GetClientRect(hWnd,&rect);
	MoveWindow(GetDlgItem(hWnd,IDC_UNWARP_TOOLBAR),
		0, 0, toolSize, TOOL_HEIGHT, TRUE);

	SetWindowXPos(GetDlgItem(hWnd,IDC_UNWRAP_STR),toolSize);
	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_STR),6);
	SetWindowXPos(GetDlgItem(hWnd,IDC_UNWRAP_STRSPIN),toolSize+32);
	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_STRSPIN),6);

	MoveWindow(GetDlgItem(hWnd,IDC_UNWRAP_VERTS_TOOLBAR),
		toolSize+56, 0, vertSize, TOOL_HEIGHT, TRUE);


	MoveWindow(GetDlgItem(hWnd,IDC_UNWRAP_OPTION_TOOLBAR),
		310+64, 0, 320, TOOL_HEIGHT, TRUE);


	/*
	MoveWindow(GetDlgItem(hWnd,IDC_UNWRAP_TEXTURE_COMBO),
	480, 4, 100, TOOL_HEIGHT, TRUE);
	MoveWindow(GetDlgItem(hWnd,IDC_MATID_STRING),
	582, 0, 36, TOOL_HEIGHT-4, TRUE);
	MoveWindow(GetDlgItem(hWnd,IDC_MATID_COMBO),
	620, 4, 50, TOOL_HEIGHT, TRUE);
	*/

	MoveWindow(GetDlgItem(hWnd,IDC_UNWRAP_VIEW),
		2, TOOL_HEIGHT, rect.w()-5, rect.h()-TOOL_HEIGHT-SPINNER_HEIGHT-3,FALSE);


	int ys = rect.h()-TOOL_HEIGHT+3;
	int yl = rect.h()-TOOL_HEIGHT+5;


	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_ULABEL),yl);
	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_VLABEL),yl);
	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_WLABEL),yl);

	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_U),ys);
	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_V),ys);
	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_W),ys);

	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_USPIN),ys);
	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_VSPIN),ys);
	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_WSPIN),ys);

	SetWindowYPos(GetDlgItem(hWnd,IDC_UNWRAP_ABSOLUTE_TOOLBAR),ys);

	{
		int w = rect.w() - filterSize - viewSize - 4;
		if (w < 220+12)
			w = 220+12;

		MoveWindow(GetDlgItem(hWnd,IDC_UNWRAP_FILTER_TOOLBAR),
			w, ys-5, filterSize, TOOL_HEIGHT, TRUE);

		MoveWindow(GetDlgItem(hWnd,IDC_UNWARP_VIEW_TOOLBAR),
			w + filterSize, ys-5, viewSize, TOOL_HEIGHT, TRUE);

	}

	InvalidateRect(hWnd,NULL,TRUE);
	InvalidateRect(GetDlgItem(hWnd,IDC_UNWRAP_FILTER_TOOLBAR),NULL,FALSE);
	InvalidateRect(GetDlgItem(hWnd,IDC_UNWARP_VIEW_TOOLBAR),NULL,FALSE);

}

Point2 UnwrapMod::UVWToScreen(Point3 pt,float xzoom, float yzoom,int w,int h)
{	
	int i1, i2;
	GetUVWIndices(i1,i2);
	int tx = (w-int(xzoom))/2;
	int ty = (h-int(yzoom))/2;
	return Point2(pt[i1]*xzoom+xscroll+tx, (float(h)-pt[i2]*yzoom)+yscroll-ty);
}

void UnwrapMod::ComputeZooms(
							 HWND hWnd, float &xzoom, float &yzoom,int &width, int &height)
{
	Rect rect;
	GetClientRect(hWnd,&rect);	
	width = rect.w()-1;
 	height = rect.h()-1;

	if (zoom < 0.000001f) zoom = 0.000001f;
	if (zoom > 100000.0f) zoom = 100000.0f;

	if (lockAspect)
		xzoom = zoom*aspect*float(height);
	else xzoom = zoom*aspect*float(width);
	yzoom = zoom*float(height);

}


void UnwrapMod::SetMatFilters()
{
	if (vertMatIDList.GetSize() != TVMaps.v.Count())
		vertMatIDList.SetSize(TVMaps.v.Count());
	if (matid == -1)
		vertMatIDList.SetAll();
	else 
	{
		vertMatIDList.ClearAll();
		for (int j = 0; j < TVMaps.f.Count(); j++)
		{
			int pcount = 3;
			//		if (TVMaps.f[j].flags & FLAG_QUAD) pcount = 4;
			pcount = TVMaps.f[j]->count;

			for (int k = 0; k < pcount; k++)
			{
				int index = TVMaps.f[j]->t[k];
				//6-29-99 watje
				if ((matid < filterMatID.Count()) && (filterMatID[matid] == TVMaps.f[j]->MatID) && (index < vertMatIDList.GetSize()))
					vertMatIDList.Set(index);

				if ( (TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
					(TVMaps.f[j]->vecs) 
					)
				{
					index = TVMaps.f[j]->vecs->handles[k*2];
					//6-29-99 watje
					if ((matid < filterMatID.Count()) && (filterMatID[matid] == TVMaps.f[j]->MatID) && (index < vertMatIDList.GetSize()))
						vertMatIDList.Set(index);

					index = TVMaps.f[j]->vecs->handles[k*2+1];
					//6-29-99 watje
					if ((matid < filterMatID.Count()) && (filterMatID[matid] == TVMaps.f[j]->MatID) && (index < vertMatIDList.GetSize()))
						vertMatIDList.Set(index);

					if (TVMaps.f[j]->flags & FLAG_INTERIOR)
					{
						index = TVMaps.f[j]->vecs->interiors[k];
						//6-29-99 watje
						if ((matid < filterMatID.Count()) && (filterMatID[matid] == TVMaps.f[j]->MatID) && (index < vertMatIDList.GetSize()))
							vertMatIDList.Set(index);

					}

				}


			}
		}
	}

}


int UnwrapMod::IsFaceVisible(int i)
{
	if (!(TVMaps.f[i]->flags & FLAG_DEAD) )
	{
		if ((filterSelectedFaces==0) || (TVMaps.f[i]->flags & FLAG_SELECTED))
		{
			if ((matid == -1) || (matid >= filterMatID.Count()))
				return 1;
			else if (filterMatID[matid] ==TVMaps.f[i]->MatID)
				return 1;

		}
	}
	return 0;

}

int UnwrapMod::IsVertVisible(int i)
{
	if ( (!(TVMaps.v[i].flags & FLAG_DEAD)) && (!(TVMaps.v[i].flags & FLAG_HIDDEN)) )
	{
		if ((filterSelectedFaces==0) || (IsSelected(i) == 1))
		{
			if ((matid == -1)|| (i >= vertMatIDList.GetSize()))
				return 1;
			else 
			{
				if (vertMatIDList[i]) return 1;
				/*
				for (int j = 0; j < TVMaps.f.Count(); j++)
				{
				int pcount = 3;
				if (TVMaps.f[j].flags & FLAG_QUAD) pcount = 4;
				for (int k = 0; k < pcount; k++)
				{
				int index = TVMaps.f[j].t[k];
				if (index==i)
				{
				if (filterMatID[matid] ==TVMaps.f[j].MatID)
				return 1;
				}
				}
				}
				*/
			}
		}
	}
	return 0;

}

static inline  void RectNoFill(HDC hdc,Point2 a, Point2 b)
{
	MoveToEx(hdc,(int)a.x, (int)a.y, NULL);
	LineTo(hdc,(int)b.x, (int)a.y);
	LineTo(hdc,(int)b.x, (int)b.y);
	LineTo(hdc,(int)a.x, (int)b.y);
	LineTo(hdc,(int)a.x, (int)a.y);

}

static inline  void MiniRect(HDC hdc,Point2 a, int offset)
{
	MoveToEx(hdc,(int)a.x-offset, (int)a.y-offset, NULL);
	LineTo(hdc,(int)a.x+offset, (int)a.y-offset);
	LineTo(hdc,(int)a.x+offset, (int)a.y+offset);
	LineTo(hdc,(int)a.x-offset, (int)a.y+offset);
	LineTo(hdc,(int)a.x-offset, (int)a.y-offset);

}

static inline  void IMiniRect(HDC hdc,IPoint2 a, int offset)
{
	MoveToEx(hdc,a.x-offset, a.y-offset, NULL);
	LineTo(hdc,a.x+offset, a.y-offset);
	LineTo(hdc,a.x+offset, a.y+offset);
	LineTo(hdc,a.x-offset, a.y+offset);
	LineTo(hdc,a.x-offset, a.y-offset);

}

void UnwrapMod::DrawEdge(HDC hdc, int a,int b, int vecA,int vecB, 
						 IPoint2 pa, IPoint2 pb, IPoint2 pvecA, IPoint2 pvecB)
{
	if ((vecA ==-1) || (vecB == -1))
	{
		MoveToEx(hdc,pa.x, pa.y, NULL);
		LineTo(hdc,pb.x, pb.y);
	}
	else
	{
		Point2 fpa,fpb,fpvecA,fpvecB;

		//		pvecA = pva;
		//		pvecB = pvb;

		fpa.x = (float)pa.x;
		fpa.y = (float)pa.y;

		fpb.x = (float)pb.x;
		fpb.y = (float)pb.y;

		fpvecA.x = (float)pvecA.x;
		fpvecA.y = (float)pvecA.y;

		fpvecB.x = (float)pvecB.x;
		fpvecB.y = (float)pvecB.y;

		MoveToEx(hdc,pa.x, pa.y, NULL);
		for (int i = 1; i < 7; i++)
		{
			float t = float (i)/7.0f;

			float s = (float)1.0-t;
			float t2 = t*t;
			Point2 p = ( ( s*fpa + (3.0f*t)*fpvecA)*s + (3.0f*t2)* fpvecB)*s + t*t2*fpb;
			LineTo(hdc,(int)p.x, (int)p.y);
		}

		LineTo(hdc,pb.x, pb.y);
		/*
		if ((currentPen!=selPen) && (currentPen!=frozenPen))
		SelectObject(hdc,handlePen);
		//draw handleA
		MoveToEx(hdc,pa.x, pa.y, NULL);
		LineTo(hdc,pvecA.x, pvecA.y);
		//draw handleB
		MoveToEx(hdc,pb.x, pb.y, NULL);
		LineTo(hdc,pvecB.x, pvecB.y);
		//draw curve
		*/
	}

}

void UnwrapMod::PaintView()
{

 	if (bringUpPanel)
	{
		MoveScriptUI();
		bringUpPanel = FALSE;
		SetFocus(hWnd);
		
	}

	// russom - August 21, 2006 - 804464
	// Very very quick switching between modifiers might create a
	// situation in which EndEditParam is called within the MoveScriptUI call above.
	// This will set the ip ptr to NULL and we'll crash further down.
	if( ip == NULL )
		return;

	//this can cached to only update when the selection has changed
	static BitArray clustersUsed;
	if ((showVertexClusterList) || (showShared))
	{

		BuildVertexClusterList();

		int largestClusterID = -1;
		for (int i=0; i<vertexClusterList.Count(); i++) 
		{
			if (vertexClusterList[i] > largestClusterID)
				largestClusterID = vertexClusterList[i];
		}

		clustersUsed.SetSize(largestClusterID+1);
		clustersUsed.ClearAll();

		//		BitArray holdSel(vsel);
		TransferSelectionStart();
      for (int i=0; i<TVMaps.v.Count(); i++) 
		{
			if ((i < vsel.GetSize()) && (vsel[i]) && (IsVertVisible(i)))
			{
				if (vertexClusterListCounts[i] >= 1)
				{
					int vCluster = vertexClusterList[i];
					//PELT
					if ((vCluster >= 0) && (vCluster < clustersUsed.GetSize()))
						clustersUsed.Set(vCluster);
				}
			}
		}
		TransferSelectionEnd(FALSE,FALSE);
		//		vsel = holdSel;
	}

	PAINTSTRUCT		ps;
	BeginPaint(hView,&ps);
	EndPaint(hView,&ps);
	TimeValue t = ip->GetTime();

	int frozenr = (int) (GetRValue(lineColor)*0.5f);
	int frozeng = (int) (GetGValue(lineColor)*0.5f);
	int frozenb = (int) (GetBValue(lineColor)*0.5f);
	COLORREF frozenColor = RGB(frozenr,frozeng,frozenb);
	COLORREF yellowColor = RGB(255,255,64);
	COLORREF darkyellowColor = RGB(117,117,28);
	COLORREF darkgreenColor = RGB(28,117,28);
	COLORREF greenColor = RGB(0,255,0);
	COLORREF blueColor = RGB(0,0,255);


	if (!fnGetRelativeTypeInMode())
		SetupTypeins();

	//need to compute the largest number of edges to store some data per poly
	int size = 0;
	for (int fi=0; fi<TVMaps.f.Count(); fi++) 
	{
		if (TVMaps.f[fi]->count > size)
			size = TVMaps.f[fi]->count;
	}

	if (!viewValid) 
	{
		//make sure our background texture is right
		if (!image && GetActiveMap()) 
			SetupImage();

		viewValid = TRUE;
		Point2 *pt = new Point2[size];
		IPoint2 *ipt = new IPoint2[size];

		//compute your zooms and scale
		float xzoom, yzoom;
		int width,height;
		ComputeZooms(hView,xzoom,yzoom,width,height);

		iBuf->Erase();
		HDC hdc = iBuf->GetDC();
		int i1, i2;
		GetUVWIndices(i1,i2);

		//blit the background to the back buffer		
		if (image && showMap) 
		{	
			//watje tile
			HDC tilehdc = iTileBuf->GetDC();
			if (!tileValid)
			{
				iTileBuf->Erase();
				tileValid = TRUE;
				Point3 p(0.0f,0.0f,0.0f);
				p[i2] = 1.0f;
				Point2 sp;
				sp = UVWToScreen(p,xzoom,yzoom,width,height);
				Rect dest;
				dest.left   = (int)sp.x;
				dest.top    = (int)sp.y;
				dest.right  = dest.left + int(xzoom)-1;
				dest.bottom = dest.top + int(yzoom)-1;
				Rect src;
				src.left   = src.top = 0;
				src.right  = iw;
				src.bottom = ih;

				Point2 spCenter;
				Point3 pCenter(0.0f,0.0f,0.0f);

				spCenter = UVWToScreen(pCenter,xzoom,yzoom,width,height);

				BOOL tileMap = TRUE;
				int iTileLimit = this->iTileLimit;
				if (!bTile) iTileLimit = 0;
				//				if ((!bTile) || (iTileLimit == 0))
				//compute startx
				//compute starty
				while (sp.x > 0)
					sp.x -= xzoom;
				while (sp.y > 0)
					sp.y -= yzoom;

				float ix = sp.x;


				int centerX,centerY;
				centerX = (int)spCenter.x;
				centerY = (int)spCenter.y;
				int xMinLimit,yMinLimit;
				int xMaxLimit,yMaxLimit;

				xMinLimit = centerX - ((int)xzoom * (iTileLimit));
				yMinLimit = (height-centerY) - ((int)yzoom * (iTileLimit));


				xMaxLimit = centerX + ((int)xzoom * (iTileLimit+1)) ;
				yMaxLimit = (height-centerY) + ((int)yzoom * (iTileLimit+1));

				int cxMinLimit,cyMinLimit;
				int cxMaxLimit,cyMaxLimit;

				cxMinLimit = centerX - ((int)xzoom * (0));
				cyMinLimit = (height-centerY) - ((int)yzoom * (0));


				cxMaxLimit = centerX + ((int)xzoom/* * (iTileLimit)*/) ;
				cyMaxLimit = (height-centerY) + ((int)yzoom /** (iTileLimit)*/);


				UBYTE *imageScaledContrast = NULL;

				Point3 backColor;
				backColor.x =  GetRValue(backgroundColor);
				backColor.y =  GetGValue(backgroundColor);
				backColor.z =  GetBValue(backgroundColor);

				int *xscaleList = new int[width];
				int *yscaleList = new int[height];

				float cx = sp.x;
				while (cx < 0.0f)
					cx += xzoom;
				float cy = sp.y;
				cy *= -1.0f;
				while (cy < 0.0f)
					cy += yzoom;



				float xinc = iw/(xzoom);
				float yinc = ih/(yzoom);

				float c = (xzoom - cx);							
				c = (c * iw)/xzoom;
				//compute x scale
				int id;
				for (int tix = 0; tix < width; tix++)
				{

					id = ((int)c)%iw;
					xscaleList[tix] = ((int)c)%iw;
					c+=xinc;
				}
				c = yzoom - cy;	
				c -= height - yzoom;
				c = (c * ih)/yzoom;
				//compute x scale
				//DebugPrint("Start y %f %f\n",yzoom,sp.y);
				//c = 0;
				for (int tix = 0; tix < height; tix++)
				{

					id = ((int)c)%ih;
					if (id < 0)
					{
						id = ih+id;
					}

					yscaleList[tix] = id;
					c+=yinc;
				}

				UBYTE *imageScaled = new UBYTE[ByteWidth(width)*(height)];
				UBYTE *p1 = imageScaled;
				float contrast = fContrast;

				for (int iy = 0; iy < height; iy++)
				{
					int y = yscaleList[iy];

					UBYTE *p2 = image +  (y*ByteWidth(iw));
					p1 = imageScaled + (iy*ByteWidth(width));
					for (int ix = 0; ix < width; ix++)
					{
						int x = xscaleList[ix];

						if ((ix > xMaxLimit) || (ix < xMinLimit) ||
							(iy > yMaxLimit) || (iy < yMinLimit) 
							)
						{
							*p1++ = backColor.z;
							*p1++ = backColor.y;
							*p1++ = backColor.x;	
						}
						else if (bTile)
						{

							if ( (!brightCenterTile) &&
								((ix <= cxMaxLimit) && (ix >= cxMinLimit) &&
								(iy <= cyMaxLimit) && (iy >= cyMinLimit) ) )
							{
								*p1++ = p2[x*3];
								*p1++ = p2[x*3+1];
								*p1++ = p2[x*3+2];

							}
							else
							{
								if (blendTileToBackGround)
								{
									*p1++ = (UBYTE) (backColor.z + ( (float)p2[x*3]-backColor.z) * contrast);
									*p1++ = (UBYTE) (backColor.y + ( (float)p2[x*3+1]-backColor.y) * contrast);
									*p1++ = (UBYTE) (backColor.x + ( (float)p2[x*3+2]-backColor.x) * contrast);
								}
								else
								{	
									*p1++ = (UBYTE) ((float)p2[x*3] * contrast);
									*p1++ = (UBYTE) ((float)p2[x*3+1] * contrast);
									*p1++ = (UBYTE) ((float)p2[x*3+2] * contrast);
								}


							}
						}
						else
						{
							if ((contrast == 1.0f) || (!brightCenterTile))
							{
								*p1++ = p2[x*3];
								*p1++ = p2[x*3+1];
								*p1++ = p2[x*3+2];
							}
							else if (blendTileToBackGround)
							{
								*p1++ = (UBYTE) (backColor.z + ( (float)p2[x*3]-backColor.z) * contrast);
								*p1++ = (UBYTE) (backColor.y + ( (float)p2[x*3+1]-backColor.y) * contrast);
								*p1++ = (UBYTE) (backColor.x + ( (float)p2[x*3+2]-backColor.x) * contrast);
							}
							else
							{	
								*p1++ = (UBYTE) ((float)p2[x*3] * contrast);
								*p1++ = (UBYTE) ((float)p2[x*3+1] * contrast);
								*p1++ = (UBYTE) ((float)p2[x*3+2] * contrast);
							}

						}
					}
				}
				dest.left   = 0;
				dest.top    = 0;
				dest.right  =  width;
				dest.bottom =  height;
				GetGPort()->DisplayMap(tilehdc, dest, 0,0,  imageScaled, ByteWidth(width));

				if (imageScaled)
					delete [] imageScaled;




			}
			BitBlt(hdc,0,0,width,height,tilehdc,0,0,SRCCOPY);

		} 

		/*
		HPEN gPen = CreatePen(PS_SOLID,2,RGB(100,100,100));
		Point3 p1(0.0f,0.0f,0.0f),p2(0.0f,0.0f,0.0f);
		Point2 sp1, sp2;
		p2[i1] = 1.0f;
		p2[i2] = 1.0f;
		sp1 = UVWToScreen(p1,xzoom,yzoom,width,height);
		sp2 = UVWToScreen(p2,xzoom,yzoom,width,height);			
		SelectObject(hdc,gPen);
		Rectangle(hdc,(int)sp1.x,(int)sp2.y,(int)sp2.x,(int)sp1.y);

		MoveToEx(hdc,(int)sp1.x, (int)0, NULL);
		LineTo(hdc,(int)sp1.x, (int)sp1.y);

		MoveToEx(hdc,(int)sp1.x, (int)sp1.y, NULL);
		LineTo(hdc,(int)width, (int)sp1.y);

		SelectObject(hdc,GetStockObject(WHITE_PEN));


		DeleteObject(gPen);
		*/

		Point3 selP3Color;
		selP3Color.x = (int) GetRValue(selColor);
		selP3Color.y = (int) GetGValue(selColor);
		selP3Color.z = (int) GetBValue(selColor);

		selP3Color.x += (255.0f - selP3Color.x) *0.25f;
		selP3Color.y += (255.0f - selP3Color.y) *0.25f;
		selP3Color.z += (255.0f - selP3Color.z) *0.25f;

		COLORREF selFillColor  = RGB((int)selP3Color.x,(int)selP3Color.y,(int)selP3Color.z);
		// Paint faces		
		HPEN selPen   = CreatePen(PS_SOLID,2,selColor);
		HPEN selThinPen   = CreatePen(PS_DOT,1,selColor);
		HPEN unselPen = CreatePen(PS_SOLID,0,lineColor);
		HPEN frozenPen = CreatePen(PS_SOLID,0,frozenColor);
		HPEN yellowPen = CreatePen(PS_SOLID,0,yellowColor);
		HPEN darkyellowPen = CreatePen(PS_SOLID,0,darkyellowColor);
		HPEN darkgreenPen = CreatePen(PS_SOLID,0,darkgreenColor);
		HPEN greenPen = CreatePen(PS_SOLID,0,greenColor);
		HPEN bluePen = CreatePen(PS_SOLID,0,blueColor);

		HPEN openEdgePen = CreatePen(PS_SOLID,0,openEdgeColor);

		HPEN handlePen = CreatePen(PS_SOLID,0,handleColor);

		HPEN freeFormPen = CreatePen(PS_SOLID,0,freeFormColor);
		HPEN sharedPen = CreatePen(PS_SOLID,0,sharedColor);

		HPEN currentPen = NULL;

		BitArray visibleFaces;
		visibleFaces.SetSize(TVMaps.f.Count());
		visibleFaces.ClearAll();

		Tab<IPoint2> transformedPoints;
		transformedPoints.SetCount(TVMaps.v.Count());

		for (int i=0; i<TVMaps.v.Count(); i++) 
		{
			Point3 tvPoint = UVWToScreen(GetPoint(t,i),xzoom,yzoom,width,height);
			IPoint2 tvPoint2;
			tvPoint2.x  = (int) tvPoint.x;
			tvPoint2.y  = (int) tvPoint.y;
			//			tvPoint2.x  = (int) tvPoint[i1];
			//			tvPoint2.y  = (int) tvPoint[i2];

			transformedPoints[i] = tvPoint2;
		}

		///this is a debug flag so you can turn off the display of faces and vertices



		if (!drawOnlyBounds)
		{


			currentPen = unselPen;
			IsSelectedSetup();
			int colorMode = 1;



			//draw grid
			if ((gridVisible) && (gridSize>0.0f))
			{




				Point3 pz(0.0f,0.0f,0.0f);
				pz[i1] = gridSize;

				Point2 checkp = UVWToScreen(Point3(0.0f,0.0f,0.0f),xzoom,yzoom,width,height);
				Point2 checkp2 = UVWToScreen(pz,xzoom,yzoom,width,height);

				if ((checkp2.x - checkp.x) > 4.5f)
				{


					HPEN gridPen = CreatePen(PS_SOLID,1,gridColor);
					SelectObject(hdc,gridPen);

					//do x++
					float x = 0.0f;
					int iX = -1;
					while (iX  < width) 
					{

						//						Point3 p1(x,0.0f,0.0f);
						Point3 p1(0.0f,0.0f,0.0f);
						p1[i1] = x;

						Point2 p = UVWToScreen(p1,xzoom,yzoom,width,height);

						int dX = int(p.x); 

						if (dX != iX)
						{
							MoveToEx(hdc,dX, 0, NULL);
							LineTo(hdc,dX, height);						
						}

						iX = dX;

						x+= gridSize;
					}

					//do x--
					iX = 1;
					x = 0.0f;
					while (iX  > 0) 
					{

						//						Point3 p1(x,0.0f,0.0f);
						Point3 p1(0.0f,0.0f,0.0f);
						p1[i1] = x;

						Point2 p = UVWToScreen(p1,xzoom,yzoom,width,height);

						int dX = int(p.x); 

						if (dX != iX)
						{
							MoveToEx(hdc,dX, 0, NULL);
							LineTo(hdc,dX, height);						
						}

						iX = dX;

						x-= gridSize;
					}

					//do y++
					float y = 0.0f;
					int iY = -1;
					while (iY  < height) 
					{

						//						Point3 p1(0.0f,y,0.0f);
						Point3 p1(0.0f,0.0f,0.0f);
						p1[i2] = y;

						Point2 p = UVWToScreen(p1,xzoom,yzoom,width,height);

						int dY = int(p.y); 

						if (dY != iY)
						{
							MoveToEx(hdc,0,dY, NULL);
							LineTo(hdc,width, dY);						
						}

						iY = dY;

						y-= gridSize;
					}

					//do x--
					iY = 1;
					y = 0.0f;
					while (iY  > 0) 
					{

						//						Point3 p1(0.0f,y,0.0f);
						Point3 p1(0.0f,0.0f,0.0f);
						p1[i2] = y;

						Point2 p = UVWToScreen(p1,xzoom,yzoom,width,height);

						int dY = int(p.y); 

						if (dY != iY)
						{
							MoveToEx(hdc,0,dY, NULL);
							LineTo(hdc,width, dY);						
						}

						iY = dY;

						y+= gridSize;
					}
					SelectObject(hdc,GetStockObject(WHITE_PEN));
					DeleteObject(gridPen);

				}
			}

			//new
			int gr,gg,gb;
			gr = (int) (GetRValue(gridColor) *0.45f);
			gg = (int) (GetGValue(gridColor) *0.45f);
			gb = (int) (GetBValue(gridColor) *0.45f);
			HPEN gPen = CreatePen(PS_SOLID,3,RGB(gr,gg,gb));
			Point3 p1(0.0f,0.0f,0.0f),p2(0.0f,0.0f,0.0f);
			Point2 sp1, sp2;
			p2[i1] = 1.0f;
			p2[i2] = 1.0f;
			sp1 = UVWToScreen(p1,xzoom,yzoom,width,height);
			sp2 = UVWToScreen(p2,xzoom,yzoom,width,height);			
			SelectObject(hdc,gPen);
			Rectangle(hdc,(int)sp1.x,(int)sp2.y,(int)sp2.x+1,(int)sp1.y);

			MoveToEx(hdc,(int)sp1.x, (int)0, NULL);
			LineTo(hdc,(int)sp1.x, (int)sp1.y);

			MoveToEx(hdc,(int)sp1.x, (int)sp1.y, NULL);
			LineTo(hdc,(int)width, (int)sp1.y);

			SelectObject(hdc,GetStockObject(WHITE_PEN));
			DeleteObject(gPen);

			

			if (TVSubObjectMode == 0) //only need to show open edges, handles, and shared edges/verts
			{
				//draw regular edges
            for (int i=0; i<TVMaps.ePtrList.Count(); i++) 
				{
					int a = TVMaps.ePtrList[i]->a;
					int b = TVMaps.ePtrList[i]->b;

					BOOL sharedEdge = FALSE;
					if (showShared)
					{
						int ct =0;
						int vCluster = vertexClusterList[a];
						if ((vCluster >=0) && (clustersUsed[vCluster])) ct++;

						vCluster = vertexClusterList[b];
						if ((vCluster >=0) && (clustersUsed[vCluster])) ct++;
						if (ct ==2) sharedEdge = TRUE;
					}

					BOOL paintEdge = TRUE;
					//					if ((!esel[i]) && (!displayHiddenEdges) && (TVMaps.ePtrList[i]->faceList.Count() > 1))
					if ( (!displayHiddenEdges) && (TVMaps.ePtrList[i]->faceList.Count() > 1))
					{
						if (TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA)
							paintEdge =FALSE;
					}

					if (IsVertVisible(a) && IsVertVisible(b) && paintEdge)
					{
						int veca = TVMaps.ePtrList[i]->avec;
						int vecb = TVMaps.ePtrList[i]->bvec;

						//draw shared edges
						if (sharedEdge && (!(vsel[a] && vsel[b])))
						{
							SelectObject(hdc,sharedPen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
						}
						//draw open edges
						else if ((TVMaps.ePtrList[i]->faceList.Count() == 1) && (displayOpenEdges))
						{

							SelectObject(hdc,openEdgePen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
						}
						else if ((TVMaps.v[a].flags & FLAG_FROZEN) || (TVMaps.v[b].flags & FLAG_FROZEN))
						{
							SelectObject(hdc,frozenPen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);

						}
						//draw normal edge
						else 
						{
							if (!showEdgeDistortion)
							{
								SelectObject(hdc,unselPen);
								DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
							}
						}
						//draw handles			
						if ((veca!= -1) && (vecb!= -1))
						{
							SelectObject(hdc,handlePen);
							DrawEdge(hdc,  b, vecb, -1,-1, transformedPoints[b], transformedPoints[vecb], transformedPoints[b], transformedPoints[vecb]);
							DrawEdge(hdc,  a, veca, -1,-1, transformedPoints[a], transformedPoints[veca], transformedPoints[a], transformedPoints[veca]);
						}
					}
				}
			}
			else if (TVSubObjectMode == 1) //edge mode
			{
				//draw regular edges
            for (int i=0; i<TVMaps.ePtrList.Count(); i++) 
				{
					int a = TVMaps.ePtrList[i]->a;
					int b = TVMaps.ePtrList[i]->b;

					BOOL sharedEdge = FALSE;
					if (showShared)
					{
						int ct =0;
						int vCluster = vertexClusterList[a];
						if ((vCluster >=0) && (clustersUsed[vCluster])) ct++;

						vCluster = vertexClusterList[b];
						if ((vCluster >=0) && (clustersUsed[vCluster])) ct++;
						if (ct ==2) sharedEdge = TRUE;


					}


					BOOL paintEdge = TRUE;
					if ((!esel[i]) && (!displayHiddenEdges) && (TVMaps.ePtrList[i]->faceList.Count() > 1))
					{
						if (TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA)
							paintEdge =FALSE;
					}
					

					if ((IsVertVisible(a) || IsVertVisible(b)) && paintEdge)
					{
						int veca = TVMaps.ePtrList[i]->avec;
						int vecb = TVMaps.ePtrList[i]->bvec;

						//draw selected hidden edges
						if ((TVMaps.v[a].flags & FLAG_FROZEN) && (TVMaps.v[b].flags & FLAG_FROZEN))
						{
							SelectObject(hdc,frozenPen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
						}
						else if ((TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA) && (esel[i]) )
						{
							SelectObject(hdc,selThinPen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
						}
						//draw selected  edges
						else if (esel[i]) 
						{
							SelectObject(hdc,selPen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
						}
						//draw shared edges
						else if ((sharedEdge) && (!esel[i]))
						{
							SelectObject(hdc,sharedPen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
						}
						//draw open edges
						else if ((TVMaps.ePtrList[i]->faceList.Count() == 1) && (displayOpenEdges))
						{
							SelectObject(hdc,openEdgePen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
						}
						else //draw normal edge
						{
							if (!showEdgeDistortion)
							{
								SelectObject(hdc,unselPen);
								DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
							}
						}
						//draw handles			
						if ((veca!= -1) && (vecb!= -1))
						{
							SelectObject(hdc,handlePen);
							DrawEdge(hdc,  b, vecb, -1,-1, transformedPoints[b], transformedPoints[vecb], transformedPoints[b], transformedPoints[vecb]);
							DrawEdge(hdc,  a, veca, -1,-1, transformedPoints[a], transformedPoints[veca], transformedPoints[a], transformedPoints[veca]);
						}
					}
				}
			}
			else if (TVSubObjectMode == 2) //face mode
			{
				//draw regular edges
            for (int i=0; i<TVMaps.ePtrList.Count(); i++) 
				{
					int a = TVMaps.ePtrList[i]->a;
					int b = TVMaps.ePtrList[i]->b;

					BOOL sharedEdge = FALSE;
					if (showShared)
					{
						int ct =0;
						int vCluster = vertexClusterList[a];
						if ((vCluster >=0) && (clustersUsed[vCluster])) ct++;

						vCluster = vertexClusterList[b];
						if ((vCluster >=0) && (clustersUsed[vCluster])) ct++;
						if (ct ==2) sharedEdge = TRUE;
					}


					BOOL paintEdge = TRUE;
					//					if ((!esel[i]) && (!displayHiddenEdges) && (TVMaps.ePtrList[i]->faceList.Count() > 1))
					if ( (!displayHiddenEdges) && (TVMaps.ePtrList[i]->faceList.Count() > 1))
					{
						if (TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA)
							paintEdge =FALSE;
					}

					if ((IsVertVisible(a) || IsVertVisible(b)) && paintEdge)
					{
						int veca = TVMaps.ePtrList[i]->avec;
						int vecb = TVMaps.ePtrList[i]->bvec;


						//draw shared edges
						if ((TVMaps.v[a].flags & FLAG_FROZEN) && (TVMaps.v[b].flags & FLAG_FROZEN))
						{
							SelectObject(hdc,frozenPen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
						}
						else if (sharedEdge)
						{
							SelectObject(hdc,sharedPen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
						}
						//draw open edges
						else if ((TVMaps.ePtrList[i]->faceList.Count() == 1) && (displayOpenEdges))
						{
							SelectObject(hdc,openEdgePen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
						}
						else //draw normal edge
						{
							if (!showEdgeDistortion)
							{
							SelectObject(hdc,unselPen);
							DrawEdge(hdc,  a, b, veca,vecb, transformedPoints[a], transformedPoints[b], transformedPoints[veca], transformedPoints[vecb]);
							}
						}
						//draw handles			
						if ((veca!= -1) && (vecb!= -1))
						{
							SelectObject(hdc,handlePen);
							DrawEdge(hdc,  b, vecb, -1,-1, transformedPoints[b], transformedPoints[vecb], transformedPoints[b], transformedPoints[vecb]);
							DrawEdge(hdc,  a, veca, -1,-1, transformedPoints[a], transformedPoints[veca], transformedPoints[a], transformedPoints[veca]);
						}
					}
				}

			}
			//now do selected faces
			SelectObject(hdc,selPen);
			HBRUSH  selBrush = NULL;

			if (fnGetFillMode() != FILL_MODE_OFF)
			{
				if (fnGetFillMode() != FILL_MODE_SOLID)
					SetBkMode( hdc, TRANSPARENT);

				if (fnGetFillMode() == FILL_MODE_SOLID)
					selBrush = CreateSolidBrush(selFillColor);
				else if (fnGetFillMode() == FILL_MODE_BDIAGONAL)
					selBrush = CreateHatchBrush(HS_BDIAGONAL, selFillColor );
				else if (fnGetFillMode() == FILL_MODE_CROSS)
					selBrush = CreateHatchBrush(HS_CROSS, selFillColor );
				else if (fnGetFillMode() == FILL_MODE_DIAGCROSS)
					selBrush = CreateHatchBrush(HS_DIAGCROSS, selFillColor );
				else if (fnGetFillMode() == FILL_MODE_FDIAGONAL)
					selBrush = CreateHatchBrush(HS_FDIAGONAL, selFillColor );
				else if (fnGetFillMode() == FILL_MODE_HORIZONAL)
					selBrush = CreateHatchBrush(HS_HORIZONTAL, selFillColor );
				else if (fnGetFillMode() == FILL_MODE_VERTICAL)
					selBrush = CreateHatchBrush(HS_VERTICAL, selFillColor );

				if (selBrush) SelectObject(hdc,selBrush);
			}

			POINT *polypt = new POINT[size+(7*7)];

			if (selBrush)
			{	
            for (int i=0; i<TVMaps.f.Count(); i++) 
				{
					// Grap the three points, xformed
					BOOL hidden = FALSE;

					if (IsFaceVisible(i) && (TVSubObjectMode == 2) && (fsel[i]))
					{


						int pcount = 3;
						pcount = TVMaps.f[i]->count;
						//if it is patch with curve mapping
						if ( (TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) &&
							(TVMaps.f[i]->vecs) )

						{
							Spline3D spl;
							spl.NewSpline();
							BOOL pVis[4];
							for (int j=0; j<pcount; j++) 
							{
								Point3 in, p, out;
								IPoint2 iin, ip, iout;
								int index = TVMaps.f[i]->t[j];
								ip = transformedPoints[index];
								pVis[j] = IsVertVisible(index);

								index = TVMaps.f[i]->vecs->handles[j*2];
								iout = transformedPoints[index];
								BOOL outVis = IsVertVisible(index);
								if (j==0)
									index = TVMaps.f[i]->vecs->handles[pcount*2-1];
								else index = TVMaps.f[i]->vecs->handles[j*2-1];

								iin = transformedPoints[index];

								in.x = (float)iin.x;
								in.y = (float)iin.y;
								in.z = 0.0f;

								out.x = (float)iout.x;
								out.y = (float)iout.y;
								out.z = 0.0f;

								p.x = (float)ip.x;
								p.y = (float)ip.y;
								p.z = 0.0f;

								BOOL inVis = IsVertVisible(index);

								SplineKnot kn(KTYPE_BEZIER_CORNER, LTYPE_CURVE, p, in, out);
								spl.AddKnot(kn);

							}
							spl.SetClosed();
							spl.ComputeBezPoints();
							//draw curves
							Point2 lp;
							int polyct = 0;
                     for (int j=0; j<pcount; j++) 
							{
								int jNext = j+1;
								if (jNext >= pcount) jNext = 0;
								if (pVis[j] && pVis[jNext])
								{
									Point3 p;
									IPoint2 ip;
									int index = TVMaps.f[i]->t[j];
									ip = transformedPoints[index];
									MoveToEx(hdc,ip.x, ip.y, NULL);
									if ((j==0) && (selBrush))
									{
										polypt[polyct].x = ip.x;
										polypt[polyct++].y = ip.y;
									}

									for (int iu = 1; iu < 5; iu++)
									{
										float u = (float) iu/5.f;
										p = spl.InterpBezier3D(j, u);
										if (selBrush) 
										{
											polypt[polyct].x = (int)p.x;
											polypt[polyct++].y = (int)p.y;
										}
									}

									if (j == pcount-1)
										index = TVMaps.f[i]->t[0];
									else index = TVMaps.f[i]->t[j+1];
									ip = transformedPoints[index];
									if (selBrush) 
									{
										polypt[polyct].x = (int)ip.x;
										polypt[polyct++].y = (int)ip.y;
									}

								}

							}
							if (selBrush) Polygon(hdc, polypt,  polyct);


						}

						else  //else it is just regular poly so just draw the straight edges
						{
							for (int j=0; j<pcount; j++) 
							{
								int index = TVMaps.f[i]->t[j];
								ipt[j] = transformedPoints[index]; //UVWToScreen(GetPoint(t,index),xzoom,yzoom,width,height);
								if (TVMaps.v[index].flags & FLAG_HIDDEN) hidden = TRUE;
							}
							// Now draw the face
							if (!hidden)
							{
								MoveToEx(hdc,ipt[0].x, ipt[0].y, NULL);
								if (selBrush) 
								{
									polypt[0].x = ipt[0].x;
									polypt[0].y = ipt[0].y;
								}
                        for (int j=0; j<pcount; j++) 
								{
									if ((selBrush) && (j != (pcount-1)))
									{
										polypt[j+1].x = ipt[j+1].x;
										polypt[j+1].y = ipt[j+1].y;
									}

								}
								if (selBrush) Polygon(hdc, polypt,  pcount);
							}
						}
					}

				}
			}
			delete [] polypt;
			if (fnGetFillMode() != FILL_MODE_OFF)
			{
				if (selBrush) 
				{
					SelectObject(hdc,GetStockObject(WHITE_BRUSH));
					DeleteObject(selBrush);
				}
				SetBkMode( hdc, OPAQUE);
			}

			//PELT 
			//draw the mirror plane
			BitArray isSpring;
			isSpring.SetSize(TVMaps.ePtrList.Count());
			isSpring.ClearAll();
			Tab<float> peltSpringLength;
			peltSpringLength.SetCount(TVMaps.ePtrList.Count());
			if (peltData.peltDialog.hWnd)
			{
				Point3 mirrorVec = Point3(0.0f,1.0f,0.0f);
				Matrix3 tm(1);
				tm.RotateZ(peltData.GetMirrorAngle());
				mirrorVec = mirrorVec  * tm;
				mirrorVec *= 2.0f;

				Matrix3 tma(1);
				tma.RotateZ(peltData.GetMirrorAngle()+PI*0.5f);
				Point3 mirrorVeca = Point3(0.0f,1.0f,0.0f);
				mirrorVeca = mirrorVeca  * tma;
				mirrorVeca *= 2.0f;


				//get the center
				Point3 ma,mb;
				ma = peltData.GetMirrorCenter() + mirrorVec;
				mb = peltData.GetMirrorCenter() - mirrorVec;
				//get our vec
				Point2 pa = UVWToScreen(ma,xzoom,yzoom,width,height);
				Point2 pb = UVWToScreen(mb,xzoom,yzoom,width,height);

				SelectObject(hdc,yellowPen);

				MoveToEx(hdc,(int)pa.x,(int) pa.y, NULL);
				LineTo(hdc,(int)pb.x,(int) pb.y);						

				ma = peltData.GetMirrorCenter();
				mb = peltData.GetMirrorCenter() - mirrorVeca;
				//get our vec
				pa = UVWToScreen(ma,xzoom,yzoom,width,height);
				pb = UVWToScreen(mb,xzoom,yzoom,width,height);

				MoveToEx(hdc,(int)pa.x,(int) pa.y, NULL);
				LineTo(hdc,(int)pb.x,(int) pb.y);						

			
			//draw our springs	



				COLORREF baseColor = ColorMan()->GetColor(EDGEDISTORTIONCOLORID);
				COLORREF goalColor = ColorMan()->GetColor(EDGEDISTORTIONGOALCOLORID);

				Color goalc(goalColor);
				Color basec(baseColor);

            for (int i = 0; i < peltData.springEdges.Count(); i++)
				{
					
					if (peltData.springEdges[i].isEdge)
					{
						int a,b;
						a = peltData.springEdges[i].v1;
						b = peltData.springEdges[i].v2;

						if ( ( a >= 0) && (a < TVMaps.v.Count()) && 
							 ( b >= 0) && (b < TVMaps.v.Count()) )
						{

							SelectObject(hdc,selThinPen);
							MoveToEx(hdc,transformedPoints[a].x,transformedPoints[a].y, NULL);
							LineTo(hdc,transformedPoints[b].x,transformedPoints[b].y);	

							Point3 vec = (TVMaps.v[b].p-TVMaps.v[a].p);
							if (Length(vec) > 0.00001f)
							{
								vec = Normalize(vec)*peltData.springEdges[i].dist*peltData.springEdges[i].distPer;
								vec = TVMaps.v[b].p - vec;

								Point3 tvPoint = UVWToScreen(vec ,xzoom,yzoom,width,height);
								IPoint2 tvPoint2;
								tvPoint2.x  = (int) tvPoint.x;
								tvPoint2.y  = (int) tvPoint.y;

								SelectObject(hdc,sharedPen);
								MoveToEx(hdc,transformedPoints[b].x,transformedPoints[b].y,NULL);
								LineTo(hdc,tvPoint2.x,tvPoint2.y);	
							}
						}
					}
					else if (showEdgeDistortion)
					{
						int edgeIndex = peltData.springEdges[i].edgeIndex;
						if (edgeIndex != -1)
						{
							isSpring.Set(edgeIndex);
							peltSpringLength[edgeIndex] = peltData.springEdges[i].dist;
						}
					}
				}
			}

			if (showEdgeDistortion)
			{
 				COLORREF goalColor = ColorMan()->GetColor(EDGEDISTORTIONCOLORID);
				COLORREF baseColor = ColorMan()->GetColor(EDGEDISTORTIONGOALCOLORID);

				Color goalc(goalColor);
				Color basec(baseColor);

 				gr = (int) (255 * basec.r);
				gg = (int) (255 * basec.g);
 				gb = (int) (255 * basec.b);
				HPEN gPen = CreatePen(PS_SOLID,1,RGB(gr,gg,gb));
				SelectObject(hdc,gPen);

				Tab<int> whichColor;
				Tab<float> whichPer;
				Tab<float> goalLength;
				Tab<float> currentLength;
				Tab<Point3> vec;
				whichColor.SetCount(TVMaps.ePtrList.Count());
				whichPer.SetCount(TVMaps.ePtrList.Count());
				currentLength.SetCount(TVMaps.ePtrList.Count());
				goalLength.SetCount(TVMaps.ePtrList.Count());
				vec.SetCount(TVMaps.ePtrList.Count());
				int numberOfColors = 10;
				

            for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
				{
					BOOL showEdge = TRUE;
					int edgeIndex =  i;
					if (edgeIndex != -1)
					{
						if (TVSubObjectMode == 1)
						{
							if (esel[edgeIndex])
								showEdge = FALSE;

						}
						if (TVMaps.ePtrList[edgeIndex]->flags & FLAG_HIDDENEDGEA)
							showEdge = FALSE;
						if (peltData.peltDialog.hWnd)
						{
							if (!isSpring[i])
								showEdge = FALSE;
						}

						whichColor[i] = -1;
						if (showEdge)
						{
 							Point3 pa, pb;
							int a,b;
							a = TVMaps.ePtrList[edgeIndex]->a;
 							b = TVMaps.ePtrList[edgeIndex]->b;

							int va,vb;
							va = TVMaps.ePtrList[edgeIndex]->ga;
							vb = TVMaps.ePtrList[edgeIndex]->gb;
							
							if ((IsVertVisible(a) || IsVertVisible(b)) )
							{
								if (peltData.peltDialog.hWnd)
									goalLength[i] = peltSpringLength[i];
								else goalLength[i] = Length(TVMaps.geomPoints[va]-TVMaps.geomPoints[vb])*edgeScale* edgeDistortionScale;
								pa = TVMaps.v[a].p;
								pb = TVMaps.v[b].p;
								pa.z = 0.0f;
								pb.z = 0.0f;

								vec[i] = (pb-pa);
								Point3 uva,uvb;
								uva = pa;
								uvb = pb;
								uva.z = 0.0f;
								uvb.z = 0.0f;

								currentLength[i] = Length(uva-uvb);

								
								float dif = fabs(goalLength[i]-currentLength[i])*4.0f;
								float per = ((dif/goalLength[i]) * numberOfColors);
								if (per < 0.0f) per = 0.0f;
								if (per > 1.0f) per = 1.0f;
								whichPer[i] = per ;
								whichColor[i] = (int)(whichPer[i]*(numberOfColors-1));
							}
						}
					}
				}

				Tab<IPoint2> la,lb;
				
				la.SetCount(TVMaps.ePtrList.Count());
				lb.SetCount(TVMaps.ePtrList.Count());
				
				Tab<int> colorList;
				colorList.SetCount(TVMaps.ePtrList.Count());
				BitArray fullEdge;
				fullEdge.SetSize(TVMaps.ePtrList.Count());
				fullEdge.ClearAll();

				SelectObject(hdc,GetStockObject(WHITE_PEN));

				numberOfColors = 10;
            for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
				{
					BOOL showEdge = TRUE;
					int edgeIndex =  i;
					if (edgeIndex != -1)
					{
						if (TVSubObjectMode == 1)
						{
							if (esel[edgeIndex])
								showEdge = FALSE;

						}
						if (TVMaps.ePtrList[edgeIndex]->flags & FLAG_HIDDENEDGEA)
							showEdge = FALSE;
					}
					else showEdge = FALSE;
					//get our edge spring length goal
					if (showEdge)
					{
						int va,vb;
						va = TVMaps.ePtrList[edgeIndex]->ga;
						vb = TVMaps.ePtrList[edgeIndex]->gb;
						

//						float goalLength = peltData.springEdges[i].dist;
						int a,b;
 						a = TVMaps.ePtrList[edgeIndex]->a;
 						b = TVMaps.ePtrList[edgeIndex]->b;
 						if ((IsVertVisible(a) || IsVertVisible(b)) )
						{
							
							float goalL = goalLength[i];//Length(TVMaps.geomPoints[va]-TVMaps.geomPoints[vb])*edgeScale* edgeDistortionScale;
/*
							if (i < edgeWeight.Count())
							{
//								if (edgeWeight[i] > 0.0f)
									goalL = edgeWeight[i];
								
							}
							if (goalL < 0.0f) continue;
*/

							//get our current length
 							Point3 pa, pb;
							pa = TVMaps.v[a].p;
							pb = TVMaps.v[b].p;
							pa.z = 0.0f;
							pb.z = 0.0f;

							Point3 vec = (pb-pa);
							Point3 uva,uvb;
							uva = pa;
							uvb = pb;
							uva.z = 0.0f;
							uvb.z = 0.0f;

							float currentL = currentLength[i];//Length(uva-uvb);
							float per = 1.0f;
							float dif = fabs(goalL-currentL)*4.0f;
							per = (dif/goalL);

							if (per < 0.0f) per = 0.0f;
							if (per > 1.0f) per = 1.0f;
							//compute the color
							//get the center and draw out
							Color c = (basec * (per)) + (goalc*(1.0f-per));
							int gr=0,gg=0,gb=0;
 							gr = (int) (255 * c.r);
							gg = (int) (255 * c.g);
 					 		gb = (int) (255 * c.b);

							colorList[i] = int(per * numberOfColors);

							if (currentL <= goalL)
							{
								fullEdge.Set(i);
								la[edgeIndex] = transformedPoints[a];
								lb[edgeIndex] = transformedPoints[b];

							}
							else
							{
								Point3 mid = (pb+pa) * 0.5f;
								Point3 nvec = Normalize(vec);
								nvec = nvec * goalL * 0.5f;

								Point3 tvPoint = UVWToScreen(mid + nvec ,xzoom,yzoom,width,height);
								IPoint2 aPoint;
								aPoint.x  = (int) tvPoint.x;
								aPoint.y  = (int) tvPoint.y;

								Point3 tvPoint2 = UVWToScreen(mid - nvec ,xzoom,yzoom,width,height);
								IPoint2 bPoint;
								bPoint.x  = (int) tvPoint2.x;
								bPoint.y  = (int) tvPoint2.y;



								la[edgeIndex] = aPoint;
								lb[edgeIndex] = bPoint;



								MoveToEx(hdc,aPoint.x,aPoint.y,NULL);
 								LineTo(hdc,transformedPoints[b].x,transformedPoints[b].y);
								MoveToEx(hdc,bPoint.x,bPoint.y,NULL);
								LineTo(hdc,transformedPoints[a].x,transformedPoints[a].y);


							}

						}

					}
				}
				for (int j = 0; j <= numberOfColors;j++)
				{
					float per =(float)j/(float)numberOfColors;
					Color c = (basec * (per)) + (goalc*(1.0f-per));
					int gr=0,gg=0,gb=0;
 					gr = (int) (255 * c.r);
					gg = (int) (255 * c.g);
 					gb = (int) (255 * c.b);

					HPEN gPen = CreatePen(PS_SOLID,1,RGB(gr,gg,gb));
					SelectObject(hdc,gPen);
               for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
					{
						BOOL showEdge = TRUE;
						int edgeIndex =  i;
						if (edgeIndex != -1)
						{
							if (TVSubObjectMode == 1)
							{
								if (esel[edgeIndex])
									showEdge = FALSE;

							}
							if (TVMaps.ePtrList[edgeIndex]->flags & FLAG_HIDDENEDGEA)
								showEdge = FALSE;
						}
						else showEdge = FALSE;

						if (showEdge && (colorList[i] == j))
						{
							int a,b;
							a = TVMaps.ePtrList[i]->a;
 							b = TVMaps.ePtrList[i]->b;

							MoveToEx(hdc,la[i].x,la[i].y,NULL);
							LineTo(hdc,lb[i].x,lb[i].y);	
						}
					}
					SelectObject(hdc,GetStockObject(WHITE_BRUSH));					
					DeleteObject(gPen);
				}
				

			}


				
				

			SelectObject(hdc,sharedPen);
         for (int i = 0; i < peltData.rigPoints.Count(); i++)
			{
				int a = peltData.rigPoints[i].lookupIndex;
				int b = peltData.rigPoints[0].lookupIndex;
				if ((i+1) <  peltData.rigPoints.Count())
					b = peltData.rigPoints[i+1].lookupIndex;

				MoveToEx(hdc,transformedPoints[a].x,transformedPoints[a].y, NULL);
				LineTo(hdc,transformedPoints[b].x,transformedPoints[b].y);
			}




			//get our soft selection colors
			Point3 selSoft = GetUIColor(COLOR_SUBSELECTION_SOFT);
			Point3 selMedium = GetUIColor(COLOR_SUBSELECTION_MEDIUM);
			Point3 selHard = GetUIColor(COLOR_SUBSELECTION_HARD);


			// Now paint points	
 			if (TVSubObjectMode == 0)
			{


				SelectObject(hdc,selPen);
            for (int i=0; i<TVMaps.v.Count(); i++) 
				{
					//PELT
					if (IsVertVisible(i) && (i < usedVertices.GetSize()) && usedVertices[i] && vsel[i])

					{
						

						int x  = transformedPoints[i].x;
						int y  = transformedPoints[i].y;


						if (tickSize==1)
						{
							MoveToEx(hdc,x-1,y-1, NULL);
							LineTo(hdc,x,y);						
						}
						else
							Rectangle(hdc,
								x-tickSize+1,y-tickSize+1,
								x+tickSize+1,y+tickSize+1);		

						if ( (showVertexClusterList) )
						{
							int vCluster = vertexClusterList[i];
							if ((vCluster >=0) && (clustersUsed[vCluster]))
							{

								TSTR vertStr;
								vertStr.printf("%d",vCluster,vertexClusterListCounts[i]);
								TextOut(hdc, x+4,y-4,vertStr,vertStr.Length());
							}
						}


					}

				}
				SelectObject(hdc,GetStockObject(BLACK_PEN));

            for (int i=0; i<TVMaps.v.Count(); i++) 
				{
					//PELT
					if (IsVertVisible(i) && (i < usedVertices.GetSize()) && usedVertices[i] && (!vsel[i]))

					{
						Point3 selColor(0.0f,0.0f,0.0f);

						ipt[0] = transformedPoints[i];


						if (TVMaps.v[i].influence == 0.0f)
						{
						}
						else if (TVMaps.v[i].influence <0.5f)
							selColor = selSoft + ( (selMedium-selSoft) * (TVMaps.v[i].influence/0.5f));
						else if (TVMaps.v[i].influence<=1.0f)
							selColor = selMedium + ( (selHard-selMedium) * ((TVMaps.v[i].influence-0.5f)/0.5f));

						COLORREF inflColor;
						inflColor = RGB((int)(selColor.x * 255.f), (int)(selColor.y * 255.f),(int)(selColor.z * 255.f));

						HPEN inflPen = CreatePen(PS_SOLID,0,inflColor);
						BOOL largeTick = FALSE;
						if (TVMaps.v[i].flags & FLAG_FROZEN)
							SelectObject(hdc,frozenPen);
						else if (vsel[i]) 
						{
							SelectObject(hdc,selPen);
							largeTick = TRUE;
						}
						else if (TVMaps.v[i].influence == 0.0f)
							SelectObject(hdc,unselPen);
						else 
						{
							SelectObject(hdc,inflPen);
							largeTick = TRUE;
						}

						int x  = transformedPoints[i].x;
						int y  = transformedPoints[i].y;

						if (largeTick)
						{
						if (tickSize==1)
						{
							MoveToEx(hdc,x-1,y-1, NULL);
							LineTo(hdc,x,y);						
						}
						else Rectangle(hdc,
								x-tickSize+1,y-tickSize+1,
								x+tickSize+1,y+tickSize+1);		
						}
						else Rectangle(hdc,
							x-1,y-1,
							x+1,y+1);		

						if ((showShared) && (!vsel[i]))
						{
							int vCluster = vertexClusterList[i];
							if ((vCluster >=0) && (clustersUsed[vCluster]))
							{
								SelectObject(hdc,sharedPen);

								Rectangle(hdc,
									x-2,y-2,
									x+2,y+2);
							}

						}
						if ( (showVertexClusterList) )
						{
							int vCluster = vertexClusterList[i];
							if ((vCluster >=0) && (clustersUsed[vCluster]))
							{

								TSTR vertStr;
								vertStr.printf("%d",vCluster,vertexClusterListCounts[i]);
								TextOut(hdc, x+4,y-4,vertStr,vertStr.Length());
							}
						}

						SelectObject(hdc,GetStockObject(BLACK_PEN));
						DeleteObject(inflPen);
					}

				}
			}
			if ((gridSnap) && (mouseHitVert >= 0) && (mouseHitVert < transformedPoints.Count()))
			{
				int sr,sg,sb;
				sr = (int) GetRValue(gridColor) + (int) (GetRValue(gridColor) *0.5f);
				if (sr > 255) sr = 255;
				sg = (int) GetGValue(gridColor) + (int) (GetGValue(gridColor) *0.5f);
				if (sg > 255) sg = 255;
				sb = (int) GetBValue(gridColor) + (int) (GetBValue(gridColor) *0.5f);
				if (sb > 255) sb = 255;
				HPEN sPen = CreatePen(PS_SOLID,1,RGB(sr,sg,sb));

				SelectObject(hdc,sPen);


				IMiniRect(hdc,transformedPoints[mouseHitVert],4);

				SelectObject(hdc,GetStockObject(WHITE_PEN));
				DeleteObject(sPen);
			}
		}


		/******************************************************************************************
		INORE THIS CODE THIS IS JUST A SPACE WHERE I CAN DISPLAY DRAW DEBUG INFO INTO THE DISPLAY
		WHEN TESTINF A FEATURE
		*******************************************************************************************/
 		if (gDebugLevel >=3 )
		{
			for (int cid = 0; cid < clusterList.Count(); cid++)


			{
				currentCluster = cid;
				if (currentCluster<clusterList.Count())
				{

					//					for (int j =0; j < clusterList[currentCluster]->boundsList.Count(); j++)
					{
						Point3 rect[2];
						rect[0].x = clusterList[currentCluster]->bounds.pmin.x+ clusterList[currentCluster]->offset.x;
						rect[0].y = clusterList[currentCluster]->bounds.pmin.y+clusterList[currentCluster]->offset.y;
						rect[0].z = 0.0f;

						rect[1].x = rect[0].x+clusterList[currentCluster]->bounds.pmax.x-clusterList[currentCluster]->bounds.pmin.x;
						rect[1].y = rect[0].y+clusterList[currentCluster]->bounds.pmax.y-clusterList[currentCluster]->bounds.pmin.y;
						rect[1].z = 0.0f;

						SelectObject(hdc,GetStockObject(BLACK_PEN));

						Point2 prect[2];
						for (int i = 0; i < 2; i++)
							prect[i] = UVWToScreen(rect[i],xzoom,yzoom,width,height);
						Rectangle(hdc, (int)prect[0].x,(int)prect[0].y,(int)prect[1].x,(int)prect[1].y);
					}
				}



			}	
		}

/*
 			for (int i = 0; i < TVMaps.v.Count(); i++)
			{
				if (i < dverts.GetSize())
				{
					if (dverts[i])
					{
						int x,y;
						x = transformedPoints[i].x;
						y = transformedPoints[i].y;

						Rectangle(hdc,
								x-5,y-5,
								x+5,y+5);	
					}
				}
			}
*/  	
/*
			Tab<IPoint2> axy;
			Tab<int> axyct;
			axy.SetCount(clusterInfo.Count());
			axyct.SetCount(clusterInfo.Count());
            for (int i = 0; i < clusterInfo.Count(); i++)
			{
				axy[i].x = 0;
				axy[i].y = 0;
				axyct[i] = 0;
			}

  			SelectObject(hdc,GetStockObject(WHITE_PEN));
         for (int i = 0; i < clusterInfo.Count(); i++)
			{
   				if (clusterInfo[i] && vsel[i])
				{
					int x,y;
  					Point3 p = TVMaps.v[i].p;

   	 				int ct = clusterInfo[i]->edgeVerts.Count();
					for (int j = 0; j < ct; j++)
					{
						
						{

							Point3 v = clusterInfo[i]->edgeVerts[j].tempPos ;
							Point3 p2 = p + v;

							Point3 tvPoint2 = UVWToScreen(p2,xzoom,yzoom,width,height);
							
							
							x  = (int) tvPoint2.x;
							y  = (int) tvPoint2.y;


//							if (vsel[i])

							{
		 						SelectObject(hdc,GetStockObject(WHITE_PEN));
  								MoveToEx(hdc,transformedPoints[i].x,transformedPoints[i].y, NULL);
								LineTo(hdc,x,y);

								TSTR vertStr;
								vertStr.printf("%d",clusterInfo[i]->edgeVerts[j].uvIndexN);
//								TextOut(hdc, x,y,vertStr,vertStr.Length());

							}


						}

					}

				}

			}
*/
/*
            for (int i = 0; i < clusterInfo.Count(); i++)
			{
				if (axyct[i] != 0)
				{
					int x = axy[i].x/axyct[i];
					int y = axy[i].y/axyct[i];
					Rectangle(hdc,
								x-2,y-2,
								x+2,y+2);	
				}
			}

*/

/*       for (int i = 0; i < dspringEdges.Count(); i++)
			{
				int a = dspringEdges[i].v1;
				int b = dspringEdges[i].v2;
				MoveToEx(hdc,transformedPoints[a].x,transformedPoints[a].y, NULL);
				LineTo(hdc,transformedPoints[b].x,transformedPoints[b].y);

				TSTR vertStr;
				float d = Length(TVMaps.v[a].p-TVMaps.v[b].p);
				vertStr.printf("%3.2f(%3.2f)",dspringEdges[i].dist,d);
				int x,y;
				x = (transformedPoints[a].x+transformedPoints[b].x)/2;
				y = (transformedPoints[a].y+transformedPoints[b].y)/2;
				TextOut(hdc, x,y,vertStr,vertStr.Length());

			}
*/


		/***  END DEBUG STUFF **********************************/

		if (mode == ID_FREEFORMMODE)
		{
			int count = 0;

			TransferSelectionStart();

			count = vsel.NumberSet();

			int vselCount = vsel.GetSize();
			freeFormBounds.Init();
			if (!inRotation)
				selCenter = Point3(0.0f,0.0f,0.0f);
			int setCount = 0;
			for (int i = 0; i < vselCount; i++)
			{
				if ((vsel[i]) && (!(TVMaps.v[i].flags&FLAG_DEAD)))
				{
					//get bounds
					Point3 p(0.0f,0.0f,0.0f);
					p[i1] = TVMaps.v[i].p[i1];
					p[i2] = TVMaps.v[i].p[i2];
					//					p.z = TVMaps.v[i].p.z;
					freeFormBounds += p;
				}
			}
			Point3 tempCenter;
			if (!inRotation)
				selCenter = freeFormBounds.Center();			
			else tempCenter = freeFormBounds.Center();			

			if (vsel.NumberSet() > 0)
			{	
				SelectObject(hdc,freeFormPen);
				SetBkMode( hdc, TRANSPARENT);
				//draw center

				Point2 prect[4];
				Point2 prectNoExpand[4];
				Point2 pivotPoint;
				prect[0] = UVWToScreen(selCenter,xzoom,yzoom,width,height);
				pivotPoint = prect[0];
				prect[1] = prect[0];
				prect[0].x -= 2;
				prect[0].y -= 2;
				prect[1].x += 2;
				prect[1].y += 2;
				if (!inRotation)
					RectNoFill(hdc, prect[0],prect[1]);

				//draw gizmo bounds
				prect[0] = UVWToScreen(freeFormBounds.pmin,xzoom,yzoom,width,height);
				prect[1] = UVWToScreen(freeFormBounds.pmax,xzoom,yzoom,width,height);
				float xexpand = 15.0f/xzoom;
				float yexpand = 15.0f/yzoom;

				BOOL expandedFFX = FALSE;
				BOOL expandedFFY = FALSE;

				prectNoExpand[0] = prect[0];
				prectNoExpand[1] = prect[1];
				prectNoExpand[2] = prect[2];
				prectNoExpand[3] = prect[3];

				if (!freeFormMode->dragging)
				{
					if ((prect[1].x-prect[0].x) < 30)
					{
						prect[1].x += 15;
						prect[0].x -= 15;
						//expand bounds
						freeFormBounds.pmax[i1] += xexpand;
						freeFormBounds.pmin[i1] -= xexpand;
						expandedFFX = TRUE;
					}
					else expandedFFX = FALSE;
					if ((prect[0].y-prect[1].y) < 30)
					{
						prect[1].y -= 15;
						prect[0].y += 15;
						freeFormBounds.pmax[i2] += yexpand;
						freeFormBounds.pmin[i2] -= yexpand;
						expandedFFY = TRUE;
					}
					else expandedFFY = FALSE;

				}


				if (!inRotation)
					RectNoFill(hdc, prect[0],prect[1]);
				//draw hit boxes
				Point3 frect[4];
				Point2 pmin, pmax;

				pmin = prect[0];
				pmax = prect[1];

				Point2 pminNoExpand, pmaxNoExpand;
				pminNoExpand = prectNoExpand[0];
				pmaxNoExpand = prectNoExpand[1];



				int corners[4];
				if ((i1 == 0) && (i2 == 1))
				{
					corners[0] = 0;
					corners[1] = 1;
					corners[2] = 2;
					corners[3] = 3;
				}
				else if ((i1 == 1) && (i2 == 2)) 
				{
					corners[0] = 1;//1,2,5,6
					corners[1] = 3;
					corners[2] = 5;
					corners[3] = 7;
				}
				else if ((i1 == 0) && (i2 == 2))
				{
					corners[0] = 0;
					corners[1] = 1;
					corners[2] = 4;
					corners[3] = 5;
				}

            for (int i = 0; i < 4; i++)
				{
					int index = corners[i];
					freeFormCorners[i] = freeFormBounds[index];
					//					prect[i] = UVWToScreen(freeFormCorners[i],xzoom,yzoom,width,height);
					if (i==0)
						prect[i] = pmin;
					else if (i==1)
					{
						prect[i].x = pmin.x;
						prect[i].y = pmax.y;
					}
					else if (i==2)
						prect[i] = pmax;
					else if (i==3)
					{
						prect[i].x = pmax.x;
						prect[i].y = pmin.y;
					}

					if (i==0)
						prectNoExpand[i] = pminNoExpand;
					else if (i==1)
					{
						prectNoExpand[i].x = pminNoExpand.x;
						prectNoExpand[i].y = pmaxNoExpand.y;
					}
					else if (i==2)
						prectNoExpand[i] = pmaxNoExpand;
					else if (i==3)
					{
						prectNoExpand[i].x = pmaxNoExpand.x;
						prectNoExpand[i].y = pminNoExpand.y;
					}

					freeFormCornersScreenSpace[i] = prectNoExpand[i];
					if (!inRotation) MiniRect(hdc,prect[i],3);
				}
				Point2 centerEdge;
				centerEdge = (prect[0] + prect[1]) *0.5f;
				freeFormEdges[0] = (freeFormCorners[0] + freeFormCorners[1]) *0.5f;
				freeFormEdgesScreenSpace[0] = centerEdge;
				if (!inRotation) MiniRect(hdc,centerEdge,3);

				centerEdge = (prect[1] + prect[2]) *0.5f;
				freeFormEdges[2] = (freeFormCorners[2] + freeFormCorners[3]) *0.5f;
				freeFormEdgesScreenSpace[2] = centerEdge;
				if (!inRotation) MiniRect(hdc,centerEdge,3);

				centerEdge = (prect[2] + prect[3]) *0.5f;
				freeFormEdges[3] = (freeFormCorners[0] + freeFormCorners[2]) *0.5f;
				freeFormEdgesScreenSpace[3] = centerEdge;
				if (!inRotation) MiniRect(hdc,centerEdge,3);

				centerEdge = (prect[3] + prect[0]) *0.5f;
				freeFormEdges[1] = (freeFormCorners[1] + freeFormCorners[3]) *0.5f;
				freeFormEdgesScreenSpace[1] = centerEdge;
				if (!inRotation) MiniRect(hdc,centerEdge,3);

				//draw pivot
				Point2 aPivot = UVWToScreen(freeFormPivotOffset,xzoom,yzoom,width,height);
				Point2 bPivot = UVWToScreen(Point3(0.0f,0.0f,0.0f),xzoom,yzoom,width,height);

				pivotPoint = pivotPoint + ( aPivot-bPivot);
				freeFormPivotScreenSpace = pivotPoint;

				MoveToEx(hdc,(int)pivotPoint.x-20, (int)pivotPoint.y, NULL);
				LineTo(hdc,(int)pivotPoint.x+20, (int)pivotPoint.y);
				MoveToEx(hdc,(int)pivotPoint.x, (int)pivotPoint.y-20, NULL);
				LineTo(hdc,(int)pivotPoint.x, (int)pivotPoint.y+20);
				SetBkMode( hdc, OPAQUE);
				if (inRotation)
				{
					Point2 a = UVWToScreen(tempCenter,xzoom,yzoom,width,height);
					Point2 b = UVWToScreen(origSelCenter,xzoom,yzoom,width,height);
					MoveToEx(hdc,(int)a.x, (int)a.y, NULL);
					LineTo(hdc,(int)pivotPoint.x, (int)pivotPoint.y);
					LineTo(hdc,(int)b.x, (int)b.y);

					TSTR rotAngleStr;
					rotAngleStr.printf("%3.2f",currentRotationAngle);
               DLTextOut(hdc, (int)pivotPoint.x,(int)pivotPoint.y,rotAngleStr);

				}

			}

			TransferSelectionEnd(FALSE,FALSE);


		}

		if ( (mode == ID_SKETCHMODE))
		{
			SelectObject(hdc,selPen);
			for (int i = 0; i < sketchMode->indexList.Count(); i++)
			{
				int index = sketchMode->indexList[i];

				ipt[0] = transformedPoints[index];

				Rectangle(hdc,	ipt[0].x-1,ipt[0].y-1,
					ipt[0].x+1,ipt[0].y+1);		

				if (sketchDisplayPoints)
				{
					TSTR vertStr;
					vertStr.printf("%d",i);
               DLTextOut(hdc, ipt[0].x+4,ipt[0].y-4,vertStr);
				}
			}
		}


		if (fnGetShowCounter())
		{

			SelectObject(hdc,GetStockObject(WHITE_PEN));
			TSTR vertStr;
			if (fnGetTVSubMode() == TVVERTMODE)			
				vertStr.printf("%d %s",(int)vsel.NumberSet(),GetString(IDS_VERTEXCOUNTER));
			if (fnGetTVSubMode() == TVEDGEMODE)			
				vertStr.printf("%d %s",(int)esel.NumberSet(),GetString(IDS_EDGECOUNTER));
			if (fnGetTVSubMode() == TVFACEMODE)			
				vertStr.printf("%d %s",(int)fsel.NumberSet(),GetString(IDS_FACECOUNTER));
         DLTextOut(hdc, 2,2,vertStr);

		}


		SelectObject(hdc,GetStockObject(BLACK_PEN));
		DeleteObject(selPen);
		DeleteObject(selThinPen);

		DeleteObject(yellowPen);		
		DeleteObject(darkyellowPen);		
		DeleteObject(darkgreenPen);		
		DeleteObject(greenPen);		
		DeleteObject(bluePen);		
		DeleteObject(frozenPen);		
		DeleteObject(unselPen);		
		DeleteObject(openEdgePen);		
		DeleteObject(handlePen);		
		DeleteObject(freeFormPen);		
		DeleteObject(sharedPen);		
		delete [] pt;
		delete [] ipt;
		paintSelectMode->first = TRUE;
		iBuf->Blit();		
	}	
	else
	{
		if (!fnGetPaintMode())
		{
			iBuf->Blit();	
		}

	}		

}

void UnwrapMod::InvalidateView()
{
	InvalidateTypeins();
	viewValid = FALSE;
	if (hView) {
		InvalidateRect(hView,NULL,TRUE);
	}
}

void UnwrapMod::SetMode(int m, BOOL updateMenuBar)
{
	BOOL invalidView = FALSE;
	if ((mode == ID_SKETCHMODE) && (m != ID_SKETCHMODE))
	{

		if (theHold.Holding())
			theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));		

		invalidView = TRUE;
		sketchSelMode = restoreSketchSelMode;
		sketchType = restoreSketchType;
		sketchDisplayPoints = restoreSketchDisplayPoints;
		sketchInteractiveMode = restoreSketchInteractiveMode;

		sketchCursorSize = restoreSketchCursorSize; 


	}

	switch (mode) {
		case ID_FREEFORMMODE:
			if (iFreeForm) iFreeForm->SetCheck(FALSE);
			oldMode = mode;
			invalidView = TRUE;
			break;
		case ID_TOOL_MOVE:
		case ID_UNWRAP_MOVE:
			if (iMove) iMove->SetCheck(FALSE);   
			oldMode = mode;
			break;
		case ID_UNWRAP_ROTATE:
		case ID_TOOL_ROTATE:  
			if (iRot) iRot->SetCheck(FALSE);    
			oldMode = mode;
			break;
		case ID_UNWRAP_SCALE:
		case ID_TOOL_SCALE:   
			if (iScale) iScale->SetCheck(FALSE);  
			oldMode = mode;
			break;
		case ID_UNWRAP_PAN:
		case ID_TOOL_PAN:     if (iPan) iPan->SetCheck(FALSE);    break;
		case ID_UNWRAP_ZOOM:
		case ID_TOOL_ZOOM:    
			if (iZoom) iZoom->SetCheck(FALSE);   
			break;
		case ID_UNWRAP_ZOOMREGION:
		case ID_TOOL_ZOOMREG: 
			if (iZoomReg) iZoomReg->SetCheck(FALSE);break;
		case ID_UNWRAP_WELD:
		case ID_TOOL_WELD:	  
			if (iWeld) iWeld->SetCheck(FALSE);
			oldMode = mode;
			break;
		case ID_TOOL_PELTSTRAIGHTEN:	
			peltData.peltDialog.SetStraightenButton(FALSE);
			//	peltData.peltDialog.SetStraightenMode(FALSE);
			oldMode = mode;
			break;
	}

	int prevMode = mode;
	mode = m;

	if (prevMode == ID_PAINTSELECTMODE)
		MoveScriptUI();

	switch (mode) {
			case ID_PAINTSELECTMODE:   
				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setPaintSelectMode"), 1, 0,mr_bool,TRUE);
				macroRecorder->EmitScript();
				mouseMan.SetMouseProc(paintSelectMode, LEFT_BUTTON);
				break;
			case ID_SKETCHMODE:   
				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.sketchNoParams"), 0, 0);
				macroRecorder->EmitScript();
				sketchMode->pointCount = 0;
				mouseMan.SetMouseProc(sketchMode, LEFT_BUTTON);
				break;
			case ID_FREEFORMMODE:   

				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap2.setFreeFormMode"), 1, 0,
					mr_bool,TRUE);
				macroRecorder->EmitScript();
				if (iFreeForm) iFreeForm->SetCheck(TRUE);  
				mouseMan.SetMouseProc(freeFormMode, LEFT_BUTTON);
				invalidView = TRUE;
				break;
			case ID_UNWRAP_MOVE:   
			case ID_TOOL_MOVE:   
				if (iMove)
				{
					if (iMove->GetCurFlyOff() == 0)
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.move"), 0, 0);
					else if (iMove->GetCurFlyOff() == 1)
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.moveh"), 0, 0);
					else if (iMove->GetCurFlyOff() == 2)
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.movev"), 0, 0);
					macroRecorder->EmitScript();
					iMove->SetCheck(TRUE);  

				}
				mouseMan.SetMouseProc(moveMode, LEFT_BUTTON);
				break;

			case ID_UNWRAP_ROTATE:   
			case ID_TOOL_ROTATE: 
				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.rotate"), 0, 0);
				macroRecorder->EmitScript();

				if (iRot) iRot->SetCheck(TRUE);   
				mouseMan.SetMouseProc(rotMode, LEFT_BUTTON);
				break;

			case ID_UNWRAP_SCALE:   
			case ID_TOOL_SCALE:  
				if (iScale)
				{
					if (iScale->GetCurFlyOff() == 0)
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.scale"), 0, 0);
					else if (iScale->GetCurFlyOff() == 1)
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.scaleh"), 0, 0);
					else if (iScale->GetCurFlyOff() == 2)
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.scalev"), 0, 0);
					macroRecorder->EmitScript();
					iScale->SetCheck(TRUE); 
				}

				mouseMan.SetMouseProc(scaleMode, LEFT_BUTTON);
				break;
			case ID_UNWRAP_WELD:
			case ID_TOOL_WELD:   
				if (fnGetTVSubMode() != TVFACEMODE)
				{

					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.weld"), 0, 0);
					macroRecorder->EmitScript();
					if (iWeld) iWeld->SetCheck(TRUE);  

					if (fnGetTVSubMode() == TVVERTMODE)
						vsel.ClearAll();			
					else if (fnGetTVSubMode() == TVEDGEMODE)
						esel.ClearAll();			

					if (ip) ip->RedrawViews(ip->GetTime());

					mouseMan.SetMouseProc(weldMode, LEFT_BUTTON);
				}
				break;

			case ID_TOOL_PELTSTRAIGHTEN:	  
				//	peltData.peltDialog.SetStraightenMode(TRUE);


				peltData.peltDialog.SetStraightenButton(TRUE);
				if (ip) ip->RedrawViews(ip->GetTime());
				mouseMan.SetMouseProc(peltStraightenMode, LEFT_BUTTON);


				break;



			case ID_UNWRAP_PAN:
			case ID_TOOL_PAN:    
				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.pan"), 0, 0);
				macroRecorder->EmitScript();

				if (iPan) iPan->SetCheck(TRUE);   
				mouseMan.SetMouseProc(panMode, LEFT_BUTTON);
				break;

			case ID_UNWRAP_ZOOM:
			case ID_TOOL_ZOOM:   
				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.zoom"), 0, 0);
				macroRecorder->EmitScript();

				if (iZoom) iZoom->SetCheck(TRUE);
				mouseMan.SetMouseProc(zoomMode, LEFT_BUTTON);
				break;



			case ID_UNWRAP_ZOOMREGION:
			case ID_TOOL_ZOOMREG:
				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.zoomRegion"), 0, 0);
				macroRecorder->EmitScript();

				if (iZoomReg) iZoomReg->SetCheck(TRUE);
				mouseMan.SetMouseProc(zoomRegMode, LEFT_BUTTON);
				break;
	}



	if (updateMenuBar)
	{
		if (hWnd)
		{
			IMenuBarContext* pContext = (IMenuBarContext*) GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
			if (pContext)
				pContext->UpdateWindowsMenu();
		}
	}
	if (invalidView) InvalidateView();

}

void UnwrapMod::RegisterClasses()
{
	if (!hToolImages) {
		HBITMAP hBitmap, hMask;	
		hToolImages = ImageList_Create(16, 15, TRUE, 4, 0);
		hBitmap = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_UNWRAP_TRANSFORM));
		hMask   = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_UNWRAP_TRANSFORM_MASK));
		ImageList_Add(hToolImages,hBitmap,hMask);
		DeleteObject(hBitmap);
		DeleteObject(hMask);
	}

	if (!hOptionImages) {
		HBITMAP hBitmap, hMask;	
		hOptionImages = ImageList_Create(16, 15, TRUE, 4, 0);
		hBitmap = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_UNWRAP_OPTION));
		hMask   = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_UNWRAP_OPTION_MASK));
		ImageList_Add(hOptionImages,hBitmap,hMask);
		DeleteObject(hBitmap);
		DeleteObject(hMask);
	}

	if (!hViewImages) {
		HBITMAP hBitmap, hMask;	
		hViewImages = ImageList_Create(16, 15, TRUE, 4, 0);
		hBitmap = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_UNWRAP_VIEW));
		hMask   = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_UNWRAP_VIEW_MASK));
		ImageList_Add(hViewImages,hBitmap,hMask);
		DeleteObject(hBitmap);
		DeleteObject(hMask);
	}

	if (!hVertexImages) {
		HBITMAP hBitmap, hMask;	
		hVertexImages = ImageList_Create(16, 15, TRUE, 4, 0);
		hBitmap = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_UNWRAP_VERT));
		hMask   = LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_UNWRAP_VERT_MASK));
		ImageList_Add(hVertexImages,hBitmap,hMask);
		DeleteObject(hBitmap);
		DeleteObject(hMask);
	}

	static BOOL registered = FALSE;
	if (!registered) {
		registered = TRUE;
		WNDCLASS  wc;
		wc.style         = 0;
		wc.hInstance     = hInstance;
		wc.hIcon         = NULL;
		wc.hCursor       = NULL;
		wc.hbrBackground = NULL; //(HBRUSH)GetStockObject(WHITE_BRUSH);	
		wc.lpszMenuName  = NULL;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.lpfnWndProc   = UnwrapViewProc;
		wc.lpszClassName = _T(GetString(IDS_PW_UNWRAPVIEW));
		RegisterClass(&wc);

	}

}

//static int lStart[12] = {0,1,3,2,4,5,7,6,0,1,2,3};
//static int lEnd[12]   = {1,3,2,0,5,7,6,4,4,5,6,7};
/*
static void DoBoxIcon(BOOL sel,float length, PolyLineProc& lp)
{
Point3 pt[3];

length *= 0.5f;
Box3 box;
box.pmin = Point3(-length,-length,-length);
box.pmax = Point3( length, length, length);

if (sel) //lp.SetLineColor(1.0f,1.0f,0.0f);
lp.SetLineColor(GetUIColor(COLOR_SEL_GIZMOS));
else //lp.SetLineColor(0.85f,0.5f,0.0f);		
lp.SetLineColor(GetUIColor(COLOR_GIZMOS));

for (int i=0; i<12; i++) {
pt[0] = box[lStart[i]];
pt[1] = box[lEnd[i]];
lp.proc(pt,2);
}
}

*/

/*************************************************************

Modified from Graphics Gem 3 Fast Liner intersection by Franklin Antonio

**************************************************************/


#define SAME_SIGNS(A, B) (((A>=0.0f) && (B>0.0f)) || ((A<0.0f) && (B<0.0f)))

#define  maxmin(x1, x2, min) (x1 >= x2 ? (min = x2, x1) : (min = x1, x2))

BOOL UnwrapMod::LineIntersect(Point3 p1, Point3 p2, Point3 q1, Point3 q2)
{


	float a, b, c, d, det;  /* parameter calculation variables */
	float max1, max2, min1, min2; /* bounding box check variables */

	/*  First make the bounding box test. */
	max1 = maxmin(p1.x, p2.x, min1);
	max2 = maxmin(q1.x, q2.x, min2);
	if((max1 < min2) || (min1 > max2)) return(FALSE); /* no intersection */
	max1 = maxmin(p1.y, p2.y, min1);
	max2 = maxmin(q1.y, q2.y, min2);
	if((max1 < min2) || (min1 > max2)) return(FALSE); /* no intersection */

	/* See if the endpoints of the second segment lie on the opposite
	sides of the first.  If not, return 0. */
	a = (q1.x - p1.x) * (p2.y - p1.y) -
		(q1.y - p1.y) * (p2.x - p1.x);
	b = (q2.x - p1.x) * (p2.y - p1.y) -
		(q2.y - p1.y) * (p2.x - p1.x);
	if(a!=0.0f && b!=0.0f && SAME_SIGNS(a, b)) return(FALSE);

	/* See if the endpoints of the first segment lie on the opposite
	sides of the second.  If not, return 0.  */
	c = (p1.x - q1.x) * (q2.y - q1.y) -
		(p1.y - q1.y) * (q2.x - q1.x);
	d = (p2.x - q1.x) * (q2.y - q1.y) -
		(p2.y - q1.y) * (q2.x - q1.x);
	if(c!=0.0f && d!=0.0f && SAME_SIGNS(c, d) ) return(FALSE);

	/* At this point each segment meets the line of the other. */
	det = a - b;
	if(det == 0.0f) return(FALSE); /* The segments are colinear.  Determining */
	return(TRUE);
}

int UnwrapMod::PolyIntersect(Point3 p1, int i1, int i2, BitArray *ignoredFaces)
{
	static int startFace = 0;
	int ct = 0;
	Point3 p2 = p1;
	float x = FLT_MIN;
	for (int i =0; i < TVMaps.v.Count(); i++)
	{
		if (!(TVMaps.v[i].flags & FLAG_DEAD))
		{
			float tx = TVMaps.v[i].p[i1];
			if (tx > x) x = tx;
		}
	}
	p2.x = x+10.0f;

	if (startFace >= TVMaps.f.Count()) startFace = 0;

	
	while (ct != TVMaps.f.Count())
	{
		int pcount = TVMaps.f[startFace]->count;
		int hit = 0;
		BOOL bail = FALSE;
		if (ignoredFaces)
		{
			if ((*ignoredFaces)[startFace])
				bail = TRUE;
		}
		if (IsFaceVisible(startFace) && (!bail))
		{
			int frozen = 0, hidden = 0;
			for (int j=0; j<pcount; j++) 
			{
				int index = TVMaps.f[startFace]->t[j];
				if (TVMaps.v[index].flags & FLAG_HIDDEN) hidden++;
				if (TVMaps.v[index].flags & FLAG_FROZEN) frozen++;
			}
			if ((frozen == pcount) || (hidden == pcount))
			{
			}	
			else if ( (objType == IS_PATCH) && (!(TVMaps.f[startFace]->flags & FLAG_CURVEDMAPPING)) && (TVMaps.f[startFace]->vecs))
			{
				Spline3D spl;
				spl.NewSpline();
				int i = startFace;
            for (int j=0; j<pcount; j++) 
				{
					Point3 in, p, out;
					int index = TVMaps.f[i]->t[j];
					p = GetPoint(currentTime,index);

					index = TVMaps.f[i]->vecs->handles[j*2];
					out = GetPoint(currentTime,index);
					if (j==0)
						index = TVMaps.f[i]->vecs->handles[pcount*2-1];
					else index = TVMaps.f[i]->vecs->handles[j*2-1];

					in = GetPoint(currentTime,index);

					SplineKnot kn(KTYPE_BEZIER_CORNER, LTYPE_CURVE, p, in, out);
					spl.AddKnot(kn);

					spl.SetClosed();
					spl.ComputeBezPoints();
				}
				//draw curves
				Point3 ptList[7*4];
				int ct = 0;
            for (int j=0; j<pcount; j++) 
				{
					int jNext = j+1;
					if (jNext >= pcount) jNext = 0;
					Point3 p;
					int index = TVMaps.f[i]->t[j];
					if (j==0)
						ptList[ct++] = GetPoint(currentTime,index);

					for (int iu = 1; iu <= 5; iu++)
					{
						float u = (float) iu/5.f;
						ptList[ct++] = spl.InterpBezier3D(j, u);

					}



				}
            for (int j=0; j < ct; j++) 
				{
					int index;
					if (j == (ct-1))
						index = 0;
					else index = j+1;
					Point3 a(0.0f,0.0f,0.0f),b(0.0f,0.0f,0.0f);
					a.x = ptList[j][i1];
					a.y = ptList[j][i2];

					b.x = ptList[index][i1];
					b.y = ptList[index][i2];

					if (LineIntersect(p1, p2, a,b)) hit++;
					//					if (LineIntersect(p1, p2, ptList[j],ptList[index])) hit++;
				}


			}
			else
			{
            for (int i = 0; i < pcount; i++)
				{
					int faceIndexA;  
					faceIndexA = TVMaps.f[startFace]->t[i];
					int faceIndexB;
					if (i == (pcount-1))
						faceIndexB  = TVMaps.f[startFace]->t[0];
					else faceIndexB  = TVMaps.f[startFace]->t[i+1];
					Point3 a(0.0f,0.0f,0.0f),b(0.0f,0.0f,0.0f);
					a.x = TVMaps.v[faceIndexA].p[i1];
					a.y = TVMaps.v[faceIndexA].p[i2];
					b.x = TVMaps.v[faceIndexB].p[i1];
					b.y = TVMaps.v[faceIndexB].p[i2];
					if (LineIntersect(p1, p2, a,b)) 
						hit++;
				}
			}


			if ((hit%2) == 1) 
				return startFace;
		}
		startFace++;
		if (startFace >= TVMaps.f.Count()) startFace = 0;
		ct++;
	}
	return -1;
}

BOOL UnwrapMod::HitTest(Rect rect,Tab<int> &hits,BOOL selOnly,BOOL circleMode)
{

	if( (fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
		return FALSE;

	if( (fnGetMapMode() == PELTMAP) && (peltData.peltDialog.hWnd == NULL))
		return FALSE;

	Point2 pt;
	float xzoom, yzoom;
	int width,height;
	TimeValue t = ip->GetTime();
	ComputeZooms(hView,xzoom,yzoom,width,height);


	Point2 center = UVWToScreen(Point3(0.0f,0.0f,0.0f),xzoom,yzoom,width,height);
	Point3 p1(10000.f,10000.f,10000.f), p2(-10000.f,-10000.f,-10000.f);

	Rect smallRect = rect;

	if (circleMode)
	{

	}
	else
	{
		if (fnGetTVSubMode() == TVVERTMODE)
		{
			if ( (abs(rect.left-rect.right) <= 4) && (abs(rect.bottom-rect.top) <= 4) )
			{
				rect.left -= hitSize;
				rect.right += hitSize;
				rect.top -= hitSize;
				rect.bottom += hitSize;

				smallRect.left -= 1;
				smallRect.right += 1;
				smallRect.top -= 1;
				smallRect.bottom += 1;
			}
		}
	}

   Rect rectFF = rect;
   if ( (abs(rect.left-rect.right) <= 4) && (abs(rect.bottom-rect.top) <= 4) )
   {
      rectFF.left -= hitSize;
      rectFF.right += hitSize;
      rectFF.top -= hitSize;
      rectFF.bottom += hitSize;
   }


	int i1,i2;
	GetUVWIndices(i1,i2);

   p1[i1] = ((float)rectFF.left-center.x)/xzoom;
   p2[i1] = ((float)rectFF.right-center.x)/xzoom;
   // p1.z = 10.f;
   // p2.z = -10.f;

   p1[i2] = -((float)rectFF.top-center.y)/yzoom;
   p2[i2] = -((float)rectFF.bottom-center.y)/yzoom;

   Box3 boundsFF;
   boundsFF.Init();
   boundsFF += p1;
   boundsFF += p2;

	p1[i1] = ((float)rect.left-center.x)/xzoom;
	p2[i1] = ((float)rect.right-center.x)/xzoom;
	//	p1.z = 10.f;
	//	p2.z = -10.f;

	p1[i2] = -((float)rect.top-center.y)/yzoom;
	p2[i2] = -((float)rect.bottom-center.y)/yzoom;

	Box3 bounds;
	bounds.Init();
	bounds += p1;
	bounds += p2;




	Point3 smallp1(10000.f,10000.f,10000.f), smallp2(-10000.f,-10000.f,-10000.f);
	smallp1[i1] = ((float)smallRect.left-center.x)/xzoom;
	smallp2[i1] = ((float)smallRect.right-center.x)/xzoom;
	//	p1.z = 10.f;
	//	p2.z = -10.f;

	smallp1[i2] = -((float)smallRect.top-center.y)/yzoom;
	smallp2[i2] = -((float)smallRect.bottom-center.y)/yzoom;

	Box3 smallBounds;
	smallBounds.Init();
	smallBounds += smallp1;
	smallBounds += smallp2;



	//new
//	mouseHitVert = -1;
	if (gridSnap)
	{
		for (int i=0; i<TVMaps.v.Count(); i++) {
			if (selOnly && !vsel[i]) continue;
			if (TVMaps.v[i].flags & FLAG_HIDDEN) continue;
			if (TVMaps.v[i].flags & FLAG_FROZEN) continue;
			if (!IsVertVisible(i)) continue;
			if (!usedVertices[i]) continue;

			//			pt = UVWToScreen(GetPoint(t,i),xzoom,yzoom,width,height);
			//			IPoint2 ipt(int(pt.x),int(pt.y));
			//			if (rect.Contains(ipt)) {
			if (bounds.Contains(GetPoint(t,i)))
			{
				mouseHitVert = i;
				i = TVMaps.v.Count();
			}
		}
	}

	if (fnGetTVSubMode() == TVVERTMODE)
	{
		/*
		if ( (abs(rect.left-rect.right) <= 4) && (abs(rect.bottom-rect.top) <= 4) )
		{
		rect.left -= hitSize;
		rect.right += hitSize;
		rect.top -= hitSize;
		rect.bottom += hitSize;
		}	
		*/
		for (int i=0; i<TVMaps.v.Count(); i++) {
			if (selOnly && !vsel[i]) continue;
			if (TVMaps.v[i].flags & FLAG_HIDDEN) continue;
			if (TVMaps.v[i].flags & FLAG_FROZEN) continue;
			if (!IsVertVisible(i)) continue;
			if (!usedVertices[i]) continue;

			//			pt = UVWToScreen(GetPoint(t,i),xzoom,yzoom,width,height);
			//			IPoint2 ipt(int(pt.x),int(pt.y));
			//			if (rect.Contains(ipt)) {
			if (bounds.Contains(GetPoint(t,i)))
			{
				hits.Append(1,&i,10);
			}
			//new
/*
			if (gridSnap)
			{
				if (hits.Count() == 1)
					mouseHitVert = hits[0];
				else 
				{

				}

			}
*/
		}
	}
	else if (fnGetTVSubMode() == TVEDGEMODE)
	{
		//check if single click
		if ( (abs(rect.left-rect.right) <= 4) && (abs(rect.bottom-rect.top) <= 4) )
		{
			Point3 center = bounds.Center();
			float threshold = (1.0f/xzoom) * 2.0f;
			int edgeIndex = TVMaps.EdgeIntersect(center,threshold,i1,i2);

			if (edgeIndex >=0)
			{
				BitArray sel;
				sel.SetSize(TVMaps.v.Count());
				sel.ClearAll();

				BitArray tempVSel;
				if (selOnly)
					GetVertSelFromEdge(tempVSel);

				int index1 = TVMaps.ePtrList[edgeIndex]->b;
				if (selOnly && !tempVSel[index1]) edgeIndex = -1;
				if (TVMaps.v[index1].flags & FLAG_HIDDEN) edgeIndex = -1;
				if (TVMaps.v[index1].flags & FLAG_FROZEN) edgeIndex = -1;
				if (!IsVertVisible(index1)) edgeIndex = -1;

				if (edgeIndex >=0)
				{
					int index2 = TVMaps.ePtrList[edgeIndex]->b;

					if (selOnly && !tempVSel[index2]) edgeIndex = -1;
					if (TVMaps.v[index2].flags & FLAG_HIDDEN) edgeIndex = -1;
					if (TVMaps.v[index2].flags & FLAG_FROZEN) edgeIndex = -1;
					if (!IsVertVisible(index2)) edgeIndex = -1;
				}
			}

			if (edgeIndex >= 0)
			{
				if (selOnly && !esel[edgeIndex]) 
				{
				}
				else
				{
					hits.Append(1,&edgeIndex,10);
				}
			}

			if (gridSnap)
			{
				float closestDist = 0.0f;
				Point3 tempCenter = center;
				//				tempCenter.z = 0.0f;
				if (mode == ID_FREEFORMMODE)
				{

					for (int j =0; j < esel.GetSize(); j++)
					{
						if (esel[j])
						{
							int index = j;
							int a = TVMaps.ePtrList[index]->a;
							//							Point3 p = GetPoint(t,a);
							//							p.z = 0.0f;
							Point3 p(0.0f,0.0f,0.0f);
							Point3 tp = GetPoint(t,a);
							p[i1] = tp[i1];
							p[i2] = tp[i2];

							float dist = LengthSquared(tempCenter-p);
							if ((mouseHitVert == -1) || (dist < closestDist))
							{
								mouseHitVert = a;
								closestDist = dist;
							}
							a = TVMaps.ePtrList[index]->b;
							//							p = GetPoint(t,a);
							//							p.z = 0.0f;
							p = Point3(0.0f,0.0f,0.0f);
							tp = GetPoint(t,j);
							p[i1] = tp[i1];
							p[i2] = tp[i2];

							dist = LengthSquared(tempCenter-p);

							if ((mouseHitVert == -1) || (dist < closestDist))
							{
								mouseHitVert = a;
								closestDist = dist;
							}
						}
					}	
				}
				else
				{
					for (int j =0; j < hits.Count(); j++)
					{
						int index = hits[j];
						int a = TVMaps.ePtrList[index]->a;
						//						Point3 p = GetPoint(t,a);
						//						p.z = 0.0f;
						Point3 p(0.0f,0.0f,0.0f);
						Point3 tp = GetPoint(t,a);
						p[i1] = tp[i1];
						p[i2] = tp[i2];

						float dist = LengthSquared(tempCenter-p);
						if ((mouseHitVert == -1) || (dist < closestDist))
						{
							mouseHitVert = a;
							closestDist = dist;
						}
						a = TVMaps.ePtrList[index]->b;
						//						p = GetPoint(t,a);
						//						p.z = 0.0f;
						p = Point3(0.0f,0.0f,0.0f);
						tp = GetPoint(t,a);
						p[i1] = tp[i1];
						p[i2] = tp[i2];


						dist = LengthSquared(tempCenter-p);

						if ((mouseHitVert == -1) || (dist < closestDist))
						{
							mouseHitVert = a;
							closestDist = dist;
						}
					}	

				}	
			}

			//loop through all the faces and see if the point is contained by it
		}	
		//else it is a drag rect
		else
		{
			BitArray sel;
			sel.SetSize(TVMaps.v.Count());
			sel.ClearAll();

			BitArray tempVSel;
			if (selOnly)
				GetVertSelFromEdge(tempVSel);
			Tab<Point2> screenSpaceList; 
			screenSpaceList.SetCount(TVMaps.v.Count()); 
			for (int i = 0; i < screenSpaceList.Count(); i++)
				screenSpaceList[i] = UVWToScreen(GetPoint(t,i),xzoom,yzoom,width,height);
			/*
			for (int i=0; i<TVMaps.v.Count(); i++) {
			if (selOnly && !tempVSel[i]) continue;
			if (TVMaps.v[i].flags & FLAG_HIDDEN) continue;
			if (TVMaps.v[i].flags & FLAG_FROZEN) continue;
			if (!IsVertVisible(i)) continue;


			if (bounds.Contains(GetPoint(t,i))) 
			{
			sel.Set(i);
			}
			}
			*/
			Point3 rectPoints[4];

			rectPoints[0].x = rect.left; 
			rectPoints[0].y = rect.top; 
			rectPoints[0].z = 0.0f;

			rectPoints[1].x = rect.right; 
			rectPoints[1].y = rect.top; 
			rectPoints[1].z = 0.0f;

			rectPoints[2].x = rect.left; 
			rectPoints[2].y = rect.bottom; 
			rectPoints[2].z = 0.0f;

			rectPoints[3].x = rect.right; 
			rectPoints[3].y = rect.bottom; 
			rectPoints[3].z = 0.0f;

			BOOL cross = GetCOREInterface()->GetCrossing();

         for (int i=0; i<TVMaps.ePtrList.Count(); i++) 
			{
				int index1 = TVMaps.ePtrList[i]->a;

				if (selOnly && !tempVSel[index1]) continue;
				if (TVMaps.v[index1].flags & FLAG_HIDDEN) continue;
				if (TVMaps.v[index1].flags & FLAG_FROZEN) continue;
				if (!IsVertVisible(index1)) continue;

				int index2 = TVMaps.ePtrList[i]->b;

				if (selOnly && !tempVSel[index2]) continue;
				if (TVMaps.v[index2].flags & FLAG_HIDDEN) continue;
				if (TVMaps.v[index2].flags & FLAG_FROZEN) continue;
				if (!IsVertVisible(index2)) continue;

				Point2 a = screenSpaceList[index1];
				Point2 b = screenSpaceList[index2];

				BOOL hitEdge = FALSE;
				if ( (a.x >= rect.left) && (a.x <= rect.right) && 
					(a.y <= rect.bottom) && (a.y >= rect.top) && 
					(b.x >= rect.left) && (b.x <= rect.right) && 
					(b.y <= rect.bottom) && (b.y >= rect.top) )
				{
					hits.Append(1,&i,10);
					hitEdge = TRUE;
				}

				Point3 ap,bp;
				ap.x = a.x;
				ap.y = a.y;
				ap.z = 0.0f;
				bp.x = b.x;
				bp.y = b.y;
				bp.z = 0.0f;

				if ((cross) && (!hitEdge))
				{
					if (LineIntersect(ap, bp, rectPoints[0],rectPoints[1]))
					{
						hits.Append(1,&i,10);
					}
					else if (LineIntersect(ap, bp, rectPoints[2],rectPoints[3]))
					{
						hits.Append(1,&i,10);
					}
					else if (LineIntersect(ap, bp, rectPoints[0],rectPoints[2]))
					{
						hits.Append(1,&i,10);
					}
					else if (LineIntersect(ap, bp, rectPoints[1],rectPoints[3]))
					{
						hits.Append(1,&i,10);
					}
				}


			}

			/*			
			int ct = sel.GetSize();
         for (int i=0; i<TVMaps.ePtrList.Count(); i++) 
			{
			int index1 = TVMaps.ePtrList[i]->a;
			int index2 = TVMaps.ePtrList[i]->b;
			int total = 0;
			if ((index1 >= 0) && (index1 < ct) && (sel[index1]))
			total++;
			if ((index2 >= 0) && (index2 < ct) && (sel[index2]))
			total++;

			if ((cross) && (total >0))
			hits.Append(1,&i,10);
			else if ((!cross) && (total == 2))
			hits.Append(1,&i,10);

			}
			*/

		}
	}
	else if (fnGetTVSubMode() == TVFACEMODE)
	{
		//check if single click
		if ( (abs(rect.left-rect.right) <= 4) && (abs(rect.bottom-rect.top) <= 4) )
		{
			Point3 center(0.0f,0.0f,0.0f);

 			Point3 tcent = bounds.Center();
			center.x = tcent[i1];
			center.y = tcent[i2];

			int faceIndex = -1;
			if (selOnly)
			{
				BitArray ignoreFaces;
				ignoreFaces = ~fsel;
				faceIndex = PolyIntersect(center,i1,i2,&ignoreFaces);
			}
			else faceIndex = PolyIntersect(center,i1,i2);

			if (faceIndex >= 0)
			{
				if (selOnly && !fsel[faceIndex]) 
				{
				}
				else hits.Append(1,&faceIndex,10);
			}

			//loop through all the faces and see if the point is contained by it
			if (gridSnap)
			{
				float closestDist = 0.0f;
				Point3 tempCenter = center;
				//				tempCenter.z = 0.0f;
				for (int j =0; j < hits.Count(); j++)
				{
					int index = hits[j];
					for (int k = 0; k < TVMaps.f[index]->count; k++)
					{
						int a = TVMaps.f[index]->t[k];
						//						Point3 p = GetPoint(t,a);
						//						p.z = 0.0f;
						Point3 p(0.0f,0.0f,0.0f);
						Point3 tp = GetPoint(t,a);
						p[i1] = tp[i1];
						p[i2] = tp[i2];

						float dist = LengthSquared(tempCenter-p);
						if ((mouseHitVert == -1) || (dist < closestDist))
						{
							mouseHitVert = a;
							closestDist = dist;
						}

					}
				}
			}
		}	
		//else it is a drag rect
		else
		{
			BitArray sel;
			sel.SetSize(TVMaps.v.Count());
			sel.ClearAll();

			BitArray tempVSel;
			if (selOnly)
				GetVertSelFromFace(tempVSel);

			Tab<Point2> screenSpaceList; 
			screenSpaceList.SetCount(TVMaps.v.Count()); 
			for (int i = 0; i < screenSpaceList.Count(); i++)
				screenSpaceList[i] = UVWToScreen(GetPoint(t,i),xzoom,yzoom,width,height);


			Point3 rectPoints[4];

			rectPoints[0].x = rect.left; 
			rectPoints[0].y = rect.top; 
			rectPoints[0].z = 0.0f;

			rectPoints[1].x = rect.right; 
			rectPoints[1].y = rect.top; 
			rectPoints[1].z = 0.0f;

			rectPoints[2].x = rect.left; 
			rectPoints[2].y = rect.bottom; 
			rectPoints[2].z = 0.0f;

			rectPoints[3].x = rect.right; 
			rectPoints[3].y = rect.bottom; 
			rectPoints[3].z = 0.0f;

			BOOL cross = GetCOREInterface()->GetCrossing();


         for (int i=0; i<TVMaps.f.Count(); i++) 
			{
				int faceIndex = i;
				int total = 0;
				int pcount = TVMaps.f[faceIndex]->count;
				BOOL hitEdge = FALSE;
				BOOL frozen = FALSE;
				for (int k = 0; k <  pcount; k++)
				{
					int index = TVMaps.f[faceIndex]->t[k];
					if (TVMaps.v[index].flags & FLAG_FROZEN) frozen = TRUE;
					if (!IsVertVisible(index)) frozen = TRUE;
					Point2 a = screenSpaceList[index];			
					if ( (a.x >= rect.left) && (a.x <= rect.right) && 
						(a.y <= rect.bottom) && (a.y >= rect.top)  )
						total++;
				}
				if (frozen) 
				{
					total=0;
				}
				else if (total == pcount)
				{
					hits.Append(1,&i,10);
				}
				else if (cross)
				{
					for (int k = 0; k <  pcount; k++)
					{
						int index = TVMaps.f[faceIndex]->t[k];
						int index2;
						if (k == (pcount -1))
							index2 = TVMaps.f[faceIndex]->t[0];
						else index2 = TVMaps.f[faceIndex]->t[k+1];

						Point2 a = screenSpaceList[index];			
						Point2 b = screenSpaceList[index2];			

						Point3 ap,bp;
						ap.x = a.x;
						ap.y = a.y;
						ap.z = 0.0f;
						bp.x = b.x;
						bp.y = b.y;
						bp.z = 0.0f;
						if (LineIntersect(ap, bp, rectPoints[0],rectPoints[1]))
						{
							hits.Append(1,&i,10);
							k =  pcount;
						}
						else if (LineIntersect(ap, bp, rectPoints[2],rectPoints[3]))
						{
							hits.Append(1,&i,10);
							k =  pcount;
						}
						else if (LineIntersect(ap, bp, rectPoints[0],rectPoints[2]))
						{
							hits.Append(1,&i,10);
							k =  pcount;
						}
						else if (LineIntersect(ap, bp, rectPoints[1],rectPoints[3]))
						{
							hits.Append(1,&i,10);
							k =  pcount;
						}
					}


				}

			}


			/*
			for (int i=0; i<TVMaps.v.Count(); i++) {
			if (selOnly && !tempVSel[i]) continue;
			if (TVMaps.v[i].flags & FLAG_HIDDEN) continue;
			if (TVMaps.v[i].flags & FLAG_FROZEN) continue;
			if (!IsVertVisible(i)) continue;

			//				pt = UVWToScreen(GetPoint(t,i),xzoom,yzoom,width,height);
			//				IPoint2 ipt(int(pt.x),int(pt.y));
			//				if (rect.Contains(ipt)) {
			if (bounds.Contains(GetPoint(t,i))) 
			{
			sel.Set(i);
			}
			}
			BOOL cross = GetCOREInterface()->GetCrossing();
			int ct = sel.GetSize();
         for (int i=0; i<TVMaps.f.Count(); i++) 
			{
			int faceIndex = i;
			int total = 0;
			int pcount = TVMaps.f[faceIndex]->count;
			for (int k = 0; k <  pcount; k++)
			{
			int index = TVMaps.f[faceIndex]->t[k];
			if ((index >= 0) && (index < ct) && (sel[index]))
			total++;
			}

			if ((cross) && (total >0))
			hits.Append(1,&i,10);
			else if ((!cross) && (total == pcount ))
			hits.Append(1,&i,10);

			}
			*/

		}
	}



	BOOL bail = FALSE;
	if (fnGetTVSubMode() == TVVERTMODE)
	{
		if (vsel.NumberSet() == 0 ) bail = TRUE;
		freeFormSubMode = ID_TOOL_SELECT;
	}
	else if (fnGetTVSubMode() == TVEDGEMODE)
	{
		if (esel.NumberSet() == 0 ) bail = TRUE;
		freeFormSubMode = ID_TOOL_SELECT;
	}
	else if (fnGetTVSubMode() == TVFACEMODE)
	{
		if (fsel.NumberSet() == 0 ) bail = TRUE;
		freeFormSubMode = ID_TOOL_SELECT;
	}

	if ( (mode == ID_FREEFORMMODE) && (!bail))
	{
		//check if cursor is inside bounds
		Point3 center = bounds.Center();
		int move = -99;
		Point3 hold1, hold2;
		hold1 = p1;
		hold2 = p2;
		p1 = Point3(0.0f,0.0f,0.0f);
		p2 = Point3(0.0f,0.0f,0.0f);

		p1[i1] = hold1[i1];
		p1[i2] = hold1[i2];

		p2[i1] = hold2[i1];
		p2[i2] = hold2[i2];

		Point2 min = UVWToScreen(freeFormCorners[0],xzoom,yzoom,width,height);
		Point2 max = UVWToScreen(freeFormCorners[3],xzoom,yzoom,width,height);

      bounds = boundsFF;

		if ((fabs(max.x - min.x) < 40.0f) || (fabs(min.y - max.y) < 40.0f))
		{
			bounds = smallBounds;
		}

		//		p1.z = 0.0f;
		//		p2.z = 0.0f;
		//		bounds.Scale(2.0f);

		BOOL forceMove = FALSE;
		if (fnGetTVSubMode() == TVVERTMODE)
		{
			if (vsel.NumberSet() <= 1 ) forceMove = TRUE;
		}
		else if (fnGetTVSubMode() == TVEDGEMODE)
		{
			//			if (esel.NumberSet() < 1 ) forceMove = TRUE;
		}
		else if (fnGetTVSubMode() == TVFACEMODE)
		{
			//			if (esel.NumberSet() < 1 ) forceMove = TRUE;
		}
		if ((!forceMove) && (smallBounds.Contains(selCenter+freeFormPivotOffset)) )
		{
			freeFormSubMode = ID_TOOL_MOVEPIVOT;
			return TRUE;
		}
		else if ( (bounds.Contains(freeFormCorners[0]) || bounds.Contains(freeFormCorners[1]) ||
			bounds.Contains(freeFormCorners[2]) || bounds.Contains(freeFormCorners[3]) ))
		{
			freeFormSubMode = ID_TOOL_SCALE;
			if (bounds.Contains(freeFormCorners[0]))
			{

				scaleCorner = 0;
				scaleCornerOpposite = 2;
			}
			else if (bounds.Contains(freeFormCorners[1]))
			{

				scaleCorner = 3;
				scaleCornerOpposite = 1;
			}
			else if (bounds.Contains(freeFormCorners[2]))
			{

				scaleCorner = 1;
				scaleCornerOpposite = 3;
			}
			else if (bounds.Contains(freeFormCorners[3]))
			{

				scaleCorner = 2;
				scaleCornerOpposite = 0;
			}

			return TRUE;
		}
		BOOL useMoveMode = FALSE;

		if (allowSelectionInsideGizmo)
		{
			if ((hits.Count() == 0) || forceMove)
				useMoveMode = TRUE;
			else useMoveMode = FALSE;
		}
		else
		{

			useMoveMode = TRUE;

		}

		if ( (bounds.Contains(freeFormEdges[0]) || bounds.Contains(freeFormEdges[1]) ||
			bounds.Contains(freeFormEdges[2]) || bounds.Contains(freeFormEdges[3]) ))
		{
			freeFormSubMode = ID_TOOL_ROTATE;


			return TRUE;
		}
		else if ((freeFormBounds.Contains(p1) && freeFormBounds.Contains(p2)) && (useMoveMode) )
		{
			freeFormSubMode = ID_TOOL_MOVE;
			//DebugPrint("Movemode\n");
			return TRUE;
		}
		else 
		{
			freeFormSubMode = ID_TOOL_SELECT;
			//DebugPrint("Selmode\n");

		}


	}

	if ((mode == ID_TOOL_WELD) &&  (hits.Count()>0))
	{
		if (fnGetTVSubMode() == TVFACEMODE) 
			hits.SetCount(0);
		else hits.SetCount(1);
	}
	return hits.Count();
}

void UnwrapMod::InvalidateTypeins()
{
	typeInsValid = FALSE;	
}

void UnwrapMod::SetupTypeins()
{
	if (typeInsValid) return;
	typeInsValid = TRUE;

	Point3 uv(0,0,0);
	BOOL found = FALSE;
	BOOL u = TRUE, v = TRUE, w = TRUE;

	TransferSelectionStart();

	for (int i=0; i<TVMaps.v.Count(); i++) {
		if (!vsel[i]) continue;

		if (found) {
			if (uv.x!=TVMaps.v[i].p.x) {
				u = FALSE;				
			}
			if (uv.y!=TVMaps.v[i].p.y) {
				v = FALSE;				
			}
			if (uv.z!=TVMaps.v[i].p.z) {
				w = FALSE;				
			}			
		} else {
			uv = TVMaps.v[i].p;
			found = TRUE;
		}
	}
	TransferSelectionEnd(FALSE,FALSE);

	if (!found) {
		if (iU) iU->Disable();
		if (iV) iV->Disable();
		if (iW) iW->Disable();
	} else {
		if (absoluteTypeIn)
		{
			if (iU) iU->Enable();
			if (iV) iV->Enable();
			if (iW) iW->Enable();
			if ((u) && (iU)) {
				iU->SetIndeterminate(FALSE);
				iU->SetValue(uv.x,FALSE);
			} else  if (iU) {
				iU->SetIndeterminate(TRUE);
			}

			if ((v) && (iV)){
				iV->SetIndeterminate(FALSE);
				iV->SetValue(uv.y,FALSE);
			} else if (iV) {
				iV->SetIndeterminate(TRUE);
			}

			if ((w) && (iW)){
				iW->SetIndeterminate(FALSE);
				iW->SetValue(uv.z,FALSE);
			} else  if (iW) {
				iW->SetIndeterminate(TRUE);
			}
		}

		else
		{
			iU->SetIndeterminate(FALSE);
			iU->SetValue(0.0f,FALSE);

			iV->SetIndeterminate(FALSE);
			iV->SetValue(0.0f,FALSE);

			iW->SetIndeterminate(FALSE);
			iW->SetValue(0.0f,FALSE);

		}
	}
}

void UnwrapMod::Select(Tab<int> &hits,BOOL toggle,BOOL subtract,BOOL all)
{
	if ( FALSE/*(ip && (ip->GetSubObjectLevel() == 3) )*/)
	{
		GizmoSelected = FALSE;
		for (int i=0; i<hits.Count(); i++) 
		{
			if (hits[i]!= 0)
				GizmoSelected = TRUE;
		}

	}
	else
	{

		if (!fnGetPaintMode()) HoldSelection();

		IsSelectedSetup();
		if (fnGetTVSubMode() == TVVERTMODE)
		{
			for (int i=0; i<hits.Count(); i++) 
			{
				if ( (IsVertVisible(hits[i])) &&  (!(TVMaps.v[hits[i]].flags & FLAG_FROZEN)) )
				{

					if (toggle) vsel.Set(hits[i],!vsel[hits[i]]);
					else if (subtract) vsel.Set(hits[i],FALSE);
					else vsel.Set(hits[i],TRUE);
					if (!all) break;
				}
			}	
			if ((!subtract) && (mode != ID_TOOL_WELD) )
				SelectHandles(0);
		}
		else if (fnGetTVSubMode() == TVEDGEMODE)
		{
			for (int i=0; i<hits.Count(); i++) 
			{

				if (toggle) esel.Set(hits[i],!esel[hits[i]]);
				else if (subtract) esel.Set(hits[i],FALSE);
				else esel.Set(hits[i],TRUE);
				if (!all) break;
			}	

		}
		else if (fnGetTVSubMode() == TVFACEMODE)
		{
			for (int i=0; i<hits.Count(); i++) 
			{
				if ( (IsFaceVisible(hits[i])) && (!(TVMaps.f[hits[i]]->flags & FLAG_FROZEN)) )
				{

					if (toggle) fsel.Set(hits[i],!fsel[hits[i]]);
					else if (subtract) fsel.Set(hits[i],FALSE);
					else fsel.Set(hits[i],TRUE);
					if (!all) break;
				}
			}	
		}


		if ( (tvElementMode) && (mode != ID_TOOL_WELD)) 
		{
			if (subtract)
				SelectElement(FALSE);
			else SelectElement(TRUE);
		}
		if ( (uvEdgeMode) && (fnGetTVSubMode() == TVEDGEMODE))
		{
			if (!fnGetPaintMode())
				SelectUVEdge();
		}
		if ( (openEdgeMode) && (fnGetTVSubMode() == TVEDGEMODE))
		{
			if (!fnGetPaintMode()) 
				SelectOpenEdge();
		}
		if ( (polyMode) && (fnGetTVSubMode() == TVFACEMODE))
		{
			if (subtract)
				fnPolySelect2(FALSE);
			else fnPolySelect();
		}
		RebuildDistCache();
	}
	if (!fnGetPaintMode())
	{
		if (fnGetTVSubMode() == TVVERTMODE)
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.selectVertices"), 1, 0,
			mr_bitarray,&vsel);
		else if (fnGetTVSubMode() == TVEDGEMODE)
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.selectEdges"), 1, 0,
			mr_bitarray,&esel);
		else if (fnGetTVSubMode() == TVFACEMODE)
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.selectFaces"), 1, 0,
			mr_bitarray,&fsel);
		macroRecorder->EmitScript();
	}

	if ((mode == ID_FREEFORMMODE) && (fnGetResetPivotOnSel()))
	{
		freeFormPivotOffset.x = 0.0f;
		freeFormPivotOffset.y = 0.0f;
		freeFormPivotOffset.z = 0.0f;
	}

	if (fnGetSyncSelectionMode() && (!fnGetPaintMode())) fnSyncGeomSelection();
}

void UnwrapMod::ClearSelect()
{
	if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP)  || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
		return;

	if ((fnGetMapMode() == PELTMAP) && (peltData.peltDialog.hWnd==NULL))
		return;

	HoldSelection();
	if (fnGetTVSubMode() == TVVERTMODE)
	{
		vsel.ClearAll();	
		for (int i=0; i<TVMaps.v.Count(); i++) 
		{
			if (!(TVMaps.v[i].flags & FLAG_WEIGHTMODIFIED))
				TVMaps.v[i].influence = 0.0f;
		}
	}
	else if (fnGetTVSubMode() == TVEDGEMODE)
	{
		esel.ClearAll();
	}
	else if (fnGetTVSubMode() == TVFACEMODE)
	{
		fsel.ClearAll();
	}

	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

}

void UnwrapMod::HoldPoints()
{
	if (theHold.Holding() && !TestAFlag(A_HELD)) {
		SetAFlag(A_HELD);
		theHold.Put(new TVertRestore(this,FALSE));
	}
}

void UnwrapMod::HoldPointsNoFlags()
{
	if (theHold.Holding() && !TestAFlag(A_HELD)) {
		SetAFlag(A_HELD);
		theHold.Put(new TVertJustPosRestore(this,FALSE));
	}
}


void UnwrapMod::HoldSelection()
{
	if (theHold.Holding() && !TestAFlag(A_HELD)) {	
		SetAFlag(A_HELD);
		theHold.Put(new TSelRestore(this));
	}
}

void UnwrapMod::HoldPointsAndFaces(bool hidePeltDialog)
{
	if (theHold.Holding() && !TestAFlag(A_HELD)) {
		SetAFlag(A_HELD);
		theHold.Put(new TVertAndTFaceRestore(this,hidePeltDialog));
	}
}


void UnwrapMod::TypeInChanged(int which)
{
	theHold.Restore();
	TimeValue t = ip->GetTime();
	HoldPoints();

	TransferSelectionStart();

	Point3 uvw(0.0f,0.0f,0.0f);
	if (iU) uvw[0] =  iU->GetFVal();
	if (iV) uvw[1] =  iV->GetFVal();
	if (iW) uvw[2] =  iW->GetFVal();

	for (int i=0; i<TVMaps.v.Count(); i++) {

		if (absoluteTypeIn)
		{
			if (vsel[i]) {
				if (TVMaps.cont[i]) TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
				TVMaps.v[i].p[which] = uvw[which];
				if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
			}
			else if(TVMaps.v[i].influence != 0.0f)
			{
				float infl = TVMaps.v[i].influence;
				if (TVMaps.cont[i]) TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
				TVMaps.v[i].p[which] += (uvw[which] - TVMaps.v[i].p[which])*infl;
				if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);

			}
		}
		else
		{
			if (vsel[i]) {
				if (TVMaps.cont[i]) TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
				TVMaps.v[i].p[which] += uvw[which];
				if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
			}
			else if(TVMaps.v[i].influence != 0.0f)
			{
				float infl = TVMaps.v[i].influence;
				if (TVMaps.cont[i]) TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
				TVMaps.v[i].p[which] += uvw[which] * infl;
				if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);

			}
		}

	}
	TransferSelectionEnd(FALSE,FALSE);

	tempAmount = uvw[which];

	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
	ip->RedrawViews(ip->GetTime());
}

void UnwrapMod::ChannelChanged(int which, float x)
{
	TimeValue t = ip->GetTime();
	theHold.Begin();
	HoldPoints();
	Point3 uvw;
	uvw[0] = 0.0f;
	uvw[1] = 0.0f;
	uvw[2] = 0.0f;
	PlugControllers();

	uvw[which] = x;

	for (int i=0; i<TVMaps.v.Count(); i++) {
		if (vsel[i]) {
			if (TVMaps.cont[i]) TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			TVMaps.v[i].p[which] = uvw[which];
			if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}
		else if(TVMaps.v[i].influence != 0.0f)
		{
			float infl = TVMaps.v[i].influence;
			if (TVMaps.cont[i]) TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			TVMaps.v[i].p[which] += (uvw[which] - TVMaps.v[i].p[which])*infl;
			if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);

		}
	}
	theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));

	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
	ip->RedrawViews(ip->GetTime());
}

void UnwrapMod::SetVertexPosition(TimeValue t, int which, Point3 pos, BOOL hold, BOOL update)
{
	if (hold)
	{
		theHold.Begin();
		HoldPoints();
	}


	if ((which >=0) && (which < TVMaps.v.Count()))
	{
		PlugControllers(which);
		TVMaps.v[which].p = pos;
		if (TVMaps.cont[which]) TVMaps.cont[which]->SetValue(t,&TVMaps.v[which].p);
	}


	if (hold)
	{
		theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));
	}

	if (update)
	{
		NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
		InvalidateView();
		if (ip) ip->RedrawViews(ip->GetTime());
	}
}


void UnwrapMod::DeleteSelected()
{
	if (TVMaps.v.Count() == 0) return;
	theHold.SuperBegin();
	theHold.Begin();

	HoldPointsAndFaces();	

	for (int i=0; i<TVMaps.v.Count(); i++) 
	{
		if ( (vsel[i]) && (!(TVMaps.v[i].flags & FLAG_DEAD)) )
		{
			TVMaps.v[i].flags |= FLAG_DEAD;
			vsel.Set(i,FALSE);
		}
	}
	for (int i=0; i<TVMaps.f.Count(); i++) 
	{
		if (!(TVMaps.f[i]->flags & FLAG_DEAD))
		{
			for (int j=0; j<3; j++) 
			{
				int index = TVMaps.f[i]->t[j];
				if ((index < 0) || (index >= TVMaps.v.Count()))
				{
					DbgAssert(1);
				}
				else if (TVMaps.v[index].flags & FLAG_DEAD)
				{
					TVMaps.f[i]->flags |= FLAG_DEAD;
				}
			}
		}
	}
	// loop through faces all
	theHold.Accept(_T(GetString(IDS_PW_DELETE_SELECTED)));
	theHold.SuperAccept(_T(GetString(IDS_PW_DELETE_SELECTED)));

}

void UnwrapMod::HideSelected()
{

	theHold.SuperBegin();
	theHold.Begin();
	HoldPoints();	

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	for (int i=0; i<TVMaps.v.Count(); i++) 
	{
		if ( (vsel[i]) && (!(TVMaps.v[i].flags & FLAG_DEAD)) )
		{

			TVMaps.v[i].flags |= FLAG_HIDDEN;
			vsel.Set(i,FALSE);
		}
	}

	theHold.Accept(_T(GetString(IDS_PW_HIDE_SELECTED)));
	theHold.SuperAccept(_T(GetString(IDS_PW_HIDE_SELECTED)));

	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE,FALSE);

	if (fnGetTVSubMode() == TVVERTMODE)
		vsel.ClearAll();
	else if (fnGetTVSubMode() == TVEDGEMODE)
		esel.ClearAll();
	if (fnGetTVSubMode() == TVFACEMODE)
		fsel.ClearAll();


}

void UnwrapMod::UnHideAll()
{
	theHold.SuperBegin();
	theHold.Begin();
	HoldPoints();	

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	for (int i=0; i<TVMaps.v.Count(); i++) 
	{
		if ( (TVMaps.v[i].flags & FLAG_HIDDEN) && (!(TVMaps.v[i].flags & FLAG_DEAD)) )
		{
			TVMaps.v[i].flags -= FLAG_HIDDEN;
		}
	}

	theHold.Accept(_T(GetString(IDS_PW_UNHIDEALL)));
	theHold.SuperAccept(_T(GetString(IDS_PW_UNHIDEALL)));

	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE,FALSE);


}


void UnwrapMod::FreezeSelected()
{
	theHold.SuperBegin();
	theHold.Begin();
	HoldPoints();

	//convert our sub selection type to vertex selection
	TransferSelectionStart();


	for (int i=0; i<TVMaps.v.Count(); i++) 
	{
		if ( (vsel[i]) && (!(TVMaps.v[i].flags & FLAG_DEAD)) )
		{
			TVMaps.v[i].flags |= FLAG_FROZEN;
			vsel.Set(i,FALSE);
		}
	}
	theHold.Accept(_T(GetString(IDS_PW_FREEZE_SELECTED)));
	theHold.SuperAccept(_T(GetString(IDS_PW_FREEZE_SELECTED)));

	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE,FALSE);

	if (fnGetTVSubMode() == TVVERTMODE)
		vsel.ClearAll();
	else if (fnGetTVSubMode() == TVEDGEMODE)
		esel.ClearAll();
	if (fnGetTVSubMode() == TVFACEMODE)
		fsel.ClearAll();

	for (int i=0; i<TVMaps.v.Count(); i++) 
	{
		if ( (vsel[i]) && (TVMaps.v[i].flags & FLAG_FROZEN) )
		{			
			vsel.Set(i,FALSE);
		}
	}

	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		int deg = TVMaps.f[i]->count;
		BOOL frozen = FALSE;
		for (int j = 0; j < deg; j++)
		{
			int index = TVMaps.f[i]->t[j];
			if (TVMaps.v[index].flags & FLAG_FROZEN)
				frozen = TRUE;
		}
		if (frozen)
		{
			fsel.Set(i,FALSE);
		}
	}

	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		BOOL frozen = FALSE;
		if ( (TVMaps.v[a].flags & FLAG_FROZEN) ||
			 (TVMaps.v[b].flags & FLAG_FROZEN) )
		{
			esel.Set(i,FALSE);
		}
	}

}

void UnwrapMod::UnFreezeAll()
{
	theHold.SuperBegin();
	theHold.Begin();
	HoldPoints();	

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	for (int i=0; i<TVMaps.v.Count(); i++) 
	{
		if ( !(TVMaps.v[i].flags & FLAG_DEAD)) 
		{
			if ( (TVMaps.v[i].flags & FLAG_FROZEN)) 
				TVMaps.v[i].flags -= FLAG_FROZEN;
		}
	}
	theHold.Accept(_T(GetString(IDS_PW_UNFREEZEALL)));
	theHold.SuperAccept(_T(GetString(IDS_PW_UNFREEZEALL)));


	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE,FALSE);

}

void UnwrapMod::WeldSelected(BOOL hold, BOOL notify)
{

	if (hold)
	{
		theHold.Begin();
		HoldPointsAndFaces();	
	}

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	float sweldThreshold;

	sweldThreshold = weldThreshold * weldThreshold;

	for (int m=0; m<TVMaps.v.Count(); m++) 
	{
		if (vsel[m])
		{
			Point3 p(0.0f,0.0f,0.0f);
			Point3 op(0.0f,0.0f,0.0f);
			p = GetPoint(ip->GetTime(),m);
			op = p;
			int ct = 0;
			int index = -1;
			for (int i=m+1; i<TVMaps.v.Count(); i++) 
			{
				if ( (vsel[i]) && (!(TVMaps.v[i].flags & FLAG_DEAD)) )
					//	if (vsel[i]) 
				{
					Point3 np;

					np= GetPoint(ip->GetTime(),i);
					if (LengthSquared(np-op) < sweldThreshold)
					{
						p+= np;
						ct++;
						if (index == -1)
							index = m;
						TVMaps.v[i].flags |= FLAG_DEAD;
					}
				}
			}

			if ((index == -1) || (ct == 0))
			{
				//			theHold.SuperCancel();
				//			theHold.Cancel();
				//			return;
			}
			else
			{
				ct++;
				p = p /(float)ct;


				if (TVMaps.cont[index]) 
					TVMaps.cont[index]->GetValue(ip->GetTime(),&TVMaps.v[index].p,FOREVER);

				TVMaps.v[index].p = p;
				if (TVMaps.cont[index]) 
					TVMaps.cont[index]->SetValue(ip->GetTime(),&TVMaps.v[index].p);

         for (int i=0; i<TVMaps.f.Count(); i++) 
				{
					int pcount = 3;
					pcount = TVMaps.f[i]->count;
					for (int j=0; j<pcount; j++) 
					{
						int tvfIndex = TVMaps.f[i]->t[j];
						Point3 np = TVMaps.v[tvfIndex].p;
						if ((vsel[tvfIndex]) && (LengthSquared(np-op) < sweldThreshold)) 
						{
							if (tvfIndex != index)
							{
								TVMaps.f[i]->t[j] = index;
							}

						}

						if ( (TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) &&
							(TVMaps.f[i]->vecs) 
							)
						{
							tvfIndex = TVMaps.f[i]->vecs->handles[j*2];
							Point3 np = TVMaps.v[tvfIndex].p;
							if ((vsel[tvfIndex]) && (LengthSquared(np-op) < sweldThreshold)) 
							{
								if (tvfIndex != index)
								{
									TVMaps.f[i]->vecs->handles[j*2] = index;
								}

							}

							tvfIndex = TVMaps.f[i]->vecs->handles[j*2+1];
							np = TVMaps.v[tvfIndex].p;
							if ((vsel[tvfIndex]) && (LengthSquared(np-op) < sweldThreshold)) 
							{
								if (tvfIndex != index)
								{
									TVMaps.f[i]->vecs->handles[j*2+1] = index;
								}

							}
							if (TVMaps.f[i]->flags & FLAG_INTERIOR)
							{
								tvfIndex = TVMaps.f[i]->vecs->interiors[j];
								np = TVMaps.v[tvfIndex].p;
								if ((vsel[tvfIndex]) && (LengthSquared(np-op) < sweldThreshold)) 
								{
									if (tvfIndex != index)
									{
										TVMaps.f[i]->vecs->interiors[j] = index;
									}

								}

							}


						}
					}
				}
			}
		}
	}

	if (hold)
	{
		theHold.Accept(_T(GetString(IDS_PW_WELDSELECTED)));
	}	

	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE,TRUE);




	TVMaps.edgesValid= FALSE;
	if (notify)
	{
		NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
		if (ip) ip->RedrawViews(ip->GetTime());
		InvalidateView();
	}

	int largest = 0;
	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		int pcount = TVMaps.f[i]->count;
		for (int j=0; j<pcount; j++) 
		{
			int tvfIndex = TVMaps.f[i]->t[j];
			if (tvfIndex > largest) largest = tvfIndex;
		}
	}

	if (largest > TVMaps.v.Count()) DebugPrint("Error welding\n");


}

BOOL UnwrapMod::WeldPoints(HWND h, IPoint2 m)
{
	//theHold.SuperBegin();
	//theHold.Begin();
	theHold.Put(new TVertAndTFaceRestore(this));
	//HoldPointsAndFaces();	

	Point3 p(0.0f,0.0f,0.0f);
	int ct = 0;
	Point2 mp;
	mp.x = (float) m.x;
	mp.y = (float) m.y;

	float xzoom, yzoom;
	int width,height;
	ComputeZooms(h,xzoom,yzoom,width,height);
	int index = -1;
	BOOL holdNeeded = FALSE;

	Rect rect;
	rect.left = m.x ;
	rect.right = m.x;
	rect.top = m.y ;
	rect.bottom = m.y ;
	Tab<int> hits;

	int tempMode = mode;
	mode = 0;
	if (HitTest(rect,hits,FALSE))
	{
		for (int i=0; i<hits.Count(); i++) 
		{
			if (fnGetTVSubMode() == TVVERTMODE)
			{

				if (!vsel[hits[i]]) 
				{
					index = hits[i];
					i = hits.Count();
				}
			}
			else if (fnGetTVSubMode() == TVEDGEMODE)
			{
				index = tWeldHit;
				/*
				if (!esel[hits[i]]) 
				{
				index = hits[i];
				i = hits.Count();
				}
				*/
			}

		}
	}
	mode = ID_TOOL_WELD;

	/*
	for (int i=0; i<TVMaps.v.Count(); i++) 
	{
	if (!vsel[i]) 
	{
	Point2 sp;
	p = GetPoint(ip->GetTime(),i);
	sp = UVWToScreen(p,xzoom,yzoom,width,height);

	//		rect.left = sp.x + hitSize;
	//		rect.right = sp.x + hitSize;
	//		rect.top = sp.y - hitSize;
	//		rect.bottom = sp.y + hitSize;;

	if (Length(sp-mp) < hitSize)
	{
	index = i;
	i = TVMaps.v.Count();
	}

	}
	}
	*/
	Tab<int> selected, hit;
	if (fnGetTVSubMode() == TVVERTMODE)
	{
		selected.SetCount(1);
		hit.SetCount(1);
		hit[0] = index;
		for (int i = 0; i < vsel.GetSize(); i++)
		{
			if (vsel[i]) selected[0] = i;
		}
	}

	else if ((fnGetTVSubMode() == TVEDGEMODE) && (index != -1))
	{
		selected.SetCount(2);
		hit.SetCount(2);
		hit[0] = TVMaps.ePtrList[index]->a;
		hit[1] = TVMaps.ePtrList[index]->b;

		int otherEdgeIndex = 0; 

		for (int i = 0; i < esel.GetSize(); i++)
		{
			if (esel[i]) 
			{
				otherEdgeIndex = i; 
				selected[0] = TVMaps.ePtrList[i]->a;
				selected[1] = TVMaps.ePtrList[i]->b;
			}
		}
		int face1, face2;
		int firstVert1,firstVert2;
		int nextVert1,nextVert2;
		int prevVert1,prevVert2;
		face1 = TVMaps.ePtrList[index]->faceList[0];
		face2 = TVMaps.ePtrList[otherEdgeIndex]->faceList[0];
		firstVert1 = hit[0];
		firstVert2 = selected[0];
		//fix up points
		int pcount = 3;
		pcount = TVMaps.f[face1]->count;
   for (int i = 0; i < pcount; i++)
		{
			int tvfIndex = TVMaps.f[face1]->t[i];
			if (tvfIndex == firstVert1)
			{
				if (i != (pcount-1))
					nextVert1 = TVMaps.f[face1]->t[i+1];
				else nextVert1 = TVMaps.f[face1]->t[0];
				if (i!=0)
					prevVert1 = TVMaps.f[face1]->t[i-1];
				else prevVert1 = TVMaps.f[face1]->t[pcount-1];
			}
		}

		pcount = TVMaps.f[face2]->count;
   for (int i = 0; i < pcount; i++)
		{
			int tvfIndex = TVMaps.f[face2]->t[i];
			if (tvfIndex == firstVert2)
			{
				if (i != (pcount-1))
					nextVert2 = TVMaps.f[face2]->t[i+1];
				else nextVert2 = TVMaps.f[face2]->t[0];
				if (i!=0)
					prevVert2 = TVMaps.f[face2]->t[i-1];
				else prevVert2 = TVMaps.f[face2]->t[pcount-1];

			}
		}

		if (prevVert1 == hit[1])
		{
			int temp = hit[0];
			hit[0] = hit[1];
			hit[1] = temp;
		}
		if (prevVert2 == selected[1])
		{
			int temp = selected[0];
			selected[0] = selected[1];
			selected[1] = temp;
		}

		int tempSel = selected[0];
		selected[0] = selected[1];
		selected[1] = tempSel;

		/*	if (nextVert2 != selected[1])
		{
		int temp = selected[0];
		selected[0] = selected[1];
		selected[1] = temp;
		}

		if (nextVert1 != hit[1])
		{
		int temp = hit[0];
		hit[0] = hit[1];
		hit[1] = temp;
		}
		*/
		if (hit[0] == selected[1])
		{
			hit.Delete(0,1);
			selected.Delete(1,1);
		}

		else if (hit[1] == selected[0])
		{
			hit.Delete(1,1);
			selected.Delete(0,1);
		}

		//	hit.SetCount(1);
		DebugPrint("selected %d %d\n",selected[0],selected[1]);
		DebugPrint("hit      %d %d\n",hit[0],hit[1]);
	}


	for (int selIndex = 0; selIndex < selected.Count(); selIndex++)
	{
		BOOL first = TRUE;
		int index = hit[selIndex];
		if ( index != -1)
		{
			for (int i=0; i<TVMaps.f.Count(); i++) 
			{
				int pcount = 3;
				//		if (TVMaps.f[i].flags & FLAG_QUAD) pcount = 4;
				pcount = TVMaps.f[i]->count;

				for (int j=0; j<pcount; j++) 
				{
					int tvfIndex = TVMaps.f[i]->t[j];
					if (tvfIndex == selected[selIndex])
					{
						TVMaps.f[i]->t[j] = index;
						TVMaps.v[tvfIndex].flags |= FLAG_DEAD;
						holdNeeded = TRUE;
					}
					if ( (TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) &&
						(TVMaps.f[i]->vecs) 
						)
					{
						tvfIndex = TVMaps.f[i]->vecs->handles[j*2];
						if (tvfIndex == selected[selIndex])
						{
							TVMaps.f[i]->vecs->handles[j*2] = index;
							TVMaps.v[tvfIndex].flags |= FLAG_DEAD;
							holdNeeded = TRUE;
						}

						tvfIndex = TVMaps.f[i]->vecs->handles[j*2+1];
						if (tvfIndex == selected[selIndex])
						{
							TVMaps.f[i]->vecs->handles[j*2+1] = index;
							TVMaps.v[tvfIndex].flags |= FLAG_DEAD;
							holdNeeded = TRUE;
						}

						if (TVMaps.f[i]->flags & FLAG_INTERIOR) 
						{
							tvfIndex = TVMaps.f[i]->vecs->interiors[j];
							if ( tvfIndex == selected[selIndex])
							{
								TVMaps.f[i]->vecs->interiors[j] = index;
								TVMaps.v[tvfIndex].flags |= FLAG_DEAD;
								holdNeeded = TRUE;
							}

						}


					}
				}
			}

		}
	}

	int largest = 0;
	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		int pcount = TVMaps.f[i]->count;
		for (int j=0; j<pcount; j++) 
		{
			int tvfIndex = TVMaps.f[i]->t[j];
			if (tvfIndex > largest) largest = tvfIndex;
		}
	}

	if (largest > TVMaps.v.Count()) DebugPrint("Error welding\n");


	theHold.Accept(_T(GetString(IDS_PW_WELD)));
	TVMaps.edgesValid= FALSE;

	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
	InvalidateView();

	return holdNeeded;
}

int UnwrapMod::AddUVWPoint(Point3 p)
{
	//loop through vertex list looking for dead ones else attache to end
	int found = -1;
	for (int m= 0; m <TVMaps.v.Count();m++)
	{
		if (TVMaps.v[m].flags & FLAG_DEAD)
		{
			found =m;
			m = TVMaps.v.Count();
		}
	}
	//found dead spot add to it
	if (found != -1)
	{
		TVMaps.v[found].p = p;
		TVMaps.v[found].influence = 0.0f;
		if (TVMaps.cont[found]) 
			TVMaps.cont[found]->SetValue(ip->GetTime(),&TVMaps.v[found].p);

		TVMaps.v[found].flags -= FLAG_DEAD;
		return found;
	}
	//create a new vert
	else
	{
		UVW_TVVertClass tv;
		tv.p = p;
		tv.flags = 0;
		tv.influence = 0.0f;
		TVMaps.v.Append(1,&tv,1);

		vsel.SetSize(TVMaps.v.Count(), 1);
		Control* c;
		c = NULL;
		TVMaps.cont.Append(1,&c,1);		
		return TVMaps.v.Count()-1;
	}
}

void UnwrapMod::AddPoint(Point3 p, int j, int k, BOOL sel)
{
	//loop through vertex list looking for dead ones else attache to end
	int found = -1;
	for (int m= 0; m <TVMaps.v.Count();m++)
	{
		if (TVMaps.v[m].flags & FLAG_DEAD)
		{
			found =m;
			m = TVMaps.v.Count();
		}
	}
	//found dead spot add to it
	if (found != -1)
	{
		TVMaps.v[found].p = p;
		TVMaps.v[found].influence = 0.0f;
		if (TVMaps.cont[found]) 
			TVMaps.cont[found]->SetValue(ip->GetTime(),&TVMaps.v[found].p);

		TVMaps.f[j]->t[k] = found;
		TVMaps.v[found].flags -= FLAG_DEAD;
		if (sel) vsel.Set(found);
	}
	//create a new vert
	else
	{
		UVW_TVVertClass tv;
		tv.p = p;
		tv.flags = 0;
		tv.influence = 0.0f;
		TVMaps.v.Append(1,&tv,1);
		TVMaps.f[j]->t[k] = TVMaps.v.Count()-1;

		vsel.SetSize(TVMaps.v.Count(), 1);
		Control* c;
		c = NULL;
		TVMaps.cont.Append(1,&c,1);
		if (sel) vsel.Set(TVMaps.v.Count()-1);
	}
}
void UnwrapMod::AddHandle(Point3 p, int j, int k, BOOL sel)
{
	//loop through vertex list looking for dead ones else attache to end
	int found = -1;
	for (int m= 0; m <TVMaps.v.Count();m++)
	{
		if (TVMaps.v[m].flags & FLAG_DEAD)
		{
			found =m;
			m = TVMaps.v.Count();
		}
	}
	//found dead spot add to it
	if (found != -1)
	{
		TVMaps.v[found].p = p;
		if (TVMaps.cont[found]) 
			TVMaps.cont[found]->SetValue(ip->GetTime(),&TVMaps.v[found].p);

		TVMaps.f[j]->vecs->handles[k] = found;
		TVMaps.v[found].flags -= FLAG_DEAD;
		if (sel) vsel.Set(found);
	}
	//create a new vert
	else
	{
		UVW_TVVertClass tv;
		tv.p = p;
		tv.flags = 0;
		TVMaps.v.Append(1,&tv,1);
		TVMaps.f[j]->vecs->handles[k] = TVMaps.v.Count()-1;

		vsel.SetSize(TVMaps.v.Count(), 1);
		Control* c;
		c = NULL;
		TVMaps.cont.Append(1,&c,1);
		if (sel) vsel.Set(TVMaps.v.Count()-1);
	}

}
void UnwrapMod::AddInterior(Point3 p, int j, int k, BOOL sel)
{
	//loop through vertex list looking for dead ones else attache to end
	int found = -1;
	for (int m= 0; m <TVMaps.v.Count();m++)
	{
		if (TVMaps.v[m].flags & FLAG_DEAD)
		{
			found =m;
			m = TVMaps.v.Count();
		}
	}
	//found dead spot add to it
	if (found != -1)
	{
		TVMaps.v[found].p = p;
		if (TVMaps.cont[found]) 
			TVMaps.cont[found]->SetValue(ip->GetTime(),&TVMaps.v[found].p);

		TVMaps.f[j]->vecs->interiors[k] = found;
		TVMaps.v[found].flags -= FLAG_DEAD;
		if (sel) vsel.Set(found);
	}
	//create a new vert
	else
	{
		UVW_TVVertClass tv;
		tv.p = p;
		tv.flags = 0;
		TVMaps.v.Append(1,&tv,1);
		TVMaps.f[j]->vecs->interiors[k] = TVMaps.v.Count()-1;

		vsel.SetSize(TVMaps.v.Count(), 1);
		Control* c;
		c = NULL;
		TVMaps.cont.Append(1,&c,1);
		if (sel) vsel.Set(TVMaps.v.Count()-1);
	}


}

void UnwrapMod::BreakSelected()
{

	if (fnGetTVSubMode() == TVFACEMODE)
	{
		DetachEdgeVerts();
	}
	else
	{
		theHold.SuperBegin();
		theHold.Begin();
		HoldPointsAndFaces();	


		BitArray weldEdgeList;
		weldEdgeList.SetSize(TVMaps.ePtrList.Count()); 
		weldEdgeList.ClearAll();



		if (fnGetTVSubMode() == TVEDGEMODE)
		{

			BitArray vertsOnselectedEdge;
			vertsOnselectedEdge.SetSize(TVMaps.v.Count());
			vertsOnselectedEdge.ClearAll();

			for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
			{
				if (esel[i])
				{
					int a = TVMaps.ePtrList[i]->a;
					vertsOnselectedEdge.Set(a,TRUE);
					a = TVMaps.ePtrList[i]->b;
					vertsOnselectedEdge.Set(a,TRUE);
				}
			}
			for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
			{
				if (!esel[i])
				{
					int a = TVMaps.ePtrList[i]->a;
					int b = TVMaps.ePtrList[i]->b;
					if (vertsOnselectedEdge[a] || vertsOnselectedEdge[b])
						weldEdgeList.Set(i,TRUE);

				}
			}

			BitArray processedVerts;
			processedVerts.SetSize(TVMaps.v.Count());
			processedVerts.ClearAll();

			BitArray processedFace;
			processedFace.SetSize(TVMaps.f.Count());
			processedFace.ClearAll();


			Tab<int> groupsAtThisVert;
			groupsAtThisVert.SetCount(TVMaps.v.Count());

			for (int i = 0; i < TVMaps.v.Count(); i++)
			{
				groupsAtThisVert[i] = 0;
			}

			Tab<AdjacentItem*> edgesAtVert;
			TVMaps.BuildAdjacentUVEdgesToVerts( edgesAtVert);

			Tab<AdjacentItem*> facesAtVert;
			TVMaps.BuildAdjacentUVFacesToVerts( facesAtVert);

			BitArray boundaryVert;
			boundaryVert.SetSize(TVMaps.v.Count());

			Tab<UVW_TVFaceClass*> tempF;
			tempF.SetCount(TVMaps.f.Count());
			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				tempF[i] = TVMaps.f[i]->Clone();
			}

			Tab<UVW_TVVertClass> tempV;
			tempV =  TVMaps.v;

			for (int i = 0; i < tempV.Count(); i++)
			{
				if (vertsOnselectedEdge[i])
				{
					//get a selected edge
					int numberOfEdges = edgesAtVert[i]->index.Count();
					boundaryVert.ClearAll();
					for (int j = 0; j < numberOfEdges; j++)
					{
						int edgeIndex = edgesAtVert[i]->index[j];
						if (esel[edgeIndex] || TVMaps.ePtrList[edgeIndex]->faceList.Count()==1)
						{
							
							int a = TVMaps.ePtrList[edgeIndex]->a;
							int b = TVMaps.ePtrList[edgeIndex]->b;
							if (a == i)
							{
								boundaryVert.Set(b,TRUE);
							}
							else
							{
								boundaryVert.Set(a,TRUE);
							}
						}
					}
					//get a seed face
					int currentGroup = 0;
					
					while (facesAtVert[i]->index.Count() > 0)
					{
						Tab<int> groupFaces;
						int faceIndex = facesAtVert[i]->index[0];
						groupFaces.Append(1,&faceIndex,20);
						facesAtVert[i]->index.Delete(0,1);
						int a = i;
						int nextVert = -1;
						int previousVert = -1;
						tempF[faceIndex]->GetConnectedUVVerts(a, nextVert, previousVert);

						BOOL done = FALSE;
						while (!done)
						{
						//add face
							
							for (int k =0; k < facesAtVert[i]->index.Count(); k++)
							{
								int testFaceIndex = facesAtVert[i]->index[k];
								int n,p;

								tempF[testFaceIndex]->GetConnectedUVVerts(a, n, p);

								if ((n == previousVert) && (!boundaryVert[previousVert]))
								{
									previousVert = p;
									groupFaces.Append(1,&testFaceIndex,20);
									facesAtVert[i]->index.Delete(k,1);
									k--;
								}
								else if ((p == nextVert) && (!boundaryVert[nextVert]))
								{
									nextVert = n;
									groupFaces.Append(1,&testFaceIndex,20);
									facesAtVert[i]->index.Delete(k,1);
									k--;
								}
							}

							if (boundaryVert[nextVert] && boundaryVert[previousVert])
								done = TRUE;
						}

						if (currentGroup > 0)
						{
							int newIndex = -1;
							for (int k = 0; k < groupFaces.Count(); k++)
							{
							//find all the verts == i 
								int faceIndex = groupFaces[k];
								int deg = TVMaps.f[faceIndex]->count;
								//make a new vert
								
								for (int m = 0; m < deg; m++)
								{
									int tindex = TVMaps.f[faceIndex]->t[m];
									if (tindex == a)
									{
										if (newIndex == -1)
										{
											Point3 p = TVMaps.v[tindex].p;
											AddPoint(p, faceIndex,m, FALSE);
											newIndex  = TVMaps.f[faceIndex]->t[m];
										}
										else 
										{
											TVMaps.f[faceIndex]->t[m] = newIndex;
										}
									}
								}
							
							}
						}

						currentGroup++;

						
					}


				}
			}

			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				if (tempF[i])
					delete tempF[i];
			}

			for (int i = 0; i < facesAtVert.Count(); i++)
			{
				if (facesAtVert[i])
					delete facesAtVert[i];
			}
			for (int i = 0; i < edgesAtVert.Count(); i++)
			{
				if (edgesAtVert[i])
					delete edgesAtVert[i];
			}
			//now split patch handles if any
			for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
			{
				if (esel[i])
				{
					int veca = TVMaps.ePtrList[i]->avec;
					if (veca != -1)
					{
						int faceIndex = TVMaps.ePtrList[i]->faceList[0];
						int deg = TVMaps.f[faceIndex]->count;
						int j = faceIndex;
                  for (int k =0; k < deg*2; k++)
						{

							if ( (TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
								(TVMaps.f[j]->vecs) &&
                        (TVMaps.f[j]->vecs->handles[k] == veca) && 
								(!(TVMaps.f[j]->flags & FLAG_DEAD))
								)						
							{
								Point3 p = TVMaps.v[veca].p;
                        AddHandle(p, j, k,TRUE);
							}

						}
					}
					veca = TVMaps.ePtrList[i]->bvec;
					if (veca != -1)
					{
						int faceIndex = TVMaps.ePtrList[i]->faceList[0];
						int deg = TVMaps.f[faceIndex]->count;
						int j = faceIndex;
                  for (int k =0; k < deg*2; k++)
						{

							if ( (TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
								(TVMaps.f[j]->vecs) &&
                        (TVMaps.f[j]->vecs->handles[k] == veca) && 
								(!(TVMaps.f[j]->flags & FLAG_DEAD))
								)						
							{
								Point3 p = TVMaps.v[veca].p;
                        AddHandle(p, j, k,TRUE);
							}

						}
					}
				}
			}
		}
		else
		{
			TransferSelectionStart();

			for (int i=0; i<TVMaps.v.Count(); i++) 
			{
				if ( (vsel[i]) && (!(TVMaps.v[i].flags & FLAG_DEAD)) )
				{
					//find all faces attached to this vertex
					Point3 p = GetPoint(ip->GetTime(),i);
					BOOL first = TRUE;
					for (int j=0; j<TVMaps.f.Count(); j++) 
					{
						int pcount = 3;
						pcount = TVMaps.f[j]->count ;
						for (int k = 0; k < pcount;k++)
						{
							if ((TVMaps.f[j]->t[k] == i) && (!(TVMaps.f[j]->flags & FLAG_DEAD)))
							{
								if (first)
								{
									first = FALSE;
								}
								else
								{
									AddPoint(p, j, k,TRUE);
								}
							}
							if ( (TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
								(TVMaps.f[j]->vecs) &&
								(TVMaps.f[j]->vecs->handles[k*2] == i) && 
								(!(TVMaps.f[j]->flags & FLAG_DEAD))
								)
							{
								if (first)
								{
									first = FALSE;
								}
								else
								{
									AddHandle(p, j, k*2,TRUE);
								}

							}
							if ( (TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
								(TVMaps.f[j]->vecs) &&
								(TVMaps.f[j]->vecs->handles[k*2+1] == i) && 
								(!(TVMaps.f[j]->flags & FLAG_DEAD))
								)
							{
								if (first)
								{
									first = FALSE;
								}
								else
								{
									AddHandle(p, j, k*2+1,TRUE);
								}

							}
							if ( (TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
								(TVMaps.f[j]->flags & FLAG_INTERIOR) &&
								(TVMaps.f[j]->vecs) &&
								(TVMaps.f[j]->vecs->interiors[k] == i) && 
								(!(TVMaps.f[j]->flags & FLAG_DEAD))
								)
							{
								if (first)
								{
									first = FALSE;
								}
								else
								{
									AddInterior(p, j, k,TRUE);
								}

							}



						}
					}

				}
			}

			TransferSelectionEnd(FALSE,TRUE);
		}


/*
		if (fnGetTVSubMode() == TVEDGEMODE)
		{
			BitArray originalFSel;
			originalFSel.SetSize(fsel.GetSize());
			originalFSel = fsel;

 	 		fnSetTVSubMode(TVFACEMODE);

         for (int i = 0; i < weldEdgeList.GetSize(); i++)
			{
				if (weldEdgeList[i])
				{
					fsel.ClearAll();	

	DebugPrint("Welding\n");
					for (int j =0; j < TVMaps.ePtrList[i]->faceList.Count(); j++)
					{
						fsel.Set(TVMaps.ePtrList[i]->faceList[j]);
						DebugPrint("  Face %d\n",TVMaps.ePtrList[i]->faceList[j]);
					}
					TransferSelectionStart();
					WeldSelected(FALSE);

					TransferSelectionEnd(FALSE,TRUE);
				}
			}

			fnSetTVSubMode(TVEDGEMODE);

			fsel = originalFSel;
		}
*/
		theHold.Accept(_T(GetString(IDS_PW_BREAK)));
		theHold.SuperAccept(_T(GetString(IDS_PW_BREAK)));
		TVMaps.edgesValid= FALSE;

		NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);	
		if (ip) ip->RedrawViews(ip->GetTime());
		InvalidateView();

	}

}

void UnwrapMod::SnapPoint(Point3 &p)
{
	int i1,i2;
	GetUVWIndices(i1,i2);
	//	int ix, iy;
	float fx,fy;
	double dx,dy;
	//compute in pixel space
	//find closest whole pixel
	fx = (float) modf(( (float) p[i1] * (float) (bitmapWidth) ),&dx);
	fy = (float) modf(( (float) p[i2] * (float) (bitmapHeight) ),&dy);
	if (midPixelSnap)
	{
		//		if (fx > 0.5f) dx+=1.0f;
		//		if (fy > 0.5f) dy+=1.0f;
		dx += 0.5f;
		dy += 0.5f;
	}
	else
	{
		if (fx > 0.5f) dx+=1.0f;
		if (fy > 0.5f) dy+=1.0f;
	}
	//put back in UVW space
	p[i1] = (float)dx/(float)(bitmapWidth);
	p[i2] = (float)dy/(float)(bitmapHeight);
}

Point3 UnwrapMod::SnapPoint(Point3 snapPoint, int snapIndex)
{
   int i1, i2;
   GetUVWIndices(i1,i2);
   suspendNotify = TRUE;
   Point3 p = snapPoint;
//VSNAP
   if (gridSnap)
   {
      BOOL vSnap,eSnap, gSnap;
      pblock->GetValue(unwrap_vertexsnap,0,vSnap,FOREVER);
      pblock->GetValue(unwrap_edgesnap,0,eSnap,FOREVER);
      pblock->GetValue(unwrap_gridsnap,0,gSnap,FOREVER);

      BOOL snapped = FALSE;
      if ( vSnap  )
      {
         //convert to screen space
         //get our window width height
         float xzoom, yzoom;
         int width,height;
         ComputeZooms(hView,xzoom,yzoom,width,height);
         Point3 tvPoint = UVWToScreen(p,xzoom,yzoom,width,height);
         //look up that point in our list
         //get our pixel distance
         int gridStr = (int)(fnGetGridStr()*60.0f);
         int x,y;
         x = (int) tvPoint.x;
         y = (int) tvPoint.y;
         int startX, startY, endX, endY;

         int vertID = -1;
         for (int i = 0; i < gridStr; i++)
         {

            startX = x - i;
            endX = x + i;

            startY = y - i;
            endY = y + i;

            if (startX < 0) startX = 0;
            if (startY < 0) startY = 0;

            if (endX < 0) endX = 0;
            if (endY < 0) endY = 0;

            if (startX >= width) startX = width-1;
            if (startY >= height) startY = height-1;

            if (endX >= width) endX = width-1;
            if (endY >= height) endY = height-1;
            for (int iy = startY; iy <= endY; iy++)
            {
               for (int ix = startX; ix <= endX; ix++)
               {
                  int index = iy * width + ix;
                  if (index < vertexSnapBuffer.Count())
                  {
                     int ID = vertexSnapBuffer[index];
                     if ((ID != -1) && (ID != snapIndex))
                     {
                        vertID = ID;
                        ix = endX;
                        iy = endY;
                        i = gridStr;
                     }
                  }
               }
            }
         }

         if (vertID != -1)
         {
            p = TVMaps.v[vertID].p;
            snapped = TRUE;
         }
      }

      if ( eSnap && (!snapped)  )
      {

         //convert to screen space
         //get our window width height
         float xzoom, yzoom;
         int width,height;
         ComputeZooms(hView,xzoom,yzoom,width,height);
         Point3 tvPoint = UVWToScreen(p,xzoom,yzoom,width,height);
         //look up that point in our list
         //get our pixel distance
         int gridStr = (int)(fnGetGridStr()*60.0f);
         int x,y;
         x = (int) tvPoint.x;
         y = (int) tvPoint.y;
         int startX, startY, endX, endY;

         int egdeID = -1;
         for (int i = 0; i < gridStr; i++)
         {

            startX = x - i;
            endX = x + i;

            startY = y - i;
            endY = y + i;

            if (startX < 0) startX = 0;
            if (startY < 0) startY = 0;

            if (endX < 0) endX = 0;
            if (endY < 0) endY = 0;

            if (startX >= width) startX = width-1;
            if (startY >= height) startY = height-1;

            if (endX >= width) endX = width-1;
            if (endY >= height) endY = height-1;
            for (int iy = startY; iy <= endY; iy++)
            {
               for (int ix = startX; ix <= endX; ix++)
               {
                  int index = iy * width + ix;
                  if (index < edgeSnapBuffer.Count())
                  {
                     int ID = edgeSnapBuffer[index];
                     if ((ID >= 0) && (ID != snapIndex))
                     {
                        if (!edgesConnectedToSnapvert[ID])
                        {
                           egdeID = ID;
                           ix = endX;
                           iy = endY;
                           i = gridStr;
                        }
                     }
                  }
               }
            }
         }

         if (egdeID >= 0)
         {
//          Point3 p(0.0f,0.0f,0.0f);

            int a,b;
            a = TVMaps.ePtrList[egdeID]->a;
            b = TVMaps.ePtrList[egdeID]->b;

            Point3 pa,pb;
            pa = TVMaps.v[a].p;
            Point3 aPoint = UVWToScreen(pa,xzoom,yzoom,width,height);

            pb = TVMaps.v[b].p;
            Point3 bPoint = UVWToScreen(pb,xzoom,yzoom,width,height);

            float screenFLength = Length(aPoint-bPoint);
            float screenSLength = Length(tvPoint-aPoint);
            float per = screenSLength/screenFLength;

         
            Point3 vec = (pb-pa) * per;
            p = pa + vec;
         
            snapped = TRUE;
         }
      }

      if ( (gSnap)  && (!snapped) )
      {


         float rem = fmod(p[i1],gridSize);
         float per = rem/gridSize;

         per = gridSize * fnGetGridStr();

         float snapPos ;
         if (p[i1] >= 0)
            snapPos = (int)((p[i1]+(gridSize*0.5f))/gridSize) * gridSize;
         else snapPos = (int)((p[i1]-(gridSize*0.5f))/gridSize) * gridSize;

         if ( fabs(p[i1] - snapPos) < per)
            p[i1] = snapPos;

         if (p[i2] >= 0)
            snapPos = (int)((p[i2]+(gridSize*0.5f))/gridSize) * gridSize;
         else snapPos = (int)((p[i2]-(gridSize*0.5f))/gridSize) * gridSize;

         if ( fabs(p[i2] -snapPos) < per)
            p[i2] = snapPos;


      }
   }

   if ((isBitmap) && (pixelSnap))
   {
      SnapPoint(p);
   }

   return p;

}
void UnwrapMod::MovePoints(Point2 pt)
{
	int i1, i2;
	GetUVWIndices(i1,i2);
	HoldPoints();	
	TimeValue t = ip->GetTime();

	suspendNotify = TRUE;
//VSNAP
	if (gridSnap)
	{
		BOOL vSnap,eSnap, gSnap;
		pblock->GetValue(unwrap_vertexsnap,0,vSnap,FOREVER);
		pblock->GetValue(unwrap_edgesnap,0,eSnap,FOREVER);
		pblock->GetValue(unwrap_gridsnap,0,gSnap,FOREVER);

		BOOL snapped = FALSE;
		if ( vSnap && (mouseHitVert != -1) )
		{
			Point3 p = TVMaps.v[mouseHitVert].p;

			p[i1] += pt.x;
			p[i2] += pt.y;

			//convert to screen space
			//get our window width height
			float xzoom, yzoom;
			int width,height;
			ComputeZooms(hView,xzoom,yzoom,width,height);
			Point3 tvPoint = UVWToScreen(p,xzoom,yzoom,width,height);
			//look up that point in our list
			//get our pixel distance
			int gridStr = (int)(fnGetGridStr()*60.0f);
			int x,y;
			x = (int) tvPoint.x;
			y = (int) tvPoint.y;
			int startX, startY, endX, endY;

			int vertID = -1;
			for (int i = 0; i < gridStr; i++)
			{

				startX = x - i;
				endX = x + i;

				startY = y - i;
				endY = y + i;

				if (startX < 0) startX = 0;
				if (startY < 0) startY = 0;

				if (endX < 0) endX = 0;
				if (endY < 0) endY = 0;

				if (startX >= width) startX = width-1;
				if (startY >= height) startY = height-1;

				if (endX >= width) endX = width-1;
				if (endY >= height) endY = height-1;
				for (int iy = startY; iy <= endY; iy++)
				{
					for (int ix = startX; ix <= endX; ix++)
					{
						int index = iy * width + ix;
						if (index < vertexSnapBuffer.Count())
						{
							int ID = vertexSnapBuffer[index];
							if ((ID != -1) && (ID != mouseHitVert))
							{
								vertID = ID;
								ix = endX;
								iy = endY;
								i = gridStr;
							}
						}
					}
				}
			}

			if (vertID != -1)
			{
				Point3 p = TVMaps.v[vertID].p;
				pt.x = p[i1] - TVMaps.v[mouseHitVert].p[i1];
				pt.y = p[i2] - TVMaps.v[mouseHitVert].p[i2];
				snapped = TRUE;
			}
		}

		if ( eSnap && (!snapped) && (mouseHitVert != -1) )
		{
			Point3 p = TVMaps.v[mouseHitVert].p;

			p[i1] += pt.x;
			p[i2] += pt.y;

			//convert to screen space
			//get our window width height
			float xzoom, yzoom;
			int width,height;
			ComputeZooms(hView,xzoom,yzoom,width,height);
			Point3 tvPoint = UVWToScreen(p,xzoom,yzoom,width,height);
			//look up that point in our list
			//get our pixel distance
			int gridStr = (int)(fnGetGridStr()*60.0f);
			int x,y;
			x = (int) tvPoint.x;
			y = (int) tvPoint.y;
			int startX, startY, endX, endY;

			int egdeID = -1;
			for (int i = 0; i < gridStr; i++)
			{

				startX = x - i;
				endX = x + i;

				startY = y - i;
				endY = y + i;

				if (startX < 0) startX = 0;
				if (startY < 0) startY = 0;

				if (endX < 0) endX = 0;
				if (endY < 0) endY = 0;

				if (startX >= width) startX = width-1;
				if (startY >= height) startY = height-1;

				if (endX >= width) endX = width-1;
				if (endY >= height) endY = height-1;
				for (int iy = startY; iy <= endY; iy++)
				{
					for (int ix = startX; ix <= endX; ix++)
					{
						int index = iy * width + ix;
						if (index < edgeSnapBuffer.Count())
						{
							int ID = edgeSnapBuffer[index];
							if ((ID >= 0) && (ID != mouseHitVert))
							{
								if (!edgesConnectedToSnapvert[ID])
								{
									egdeID = ID;
									ix = endX;
									iy = endY;
									i = gridStr;
								}
							}
						}
					}
				}
			}

			if (egdeID >= 0)
			{
				Point3 p(0.0f,0.0f,0.0f);

				int a,b;
				a = TVMaps.ePtrList[egdeID]->a;
				b = TVMaps.ePtrList[egdeID]->b;

				Point3 pa,pb;
				pa = TVMaps.v[a].p;
				Point3 aPoint = UVWToScreen(pa,xzoom,yzoom,width,height);

				pb = TVMaps.v[b].p;
				Point3 bPoint = UVWToScreen(pb,xzoom,yzoom,width,height);

				float screenFLength = Length(aPoint-bPoint);
				float screenSLength = Length(tvPoint-aPoint);
				float per = screenSLength/screenFLength;

			
				Point3 vec = (pb-pa) * per;
				p = pa + vec;
			

				pt.x = p[i1] - TVMaps.v[mouseHitVert].p[i1];
				pt.y = p[i2] - TVMaps.v[mouseHitVert].p[i2];
				snapped = TRUE;
			}
		}

		if ( (gSnap) && (mouseHitVert != -1) && (!snapped) )
		{
			Point3 p = TVMaps.v[mouseHitVert].p;

			p[i1] += pt.x;
			p[i2] += pt.y;

			float rem = fmod(p[i1],gridSize);
			float per = rem/gridSize;

			per = gridSize * fnGetGridStr();

			float snapPos ;
			if (p[i1] >= 0)
				snapPos = (int)((p[i1]+(gridSize*0.5f))/gridSize) * gridSize;
			else snapPos = (int)((p[i1]-(gridSize*0.5f))/gridSize) * gridSize;

			if ( fabs(p[i1] - snapPos) < per)
				p[i1] = snapPos;

			if (p[i2] >= 0)
				snapPos = (int)((p[i2]+(gridSize*0.5f))/gridSize) * gridSize;
			else snapPos = (int)((p[i2]-(gridSize*0.5f))/gridSize) * gridSize;

			if ( fabs(p[i2] -snapPos) < per)
				p[i2] = snapPos;

			pt.x = p[i1] - TVMaps.v[mouseHitVert].p[i1];
			pt.y = p[i2] - TVMaps.v[mouseHitVert].p[i2];
		}
	}

	for (int i=0; i<TVMaps.v.Count(); i++) {
		if (vsel[i]) {
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			TVMaps.v[i].p[i1] += pt.x;
			TVMaps.v[i].p[i2] += pt.y;
			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}
		else if((TVMaps.v[i].influence != 0.0f) && (fnGetTVSubMode() == TVVERTMODE))
		{
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			Point3 NewPoint = TVMaps.v[i].p;
			NewPoint[i1] += pt.x;
			NewPoint[i2] += pt.y;
			Point3 vec;
			vec = (NewPoint - TVMaps.v[i].p) * TVMaps.v[i].influence;
			TVMaps.v[i].p += vec;
			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}

	}
	suspendNotify = FALSE;
	if (update)
		NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
}

void UnwrapMod::MoveGizmo(Point2 pt)
{
	int i1, i2;
	GetUVWIndices(i1,i2);


	HoldPoints();	
	TimeValue t = ip->GetTime();

	if (offsetControl) 
		offsetControl->GetValue(t,&gOffset,FOREVER);
	gOffset[i1] += pt.x;
	gOffset[i2] += pt.y;
	if (offsetControl) offsetControl->SetValue(t,&gOffset);

	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
}

void UnwrapMod::RotatePoints(HWND h, float ang)
{
	HoldPoints();	
	TimeValue t = ip->GetTime();

	Point3 cent(0,0,0);

	if (centeron)
	{
		float xzoom, yzoom;
		int width,height;

		ComputeZooms(h,xzoom,yzoom,width,height);


		int tx = (width-int(xzoom))/2;
		int ty = (height-int(yzoom))/2;
		int i1, i2;
		GetUVWIndices(i1,i2);

		cent[i1] = (center.x-tx-xscroll)/xzoom;
		cent[i2] = (center.y+ty-yscroll - height)/-yzoom;
		//		cent.z = 0.0f;
		//		cent.x = 0.0f(center.x-tx-xscroll)/xzoom;
		//		cent.y = 0.0f;//(center.y+ty-yscroll - height)/-yzoom;
	}
	else
	{

		int ct = 0;
		Box3 bbox;

		bbox.Init();

		for (int i=0; i<TVMaps.v.Count(); i++) {
			if (vsel[i]) {
				cent += TVMaps.v[i].p;
				bbox += TVMaps.v[i].p;
				ct++;
			}
		}
		if (!ct) return;
		cent /= float(ct);
		cent = bbox.Center();
	}

	axisCenter.x = cent.x;
	axisCenter.y = cent.y;
	axisCenter.z = 0.0f;
	Matrix3 mat(1);	
	ang = ip->SnapAngle(ang,FALSE);

	BOOL respectAspectRatio = rotationsRespectAspect;

	if (aspect == 1.0f)
		respectAspectRatio = FALSE;


	if (respectAspectRatio)
	{
		cent[uvw] *= aspect;
	}

	mat.Translate(-cent);

	currentRotationAngle = ang * 180.0f/PI;
	switch (uvw) {
case 0: mat.RotateZ(ang); break;
case 1: mat.RotateX(ang); break;
case 2: mat.RotateY(ang); break;
	}
	mat.Translate(cent);

	suspendNotify = TRUE;
	for (int i=0; i<TVMaps.v.Count(); i++) {
		if (vsel[i]) {
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			if (respectAspectRatio)
			{
				TVMaps.v[i].p[uvw] *= aspect;
				TVMaps.v[i].p = mat * TVMaps.v[i].p;
				TVMaps.v[i].p[uvw] /= aspect;
			}
			else TVMaps.v[i].p = mat * TVMaps.v[i].p;

			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}
		else if ((TVMaps.v[i].influence != 0.0f) && (fnGetTVSubMode() == TVVERTMODE))
		{
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			Point3 NewPoint = TVMaps.v[i].p;
			NewPoint = mat * TVMaps.v[i].p;

			Point3 vec;
			vec = (NewPoint - TVMaps.v[i].p) * TVMaps.v[i].influence;
			TVMaps.v[i].p += vec;

			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}

	}
	suspendNotify = FALSE;
	if (update)
		NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);

	InvalidateView();
}

void UnwrapMod::RotateAroundAxis(HWND h, float ang, Point3 axis)
{
	HoldPoints();	
	TimeValue t = ip->GetTime();

	Point3 cent(0,0,0);


	cent = axis;
	Matrix3 mat(1);	
	ang = ip->SnapAngle(ang,FALSE);

	BOOL respectAspectRatio = rotationsRespectAspect;

	if (aspect == 1.0f)
		respectAspectRatio = FALSE;


	if (respectAspectRatio)
	{
		cent[uvw] *= aspect;
	}

	mat.Translate(-cent);

	currentRotationAngle = ang * 180.0f/PI;
	switch (uvw) 
	{
		case 0: mat.RotateZ(ang); break;
		case 1: mat.RotateX(ang); break;
		case 2: mat.RotateY(ang); break;
	}
	mat.Translate(cent);

	suspendNotify = TRUE;
	for (int i=0; i<TVMaps.v.Count(); i++) {
		if (vsel[i]) {
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			if (respectAspectRatio)
			{
				TVMaps.v[i].p[uvw] *= aspect;
				TVMaps.v[i].p = mat * TVMaps.v[i].p;
				TVMaps.v[i].p[uvw] /= aspect;
			}
			else TVMaps.v[i].p = mat * TVMaps.v[i].p;

			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}
		else if ((TVMaps.v[i].influence != 0.0f) && (fnGetTVSubMode() == TVVERTMODE))
		{
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			Point3 NewPoint = TVMaps.v[i].p;
			NewPoint = mat * TVMaps.v[i].p;

			Point3 vec;
			vec = (NewPoint - TVMaps.v[i].p) * TVMaps.v[i].influence;
			TVMaps.v[i].p += vec;

			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}

	}
	suspendNotify = FALSE;
	if (update)
		NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);

	InvalidateView();
}

void UnwrapMod::RotateGizmo(HWND h, float ang)
{
	HoldPoints();	
	TimeValue t = ip->GetTime();

	Point3 cent(0,0,0);

	if (centeron)
	{
		float xzoom, yzoom;
		int width,height;

		ComputeZooms(h,xzoom,yzoom,width,height);


		int tx = (width-int(xzoom))/2;
		int ty = (height-int(yzoom))/2;
		cent.x = (center.x-tx-xscroll)/xzoom;
		cent.y = (center.y+ty-yscroll - height)/-yzoom;
		cent.z = 0.0f;
		//		cent.x = 0.0f(center.x-tx-xscroll)/xzoom;
		//		cent.y = 0.0f;//(center.y+ty-yscroll - height)/-yzoom;
	}
	else
	{
		cent = gOffset;
		/*
		int ct = 0;
		for (int i=0; i<TVMaps.v.Count(); i++) {
		if (vsel[i]) {
		cent += TVMaps.v[i].p;
		ct++;
		}
		}
		if (!ct) return;
		cent /= float(ct);
		*/
	}

	if (rotateControl) 
		rotateControl->GetValue(t,&gRotate,FOREVER);
	gRotate += ang;

	if (rotateControl) 
		rotateControl->SetValue(t,&gRotate);

	/*
	Matrix3 mat(1);	
	mat.Translate(-cent);
	switch (uvw) {
	case 0: mat.RotateZ(ang); break;
	case 1: mat.RotateX(ang); break;
	case 2: mat.RotateY(ang); break;
	}
	mat.Translate(cent);

	for (int i=0; i<TVMaps.v.Count(); i++) {
	if (vsel[i]) {
	if (TVMaps.cont[i]) 
	TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
	//check snap and bitmap
	TVMaps.v[i].p = mat * TVMaps.v[i].p;
	if ((isBitmap) && (pixelSnap))
	{
	SnapPoint(TVMaps.v[i].p);
	}
	if (TVMaps.cont[i]) 
	TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
	}
	}
	*/
	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
}


void UnwrapMod::ScaleGizmo(HWND h, float scale, int direction)
{
	HoldPoints();	
	TimeValue t = ip->GetTime();

	Point3 cent(0,0,0);
	scale = scale - 1.0f;
	if (centeron)
	{
		float xzoom, yzoom;
		int width,height;

		ComputeZooms(h,xzoom,yzoom,width,height);

		int tx = (width-int(xzoom))/2;
		int ty = (height-int(yzoom))/2;
		cent.x = (center.x-tx-xscroll)/xzoom;
		cent.y = (center.y+ty-yscroll - height)/-yzoom;
		cent.z = 0.0f;
	}
	else
	{
		cent = gOffset;
		/*
		int ct = 0;
		for (int i=0; i<TVMaps.v.Count(); i++) {
		if (vsel[i]) {
		cent += TVMaps.v[i].p;
		ct++;
		}
		}
		if (!ct) return;
		cent /= float(ct);
		*/
	}

	Matrix3 mat(1);	
	mat.Translate(-cent);
	Point3 sc(0.0f,0.0f,0.0f);
	int i1, i2;
	GetUVWIndices(i1,i2);
	if (direction == 0)
	{
		sc[i1] = scale;
		sc[i2] = scale;
	}
	else if (direction == 1)
	{
		sc[i1] = -scale;
		//		sc[i2] = scale;
	}
	else if (direction == 2)
	{
		//		sc[i1] = scale;
		sc[i2] = scale;
	}

	//	sc[i1] = scale;
	//	sc[i2] = scale;

	mat.Scale(sc,TRUE);
	mat.Translate(cent);
	if (scaleControl) 
		scaleControl->GetValue(t,&gScale,FOREVER);
	gScale += sc;

	if (scaleControl) 
		scaleControl->SetValue(t,&gScale);

	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
}


void UnwrapMod::ScalePoints(HWND h, float scale, int direction)
{
	HoldPoints();	
	TimeValue t = ip->GetTime();

	Point3 cent(0,0,0);
	int i;

	int i1, i2;
	GetUVWIndices(i1,i2);

	if (centeron)
	{
		float xzoom, yzoom;
		int width,height;

		ComputeZooms(h,xzoom,yzoom,width,height);

		int tx = (width-int(xzoom))/2;
		int ty = (height-int(yzoom))/2;
		cent[i1] = (center.x-tx-xscroll)/xzoom;
		cent[i2] = (center.y+ty-yscroll - height)/-yzoom;
		cent.z = 0.0f;
	}
	else
	{
		int ct = 0;
		for (int i=0; i<TVMaps.v.Count(); i++) {
			if (vsel[i]) {
				cent += TVMaps.v[i].p;
				ct++;
			}
		}
		if (!ct) return;
		cent /= float(ct);
	}

	Matrix3 mat(1);	
	mat.Translate(-cent);
	Point3 sc(1,1,1);

	if (direction == 0)
	{
		sc[i1] = scale;
		sc[i2] = scale;
	}
	else if (direction == 1)
	{
		sc[i1] = scale;
		//		sc[i2] = scale;
	}
	else if (direction == 2)
	{
		//		sc[i1] = scale;
		sc[i2] = scale;
	}

	//	sc[i1] = scale;
	//	sc[i2] = scale;

	axisCenter.x = cent.x;
	axisCenter.y = cent.y;
	axisCenter.z = 0.0f;

	mat.Scale(sc,TRUE);
	mat.Translate(cent);

	suspendNotify = TRUE;
	for (i=0; i<TVMaps.v.Count(); i++) {
		if (vsel[i]) {
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			TVMaps.v[i].p = mat * TVMaps.v[i].p;
			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}

			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}
		else if ((TVMaps.v[i].influence != 0.0f) && (fnGetTVSubMode() == TVVERTMODE))
		{
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			//			TVMaps.v[i].p = mat * TVMaps.v[i].p;
			Point3 NewPoint = TVMaps.v[i].p;
			NewPoint = mat * TVMaps.v[i].p;
			Point3 vec;
			vec = (NewPoint - TVMaps.v[i].p) * TVMaps.v[i].influence;
			TVMaps.v[i].p += vec;

			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}

			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}





	}
	suspendNotify = FALSE;
	if (update)
		NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);

	InvalidateView();
}



void UnwrapMod::ScalePointsXY(HWND h, float scaleX, float scaleY)
{
	HoldPoints();	
	TimeValue t = ip->GetTime();

	Point3 cent(0,0,0);
	int i;

	float xzoom, yzoom;
	int width,height;

	ComputeZooms(h,xzoom,yzoom,width,height);

	int i1,i2;
	GetUVWIndices(i1,i2);

	int tx = (width-int(xzoom))/2;
	int ty = (height-int(yzoom))/2;
	cent[i1] = (center.x-tx-xscroll)/xzoom;
	cent[i2] = (center.y+ty-yscroll - height)/-yzoom;
	//	cent.z = 0.0f;

	Matrix3 mat(1);	

	axisCenter.x = cent.x;
	axisCenter.y = cent.y;
	axisCenter.z = 0.0f;

	mat.Translate(-cent);
	Point3 sc(1,1,1);
	sc[i1] = scaleX;
	sc[i2] = scaleY;

	mat.Scale(sc,TRUE);
	mat.Translate(cent);

	suspendNotify = TRUE;
	for (i=0; i<TVMaps.v.Count(); i++) {
		if (vsel[i]) {
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			TVMaps.v[i].p = mat * TVMaps.v[i].p;
			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}

			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}
		else if(TVMaps.v[i].influence != 0.0f)
		{
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			//			TVMaps.v[i].p = mat * TVMaps.v[i].p;
			Point3 NewPoint = TVMaps.v[i].p;
			NewPoint = mat * TVMaps.v[i].p;
			Point3 vec;
			vec = (NewPoint - TVMaps.v[i].p) * TVMaps.v[i].influence;
			TVMaps.v[i].p += vec;

			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}

			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}





	}
	suspendNotify = FALSE;

	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
}



void UnwrapMod::ScaleAroundAxis(HWND h, float scaleX, float scaleY, Point3 axis)
{
	HoldPoints();	
	TimeValue t = ip->GetTime();

	Point3 cent(0,0,0);
	int i;

	float xzoom, yzoom;
	int width,height;

	ComputeZooms(h,xzoom,yzoom,width,height);

	int i1,i2;
	GetUVWIndices(i1,i2);

	cent = axis;

	Matrix3 mat(1);	
	mat.Translate(-cent);
	Point3 sc(1,1,1);
	sc[i1] = scaleX;
	sc[i2] = scaleY;

	mat.Scale(sc,TRUE);
	mat.Translate(cent);

	suspendNotify = TRUE;
	for (i=0; i<TVMaps.v.Count(); i++) {
		if (vsel[i]) {
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			TVMaps.v[i].p = mat * TVMaps.v[i].p;
			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}

			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}
		else if(TVMaps.v[i].influence != 0.0f)
		{
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			//			TVMaps.v[i].p = mat * TVMaps.v[i].p;
			Point3 NewPoint = TVMaps.v[i].p;
			NewPoint = mat * TVMaps.v[i].p;
			Point3 vec;
			vec = (NewPoint - TVMaps.v[i].p) * TVMaps.v[i].influence;
			TVMaps.v[i].p += vec;

			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}

			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}





	}
	suspendNotify = FALSE;

	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
}



void UnwrapMod::MirrorGizmo(HWND h, int direction)
{

	HoldPoints();	
	TimeValue t = ip->GetTime();
	/*
	Point3 cent(0,0,0);
	int ct = 0;
	for (int i=0; i<TVMaps.v.Count(); i++) {
	if (vsel[i]) {
	cent += TVMaps.v[i].p;
	ct++;
	}
	}
	if (!ct) return;
	cent /= float(ct);
	*/
	//	Matrix3 mat(1);	
	//	mat.Translate(-cent);
	Point3 sc(1.0f,1.0f,1.0f);
	int i1, i2;
	GetUVWIndices(i1,i2);
	if (direction == 0)
	{
		sc[i1] = -1.0f;
	}
	else if (direction == 1)
	{
		sc[i2] = -1.0f;
	}


	//flip the scale transform
	//	mat.Scale(sc,TRUE);
	//	mat.Translate(cent);
	if (scaleControl) 
		scaleControl->GetValue(t,&gScale,FOREVER);
	gScale *= sc;

	if (scaleControl) 
		scaleControl->SetValue(t,&gScale);

	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
}

void UnwrapMod::DetachEdgeVerts(BOOL hold)
{
	if (hold)
	{
		theHold.Begin();
		HoldPointsAndFaces();	
	}


	if (fnGetTVSubMode() == TVFACEMODE)
	{
		//get our shared verts
		BitArray selVerts;
		BitArray unSelVerts;
		selVerts.SetSize(TVMaps.v.Count());
		unSelVerts.SetSize(TVMaps.v.Count());
		selVerts.ClearAll();
		unSelVerts.ClearAll();
		//loop though our face selection get an array of vertices that the face selection uses
		//loop through our non selected verts get an array of those vertices
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			int deg = TVMaps.f[i]->count;
			for (int j = 0; j < deg; j++)
			{
				int index = TVMaps.f[i]->t[j];
				if (fsel[i])
					selVerts.Set(index,TRUE);
				else unSelVerts.Set(index,TRUE);
				if (TVMaps.f[i]->vecs)
				{
					int index = TVMaps.f[i]->vecs->handles[j*2];
					if (index != -1)
					{
						if (fsel[i])
							selVerts.Set(index,TRUE);
						else unSelVerts.Set(index,TRUE);
					}
					index = TVMaps.f[i]->vecs->handles[j*2+1];
					if (index != -1)
					{
						if (fsel[i])
							selVerts.Set(index,TRUE);
						else unSelVerts.Set(index,TRUE);
					}
					index = TVMaps.f[i]->vecs->interiors[j];
					if (index != -1)
					{
						if (fsel[i])
							selVerts.Set(index,TRUE);
						else unSelVerts.Set(index,TRUE);
					}
				}

			}
		}

		//loop through for matching verts they are shared
		//create clone of those
		//store a look up
		Tab<int> oldToNewIndex;
		oldToNewIndex.SetCount(TVMaps.v.Count());
		for (int i = 0; i < oldToNewIndex.Count(); i++)
		{
			oldToNewIndex[i] = -1;
			if (selVerts[i] && unSelVerts[i])
			{
				Point3 p = TVMaps.v[i].p;
				int newIndex = AddUVWPoint(p);
				oldToNewIndex[i] = newIndex;
			}
		}

		//go back and fix and faces that use the look up vertices
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			if (fsel[i])
			{
				int deg = TVMaps.f[i]->count;
				for (int j = 0; j < deg; j++)
				{
					int index = TVMaps.f[i]->t[j];					
					int newIndex = oldToNewIndex[index];
					if (newIndex != -1)
						TVMaps.f[i]->t[j] = newIndex;
					if (TVMaps.f[i]->vecs)
					{
						int index = TVMaps.f[i]->vecs->handles[j*2];
						if (index != -1)
						{
							newIndex = oldToNewIndex[index];
							if (newIndex != -1)
                        TVMaps.f[i]->vecs->handles[j*2] = newIndex;
						}
						index = TVMaps.f[i]->vecs->handles[j*2+1];
						if (index != -1)
						{
							newIndex = oldToNewIndex[index];
							if (newIndex != -1)
                        TVMaps.f[i]->vecs->handles[j*2+1] = newIndex;
						}
						index = TVMaps.f[i]->vecs->interiors[j];
						if (index != -1)
						{
							newIndex = oldToNewIndex[index];
							if (newIndex != -1)
                        TVMaps.f[i]->vecs->interiors[j] = newIndex;
						}
					}
				}
			}
		}
	}
	else
	{
		//convert our sub selection type to vertex selection
		TransferSelectionStart();

		//convert our selectoin to faces and then back to verts	
		//this will clean out any invalid vertices
		BitArray holdFace(fsel);
		GetFaceSelFromVert(fsel,FALSE);	
		GetVertSelFromFace(vsel);
		fsel = holdFace;



		//loop through verts 
		for (int i=0; i<TVMaps.v.Count(); i++) 
		{
			//check if selected
			if (vsel[i])
			{
				//if selected loop through faces that have this vert
				BOOL first = TRUE;
				int newID;
				Point3 p;

				first = TRUE;
				for (int j=0; j<TVMaps.f.Count(); j++) 
				{
					//if this vert is not selected  create a new vert and point it to it
					int ct = 0;
					int whichVert=-1;
					for (int k =0; k < TVMaps.f[j]->count;k++)
					{
						int id;
						id = TVMaps.f[j]->t[k];
						if (vsel[id]) ct++;
						if (id == i)
						{
							whichVert = k;
							p = TVMaps.v[id].p;
						}
					}
					//this face contains the vert
					if (whichVert != -1)
					{
						//checkif all selected;
						if (ct!=TVMaps.f[j]->count)
						{
							if (first)
							{
								first = FALSE;
								AddPoint(p, j, whichVert, FALSE);
								newID = TVMaps.f[j]->t[whichVert];
							}
							else
							{
								TVMaps.f[j]->t[whichVert] = newID;
							}
						}
					}
				}
				//now do handles if need be
				first = TRUE;
				BOOL removeIsolatedFaces = FALSE;
				int isoVert;
         for (int j=0; j<TVMaps.f.Count(); j++) 
				{
					if ( (TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[j]->vecs) )
					{
						int whichVert = -1;
						int ct = 0;
						for (int k =0; k < TVMaps.f[j]->count*2;k++)
						{
							int id;
							id = TVMaps.f[j]->vecs->handles[k];
							if (vsel[id]) ct++;
							if (id == i)
							{
								whichVert = k;
								p = TVMaps.v[id].p;
							}
						}
						//this face contains the vert
						if (whichVert != -1)
						{
							//checkif all selected;
							int owner = whichVert/2;
							//if owner selected break
							if (ct!=TVMaps.f[j]->count*2)
							{
								if (first)
								{
									first = FALSE;
									isoVert = TVMaps.f[j]->vecs->handles[whichVert];
									AddHandle(p, j, whichVert, FALSE);
									newID = TVMaps.f[j]->vecs->handles[whichVert];
									removeIsolatedFaces = TRUE;
								}
								else
								{
									TVMaps.f[j]->vecs->handles[whichVert] = newID;
								}
							}
						}

					}
				}

				if (removeIsolatedFaces)
				{
					BOOL hit = FALSE;
            for (int j=0; j<TVMaps.f.Count(); j++) 
					{
						for (int k =0; k < TVMaps.f[j]->count*2;k++)
						{
							int id;
							id = TVMaps.f[j]->vecs->handles[k];
							if (id == isoVert)
							{
								hit = TRUE;
								//							k = TVMaps.f[j]->count*2;
								//							j = TVMaps.f.Count();
							}
						}
					}
					if (!hit)
					{
						TVMaps.v[isoVert].flags |= FLAG_DEAD;
						vsel.Set(isoVert,FALSE);
					}
				}



			}
		}
		TransferSelectionEnd(FALSE,TRUE);
	}	

	if (hold)
		theHold.Accept(_T(GetString(IDS_PW_DETACH)));

	//put back our old vertex selection if need be
	
	TVMaps.edgesValid= FALSE;
	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
	InvalidateView();

}
void UnwrapMod::FlipPoints(int direction)
{
	//loop through faces
	TimeValue t = ip->GetTime();
	theHold.Begin();

	HoldPointsAndFaces();	
	//	HoldPoints();
	theHold.Accept(_T(GetString(IDS_PW_FLIP)));


	//	theHold.Suspend();
	DetachEdgeVerts(FALSE);
	MirrorPoints(hView, direction,FALSE);
	//	theHold.Resume();

	for (int i=0; i < TVMaps.v.Count(); i++)
	{
		if (TVMaps.cont[i]) 
			TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
	}


	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
	InvalidateView();

}

void UnwrapMod::MirrorPoints(HWND h, int direction,BOOL hold )
{
	TimeValue t = ip->GetTime();


	if (hold)
	{
		theHold.SuperBegin();
		theHold.Begin();
		HoldPoints();	
	}

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	Point3 cent(0,0,0);
	int i;
	int ct = 0;
	for (i=0; i<TVMaps.v.Count(); i++) {
		if (vsel[i]) {
			cent += TVMaps.v[i].p;
			ct++;
		}
	}
	if (!ct) 
	{
		if (hold)
		{
			theHold.Cancel();
			theHold.SuperCancel();
		}
		return;
	}
	cent /= float(ct);

	Matrix3 mat(1);	
	mat.Translate(-cent);
	Point3 sc(1.0f,1.0f,1.0f);
	int i1, i2;
	GetUVWIndices(i1,i2);
	if (direction == 0)
	{
		sc[i1] = -1.0f;
	}
	else if (direction == 1)
	{
		sc[i2] = -1.0f;
	}

	//	sc[i1] = scale;
	//	sc[i2] = scale;

	mat.Scale(sc,TRUE);
	mat.Translate(cent);

	for (i=0; i<TVMaps.v.Count(); i++) {
		if (vsel[i]) {
			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
			//check snap and bitmap
			TVMaps.v[i].p = mat * TVMaps.v[i].p;
			if ((isBitmap) && (pixelSnap))
			{
				SnapPoint(TVMaps.v[i].p);
			}

			if (TVMaps.cont[i]) 
				TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
		}
	}

	if (hold)
	{
		theHold.Accept(_T(GetString(IDS_TH_MIRROR)));
		theHold.SuperAccept(_T(GetString(IDS_TH_MIRROR)));
	}

	//put back our old vertex selection if need be
	TransferSelectionEnd(FALSE,FALSE);


	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
	InvalidateView();
}

void UnwrapMod::UpdateListBox()
{

	int ct = pblock->Count(unwrap_texmaplist);

	for (int i = 0; i < ct; i++)
	{
		Texmap *map;
		pblock->GetValue(unwrap_texmaplist,0,map,FOREVER,i);
		if(map)
		{
			int id = -1;
			pblock->GetValue(unwrap_texmapidlist,0,id,FOREVER,i);
			DebugPrint("Mat %s matid %d\n", map->GetFullName(),id);
		}
	}
	SendMessage(hTextures, CB_RESETCONTENT, 0, 0);

   HDC hdc = GetDC(hTextures);
   Rect rect;
   GetClientRectP( hTextures, &rect );
    SIZE size;
   int width = rect.w();

	dropDownListIDs.ZeroCount();
	if (ct > 0)
	{
		Texmap *map;
		int i = 0;
		pblock->GetValue(unwrap_texmaplist,0,map,FOREVER,i);
		SendMessage(hTextures, CB_ADDSTRING , 0, (LPARAM) (TCHAR*) map->GetFullName());		
      
      DLGetTextExtent(hdc, map->GetFullName(), &size);
      if ( size.cx > width ) 
         width = size.cx;

		dropDownListIDs.Append(1,&i,100);
	}

	for (int i = 1; i < ct; i++)
	{
		Texmap *map;
		pblock->GetValue(unwrap_texmaplist,0,map,FOREVER,i);
		if (map != NULL) 
		{
			if (matid == -1)
			{
				SendMessage(hTextures, CB_ADDSTRING , 0, (LPARAM) (TCHAR*) map->GetFullName());
				dropDownListIDs.Append(1,&i,100);

            DLGetTextExtent(hdc, map->GetFullName(), &size);
            if ( size.cx > width ) 
               width = size.cx;
			}
			else
			{
				int id = -1;
				pblock->GetValue(unwrap_texmapidlist,0,id,FOREVER,i);
				if (filterMatID[matid] == id)
				{
					SendMessage(hTextures, CB_ADDSTRING , 0, (LPARAM) (TCHAR*) map->GetFullName());
					dropDownListIDs.Append(1,&i,100);

               DLGetTextExtent(hdc, map->GetFullName(), &size);
               if ( size.cx > width ) 
                  width = size.cx;

				}
			}
		}
	}

	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)_T("---------------------"));	
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_PW_PICK));	
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_PW_REMOVE));	
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_PW_RESET));	
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)_T("---------------------"));

	if (CurrentMap > ct )
	{
		if (ct > 1)
			CurrentMap = 1;
		else CurrentMap = 0;
	}
	SendMessage(hTextures, CB_SETCURSEL, CurrentMap, 0 );

   ReleaseDC(hTextures,hdc); 
   if ( width > 0 ) 
      SendMessage(hTextures, CB_SETDROPPEDWIDTH, width+5, 0);

}




void UnwrapMod::ShowCheckerMaterial(BOOL show)
{
	
	MyEnumProc dep;              
	DoEnumDependents(&dep);
	if (dep.Nodes.Count() > 0)
	{
		INode *SelfNode = dep.Nodes[0];

		macroRecorder->Disable();
		if (SelfNode)
		{
			Interface *ip = GetCOREInterface();
			TimeValue t = ip->GetTime();
			Mtl *checkerMat = GetCheckerMap();
			Mtl *storedBaseMtl = NULL;
			pblock->GetValue(unwrap_originalmtl,0,storedBaseMtl,FOREVER);
			Mtl *currentBaseMtl = SelfNode->GetMtl();
			if ((show) && (checkerMat))
			{
				//copy the node material into the ref 101
				if (currentBaseMtl != checkerMat)
					pblock->SetValue(unwrap_originalmtl,0,currentBaseMtl);
					
			//set the checker to the material
				SelfNode->SetMtl(checkerMat);
			//turn it on
				ip->ActivateTexture(checkerMat->GetSubTexmap(1), checkerMat, 1);

			}
			else 
			{
				if (checkerMat)
					ip->DeActivateTexture(checkerMat->GetSubTexmap(1), checkerMat, 1);
				//copy the original material back into the node
				checkerWasShowing = FALSE;
				if (currentBaseMtl == checkerMat)
				{
					SelfNode->SetMtl((Mtl*)storedBaseMtl);
					checkerWasShowing = TRUE;
				}
				Mtl *nullMat = NULL;
				pblock->SetValue(unwrap_originalmtl,0,nullMat);

				
			}
			ip->RedrawViews(t);
		}
		macroRecorder->Enable();

	}

}

void UnwrapMod::AddMaterial(MtlBase *mtl, BOOL update)
{
	int found = -1;

	if (mtl->ClassID() == Class_ID(0x243e22c6, 0x63f6a014)) //gnormal material
	{
		if (mtl->GetSubTexmap(0) != NULL)
			mtl = mtl->GetSubTexmap(0);
	}

	int ct = pblock->Count(unwrap_texmaplist);
	for (int i = 0; i < ct; i++)
	{
		Texmap *map;
		pblock->GetValue(unwrap_texmaplist,0,map,FOREVER,i);
		if (map == mtl) 
		{
			return;
		}
	}
	AddToMaterialList((Texmap*) mtl, -1);
	
	CurrentMap = pblock->Count(unwrap_texmaplist) -1;
	UpdateListBox();


/*
   for (int i = 0; i < 90; i++)
	{
		if (map[i] == NULL) 
		{
			found = i;
			i = 90;
		}
	}
	if (found == -1)
	{
      for (int i = 89; i >0 ;i--)
			ReplaceReference(i+1,map[i-1]);
		ReplaceReference(1,mtl);
		SendMessage(hTextures, CB_DELETESTRING , 0, (LPARAM) (TCHAR*) mtl->GetFullName());
		SendMessage(hTextures, CB_INSERTSTRING , 0, (LPARAM) (TCHAR*) mtl->GetFullName());
		//		SendMessage(hTextures, CB_INSERTSTRING , found, (LPARAM) (TCHAR*) mtl->GetName());
		found = 0;
	}
	else 
	{
		ReplaceReference(found+1,mtl);
		SendMessage(hTextures, CB_INSERTSTRING , found, (LPARAM) (TCHAR*) mtl->GetFullName());
	}
	CurrentMap = found;
	if (update)
		SetupImage();
*/
}

void UnwrapMod::PickMap()
{	
	BOOL newMat=FALSE, cancel=FALSE;
	MtlBase *mtl = ip->DoMaterialBrowseDlg(
		hWnd,
		BROWSE_MAPSONLY|BROWSE_INCNONE|BROWSE_INSTANCEONLY,
		newMat,cancel);
	if (cancel) {
		if (newMat) mtl->MaybeAutoDelete();
		return;
	}

	if (mtl != NULL)
	{
		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap4.AddMap"), 1, 0,
			mr_reftarg,mtl);

		AddMaterial(mtl);
	}
}

UBYTE *RenderBitmap(Bitmap *bmp,int w, int h)
{
	float du = 1.0f/float(w);
	float dv = 1.0f/float(h);
	AColor col;
	//	SCTex sc;
	//	int scanw = ByteWidth(w*3);
	int scanw = ByteWidth(w);
	//	UBYTE *image = new UBYTE[ByteWidth(w*3)*h];
	UBYTE *image = new UBYTE[ByteWidth(w)*h];
	UBYTE *p1;

	//	sc.scale = 1.0f;
	//	sc.duvw = Point3(du,dv,0.0f);
	//	sc.dpt  = sc.duvw;
	//	sc.uvw.y = 1.0f-0.5f*dv;

	BMM_Color_64 color;
	for (int j=0; j<h; j++) {
		//		sc.scrPos.y = j;
		//		sc.uvw.x = 0.5f*du;				
		p1 = image + (h-j-1)*scanw;
		for (int i=0; i<w; i++) {
			bmp->GetPixels(i,j,1,&color);

			*p1++ = (UBYTE)(color.b>>8);
			*p1++ = (UBYTE)(color.g>>8);
			*p1++ = (UBYTE)(color.r>>8);	

		}		
	}
	return image;
}


void UnwrapMod::SetupImage()
{
	delete image; image = NULL;
	if (GetActiveMap()) {		
		iw = rendW;
		ih = rendH;
		aspect = 1.0f;
		//		Class_ID bid = Class_ID(BMTEX_CLASS_ID);
		Bitmap *bmp = NULL;
		if (GetActiveMap()->ClassID() == Class_ID(BMTEX_CLASS_ID,0) )
		{
			isBitmap = 1;
			BitmapTex *bmt;
			bmt = (BitmapTex *) GetActiveMap();
			bmp = bmt->GetBitmap(ip->GetTime());
			if (bmp!= NULL)
			{
				if (useBitmapRes)
				{
					bitmapWidth = bmp->Width();
					bitmapHeight = bmp->Height();
					iw = bitmapWidth;
					ih = bitmapHeight;
					aspect = (float)bitmapWidth/(float)bitmapHeight;
				}
				else	
				{
					bitmapWidth = iw;
					bitmapHeight = ih;

					aspect = (float)iw/(float)ih;
				}


			}
		}
		else
		{
         aspect = (float)iw/(float)ih;
			isBitmap = 0;
		}
		if (iw==0 || ih==0) return;
		GetActiveMap()->Update(ip->GetTime(), FOREVER);
		GetActiveMap()->LoadMapFiles(ip->GetTime());
		SetCursor(LoadCursor(NULL,IDC_WAIT));
		//		if (isBitmap)
		//			image = RenderTexMap(map[CurrentMap],bitmapWidth,bitmapHeight);
		//		else 
		if (GetActiveMap()->ClassID() == Class_ID(BMTEX_CLASS_ID,0) )
		{
			if (bmp != NULL)
			{
				if (useBitmapRes)
					image = RenderBitmap(bmp,iw,ih);
				else image = RenderTexMap(GetActiveMap(),iw,ih,GetShowImageAlpha());
			}
		}
		else image = RenderTexMap(GetActiveMap(),iw,ih,GetShowImageAlpha());
		SetCursor(LoadCursor(NULL,IDC_ARROW));
		InvalidateView();
	}
	if ((image) && (iShowMap)) iShowMap->Enable();
	else if (iShowMap) iShowMap->Disable();
	tileValid = FALSE;
}

void UnwrapMod::PlugControllers()
{

	if (Animating())
	{
		theHold.Begin();
		SuspendAnimate();
		AnimateOff();
		for (int i=0; i<TVMaps.v.Count(); i++) {
			if ((vsel[i] && !TVMaps.cont[i]) || ((TVMaps.v[i].influence != 0.0f) && !TVMaps.cont[i])){
				ReplaceReference(i+11+100,NewDefaultPoint3Controller());			
				TVMaps.cont[i]->SetValue(0,&TVMaps.v[i].p);
			}
		}
		ResumeAnimate();
		theHold.Accept(_T(GetString(IDS_PW_ASSIGN_CONTROLLER)));
	}
}

void UnwrapMod::PlugControllers(int i)
{

	if (Animating())
	{
		theHold.Begin();
		SuspendAnimate();
		AnimateOff();

		if ((i >= 0) && (i < TVMaps.v.Count()))
		{
			if (TVMaps.cont[i]==NULL)
			{
				ReplaceReference(i+11+100,NewDefaultPoint3Controller());			
				TVMaps.cont[i]->SetValue(0,&TVMaps.v[i].p);
			}
		
		}
		ResumeAnimate();
		theHold.Accept(_T(GetString(IDS_PW_ASSIGN_CONTROLLER)));
	}
}

void UnwrapMod::PlugControllers(BitArray whichVerts)
{

	if (Animating())
	{
		theHold.Begin();
		SuspendAnimate();
		AnimateOff();
		for (int i=0; i<TVMaps.v.Count(); i++) {
			if ((i <whichVerts.GetSize()) && whichVerts[i])
			{
				if (TVMaps.cont[i]==NULL)
				{
					ReplaceReference(i+11+100,NewDefaultPoint3Controller());			
					TVMaps.cont[i]->SetValue(0,&TVMaps.v[i].p);
				}
			}
		}
		ResumeAnimate();
		theHold.Accept(_T(GetString(IDS_PW_ASSIGN_CONTROLLER)));
	}
}

int UnwrapMod::GetAxis()
{
	return 2;
	return axis;
}

Point3 UnwrapMod::GetPoint(TimeValue t,int i)
{
	if (i>=TVMaps.cont.Count() || i>= TVMaps.v.Count()) {
		return Point3(0,0,0);
	}
	if (TVMaps.v[i].flags & FLAG_DEAD) {
		return Point3(0,0,0);
	}

	if (TVMaps.cont[i]) TVMaps.cont[i]->GetValue(t,&TVMaps.v[i].p,FOREVER);
	return TVMaps.v[i].p;
}


void UnwrapMod::BuildObjectPoints()
{
	objectPointList.SetCount(TVMaps.v.Count());

	Point3 p(0.0f,0.0f,0.0f);
	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		int pcount = 3;
		//		if (TVMaps.f[i].flags & FLAG_QUAD) pcount = 4;
		pcount = TVMaps.f[i]->count;
		for (int j = 0 ; j < pcount; j++)
		{
			if (TVMaps.f[i]->t[j]<objectPointList.Count())
				objectPointList[TVMaps.f[i]->t[j]] = TVMaps.geomPoints[TVMaps.f[i]->v[j]];
			if ( (TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) &&
				(TVMaps.f[i]->vecs)
				)
			{
				objectPointList[TVMaps.f[i]->vecs->handles[j*2]] = TVMaps.geomPoints[TVMaps.f[i]->vecs->vhandles[j*2]];
				objectPointList[TVMaps.f[i]->vecs->handles[j*2+1]] = TVMaps.geomPoints[TVMaps.f[i]->vecs->vhandles[j*2+1]];
				if (TVMaps.f[i]->flags & FLAG_INTERIOR) 
				{
					objectPointList[TVMaps.f[i]->vecs->interiors[j]] = TVMaps.geomPoints[TVMaps.f[i]->vecs->vinteriors[j]];
				}

			}


			//			if (index == TVMaps.f[i].t[j])
			//				return TVMaps.f[i].pt[j];
		}

	}

}

Point3 UnwrapMod::GetObjectPoint(TimeValue t,int index)
{

	Point3 p(0.0f,0.0f,0.0f);
	if (index<objectPointList.Count())
		p = objectPointList[index];
	/*	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
	int pcount = 3;
	if (TVMaps.f[i].flags & FLAG_QUAD) pcount = 4;
	for (int j = 0 ; j < pcount; j++)
	{
	if (index == TVMaps.f[i].t[j])
	return TVMaps.f[i].pt[j];
	}

	}
	*/
	return p;

}
/*
Point3 UnwrapMod::GetObjectPoint(TimeValue t,int index)
{
Point3 p(0.0f,0.0f,0.0f);
for (int i = 0; i < TVMaps.f.Count(); i++)
{
int pcount = 3;
if (TVMaps.f[i].flags & FLAG_QUAD) pcount = 4;
for (int j = 0 ; j < pcount; j++)
{
if (index == TVMaps.f[i].t[j])
return TVMaps.f[i].pt[j];
}

}
return p;

}
*/

void UnwrapMod::ZoomExtents()
{

	for (int k = 0; k < 2; k++)
	{
		Rect brect;
		Point2 pt;
		float xzoom, yzoom;
		int width,height;
		TimeValue t = ip->GetTime();
		ComputeZooms(hView,xzoom,yzoom,width,height);	
		brect.SetEmpty();
		IsSelectedSetup();
		for (int i=0; i<TVMaps.v.Count(); i++) {		
			if (!(TVMaps.v[i].flags & FLAG_DEAD))
			{
				if (IsVertVisible(i))
				{
					pt = UVWToScreen(GetPoint(t,i),xzoom,yzoom,width,height);
					IPoint2 ipt(int(pt.x),int(pt.y));
					brect += ipt;		
				}
			}
		}
		if (brect.IsEmpty()) return;

		if  (  (brect.w() == 1) || (brect.h() == 1) )
		{
			pt = UVWToScreen(Point3(0.0f,0.0f,0.0f),xzoom,yzoom,width,height);
			IPoint2 ipt(int(pt.x),int(pt.y));
			brect += ipt;		

			pt = UVWToScreen(Point3(1.0f,1.0f,1.0f),xzoom,yzoom,width,height);
			IPoint2 ipt2(int(pt.x),int(pt.y));
			brect += ipt2;		

		}

		Rect srect;
		GetClientRect(hView,&srect);

		float rat1, rat2;
		double bw,bh;
		double sw,sh;

		if (brect.w() == 1) 
		{

			brect.left--;
			brect.right++;
		}

		if (brect.h() == 1) 
		{

			brect.top--;
			brect.bottom++;
		}


		bw = brect.w();
		bh = brect.h();

		sw = srect.w();
		sh = srect.h();




		rat1 = float(sw-1.0f)/float(fabs(double(bw-1.0f)));
		rat2 = float(sh-1.0f)/float(fabs(double(bh-1.0f)));
		float rat = (rat1<rat2?rat1:rat2) * 0.9f;

		BOOL redo = FALSE;
		if (_isnan(rat))
		{
			rat = 1.0f;
			redo = TRUE;
		}
		if (!_finite(rat)) 
		{
			rat = 1.0f;
			redo = TRUE;
		}

		zoom *= rat;



		IPoint2 delta = srect.GetCenter() - brect.GetCenter();
		xscroll += delta.x;
		yscroll += delta.y;
		xscroll *= rat;
		yscroll *= rat;
	}




	InvalidateView();
}

void UnwrapMod::FrameSelectedElement()
{
	TransferSelectionStart();

	BitArray faceHasSelectedVert;

	BitArray tempVSel = vsel;

	faceHasSelectedVert.SetSize(TVMaps.f.Count());
	faceHasSelectedVert.ClearAll();

	int count = -1;

	while (count != vsel.NumberSet())
	{
		count = vsel.NumberSet();
		for (int i = 0; i < TVMaps.f.Count();i++)
		{
			if (!(TVMaps.f[i]->flags & FLAG_DEAD))
			{
				int pcount = 3;
				pcount = TVMaps.f[i]->count;
				int totalSelected = 0;
				for (int k = 0; k < pcount; k++)
				{
					int index = TVMaps.f[i]->t[k];
					if (vsel[index])
					{
						totalSelected++;
					}
				}

				if ( (totalSelected != pcount) && (totalSelected!= 0))
				{
					faceHasSelectedVert.Set(i);
				}
			}
		}
      for (int i = 0; i < TVMaps.f.Count();i++)
		{
			if (faceHasSelectedVert[i])
			{
				int pcount = 3;
				pcount = TVMaps.f[i]->count;
				int totalSelected = 0;
				for (int k = 0; k < pcount; k++)
				{
					int index = TVMaps.f[i]->t[k];
					vsel.Set(index,1);
				}
			}

		}
	}
	
	Rect brect;
	Point2 pt;
	float xzoom, yzoom;
	int width,height;
	TimeValue t = ip->GetTime();
	ComputeZooms(hView,xzoom,yzoom,width,height);	
	brect.SetEmpty();
	int found = 0;
	BOOL doAll = FALSE;

	if (vsel.NumberSet() == 0) doAll = TRUE;

	for (int i=0; i<TVMaps.v.Count(); i++) {		
		if (!(TVMaps.v[i].flags & FLAG_DEAD))
		{
			if ((vsel[i]) || doAll)
			{
				pt = UVWToScreen(GetPoint(t,i),xzoom,yzoom,width,height);
				IPoint2 ipt(int(pt.x),int(pt.y));
				brect += ipt;		
				found++;
			}
		}
	}

	vsel = tempVSel;
	TransferSelectionEnd(FALSE,FALSE);

	if (brect.w() < 5)
	{
		brect.left -= 5;
		brect.right += 5;
	}

	if (brect.h() < 5)
	{
		brect.top -= 5;
		brect.bottom += 5;
	}


	if (found <=1) return;
	Rect srect;
	GetClientRect(hView,&srect);		
	float rat1 = 1.0f, rat2 = 1.0f;
	if (brect.w()>2.0f )
		rat1 = float(srect.w()-1)/float(fabs(double(brect.w()-1)));
	if (brect.h()>2.0f )
		rat2 = float(srect.h()-1)/float(fabs(double(brect.h()-1)));
	float rat = (rat1<rat2?rat1:rat2) * 0.9f;
	float tempZoom = zoom *rat;
	if ( tempZoom <= 1000.0f)
	{
		zoom *= rat;
		IPoint2 delta = srect.GetCenter() - brect.GetCenter();
		xscroll += delta.x;
		yscroll += delta.y;
		xscroll *= rat;
		yscroll *= rat;	
		InvalidateView();
	}

}

void UnwrapMod::fnFrameSelectedElement()
{
	if (iZoomExt) 		
		iZoomExt->SetCurFlyOff(2,TRUE);
	FrameSelectedElement();
}

void UnwrapMod::ZoomSelected()
{
	TransferSelectionStart();

	Rect brect;
	Point2 pt;
	float xzoom, yzoom;
	int width,height;
	TimeValue t = ip->GetTime();
	ComputeZooms(hView,xzoom,yzoom,width,height);	
	brect.SetEmpty();
	int found = 0;
	BOOL doAll = FALSE;

	if (vsel.NumberSet() == 0) doAll = TRUE;

	for (int i=0; i<TVMaps.v.Count(); i++) {		
		if (!(TVMaps.v[i].flags & FLAG_DEAD))
		{
			if ((vsel[i]) || doAll)
			{
				pt = UVWToScreen(GetPoint(t,i),xzoom,yzoom,width,height);
				IPoint2 ipt(int(pt.x),int(pt.y));
				brect += ipt;		
				found++;
			}
		}
	}

	TransferSelectionEnd(FALSE,FALSE);

	if (brect.w() < 5)
	{
		brect.left -= 5;
		brect.right += 5;
	}

	if (brect.h() < 5)
	{
		brect.top -= 5;
		brect.bottom += 5;
	}


	if (found <=1) return;
	Rect srect;
	GetClientRect(hView,&srect);		
	float rat1 = 1.0f, rat2 = 1.0f;
	if (brect.w()>2.0f )
		rat1 = float(srect.w()-1)/float(fabs(double(brect.w()-1)));
	if (brect.h()>2.0f )
		rat2 = float(srect.h()-1)/float(fabs(double(brect.h()-1)));
	float rat = (rat1<rat2?rat1:rat2) * 0.9f;
	float tempZoom = zoom *rat;
	if ( tempZoom <= 1000.0f)
	{
		zoom *= rat;
		IPoint2 delta = srect.GetCenter() - brect.GetCenter();
		xscroll += delta.x;
		yscroll += delta.y;
		xscroll *= rat;
		yscroll *= rat;	
		InvalidateView();
	}
}


void UnwrapMod::AlignMap()
{
	TimeValue t = GetCOREInterface()->GetTime();	
	if (tmControl==NULL) InitControl(t);
	//	GeomObject *obj;
	Point3 norm, pt;
	Interval valid;

	// Get mod contexts and nodes for this modifier
	//	ModContextList mcList;
	//	INodeTab nodeList;
	//	ip->GetModContexts(mcList,nodeList);

	// Calculate a ray from the mouse point

	//	for (int i=0; i<nodeList.Count(); i++) {
	//		INode *node = nodeList[i];

	// Get the object from the node
	//		ObjectState os = node->EvalWorldState(t);
	//		if (os.obj->SuperClassID()==GEOMOBJECT_CLASS_ID) {
	//			obj = (GeomObject*)os.obj;
	//		} else {
	//			continue;
	//			}

	//compute the average normal or use x/y/z
	// See if we hit the object
	//find center of selection
	//average normal if needed
	Point3 zeroPoint(.0f,.0f,.0f);
	Ray ray;
	ray.p  = zeroPoint;
	ray.dir = zeroPoint;
	int dir = GetQMapAlign();
	if (dir == 0) //x
	{
		ray.dir.x = 1.0f; 
		norm = -ray.dir;
	}
	else if (dir == 1) //y
	{	
		ray.dir.y = 1.0f; 
		norm = -ray.dir;
	}	
	else if (dir == 2) //z
	{
		ray.dir.z = 1.0f; 
		norm = ray.dir;
	}
	else
	{
		//compute average normal
		Point3 pnorm(0.0f,0.0f,0.0f);
		for (int k=0; k<gfaces.Count(); k++) {
			// Grap the three points, xformed
			int pcount = 3;
			//				if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
			pcount = gfaces[k]->count;

			Point3 temp_point[4];
			for (int j=0; j<pcount; j++) {
				int index = gfaces[k]->t[j];
				//					Point3 temp_point;
				if (j < 4)
					temp_point[j] = gverts.d[index].p;
			}
			pnorm += Normalize(temp_point[1]-temp_point[0]^temp_point[2]-temp_point[1]);
		}
		if (gfaces.Count())
		{
			ray.dir = pnorm / (float) gfaces.Count();
			norm = -ray.dir;
		}
		else
		{
			norm = Point3(1.0f,0.0f,0.0f);
			ray.dir = Point3(1.0f,0.0f,0.0f);
		}
	}


	//		if (obj->IntersectRay(t,ray,at,norm)) {
	if (1) {
		// Calculate the hit point
		pt = ray.p + ray.dir;

		// Get the mod context tm
		Matrix3 tm(1);
		//			if (mcList[0]->tm) tm = tm * *mcList[0]->tm;

		// Transform the point and ray into mod context space
		pt = pt * tm;
		norm = Normalize(VectorTransform(tm,norm));

		// Construct the target transformation in mod context space
		Matrix3 destTM;
		UnwrapMatrixFromNormal(norm,destTM);
		destTM.SetTrans(pt);
		destTM.PreRotateZ(0);

		switch (GetAxis()) 
		{
			case 0:
				destTM.PreRotateY(-HALFPI);
				break;
			case 1:
				destTM.PreRotateX(HALFPI);
			break;

		}

		// Our current transformation... gives relative TM
		Matrix3 curTM(1), relTM, id(1);
		tmControl->GetValue(t,&curTM,valid,CTRL_RELATIVE);
		relTM = Inverse(curTM) * destTM;

		// Here's the modifications we need to make to get there
		tm.IdentityMatrix();
		tm.SetTrans(curTM.GetTrans());
		AffineParts parts;			
		decomp_affine(relTM,&parts);
		Point3 delta = destTM.GetTrans()-curTM.GetTrans();
//		Rotate(t,id,tm,parts.q);
//		Move(t,id,id,delta);

	}
}


void UnwrapMod::RebuildFreeFormData()
{

	//compute your zooms and scale
	float xzoom, yzoom;
	int width,height;
	ComputeZooms(hView,xzoom,yzoom,width,height);

	int count = 0;

	TransferSelectionStart();

	count = vsel.NumberSet();

	int vselCount = vsel.GetSize();
	freeFormBounds.Init();
	if (!inRotation)
		selCenter = Point3(0.0f,0.0f,0.0f);
	int setCount = 0;
	int i1, i2;
	GetUVWIndices(i1, i2);

	for (int i = 0; i < vselCount; i++)
	{
		if (vsel[i])
		{
			//get bounds
			Point3 p(0.0f,0.0f,0.0f);
			p[i1] = TVMaps.v[i].p[i1];
			p[i2] = TVMaps.v[i].p[i2];
			//			p.z = 0.0f;
			freeFormBounds += p;
		}
	}
	Point3 tempCenter;
	if (!inRotation)
		selCenter = freeFormBounds.Center();			
	else tempCenter = freeFormBounds.Center();			

	if (vsel.NumberSet() > 0)
	{	

		//draw gizmo bounds
		Point2 prect[4];
		prect[0] = UVWToScreen(freeFormBounds.pmin,xzoom,yzoom,width,height);
		prect[1] = UVWToScreen(freeFormBounds.pmax,xzoom,yzoom,width,height);
		float xexpand = 15.0f/xzoom;
		float yexpand = 15.0f/yzoom;

		if (!freeFormMode->dragging)
		{
			if ((prect[1].x-prect[0].x) < 30)
			{
				prect[1].x += 15;
				prect[0].x -= 15;
				//expand bounds
				freeFormBounds.pmax.x += xexpand;
				freeFormBounds.pmin.x -= xexpand;
			}
			if ((prect[0].y-prect[1].y) < 30)
			{
				prect[1].y -= 15;
				prect[0].y += 15;
				freeFormBounds.pmax.y += yexpand;
				freeFormBounds.pmin.y -= yexpand;

			}
		}
	}
	TransferSelectionEnd(FALSE,FALSE);
}
//--- Mouse procs for modes -----------------------------------------------

int SelectMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{


	switch (msg) {
case MOUSE_POINT:
	if (point==0) {
		// First click

		region   = FALSE;
		toggle   = flags&MOUSE_CTRL;
		subtract = flags&MOUSE_ALT;

		// Hit test
		Tab<int> hits;
		Rect rect;
		rect.left = m.x-2;
		rect.right = m.x+2;
		rect.top = m.y-2;
		rect.bottom = m.y+2;
		// First hit test sel only
		mod->centeron = 0;
		if (toggle && subtract)
		{
			mod->centeron = 1;
			return subproc(hWnd,msg,point,flags,m);
		}

		// First hit test sel only
		if  ( ((!toggle && !subtract && mod->HitTest(rect,hits,TRUE)) || (mod->lockSelected==1)) ||
			((mod->freeFormSubMode != ID_TOOL_SELECT) && (mod->mode == ID_FREEFORMMODE)) )
		{
			return subproc(hWnd,msg,point,flags,m);
		} else
			// Next hit test everything
			if (mod->HitTest(rect,hits,subtract)) 
			{
				theHold.Begin();
				mod->HoldSelection();
				theHold.Accept(_T(GetString(IDS_PW_SELECT_UVW)));

				theHold.Suspend();
				if (!toggle && !subtract) mod->ClearSelect();
				mod->Select(hits,toggle,subtract,FALSE);
				mod->InvalidateView();
				theHold.Resume();

				if (mod->showVerts)					
				{
					mod->NotifyDependents(FOREVER,PART_DISPLAY ,REFMSG_CHANGE);
					mod->ip->RedrawViews(mod->ip->GetTime());
				}
				if (mod->mode == ID_FREEFORMMODE)
				{
					mod->RebuildFreeFormData();
					mod->HitTest(rect,hits,FALSE);
					if ((toggle || subtract) && (mod->freeFormSubMode!=ID_TOOL_SCALE)) return FALSE;
				}
				else
				{
					if (toggle || subtract) return FALSE;
				}
				return subproc(hWnd,msg,point,flags,m);
			} 
			else 
			{					
				region = TRUE;
				lm = om = m;
				XORDottedRect(hWnd,om,m);
			}				
	} else 
	{
		// Second click
		if (region) 
		{
			Rect rect;
			rect.left   = om.x;
			rect.top    = om.y;
			rect.right  = m.x;
			rect.bottom = m.y;
			rect.Rectify();					
			Tab<int> hits;
			theHold.Begin();
			mod->HoldSelection();
			theHold.Accept(_T(GetString(IDS_PW_SELECT_UVW)));
			theHold.Suspend();
			if (!toggle && !subtract) mod->ClearSelect();
			if (mod->HitTest(rect,hits,subtract)) {						
				mod->Select(hits,FALSE,subtract,TRUE);											
			}
			theHold.Resume();

			if (mod->showVerts)					
			{
				mod->NotifyDependents(FOREVER,PART_DISPLAY ,REFMSG_CHANGE);
				mod->ip->RedrawViews(mod->ip->GetTime());
			}

			mod->InvalidateView();
		} 
		else 
		{
			return subproc(hWnd,msg,point,flags,m);
		}
	}
	break;			

case MOUSE_MOVE:

	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		if (region) 
		{
			XORDottedRect(hWnd,om,lm);
			XORDottedRect(hWnd,om,m);
			lm = m;
		} 
		else 
		{
			SetCursor(GetXFormCur());
			return subproc(hWnd,msg,point,flags,m);
		}
	}
	break;
case MOUSE_FREEMOVE: {
	Tab<int> hits;
	Rect rect;
	rect.left = m.x-2;
	rect.right = m.x+2;
	rect.top = m.y-2;
	rect.bottom = m.y+2;

	if ((flags&MOUSE_CTRL) &&  (flags&MOUSE_ALT))
		mod->ip->ReplacePrompt( GetString(IDS_PW_MOUSE_CENTER));
	else if (flags&MOUSE_CTRL)
		mod->ip->ReplacePrompt( GetString(IDS_PW_MOUSE_ADD));
	else if (flags&MOUSE_ALT)
		mod->ip->ReplacePrompt( GetString(IDS_PW_MOUSE_SUBTRACT));
	else if (flags&MOUSE_SHIFT)
		mod->ip->ReplacePrompt( GetString(IDS_PW_MOUSE_CONSTRAIN));
	else mod->ip->ReplacePrompt( GetString(IDS_PW_MOUSE_SELECTTV));



	if (mod->HitTest(rect,hits,FALSE)) 
	{
		if (mod->mode == ID_FREEFORMMODE)
		{
			SetCursor(GetXFormCur());
		}
		else if (mod->fnGetTVSubMode() == TVVERTMODE)
		{
			if (mod->vsel[hits[0]]) 
			{
				SetCursor(GetXFormCur());
			} 
			else 
			{
				SetCursor(mod->selCur);
			}
		}
		else if (mod->fnGetTVSubMode() == TVEDGEMODE)
		{
			if (mod->esel[hits[0]]) 
			{
				SetCursor(GetXFormCur());
			} 
			else 
			{
				SetCursor(mod->selCur);
			}
		}
		else if (mod->fnGetTVSubMode() == TVFACEMODE)
		{
			hits.ZeroCount();
			mod->HitTest(rect,hits,TRUE);

			if (hits.Count() && mod->fsel[hits[0]]) 
			{
				SetCursor(GetXFormCur());
			} 
			else 
			{
				SetCursor(mod->selCur);
			}
		}

	} 
 	else 
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}


	return subproc(hWnd,msg,point,flags,m);
					 }

case MOUSE_ABORT:
	if (region) 
	{
		InvalidateRect(hWnd,NULL,FALSE);
	} 
	else 
	{
		return subproc(hWnd,msg,point,flags,m);
	}
	break;
	}
	return 1;
}

int PaintSelectMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{

	static IPoint2 lastm;
	static int lastRadius;

	switch (msg) {
case MOUSE_POINT:
	if (point==0) {
		// First click

		toggle   = flags&MOUSE_CTRL;
		subtract = flags&MOUSE_ALT;

		// Hit test
		Tab<int> hits;
		Rect rect;
		rect.left   = m.x-mod->fnGetPaintSize();
		rect.top    = m.y-mod->fnGetPaintSize();
		rect.right  = m.x+mod->fnGetPaintSize();
		rect.bottom = m.y+mod->fnGetPaintSize();
		// First hit test sel only
		// Next hit test everything
		//hold the selection
		theHold.Begin();
		theHold.Put (new TSelRestore (mod));

		if (!toggle && !subtract) 
		{
			mod->ClearSelect();
			mod->InvalidateView();
			UpdateWindow(mod->hWnd);
		}

		if (mod->HitTest(rect,hits,subtract)) 
		{
			mod->Select(hits,toggle,subtract,TRUE);
			mod->InvalidateView();
         if (mod->fnGetSyncSelectionMode()) 
         {
            mod->fnSyncGeomSelection();
         }
         else UpdateWindow(mod->hWnd);
		}
		om = m;
	} 
	else 
	{
		mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		mod->ip->RedrawViews(mod->ip->GetTime());
		theHold.Accept(_T(GetString(IDS_PW_SELECT_UVW)));
	}
	break;			

case MOUSE_MOVE:
	{
		float len = Length(om-m);
		if (len > ((float)mod->fnGetPaintSize()*2.0f))
		{
			int ct = (len/(mod->fnGetPaintSize()*2.0f));
			Point2 start, end,vec;
			start.x = (float)om.x;
			start.y = (float)om.y;
			end.x = (float)m.x;
			end.y = (float)m.y;
			vec = (end-start)/ct;
			Point2 current;
			current = start;
			toggle   = flags&MOUSE_CTRL;
			subtract = flags&MOUSE_ALT;
			BOOL redraw = FALSE;
			for (int i =0; i < (ct+1); i++)
			{
				m.x = (int)current.x;
				m.y = (int)current.y;
				Rect rect;
				rect.left   = m.x-mod->fnGetPaintSize();
				rect.top    = m.y-mod->fnGetPaintSize();
				rect.right  = m.x+mod->fnGetPaintSize();
				rect.bottom = m.y+mod->fnGetPaintSize();
				rect.Rectify();					
				Tab<int> hits;
				if (mod->HitTest(rect,hits,subtract)) 
				{										
					mod->Select(hits,FALSE,subtract,TRUE);		
					redraw = TRUE;
				}
				current += vec;
			}	
			if (redraw)
			{
				mod->InvalidateView();
				if (mod->fnGetSyncSelectionMode()) 
				{
					mod->fnSyncGeomSelection();
				}
				else UpdateWindow(mod->hWnd);
			}
		}
		else
		{
			Rect rect;
			rect.left   = m.x-mod->fnGetPaintSize();
			rect.top    = m.y-mod->fnGetPaintSize();
			rect.right  = m.x+mod->fnGetPaintSize();
			rect.bottom = m.y+mod->fnGetPaintSize();
			rect.Rectify();					
			Tab<int> hits;
			toggle   = flags&MOUSE_CTRL;
			subtract = flags&MOUSE_ALT;
			if (mod->HitTest(rect,hits,subtract)) 
			{										
				mod->Select(hits,FALSE,subtract,TRUE);		
				mod->InvalidateView();
				UpdateWindow(mod->hWnd);
			}
			mod->fnSyncGeomSelection();
		}

		om = m;

		IPoint2 r = lastm;
		if (!first)
		{				
			r.x += lastRadius;
			XORDottedCircle(hWnd, lastm,r);
		}
		first = FALSE;

		r = m;
		r.x += mod->fnGetPaintSize();
		XORDottedCircle(hWnd, m,r);

		lastm = m;
		lastRadius = mod->fnGetPaintSize();


//		mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
//		mod->ip->RedrawViews(mod->ip->GetTime());

	}
	break;
case MOUSE_FREEMOVE: 
	{
		IPoint2 r = lastm;
		if (!first)
		{				
			r.x += lastRadius;
			XORDottedCircle(hWnd, lastm,r);
		}
		first = FALSE;

		r = m;
		r.x += mod->fnGetPaintSize();
		XORDottedCircle(hWnd, m,r);

		lastm = m;
		lastRadius = mod->fnGetPaintSize();;			
	}

case MOUSE_ABORT:
	{
		//cancel the hold
		theHold.Restore();
		theHold.Cancel();
		break;
	}
	}
	return 1;
}


int MoveMode::subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	switch (msg) {
case MOUSE_POINT:
	if (point==0) 
	{
//VSNAP
		mod->BuildSnapBuffer();

		theHold.SuperBegin();
		mod->PlugControllers();
		theHold.Begin();
		mod->HoldPoints();
		om = m;
		//convert our sub selection type to vertex selection
		mod->tempVert = Point3(0.0f,0.0f,0.0f);
	} 
	else 
	{

		macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap2.MoveSelected"), 1, 0,
			mr_point3,&mod->tempVert);

		if (mod->tempVert == Point3(0.0f,0.0f,0.0f))
		{
			theHold.Cancel();
			theHold.SuperCancel();
		}
		else
		{
			theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));
			theHold.SuperAccept(_T(GetString(IDS_PW_MOVE_UVW)));
		}


		mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);

		mod->ip->RedrawViews(mod->ip->GetTime());
	}
	break;

case MOUSE_MOVE: {
	theHold.Restore();
	float xzoom, yzoom;
	int width, height;
	IPoint2 delta = m-om;
	if (flags&MOUSE_SHIFT && mod->move==0) {
		if (abs(delta.x) > abs(delta.y)) delta.y = 0;
		else delta.x = 0;
	} else if (mod->move==1) {
		delta.y = 0;
	} else if (mod->move==2) {
		delta.x = 0;
	}
	mod->ComputeZooms(hWnd,xzoom,yzoom,width,height);
	Point2 mv;
	mv.x = delta.x/xzoom;
	mv.y = -delta.y/yzoom;
	//check if moving points or gizmo

	mod->tempVert.x = mv.x;
	mod->tempVert.y = mv.y;
	mod->tempVert.z = 0.0f;

	mod->TransferSelectionStart();
	mod->MovePoints(mv);
	mod->TransferSelectionEnd(FALSE,FALSE);



	if (mod->update) mod->ip->RedrawViews(mod->ip->GetTime());
	UpdateWindow(hWnd);
	break;		
				 }


case MOUSE_ABORT:



		theHold.Cancel();
		theHold.SuperCancel();
	
	mod->ip->RedrawViews(mod->ip->GetTime());
	mod->InvalidateView();
	break;
	}
	return 1;
}

#define ZOOM_FACT	0.01f
#define ROT_FACT	DegToRad(0.5f)

int RotateMode::subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	switch (msg) {
case MOUSE_POINT:
	if (point==0) {
		theHold.SuperBegin();
		mod->PlugControllers();
		theHold.Begin();
		mod->HoldPoints();
		mod->center.x = (float) m.x;
		mod->center.y = (float) m.y;
		mod->tempCenter.x = mod->center.x;
		mod->tempCenter.y = mod->center.y;
		//				mod->tempCenter.z = 0.0f;
		om = m;


	} else {

		float angle = float(m.y-om.y)*ROT_FACT;
		mod->tempHwnd = hWnd;
		if (mod->centeron)
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap2.RotateSelected"), 2, 0,
			mr_float,angle,
			mr_point3,&mod->axisCenter);
		else macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap2.RotateSelectedCenter"), 1, 0,
			mr_float,angle);
		macroRecorder->EmitScript();

		if (angle == 0.0f)
		{
			theHold.Cancel();
			theHold.SuperCancel();
		}
		else
		{
			theHold.Accept(_T(GetString(IDS_PW_ROTATE_UVW)));
			theHold.SuperAccept(_T(GetString(IDS_PW_ROTATE_UVW)));
		}
		mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
		mod->ip->RedrawViews(mod->ip->GetTime());
	}
	break;

case MOUSE_MOVE:
	theHold.Restore();

	//convert our sub selection type to vertex selection
	mod->TransferSelectionStart();

	mod->RotatePoints(hWnd,float(m.y-om.y)*ROT_FACT);
	mod->TransferSelectionEnd(FALSE,FALSE);

	if (mod->update) mod->ip->RedrawViews(mod->ip->GetTime());
	UpdateWindow(hWnd);
	break;		

case MOUSE_ABORT:

	theHold.Cancel();
	theHold.SuperCancel();
	mod->ip->RedrawViews(mod->ip->GetTime());
	mod->InvalidateView();
	break;
	}
	return 1;
}

int ScaleMode::subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{

	switch (msg) {
case MOUSE_POINT:
	if (point==0) {
		theHold.SuperBegin();
		mod->PlugControllers();
		theHold.Begin();
		mod->HoldPoints();
		mod->center.x = (float) m.x;
		mod->center.y = (float) m.y;
		om = m;
		mod->tempCenter.x = mod->center.x;
		mod->tempCenter.y = mod->center.y;
		mod->tempAmount = 1.0f;



	} else {
 		mod->tempHwnd = hWnd;
		if (mod->centeron)
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap2.ScaleSelected"), 3, 0,
			mr_float,mod->tempAmount,
			mr_int,mod->tempDir,
			mr_point3,&mod->tempCenter);
		else macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap2.ScaleSelectedCenter"), 2, 0,
			mr_float,mod->tempAmount,
			mr_int,mod->tempDir);
		macroRecorder->EmitScript();

		if (mod->tempAmount == 1.0f)
		{
			theHold.Cancel();
			theHold.SuperCancel();
		}
		else
		{
			theHold.Accept(_T(GetString(IDS_PW_SCALE_UVW)));
			theHold.SuperAccept(_T(GetString(IDS_PW_SCALE_UVW)));
		}
		mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
		mod->ip->RedrawViews(mod->ip->GetTime());
	}
	break;

case MOUSE_MOVE: {
	theHold.Restore();
	IPoint2 delta = om-m;
	int direction = 0;
	if (flags&MOUSE_SHIFT ){
		if (abs(delta.x) > abs(delta.y)) 
		{
			delta.y = 0;
			direction = 1;
		}
		else 
		{	
			delta.x = 0;
			direction = 2;
		}
	}
	else if (mod->scale > 0)
	{
		if (mod->scale == 1) 
		{
			delta.y = 0;
			direction = 1;
		}
		else if (mod->scale == 2) 
		{	
			delta.x = 0;
			direction = 2;
		}

	}

	float z;
	if (direction == 0)
	{
		if (delta.y<0)
			z = (1.0f/(1.0f-ZOOM_FACT*delta.y));
		else z = (1.0f+ZOOM_FACT*delta.y);
	}
	else if (direction == 1)
	{
		if (delta.x<0)
			z = (1.0f/(1.0f-ZOOM_FACT*delta.x));
		else z = (1.0f+ZOOM_FACT*delta.x);

	}
	else if (direction == 2)
	{
		if (delta.y<0)
			z = (1.0f/(1.0f-ZOOM_FACT*delta.y));
		else z = (1.0f+ZOOM_FACT*delta.y);
	}
	//Rect rect;
	//GetWindowRect(hWnd,&rect);

	if (mod->ip)
		z = mod->ip->SnapPercent(z);

	mod->tempDir = direction;
	mod->tempAmount = z;


	mod->TransferSelectionStart();
	mod->ScalePoints(hWnd, z,direction);
	mod->TransferSelectionEnd(FALSE,FALSE);

	if (mod->update) mod->ip->RedrawViews(mod->ip->GetTime());
	UpdateWindow(hWnd);
	break;
				 }

case MOUSE_ABORT:

	theHold.Cancel();
	theHold.SuperCancel();
	mod->ip->RedrawViews(mod->ip->GetTime());
	mod->InvalidateView();
	break;
	}
	return 1;
}

int FreeFormMode::subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	static float tempXScale = 0.0f;
	static float tempYScale = 0.0f;

	static float xLength = 0.0f;
	static float yLength = 0.0f;

	static float angle = 0.0f;

	static float tempCenterX = 0.0f;
	static float tempCenterY = 0.0f;

	static float tempXLength = 0.0f;
	static float tempYLength = 0.0f;


	static float xAltLength = 0.0f;
	static float yAltLength = 0.0f;

	static float xScreenPivot = 0.0f;
	static float yScreenPivot = 0.0f;


	switch (msg) {
case MOUSE_POINT:
	if (point==0) 
	{
		mod->tempVert = Point3(0.0f,0.0f,0.0f);
		dragging = TRUE;
		if (mod->freeFormSubMode == ID_TOOL_MOVE)
		{
//VSNAP
			mod->BuildSnapBuffer();

			theHold.SuperBegin();
			mod->PlugControllers();
			theHold.Begin();
			mod->HoldPoints();
			om = m;
		}
		else if (mod->freeFormSubMode == ID_TOOL_ROTATE)
		{
			mod->inRotation = TRUE;
			theHold.SuperBegin();
			mod->PlugControllers();
			theHold.Begin();


			om = m;
			mod->center.x =  mod->freeFormPivotScreenSpace.x;
			mod->center.y =  mod->freeFormPivotScreenSpace.y;

			mod->tempCenter.x = mod->center.x;
			mod->tempCenter.y = mod->center.y;
			mod->centeron = TRUE;
			mod->origSelCenter = mod->selCenter;
		}
		else if (mod->freeFormSubMode == ID_TOOL_SCALE)
		{
			theHold.SuperBegin();
			mod->PlugControllers();
			theHold.Begin();
			theHold.Put(new UnwrapPivotRestore(mod));
			float cx = mod->freeFormCornersScreenSpace[mod->scaleCornerOpposite].x;
			float cy = mod->freeFormCornersScreenSpace[mod->scaleCornerOpposite].y;
			float tx = mod->freeFormCornersScreenSpace[mod->scaleCorner].x;
			float ty = mod->freeFormCornersScreenSpace[mod->scaleCorner].y;
			tempXLength = tx -cx;
			tempYLength = ty -cy;

			xAltLength = tx-mod->freeFormPivotScreenSpace.x;
			yAltLength = ty-mod->freeFormPivotScreenSpace.y;

			xScreenPivot = mod->freeFormPivotScreenSpace.x;
			yScreenPivot = mod->freeFormPivotScreenSpace.y;


			mod->center.x =  cx;
			mod->center.y =  cy;
			tempCenterX = cx;
			tempCenterY = cy;
			om = m;
			mod->tempCenter.x = mod->center.x;
			mod->tempCenter.y = mod->center.y;
			Point3 originalPt = mod->selCenter + mod->freeFormPivotOffset;
		}
		else if (mod->freeFormSubMode == ID_TOOL_MOVEPIVOT)
		{
			theHold.Begin();
			theHold.Put(new UnwrapPivotRestore(mod));
			om = m;
			mod->origSelCenter = mod->selCenter;
		}


	} 
	else 
	{
		mod->inRotation = FALSE;
		dragging = FALSE;

		if (mod->freeFormSubMode == ID_TOOL_MOVE)
		{

			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap2.MoveSelected"), 1, 0,
				mr_point3,&mod->tempVert);

			if (mod->tempVert == Point3(0.0f,0.0f,0.0f))
			{
				theHold.Cancel();
				theHold.SuperCancel();
			}
			else
			{
				theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));
				theHold.SuperAccept(_T(GetString(IDS_PW_MOVE_UVW)));
			}
		}
		else if (mod->freeFormSubMode == ID_TOOL_SCALE)
		{

			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap2.ScaleSelectedXY"), 3, 0,
				mr_float,tempXScale,
				mr_float,tempYScale,
				mr_point3,&mod->axisCenter);

			theHold.Accept(_T(GetString(IDS_PW_SCALE_UVW)));
			theHold.SuperAccept(_T(GetString(IDS_PW_SCALE_UVW)));
		}
		else if (mod->freeFormSubMode == ID_TOOL_ROTATE)
		{

			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap2.RotateSelected"), 2, 0,
				mr_float,angle,
				mr_point3,&mod->axisCenter);

			if (angle == 0.0f)
			{
				theHold.Cancel();
				theHold.SuperCancel();
			}
			else
			{

				theHold.Accept(_T(GetString(IDS_PW_ROTATE_UVW)));
				theHold.SuperAccept(_T(GetString(IDS_PW_ROTATE_UVW)));
			}
			//recompute pivot point
			int vselCount = mod->vsel.GetSize();
			Box3 bounds;
			bounds.Init();
			int i1,i2;
			mod->GetUVWIndices(i1,i2);
			for (int i = 0; i < vselCount; i++)
			{
				if (mod->vsel[i])
				{
					//get bounds
					Point3 p = Point3(0.0f,0.0f,0.0f);
					p[i1] = mod->TVMaps.v[i].p[i1];
					p[i2] = mod->TVMaps.v[i].p[i2];

					//							p.z = 0.0f;
					bounds += p;
				}
			}
			Point3 originalPt = (mod->selCenter+mod->freeFormPivotOffset);
			mod->freeFormPivotOffset = originalPt - bounds.Center();

		}
		else if (mod->freeFormSubMode == ID_TOOL_MOVEPIVOT)
		{


			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setPivotOffset"), 1, 0,
				mr_point3,&mod->freeFormPivotOffset);

			theHold.Accept(_T(GetString(IDS_PW_PIVOTRESTORE)));
		}
		mod->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
		mod->ip->RedrawViews(mod->ip->GetTime());
		mod->InvalidateView();
	}
	break;

case MOUSE_MOVE: {

	if (mod->freeFormSubMode == ID_TOOL_MOVE)
	{
		theHold.Restore();

		float xzoom, yzoom;
		int width, height;
		IPoint2 delta = m-om;
		if (flags&MOUSE_SHIFT && mod->move==0) {
			if (abs(delta.x) > abs(delta.y)) delta.y = 0;
			else delta.x = 0;
		} else if (mod->move==1) {
			delta.y = 0;
		} else if (mod->move==2) {
			delta.x = 0;
		}
		mod->ComputeZooms(hWnd,xzoom,yzoom,width,height);
		Point2 mv;
		mv.x = delta.x/xzoom;
		mv.y = -delta.y/yzoom;
		//check if moving points or gizmo

		mod->tempVert.x = mv.x;
		mod->tempVert.y = mv.y;
		mod->tempVert.z = 0.0f;

		//convert our sub selection type to vertex selection
		mod->TransferSelectionStart();

		mod->MovePoints(mv);

		//convert our sub selection type to current selection
		mod->TransferSelectionEnd(FALSE,FALSE);


	}

	else if (mod->freeFormSubMode == ID_TOOL_ROTATE)
	{
		theHold.Restore();
		Point3 vecA, vecB;
		Point3 a,b,center;
		a.x = (float) om.x;
		a.y = (float) om.y;
		a.z = 0.f;

		b.x = (float) m.x;
		b.y = (float) m.y;
		b.z = 0.0f;
		center.x = mod->freeFormPivotScreenSpace.x;
		center.y = mod->freeFormPivotScreenSpace.y;
		center.z = 0.0f;

		vecA = Normalize(a - center);
		vecB = Normalize(b - center);
		Point3 cross = CrossProd(vecA,vecB);
		if (cross.z < 0.0f)
			angle = acos(DotProd(vecA,vecB));
		else angle = -acos(DotProd(vecA,vecB));

		if (flags&MOUSE_ALT )
		{
			angle = floor(angle * 180.0f/PI) * PI/180.0f;
		}
		else if (flags&MOUSE_CTRL )
		{
			int iangle = (int) floor(angle * 180.0f/PI);
			int addOffset = iangle % 5;
			angle = (float)(iangle - addOffset) * PI/180.0f;
		}

		int i1,i2;
		mod->GetUVWIndices(i1,i2);
		if ((i1==0) && (i2==2)) angle *= -1.0f;
		//convert our sub selection type to vertex selection
		mod->TransferSelectionStart();

		mod->RotatePoints(hWnd,angle);
		//convert our sub selection type to current selection
		mod->TransferSelectionEnd(FALSE,FALSE);

	}
	else if (mod->freeFormSubMode == ID_TOOL_SCALE)
	{
		theHold.Restore();
		IPoint2 delta = om-m;
		int direction = 0;
		float xScale = 1.0f, yScale = 1.0f;

		if (flags&MOUSE_ALT )
		{
			xLength = xAltLength;
			yLength = yAltLength;
		}
		else
		{
			xLength = tempXLength;
			yLength = tempYLength;
		}

		if (flags&MOUSE_SHIFT ){
			if (abs(delta.x) > abs(delta.y)) 
			{
				delta.y = 0;
				direction = 1;
			}
			else 
			{	
				delta.x = 0;
				direction = 2;
			}
		}


		if (direction == 0)
		{
			if (xLength != 0.0f)
				xScale = 1.0f- (delta.x/xLength);
			else xScale = 0.0f;
			if (yLength != 0.0f)
				yScale = 1.0f- (delta.y/yLength);
			else yScale = 0.0f;
		}
		else if (direction == 1)
		{
			if (xLength != 0.0f)					
				xScale = 1.0f- (delta.x/xLength);
			else xScale = 0.0f;

		}
		else if (direction == 2)
		{
			if (yLength != 0.0f)
				yScale = 1.0f- (delta.y/yLength);
			else yScale = 0.0f;
		}
		if (flags&MOUSE_CTRL )
		{
			if (yScale > xScale)
				xScale = yScale;
			else yScale = xScale;
		}

		if (flags&MOUSE_ALT )
		{
			mod->center.x =  xScreenPivot;
			mod->center.y =  yScreenPivot;
		}
		else
		{
			mod->center.x =  tempCenterX;
			mod->center.y =  tempCenterY;
		}


		if (mod->ip)
		{
			xScale = mod->ip->SnapPercent(xScale);
			yScale = mod->ip->SnapPercent(yScale);
		}

		tempXScale = xScale;
		tempYScale = yScale;



		//DebugPrint(" offset %f %f\n",mod->freeFormPivotOffset.x,mod->freeFormPivotOffset.y);

		//convert our sub selection type to vertex selection
		mod->TransferSelectionStart();

		mod->ScalePointsXY(hWnd, xScale,yScale);

		//convert our sub selection type to current selection
		mod->TransferSelectionEnd(FALSE,FALSE);

		mod->freeFormPivotOffset.x *= xScale;
		mod->freeFormPivotOffset.y *= yScale;
	}
	else if (mod->freeFormSubMode == ID_TOOL_MOVEPIVOT)
	{
		theHold.Restore();
		IPoint2 delta = om-m;
		if (flags&MOUSE_SHIFT ) 
		{
			if (abs(delta.x) > abs(delta.y)) 
				delta.y = 0;
			else delta.x = 0;
		}
		float xzoom, yzoom;
		int width, height;				

		mod->ComputeZooms(hWnd,xzoom,yzoom,width,height);
		Point3 mv(0.0f,0.0f,0.0f);
		int i1, i2;
		mod->GetUVWIndices(i1,i2);
		mv[i1] = -delta.x/xzoom;
		mv[i2] = delta.y/yzoom;
		//				mv.z = 0.0f;
		mod->freeFormPivotOffset += mv;

	}

	if (mod->update) mod->ip->RedrawViews(mod->ip->GetTime());
	UpdateWindow(hWnd);
	break;		
				 }


case MOUSE_ABORT:

	mod->inRotation = FALSE;
	dragging = FALSE;
	theHold.Cancel();
	if (mod->freeFormSubMode != ID_TOOL_MOVEPIVOT)
		theHold.SuperCancel();

	mod->ip->RedrawViews(mod->ip->GetTime());
	mod->InvalidateView();
	break;
	}
	return 1;
}



int WeldMode::subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	switch (msg) {
case MOUSE_POINT:
	if (point==0) {
		theHold.SuperBegin();
		mod->PlugControllers();
		theHold.Begin();
		om = m;
		mod->tWeldHit = -1;
	} else {
		if (mod->WeldPoints(hWnd,m))
		{
			theHold.Accept(_T(GetString(IDS_PW_WELD_UVW)));
			theHold.SuperAccept(_T(GetString(IDS_PW_WELD_UVW)));
		}
		else{
			theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));
			theHold.SuperAccept(_T(GetString(IDS_PW_MOVE_UVW)));
		}

		mod->ip->RedrawViews(mod->ip->GetTime());
	}
	break;

case MOUSE_MOVE: {
	theHold.Restore();

	Tab<int> hits;
	Rect rect;
	rect.left = m.x;
	rect.right = m.x;
	rect.top = m.y;
	rect.bottom = m.y;
	mod->tWeldHit = -1;

	if (mod->HitTest(rect,hits,FALSE) )
	{
		BOOL sel = TRUE;
		for (int i =0; i < hits.Count(); i++)
		{
			if (mod->fnGetTVSubMode() == TVVERTMODE) 
			{
				if (!mod->vsel[hits[i]])
					sel = FALSE;
			}
			else if (mod->fnGetTVSubMode() == TVEDGEMODE) 
			{
				if (!mod->esel[hits[i]])
				{
					int edgeCount = mod->TVMaps.ePtrList[hits[i]]->faceList.Count();
					if ( edgeCount ==1)
					{
						sel = FALSE;
						mod->tWeldHit = hits[i];
					}
				}
			}
		}
		if (!sel)
			SetCursor(mod->weldCurHit);
		else SetCursor(mod->weldCur);
	}
	else SetCursor(mod->weldCur);



	float xzoom, yzoom;
	int width, height;
	IPoint2 delta = m-om;
	if (flags&MOUSE_SHIFT && mod->move==0) {
		if (abs(delta.x) > abs(delta.y)) delta.y = 0;
		else delta.x = 0;
	} else if (mod->move==1) {
		delta.y = 0;
	} else if (mod->move==2) {
		delta.x = 0;
	}
	mod->ComputeZooms(hWnd,xzoom,yzoom,width,height);
	Point2 mv;
	mv.x = delta.x/xzoom;
	mv.y = -delta.y/yzoom;

	mod->TransferSelectionStart();
	mod->MovePoints(mv);
	mod->TransferSelectionEnd(FALSE,FALSE);

	if (mod->update) mod->ip->RedrawViews(mod->ip->GetTime());
	UpdateWindow(hWnd);
	break;		
				 }

case MOUSE_ABORT:
	theHold.Cancel();
	theHold.SuperCancel();
	mod->ip->RedrawViews(mod->ip->GetTime());
	break;
	}
	return 1;
}


int PanMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	switch (msg) {
case MOUSE_POINT:
	if (point==0) {
		om = m;
		oxscroll = mod->xscroll;
		oyscroll = mod->yscroll;
	}
	break;

case MOUSE_MOVE: {
	//watje tile
	mod->tileValid = FALSE;

	IPoint2 delta = m-om;
	mod->xscroll = oxscroll + float(delta.x);
	mod->yscroll = oyscroll + float(delta.y);
	mod->InvalidateView();
	SetCursor(GetPanCursor());
	break;
				 }

case MOUSE_ABORT:
	//watje tile
	mod->tileValid = FALSE;

	mod->xscroll = oxscroll;
	mod->yscroll = oyscroll;
	mod->InvalidateView();
	break;

case MOUSE_FREEMOVE:
	SetCursor(GetPanCursor());
	break;		
	}
	return 1;
}

int ZoomMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	switch (msg) {
case MOUSE_POINT:
	if (point==0) {
		om = m;
		ozoom = mod->zoom;
		oxscroll = mod->xscroll;
		oyscroll = mod->yscroll;


	}
	break;

case MOUSE_MOVE: {

	//watje tile
	mod->tileValid = FALSE;

	Rect rect;
	GetClientRect(hWnd,&rect);	
	int centerX = (rect.w()-1)/2-om.x;
	int centerY = (rect.h()-1)/2-om.y;

	//			oxscroll = oxscroll + centerX;
	//			oyscroll = oyscroll + centerY;


	IPoint2 delta = om-m;
	float z;
	if (delta.y<0)
		z = (1.0f/(1.0f-ZOOM_FACT*delta.y));
	else z = (1.0f+ZOOM_FACT*delta.y);
	mod->zoom = ozoom * z;
	mod->xscroll = (oxscroll + centerX)*z;
	mod->yscroll = (oyscroll + centerY)*z;


	mod->xscroll -= centerX;
	mod->yscroll -= centerY;

	mod->InvalidateView();
	SetCursor(mod->zoomCur);

	//			SetCursor(LoadCursor(NULL, IDC_ARROW));
	break;
				 }

case MOUSE_ABORT:
	//watje tile
	mod->tileValid = FALSE;

	mod->zoom = ozoom;
	mod->xscroll = oxscroll;
	mod->yscroll = oyscroll;
	mod->InvalidateView();
	break;

case MOUSE_FREEMOVE:
	SetCursor(mod->zoomCur);

	//			SetCursor(LoadCursor(NULL, IDC_ARROW));
	break;		
	}
	return 1;
}

int ZoomRegMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	switch (msg) {
case MOUSE_POINT:
	if (point==0) {
		lm = om = m;
		XORDottedRect(hWnd,om,lm);
	} else {

		if (om!=m)
		{
			//watje tile
			mod->tileValid = FALSE;

			Rect rect;
			GetClientRect(hWnd,&rect);
			IPoint2 mcent = (om+m)/2;
			IPoint2 scent = rect.GetCenter();
			IPoint2 delta = m-om;
			float rat1, rat2;
			if ((delta.x  != 0) && (delta.y != 0))
			{
				rat1 = float(rect.w()-1)/float(fabs((double)delta.x));
				rat2 = float(rect.h()-1)/float(fabs((double)delta.y));
				float rat = rat1<rat2?rat1:rat2;
				mod->zoom *= rat;
				delta = scent - mcent;
				mod->xscroll += delta.x;
				mod->yscroll += delta.y;
				mod->xscroll *= rat;
				mod->yscroll *= rat;
			}
		}
		mod->InvalidateView();
	}
	break;

case MOUSE_MOVE:


	XORDottedRect(hWnd,om,lm);
	XORDottedRect(hWnd,om,m);
	lm = m;
	SetCursor(mod->zoomRegionCur);
	break;

case MOUSE_ABORT:
	InvalidateRect(hWnd,NULL,FALSE);
	break;

case MOUSE_FREEMOVE:
	SetCursor(mod->zoomRegionCur);
	break;		
	}
	return 1;
}

int RightMouseMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	switch (msg) {
case MOUSE_POINT:
case MOUSE_PROPCLICK:
	if ((mod->mode == ID_SKETCHMODE) && (mod->sketchSelMode == SKETCH_SELPICK) && (mod->vsel.NumberSet() >0))
	{

		mod->sketchSelMode = SKETCH_DRAWMODE;
		if (mod->sketchType == SKETCH_LINE)
			mod->ip->ReplacePrompt( GetString(IDS_PW_SKETCHPROMPT_LINE));
		else if (mod->sketchType == SKETCH_FREEFORM)
		{
			mod->ip->ReplacePrompt( GetString(IDS_PW_SKETCHPROMPT_FREEFORM));
		}
		else if (mod->sketchType == SKETCH_BOX)
		{
			mod->ip->ReplacePrompt( GetString(IDS_PW_SKETCHPROMPT_BOX));
		}
		else if (mod->sketchType == SKETCH_CIRCLE)
		{
			mod->ip->ReplacePrompt( GetString(IDS_PW_SKETCHPROMPT_CIRCLE));
		}

	}
	//check if in pan zoom or soom region mode
	else if ( (mod->mode == ID_UNWRAP_PAN) ||
		(mod->mode == ID_TOOL_PAN) ||
		(mod->mode == ID_UNWRAP_ZOOM) ||
		(mod->mode == ID_TOOL_ZOOM) ||
		(mod->mode == ID_UNWRAP_ZOOMREGION) ||
		(mod->mode == ID_TOOL_ZOOMREG) ||
		(mod->mode == ID_UNWRAP_WELD) ||
		(mod->mode == ID_TOOL_WELD) ||
		(mod->mode == ID_SKETCHMODE) ||
		(mod->mode == ID_PAINTSELECTMODE)
		)
	{
		if (!( (mod->oldMode == ID_UNWRAP_PAN) ||
			(mod->oldMode == ID_TOOL_PAN) ||
			(mod->oldMode == ID_UNWRAP_ZOOM) ||
			(mod->oldMode == ID_TOOL_ZOOM) ||
			(mod->oldMode == ID_UNWRAP_ZOOMREGION) ||
			(mod->oldMode == ID_TOOL_ZOOMREG) ||
			(mod->oldMode == ID_UNWRAP_WELD) ||
			(mod->oldMode == ID_TOOL_WELD)  ||
			(mod->oldMode == ID_SKETCHMODE) ||
			(mod->oldMode == ID_PAINTSELECTMODE)
			))
		{
			mod->SetMode(mod->oldMode);
		}
		else 
		{
			IMenuContext *pContext = GetCOREInterface()->GetMenuManager()->GetContext(kIUVWUnwrapQuad);
			DbgAssert(pContext);
			DbgAssert(pContext->GetType() == kMenuContextQuadMenu);
			IQuadMenuContext *pQMContext = (IQuadMenuContext *)pContext;
			int curIndex = pQMContext->GetCurrentMenuIndex();
			IQuadMenu *pMenu = pQMContext->GetMenu( curIndex );
			DbgAssert(pMenu);
			pMenu->TrackMenu(hWnd, pQMContext->GetShowAllQuads(curIndex));
			//					return TRUE;
			//					mod->TrackRBMenu(hWnd, m.x, m.y);
		}

	}
	else{ 
		IMenuContext *pContext = GetCOREInterface()->GetMenuManager()->GetContext(kIUVWUnwrapQuad);
		DbgAssert(pContext);
		DbgAssert(pContext->GetType() == kMenuContextQuadMenu);
		IQuadMenuContext *pQMContext = (IQuadMenuContext *)pContext;
		int curIndex = pQMContext->GetCurrentMenuIndex();
		IQuadMenu *pMenu = pQMContext->GetMenu( curIndex );
		DbgAssert(pMenu);
		pMenu->TrackMenu(hWnd, pQMContext->GetShowAllQuads(curIndex));
		//				return TRUE;

		//				mod->TrackRBMenu(hWnd, m.x, m.y);
	}

	break;
	}
	return 1;
}

void MiddleMouseMode::ResetInitialParams()
{
	oxscroll = mod->xscroll;
	oyscroll = mod->yscroll;
	ozoom = mod->zoom;
	reset = TRUE;
	//DebugPrint("reset parms\n");
}

int MiddleMouseMode::proc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
{
	static int modeType = 0;
	switch (msg) {

case MOUSE_POINT:
	if (point==0) {

		inDrag = TRUE;
		BOOL ctrl = flags & MOUSE_CTRL;
		BOOL alt = flags & MOUSE_ALT;

		if (ctrl && alt)
		{
			modeType = ID_TOOL_ZOOM;
			ozoom = mod->zoom;
		}
		else modeType = ID_TOOL_PAN;

		om = m;
		oxscroll = mod->xscroll;
		oyscroll = mod->yscroll;
	}
	//tile
	else
	{
		inDrag = FALSE;
		mod->tileValid = FALSE;
		mod->InvalidateView();

	}
	break;

case MOUSE_MOVE: {
	if (reset)
	{
		om = m;
	}
	reset = FALSE;
	//watje tile
	mod->tileValid = FALSE;

	if (modeType == ID_TOOL_PAN)
	{
		IPoint2 delta = m-om;
		mod->xscroll = oxscroll + float(delta.x);
		mod->yscroll = oyscroll + float(delta.y);
		mod->InvalidateView();
		SetCursor(GetPanCursor());
	}
	else if (modeType == ID_TOOL_ZOOM)
	{
		IPoint2 delta = om-m;
		float z;
		if (delta.y<0)
			z = (1.0f/(1.0f-ZOOM_FACT*delta.y));
		else z = (1.0f+ZOOM_FACT*delta.y);
		mod->zoom = ozoom * z;
		mod->xscroll = oxscroll*z;
		mod->yscroll = oyscroll*z;
		mod->InvalidateView();
		SetCursor(mod->zoomCur);
	}
	break;
				 }

case MOUSE_ABORT:
	//watje tile
	mod->tileValid = FALSE;
	inDrag = FALSE;

	mod->xscroll = oxscroll;
	mod->yscroll = oyscroll;
	if (modeType == ID_TOOL_ZOOM)
		mod->zoom = ozoom;
	mod->InvalidateView();
	break;



	/*		case MOUSE_FREEMOVE:
	SetCursor(GetPanCursor());
	break;		
	*/

	/*
	case MOUSE_PROPCLICK:
	//check if in pan zoom or soom region mode
	if ( (mod->mode == ID_UNWRAP_PAN) ||
	(mod->mode == ID_TOOL_PAN) ||
	(mod->mode == ID_UNWRAP_ZOOM) ||
	(mod->mode == ID_TOOL_ZOOM) ||
	(mod->mode == ID_UNWRAP_ZOOMREGION) ||
	(mod->mode == ID_TOOL_ZOOMREG) ||
	(mod->mode == ID_UNWRAP_WELD) ||
	(mod->mode == ID_TOOL_WELD) 
	)
	mod->SetMode(mod->oldMode);
	else mod->TrackRBMenu(hWnd, m.x, m.y);
	break;
	*/
	}
	return 1;
}




static INT_PTR CALLBACK PropDlgProc(
									HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static IColorSwatch *iSelColor, *iLineColor, *iOpenEdgeColor, *iHandleColor, *iGizmoColor, *iSharedColor, *iBackgroundColor;
	static ISpinnerControl *spinW, *spinH;
	static ISpinnerControl *spinThreshold ;

	static ISpinnerControl *spinTileLimit ;
	static ISpinnerControl *spinTileContrast ;

	static ISpinnerControl *spinLimitSoftSel;
	static ISpinnerControl *spinHitSize;
	static ISpinnerControl *spinTickSize;
	//new
	static IColorSwatch *iGridColor;
	static ISpinnerControl *spinGridSize;
	static ISpinnerControl *spinGridStr;
	static BOOL uiDestroyed = FALSE;

   UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);

	static COLORREF selColor,lineColor,openEdgeColor, handleColor,sharedColor,backgroundColor,freeFormColor,gridColor;

	static BOOL update = FALSE;
	static BOOL showVerts = FALSE;
	static BOOL midPixelSnap = FALSE;
	static BOOL useBitmapRes = FALSE;
	static BOOL fnGetTile = FALSE;
	static BOOL fnGetDisplayOpenEdges = FALSE;

	static BOOL fnGetThickOpenEdges = FALSE;
	static BOOL fnGetViewportOpenEdges = FALSE;

	static BOOL fnGetDisplayHiddenEdges = FALSE;
	static BOOL fnGetShowShared = FALSE;
	static BOOL fnGetBrightCenterTile = FALSE;
	static BOOL fnGetBlendToBack = FALSE;
	static BOOL fnGetGridVisible = FALSE;
	static BOOL fnGetLimitSoftSel = FALSE;

	static int rendW = 256;
	static int rendH = 256;
	static int fnGetHitSize = 3;

	static int fnGetTickSize = 2;
	static int fnGetLimitSoftSelRange = 8;
	static int fnGetFillMode = 0;
	static int fnGetTileLimit = 2;

	static float fnGetTileContrast = 0.0f;
	static float weldThreshold = 0.05f;

	static float fnGetGridSize = .2f;
	static float fnGetGridStr = .2f;

	BOOL updateUI = FALSE;

	switch (msg) {
case WM_INITDIALOG: {
	uiDestroyed = FALSE;
	mod = (UnwrapMod*)lParam;
         DLSetWindowLongPtr(hWnd, lParam);

	mod->optionsDialogActive = TRUE;
	mod->hOptionshWnd = hWnd;

	iSelColor = GetIColorSwatch(GetDlgItem(hWnd,IDC_UNWRAP_SELCOLOR), 
		mod->selColor, _T(GetString(IDS_PW_LINECOLOR)));			
	iLineColor = GetIColorSwatch(GetDlgItem(hWnd,IDC_UNWRAP_LINECOLOR), 
		mod->lineColor, _T(GetString(IDS_PW_LINECOLOR)));						


	CheckDlgButton(hWnd,IDC_UNWRAP_CONSTANTUPDATE,mod->update);


	CheckDlgButton(hWnd,IDC_UNWRAP_SELECT_VERTS,mod->showVerts);
	CheckDlgButton(hWnd,IDC_UNWRAP_MIDPIXEL_SNAP,mod->midPixelSnap);

	CheckDlgButton(hWnd,IDC_UNWRAP_USEBITMAPRES,!mod->useBitmapRes);
	Texmap *activeMap = mod->GetActiveMap();
	if (activeMap && (activeMap->ClassID() != Class_ID(BMTEX_CLASS_ID,0) ) )
		EnableWindow(GetDlgItem(hWnd,IDC_UNWRAP_USEBITMAPRES),TRUE);

	spinThreshold = SetupFloatSpinner(
		hWnd,IDC_UNWRAP_WELDTHRESHSPIN,IDC_UNWRAP_WELDTHRESH,
		0.0f,10.0f,mod->weldThreshold);			
	spinW = SetupIntSpinner(
		hWnd,IDC_UNWRAP_WIDTHSPIN,IDC_UNWRAP_WIDTH,
		2,2048,mod->rendW);			
	spinH = SetupIntSpinner(
		hWnd,IDC_UNWRAP_HEIGHTSPIN,IDC_UNWRAP_HEIGHT,
		2,2048,mod->rendH);			

	CheckDlgButton(hWnd,IDC_UNWRAP_TILEMAP,mod->fnGetTile());

	spinTileLimit = SetupIntSpinner(
		hWnd,IDC_UNWRAP_TILELIMITSPIN,IDC_UNWRAP_TILELIMIT,
		0,50,mod->fnGetTileLimit());			
	spinTileContrast = SetupFloatSpinner(
		hWnd,IDC_UNWRAP_TILECONTRASTSPIN,IDC_UNWRAP_TILECONTRAST,
		0.0f,1.0f,mod->fnGetTileContrast());

	CheckDlgButton(hWnd,IDC_UNWRAP_LIMITSOFTSEL,mod->fnGetLimitSoftSel());
	spinLimitSoftSel = SetupIntSpinner(
		hWnd,IDC_UNWRAP_LIMITSOFTSELRANGESPIN,IDC_UNWRAP_LIMITSOFTSELRANGE,
		0,100,mod->fnGetLimitSoftSelRange());



	HWND hFill = GetDlgItem(hWnd,IDC_FILL_COMBO);
	SendMessage(hFill, CB_RESETCONTENT, 0, 0);

	SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_NOFILL));
	SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_SOLID));
	SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_BDIAG));
	SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_CROSS));
	SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_DIAGCROSS));
	SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_FDIAG));
	SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_HORIZONTAL));
	SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_VERTICAL));

	SendMessage(hFill, CB_SETCURSEL, mod->fnGetFillMode()-1, 0L);

	iOpenEdgeColor = GetIColorSwatch(GetDlgItem(hWnd,IDC_UNWRAP_OPENEDGECOLOR), 
		mod->openEdgeColor, _T(GetString(IDS_PW_OPENEDGECOLOR)));
	CheckDlgButton(hWnd,IDC_DISPLAYOPENEDGES_CHECK,mod->fnGetDisplayOpenEdges());

	CheckDlgButton(hWnd,IDC_THICKOPENEDGES_CHECK,mod->fnGetThickOpenEdges());
	CheckDlgButton(hWnd,IDC_VIEWSEAMSCHECK,mod->fnGetViewportOpenEdges());

	CheckDlgButton(hWnd,IDC_UNWRAP_DISPLAYHIDDENEDGES,mod->fnGetDisplayHiddenEdges());
   if (!mod->fnIsMesh())
      EnableWindow(GetDlgItem(hWnd,IDC_UNWRAP_DISPLAYHIDDENEDGES),FALSE);

	iHandleColor = GetIColorSwatch(GetDlgItem(hWnd,IDC_UNWRAP_HANDLECOLOR), 
		mod->handleColor, _T(GetString(IDS_PW_HANDLECOLOR)));

	iGizmoColor = GetIColorSwatch(GetDlgItem(hWnd,IDC_UNWRAP_GIZMOCOLOR), 
		mod->freeFormColor, _T(GetString(IDS_PW_HANDLECOLOR)));

	spinHitSize = SetupIntSpinner(
		hWnd,IDC_UNWRAP_HITSIZESPIN,IDC_UNWRAP_HITSIZE,
		1,10,mod->fnGetHitSize());

	iSharedColor = GetIColorSwatch(GetDlgItem(hWnd,IDC_UNWRAP_SHARECOLOR), 
		mod->sharedColor, _T(GetString(IDS_PW_HANDLECOLOR)));
	CheckDlgButton(hWnd,IDC_SHOWSHARED_CHECK,mod->fnGetShowShared());

	iBackgroundColor = GetIColorSwatch(GetDlgItem(hWnd,IDC_UNWRAP_BACKGROUNDCOLOR), 
		mod->backgroundColor, _T(GetString(IDS_PW_BACKGROUNDCOLOR)));

	CheckDlgButton(hWnd,IDC_UNWRAP_AFFECTCENTERTILE,mod->fnGetBrightCenterTile());
	CheckDlgButton(hWnd,IDC_UNWRAP_BLENDTOBACK,mod->fnGetBlendToBack());

	spinTickSize = SetupIntSpinner(
		hWnd,IDC_UNWRAP_TICKSIZESPIN,IDC_UNWRAP_TICKSIZE,
		1,10,mod->fnGetTickSize());

	//new
	CheckDlgButton(hWnd,IDC_SHOWGRID_CHECK,mod->fnGetGridVisible());
	spinGridSize = SetupFloatSpinner(
		hWnd,IDC_UNWRAP_GRIDSIZESPIN,IDC_UNWRAP_GRIDSIZE,
		0.00001f,1.0f,mod->fnGetGridSize());
	spinGridStr = SetupFloatSpinner(
		hWnd,IDC_UNWRAP_GRIDSTRSPIN,IDC_UNWRAP_GRIDSTR,
		0.0f,0.5f,mod->fnGetGridStr());
	iGridColor = GetIColorSwatch(GetDlgItem(hWnd,IDC_UNWRAP_GRIDCOLOR), 
		mod->gridColor, _T(GetString(IDS_PW_OPENEDGECOLOR)));



	selColor = mod->selColor;
	lineColor = mod->lineColor;
	openEdgeColor = mod->openEdgeColor;
	handleColor = mod->handleColor;
	sharedColor = mod->sharedColor;
	backgroundColor = mod->backgroundColor;
	freeFormColor = mod->freeFormColor;
	gridColor = mod->gridColor;

	update = mod->update;
	showVerts = mod->showVerts;
	midPixelSnap = mod->midPixelSnap;
	useBitmapRes = mod->useBitmapRes;
	fnGetTile = mod->fnGetTile();
	fnGetDisplayOpenEdges = mod->fnGetDisplayOpenEdges();

	fnGetThickOpenEdges = mod->fnGetThickOpenEdges();
	fnGetViewportOpenEdges = mod->fnGetViewportOpenEdges();


	fnGetDisplayHiddenEdges = mod->fnGetDisplayHiddenEdges();
	fnGetShowShared = mod->fnGetShowShared();
	fnGetBrightCenterTile = mod->fnGetBrightCenterTile();
	fnGetBlendToBack = mod->fnGetBlendToBack();
	fnGetGridVisible = mod->fnGetGridVisible();
	fnGetLimitSoftSel = mod->fnGetLimitSoftSel();

	rendW = mod->rendW;
	rendH = mod->rendH;
	fnGetHitSize = mod->fnGetHitSize();

	fnGetTickSize = mod->fnGetTickSize();
	fnGetLimitSoftSelRange = mod->fnGetLimitSoftSelRange();
	fnGetFillMode = mod->fnGetFillMode();
	fnGetTileLimit = mod->fnGetTileLimit();

	fnGetTileContrast = mod->fnGetTileContrast();
	weldThreshold = mod->weldThreshold;

	fnGetGridSize = mod->fnGetGridSize();
	fnGetGridStr = mod->fnGetGridStr();

	CheckDlgButton(hWnd,IDC_UNWRAP_GRID_SNAP,mod->GetGridSnap());
	CheckDlgButton(hWnd,IDC_UNWRAP_VERTEX_SNAP,mod->GetVertexSnap());
	CheckDlgButton(hWnd,IDC_UNWRAP_EDGE_SNAP,mod->GetEdgeSnap());

	CheckDlgButton(hWnd,IDC_UNWRAP_SHOWIMAGEALPHA,mod->GetShowImageAlpha());

	break;
					}

case WM_CUSTEDIT_ENTER:	
case CC_SPINNER_BUTTONUP:
case CC_COLOR_CHANGE:
	{
		updateUI = TRUE;
		break;
	}
case WM_DESTROY:
	{
		mod->optionsDialogActive = FALSE;
		mod->hOptionshWnd = NULL;
		updateUI = FALSE;
		if (!uiDestroyed)
		{
			ReleaseIColorSwatch(iSelColor);
			ReleaseIColorSwatch(iLineColor);
			ReleaseIColorSwatch(iOpenEdgeColor);
			ReleaseIColorSwatch(iHandleColor);
			ReleaseIColorSwatch(iGizmoColor);
			ReleaseIColorSwatch(iSharedColor);
			ReleaseIColorSwatch(iBackgroundColor);

			ReleaseISpinner(spinThreshold);
			ReleaseISpinner(spinW);
			ReleaseISpinner(spinH);

			ReleaseISpinner(spinTileLimit);
			ReleaseISpinner(spinTileContrast);

			ReleaseISpinner(spinLimitSoftSel);
			ReleaseISpinner(spinHitSize);

			//new
			ReleaseISpinner(spinGridSize);
			ReleaseISpinner(spinGridStr);
			ReleaseIColorSwatch(iGridColor);

		}
	}
case WM_COMMAND:
	switch (LOWORD(wParam)) 
	{
	case IDC_FILL_COMBO:
		if ( HIWORD(wParam) == CBN_SELCHANGE ) 
			updateUI = TRUE;
		break;

	case IDC_DISPLAYOPENEDGES_CHECK:
	case IDC_THICKOPENEDGES_CHECK:
	case IDC_VIEWSEAMSCHECK:
	case IDC_SHOWGRID_CHECK:
	case IDC_UNWRAP_USEBITMAPRES:
	case IDC_UNWRAP_TILEMAP:
	case IDC_UNWRAP_CONSTANTUPDATE:
	case IDC_UNWRAP_AFFECTCENTERTILE:
	case IDC_UNWRAP_SELECT_VERTS:
	case IDC_UNWRAP_DISPLAYHIDDENEDGES:
	case IDC_UNWRAP_BLENDTOBACK:
	case IDC_UNWRAP_MIDPIXEL_SNAP:
	case IDC_UNWRAP_LIMITSOFTSEL:
		updateUI = TRUE;
		break;
	case IDOK:
		{
			mod->hOptionshWnd = NULL;
			mod->lineColor = iLineColor->GetColor();
			mod->selColor  = iSelColor->GetColor();					
			mod->update = IsDlgButtonChecked(hWnd,IDC_UNWRAP_CONSTANTUPDATE);
			mod->showVerts = IsDlgButtonChecked(hWnd,IDC_UNWRAP_SELECT_VERTS);
			mod->midPixelSnap = IsDlgButtonChecked(hWnd,IDC_UNWRAP_MIDPIXEL_SNAP);
			//watje 5-3-99
			BOOL oldRes = mod->useBitmapRes;

			mod->SetGridSnap(IsDlgButtonChecked(hWnd,IDC_UNWRAP_GRID_SNAP));
			mod->SetEdgeSnap(IsDlgButtonChecked(hWnd,IDC_UNWRAP_EDGE_SNAP));
			mod->SetVertexSnap(IsDlgButtonChecked(hWnd,IDC_UNWRAP_VERTEX_SNAP));
	
	
			mod->SetShowImageAlpha(IsDlgButtonChecked(hWnd,IDC_UNWRAP_SHOWIMAGEALPHA));
		

			mod->useBitmapRes = !IsDlgButtonChecked(hWnd,IDC_UNWRAP_USEBITMAPRES);
			mod->weldThreshold = spinThreshold->GetFVal();
			mod->rendW = spinW->GetIVal();
			mod->rendH = spinH->GetIVal();
			//watje 5-3-99
			if (mod->rendW!=mod->iw ||
				mod->rendH!=mod->ih || oldRes!=mod->useBitmapRes) {
					mod->SetupImage();
				}
				mod->fnSetTileLimit(spinTileLimit->GetIVal());
				mod->fnSetTileContrast(spinTileContrast->GetFVal());

				mod->fnSetTile(IsDlgButtonChecked(hWnd,IDC_UNWRAP_TILEMAP));


				mod->fnSetLimitSoftSel(IsDlgButtonChecked(hWnd,IDC_UNWRAP_LIMITSOFTSEL));
				mod->fnSetLimitSoftSelRange(spinLimitSoftSel->GetIVal());

				HWND hFill = GetDlgItem(hWnd,IDC_FILL_COMBO);
				mod->fnSetFillMode(SendMessage(hFill, CB_GETCURSEL, 0L, 0L)+1);

				mod->fnSetDisplayOpenEdges(IsDlgButtonChecked(hWnd,IDC_DISPLAYOPENEDGES_CHECK));
				mod->fnSetThickOpenEdges(IsDlgButtonChecked(hWnd,IDC_THICKOPENEDGES_CHECK));
				mod->fnSetViewportOpenEdges(IsDlgButtonChecked(hWnd,IDC_VIEWSEAMSCHECK));

				mod->fnSetDisplayHiddenEdges(IsDlgButtonChecked(hWnd,IDC_UNWRAP_DISPLAYHIDDENEDGES));
				mod->openEdgeColor = iOpenEdgeColor->GetColor();
				mod->handleColor = iHandleColor->GetColor();
				mod->freeFormColor = iGizmoColor->GetColor();
				mod->sharedColor = iSharedColor->GetColor();

				mod->backgroundColor = iBackgroundColor->GetColor();

				mod->fnSetHitSize(spinHitSize->GetIVal());
				mod->fnSetTickSize(spinTickSize->GetIVal());


				mod->fnSetShowShared(IsDlgButtonChecked(hWnd,IDC_SHOWSHARED_CHECK));
				mod->fnSetBrightCenterTile(IsDlgButtonChecked(hWnd,IDC_UNWRAP_AFFECTCENTERTILE));
				mod->fnSetBlendToBack(IsDlgButtonChecked(hWnd,IDC_UNWRAP_BLENDTOBACK));


				if (mod->iBuf) mod->iBuf->SetBkColor(mod->backgroundColor);
				if (mod->iTileBuf) mod->iTileBuf->SetBkColor(mod->backgroundColor);


				//new			
				mod->fnSetGridVisible(IsDlgButtonChecked(hWnd,IDC_SHOWGRID_CHECK));
				mod->fnSetGridSize(spinGridSize->GetFVal());
				mod->fnSetGridStr(spinGridStr->GetFVal());
				mod->gridColor = iGridColor->GetColor();

				mod->tileValid = FALSE;
				mod->RebuildDistCache();
				mod->InvalidateView();

				ReleaseIColorSwatch(iSelColor);
				ReleaseIColorSwatch(iLineColor);
				ReleaseIColorSwatch(iOpenEdgeColor);
				ReleaseIColorSwatch(iHandleColor);
				ReleaseIColorSwatch(iGizmoColor);
				ReleaseIColorSwatch(iSharedColor);
				ReleaseIColorSwatch(iBackgroundColor);

				ReleaseISpinner(spinThreshold);
				ReleaseISpinner(spinW);
				ReleaseISpinner(spinH);

				ReleaseISpinner(spinTileLimit);
				ReleaseISpinner(spinTileContrast);

				ReleaseISpinner(spinLimitSoftSel);
				ReleaseISpinner(spinHitSize);

				//new
				ReleaseISpinner(spinGridSize);
				ReleaseISpinner(spinGridStr);
				ReleaseIColorSwatch(iGridColor);

				mod->optionsDialogActive = FALSE;
				updateUI = FALSE;

				uiDestroyed = TRUE;
				mod->MoveScriptUI();

				EndDialog(hWnd,0);

				//fall through to release controls'
				break;
		}
	case IDCANCEL:

		mod->hOptionshWnd = NULL;
		ReleaseIColorSwatch(iSelColor);
		ReleaseIColorSwatch(iLineColor);
		ReleaseIColorSwatch(iOpenEdgeColor);
		ReleaseIColorSwatch(iHandleColor);
		ReleaseIColorSwatch(iGizmoColor);
		ReleaseIColorSwatch(iSharedColor);
		ReleaseIColorSwatch(iBackgroundColor);

		ReleaseISpinner(spinThreshold);
		ReleaseISpinner(spinW);
		ReleaseISpinner(spinH);

		ReleaseISpinner(spinTileLimit);
		ReleaseISpinner(spinTileContrast);

		ReleaseISpinner(spinLimitSoftSel);
		ReleaseISpinner(spinHitSize);

		//new
		ReleaseISpinner(spinGridSize);
		ReleaseISpinner(spinGridStr);
		ReleaseIColorSwatch(iGridColor);


		mod->selColor = selColor;
		mod->lineColor = lineColor;
		mod->openEdgeColor = openEdgeColor;
		mod->handleColor = handleColor;
		mod->sharedColor = sharedColor;
		mod->backgroundColor = backgroundColor;
		mod->freeFormColor = freeFormColor;
		mod->gridColor = gridColor;

		mod->update = update;
		mod->showVerts = showVerts;
		mod->midPixelSnap = midPixelSnap;
		mod->useBitmapRes = useBitmapRes;
		mod->fnSetTile(fnGetTile);

		mod->fnSetDisplayOpenEdges(fnGetDisplayOpenEdges);

		mod->fnSetThickOpenEdges(fnGetThickOpenEdges);
		mod->fnSetViewportOpenEdges(fnGetViewportOpenEdges);

		mod->fnSetDisplayHiddenEdges(fnGetDisplayHiddenEdges);
		mod->fnSetShowShared(fnGetShowShared);
		mod->fnSetBrightCenterTile(fnGetBrightCenterTile);
		mod->fnSetBlendToBack(fnGetBlendToBack);
		mod->fnSetGridVisible(fnGetGridVisible);
		mod->fnSetLimitSoftSel(fnGetLimitSoftSel);

		mod->rendW = rendW;
		mod->rendH = rendH;
		mod->fnSetHitSize(fnGetHitSize);

		mod->fnSetTickSize(fnGetTickSize);
		mod->fnSetLimitSoftSelRange(fnGetLimitSoftSelRange);
		mod->fnSetFillMode(fnGetFillMode);
		mod->fnSetTileLimit(fnGetTileLimit);

		mod->fnSetTileContrast(fnGetTileContrast);
		mod->weldThreshold = weldThreshold;

		mod->fnSetGridSize(fnGetGridSize);
		mod->fnSetGridStr(fnGetGridStr);

		if (mod->iBuf) mod->iBuf->SetBkColor(mod->backgroundColor);
		if (mod->iTileBuf) mod->iTileBuf->SetBkColor(mod->backgroundColor);

		mod->optionsDialogActive = FALSE;
		updateUI = FALSE;

		mod->tileValid = FALSE;
		mod->RebuildDistCache();
		mod->InvalidateView();

		uiDestroyed = TRUE;

		EndDialog(hWnd,0);
		break;

	case IDC_UNWRAP_DEFAULTS:
		mod->fnLoadDefaults();

		mod->lineColor = RGB(255,255,255);
		mod->selColor  = RGB(255,0,0);
		mod->openEdgeColor = RGB(0,255,0);
		mod->handleColor = RGB(255,255,0);
		mod->freeFormColor = RGB(255,100,50);
		mod->sharedColor = RGB(0,0,255);

		mod->backgroundColor = RGB(60,60,60);

		selColor = mod->selColor;
		lineColor = mod->lineColor;
		openEdgeColor = mod->openEdgeColor;
		handleColor = mod->handleColor;
		sharedColor = mod->sharedColor;
		backgroundColor = mod->backgroundColor;
		freeFormColor = mod->freeFormColor;
		gridColor = mod->gridColor;

		update = mod->update;
		showVerts = mod->showVerts;
		midPixelSnap = mod->midPixelSnap;
		useBitmapRes = mod->useBitmapRes;
		fnGetTile = mod->fnGetTile();
		fnGetDisplayOpenEdges = mod->fnGetDisplayOpenEdges();

		fnGetThickOpenEdges = mod->fnGetThickOpenEdges();
		fnGetViewportOpenEdges = mod->fnGetViewportOpenEdges();


		fnGetDisplayHiddenEdges = mod->fnGetDisplayHiddenEdges();
		fnGetShowShared = mod->fnGetShowShared();
		fnGetBrightCenterTile = mod->fnGetBrightCenterTile();
		fnGetBlendToBack = mod->fnGetBlendToBack();
		fnGetGridVisible = mod->fnGetGridVisible();
		fnGetLimitSoftSel = mod->fnGetLimitSoftSel();

		rendW = mod->rendW;
		rendH = mod->rendH;
		fnGetHitSize = mod->fnGetHitSize();

		fnGetTickSize = mod->fnGetTickSize();
		fnGetLimitSoftSelRange = mod->fnGetLimitSoftSelRange();
		fnGetFillMode = mod->fnGetFillMode();
		fnGetTileLimit = mod->fnGetTileLimit();

		fnGetTileContrast = mod->fnGetTileContrast();
		weldThreshold = mod->weldThreshold;

		fnGetGridSize = mod->fnGetGridSize();
		fnGetGridStr = mod->fnGetGridStr();


		iSelColor->SetColor(mod->selColor);
		iLineColor->SetColor(mod->lineColor);


		CheckDlgButton(hWnd,IDC_UNWRAP_CONSTANTUPDATE,mod->update);


		CheckDlgButton(hWnd,IDC_UNWRAP_SELECT_VERTS,mod->showVerts);
		CheckDlgButton(hWnd,IDC_UNWRAP_MIDPIXEL_SNAP,mod->midPixelSnap);

		CheckDlgButton(hWnd,IDC_UNWRAP_USEBITMAPRES,!mod->useBitmapRes);
		Texmap *activeMap = mod->GetActiveMap();
		if (activeMap && (activeMap->ClassID() != Class_ID(BMTEX_CLASS_ID,0) ) )
			EnableWindow(GetDlgItem(hWnd,IDC_UNWRAP_USEBITMAPRES),TRUE);

		spinThreshold->SetValue(mod->weldThreshold,FALSE);			
		spinW->SetValue(mod->rendW,FALSE);			
		spinH->SetValue(mod->rendH,FALSE);			

		CheckDlgButton(hWnd,IDC_UNWRAP_TILEMAP,mod->fnGetTile());

		spinTileLimit->SetValue(mod->fnGetTileLimit(),FALSE);			
		spinTileContrast->SetValue(mod->fnGetTileContrast(),FALSE);

		CheckDlgButton(hWnd,IDC_UNWRAP_LIMITSOFTSEL,mod->fnGetLimitSoftSel());
		spinLimitSoftSel->SetValue(mod->fnGetLimitSoftSelRange(),FALSE);



		HWND hFill = GetDlgItem(hWnd,IDC_FILL_COMBO);
		SendMessage(hFill, CB_RESETCONTENT, 0, 0);

		SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_NOFILL));
		SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_SOLID));
		SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_BDIAG));
		SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_CROSS));
		SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_DIAGCROSS));
		SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_FDIAG));
		SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_HORIZONTAL));
		SendMessage(hFill, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) GetString(IDS_PW_FILL_VERTICAL));

		SendMessage(hFill, CB_SETCURSEL, mod->fnGetFillMode()-1, 0L);

		iOpenEdgeColor->SetColor(mod->openEdgeColor);
		CheckDlgButton(hWnd,IDC_DISPLAYOPENEDGES_CHECK,mod->fnGetDisplayOpenEdges());

		CheckDlgButton(hWnd,IDC_THICKOPENEDGES_CHECK,mod->fnGetThickOpenEdges());
		CheckDlgButton(hWnd,IDC_VIEWSEAMSCHECK,mod->fnGetViewportOpenEdges());

		CheckDlgButton(hWnd,IDC_UNWRAP_DISPLAYHIDDENEDGES,mod->fnGetDisplayHiddenEdges());

		iHandleColor->SetColor(mod->handleColor);

		iGizmoColor->SetColor(mod->freeFormColor);

		spinHitSize->SetValue(mod->fnGetHitSize(),FALSE);

		iSharedColor->SetColor(mod->sharedColor);
		CheckDlgButton(hWnd,IDC_SHOWSHARED_CHECK,mod->fnGetShowShared());

		iBackgroundColor->SetColor(mod->backgroundColor);

		CheckDlgButton(hWnd,IDC_UNWRAP_AFFECTCENTERTILE,mod->fnGetBrightCenterTile());
		CheckDlgButton(hWnd,IDC_UNWRAP_BLENDTOBACK,mod->fnGetBlendToBack());

		spinTickSize->SetValue(mod->fnGetTickSize(),FALSE);

		//new
		CheckDlgButton(hWnd,IDC_SHOWGRID_CHECK,mod->fnGetGridVisible());
		spinGridSize->SetValue(mod->fnGetGridSize(),FALSE);
		spinGridStr->SetValue(mod->fnGetGridStr(),FALSE);
		iGridColor->SetColor(mod->gridColor);


		mod->SetGridSnap(TRUE);
		mod->SetGridSnap(TRUE);
		mod->SetGridSnap(TRUE);

		CheckDlgButton(hWnd,IDC_UNWRAP_GRID_SNAP,mod->GetGridSnap());
		CheckDlgButton(hWnd,IDC_UNWRAP_VERTEX_SNAP,mod->GetVertexSnap());
		CheckDlgButton(hWnd,IDC_UNWRAP_EDGE_SNAP,mod->GetEdgeSnap());

		mod->SetShowImageAlpha(FALSE);
		CheckDlgButton(hWnd,IDC_UNWRAP_SHOWIMAGEALPHA,mod->GetShowImageAlpha());

		break;
	}
	break;

default:
	return FALSE;
	} 
	if ((updateUI) && mod->optionsDialogActive)
	{
		mod->lineColor = iLineColor->GetColor();
		mod->selColor  = iSelColor->GetColor();					
		mod->update = IsDlgButtonChecked(hWnd,IDC_UNWRAP_CONSTANTUPDATE);
		mod->showVerts = IsDlgButtonChecked(hWnd,IDC_UNWRAP_SELECT_VERTS);
		mod->midPixelSnap = IsDlgButtonChecked(hWnd,IDC_UNWRAP_MIDPIXEL_SNAP);
		//watje 5-3-99
		BOOL oldRes = mod->useBitmapRes;

		mod->useBitmapRes = !IsDlgButtonChecked(hWnd,IDC_UNWRAP_USEBITMAPRES);
		mod->weldThreshold = spinThreshold->GetFVal();
		mod->rendW = spinW->GetIVal();
		mod->rendH = spinH->GetIVal();
		//watje 5-3-99
		if (mod->rendW!=mod->iw ||
			mod->rendH!=mod->ih || oldRes!=mod->useBitmapRes) {
				mod->SetupImage();
			}
			mod->fnSetTileLimit(spinTileLimit->GetIVal());
			mod->fnSetTileContrast(spinTileContrast->GetFVal());

			mod->fnSetTile(IsDlgButtonChecked(hWnd,IDC_UNWRAP_TILEMAP));


			mod->fnSetLimitSoftSel(IsDlgButtonChecked(hWnd,IDC_UNWRAP_LIMITSOFTSEL));
			mod->fnSetLimitSoftSelRange(spinLimitSoftSel->GetIVal());

			HWND hFill = GetDlgItem(hWnd,IDC_FILL_COMBO);
			mod->fnSetFillMode(SendMessage(hFill, CB_GETCURSEL, 0L, 0L)+1);

			mod->fnSetDisplayOpenEdges(IsDlgButtonChecked(hWnd,IDC_DISPLAYOPENEDGES_CHECK));

			mod->fnSetThickOpenEdges(IsDlgButtonChecked(hWnd,IDC_THICKOPENEDGES_CHECK));
			mod->fnSetViewportOpenEdges(IsDlgButtonChecked(hWnd,IDC_VIEWSEAMSCHECK));

			mod->fnSetDisplayHiddenEdges(IsDlgButtonChecked(hWnd,IDC_UNWRAP_DISPLAYHIDDENEDGES));
			mod->openEdgeColor = iOpenEdgeColor->GetColor();
			mod->handleColor = iHandleColor->GetColor();
			mod->freeFormColor = iGizmoColor->GetColor();
			mod->sharedColor = iSharedColor->GetColor();

			mod->backgroundColor = iBackgroundColor->GetColor();

			mod->fnSetHitSize(spinHitSize->GetIVal());
			mod->fnSetTickSize(spinTickSize->GetIVal());


			mod->fnSetShowShared(IsDlgButtonChecked(hWnd,IDC_SHOWSHARED_CHECK));
			mod->fnSetBrightCenterTile(IsDlgButtonChecked(hWnd,IDC_UNWRAP_AFFECTCENTERTILE));
			mod->fnSetBlendToBack(IsDlgButtonChecked(hWnd,IDC_UNWRAP_BLENDTOBACK));


			if (mod->iBuf) mod->iBuf->SetBkColor(mod->backgroundColor);
			if (mod->iTileBuf) mod->iTileBuf->SetBkColor(mod->backgroundColor);


			//new			
			mod->fnSetGridVisible(IsDlgButtonChecked(hWnd,IDC_SHOWGRID_CHECK));
			mod->fnSetGridSize(spinGridSize->GetFVal());
			mod->fnSetGridStr(spinGridStr->GetFVal());
			mod->gridColor = iGridColor->GetColor();

			mod->tileValid = FALSE;
			mod->RebuildDistCache();
			mod->InvalidateView();
			mod->MoveScriptUI();
	}
	return TRUE;
}

void UnwrapMod::PropDialog() 
{
	if (!optionsDialogActive)
	{
		CreateDialogParam(
			hInstance,
			MAKEINTRESOURCE(IDD_UNWRAP_PROP),
			hWnd,
			PropDlgProc,
			(LONG_PTR)this);
		/*		DialogBoxParam(
		hInstance,
		MAKEINTRESOURCE(IDD_UNWRAP_PROP),
		hWnd,
		PropDlgProc,
		(LONG_PTR)this);
		*/
	}
}


void UnwrapMod::UnwrapMatrixFromNormal(Point3& normal, Matrix3& mat)
{
	Point3 vx;
	vx.z = .0f;
	vx.x = -normal.y;
	vx.y = normal.x;	
	if ( vx.x == .0f && vx.y == .0f ) {
		vx.x = 1.0f;
	}
	mat.SetRow(0,vx);
	mat.SetRow(1,normal^vx);
	mat.SetRow(2,normal);
	mat.SetTrans(Point3(0,0,0));
	mat.NoScale();
}


//--- Named selection sets -----------------------------------------

int UnwrapMod::FindSet(TSTR &setName) 
{
	if  (ip->GetSubObjectLevel() == 1) 
	{
		for (int i=0; i<namedVSel.Count(); i++) 
		{
			if (setName == *namedVSel[i]) return i;
		}
		return -1;
	}
	else if  (ip->GetSubObjectLevel() == 2) 
	{
		for (int i=0; i<namedESel.Count(); i++) 
		{
			if (setName == *namedESel[i]) return i;
		}
		return -1;
	}
	else if  (ip->GetSubObjectLevel() == 3) 
	{
		for (int i=0; i<namedSel.Count(); i++) 
		{
			if (setName == *namedSel[i]) return i;
		}
		return -1;
	}
	return -1;
}

DWORD UnwrapMod::AddSet(TSTR &setName) {
	DWORD id = 0;
	TSTR *name = new TSTR(setName);

	if  (ip->GetSubObjectLevel() == 1) 
	{
		namedVSel.Append(1,&name);
		BOOL found = FALSE;
		while (!found) {
			found = TRUE;
			for (int i=0; i<idsV.Count(); i++) {
				if (idsV[i]!=id) continue;
				id++;
				found = FALSE;
				break;
			}

		}
		idsV.Append(1,&id);
		return id;
	}
	else if  (ip->GetSubObjectLevel() == 2) 
	{
		namedESel.Append(1,&name);
		BOOL found = FALSE;
		while (!found) {
			found = TRUE;
			for (int i=0; i<idsE.Count(); i++) {
				if (idsE[i]!=id) continue;
				id++;
				found = FALSE;
				break;
			}

		}
		idsE.Append(1,&id);
		return id;
	}
	else if  (ip->GetSubObjectLevel() == 3) 
	{
		namedSel.Append(1,&name);
		BOOL found = FALSE;
		while (!found) {
			found = TRUE;
			for (int i=0; i<ids.Count(); i++) {
				if (ids[i]!=id) continue;
				id++;
				found = FALSE;
				break;
			}

		}
		ids.Append(1,&id);
		return id;
	}
	return -1;
}

void UnwrapMod::RemoveSet(TSTR &setName) {
	int i = FindSet(setName);
	if (i<0) return;

	if  (ip->GetSubObjectLevel() == 1) 
	{
		delete namedVSel[i];
		namedVSel.Delete(i,1);
		idsV.Delete(i,1);
	}
	else if  (ip->GetSubObjectLevel() == 2) 
	{
		delete namedESel[i];
		namedESel.Delete(i,1);
		idsE.Delete(i,1);
	}
	else if  (ip->GetSubObjectLevel() == 3) 
	{
		delete namedSel[i];
		namedSel.Delete(i,1);
		ids.Delete(i,1);
	}
}

void UnwrapMod::ClearSetNames() 
{

	if  (ip->GetSubObjectLevel() == 1) 
	{
		for (int j=0; j<namedVSel.Count(); j++) 
		{
			delete namedVSel[j];
			namedVSel[j] = NULL;
		}
	}

	else if  (ip->GetSubObjectLevel() == 2) 
	{
		for (int j=0; j<namedESel.Count(); j++) 
		{
			delete namedESel[j];
			namedESel[j] = NULL;
		}
	}

	else if  (ip->GetSubObjectLevel() == 3) 
	{
		for (int j=0; j<namedSel.Count(); j++) 
		{
			delete namedSel[j];
			namedSel[j] = NULL;
		}
	}
}

void UnwrapMod::ActivateSubSelSet(TSTR &setName) {
	ModContextList mcList;
	INodeTab nodes;
	int index = FindSet (setName);	
	if (index<0 || !ip) return;

	ip->GetModContexts(mcList,nodes);

	for (int i = 0; i < mcList.Count(); i++) 
	{
		MeshTopoData *meshData = (MeshTopoData*)mcList[i]->localData;
		if (!meshData) continue;
		//		if (theHold.Holding() && !meshData->held) theHold.Put(new UnwrapRestore(this,meshData));

		BitArray *set = NULL;

		if  (ip->GetSubObjectLevel() == 1) 
		{

			set = meshData->vselSet.GetSet(idsV[index]);
			if (set) 
			{
				if (set->GetSize()!=gvsel.GetSize()) 
				{
					set->SetSize(gvsel.GetSize(),TRUE);
				}
				gvsel = *set;
				SyncTVToGeomSelection(meshData);
				RebuildDistCache();
				InvalidateView();
				UpdateWindow(hWnd);
			}
		}
		else if  (ip->GetSubObjectLevel() == 2) 
		{

			set = meshData->eselSet.GetSet(idsE[index]);
			if (set) 
			{
				if (set->GetSize()!=gesel.GetSize()) 
				{
					set->SetSize(gesel.GetSize(),TRUE);
				}
				gesel = *set;
				SyncTVToGeomSelection(meshData);
				InvalidateView();
				UpdateWindow(hWnd);
			}
		}
		if  (ip->GetSubObjectLevel() == 3) 
		{

			set = meshData->fselSet.GetSet(ids[index]);
			if (set) 
			{
				if (set->GetSize()!=meshData->faceSel.GetSize()) 
				{
					set->SetSize(meshData->faceSel.GetSize(),TRUE);
				}
				meshData->SetFaceSel (*set, this, ip->GetTime());
				UpdateFaceSelection(*set);
				ComputeSelectedFaceData();
				fnSyncTVSelection();
				InvalidateView();
				UpdateWindow(hWnd);
			}
		}
	}

	nodes.DisposeTemporary();
	LocalDataChanged ();
	ip->RedrawViews(ip->GetTime());
}

void UnwrapMod::NewSetFromCurSel(TSTR &setName) 
{
	ModContextList mcList;
	INodeTab nodes;
	DWORD id = -1;
	int index = FindSet(setName);
	if (index<0) id = AddSet(setName);
	else id = ids[index];

	ip->GetModContexts(mcList,nodes);

	for (int i = 0; i < mcList.Count(); i++) {
		MeshTopoData *meshData = (MeshTopoData*)mcList[i]->localData;
		if (!meshData) continue;

		BitArray *set = NULL;
		if  (ip->GetSubObjectLevel() == 1) 
		{
			if (index>=0 && (set = meshData->vselSet.GetSet(id))) 
			{
				*set = gvsel;
			} else meshData->vselSet.AppendSet(gvsel,id);
		}
		else if  (ip->GetSubObjectLevel() == 2) 
		{
			if (index>=0 && (set = meshData->eselSet.GetSet(id))) 
			{
				*set = gesel;
			} else meshData->eselSet.AppendSet(gesel,id);
		}
		else if  (ip->GetSubObjectLevel() == 3) 
		{
			if (index>=0 && (set = meshData->fselSet.GetSet(id))) 
			{
				*set = meshData->faceSel;
			} else meshData->fselSet.AppendSet(meshData->faceSel,id);
		}
	}

	nodes.DisposeTemporary();
}

void UnwrapMod::RemoveSubSelSet(TSTR &setName) {
	int index = FindSet (setName);
	if (index<0 || !ip) return;		

	ModContextList mcList;
	INodeTab nodes;
	ip->GetModContexts(mcList,nodes);

	DWORD id = ids[index];

	for (int i = 0; i < mcList.Count(); i++) 
	{
		MeshTopoData *meshData = (MeshTopoData*)mcList[i]->localData;
		if (!meshData) continue;		

		//				if (theHold.Holding()) theHold.Put(new DeleteSetRestore(&meshData->fselSet,id));
		if  (ip->GetSubObjectLevel() == 1) 
			meshData->vselSet.RemoveSet(id);
		else if  (ip->GetSubObjectLevel() == 2) 
			meshData->eselSet.RemoveSet(id);
		else if  (ip->GetSubObjectLevel() == 3) 
			meshData->fselSet.RemoveSet(id);
	}

	//	if (theHold.Holding()) theHold.Put(new DeleteSetNameRestore(&(namedSel[nsl]),this,&(ids[nsl]),id));
	RemoveSet (setName);
	ip->ClearCurNamedSelSet();
	nodes.DisposeTemporary();
}

void UnwrapMod::SetupNamedSelDropDown() {


	ip->ClearSubObjectNamedSelSets();
	if  (ip->GetSubObjectLevel() ==1) 
	{
		for (int i=0; i<namedVSel.Count(); i++)
		{
			ip->AppendSubObjectNamedSelSet(*namedVSel[i]);
		}
	}
	if  (ip->GetSubObjectLevel() ==2) 
	{
		for (int i=0; i<namedESel.Count(); i++)
		{
			ip->AppendSubObjectNamedSelSet(*namedESel[i]);
		}
	}	
	if  (ip->GetSubObjectLevel() ==3) 
	{
		for (int i=0; i<namedSel.Count(); i++)
		{
			ip->AppendSubObjectNamedSelSet(*namedSel[i]);
		}
	}
			
/*
		else if  (ip->GetSubObjectLevel() == 2) 
			ip->AppendSubObjectNamedSelSet(*namedESel[i]);
		else if  (ip->GetSubObjectLevel() == 3) 
			ip->AppendSubObjectNamedSelSet(*namedSel[i]);

	}
*/
}

int UnwrapMod::NumNamedSelSets() 
{
	if  (ip->GetSubObjectLevel() ==1) 
		return namedVSel.Count();
	else if  (ip->GetSubObjectLevel() ==2) 
		return namedESel.Count();
	else if  (ip->GetSubObjectLevel() ==3) 
		return namedSel.Count();
	return -1;
}

TSTR UnwrapMod::GetNamedSelSetName(int i) 
{
	if  (ip->GetSubObjectLevel() ==1) 
		return *namedVSel[i];
	if  (ip->GetSubObjectLevel() ==2) 
		return *namedESel[i];
	if  (ip->GetSubObjectLevel() ==3) 
		return *namedSel[i];
	return TSTR(" ");
}


void UnwrapMod::SetNamedSelSetName(int i,TSTR &newName) 
{
	if  (ip->GetSubObjectLevel() ==1)
		*namedVSel[i] = newName;
	if  (ip->GetSubObjectLevel() ==2)
		*namedESel[i] = newName;
	if  (ip->GetSubObjectLevel() ==3)
		*namedSel[i] = newName;
}

void UnwrapMod::NewSetByOperator(TSTR &newName,Tab<int> &sets,int op) {
	ModContextList mcList;
	INodeTab nodes;

	DWORD id = AddSet(newName);
	//	if (theHold.Holding()) theHold.Put(new AppendSetNameRestore(this,&namedSel,&ids));

	BOOL delSet = TRUE;
	ip->GetModContexts(mcList,nodes);
	for (int i = 0; i < mcList.Count(); i++) {
		MeshTopoData *meshData = (MeshTopoData*)mcList[i]->localData;
		if (!meshData) continue;

		BitArray bits;
		GenericNamedSelSetList *setList;

		setList = &meshData->fselSet; break;			


		bits = (*setList)[sets[0]];

		for (int i=1; i<sets.Count(); i++) {
			switch (op) {
case NEWSET_MERGE:
	bits |= (*setList)[sets[i]];
	break;

case NEWSET_INTERSECTION:
	bits &= (*setList)[sets[i]];
	break;

case NEWSET_SUBTRACT:
	bits &= ~((*setList)[sets[i]]);
	break;
			}
		}
		if (bits.NumberSet()) delSet = FALSE;

		setList->AppendSet(bits,id);
		//		if (theHold.Holding()) theHold.Put(new AppendSetRestore(setList));
	}
	if (delSet) RemoveSubSelSet(newName);
}

void UnwrapMod::LocalDataChanged() {
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip && editMod==this) {
		//	SetNumSelLabel();
		ip->ClearCurNamedSelSet();
	}
}

void UnwrapMod::SetNumSelLabel() {	
	TSTR buf;
	int num = 0, which;

	if (!hParams) return;

	ModContextList mcList;
	INodeTab nodes;

	ip->GetModContexts(mcList,nodes);
	for (int i = 0; i < mcList.Count(); i++) {
		MeshTopoData *meshData = (MeshTopoData*)mcList[i]->localData;
		if (!meshData) continue;

		num += meshData->faceSel.NumberSet();
		if (meshData->faceSel.NumberSet() == 1) {
			for (which=0; which<meshData->faceSel.GetSize(); which++) if (meshData->faceSel[which]) break;
		}
	}

}

int UnwrapMod::NumSubObjTypes() 
{ 
	//return based on where
	return subObjCount;
}

ISubObjType *UnwrapMod::GetSubObjType(int i) 
{	
	static bool initialized = false;
	if(!initialized)
	{
		initialized = true;
		SOT_SelFace.SetName(GetString(IDS_PW_SELECTFACE));
		SOT_SelVerts.SetName(GetString(IDS_PW_SELECTVERTS));
		SOT_SelEdges.SetName(GetString(IDS_PW_SELECTEDGES));
		SOT_SelGizmo.SetName(GetString(IDS_PW_SELECTGIZMO));
		//		SOT_FaceMap.SetName(GetString(IDS_PW_FACEMAP));
		//		SOT_Planar.SetName(GetString(IDS_PW_PLANAR));
	}

	switch(i)
	{
	case 0:
		return &SOT_SelVerts;

	case 1:
		return &SOT_SelEdges;
	case 2:
		return &SOT_SelFace;
	case 3:
		return &SOT_SelGizmo;
	}

	return NULL;
}



//Pelt
void UnwrapMod::HitGeomEdgeData(Tab<UVWHitData> &hitEdges,GraphicsWindow *gw,  HitRegion hr)
{
	hitEdges.ZeroCount();

	gw->setHitRegion(&hr);	
	gw->setRndLimits(( GW_PICK) & ~GW_ILLUM);

	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{
		if (!( TVMaps.gePtrList[i]->flags & FLAG_HIDDENEDGEA))
		{
			int va = TVMaps.gePtrList[i]->avec;
			int vb = TVMaps.gePtrList[i]->bvec;

			if ((va != -1) && (vb != -1))
			{
				int a,b;
				a = TVMaps.gePtrList[i]->a;
				b = TVMaps.gePtrList[i]->b;

				Point3 avec,bvec;
				avec = TVMaps.geomPoints[vb];
				bvec = TVMaps.geomPoints[va];
				Point3 pa,pb;
				pa = TVMaps.geomPoints[a];
            pb = TVMaps.geomPoints[b];

				Spline3D sp;
				SplineKnot ka(KTYPE_BEZIER_CORNER,LTYPE_CURVE,pa,avec,avec);
				SplineKnot kb(KTYPE_BEZIER_CORNER,LTYPE_CURVE,pb,bvec,bvec);
				sp.NewSpline();
				sp.AddKnot(ka);
				sp.AddKnot(kb);
				sp.SetClosed(0);
				sp.InvalidateGeomCache();
				Point3 ip1,ip2;
				Point3 plist[3];
				//										Draw3dEdge(gw,size, plist[0], plist[1], c);
				gw->clearHitCode();
				for (int k = 0; k < 8; k++)
				{
					float per = k/7.0f;
					ip1 = sp.InterpBezier3D(0, per);
					if ( k > 0)
					{
						plist[0] = ip1;
						plist[1] = ip2;
						gw->segment(plist,1);
					}
					ip2 = ip1;
				}
				if (gw->checkHitCode()) 
				{
					UVWHitData d;
					d.index = i;
					d.dist = gw->getHitDistance();
					hitEdges.Append(1,&d,500);
				}

			}
			else
			{

				Point3 plist[3];
				int a,b;
				a = TVMaps.gePtrList[i]->a;
				b = TVMaps.gePtrList[i]->b;

				plist[0] = TVMaps.geomPoints[a];
				plist[1] = TVMaps.geomPoints[b];

				gw->clearHitCode();
				gw->segment(plist,1);
				if (gw->checkHitCode()) 
				{
					UVWHitData d;
					d.index = i;
					d.dist = gw->getHitDistance();
					hitEdges.Append(1,&d,500);
				}
			}
		}
	}

}


void UnwrapMod::HitGeomVertData(Tab<UVWHitData> &hitVerts,GraphicsWindow *gw,  HitRegion hr)
{
	hitVerts.ZeroCount();

	gw->setHitRegion(&hr);	
	gw->setRndLimits(( GW_PICK) & ~GW_ILLUM);

	for (int i = 0; i < TVMaps.geomPoints.Count(); i++)
	{
		Point3 p  = TVMaps.geomPoints[i];

		gw->clearHitCode();

		gw->marker(&p,POINT_MRKR);
		if (gw->checkHitCode()) 
		{
			UVWHitData d;
			d.index = i;
			d.dist = gw->getHitDistance();
			hitVerts.Append(1,&d,500);
		}
	}

}


void UnwrapMod::Move(
					 TimeValue t, Matrix3& partm, Matrix3& tmAxis, 
					 Point3& val, BOOL localOrigin) 
{	

	assert(tmControl);	
	SetXFormPacket pckt(val,partm,tmAxis);
	tmControl->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);
	if (fnGetConstantUpdate())
	{
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			ApplyGizmo();
	}
 
}


void UnwrapMod::Rotate(
		TimeValue t, Matrix3& partm, Matrix3& tmAxis, 
		Quat& val, BOOL localOrigin) 
	{

	assert(tmControl);
	SetXFormPacket pckt(val,localOrigin,partm,tmAxis);
	tmControl->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);	
	if (fnGetConstantUpdate())
	{
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			ApplyGizmo();
	}


	}

void UnwrapMod::Scale(
		TimeValue t, Matrix3& partm, Matrix3& tmAxis, 
		Point3& val, BOOL localOrigin) 
	{

	assert(tmControl);
	SetXFormPacket pckt(val,localOrigin,partm,tmAxis);
	tmControl->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);	

	if (fnGetConstantUpdate())
	{
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			ApplyGizmo();
	}


	}

void UnwrapMod::EnableMapButtons(BOOL enable)
{
	ICustButton *iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_PELT2));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_PLANAR));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_CYLINDRICAL));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_SPHERICAL));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_BOX));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);


	if (mapMapMode != NOMAP)
		enable = FALSE;

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_QMAP));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	HWND hControlHWND = GetDlgItem(hMapParams,IDC_PREVIEW_CHECK);
	EnableWindow(hControlHWND,enable);

	hControlHWND = GetDlgItem(hMapParams,IDC_RADIO1);
	EnableWindow(hControlHWND,enable);
	hControlHWND = GetDlgItem(hMapParams,IDC_RADIO2);
	EnableWindow(hControlHWND,enable);
	hControlHWND = GetDlgItem(hMapParams,IDC_RADIO3);
	EnableWindow(hControlHWND,enable);
	hControlHWND = GetDlgItem(hMapParams,IDC_RADIO4);
	EnableWindow(hControlHWND,enable);


	hControlHWND = GetDlgItem(hMapParams,IDC_NORMALIZEMAP_CHECK2);
	EnableWindow(hControlHWND,enable);

}

void UnwrapMod::EnableAlignButtons(BOOL enable)
{
	if (ip == NULL) return;
	ICustButton *iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_ALIGNX));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_ALIGNY));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_ALIGNZ));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_ALIGNNORMAL));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_FITMAP));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_ALIGNTOVIEW));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_CENTER));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);

	iButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_RESET));
	iButton->Enable(enable);
	ReleaseICustButton(iButton);




}

void UnwrapMod::TransformHoldingFinish(TimeValue t)
{
	if (!fnGetConstantUpdate())
	{
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			ApplyGizmo();
	}
	Matrix3 macroTM = *fnGetGizmoTM();
	macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setGizmoTM"), 1, 0,	mr_matrix3,&macroTM);						

}

void UnwrapMod::EnableFaceSelectionUI(BOOL enable)
{
	if (ip == NULL) return;

	iPlanarThreshold->Enable(enable);
	iMatID->Enable(enable);
	iSG->Enable(enable);


	HWND hControlHWND = GetDlgItem(hSelRollup,IDC_PLANARANGLE_CHECK);
	EnableWindow(hControlHWND,enable);

	ICustButton *iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_SELECTSG));
	iTempButton->Enable(enable);
	ReleaseICustButton(iTempButton);

	iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_SELECTMATID));
	iTempButton->Enable(enable);
	ReleaseICustButton(iTempButton);

	iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_EXPANDTOSEAMS));
	iTempButton->Enable(enable);
	ReleaseICustButton(iTempButton);
}

void UnwrapMod::EnableEdgeSelectionUI(BOOL enable)
{
	ICustButton *iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_RING));
	iTempButton->Enable(enable);
	ReleaseICustButton(iTempButton);

	iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_LOOP));
	iTempButton->Enable(enable);
	ReleaseICustButton(iTempButton);

	iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_SEAMPOINTTOPOINT));
	iTempButton->Enable(enable);
	ReleaseICustButton(iTempButton);


	iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_EDGESELTOSEAMS));
	iTempButton->Enable(enable);
	ReleaseICustButton(iTempButton);

	iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_SEAMSTOEDGESEL));
	iTempButton->Enable(enable);
	ReleaseICustButton(iTempButton);
}

void UnwrapMod::EnableSubSelectionUI(BOOL enable)
{
	ICustButton *iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_SELECTEXPAND));
	iTempButton->Enable(enable);
	ReleaseICustButton(iTempButton);

	iTempButton = GetICustButton(GetDlgItem(hSelRollup, IDC_UNWRAP_SELECTCONTRACT));
	iTempButton->Enable(enable);
	ReleaseICustButton(iTempButton);

	HWND hControlHWND = GetDlgItem(hSelRollup,IDC_IGNOREBACKFACING_CHECK);
	EnableWindow(hControlHWND,enable);

	hControlHWND = GetDlgItem(hSelRollup,IDC_SELECTELEMENT_CHECK);
	EnableWindow(hControlHWND,enable);

	
}

void UnwrapMod::fnSetWindowXOffset(int offset)
{
	xWindowOffset = offset;

	WINDOWPLACEMENT floaterPos;
	floaterPos.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hWnd,&floaterPos);

	if (floaterPos.showCmd == SW_MAXIMIZE)
	{	
		SetWindowPos(hWnd,NULL,0,0,maximizeWidth-2-xWindowOffset,maximizeHeight-yWindowOffset,SWP_NOZORDER );
		SizeDlg();
		MoveScriptUI();
	}

}
void UnwrapMod::fnSetWindowYOffset(int offset)
{
	yWindowOffset = offset;
	WINDOWPLACEMENT floaterPos;
	floaterPos.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hWnd,&floaterPos);

	if (floaterPos.showCmd == SW_MAXIMIZE)
	{	
		SetWindowPos(hWnd,NULL,0,0,maximizeWidth-2-xWindowOffset,maximizeHeight-yWindowOffset,SWP_NOZORDER );
		SizeDlg();
		MoveScriptUI();
	}

}

void UnwrapMod::SetCheckerMapChannel()
{
	Mtl *checkMat = GetCheckerMap();
	if (checkMat)
	{
		Texmap *checkMap = NULL;
		checkMap = (Texmap *) checkMat->GetSubTexmap(1);
		if (checkMap)
		{
			StdUVGen *uvGen = (StdUVGen*)checkMap->GetTheUVGen();
			uvGen->SetMapChannel(channel);
		}

	}
}


BOOL UnwrapMod::GetShowImageAlpha()
{
	BOOL show; 
	pblock->GetValue(unwrap_showimagealpha,0,show,FOREVER);
	return show;
}

void UnwrapMod::SetShowImageAlpha(BOOL show)
{
	pblock->SetValue(unwrap_showimagealpha,0,show);
	SetupImage();
}


BOOL  UnwrapMod::UpdateGeoFaceData(Mesh *mesh)
{

   if (TVMaps.f.Count() == 0) return FALSE;

   int topoChanged = FALSE;
   for (int i = 0; i < TVMaps.f.Count(); i++)
   {
      int deg = TVMaps.f[i]->count;
      int mdeg = 3;

      if (deg == mdeg)
      {
         for (int j = 0; j < deg; j++)
         {
            TVMaps.f[i]->v[j] = mesh->faces[i].v[j];
         }
      }
      else
      {
         topoChanged = TRUE;
      }

   }
   return topoChanged;

}

BOOL  UnwrapMod::UpdateGeoFaceData(PatchMesh *patch)
{

   if (TVMaps.f.Count() == 0) return FALSE;

   int topoChanged = FALSE;
   for (int i = 0; i < TVMaps.f.Count(); i++)
   {
      int deg = TVMaps.f[i]->count;
      int mdeg = 3;
      if ( patch->patches[i].type == PATCH_QUAD)
            mdeg = 4;

      if (deg == mdeg)
      {
         for (int j = 0; j < deg; j++)
         {
               TVMaps.f[i]->v[j] = patch->patches[i].v[j];
               if (TVMaps.f[i]->vecs)
               {
                  if (!(patch->patches[i].flags & PATCH_LINEARMAPPING))
                  {
   //do geometric points
                     int index = patch->patches[i].interior[j];
                     TVMaps.f[i]->vecs->vinteriors[j] = patch->getNumVerts()+index;
                     index = patch->patches[i].vec[j*2];
                     TVMaps.f[i]->vecs->vhandles[j*2] = patch->getNumVerts()+index;
                     index = patch->patches[i].vec[j*2+1];
                     TVMaps.f[i]->vecs->vhandles[j*2+1] = patch->getNumVerts()+index;
                  }
               }
            

         }
      }
      else
      {
         topoChanged = TRUE;
      }

   }
   return topoChanged;

}

BOOL  UnwrapMod::UpdateGeoFaceData(MNMesh *mnMesh)
{

   if (TVMaps.f.Count() == 0) return FALSE;

   int topoChanged = FALSE;
   for (int i = 0; i < TVMaps.f.Count(); i++)
   {
      int deg = TVMaps.f[i]->count;
      int mdeg = 3;
      mdeg = mnMesh->f[i].deg;


      if (deg == mdeg)
      {
         for (int j = 0; j < deg; j++)
         {
            TVMaps.f[i]->v[j] = mnMesh->f[i].vtx[j];
         }
      }
      else
      {
         topoChanged = TRUE;
      }

   }
   return topoChanged;

}

int UnwrapMod::GetQMapAlign()
{
	int align = 0;
	pblock->GetValue(unwrap_qmap_align,0,align,FOREVER);
	return align;
}

void UnwrapMod::SetQMapAlign(int align)
{	
	pblock->SetValue(unwrap_qmap_align,0,align);
}

BOOL UnwrapMod::GetQMapPreview()
{
	BOOL preview = TRUE;
	pblock->GetValue(unwrap_qmap_preview,0,preview,FOREVER);
	return preview;
}
void UnwrapMod::SetQMapPreview(BOOL preview)
{
	pblock->SetValue(unwrap_qmap_preview,0,preview);
}
