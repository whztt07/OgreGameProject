/**********************************************************************
 *<
	FILE:			ProjectionModTypeUVW.cpp
	DESCRIPTION:	Projection Modifier Type UVW
	CREATED BY:		Michael Russo
	HISTORY:		Created 03-23-2004

 *>	Copyright (c) 2004 Discreet, All Rights Reserved.
 **********************************************************************/

#include "ProjectionModTypeUVW.h"
#include "modstack.h"

//=============================================================================
//
//	Class ProjectionModTypeUVW
//
//=============================================================================


//--- ClassDescriptor and class vars ---------------------------------

Interface* ProjectionModTypeUVW::mpIP = NULL;

class ProjectionModTypeUVWClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() { return 0; }
	void *			Create(BOOL loading = FALSE) { return new ProjectionModTypeUVW(!loading);}
	const TCHAR *	ClassName() { return GetString(IDS_PROJECTIONMODTYPEUVW_CLASS_NAME); }
	SClass_ID		SuperClassID() { return REF_TARGET_CLASS_ID; }
	Class_ID		ClassID() { return PROJECTIONMODTYPEUVW_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_PROJECTIONMODTYPEUVW_CATEGORY);}

	const TCHAR*	InternalName() { return _T("ProjectionModTypeUVW"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }		// returns owning module handle
};

static ProjectionModTypeUVWClassDesc theProjectionModTypeUVWClassDesc;
extern ClassDesc2* GetProjectionModTypeUVWDesc() {return &theProjectionModTypeUVWClassDesc;}


class ProjectionModTypeUVWPBAccessor : public PBAccessor
{ 
public:
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		ProjectionModTypeUVW* p = (ProjectionModTypeUVW*)owner;
		if( !p )
			return;
		switch (id) {
			case ProjectionModTypeUVW::pb_name:
				if( p->mpPMod )
					p->mpPMod->UpdateProjectionTypeList();
				break;
			case ProjectionModTypeUVW::pb_sourceMapChannel:
				if( (v.i == VERTEX_CHANNEL_NUM) || ((v.i >= -2) && (v.i <= 100)) )
					p->SetSourceMapChannel(v.i);
				break;
			case ProjectionModTypeUVW::pb_targetMapChannel:
				if( (v.i == VERTEX_CHANNEL_NUM) || ((v.i >= -2) && (v.i <= 100)) )
					p->SetTargetMapChannel(v.i);
				break;
		}
		p->MainPanelUpdateUI();
	}

	void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		ProjectionModTypeUVW* p = (ProjectionModTypeUVW*)owner;
		if( !p )
			return;
		switch (id) {
			case ProjectionModTypeUVW::pb_sourceMapChannel:
				v.i = p->GetSourceMapChannel();
				break;
			case ProjectionModTypeUVW::pb_targetMapChannel:
				v.i = p->GetTargetMapChannel();
				break;
		}
	}
};

static ProjectionModTypeUVWPBAccessor theProjectionModTypeUVW_accessor;

static ParamBlockDesc2 theProjectionModTypeUVWBlockDesc (
	ProjectionModTypeUVW::pb_params, _T("ProjectionModTypeUVW"),  0, &theProjectionModTypeUVWClassDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, 0,

	//rollout
	IDD_PROJECTUVW, IDS_PROJECTIONMODTYPEUVW_CLASS_NAME, 0,0, NULL,

	// Projection Name
	ProjectionModTypeUVW::pb_name,	_T("name"), TYPE_STRING, 0, IDS_PARAM_NAME,
		p_ui,  TYPE_EDITBOX,  IDC_EDIT_NAME,
		p_accessor,		&theProjectionModTypeUVW_accessor,
		end,

	// Holder
	ProjectionModTypeUVW::pb_holderName,	_T("holderName"), TYPE_STRING, 0, IDS_PARAM_HOLDER_NAME,
		p_ui,  TYPE_EDITBOX,  IDC_EDIT_NAME_HOLDER,
		p_accessor,		&theProjectionModTypeUVW_accessor,
		end,
	ProjectionModTypeUVW::pb_holderAlwaysUpdate, _T("alwaysUpdate"), TYPE_BOOL, P_RESET_DEFAULT, IDS_PARAM_HOLDER_ALWAYS_UPDATE,
		p_default, FALSE,
		p_ui, TYPE_SINGLECHEKBOX, IDC_ALWAYS_UPDATE,
		p_accessor,		&theProjectionModTypeUVW_accessor,
		end,
	ProjectionModTypeUVW::pb_holderCreateNew, _T("createNewHolder"), TYPE_BOOL, P_RESET_DEFAULT, IDS_PARAM_HOLDER_CREATE_NEW,
		p_default, FALSE,
		p_ui, TYPE_SINGLECHEKBOX, IDC_CHECK_NEW_HOLDER,
		p_accessor,		&theProjectionModTypeUVW_accessor,
		end,

	// Source
	ProjectionModTypeUVW::pb_sourceMapChannel,	_T("sourceMapChannel"), TYPE_INT, P_TRANSIENT, IDS_PARAM_MAPCHANNEL,
		p_default,	1,
		p_range,	VERTEX_CHANNEL_NUM, 99,
		p_accessor,		&theProjectionModTypeUVW_accessor,
		end,

	// Target
	ProjectionModTypeUVW::pb_targetSameAsSource, _T("sameAsSource"), TYPE_BOOL, P_RESET_DEFAULT, IDS_PARAM_TARGET_SAME_AS_SOURCE,
		p_default, TRUE,
		p_ui, TYPE_SINGLECHEKBOX, IDC_TARGET_SAME_AS_SOURSE,
		p_accessor,		&theProjectionModTypeUVW_accessor,
		end,

	ProjectionModTypeUVW::pb_targetMapChannel,	_T("targetMapChannel"), TYPE_INT, P_TRANSIENT, IDS_PARAM_TARGET_MAPCHANNEL,
		p_default,	1,
		p_range,	VERTEX_CHANNEL_NUM, 99,
		p_accessor,		&theProjectionModTypeUVW_accessor,
		end,

	// Misc
	ProjectionModTypeUVW::pb_projectMaterialIDs, _T("projectMaterialIDs"), TYPE_BOOL, P_RESET_DEFAULT, IDS_PARAM_MATERIAL_IDS,
		p_default, FALSE,
		p_ui, TYPE_SINGLECHEKBOX, IDC_MATERIAL_IDS,
		p_accessor,		&theProjectionModTypeUVW_accessor,
		end,
	ProjectionModTypeUVW::pb_sameTopology, _T("sameTopology"), TYPE_BOOL, P_RESET_DEFAULT, IDS_PARAM_SAME_TOPOLOGY,
		p_default, FALSE,
		p_ui, TYPE_SINGLECHEKBOX, IDC_SAME_TOPOLOGY,
		p_accessor,		&theProjectionModTypeUVW_accessor,
		end,
	ProjectionModTypeUVW::pb_ignoreBackfacing, _T("ignoreBackfacing"), TYPE_BOOL, P_RESET_DEFAULT, IDS_PARAM_IGNORE_BACKFACING,
		p_default, FALSE,
//		p_ui, TYPE_SINGLECHEKBOX, IDC_IGNORE_BACKFACING,
		p_accessor,		&theProjectionModTypeUVW_accessor,
		end,

	end
);

