/**********************************************************************
 *<
	FILE: sf_wraps.cpp

	DESCRIPTION: 

	CREATED BY: Simon Feltman

	HISTORY: Created 9 December 1998

 *>	Copyright (c) 1998, All Rights Reserved.
 **********************************************************************/

//#include "pch.h"
#include "MAXScrpt.h"
#include "Numbers.h"
#include "3DMath.h"
#include "MAXObj.h"

#include "resource.h"

#ifdef ScripterExport
	#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

#include "ExtClass.h"
#include "MXSAgni.h"

#include "defextfn.h"
#	include "namedefs.h"

#include "definsfn.h"
#	include "sf_wraps.h"

#include "agnidefs.h"

// ============================================================================
static int vpt_lookup[] = {
	MAXCOM_VPT_LEFT,
	MAXCOM_VPT_RIGHT,
	MAXCOM_VPT_TOP,
	MAXCOM_VPT_BOTTOM,
	MAXCOM_VPT_FRONT,
	MAXCOM_VPT_BACK,
	MAXCOM_VPT_ISO_USER,
	MAXCOM_VPT_PERSP_USER,
	MAXCOM_VPT_CAMERA,
	MAXCOM_VPT_GRID,
	-1, // VIEW_NONE
	MAXCOM_VPT_TRACK,
	MAXCOM_VPT_PERSP_USER,
	MAXCOM_VPT_SPOT,
	MAXCOM_VPT_SHAPE,
};


// ============================================================================
Value* VP_SetType_cf(Value **arg_list, int count)
{
	check_arg_count(SetType, 1, count);
	def_view_types();
// AF -- (04/30/02) Removed the Trackview in a viewport feature ECO #831
#ifndef TRACKVIEW_IN_A_VIEWPORT
	if (arg_list[0] == n_view_track)
		return &false_value;
#endif //TRACKVIEW_IN_A_VIEWPORT
	int cmd = vpt_lookup[GetID(viewTypes, elements(viewTypes), arg_list[0], -1)];
	if(cmd >= 0) {
		MAXScript_interface->ExecuteMAXCommand(cmd);
		return &true_value;
	}

	return &false_value;
}


// ============================================================================
Value* VP_GetType_cf(Value **arg_list, int count)
{
	check_arg_count(GetType, 0, count);
	def_view_types();

	if (MAXScript_interface7->getActiveViewportIndex() < 0) return &undefined;
	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	int type = vpt->GetViewType();
	Value *vptType = GetName(viewTypes, elements(viewTypes), vpt->GetViewType(), &undefined);
	MAXScript_interface->ReleaseViewport(vpt);

	return vptType;
}

// ============================================================================
Value* VP_IsPerspView_cf(Value **arg_list, int count)
{
	check_arg_count(IsPerspView, 0, count);

	if (MAXScript_interface7->getActiveViewportIndex() < 0) return &undefined;
	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	BOOL isPerspView = vpt->IsPerspView();
	MAXScript_interface->ReleaseViewport(vpt);

	return bool_result(isPerspView);
}

// ============================================================================
Value* VP_GetFPS_cf(Value **arg_list, int count)
{
	check_arg_count(GetFPS, 0, count);

	if (MAXScript_interface7->getActiveViewportIndex() < 0) return &undefined;
	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	float fps = vpt->GetFPS();
	MAXScript_interface->ReleaseViewport(vpt);

	return Float::intern(fps);
}

// ============================================================================
Value* VP_SetTM_cf(Value **arg_list, int count)
{
	check_arg_count(SetTM, 1, count);

	BOOL result = FALSE;
	Matrix3 tm = arg_list[0]->to_matrix3();

	ViewExp* vpt = MAXScript_interface->GetActiveViewport();
	if(vpt)
	{
		result = vpt->SetAffineTM(tm);
		MAXScript_interface->ReleaseViewport(vpt);
	}

	if(result)
	{
		needs_complete_redraw_set();
		return &true_value;
	}

	return &false_value;
}


// ============================================================================
Value* VP_GetTM_cf(Value **arg_list, int count)
{
	check_arg_count(GetTM, 0, count);

	Matrix3 tm;
	tm.IdentityMatrix();

	ViewExp* vpt = MAXScript_interface->GetActiveViewport();
	if(vpt)
	{
		vpt->GetAffineTM(tm);
		MAXScript_interface->ReleaseViewport(vpt);
	}

	return new Matrix3Value(tm);
}