class MainPanelDlgProc : public ParamMap2UserDlgProc {
  public:
	ProjectionModTypeUVW* mpParent;
	BOOL initialized; //set to true after an init dialog message
	MainPanelDlgProc( ProjectionModTypeUVW* parent ) { this->mpParent=parent; initialized=FALSE; }
	void DeleteThis() { delete this; }

	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
		case WM_INITDIALOG: //called after BeginEditParams whenever rollout is displayed
			mpParent->MainPanelInitDialog(hWnd);
			initialized = TRUE;
			break;
		case WM_DESTROY: //called from EndEditParams
			mpParent->MainPanelDestroy(hWnd);
			initialized = FALSE;
			break;
		case CC_SPINNER_CHANGE:
			mpParent->MainPanelUpdateMapChannels();
			mpParent->MainPanelUpdateUI();
			break;
		case WM_COMMAND:
			switch( LOWORD(wParam) ) {
			case IDC_RADIO_SOURCE_MAPCHANNEL: 
			case IDC_RADIO_SOURCE_VERTEXALPHA: 
			case IDC_RADIO_SOURCE_VERTEXILLUM: 
			case IDC_RADIO_SOURCE_VERTEXCOLOR: 
			case IDC_RADIO_SOURCE_VERTEX: 
			case IDC_RADIO_TARGET_MAPCHANNEL: 
			case IDC_RADIO_TARGET_VERTEXALPHA: 
			case IDC_RADIO_TARGET_VERTEXILLUM: 
			case IDC_RADIO_TARGET_VERTEXCOLOR: 
			case IDC_RADIO_TARGET_VERTEX: 
				mpParent->MainPanelUpdateMapChannels();
				mpParent->MainPanelUpdateUI();
				break;
			}
			break;
		default:
			return FALSE;
	  }
	  return TRUE;
	}
};

//--- ProjectionModTypeUVW methods -------------------------------

ProjectionModTypeUVW::ProjectionModTypeUVW(BOOL create)
{
	mpIP = GetCOREInterface();

	mpPMod = NULL;
	mpPBlock = NULL;
	miIndex = 1;

	mhPanel = NULL;
	mbSuspendPanelUpdate = false;
	mbInModifyObject = false;

	miSourceMapChannel = 1;
	miTargetMapChannel = 1;

	mbEnabled = false;
	mbEditing = false;

	mName.printf( GetString(IDS_PROJECTIONMODTYPEUVW_INITIALNAME), 1 );

	mpPBlock = NULL; 
	theProjectionModTypeUVWClassDesc.MakeAutoParamBlocks( this );
	assert( mpPBlock );
}

ProjectionModTypeUVW::~ProjectionModTypeUVW() 
{

}

RefTargetHandle ProjectionModTypeUVW::Clone( RemapDir &remap )
{
	ProjectionModTypeUVW* newmod = new ProjectionModTypeUVW(FALSE);	
	newmod->ReplaceReference(PBLOCK_REF,remap.CloneRef(mpPBlock));

	// russom - August 15, 2005 - 643944
	newmod->miIndex = miIndex;
	newmod->miSourceMapChannel = miSourceMapChannel;
	newmod->miTargetMapChannel = miTargetMapChannel;
	newmod->mName = mName;

	BaseClone(this, newmod, remap);
	return(newmod);
}

#define SOURCE_MAPCHANNEL_CHUNK	0x100
#define TARGET_MAPCHANNEL_CHUNK	0x110

IOResult ProjectionModTypeUVW::Load(ILoad *pILoad)
{
	IOResult res;
	ULONG nb;
	while (IO_OK==(res=pILoad->OpenChunk())) {
		switch(pILoad->CurChunkID())  {
			case SOURCE_MAPCHANNEL_CHUNK:
				res=pILoad->Read( &miSourceMapChannel, sizeof(int), &nb );
				break;
			case TARGET_MAPCHANNEL_CHUNK:
				res=pILoad->Read( &miTargetMapChannel, sizeof(int), &nb );
				break;
		}
		pILoad->CloseChunk();
		if (res!=IO_OK) 
			return res;
	}

	return IO_OK;
}

IOResult ProjectionModTypeUVW::Save(ISave *pISave)
{
	ULONG nb;
	pISave->BeginChunk(SOURCE_MAPCHANNEL_CHUNK);
	pISave->Write( &miSourceMapChannel, sizeof(int), &nb );
	pISave->EndChunk();

	pISave->BeginChunk(TARGET_MAPCHANNEL_CHUNK);
	pISave->Write( &miTargetMapChannel, sizeof(int), &nb );
	pISave->EndChunk();

	return IO_OK;
}

void ProjectionModTypeUVW::BeginEditParams(IObjParam  *ip, ULONG flags,Animatable *prev)
{
	TCHAR *szName = NULL;
	mpPBlock->GetValue(pb_name, 0, szName, FOREVER);
	if( !szName )
		SetInitialName(miIndex);

	theProjectionModTypeUVWClassDesc.BeginEditParams(ip, this, flags, prev);

	ParamMap2UserDlgProc* dlgProc;
	dlgProc = new MainPanelDlgProc(this);
	theProjectionModTypeUVWBlockDesc.SetUserDlgProc( pb_params, dlgProc );
}

void ProjectionModTypeUVW::EndEditParams(IObjParam *ip,ULONG flags,Animatable *next)
{
	theProjectionModTypeUVWClassDesc.EndEditParams(ip, this, flags, next);
}

void ProjectionModTypeUVW::ModifyObject(TimeValue t, ModContext &mc, ObjectState * os, INode *inode, IProjectionModData *pPModData)
{
	if( mbInModifyObject ) 
		return;

	mbInModifyObject = true;

	BOOL bAlwaysUpdate = FALSE;
	if( mpPBlock )
		mpPBlock->GetValue(pb_holderAlwaysUpdate, 0, bAlwaysUpdate, FOREVER);

	if( bAlwaysUpdate && mpPMod ) {
		BitArray sel;
		for( int s=0; s<mpPMod->NumGeomSels(); s++ ) {
			for( int n=0; n<mpPMod->NumGeomSelNodes(s); n++ ) {
				bool bUseSOSel = pPModData->GetGeomSel(s, sel);
				INode *pNodeTarget = mpPMod->GetGeomSelNode(s, n );
				ProjectToTarget( NULL, os, pNodeTarget, bUseSOSel?&sel:NULL, pPModData );
			}
		}
	}
	mbInModifyObject = false;
}


void ProjectionModTypeUVW::SetInitialName(int iIndex)
{
	miIndex = iIndex;
	TSTR strName;
	strName.printf( GetString(IDS_PROJECTIONMODTYPEUVW_INITIALNAME), iIndex );
	SetName( strName.data() );

	strName.printf( GetString(IDS_PROJECTIONHOLDER_INITIALNAME), iIndex );
	if( mpPBlock )
		mpPBlock->SetValue(pb_holderName, 0, strName.data());
}

TCHAR *ProjectionModTypeUVW::GetName()
{
	if( mpPBlock ) {
		TCHAR *szName = NULL;
		mpPBlock->GetValue(pb_name, 0, szName, FOREVER);
		if( szName )
			return szName;
	}

	return mName.data();
}

void ProjectionModTypeUVW::SetName(TCHAR *name) 
{ 
	if( name ) {
		mName = name; 
		if( mpPBlock )
			mpPBlock->SetValue(pb_name, 0, mName);
	}
}

bool ProjectionModTypeUVW::CanProject(Tab<INode*> &tabSourceNodes, int iSelIndex, int iNodeIndex)
{
	if( !mpPMod || (mpPMod->NumObjects() <= 0) )
		return false;

	return true;
}