// ============================================================================
Value* VP_SetCamera_cf(Value **arg_list, int count)
{
	check_arg_count(SetCamera, 1, count);

	INode *camnode = arg_list[0]->to_node();
	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	if(vpt && camnode)
	{
		ObjectState os = camnode->EvalWorldState(MAXScript_interface->GetTime());
		if(os.obj && os.obj->SuperClassID() == CAMERA_CLASS_ID)
		{
			vpt->SetViewCamera(camnode);
			needs_complete_redraw_set();
		}
		else if(os.obj && os.obj->SuperClassID() == LIGHT_CLASS_ID) // LAM - 9/5/01
		{
			LightState ls;
			Interval iv;

			((LightObject *)os.obj)->EvalLightState(MAXScript_interface->GetTime(),iv,&ls);
			if (ls.type == SPOT_LGT || ls.type == DIRECT_LGT)
			{
				vpt->SetViewSpot(camnode);
				needs_complete_redraw_set();
			}
		}
	}
	if (vpt)
		MAXScript_interface->ReleaseViewport(vpt);

	return &ok;
}


//  =================================================

Value*
VP_CanSetToViewport_cf(Value** arg_list, int count) 
{
   check_arg_count(canSetToViewport, 1, count);
   Value* ret = &false_value;

   INode *camnode = arg_list[0]->to_node();
	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	if(vpt && camnode)
	{
		ObjectState os = camnode->EvalWorldState(MAXScript_interface->GetTime());
		if(os.obj && os.obj->SuperClassID() == CAMERA_CLASS_ID)
		{
			ret = &true_value;
		}
		else if(os.obj && os.obj->SuperClassID() == LIGHT_CLASS_ID) // LAM - 9/5/01
		{
			LightState ls;
			Interval iv;

			((LightObject *)os.obj)->EvalLightState(MAXScript_interface->GetTime(),iv,&ls);
			if (ls.type == SPOT_LGT || ls.type == DIRECT_LGT)
			{
				ret = &true_value;
			}
		}
	}
 
	if (vpt)
		MAXScript_interface->ReleaseViewport(vpt);
	
	return ret;
}


// ============================================================================
Value* VP_GetCamera_cf(Value **arg_list, int count)
{
	check_arg_count(GetCamera, 0, count);

	INode *camnode = NULL;
	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	if(vpt)
	{
		camnode = vpt->GetViewCamera();
		if (camnode == NULL)	// LAM - 9/5/01
			camnode = vpt->GetViewSpot();
		MAXScript_interface->ReleaseViewport(vpt);
	}

	if(camnode) return MAXNode::intern(camnode);
	return &undefined;
}


// ============================================================================
Value* VP_SetLayout_cf(Value **arg_list, int count)
{
	check_arg_count(SetLayout, 1, count);
	def_vp_layouts();

	int layoutType = GetID(vpLayouts, elements(vpLayouts), arg_list[0], -1);
	if(layoutType != -1)
	{
		MAXScript_interface->SetViewportLayout(layoutType);
		needs_redraw_set();
	}

	return &ok;
}


// ============================================================================
Value* VP_GetLayout_cf(Value **arg_list, int count)
{
	check_arg_count(GetLayout, 0, count);
	def_vp_layouts();

	Value *layoutKey = GetName(vpLayouts,
	                           elements(vpLayouts),
	                           MAXScript_interface->GetViewportLayout(),
	                           &undefined);
	return layoutKey;
}

// ============================================================================
Value* VP_SetRenderLevel_cf(Value **arg_list, int count)
{
	check_arg_count(SetRenderLevel, 1, count);
	def_vp_renderlevel();

	int renderLevel = GetID(vpRenderLevel, elements(vpRenderLevel), arg_list[0], -1);
	if(renderLevel != -1)
	{
		MAXScript_interface7->SetActiveViewportRenderLevel(renderLevel);
		needs_redraw_set();
	}

	return &ok;
}


// ============================================================================
Value* VP_GetRenderLevel_cf(Value **arg_list, int count)
{
	check_arg_count(GetRenderLevel, 0, count);
	def_vp_renderlevel();

	Value *renderLevelKey = GetName(vpRenderLevel,
	                           elements(vpRenderLevel),
	                           MAXScript_interface7->GetActiveViewportRenderLevel(),
	                           &undefined);
	return renderLevelKey;
}

// ============================================================================
Value* VP_SetShowEdgeFaces_cf(Value **arg_list, int count)
{
	check_arg_count(SetShowEdgeFaces, 1, count);

	BOOL show = arg_list[0]->to_bool();

	MAXScript_interface7->SetActiveViewportShowEdgeFaces(show);
	needs_redraw_set();

	return &ok;
}


// ============================================================================
Value* VP_GetShowEdgeFaces_cf(Value **arg_list, int count)
{
	check_arg_count(GetShowEdgeFaces, 0, count);

	BOOL show = MAXScript_interface7->GetActiveViewportShowEdgeFaces();

	return bool_result(show);
}