void ProjectionModTypeUVW::Project(Tab<INode*> &tabSourceNodes, int iSelIndex, int iNodeIndex)
{
	if( !CanProject(tabSourceNodes, iSelIndex, iNodeIndex) )
		return;

	Interface *ip = GetCOREInterface();

	bool bUseSOSel = false;
	BitArray sel;
	for( int i=0; i<tabSourceNodes.Count(); i++ ) {

		if( tabSourceNodes[i] == NULL )
			continue;

		if( iSelIndex == -1 ) {
			// Project to everything
			for( int s=0; s<mpPMod->NumGeomSels(); s++ ) {
				for( int n=0; n<mpPMod->NumGeomSelNodes(s); n++ ) {
					bUseSOSel = GetSOSelData(tabSourceNodes[i], s, sel);
					INode *pNodeTarget = mpPMod->GetGeomSelNode(s, n );
					ProjectToTarget( tabSourceNodes[i], NULL, pNodeTarget, bUseSOSel?&sel:NULL, NULL );
				}
			}
		}
		else if( iNodeIndex == -1 ) {
			// Project to the nodes of a geometry selection.
			for( int n=0; n<mpPMod->NumGeomSelNodes(iSelIndex); n++ ) {
				bUseSOSel = GetSOSelData(tabSourceNodes[i], iSelIndex, sel);
				INode *pNodeTarget = mpPMod->GetGeomSelNode(iSelIndex, n );
				ProjectToTarget( tabSourceNodes[i], NULL, pNodeTarget, bUseSOSel?&sel:NULL, NULL  );
			}
		}
		else {
			// Project to single node
			bUseSOSel = GetSOSelData(tabSourceNodes[i], iSelIndex, sel);
			INode *pNodeTarget = mpPMod->GetGeomSelNode(iSelIndex, iNodeIndex );
			ProjectToTarget( tabSourceNodes[i], NULL, pNodeTarget, bUseSOSel?&sel:NULL, NULL  );
		}
	}
}
void ProjectionModTypeUVW::MainPanelInitDialog( HWND hWnd ) 
{
	mhPanel = hWnd;

	mSpinnerSourceMapChannel = SetupIntSpinner( mhPanel, IDC_MAPCHANNELSPIN, IDC_MAPCHANNEL, 1, 100, (miSourceMapChannel > 0)?miSourceMapChannel:1 );
	mSpinnerTargetMapChannel = SetupIntSpinner( mhPanel, IDC_MAPCHANNELSPIN_TARGET, IDC_MAPCHANNEL_TARGET, 1, 100, (miTargetMapChannel > 0)?miTargetMapChannel:1 );

	MainPanelUpdateUI();
}

void ProjectionModTypeUVW::MainPanelDestroy( HWND hWnd ) 
{
	mhPanel = NULL;
	ReleaseISpinner(mSpinnerSourceMapChannel);
	ReleaseISpinner(mSpinnerTargetMapChannel);
}

void ProjectionModTypeUVW::MainPanelUpdateUI()
{
	if( (mhPanel == NULL) || mbSuspendPanelUpdate ) 
		return;

	mbSuspendPanelUpdate = true;

	// Holder Group
	BOOL bCreateNew = FALSE;
	mpPBlock->GetValue(pb_holderCreateNew, 0, bCreateNew, FOREVER);
	BOOL bSameTopo = FALSE;
	mpPBlock->GetValue(pb_sameTopology, 0, bSameTopo, FOREVER);

	EnableWindow(GetDlgItem(mhPanel,IDC_ALWAYS_UPDATE), !bCreateNew && bSameTopo );

	// Source Group
	CheckDlgButton( mhPanel, IDC_RADIO_SOURCE_MAPCHANNEL, (miSourceMapChannel > 0) );
	CheckDlgButton( mhPanel, IDC_RADIO_SOURCE_VERTEXCOLOR, (miSourceMapChannel == 0) );	
	CheckDlgButton( mhPanel, IDC_RADIO_SOURCE_VERTEXILLUM, (miSourceMapChannel == -1) );
	CheckDlgButton( mhPanel, IDC_RADIO_SOURCE_VERTEXALPHA, (miSourceMapChannel == -2) );
	CheckDlgButton( mhPanel, IDC_RADIO_SOURCE_VERTEX, (miSourceMapChannel == VERTEX_CHANNEL_NUM) );

	mSpinnerSourceMapChannel->Enable( miSourceMapChannel>0 );

	// Target Group
	BOOL bValue = FALSE;
	mpPBlock->GetValue(pb_targetSameAsSource, 0, bValue, FOREVER);
	EnableWindow(GetDlgItem(mhPanel,IDC_RADIO_TARGET_MAPCHANNEL), !bValue);
	EnableWindow(GetDlgItem(mhPanel,IDC_RADIO_TARGET_VERTEXCOLOR), !bValue);
	EnableWindow(GetDlgItem(mhPanel,IDC_RADIO_TARGET_VERTEXILLUM), !bValue);
	EnableWindow(GetDlgItem(mhPanel,IDC_RADIO_TARGET_VERTEXALPHA), !bValue);
	EnableWindow(GetDlgItem(mhPanel,IDC_RADIO_TARGET_VERTEX), !bValue);

	mSpinnerTargetMapChannel->Enable( !bValue );

	if( !bValue ) {
		CheckDlgButton( mhPanel, IDC_RADIO_TARGET_MAPCHANNEL, (miTargetMapChannel > 0) );
		CheckDlgButton( mhPanel, IDC_RADIO_TARGET_VERTEXCOLOR, (miTargetMapChannel == 0) );	
		CheckDlgButton( mhPanel, IDC_RADIO_TARGET_VERTEXILLUM, (miTargetMapChannel == -1) );
		CheckDlgButton( mhPanel, IDC_RADIO_TARGET_VERTEXALPHA, (miTargetMapChannel == -2) );
		CheckDlgButton( mhPanel, IDC_RADIO_TARGET_VERTEX, (miTargetMapChannel == VERTEX_CHANNEL_NUM) );
	}

	mbSuspendPanelUpdate = false;
}

void ProjectionModTypeUVW::MainPanelUpdateMapChannels()
{
	if( mhPanel == NULL ) 
		return;

	// Source Group
	if( IsDlgButtonChecked(mhPanel, IDC_RADIO_SOURCE_MAPCHANNEL) == BST_CHECKED )
		miSourceMapChannel = mSpinnerSourceMapChannel->GetIVal();
	else if( IsDlgButtonChecked(mhPanel, IDC_RADIO_SOURCE_VERTEXCOLOR) == BST_CHECKED )
		miSourceMapChannel = 0;
	else if( IsDlgButtonChecked(mhPanel, IDC_RADIO_SOURCE_VERTEXILLUM) == BST_CHECKED )
		miSourceMapChannel = -1;
	else if( IsDlgButtonChecked(mhPanel, IDC_RADIO_SOURCE_VERTEXALPHA) == BST_CHECKED )
		miSourceMapChannel = -2;
	else if( IsDlgButtonChecked(mhPanel, IDC_RADIO_SOURCE_VERTEX) == BST_CHECKED )
		miSourceMapChannel = VERTEX_CHANNEL_NUM;

	// Target Group
	if( IsDlgButtonChecked(mhPanel, IDC_TARGET_SAME_AS_SOURSE) == BST_CHECKED )
		miTargetMapChannel = miSourceMapChannel;
	else if( IsDlgButtonChecked(mhPanel, IDC_RADIO_TARGET_MAPCHANNEL) == BST_CHECKED )
		miTargetMapChannel = mSpinnerTargetMapChannel->GetIVal();
	else if( IsDlgButtonChecked(mhPanel, IDC_RADIO_TARGET_VERTEXCOLOR) == BST_CHECKED )
		miTargetMapChannel = 0;
	else if( IsDlgButtonChecked(mhPanel, IDC_RADIO_TARGET_VERTEXILLUM) == BST_CHECKED )
		miTargetMapChannel = -1;
	else if( IsDlgButtonChecked(mhPanel, IDC_RADIO_TARGET_VERTEXALPHA) == BST_CHECKED )
		miTargetMapChannel = -2;
	else if( IsDlgButtonChecked(mhPanel, IDC_RADIO_TARGET_VERTEX) == BST_CHECKED )
		miTargetMapChannel = VERTEX_CHANNEL_NUM;
}

bool ProjectionModTypeUVW::GetAlwaysUpdate()
{
	BOOL bCreateNew = FALSE;
	mpPBlock->GetValue(pb_holderCreateNew, 0, bCreateNew, FOREVER);
	BOOL bSameTopo = FALSE;
	mpPBlock->GetValue(pb_sameTopology, 0, bSameTopo, FOREVER);
	BOOL bAlwaysUpdate = FALSE;
	mpPBlock->GetValue(pb_holderAlwaysUpdate, 0, bAlwaysUpdate, FOREVER);

	return !bCreateNew && bSameTopo && bAlwaysUpdate;
}

bool ProjectionModTypeUVW::GetSOSelData( INode *pSourceNode, int iSelIndex, BitArray &sel )
{
	bool bUseSOSel = false;
	IProjectionModData *pModData = mpPMod->GetProjectionModData(pSourceNode);
	if( pModData )
		bUseSOSel = pModData->GetGeomSel(iSelIndex, sel);
	return bUseSOSel;
}

ProjectionHolderUVW *ProjectionModTypeUVW::GetProjectionHolderUVW( Object* obj )
{
    // For all derived objects (can be > 1)
    while (obj && (obj->SuperClassID() == GEN_DERIVOB_CLASS_ID)) {
        IDerivedObject* dobj = (IDerivedObject*)obj;
        int numMods = dobj->NumModifiers();
        // Step through all modififers and verify the class id
        for (int i=0; i<numMods; i++) {
            Modifier* mod = dobj->GetModifier(i);
            if (mod && (mod->ClassID()==PROJECTIONHOLDERUVW_CLASS_ID)) {
				ProjectionHolderUVW *pModTest = (ProjectionHolderUVW *)mod;
				if( pModTest->GetProjectionTypeLink() == this )
					return pModTest;
            }
        }
        obj = dobj->GetObjRef();
    }
    
    return NULL;
}

bool ProjectionModTypeUVW::InitProjectionState( ProjectionState &projState, INode *pNodeSrc, ObjectState * osSrc, INode *pNodeTarget, ProjectionHolderUVW *pMod, BitArray *pSOSel, IProjectionModData *pPModData )
{
	TimeValue t = GetCOREInterface()->GetTime();

	// common
	projState.pData = &pMod->GetHolderData();
	projState.pData->Reset();
	FillInHoldDataOptions( projState.pData );
	projState.pSOSel = pSOSel;

	if( projState.iProjType == ProjectionState::pt_sameTopology ) {
		if( (pNodeSrc == NULL) && (pPModData == NULL) )
			return false;

		if( pPModData == NULL ) {
			pPModData = mpPMod->GetProjectionModData(pNodeSrc);
			if( !pPModData )
				return false;
		}

		projState.objectSource = &pPModData->GetBaseObject();

		if( projState.objectSource->IsEmpty() || (projState.objectSource->NumFaces() == 0) )
			return false;

		if( (miSourceMapChannel != VERTEX_CHANNEL_NUM) && !projState.objectSource->GetChannelSupport(miSourceMapChannel) ) 
			return false;

		if( osSrc ) {
			projState.pData->mSrcInterval &= osSrc->obj->ChannelValidity(t, TEXMAP_CHAN_NUM);
			projState.pData->mSrcInterval &= osSrc->obj->ChannelValidity(t, TOPO_CHAN_NUM);
			projState.pData->mSrcInterval &= osSrc->obj->ChannelValidity(t, GEOM_CHAN_NUM);
		}
	}
	else { 
		// Get Target ObjectWrapper
		ObjectState os = pNodeTarget->EvalWorldState(t);
		Matrix3 tarTM = pNodeTarget->GetObjTMAfterWSM( t );
		projState.objectTarget = new ObjectWrapper();
		projState.objectTarget->Init(t, os, false, ObjectWrapper::allEnable, ObjectWrapper::triObject);

		if( projState.objectTarget->IsEmpty() )
			return false;

		// Get Source ObjectWrapper
		IProjectionModData *pPModData = mpPMod->GetProjectionModData(pNodeSrc);
		if( !pPModData )
			return false;

		projState.objectSource = &pPModData->GetBaseObject();
		if( projState.objectSource->IsEmpty() || (projState.objectSource->NumFaces() == 0) )
			return false;

		pNodeSrc->EvalWorldState( t );
		Matrix3 srcTM = pNodeSrc->GetObjTMAfterWSM( t );

		// Init Intersector
		ObjectWrapper &objectCage = pPModData->GetCage();	// cage can be empty

		if( (miSourceMapChannel != VERTEX_CHANNEL_NUM) && !projState.objectSource->GetChannelSupport(miSourceMapChannel) ) 
			return false;

		IProjectionIntersectorMgr* pPInterMgr = GetIProjectionIntersectorMgr();
		if( !pPInterMgr )
			return false;

		projState.pPInter = pPInterMgr->CreateProjectionIntersector();
		if( !projState.pPInter )
			return false;

		projState.pPInter->InitRoot( *(projState.objectSource), objectCage, srcTM );
		if( !projState.pPInter->RootValid() ) {
			projState.pPInter->DeleteThis();
			projState.pPInter = NULL;
			return false;
		}
		projState.mat = (tarTM*Inverse(srcTM));
	}

	return true;
}

void ProjectionModTypeUVW::ProjectToTarget(INode *pNodeSrc, ObjectState *os, INode *pNodeTarget, BitArray *pSOSel, IProjectionModData *pPModData)
{
	if( pNodeTarget ) {
		Object* obj = pNodeTarget->GetObjectRef();

		if( obj ) {
			bool bCreatedMod = false;

			BOOL bCreateNew = FALSE;
			mpPBlock->GetValue(pb_holderCreateNew, 0, bCreateNew, FOREVER);

			ProjectionHolderUVW* mod;
			if( GetAlwaysUpdate() || !bCreateNew )
				mod = GetProjectionHolderUVW(obj);
			else
				mod = NULL;

			// If we aren't reusing a holder modifier, create a new one
			if( !mod ) {
				bCreatedMod = true;
				mod = (ProjectionHolderUVW*)CreateInstance(OSM_CLASS_ID, PROJECTIONHOLDERUVW_CLASS_ID);
			}

			if( mod ) {
				Interface *ip = GetCOREInterface();

				// Set Modifier/Holder name
				TCHAR *szName = NULL;
				mpPBlock->GetValue(pb_holderName, 0, szName, FOREVER);
				if( szName )
					mod->SetName( szName );

				// Set ProjectionType Link to us
				if( mod->GetProjectionTypeLink() != this )
					mod->SetProjectionTypeLink(this);

				// Use Proper projection technique
				BOOL bSameTopology = FALSE;
				mpPBlock->GetValue(pb_sameTopology, 0, bSameTopology, FOREVER);

				ProjectionState projState;

				if( bSameTopology ) {
					projState.iProjType = ProjectionState::pt_sameTopology;
					if( InitProjectionState( projState, pNodeSrc, os, pNodeTarget, mod, pSOSel, pPModData ) )
						FillInHoldDataSameTopology( projState );
				}
				else if( miSourceMapChannel > 0 ) {
					// If the source channel is a "real" map channel, use the seam detecting algorithm
					projState.iProjType = ProjectionState::pt_projectionCluster;
					if( InitProjectionState( projState, pNodeSrc, os, pNodeTarget, mod, pSOSel, pPModData ) )
						FillInHoldDataClusterProjection( projState );

				}
				else {
					projState.iProjType = ProjectionState::pt_projection;
					if( InitProjectionState( projState, pNodeSrc, os, pNodeTarget, mod, pSOSel, pPModData ) )
						FillInHoldDataProjection( projState );
				}

				projState.Cleanup();

				if( bCreatedMod ) {
					IDerivedObject* dobj = CreateDerivedObject(obj);
					dobj->AddModifier(mod);
					pNodeTarget->SetObjectRef(dobj);
				}

				mod->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
				if( bCreatedMod )
					pNodeTarget->NotifyDependents(FOREVER,PART_ALL,REFMSG_SUBANIM_STRUCTURE_CHANGED);
			}
		}
	}

}

bool ProjectionModTypeUVW::GetProjectMaterialsIDs()
{
	if( !mpPBlock )
		return false;
	return mpPBlock->GetInt(pb_projectMaterialIDs, GetCOREInterface()->GetTime()) != 0;
}

void ProjectionModTypeUVW::FillInHoldDataOptions( ProjectionHolderUVWData *pData )
{
	pData->miSource = ProjectionHolderUVWData::source_uvw;
	pData->miMapChannel = miTargetMapChannel;
	pData->mDoMaterialIDs = GetProjectMaterialsIDs();
}

GenFace ProjectionModTypeUVW::GetChannelFace( ObjectWrapper *object, int iFace, int iMapChannel )
{
	if( iMapChannel == VERTEX_CHANNEL_NUM )
		return object->GetFace( iFace );
	else
		return object->GetMapFace( iMapChannel, iFace );
}