// ============================================================================
Value* VP_SetTransparencyLevel_cf(Value **arg_list, int count)
{
	check_arg_count(SetTransparencyLevel, 1, count);

	int level = arg_list[0]->to_int()-1;

	MAXScript_interface7->SetActiveViewportTransparencyLevel(level);
	needs_redraw_set();

	return &ok;
}


// ============================================================================
Value* VP_GetTransparencyLevel_cf(Value **arg_list, int count)
{
	check_arg_count(GetTransparencyLevel, 0, count);

	int level = MAXScript_interface7->GetActiveViewportTransparencyLevel()+1;

	return Integer::intern(level);
}


// RK: Start

Value* 
VP_SetActiveViewport(Value *val)
{
	int index = val->to_int();
	if (index < 1 || index > MAXScript_interface7->getNumViewports())
		throw RuntimeError(GetString(IDS_RK_ACTIVEVIEWPORT_INDEX_OUT_OF_RANGE));
	MAXScript_interface7->setActiveViewport(index-1);
	return val;
}

Value* 
VP_GetActiveViewport()
{
	return Integer::intern(MAXScript_interface7->getActiveViewportIndex()+1);
}

Value* VP_SetNumViews(Value* val)
{
	throw RuntimeError (GetString(IDS_RK_CANNOT_SET_NUMVIEWS));
	return NULL; // WIN64 Cleanup: Shuler -> Compiler wants a value returned.
}

Value* 
VP_GetNumViews()
{
	return Integer::intern(MAXScript_interface7->getNumViewports());
}

Value* VP_ResetAllViews_cf(Value **arg_list, int count)
{
	check_arg_count(resetAllViews, 0, count);
	MAXScript_interface7->resetAllViews();
	return &ok;
}

// RK: End

Value* 
VP_ZoomToBounds_cf(Value **arg_list, int count)
{
	check_arg_count(ZoomToBounds, 3, count);
	BOOL all = arg_list[0]->to_bool();
	Point3 a = arg_list[1]->to_point3();
	Point3 b = arg_list[2]->to_point3();
	Box3 box(a,b);
	MAXScript_interface->ZoomToBounds(all,box);

	return &ok;
}

// LAM: Start

#ifndef NO_REGIONS
Value* 
VP_SetRegionRect_cf(Value **arg_list, int count)
{
	check_arg_count_with_keys(SetRegionRect, 2, count);
	int index = arg_list[0]->to_int();
	if (index < 1 || index > MAXScript_interface7->getNumViewports())
		throw RuntimeError(GetString(IDS_RK_ACTIVEVIEWPORT_INDEX_OUT_OF_RANGE));
	Rect region = arg_list[1]->to_box2();
	Value* tmp;
	BOOL byPixel = bool_key_arg(byPixel, tmp, TRUE); 
	if (byPixel) 
		MAXScript_interface7->SetRegionRect2(index-1,region);
	else
		MAXScript_interface7->SetRegionRect(index-1,region);
	return &ok;
}

Value* 
VP_GetRegionRect_cf(Value **arg_list, int count)
{
	check_arg_count_with_keys(GetRegionRect, 1, count);
	int index = arg_list[0]->to_int();
	if (index < 1 || index > MAXScript_interface7->getNumViewports())
		throw RuntimeError(GetString(IDS_RK_ACTIVEVIEWPORT_INDEX_OUT_OF_RANGE));
	Rect region;
	Value* tmp;
	BOOL byPixel = bool_key_arg(byPixel, tmp, TRUE); 
	if (byPixel) 
		region = MAXScript_interface7->GetRegionRect2(index-1);
	else
		region = MAXScript_interface7->GetRegionRect(index-1);
	return new Box2Value(region);
}

Value* 
VP_SetBlowupRect_cf(Value **arg_list, int count)
{
	check_arg_count_with_keys(SetBlowupRect, 2, count);
	int index = arg_list[0]->to_int();
	if (index < 1 || index > MAXScript_interface7->getNumViewports())
		throw RuntimeError(GetString(IDS_RK_ACTIVEVIEWPORT_INDEX_OUT_OF_RANGE));
	Rect region = arg_list[1]->to_box2();
	Value* tmp;
	BOOL byPixel = bool_key_arg(byPixel, tmp, TRUE); 
	if (byPixel) 
		MAXScript_interface7->SetBlowupRect2(index-1,region);
	else
		MAXScript_interface7->SetBlowupRect(index-1,region);
	return &ok;
}