Point3 *ProjectionModTypeUVW::GetChannelVert( ObjectWrapper *object, int iVert, int iMapChannel )
{
	if( iMapChannel == VERTEX_CHANNEL_NUM )
		return object->GetVert( iVert );
	else
		return object->GetMapVert( iMapChannel, iVert );
}

int ProjectionModTypeUVW::FindMostHitFace(Tab<int> &tabFaceHits)
{
	// Find the most hit face
	int iMostHitIndex = 0;
	int iHitCount = 0;
	for(int i=0; i<tabFaceHits.Count(); i++) {
		int iHits = 0;
		for(int j=0; j<tabFaceHits.Count(); j++) {
			if( tabFaceHits[i] == tabFaceHits[j] )
				iHits++;
		}
		if( iHits > iHitCount ) {
			iMostHitIndex = i;
			iHitCount = iHits;
		}
	}

	// Get the "main" or most common value
	return tabFaceHits[iMostHitIndex];
}


// Expand face list by traversing edges.  If edge is UV seam, do not cross edge.
int ProjectionModTypeUVW::ExpandClusterFaces( ProjectionState &projState, Tab<int> &clusterFaceList, int iNewFaceIndex, BitArray &faceVisited )
{
	int iNumAdded = 0;
	int iCurClusterFaceListCount = clusterFaceList.Count();

	for( int f=iNewFaceIndex; f<iCurClusterFaceListCount; f++ ) {

		int iFaceIndex = clusterFaceList[f];
		faceVisited.Set(iFaceIndex);

		GenFace face = projState.objectSource->GetFace(iFaceIndex);

		int iNumEdges = projState.objectSource->NumFaceEdges(iFaceIndex);

		// Traverse through the edges for this face.
		for( int i=0; i<iNumEdges; i++ ) {

			// Find the other face index on this edge
			int iOtherFace = -1;
			int iEdgeIndex = projState.objectSource->GetFaceEdgeIndex(iFaceIndex, i);
			if( iEdgeIndex == -1 )
				continue;
			
			// If this edge is a cluster border, don't look across it
			if( projState.bitClusterEdges[iEdgeIndex] )
				continue;

			GenEdge edge = projState.objectSource->GetEdge(iEdgeIndex);
			if( ((int)edge.f[0] != -1) && ((int)edge.f[0] != iFaceIndex) )
				iOtherFace = (int)edge.f[0];
			else if( ((int)edge.f[1] != -1) && ((int)edge.f[1] != iFaceIndex) )
				iOtherFace = (int)edge.f[1];

			if( iOtherFace == -1 )
				continue;

			if( faceVisited[iOtherFace] )
				continue;

			// Add the new face to the list
			clusterFaceList.Append(1, &iOtherFace);
			faceVisited.Set(iOtherFace);
			iNumAdded++;
		}
	}

	return iNumAdded;
}

// Traverse all the edges of the source object and tag edges that represent UV seams
bool ProjectionModTypeUVW::CreateClusterEdges( ProjectionState &projState )
{
	int iEdgeCount = projState.objectSource->NumEdges();

	for( int i=0; i<iEdgeCount; i++ ) {

		GenEdge edge = projState.objectSource->GetEdge(i);
		if( ((int)edge.f[0] == -1) || ((int)edge.f[1] == -1) )
			continue;

		int iFaceIndex1 = edge.f[0];
		int iFaceIndex2 = edge.f[1];

		GenFace face1 = projState.objectSource->GetFace(iFaceIndex1);
		GenFace face2 = projState.objectSource->GetFace(iFaceIndex2);

		GenFace faceMap1 = projState.objectSource->GetMapFace(miSourceMapChannel, iFaceIndex1);
		GenFace faceMap2 = projState.objectSource->GetMapFace(miSourceMapChannel, iFaceIndex2);

		// Find the corresponding face vert index to edge vert
		int iFace1V1 = 0, iFace1V2 = 0, iFace2V1 = 0, iFace2V2 = 0;
		for( int j=0; j<face1.numVerts; j++ ) {
			if( edge.v[0] == face1.verts[j] )
				iFace1V1 = j;
			if( edge.v[1] == face1.verts[j] )
				iFace1V2 = j;
		}

		for( int j=0; j<face2.numVerts; j++ ) {
			if( edge.v[0] == face2.verts[j] )
				iFace2V1 = j;
			if( edge.v[1] == face2.verts[j] )
				iFace2V2 = j;
		}

		if( (faceMap1.verts[iFace1V1] != faceMap2.verts[iFace2V1])
			|| (faceMap1.verts[iFace1V2] != faceMap2.verts[iFace2V2]) ) 
		{
			projState.bitClusterEdges.Set(i);
		}
	}

	return true;
}

void ProjectionModTypeUVW::CreateClusterData( ProjectionState &projState )
{
	int iFaceCount = projState.objectSource->NumFaces();

	BitArray faceVisited;
	faceVisited.SetSize(iFaceCount);
	faceVisited.ClearAll();

	projState.tabCluster.SetCount(iFaceCount);

	projState.bitClusterEdges.SetSize( projState.objectSource->NumEdges() );
	projState.bitClusterEdges.ClearAll();

	// Tag cluster edges
	CreateClusterEdges(projState);

	int iClusterNum = 1;
	Point2 ptClusterCenter;
	Tab<int> clusterFaceList;
	clusterFaceList.Resize(iFaceCount);
	for( int i=0; i<iFaceCount; i++ ) {
		if( !faceVisited[i] ) {

			// Expand the cluster faces until no more are added
			int iNewFaceIndex = 0;
			clusterFaceList.SetCount(1, FALSE);
			clusterFaceList[0] = i;
			int iNumAdded = 1;
			while( (iNumAdded = ExpandClusterFaces( projState, clusterFaceList, iNewFaceIndex, faceVisited )) != 0 ) {
				iNewFaceIndex = clusterFaceList.Count() - iNumAdded;
			}

			// We now have a cluster list
			Box3 bBox;
			bBox.Init();
			for( int f=0; f<clusterFaceList.Count(); f++ ) {
				int iFaceIndex = clusterFaceList[f];

				// Set cluster number
				projState.tabCluster[iFaceIndex] = iClusterNum;

				// Record bounding box of cluster
				GenFace faceMap = projState.objectSource->GetMapFace(miSourceMapChannel, iFaceIndex);
				for( int j=0; j<faceMap.numVerts; j++ ) {
					Point3 ptVert = *GetChannelVert( projState.objectSource, faceMap.verts[j], miSourceMapChannel );
					if( ptVert[0] < bBox.pmin[0] )
						bBox.pmin[0] = ptVert[0];
					if( ptVert[1] < bBox.pmin[1] )
						bBox.pmin[1] = ptVert[1];
					if( ptVert[0] > bBox.pmax[0] )
						bBox.pmax[0] = ptVert[0];
					if( ptVert[1] > bBox.pmax[1] )
						bBox.pmax[1] = ptVert[1];
				}
			}

			// Store the cluster center UV value
			ptClusterCenter[0] = (bBox.pmax[0]+bBox.pmin[0])*0.5f;
			ptClusterCenter[1] = (bBox.pmax[1]+bBox.pmin[1])*0.5f;
			projState.tabClusterCenter.Append(1, &ptClusterCenter);

			iClusterNum++;
		}
	}

	int iWow = projState.bitClusterEdges.NumberSet();
	iClusterNum = iClusterNum;

}

bool GetDistanceToEdge( Point3 &pt, Point3 &seg1, Point3 &seg2, float &fDist )
{
    float fLength = Length( seg2 - seg1 );
 
	float u = ( ((pt.x - seg1.x) * (seg2.x - seg1.x)) +
				((pt.y - seg1.y) * (seg2.y - seg1.y)) +
				((pt.z - seg1.z) * (seg2.z - seg1.z)) ) /
				(fLength * fLength);
 
    if( (u < 0.0f) || (u > 1.0f) )
        return false;
 
	Point3 ptInter;
    ptInter.x = seg1.x + u * (seg2.x - seg1.x);
    ptInter.y = seg1.y + u * (seg2.y - seg1.y);
    ptInter.z = seg1.z + u * (seg2.z - seg1.z);
 
    fDist = Length(pt - ptInter);
 
    return true;
}

// returns percentage along edge relative to seg1.  So, 1.0f means it is on seg1. 0.0f means it is on seg2
float ProjectPointToEdge( Point3 &pt, Point3 &seg1, Point3 &seg2 )
{
	Point3 p1, p2, p3;

	p1 = seg2-seg1;
	float fLengthP1 = Length(p1);
	p2 = pt-seg1;
	p3 = (float)(DotProd(p2,p1)/(fLengthP1*fLengthP1)) * p1;

	float w1 = 1.0f;
	if( fLengthP1 > 0.0000001f )
		w1 = 1.0f - (Length(p3) / fLengthP1);

	if( w1 < 0.0f )
		w1 = 0.0f;
	if( w1 > 1.0f )
		w1 = 1.0f;

	return w1;
}

void ProjectionModTypeUVW::ProjectToEdgeToFindUV( ProjectionState &projState, Point3 &p3Tar, int iEdgeIndex, int iFaceIndex, Point3 &UV )
{
	if( (iEdgeIndex != -1) && (iFaceIndex != -1) ) {
		// Prototype code to project the hi-res point onto the matching low-res edge.
		// Once we have the point projected onto the low-res edge, weigh the UV coords
		// at the two verts on the edge based on the edge projection location.
		Point3 v1, v2;
		GenEdge edgeSource = projState.objectSource->GetEdge(iEdgeIndex);
		v1 = *projState.objectSource->GetVert( edgeSource.v[0] );
		v2 = *projState.objectSource->GetVert( edgeSource.v[1] );
		float w1 = 1.0f, w2 = 0.0f;
		w1 = ProjectPointToEdge( p3Tar, v1, v2 );
		w2 = 1.0f - w1;
		GenFace faceSource = projState.objectSource->GetFace(iFaceIndex);
		int iFaceV0 = 0, iFaceV1 = 0;
		for( int j=0; j<faceSource.numVerts; j++ ) {
			if( edgeSource.v[0] == faceSource.verts[j] )
				iFaceV0 = j;
			if( edgeSource.v[1] == faceSource.verts[j] )
				iFaceV1 = j;
		}
		GenFace faceMapSource = GetChannelFace( projState.objectSource, iFaceIndex, miSourceMapChannel );
		v1 = *GetChannelVert( projState.objectSource, faceMapSource.verts[iFaceV0], miSourceMapChannel );
		v2 = *GetChannelVert( projState.objectSource, faceMapSource.verts[iFaceV1], miSourceMapChannel );
		UV = (v1*w1)+(v2*w2);
	}
}

void ProjectionModTypeUVW::FindClosestFaceAndEdgeByCluster( ProjectionState &projState, Point3 &p3Tar, int iMainCluster, int &iClosestEdgeIndex, int &iClosestFaceIndex )
{
	bool bInit = true;
	float fDist;
	float fClosestDist;
	int iClosestEdge = -1;
	int iClosestFace = -1;
	for(int i=0; i<projState.objectSource->NumEdges(); i++ ) {

		// Criteria #1 edge needs to be on a cluster
		if( !projState.bitClusterEdges[i] )
			continue;

		GenEdge edgeSource = projState.objectSource->GetEdge(i);

		// Criteria #2 one of the faces needs to be in the correct cluster
		int iFaceIndex = -1;
		if( ((int)edgeSource.f[0] != -1) && (projState.tabCluster[edgeSource.f[0]] == iMainCluster) )
			iFaceIndex = (int)edgeSource.f[0];
		else if( ((int)edgeSource.f[1] != -1) && (projState.tabCluster[edgeSource.f[1]] == iMainCluster) )
			iFaceIndex = (int)edgeSource.f[1];

		if( iFaceIndex != -1 ) {
			// Edge okay to test
			Point3 v0 = *projState.objectSource->GetVert( edgeSource.v[0] );
			Point3 v1 = *projState.objectSource->GetVert( edgeSource.v[1] );
			if( !GetDistanceToEdge( p3Tar, v0, v1, fDist ) )
				continue;
			if( bInit || (fDist < fClosestDist) ) {
				bInit = false;
				fClosestDist = fDist;
				iClosestEdge = i;
				iClosestFace = iFaceIndex;
			}
		}
	}

	iClosestEdgeIndex = iClosestEdge;
	iClosestFaceIndex = iClosestFace;
}

int ProjectionModTypeUVW::GetUVQuadrant( ProjectionState &projState, int iCluster, Point3 &ptUVW )
{
	int iQuad = -1;

	if( (iCluster < 1) || ((iCluster-1) >= projState.tabClusterCenter.Count()) )
		return iQuad;

	Point2 ptRelUV = projState.tabClusterCenter[iCluster-1];
	ptRelUV[0] = ptUVW[0] - ptRelUV[0];
	ptRelUV[1] = ptUVW[1] - ptRelUV[1];

	//	0	1
	//	2	3
	iQuad  = (ptRelUV[1] >= 0.0f) ? 0 : 2;
	iQuad += (ptRelUV[0] >= 0.0f) ? 1 : 0;

	return iQuad;
}