Value* 
VP_GetBlowupRect_cf(Value **arg_list, int count)
{
	check_arg_count_with_keys(GetBlowupRect, 1, count);
	int index = arg_list[0]->to_int();
	if (index < 1 || index > MAXScript_interface7->getNumViewports())
		throw RuntimeError(GetString(IDS_RK_ACTIVEVIEWPORT_INDEX_OUT_OF_RANGE));
	Rect region;
	Value* tmp;
	BOOL byPixel = bool_key_arg(byPixel, tmp, TRUE); 
	if (byPixel) 
		region = MAXScript_interface7->GetBlowupRect2(index-1);
	else
		region = MAXScript_interface7->GetBlowupRect(index-1);
	return new Box2Value(region);
}
#endif //NO_REGIONS

// LAM - defect 289932
Value* 
VP_SetGridVisibility_cf(Value **arg_list, int count)
{
	check_arg_count(setGridVisibility, 2, count);
	BOOL state = arg_list[1]->to_bool();
	if (arg_list[0] == n_all)
	{
		for (int index = 0; index < MAXScript_interface7->getNumViewports(); index++)
		MAXScript_interface7->SetViewportGridVisible(index,state);
	}
	else
	{
		int index = arg_list[0]->to_int();
		if (index < 1 || index > MAXScript_interface7->getNumViewports())
			throw RuntimeError(GetString(IDS_RK_ACTIVEVIEWPORT_INDEX_OUT_OF_RANGE));
		MAXScript_interface7->SetViewportGridVisible(index-1,state);
	}
	needs_redraw_set();
	return &ok;
}

Value* 
VP_GetGridVisibility_cf(Value **arg_list, int count)
{
	check_arg_count(getGridVisibility, 1, count);
	int index = arg_list[0]->to_int();
	if (index < 1 || index > MAXScript_interface7->getNumViewports())
		throw RuntimeError(GetString(IDS_RK_ACTIVEVIEWPORT_INDEX_OUT_OF_RANGE));
	return bool_result (MAXScript_interface7->GetViewportGridVisible(index-1));
}

// LAM: End

/*
// ============================================================================
Value* VP_IsActive_cf(Value **arg_list, int count)
{
	check_arg_count(IsActive, 0, count);

	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	BOOL isActive = vpt->IsActive();
	MAXScript_interface->ReleaseViewport(vpt);

	return (isActive) ? &true_value : &false_value;
}
*/

// ============================================================================
Value* VP_IsEnabled_cf(Value **arg_list, int count)
{
	check_arg_count(IsEnabled, 0, count);

	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	BOOL isEnabled = vpt->IsEnabled();
	MAXScript_interface->ReleaseViewport(vpt);

	return (isEnabled) ? &true_value : &false_value;
}


// ============================================================================
Value* GetFOV_cf(Value **arg_list, int count)
{
	check_arg_count(GetFOV, 0, count);

	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	float fov = vpt->GetFOV() * (180.f/PI);
	MAXScript_interface->ReleaseViewport(vpt);

	return Float::intern(fov);
}


// ============================================================================
Value* GetScreenScaleFactor_cf(Value **arg_list, int count)
{
	check_arg_count(GetScreenScaleFactor, 1, count);

	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	float screenScaleFactor = vpt->GetScreenScaleFactor(arg_list[0]->to_point3());
	MAXScript_interface->ReleaseViewport(vpt);

	return Float::intern(screenScaleFactor);
}



// ============================================================================
Value* VP_IsWire_cf(Value **arg_list, int count)
{
	check_arg_count(IsWire, 0, count);

	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	BOOL isWire = vpt->IsWire();
	MAXScript_interface->ReleaseViewport(vpt);

	return (isWire) ? &true_value : &false_value;
}
/*
Value* 
VP_SetGridSize(Value *val)
{
	float size = val->to_float();
	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	vpt->SetGridSize(size);
	MAXScript_interface->ReleaseViewport(vpt);
	needs_redraw_set();
	return val;
}

Value* 
VP_GetGridSize()
{
	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	float size = vpt->GetGridSize();
	MAXScript_interface->ReleaseViewport(vpt);
	return Float::intern(size);
}
*/
Value* 
VP_SetBkgImageDsp(Value *val)
{
	BOOL on = val->to_bool();
	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	vpt->setBkgImageDsp(on);
	MAXScript_interface->ReleaseViewport(vpt);
	needs_redraw_set();
	return val;
}

Value* 
VP_GetBkgImageDsp()
{
	ViewExp *vpt = MAXScript_interface->GetActiveViewport();
	BOOL on = vpt->getBkgImageDsp();
	MAXScript_interface->ReleaseViewport(vpt);
	return bool_result(on);
}

/* --------------------- plug-in init --------------------------------- */
// this is called by the dlx initializer, register the global vars here
void sf_init()
{
#include "sf_glbls.h"
}