void ProjectionModTypeUVW::StoreMapFaceData( ProjectionState &projState, int iTargetFace, Tab<int> &tabFaceHits, int iCenterFaceHit, Point3 &ptCenter, Point3 &ptCenterUVW )
{
	GenFace faceTarget = projState.objectTarget->GetFace(iTargetFace);

	// Set number of verts and allocate vert array
	projState.pData->mFaces[iTargetFace].numVerts = faceTarget.numVerts;
	projState.pData->mFaces[iTargetFace].verts = new DWORD[faceTarget.numVerts];

	// Eval cluster vars
	BitArray bitClusterHit;

	// Eval seams vars
	BitArray bitSeamHit;
	Tab<int> tabOtherFaceHits;
	Tab<int> tabEdgeHits;

	// Regardless of the most hit face, we will use iCenterFaceHit to determine
	// the "most hit face".  This will help us make a better cluster decision.
	int iMostHitFace;
	int iMostHitClusterIndex = 0;
	if( iCenterFaceHit != -1 ) {
		iMostHitFace = iCenterFaceHit;
	}
	else {
		// no iCenterFaceHit, find the most hit cluster
		int iMostHitCount = 0;
		for(int i=0; i<tabFaceHits.Count(); i++) {
			int iHits = 0;
			for(int j=0; j<tabFaceHits.Count(); j++) {
				if( (tabFaceHits[i] != -1) 
					&& (tabFaceHits[j] != -1) 
					&& (projState.tabCluster[tabFaceHits[i]] == projState.tabCluster[tabFaceHits[j]]) )
				{
					iHits++;
				}
			}
			if( iHits > iMostHitCount ) {
				iMostHitClusterIndex = i;
				iMostHitCount = iHits;
			}
		}

		// Check if the most hit face was really a miss, then just move the vertex indices over
		if( tabFaceHits[iMostHitClusterIndex] == -1 ) {
			for( int j=0; j<faceTarget.numVerts; j++ )
				projState.pData->mFaces[iTargetFace].verts[j] = faceTarget.verts[j];
			return;
		}
		// Record most hit face
		iMostHitFace = tabFaceHits[iMostHitClusterIndex];
	}

	// Get the "main" or most common cluster value
	int iMainCluster = projState.tabCluster[iMostHitFace];

	// Eval the face hits we will need to recalc based on incorrect cluster results
	bitClusterHit.SetSize(tabFaceHits.Count());
	bitClusterHit.ClearAll();
	for( int j=0; j<faceTarget.numVerts; j++ ) {
		if( (tabFaceHits[j] == -1) || (iMainCluster != projState.tabCluster[tabFaceHits[j]]) )
			bitClusterHit.Set(j);
	}

	// Give preference to cluster hit results.  If we have cluster results, use them.
	// Else, check to see if we have any same-cluster seams.
	bool bClusterHits = true;
	BitArray bitRecalcHit;
	bitRecalcHit.SetSize(tabFaceHits.Count());
	if( bitClusterHit.NumberSet() > 0 )
		bitRecalcHit = bitClusterHit;
	else {
		// No cluster issues, check for same-cluster seams
		bitSeamHit.SetSize(tabFaceHits.Count());
		bitSeamHit.ClearAll();
		tabOtherFaceHits.SetCount(tabFaceHits.Count());
		tabEdgeHits.SetCount(tabFaceHits.Count());

		// init arrays
		for(int i=0; i<tabFaceHits.Count(); i++) {
			tabOtherFaceHits[i] = tabFaceHits[i];
			tabEdgeHits[i] = -1;
		}

		// Store the UV cluster quadrant for each face hit
		Tab<int> tabQuads;
		tabQuads.SetCount(tabFaceHits.Count());
		for( int j=0; j<faceTarget.numVerts; j++ ) {
			Point3 ptUV = projState.pData->mP3Data[faceTarget.verts[j]];
			tabQuads[j] = GetUVQuadrant( projState, projState.tabCluster[tabFaceHits[j]], ptUV );
		}

		// Do any of the hit faces have a cluster edge?
		// Start the array at -1 so we can examine the center face hit
		for(int i=-1; i<tabFaceHits.Count(); i++) {
			int iSrcFace;
			int iQuad;
			if( i == -1 ) {
				iSrcFace = iCenterFaceHit;
				iQuad = GetUVQuadrant( projState, iMainCluster, ptCenterUVW );
			}
			else {
				// If we already have a seam hit on this vert, no need to continue
				if( bitSeamHit[i] )
					continue;
				iSrcFace = tabFaceHits[i];
				iQuad = tabQuads[i];
			}
			if( iSrcFace == -1 )
				continue;
			int iNumEdges = projState.objectSource->NumFaceEdges(iSrcFace);
			for( int e=0; e<iNumEdges; e++ ) {
				int iEdgeIndex = projState.objectSource->GetFaceEdgeIndex(iSrcFace, e);
				if( iEdgeIndex == -1 )
					continue;
				if( projState.bitClusterEdges[iEdgeIndex] ) {
					// Okay, we have a seam, but see if we cross over the seam.
					// This isn't entirely accurate, since it assumes we hit the face
					// on the other side of the edge.
					GenEdge edge = projState.objectSource->GetEdge( iEdgeIndex );
					int iOtherFace = -1;
					if( iSrcFace == (int)edge.f[0] )
						iOtherFace = (int)edge.f[1];
					else
						iOtherFace = (int)edge.f[0];

					for( int j=0; j<tabFaceHits.Count(); j++ ) {
						// Other face was hit or we cross a quadrant
						if( (iQuad != tabQuads[j]) || (tabFaceHits[j] == iOtherFace) ) {
							bitSeamHit.Set(j);
							tabOtherFaceHits[j] = iSrcFace;
							tabEdgeHits[j] = iEdgeIndex;
						}
					}
				}
			}
		}
		// Use the seam hit results to recalc
		bitRecalcHit = bitSeamHit;
		bClusterHits = false;
	}

	// Handle material ID first
	if( projState.pData->mDoMaterialIDs )
		projState.pData->mMatID[iTargetFace] = projState.objectSource->GetMtlID(iMostHitFace);

	// If we do not need to Recalc any of hits, just transfer the face data from the projections.
	if( bitRecalcHit.NumberSet() == 0 ) {
		for( int j=0; j<faceTarget.numVerts; j++ )
			projState.pData->mFaces[iTargetFace].verts[j] = faceTarget.verts[j];
	}
	else {

		// We need create and calculate new map verts

		// We need to increase the number of map verts (which is held in the p3data tab)
		// We need to increase it by the number of verts outside of the main cluster
		int iNewVerts = 0;
		for(int i=0; i<tabFaceHits.Count(); i++ ) {
			if( (tabFaceHits[i] == -1) || bitRecalcHit[i] )
				iNewVerts++;
		}
		int iNewVertIndex = projState.pData->mP3Data.Count();
		projState.pData->mP3Data.SetCount(projState.pData->mP3Data.Count()+iNewVerts);

		// Loop thru face verts.  If the cluster is the same, use the
		// projection values, else calculate and reference a new UV
		for( int j=0; j<faceTarget.numVerts; j++ ) {
			if( (tabFaceHits[j] == -1) || bitRecalcHit[j] ) {
				Point3 UV(projState.pData->mP3Data[faceTarget.verts[j]]);
				Point3 p3Tar = *projState.objectTarget->GetVert( faceTarget.verts[j] );
				p3Tar = p3Tar*projState.mat;
				int iEdge, iFace;
				if( tabFaceHits[j] != -1 ) {
					// First, before we split the face up, move the UVs from the "other" cluster faces
					// to the edge of their cluster or seam side.
					if( bClusterHits )
						FindClosestFaceAndEdgeByCluster( projState, p3Tar, projState.tabCluster[tabFaceHits[j]], iEdge, iFace );
					else {
						iEdge = tabEdgeHits[j];
						iFace = tabFaceHits[j];
					}
					ProjectToEdgeToFindUV( projState, p3Tar, iEdge, iFace, UV );
					projState.pData->mP3Data[faceTarget.verts[j]] = UV;
				}

				// Need to calc new UV
				projState.pData->mFaces[iTargetFace].verts[j] = iNewVertIndex;
				if( bClusterHits )
					FindClosestFaceAndEdgeByCluster( projState, p3Tar, iMainCluster, iEdge, iFace );
				else {
					iEdge = tabEdgeHits[j];
					iFace = tabOtherFaceHits[j];
				}
				ProjectToEdgeToFindUV( projState, p3Tar, iEdge, iFace, UV );
				projState.pData->mP3Data[iNewVertIndex] = UV;
				iNewVertIndex++;
			}
			else {
				projState.pData->mFaces[iTargetFace].verts[j] = faceTarget.verts[j];
			}
		}
	}
}

void ProjectionModTypeUVW::FillInHoldDataSameTopology( ProjectionState &projState )
{
	int iNumFaces = projState.objectSource->NumFaces();
    int iNumVerts;
	if( miSourceMapChannel == VERTEX_CHANNEL_NUM )
		iNumVerts = projState.objectSource->NumVerts();
	else
		iNumVerts = projState.objectSource->NumMapVerts(miSourceMapChannel);

	// Init tab counts in holder data
	projState.pData->mFaces.SetCount(iNumFaces);
	projState.pData->mP3Data.SetCount(iNumVerts);
	if( projState.pData->mDoMaterialIDs )
		projState.pData->mMatID.SetCount(iNumFaces);

	// Store the chosen point3 data
	for( int i=0; i<iNumVerts; i++ ) 
		projState.pData->mP3Data[i] = *GetChannelVert(projState.objectSource, i, miSourceMapChannel );
	
	// Recreate GenFace structures
	for( int i=0; i<iNumFaces; i++ ) {
		if( projState.pData->mDoMaterialIDs )
			projState.pData->mMatID[i] = projState.objectSource->GetMtlID(i);
		GenFace face = GetChannelFace(projState.objectSource, i, miSourceMapChannel );
		projState.pData->mFaces[i].numVerts = face.numVerts;
		projState.pData->mFaces[i].verts = new DWORD[face.numVerts];
		for( int j=0; j<face.numVerts; j++ )
			projState.pData->mFaces[i].verts[j] = face.verts[j];
	}

}

void ProjectionModTypeUVW::FillInHoldDataProjection( ProjectionState &projState )
{
	int iNumVerts = projState.objectTarget->NumVerts();
	int iNumFaces = projState.objectTarget->NumFaces();

	// Init tab counts in holder data
	projState.pData->mFaces.SetCount(iNumFaces);
	projState.pData->mP3Data.SetCount(iNumVerts);
	if( projState.pData->mDoMaterialIDs )
		projState.pData->mMatID.SetCount(iNumFaces);

	Point3 p3Tar, uv;
	Point3 p3Zero(0.0f,0.0f,0.0f);
	float fDist;
	DWORD dwFace;
	Tab<float> tabBary;

	// cached vertex hit results
	Tab<int> tabFaceHits;
	tabFaceHits.SetCount( iNumVerts );

	// Use ProjectionIntersector to find closest source face per target vertex
	for( int i=0; i<iNumVerts; i++ ) {
		uv = p3Zero;
		tabFaceHits[i] = -1;
		// Get target vert
		p3Tar = *projState.objectTarget->GetVert( i );
		// Put into object space of source
		p3Tar = p3Tar*projState.mat;

		Ray pointAndNormal;
		pointAndNormal.p = p3Tar;
		int flags = IProjectionIntersector::FINDCLOSEST_CAGED;

		if( projState.pPInter->RootClosestFace( pointAndNormal, flags, fDist, dwFace, tabBary ) ) {
			// Check sub-object selection
			if( (projState.pSOSel == NULL) || (*projState.pSOSel)[dwFace] ) {
				tabFaceHits[i] = dwFace;
				GenFace face = GetChannelFace( projState.objectSource, dwFace, miSourceMapChannel );
				// Calc value based on bary hit results
				for( int j=0; j<tabBary.Count(); j++ ) {
					uv += ((*GetChannelVert(projState.objectSource, face.verts[j], miSourceMapChannel)) * tabBary[j]);
				}
			}
			else if( (miTargetMapChannel == VERTEX_CHANNEL_NUM) || projState.objectTarget->GetChannelSupport(miTargetMapChannel) ) {
				uv = *GetChannelVert(projState.objectTarget, i, miTargetMapChannel);
			}
		}
		projState.pData->mP3Data[i] = uv;
	}
	
	// Recreate GenFace structures
	Tab<int> tabPerFaceHit;
	for( int i=0; i<iNumFaces; i++ ) {
		GenFace face = projState.objectTarget->GetFace( i );
		projState.pData->mFaces[i].numVerts = face.numVerts;
		projState.pData->mFaces[i].verts = new DWORD[face.numVerts];
		if( projState.pData->mDoMaterialIDs )
			tabPerFaceHit.SetCount( face.numVerts );
		for( int j=0; j<face.numVerts; j++ ) {
			projState.pData->mFaces[i].verts[j] = face.verts[j];
			if( projState.pData->mDoMaterialIDs )
				tabPerFaceHit[j] = tabFaceHits[face.verts[j]];
		}
		// To get the material ID, determine the most hit source face for the target verts on this
		// target face
		if( projState.pData->mDoMaterialIDs ) {
			int iSourceFace = FindMostHitFace( tabPerFaceHit );
			if( iSourceFace != -1 )
				projState.pData->mMatID[i] = projState.objectSource->GetMtlID(iSourceFace);
			else
				projState.pData->mMatID[i] = projState.objectTarget->GetMtlID(i);
		}
	}

}

void ProjectionModTypeUVW::FillInHoldDataClusterProjection( ProjectionState &projState   )
{
	int iNumVerts = projState.objectTarget->NumVerts();
	int iNumFaces = projState.objectTarget->NumFaces();

	// Get cluster data
	// This will calculate the data to group faces into UVW clusters/charts
	// It will also mark cluster edges.
	CreateClusterData( projState );

	// Init tab counts in holder data
	projState.pData->mFaces.SetCount(iNumFaces);
	projState.pData->mP3Data.SetCount(iNumVerts);
	if( projState.pData->mDoMaterialIDs )
		projState.pData->mMatID.SetCount(iNumFaces);

	Point3 p3Tar, uv;
	Point3 p3Zero(0.0f,0.0f,0.0f);
	float fDist;
	DWORD dwFace;
	Tab<float> tabBary;

	// cached vertex hit results
	Tab<int> tabFaceHits;
	tabFaceHits.SetCount( iNumVerts );

	// Use ProjectionIntersector to find closest source face per target vertex
	for( int i=0; i<iNumVerts; i++ ) {
		tabFaceHits[i] = -1;
		uv = p3Zero;
		// Get target vert
		p3Tar = *projState.objectTarget->GetVert( i );
		// Put into object space of source
		p3Tar = p3Tar*projState.mat;

		Ray pointAndNormal;
		pointAndNormal.p = p3Tar;
		pointAndNormal.dir = *projState.objectTarget->GetVertexNormal(i);
		pointAndNormal.dir = Normalize( pointAndNormal.dir );
		int flags = IProjectionIntersector::FINDCLOSEST_CAGED | IProjectionIntersector::FINDCLOSEST_IGNOREBACKFACING;

		BOOL bInterRes = projState.pPInter->RootClosestFace( pointAndNormal, flags, fDist, dwFace, tabBary );
		if( !bInterRes )
			bInterRes = projState.pPInter->RootClosestFace( pointAndNormal, IProjectionIntersector::FINDCLOSEST_IGNOREBACKFACING, fDist, dwFace, tabBary );
		if( bInterRes ) {
			// Check sub-object selection
			if( (projState.pSOSel == NULL) || (*projState.pSOSel)[dwFace] ) {
				tabFaceHits[i] = dwFace;
				GenFace face = GetChannelFace(projState.objectSource, dwFace, miSourceMapChannel);
				// Calc value based on bary hit results
				for( int j=0; (j<tabBary.Count()) && (j<face.numVerts); j++ ) {
					uv += ((*GetChannelVert(projState.objectSource, face.verts[j], miSourceMapChannel)) * tabBary[j]);
				}
			}
		}
		projState.pData->mP3Data[i] = uv;
	}

	Point3 norm;
	Point3 ptCenter;
	Point3 ptCenterUVW;
	Tab<int> tabPerFaceHit;
	for( int i=0; i<iNumFaces; i++ ) {
		ptCenter = p3Zero;
		ptCenterUVW = p3Zero;
		norm = p3Zero;
		GenFace faceTarget = projState.objectTarget->GetFace(i);
		tabPerFaceHit.SetCount( faceTarget.numVerts );
		// Create a source face hit array for the verts on this target face
		// In addition, we will also find the closest source face to the center
		// point of this target face.  This will help use determine the proper
		// cluster to use.
		for( int j=0; j<faceTarget.numVerts; j++ ) {
			tabPerFaceHit[j] = tabFaceHits[faceTarget.verts[j]];
			ptCenter += *projState.objectTarget->GetVert(faceTarget.verts[j]);
			norm += *projState.objectTarget->GetVertexNormal(faceTarget.verts[j]);
		}
		norm /= faceTarget.numVerts;
		ptCenter /= faceTarget.numVerts;
		ptCenter = ptCenter * projState.mat;
		int iCenterFace = -1;

		Ray pointAndNormal;
		pointAndNormal.p = ptCenter;
		pointAndNormal.dir = norm;
		pointAndNormal.dir = Normalize( pointAndNormal.dir );
		int flags = IProjectionIntersector::FINDCLOSEST_CAGED | IProjectionIntersector::FINDCLOSEST_IGNOREBACKFACING;

		// Find closest source face to the center point of this target face
		BOOL bInterRes = projState.pPInter->RootClosestFace( pointAndNormal, flags, fDist, dwFace, tabBary );
		if( !bInterRes )
			bInterRes = projState.pPInter->RootClosestFace( pointAndNormal, IProjectionIntersector::FINDCLOSEST_IGNOREBACKFACING, fDist, dwFace, tabBary );
		if( bInterRes ) {
			if( (projState.pSOSel == NULL) || (*projState.pSOSel)[dwFace] ) {
				iCenterFace = dwFace;
				GenFace face = GetChannelFace(projState.objectSource, dwFace, miSourceMapChannel);
				// Calc value based on bary hit results
				for( int j=0; (j<tabBary.Count()) && (j<face.numVerts); j++ )
					ptCenterUVW += ((*GetChannelVert(projState.objectSource, face.verts[j], miSourceMapChannel)) * tabBary[j]);
			}
		}
		// Create the target face data based on hit results and cluster information
		StoreMapFaceData( projState, i, tabPerFaceHit, iCenterFace, ptCenter, ptCenterUVW );
	}
}
