// NOTE: Everything in here except for getSegLengths_cf and possibly subdivideSegment_cf 
// OK for public consumption. These methods use code plagerized from Spline3D.

// Note: Integration: fix up include path to triobjed.h

/*	Adds new primitives/globals
 *  Larry Minton, 1999
 */

#include "avg_maxver.h"

#include "MAXScrpt.h"
#include "Numbers.h"
#include "3DMath.h"
#include "MAXObj.h"
#include "Strings.h"
#include "Arrays.h"

#include "MAXMats.h"
#include "MAXclses.h"
#include "ColorVal.h"
#include "Parser.h"
#include "Listener.h"
#include "Structs.h"
#include "MSPlugin.h"
#include "mscustattrib.h"
#include "funcs.h"
#include "thunks.h"

#include "bitmaps.h"

#include "resource.h"
//#include "avg_MSPlugIn.h"

#include "SPLSHAPE.h"
#include "modstack.h"
#include "interactiverender.h"
#include "buildver.h"
#include "IMtlRender_Compatibility.h"

#ifdef ScripterExport
	#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

#include "MXSAgni.h"
#include "definsfn.h"
#include "avg_dlx.h"
#include "agnidefs.h"

#include <locale.h>
#include <Wininet.h>

#include "ICustAttribContainer.h"
#include "CustAttrib.h"

#include "xref\iXrefItem.h"
#include "3dsmaxport.h"

// ============================================================================

#include "defextfn.h"
#	include "namedefs.h"

#include "definsfn.h"
	#include "lam_wraps.h"

//	def_visible_primitive	( tester,				"tester");

//  =================================================
//  Spline methods
//  =================================================

#define NUMARCSTEPS 100

Value*
getSegLengths_cf(Value** arg_list, int count)
{
	// getSegLengths <splineShape> <spline_index> [cum:<boolean>] [byVertex:<boolean>] [numArcSteps:<integer>]
	check_arg_count_with_keys(getSegLengths, 2, count);
	MAXNode* shape = (MAXNode*)arg_list[0];
	type_check(shape, MAXNode, _T("getSegLengths"));
	deletion_check(shape);
	Value* tmp;
	BOOL cum = bool_key_arg(cum, tmp, FALSE); 
	BOOL byVertex = bool_key_arg(byVertex, tmp, FALSE); 
	int numArcSteps = int_key_arg(numArcSteps, tmp, NUMARCSTEPS); 
	int index = arg_list[1]->to_int();
	BezierShape* s = set_up_spline_access(shape->node, index);
	Spline3D* s3d = s->splines[index-1];
	s3d->ComputeBezPoints();
	int segs = s3d->Segments();
	int knotCount = s3d->KnotCount();
	one_typed_value_local(Array* result);
	vl.result = new Array (segs);
	double totalLength = 0.0;
	double uStep=(double)1.0/numArcSteps;
	double *segLen = new double [segs];

	for(int seg = 0; seg < segs; ++seg) {
		DPoint3 k1p = s3d->GetKnotPoint(seg);
		DPoint3 k1o = s3d->GetOutVec(seg);
		int seg2 = (seg+1) % knotCount;
		DPoint3 k2p = s3d->GetKnotPoint(seg2);
		DPoint3 k2i = s3d->GetInVec(seg2);
		BOOL segIsLine = s3d->GetLineType(seg) == LTYPE_LINE;
		DPoint3 pa = k1p;
		DPoint3 pb;
		double segLength = 0.0;
		for(int arcStep = 1; arcStep < numArcSteps; ++arcStep) {
			double t = uStep*arcStep;
			if (segIsLine) pb = k1p + ((k2p - k1p) * t);
			else {
				double s = (double)1.0-t;
				pb = s*s*s*k1p + t*s*3.0*(k1o*s + k2i*t) + t*t*t*k2p;
			}
			segLength += Length(pb - pa);
			pa = pb;
			}					
		pb = k2p;
		segLength += Length(pb - pa);
		segLen[seg] = segLength;
		totalLength += segLength;
		}

	double cummVal=0.0;
	if (byVertex) vl.result->append(Float::intern((float)cummVal));
	double thisVal;
   for(int seg = 0; seg < segs; ++seg) {
		thisVal=segLen[seg]/totalLength+cummVal;
		vl.result->append(Float::intern((float)thisVal));
		if (cum) cummVal=thisVal;
	}

	cummVal=0.0;
	if (byVertex) vl.result->append(Float::intern((float)cummVal));
   for(int seg = 0; seg < segs; ++seg) {
		thisVal=segLen[seg]+cummVal;
		vl.result->append(Float::intern((float)thisVal));
		if (cum) cummVal=thisVal;
	}

	vl.result->append(Float::intern((float)totalLength));
	delete [] segLen;
	return_value(vl.result);
}

Value*
subdivideSegment_cf(Value** arg_list, int count)
{
	// subdivideSegment <shape> <spline_index> <seg_index> <divisions>   -- return ok
// TODO: check to see if the line and knot types are correct.
	check_arg_count(subdivideSegment, 4, count);
	MAXNode* MAXshape = (MAXNode*)arg_list[0];
	type_check(MAXshape, MAXNode, _T("subdivideSegment"));
	deletion_check(MAXshape);
	int sp_index = arg_list[1]->to_int();
	BezierShape* shape = set_up_spline_access(MAXshape->node, sp_index);
	Spline3D* spline = shape->splines[sp_index-1];
	int seg_index = arg_list[2]->to_int();
	range_check(seg_index, 1, spline->Segments(), GetString(IDS_SPLINE_SEGMENT_INDEX_OUT_OF_RANGE))
	int knots = spline->KnotCount();
	int segs = spline->Segments();
	int segment = seg_index - 1;
	int nextSeg = (segment + 1) % knots;
	int insertSeg = (nextSeg == 0) ? -1 : nextSeg;
	int numDivisions = arg_list[3]->to_int();
	if (numDivisions <= 0) return &ok;
	spline->ComputeBezPoints();

	// Get the initial knot points
	DPoint3 v00 = spline->GetKnotPoint(segment); // prev knot location
	DPoint3 v30 = spline->GetKnotPoint(nextSeg); // next knot location

	// Special: If they're refining a line-type segment, force it to be a bezier curve again
	if(spline->GetLineType(segment) == LTYPE_LINE) {
		spline->SetKnotType(segment, KTYPE_BEZIER_CORNER);
		spline->SetKnotType(nextSeg, KTYPE_BEZIER_CORNER);
		spline->SetLineType(segment, LTYPE_CURVE);
      spline->SetOutVec(segment, DPoint3toPoint3 (v00 + (v30 - v00) / 3.0));
      spline->SetInVec(nextSeg, DPoint3toPoint3 (v30 - (v30 - v00) / 3.0));
		}

	DPoint3 v10 = spline->GetOutVec(segment);  // prev knot out vector
	DPoint3 v20 = spline->GetInVec(nextSeg);   // next knot in vector
	DPoint3 v01;
	for(int divStep = numDivisions; divStep > 0; --divStep) {
		double param=(double)divStep/(double)(divStep+1);

		        v01 = v00 + (v10 - v00) * param; // new prev knot out vector
		DPoint3 v21 = v20 + (v30 - v20) * param; // new next knot in vector
		DPoint3 v11 = v10 + (v20 - v10) * param;
		DPoint3 v02 = v01 + (v11 - v01) * param; // new knot in vector
		DPoint3 v12 = v11 + (v21 - v11) * param; // new knot out vector
		DPoint3 v03 = v02 + (v12 - v02) * param; // new knot position

		spline->SetInVec(nextSeg, DPoint3toPoint3 (v21));

		SplineKnot newKnot(KTYPE_BEZIER, LTYPE_CURVE, DPoint3toPoint3 (v03), DPoint3toPoint3 (v02), DPoint3toPoint3 (v12));
		spline->AddKnot(newKnot, insertSeg);
		v20 = v02;
		v10 = v01;
		v30 = v03;
		nextSeg=insertSeg=seg_index;
		}					
	spline->SetOutVec(segment, DPoint3toPoint3 (v01));

	spline->ComputeBezPoints();
	shape->InvalidateGeomCache();

	// Now adjust the spline selection sets
	BitArray& vsel = shape->vertSel[sp_index-1];
	BitArray& ssel = shape->segSel[sp_index-1];
	vsel.SetSize(spline->Verts(), 1);
	int where = (segment + 1) * 3;
	vsel.Shift(RIGHT_BITSHIFT, 3 * numDivisions, where);
	vsel.Clear(where);
	vsel.Clear(where+1);
	vsel.Clear(where+2);
	ssel.SetSize(spline->Segments(), 1);
	ssel.Shift(RIGHT_BITSHIFT, numDivisions, segment + 1);
	for(int i = 1; i > numDivisions; ++i) {ssel.Set(segment+i,ssel[segment]);}
	return  &ok;
}

Value*
interpCurve3D_cf(Value** arg_list, int count)
{
	// interpCurve3D <splineShape> <spline_index> <param_float> [pathParam:<boolean>]
	check_arg_count_with_keys(interpCurve3D, 3, count);
	MAXNode* shape = (MAXNode*)arg_list[0];
	type_check(shape, MAXNode, _T("interpCurve3D"));
	deletion_check(shape);
	Value* tmp;
	int ptype = (bool_key_arg(pathParam, tmp, FALSE)) ? SPLINE_INTERP_SIMPLE : SPLINE_INTERP_NORMALIZED; 
	int index = arg_list[1]->to_int();
	BezierShape* s = set_up_spline_access(shape->node, index);
	Spline3D* s3d = s->splines[index-1];
	Point3 p = s3d->InterpCurve3D(arg_list[2]->to_float(), ptype);
	shape->object_to_current_coordsys(p);
	return new Point3Value(p);
}

Value*
tangentCurve3D_cf(Value** arg_list, int count)
{
	// tangentCurve3D <splineShape> <spline_index> <param_float> [pathParam:<boolean>]
	check_arg_count_with_keys(tangentCurve3D, 3, count);
	MAXNode* shape = (MAXNode*)arg_list[0];
	type_check(shape, MAXNode, _T("tangentCurve3D"));
	deletion_check(shape);
	Value* tmp;
	int ptype = (bool_key_arg(pathParam, tmp, FALSE)) ? SPLINE_INTERP_SIMPLE : SPLINE_INTERP_NORMALIZED; 
	int index = arg_list[1]->to_int();
	BezierShape* s = set_up_spline_access(shape->node, index);
	Spline3D* s3d = s->splines[index-1];
	Point3 p = s3d->TangentCurve3D(arg_list[2]->to_float(), ptype);
	shape->object_to_current_coordsys_rotate(p);
	return new Point3Value(p);
}

Value*
setMaterialID_cf(Value** arg_list, int count)
{
	// setMaterialID <shape> <spline_index> <seg_index> <matID>   -- return ok
	check_arg_count(setMaterialID, 4, count);
	MAXNode* MAXshape = (MAXNode*)arg_list[0];
	type_check(MAXshape, MAXNode, _T("setMaterialID"));
	deletion_check(MAXshape);
	int sp_index = arg_list[1]->to_int();
	BezierShape* shape = set_up_spline_access(MAXshape->node, sp_index);
	Spline3D* spline = shape->splines[sp_index-1];
	int seg_index = arg_list[2]->to_int();
	range_check(seg_index, 1, spline->Segments(), GetString(IDS_SPLINE_SEGMENT_INDEX_OUT_OF_RANGE))
	spline->SetMatID(seg_index-1, arg_list[3]->to_int() - 1);
	return  &ok;
}

Value*
getMaterialID_cf(Value** arg_list, int count)
{
	// getMaterialID <shape> <spline_index> <seg_index>
	check_arg_count(getMaterialID, 3, count);
	MAXNode* MAXshape = (MAXNode*)arg_list[0];
	type_check(MAXshape, MAXNode, _T("getMaterialID"));
	deletion_check(MAXshape);
	int sp_index = arg_list[1]->to_int();
	BezierShape* shape = set_up_spline_access(MAXshape->node, sp_index);
	Spline3D* spline = shape->splines[sp_index-1];
	int seg_index = arg_list[2]->to_int();
	range_check(seg_index, 1, spline->Segments(), GetString(IDS_SPLINE_SEGMENT_INDEX_OUT_OF_RANGE))
	return Integer::intern((int)spline->GetMatID(seg_index-1) + 1);
}

Value*
affectRegionVal_cf(Value** arg_list, int count)
{
	// affectRegionVal <distance_float> <falloff_float> <pinch_float> <bubble_float>
	check_arg_count(affectRegionVal, 4, count);
	float res = AffectRegionFunction(arg_list[0]->to_float(), arg_list[1]->to_float(),
									 arg_list[2]->to_float(), arg_list[3]->to_float());
	return Float::intern(res);
}


//  =================================================
//  Node and Node TM methods
//  =================================================

Value*
getTransformAxis_cf(Value** arg_list, int count)
{
	check_arg_count(getTransformAxis, 2, count);
	INode* node = NULL;
	if (arg_list[0] != &undefined) node = get_valid_node(arg_list[0], getTransformAxis);
	int index = arg_list[1]->to_int()+1;
	return new Matrix3Value(MAXScript_interface->GetTransformAxis(node,index));
}

Value*
invalidateTM_cf(Value** arg_list, int count)
{
	check_arg_count(InvalidateTM, 1, count);
	INode* node = get_valid_node(arg_list[0], InvalidateTM);
	node->InvalidateTM();
	needs_redraw_set();
	return &ok;
}

Value*
invalidateTreeTM_cf(Value** arg_list, int count)
{
	check_arg_count(InvalidateTreeTM, 1, count);
	INode* node = get_valid_node(arg_list[0], InvalidateTreeTM);
	node->InvalidateTreeTM();
	needs_redraw_set();
	return &ok;
}

Value*
invalidateWS_cf(Value** arg_list, int count)
{
	check_arg_count(invalidateWS, 1, count);
	INode* node = get_valid_node(arg_list[0], invalidateWS);
	node->InvalidateWS();
	MAXScript_interface7->InvalidateObCache(node);
	needs_redraw_set();
	return &ok;
}

Value*
snapshotAsMesh_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(snapshot,1, count);
	INode* node = get_valid_node(arg_list[0], snapshotAsMesh);
	Value* tmp;
	BOOL renderMesh = bool_key_arg(renderMesh, tmp, TRUE); 
	Mesh *resMesh = getWorldStateMesh(node, renderMesh);
	Value *resMesh2 = resMesh ? (Value*)(new MeshValue (resMesh,TRUE)) : &undefined;
//	if (resMesh) delete resMesh;
	return resMesh2;
}

//  =================================================
//  Node Selection Lock methods
//  =================================================

Value*
isSelectionFrozen_cf(Value** arg_list, int count)
{
	check_arg_count(isSelectionFrozen, 0, count);
	return ((MAXScript_interface->SelectionFrozen()) ? &true_value : &false_value);
}

Value*
freezeSelection_cf(Value** arg_list, int count)
{
	check_arg_count(freezeSelection, 0, count);
	MAXScript_interface->FreezeSelection();
	return &ok;
}

Value*
thawSelection_cf(Value** arg_list, int count)
{
	check_arg_count(thawSelection, 0, count);
	MAXScript_interface->ThawSelection();
	return &ok;
}


Value*
getInheritanceFlags_cf(Value** arg_list, int count)
{
	check_arg_count(getInheritanceFlags, 1, count);
	INode* node = get_valid_node(arg_list[0], getInheritanceFlags);
	BitArray flags(9);
	DWORD iflags = node->GetTMController()->GetInheritanceFlags();
   for (int i = 0, j = 1; i < 9; i++, j= (j << 1)) 
		flags.Set(i,!(iflags & j));
	return new BitArrayValue(flags);
}

Value*
setInheritanceFlags_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(setInheritanceFlags, 2, count);
	INode* node = get_valid_node(arg_list[0], setInheritanceFlags);
	BitArray flags(9);
	Value* tmp;
	BOOL keepPos = bool_key_arg(keepPos, tmp, TRUE); 
	flags.ClearAll();
	if (arg_list[1] == n_all)
		flags.SetAll();
	else if (arg_list[1] != n_none) {
		flags = arg_list[1]->to_bitarray();
		flags.SetSize(9,1);
	}
	DWORD intFlag=0;
   for (int i = 0, j = 1; i < 9; i++, j= (j << 1)) 
		if (flags[i]) intFlag += j;
	node->GetTMController()->SetInheritanceFlags(intFlag,keepPos);
   BOOL suspendPanel = node->Selected();
   if (suspendPanel)
   {
      MAXScript_interface7->SuspendEditing(1<<TASK_MODE_HIERARCHY);
      MAXScript_interface7->ResumeEditing(1<<TASK_MODE_HIERARCHY);
   }
	return &ok;
}

Value*
getTransformLockFlags_cf(Value** arg_list, int count)
{
	check_arg_count(getTransformLockFlags, 1, count);
	INode* node = get_valid_node(arg_list[0], getTransformLockFlags);
	BitArray flags(9);
	flags.Set(0,node->GetTransformLock(INODE_LOCKPOS, INODE_LOCK_X));
	flags.Set(1,node->GetTransformLock(INODE_LOCKPOS, INODE_LOCK_Y));
	flags.Set(2,node->GetTransformLock(INODE_LOCKPOS, INODE_LOCK_Z));
	flags.Set(3,node->GetTransformLock(INODE_LOCKROT, INODE_LOCK_X));
	flags.Set(4,node->GetTransformLock(INODE_LOCKROT, INODE_LOCK_Y));
	flags.Set(5,node->GetTransformLock(INODE_LOCKROT, INODE_LOCK_Z));
	flags.Set(6,node->GetTransformLock(INODE_LOCKSCL, INODE_LOCK_X));
	flags.Set(7,node->GetTransformLock(INODE_LOCKSCL, INODE_LOCK_Y));
	flags.Set(8,node->GetTransformLock(INODE_LOCKSCL, INODE_LOCK_Z));
	return new BitArrayValue(flags);
}

Value*
setTransformLockFlags_cf(Value** arg_list, int count)
{
	check_arg_count(setTransformLockFlags, 2, count);
	INode* node = get_valid_node(arg_list[0], setTransformLockFlags);
	BitArray flags(9);
	flags.ClearAll();
	if (arg_list[1] == n_all)
		flags.SetAll();
	else if (arg_list[1] != n_none) {
		flags = arg_list[1]->to_bitarray();
		flags.SetSize(9,1);
	}
	node->SetTransformLock(INODE_LOCKPOS, INODE_LOCK_X, flags[0]);
	node->SetTransformLock(INODE_LOCKPOS, INODE_LOCK_Y, flags[1]);
	node->SetTransformLock(INODE_LOCKPOS, INODE_LOCK_Z, flags[2]);
	node->SetTransformLock(INODE_LOCKROT, INODE_LOCK_X, flags[3]);
	node->SetTransformLock(INODE_LOCKROT, INODE_LOCK_Y, flags[4]);
	node->SetTransformLock(INODE_LOCKROT, INODE_LOCK_Z, flags[5]);
	node->SetTransformLock(INODE_LOCKSCL, INODE_LOCK_X, flags[6]);
	node->SetTransformLock(INODE_LOCKSCL, INODE_LOCK_Y, flags[7]);
	node->SetTransformLock(INODE_LOCKSCL, INODE_LOCK_Z, flags[8]);
   BOOL suspendPanel = node->Selected();
   if (suspendPanel)
   {
      MAXScript_interface7->SuspendEditing(1<<TASK_MODE_HIERARCHY);
      MAXScript_interface7->ResumeEditing(1<<TASK_MODE_HIERARCHY);
   }
	return &ok;
}

//  =================================================
//  Listener methods
//  =================================================

Value*
get_listener()
{
	return the_listener->edit_stream;
}

Value*
get_macroRecorder()
{
	return the_listener->macrorec_stream;
}

//  =================================================
//  Sound methods
//  =================================================

Value*
setSoundFileName(Value* val)
{
	// Get the current sound object.
	SoundObj *snd = MAXScript_interface->GetSoundObject();
	
	// See if we can get a wave interface
	IWaveSound *iWav = GetWaveSoundInterface(snd);
	if (iWav) {
		// Set the sound file
		iWav->SetSoundFileName(val->to_filename());
	}
	return val;
}

Value*
getSoundFileName()
{
	// Get the current sound object.
	SoundObj *snd = MAXScript_interface->GetSoundObject();
	
	// See if we can get a wave interface
	IWaveSound *iWav = GetWaveSoundInterface(snd);
	if (iWav) 
		// Get the sound file
		return new String (iWav->GetSoundFileName());
	else 
		return &undefined;
}

Value*
setSoundStartTime(Value* val)
{
	// Get the current sound object.
	SoundObj *snd = MAXScript_interface->GetSoundObject();
	
	// See if we can get a wave interface
	IWaveSound *iWav = GetWaveSoundInterface(snd);
	if (iWav)
		// Set the sound file start time
		iWav->SetStartTime(val->to_timevalue());
	return val;
}

Value*
getSoundStartTime()
{
	// Get the current sound object.
	SoundObj *snd = MAXScript_interface->GetSoundObject();
	
	// See if we can get a wave interface
	IWaveSound *iWav = GetWaveSoundInterface(snd);
	if (iWav) 
		// Get the sound file start time
		return MSTime::intern(iWav->GetStartTime());
	else 
		return &undefined;
}

Value*
getSoundEndTime()
{
	// Get the current sound object.
	SoundObj *snd = MAXScript_interface->GetSoundObject();
	
	// See if we can get a wave interface
	IWaveSound *iWav = GetWaveSoundInterface(snd);
	if (iWav) 
		// Get the sound file end time
		return MSTime::intern(iWav->GetEndTime());
	else 
		return &undefined;
}

Value*
setSoundMute(Value* val)
{
	// Get the current sound object.
	SoundObj *snd = MAXScript_interface->GetSoundObject();
	snd->SetMute(val->to_bool());
	return val;
}

Value*
getSoundMute()
{
	// Get the current sound object.
	SoundObj *snd = MAXScript_interface->GetSoundObject();
	return ((snd->IsMute()) ? &true_value : &false_value);
}

Value*
getSoundIsPlaying()
{
	// Get the current sound object.
	SoundObj *snd = MAXScript_interface->GetSoundObject();
	return ((snd->Playing()) ? &true_value : &false_value);
}

Value*
SoundScrub_cf(Value** arg_list, int count)
{
	check_arg_count(setSoundMute, 1, count);
	// Get the current sound object.
	SoundObj *snd = MAXScript_interface->GetSoundObject();
	Interval range = arg_list[0]->to_interval();
	snd->Scrub(range.Start(), range.End());
	return &ok;
}

//  =================================================
//  Property controller methods
//  =================================================

Value*
getPropertyController_cf(Value** arg_list, int count)
{
	// getPropertyController <value> <string_or_name>
	check_arg_count(getPropertyController, 2, count);
	Control* c;
	Value* prop = arg_list[1];
	if (is_string(prop)) prop = Name::intern(prop->to_string());
	type_check(prop, Name, _T("getPropertyController"));
	ParamDimension* dim;
	if ((c = MAXWrapper::get_max_prop_controller(arg_list[0]->to_reftarg(), prop, &dim)) != NULL)
		return MAXControl::intern(c, dim);
	else
		return &undefined;
}

Value*
setPropertyController_cf(Value** arg_list, int count)
{
	// setPropertyController <value> <string_or_name> <controller>
	check_arg_count(setPropertyController, 3, count);

	MAXControl* c = (MAXControl*)arg_list[2]; 
	Value* prop = arg_list[1];
	if (is_string(prop)) prop = Name::intern(prop->to_string());
	type_check(prop, Name, _T("setPropertyController"));

	if (!c->is_kind_of(class_tag(MAXControl)))
		throw TypeError (GetString(IDS_ASSIGN_NEEDS_CONTROLLER_GOT), arg_list[2]);
	if (MAXWrapper::set_max_prop_controller(arg_list[0]->to_reftarg(), prop, c)) {
		if (c->flags & MAX_CTRL_NO_DIM) {
			ParamDimension *dim;
			MAXWrapper::get_max_prop_controller(arg_list[0]->to_reftarg(), prop, &dim);
			c->dim = dim;
			c->flags &= ~MAX_CTRL_NO_DIM;
		}
		needs_redraw_set();
		return c;
	}
	else
		return &undefined;
}

Value*
isPropertyAnimatable_cf(Value** arg_list, int count)
{
	// isPropertyAnimatable <value> <string_or_name>
	check_arg_count(isPropertyAnimatable, 2, count);
	Value* prop = arg_list[1];
	if (is_string(prop)) prop = Name::intern(prop->to_string());
	type_check(prop, Name, _T("isPropertyAnimatable"));
	if (arg_list[0]->derives_from_MAXWrapper())
		return bool_result(((MAXWrapper*)(arg_list[0]))->is_max_prop_animatable(prop));
	else
		type_check(arg_list[0], MAXWrapper, _T("isPropertyAnimatable"));
	return &undefined;
}

//  =================================================
//  TransferReferences method
//  =================================================

Value*
replaceInstances_cf(Value** arg_list, int count)
{
	// replaceInstances <old_MAXWrapper> <new_MAXWrapper> transferCAs:<bool>
	check_arg_count_with_keys(replaceInstances, 2, count);

	ReferenceTarget* oldRT = arg_list[0]->to_reftarg();
	ReferenceTarget* newRT = arg_list[1]->to_reftarg();

	Value* dmy;
	BOOL transferCAs = bool_key_arg(transferCAs, dmy, FALSE);

	if (oldRT == NULL || (newRT != NULL && oldRT->SuperClassID() != newRT->SuperClassID()))
		throw RuntimeError (GetString(IDS_CONTROLLER_SUPERCLASSES_NOT_SAME));

	MAXScript_interface7->SuspendEditing(0x00FF);

	// Send out a warning that we're about to paste
	oldRT->NotifyDependents(FOREVER,0,REFMSG_PRENOTIFY_PASTE);

// Transfer all the references from oldRT to newRT. Also transfer the CAs from oldRT to the new

	int i,j,k;
	RefListItem  *ptr;
	Tab<RefMakerHandle> makers;
	
	// Save a list of reference makers that reference the old target.
	RefList& refs = oldRT->GetRefList();
	for (ptr = refs.first; ptr!=NULL; ptr = ptr->next) 
		makers.Append( 1, &ptr->maker, 10 );

	// Now for each maker, replace the reference to oldRT with
	// a reference to this.
	bool first = true;
	for ( i = 0; i < makers.Count(); i++ ) 
	{
		RefMakerHandle makr = makers[i];
		j = makr->FindRef(oldRT);
		if(j >= 0) {
			if (makr->CanTransferReference(j))
			{
				if (first)
				{
					first = false;
					ICustAttribContainer* cc_oldRT = oldRT->GetCustAttribContainer();
					if (newRT != NULL && cc_oldRT && transferCAs)
					{
						if (newRT->GetCustAttribContainer() == NULL) 
							newRT->AllocCustAttribContainer();
						ICustAttribContainer* cc_newRT = newRT->GetCustAttribContainer();
						int ca_count = cc_oldRT->GetNumCustAttribs();
						for (k = 0; k < ca_count; k++)
						{
							CustAttrib *ca = cc_oldRT->GetCustAttrib(k);
							cc_newRT->AppendCustAttrib(ca);
						}
						for (k = ca_count-1; k >= 0; k--)
							cc_oldRT->RemoveCustAttrib(k);
					}
				}
				makr->ReplaceReference(j,newRT);
			}
		}
	}

	// More notifications
	newRT->NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	newRT->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);
	needs_redraw_set();

	MAXScript_interface7->ResumeEditing(0x00FF);

	return &ok;
}

//  =================================================
//  Playback methods
//  =================================================

int indexToSpeed[]={-4,-2,1,2,4};

Value*
setPlaybackSpeed(Value* val)
{
	int speed = val->to_int();
	range_check(speed, 1, 5, GetString(IDS_PLAYBACKSPEED_OUT_OF_RANGE));
	MAXScript_interface->SetPlaybackSpeed(indexToSpeed[speed-1]);
	return val;
}

Value*
getPlaybackSpeed()
{
	int speed = MAXScript_interface->GetPlaybackSpeed();
	for (int i = 0; i < 5; i++) 
		if (indexToSpeed[i] == speed) return Integer::intern(i+1);
	throw RuntimeError (GetString(IDS_BAD_GETPLAYBACKSPEED_RESULT), Integer::intern(speed));
}

//  =================================================
//  IK methods
//  =================================================

Value*
setPinNode_cf(Value** arg_list, int count)
{
	check_arg_count(setPinNode, 2, count);
	int res=0;
	INode* snode = get_valid_node(arg_list[0], setPinNode);
	if (arg_list[1] == &undefined)
		res = snode->SetProperty(PROPID_PINNODE,NULL);
	else
	{	check_arg_count(setPinNode, 2, count);
		INode* node = get_valid_node((MAXNode*)arg_list[1], setPinNode);
		if (snode->GetTMController()->OKToBindToNode(node))
			res = snode->SetProperty(PROPID_PINNODE,node);
	}
	if (res)
	{	needs_redraw_set();
		snode->GetTMController()->NodeIKParamsChanged();
      BOOL suspendPanel = snode->Selected();
      if (suspendPanel)
      {
         MAXScript_interface7->SuspendEditing(1<<TASK_MODE_HIERARCHY);
         MAXScript_interface7->ResumeEditing(1<<TASK_MODE_HIERARCHY);
      }
	}
	return (res ? &true_value : &false_value);
}

Value*
getPinNode_cf(Value** arg_list, int count)
{
	check_arg_count(getPinNode, 1, count);
	INode* node = get_valid_node(arg_list[0], getPinNode);
	INode* bnode= (INode*)node->GetProperty(PROPID_PINNODE);
	return ((bnode == NULL) ? &undefined : MAXNode ::intern(bnode));
}

Value*
getPrecedence_cf(Value** arg_list, int count)
{
	check_arg_count(getPrecedence, 1, count);
	INode* node = get_valid_node(arg_list[0], getPrecedence);
   return Integer::intern(
      static_cast<int>(reinterpret_cast<INT_PTR>(node->GetProperty(PROPID_PRECEDENCE))));
}

Value*
setPrecedence_cf(Value** arg_list, int count)
{
	check_arg_count(setPrecedence, 2, count);
	INode* node = get_valid_node(arg_list[0], setPrecedence);
	int val=arg_list[1]->to_int();
	int res = node->SetProperty(PROPID_PRECEDENCE,&val);
	node->GetTMController()->NodeIKParamsChanged();
   BOOL suspendPanel = node->Selected();
   if (suspendPanel)
   {
      MAXScript_interface7->SuspendEditing(1<<TASK_MODE_HIERARCHY);
      MAXScript_interface7->ResumeEditing(1<<TASK_MODE_HIERARCHY);
   }
	return (res ? &true_value : &false_value);
}

Value*
getIsTerminator_cf(Value** arg_list, int count)
{
	check_arg_count(getIsTerminator, 1, count);
	INode* node = get_valid_node(arg_list[0], getIsTerminator);
	return (node->TestAFlag(A_INODE_IK_TERMINATOR) ? &true_value : &false_value);
}

Value*
setIsTerminator_cf(Value** arg_list, int count)
{
	check_arg_count(setIsTerminator, 2, count);
	INode* node = get_valid_node(arg_list[0], setIsTerminator);
	BOOL val=arg_list[1]->to_bool();
	val ? node->SetAFlag(A_INODE_IK_TERMINATOR) : node->ClearAFlag(A_INODE_IK_TERMINATOR);
	node->GetTMController()->NodeIKParamsChanged();
   BOOL suspendPanel = node->Selected();
   if (suspendPanel)
   {
      MAXScript_interface7->SuspendEditing(1<<TASK_MODE_HIERARCHY);
      MAXScript_interface7->ResumeEditing(1<<TASK_MODE_HIERARCHY);
   }
	return &ok;
}

Value*
getBindPos_cf(Value** arg_list, int count)
{
	check_arg_count(getBindPos, 1, count);
	INode* node = get_valid_node(arg_list[0], getBindPos);
	return (node->TestAFlag(A_INODE_IK_POS_PINNED) ? &true_value : &false_value);
}

Value*
setBindPos_cf(Value** arg_list, int count)
{
	check_arg_count(setBindPos, 2, count);
	INode* node = get_valid_node(arg_list[0], setBindPos);
	BOOL val=arg_list[1]->to_bool();
	val ? node->SetAFlag(A_INODE_IK_POS_PINNED) : node->ClearAFlag(A_INODE_IK_POS_PINNED);
	node->GetTMController()->NodeIKParamsChanged();
	return &ok;
}

Value*
getBindOrient_cf(Value** arg_list, int count)
{
	check_arg_count(getBindOrient, 1, count);
	INode* node = get_valid_node(arg_list[0], getBindOrient);
	return (node->TestAFlag(A_INODE_IK_ROT_PINNED) ? &true_value : &false_value);
}

Value*
setBindOrient_cf(Value** arg_list, int count)
{
	check_arg_count(setBindOrient, 2, count);
	INode* node = get_valid_node(arg_list[0], setBindOrient);
	BOOL val=arg_list[1]->to_bool();
	val ? node->SetAFlag(A_INODE_IK_ROT_PINNED) : node->ClearAFlag(A_INODE_IK_ROT_PINNED);
	node->GetTMController()->NodeIKParamsChanged();
   BOOL suspendPanel = node->Selected();
   if (suspendPanel)
   {
      MAXScript_interface7->SuspendEditing(1<<TASK_MODE_HIERARCHY);
      MAXScript_interface7->ResumeEditing(1<<TASK_MODE_HIERARCHY);
   }
	return &ok;
}

//  =================================================
//  Mouse methods
//  =================================================

/*
Value*
getMouseMode_cf(Value** arg_list, int count)
{
	check_arg_count(getMouseMode, 0, count);
	return Integer::intern((int)MAXScript_interface->GetMouseManager()->GetMouseMode());
}

Value*
getMouseButtonStates_cf(Value** arg_list, int count)
{
	check_arg_count(getMouseButtonStates, 0, count);
	BitArray flags(3);
	int iflags = MAXScript_interface->GetMouseManager()->ButtonFlags();
	int i, j;
	for (i = 0, j = (1<<3); i < 3; i++, j= (j << 1)) 
		flags.Set(i,(iflags & j));
	return new BitArrayValue(flags);
}
*/

Value*
getMouseMode()
{
	return Integer::intern((int)MAXScript_interface->GetMouseManager()->GetMouseMode());
}

Value*
getMouseMAXPos()
{
	POINT pt;
	GetCursorPos( &pt );
	ViewExp* view = MAXScript_interface->GetActiveViewport();
	ScreenToClient( view->GetHWnd(), &pt );
	MAXScript_interface->ReleaseViewport(view);
	return new Point2Value(pt);
}

Value*
getMouseScreenPos()
{
	POINT pt;
	GetCursorPos( &pt );
	return new Point2Value(pt);
}

Value*
getMouseButtonStates()
{
	BitArray flags(3);
	int iflags = MAXScript_interface->GetMouseManager()->ButtonFlags();
	int i, j;
	for (i = 0, j = (1<<3); i < 3; i++, j= (j << 1)) 
		flags.Set(i,(iflags & j));
	return new BitArrayValue(flags);
}


//  =================================================
//  MtlBase methods
//  =================================================

Value*
getMTLMEditFlags_cf(Value** arg_list, int count)
{
	check_arg_count(getMTLMEditFlags, 1, count);
	MtlBase* mtl;
	if (arg_list[0]->is_kind_of(class_tag(MAXTexture)))
		mtl = (MtlBase*)arg_list[0]->to_texmap();
	else
	{
		mtl = arg_list[0]->to_mtl();
      if (mtl == NULL) throw ConversionError (arg_list[0], _T("material"));
	}
	BitArray flags(7);
	flags.Set(0,(mtl->TestMtlFlag(MTL_BEING_EDITED)));
	flags.Set(1,(mtl->TestMtlFlag(MTL_MEDIT_BACKGROUND)));
	flags.Set(2,(mtl->TestMtlFlag(MTL_MEDIT_BACKLIGHT)));

#ifndef NO_MTLEDITOR_VIDCOLORCHECK  // orb 01-08-2002
	flags.Set(3,(mtl->TestMtlFlag(MTL_MEDIT_VIDCHECK)));
#endif // NO_MTLEDITOR_VIDCOLORCHECK

	flags.Set(4,(mtl->TestMtlFlag(MTL_SUB_DISPLAY_ENABLED))); // LAM - 7/20/01 added these 3 for gMax - read/only
	flags.Set(5,(mtl->TestMtlFlag(MTL_TEX_DISPLAY_ENABLED)));
	flags.Set(6,(mtl->SupportTexDisplay()));
	return new BitArrayValue(flags);
}

Value*
setMTLMEditFlags_cf(Value** arg_list, int count)
{
	check_arg_count(setMTLMEditFlags, 2, count);
	MtlBase* mtl;
	if (arg_list[0]->is_kind_of(class_tag(MAXTexture)))
		mtl = (MtlBase*)arg_list[0]->to_texmap();
	else
	{
		mtl = arg_list[0]->to_mtl();
      if (mtl == NULL) throw ConversionError (arg_list[0], _T("material"));
	}
	BitArray flags = arg_list[1]->to_bitarray();
	flags.SetSize(7,TRUE);
//	mtl->SetMtlFlag(MTL_BEING_EDITED, flags[0]);	// read-only
	mtl->SetMtlFlag(MTL_MEDIT_BACKGROUND, flags[1]);
	mtl->SetMtlFlag(MTL_MEDIT_BACKLIGHT, flags[2]);

#ifndef NO_MTLEDITOR_VIDCOLORCHECK  // orb 01-08-2002
	mtl->SetMtlFlag(MTL_MEDIT_VIDCHECK, flags[3]);
#endif // NO_MTLEDITOR_VIDCOLORCHECK

	mtl->NotifyDependents(FOREVER,0,REFMSG_SEL_NODES_DELETED,NOTIFY_ALL,FALSE); // button update
	mtl->NotifyDependents(FOREVER,0,REFMSG_CHANGE,NOTIFY_ALL,FALSE); // slot update
	return &ok;
}

Value*
getMTLMeditObjType_cf(Value** arg_list, int count)
{
	check_arg_count(getMTLMeditObjType, 1, count);
	MtlBase* mtl;
	if (arg_list[0]->is_kind_of(class_tag(MAXTexture)))
		mtl = (MtlBase*)arg_list[0]->to_texmap();
	else
	{
		mtl = arg_list[0]->to_mtl();
      if (mtl == NULL) throw ConversionError (arg_list[0], _T("material"));
	}
	return Integer::intern((int)mtl->GetMeditObjType()+1);
}

Value*
setMTLMeditObjType_cf(Value** arg_list, int count)
{
	check_arg_count(setMTLMeditObjType, 2, count);
	MtlBase* mtl;
	if (arg_list[0]->is_kind_of(class_tag(MAXTexture)))
		mtl = (MtlBase*)arg_list[0]->to_texmap();
	else
	{
		mtl = arg_list[0]->to_mtl();
      if (mtl == NULL) throw ConversionError (arg_list[0], _T("material"));
	}
	int objType = arg_list[1]->to_int();
	range_check(objType, 1, 4, GetString(IDS_MEDIT_OBJECT_TYPE_INDEX_OUT_OF_RANGE));
	mtl->SetMeditObjType(objType-1);
	mtl->NotifyDependents(FOREVER,0,REFMSG_SEL_NODES_DELETED,NOTIFY_ALL,FALSE); // button update
	mtl->NotifyDependents(FOREVER,0,REFMSG_CHANGE,NOTIFY_ALL,FALSE); // slot update
	return &ok;
}

Value*
getMTLMeditTiling_cf(Value** arg_list, int count)
{
	check_arg_count(getMTLMeditTiling, 1, count);
#ifdef NO_MTLEDITOR_SAMPLEUVTILING // orb 01-08-2002
	NOT_SUPPORTED_BY_PRODUCT("GetMTLMeditTiling");
#else
	MtlBase* mtl;
	if (arg_list[0]->is_kind_of(class_tag(MAXTexture)))
		mtl = (MtlBase*)arg_list[0]->to_texmap();
	else
	{
		mtl = arg_list[0]->to_mtl();
      if (mtl == NULL) throw ConversionError (arg_list[0], _T("material"));
	}
	return Integer::intern((int)mtl->GetMeditTiling()+1);
#endif // NO_MTLEDITOR_SAMPLEUVTILING
}

Value*
setMTLMeditTiling_cf(Value** arg_list, int count)
{
	check_arg_count(setMTLMeditTiling, 2, count);

#ifdef NO_MTLEDITOR_SAMPLEUVTILING // orb 01-08-2002
	NOT_SUPPORTED_BY_PRODUCT("SetMTLMeditTiling");
#else

	MtlBase* mtl;
	if (arg_list[0]->is_kind_of(class_tag(MAXTexture)))
		mtl = (MtlBase*)arg_list[0]->to_texmap();
	else
	{
		mtl = arg_list[0]->to_mtl();
      if (mtl == NULL) throw ConversionError (arg_list[0], _T("material"));
	}
	int objType = arg_list[1]->to_int();
	range_check(objType, 1, 4, GetString(IDS_MEDIT_TILE_TYPE_INDEX_OUT_OF_RANGE));
	mtl->SetMeditTiling(objType-1);
	mtl->NotifyDependents(FOREVER,0,REFMSG_SEL_NODES_DELETED,NOTIFY_ALL,FALSE); // button update
	mtl->NotifyDependents(FOREVER,0,REFMSG_CHANGE,NOTIFY_ALL,FALSE); // slot update
	return &ok;
#endif // NO_MTLEDITOR_SAMPLEUVTILING
}

Value*
updateMTLInMedit_cf(Value** arg_list, int count)
{
	check_arg_count(updateMTLInMedit, 1, count);
	MtlBase* mtl;
	if (arg_list[0]->is_kind_of(class_tag(MAXTexture)))
		mtl = (MtlBase*)arg_list[0]->to_texmap();
	else
	{
		mtl = arg_list[0]->to_mtl();
      if (mtl == NULL) throw ConversionError (arg_list[0], _T("material"));
	}
	mtl->NotifyDependents(FOREVER,0,REFMSG_SEL_NODES_DELETED,NOTIFY_ALL,TRUE); // button update
	mtl->NotifyDependents(FOREVER,0,REFMSG_CHANGE,NOTIFY_ALL,TRUE); // slot update
	mtl->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED,NOTIFY_ALL,TRUE); // slot update
	return &ok;
}

//  =================================================
//  Array methods
//  =================================================

Value* qsort_user_compare_fn;
Value** qsort_user_compare_fn_args;
int qsort_user_compare_fn_arg_count;

static int
compare_fn(const void *p1, const void *p2)
{	
	// finish building arg list and apply comparison fn
	qsort_user_compare_fn_args[0] = *(Value**)p1;
	qsort_user_compare_fn_args[1] = *(Value**)p2;
	return (qsort_user_compare_fn->apply(qsort_user_compare_fn_args, qsort_user_compare_fn_arg_count))->to_int();
}

Value*
qsort_cf(Value** arg_list, int count)
{ //@doc@ qsort <array> <function> [start:<integer>] [end:<integer>] [user-defined key args passed to compare_fn]
	check_arg_count_with_keys(qsort, 2, count);
	type_check(arg_list[0], Array, "qsort");
	Array* theArray = (Array*)arg_list[0];
	if (theArray->size == 0) return &ok;
	qsort_user_compare_fn = arg_list[1];
	if (!is_function(qsort_user_compare_fn))
		throw RuntimeError (GetString(IDS_QSORT_ARG_NOT_FN), qsort_user_compare_fn);
	Value* tmp;
	int startIndex=int_key_arg(start, tmp, 1);
	range_check(startIndex, 1, theArray->size, GetString(IDS_QSORT_STARTINDEX_BAD))
	int endIndex=int_key_arg(end, tmp, theArray->size);
	range_check(endIndex, 1, theArray->size, GetString(IDS_QSORT_ENDINDEX_BAD))
	if (endIndex < startIndex)
		throw RuntimeError (GetString(IDS_QSORT_STARTENDINDEX_BAD));

	startIndex -= 1;
	int sortSize = endIndex - startIndex;

	// copy over any additional arguments that present. These will be passed to the comparison function
	// leave first two elements empty so that the p1 and p2 values in compare_fn can be put in them.
	qsort_user_compare_fn_args = (Value**)_alloca(sizeof(Value*) * count);
	for (int i = 2; i < count; i++) 
		qsort_user_compare_fn_args[i]=arg_list[i];
	qsort_user_compare_fn_arg_count=count;

	qsort(&theArray->data[startIndex], sortSize, sizeof(Value*), compare_fn);
	return &ok;
}


Value*
insertItem_cf(Value** arg_list, int count)
{
	check_arg_count(insertItem, 3, count);
	if (is_tab_param(arg_list[1]))
		return ((MAXPB2ArrayParam*)arg_list[1])->insertItem(arg_list, count);
	type_check(arg_list[1], Array, "insertItem");
	Array* theArray = (Array*)arg_list[1];
	int index;
	Value* arg=arg_list[2];
	if (!is_number(arg) || (index = arg->to_int()) < 1)
		throw RuntimeError (GetString(IDS_ARRAY_INDEX_MUST_BE_VE_NUMBER_GOT), arg);

	int newSize=theArray->size+1;
	int oldSize=newSize;
	if (index > newSize) newSize=index;

	EnterCriticalSection(&Array::array_update);

	if (newSize > theArray->data_size)
	{
		/* grow array */
		theArray->data = (Value**)realloc(theArray->data, newSize * sizeof(Value*));
		theArray->data_size = newSize;
	}

	for (int i = theArray->size; i < newSize; i++)
		theArray->data[i] = &undefined;
	theArray->size = newSize;

   for (int i = newSize-1; i >= index; i--)
		theArray->data[i] = theArray->data[i-1];
	theArray->data[index - 1] = heap_ptr(arg_list[0]);

	LeaveCriticalSection(&Array::array_update);

	return &ok;
}

Value*
amin_cf(Value** arg_list, int count)
{
	Value* smallest;
	int i;
	if (count == 0) return &undefined;
	if (arg_list[0]->is_kind_of(class_tag(Array))) {
		check_arg_count(amin, 1, count);
		Array* theArray = (Array*)arg_list[0];
		if (theArray->size == 0) return &undefined;
		smallest=theArray->data[0];
		for (i = 1; i < theArray->size; i++)
			if (smallest->lt_vf(&theArray->data[i],1) == &false_value) 
				smallest = theArray->data[i];
	}
	else {
		smallest=arg_list[0];
		for (i = 1; i < count; i++)
			if (smallest->lt_vf(&arg_list[i],1) == &false_value) 
				smallest = arg_list[i];
	}
	return smallest;
}

Value*
amax_cf(Value** arg_list, int count)
{
	Value* largest;
	int i;
	if (count == 0) return &undefined;
	if (arg_list[0]->is_kind_of(class_tag(Array))) {
		check_arg_count(amin, 1, count);
		Array* theArray = (Array*)arg_list[0];
		if (theArray->size == 0) return &undefined;
		largest=theArray->data[0];
		for (i = 1; i < theArray->size; i++)
			if (largest->gt_vf(&theArray->data[i],1) == &false_value) 
				largest = theArray->data[i];
	}
	else {
		largest=arg_list[0];
		for (i = 1; i < count; i++)
			if (largest->gt_vf(&arg_list[i],1) == &false_value) 
				largest = arg_list[i];
	}
	return largest;
}

//  =================================================
//  Renderer methods
//  =================================================

int rendTimeType[]={REND_TIMESINGLE,REND_TIMESEGMENT,REND_TIMERANGE,REND_TIMEPICKUP};

Value*
get_RendTimeType()
{
	int res = MAXScript_interface->GetRendTimeType();
	for (int i = 0; i < 4; i++) 
		if (rendTimeType[i] == res) return Integer::intern(i+1);
	throw RuntimeError (GetString(IDS_BAD_RENDTIMETYPE_RESULT), Integer::intern(res));
}


Value*
set_RendTimeType(Value *val)
{
	int i = val->to_int();
	range_check(i, 1, 4, GetString(IDS_RENDTIMETYPE_OUT_OF_RANGE));
	MAXScript_interface->SetRendTimeType (rendTimeType[i-1]);
	return val;
}

Value*
get_RendStart()
{
	return MSTime::intern(MAXScript_interface->GetRendStart ());
}


Value*
set_RendStart(Value *val)
{
	MAXScript_interface->SetRendStart (val->to_timevalue() );
	return val;
}

Value*
get_RendEnd()
{
	return MSTime::intern(MAXScript_interface->GetRendEnd ());
}


Value*
set_RendEnd(Value *val)
{
	MAXScript_interface->SetRendEnd (val->to_timevalue() );
	return val;
}

Value*
get_RendNThFrame()
{
	return Integer::intern(MAXScript_interface->GetRendNThFrame ());
}


Value*
set_RendNThFrame(Value *val)
{
	int i = val->to_int();
	if (i < 1)
		throw RuntimeError (GetString(IDS_RENDNTHFRAME_OUT_OF_RANGE), val);
	MAXScript_interface->SetRendNThFrame (i);
	return val;
}

Value*
get_RendShowVFB()
{
	return MAXScript_interface->GetRendShowVFB() ? &true_value : &false_value;
}

#ifndef WEBVERSION
Value*
set_RendShowVFB(Value* val)
{
	MAXScript_interface->SetRendShowVFB(val->to_bool());
	return val;
}
#endif // WEBVERSION 

Value*
get_RendSaveFile()
{
	return MAXScript_interface->GetRendSaveFile() ? &true_value : &false_value;
}

Value*
set_RendSaveFile(Value* val)
{
	MAXScript_interface->SetRendSaveFile(val->to_bool());
	return val;
}

Value*
get_RendUseDevice()
{
	return MAXScript_interface->GetRendUseDevice() ? &true_value : &false_value;
}

#ifndef WEBVERSION
Value*
set_RendUseDevice(Value* val)
{
	MAXScript_interface->SetRendUseDevice(val->to_bool());
	return val;
}
#endif // WEBVERSION

Value*
get_RendUseNet()
{
	return MAXScript_interface->GetRendUseNet() ? &true_value : &false_value;
}

#ifndef WEBVERSION
Value*
set_RendUseNet(Value* val)
{
	MAXScript_interface->SetRendUseNet(val->to_bool());
	return val;
}
#endif // WEBVERSION

Value*
get_RendFieldRender()
{
	return MAXScript_interface->GetRendFieldRender() ? &true_value : &false_value;
}

#ifndef WEBVERSION
Value*
set_RendFieldRender(Value* val)
{
	MAXScript_interface->SetRendFieldRender(val->to_bool());
	return val;
}
#endif // WEBVERSION

Value*
get_RendColorCheck()
{
	return MAXScript_interface->GetRendColorCheck() ? &true_value : &false_value;
}

#ifndef WEBVERSION
Value*
set_RendColorCheck(Value* val)
{
	MAXScript_interface->SetRendColorCheck(val->to_bool());
	return val;
}
#endif // WEBVERSION

Value*
get_RendSuperBlack()
{
	return MAXScript_interface->GetRendSuperBlack() ? &true_value : &false_value;
}

#ifndef WEBVERSION
Value*
set_RendSuperBlack(Value* val)
{
	MAXScript_interface->SetRendSuperBlack(val->to_bool());
	return val;
}
#endif // WEBVERSION

Value*
get_RendHidden()
{
	return MAXScript_interface->GetRendHidden() ? &true_value : &false_value;
}

Value*
set_RendHidden(Value* val)
{
	MAXScript_interface->SetRendHidden(val->to_bool());
	return val;
}

Value*
get_RendForce2Side()
{
	return MAXScript_interface->GetRendForce2Side() ? &true_value : &false_value;
}

Value*
set_RendForce2Side(Value* val)
{
	MAXScript_interface->SetRendForce2Side(val->to_bool());
	return val;
}

Value*
get_RendAtmosphere()
{
	return MAXScript_interface->GetRendAtmosphere() ? &true_value : &false_value;
}

#ifndef WEBVERSION
Value*
set_RendAtmosphere(Value* val)
{
	MAXScript_interface->SetRendAtmosphere(val->to_bool());
	return val;
}
#endif // WEBVERSION

Value*
get_RendDitherTrue()
{
	return MAXScript_interface->GetRendDitherTrue() ? &true_value : &false_value;
}

Value*
set_RendDitherTrue(Value* val)
{
	MAXScript_interface->SetRendDitherTrue(val->to_bool());
	return val;
}

Value*
get_RendDither256()
{
	return MAXScript_interface->GetRendDither256() ? &true_value : &false_value;
}

Value*
set_RendDither256(Value* val)
{
	MAXScript_interface->SetRendDither256(val->to_bool());
	return val;
}

Value*
get_RendMultiThread()
{
	return MAXScript_interface->GetRendMultiThread() ? &true_value : &false_value;
}

Value*
set_RendMultiThread(Value* val)
{
	MAXScript_interface->SetRendMultiThread(val->to_bool());
	return val;
}

Value*
get_RendNThSerial()
{
	return MAXScript_interface->GetRendNThSerial() ? &true_value : &false_value;
}

Value*
set_RendNThSerial(Value* val)
{
	MAXScript_interface->SetRendNThSerial(val->to_bool());
	return val;
}

Value*
get_RendVidCorrectMethod()
{
	return Integer::intern(MAXScript_interface->GetRendVidCorrectMethod () + 1);
}


Value*
set_RendVidCorrectMethod(Value *val)
{
	int i = val->to_int();
	range_check(i, 1, 3, GetString(IDS_RENDVIDCORRECTMETHOD_OUT_OF_RANGE));
	MAXScript_interface->SetRendVidCorrectMethod (i-1);
	return val;
}

Value*
get_RendFieldOrder()
{
	return Integer::intern(MAXScript_interface->GetRendFieldOrder () + 1);
}


Value*
set_RendFieldOrder(Value *val)
{
	int i = val->to_int();
	range_check(i, 1, 2, GetString(IDS_RENDFIELDORDER_OUT_OF_RANGE));
	MAXScript_interface->SetRendFieldOrder (i-1);
	return val;
}

Value*
get_RendNTSC_PAL()
{
	return Integer::intern(MAXScript_interface->GetRendNTSC_PAL () + 1);
}


Value*
set_RendNTSC_PAL(Value *val)
{
	int i = val->to_int();
	range_check(i, 1, 2, GetString(IDS_RENDNTSC_PAL_OUT_OF_RANGE));
	MAXScript_interface->SetRendNTSC_PAL (i-1);
	return val;
}

Value* get_LockImageAspRatio()
{
	return MAXScript_interface7->GetLockImageAspRatio() ? &true_value : &false_value;
}

#ifndef WEBVERSION
Value* set_LockImageAspRatio(Value* val)
{
	MAXScript_interface7->SetLockImageAspRatio(val->to_bool());
	return val;
}
#endif // WEBVERSION

Value* get_ImageAspRatio()
{
	return Float::intern(MAXScript_interface7->GetImageAspRatio());
}

#ifndef WEBVERSION
Value* set_ImageAspRatio(Value* val)
{
	float v = val->to_float();
	MAXScript_interface7->SetImageAspRatio(v);
	return val;
}
#endif // WEBVERSION

Value* get_LockPixelAspRatio()
{
	return MAXScript_interface7->GetLockPixelAspRatio() ? &true_value : &false_value;
}

#ifndef WEBVERSION
Value* set_LockPixelAspRatio(Value* val)
{
	MAXScript_interface7->SetLockPixelAspRatio(val->to_bool());
	return val;
}
#endif // WEBVERSION

Value* get_PixelAspRatio()
{
	return Float::intern(MAXScript_interface7->GetPixelAspRatio());
}

#ifndef WEBVERSION
Value* set_PixelAspRatio(Value* val)
{
	float v = val->to_float();
	if (v > 0.0)
		MAXScript_interface7->SetPixelAspRatio(v);
	return val;
}
#endif // WEBVERSION


Value*
get_RendSuperBlackThresh()
{
	return Integer::intern(MAXScript_interface->GetRendSuperBlackThresh ());
}


#ifndef WEBVERSION
Value*
set_RendSuperBlackThresh(Value *val)
{
	int i = val->to_int();
	range_check(i, 1, 255, GetString(IDS_RENDSUPERBLACKTHRESH_OUT_OF_RANGE));
	MAXScript_interface->SetRendSuperBlackThresh (i);
	return val;
}
#endif // WEBVERSION

Value*
get_RendFileNumberBase()
{
	return Integer::intern(MAXScript_interface->GetRendFileNumberBase ());
}


Value*
set_RendFileNumberBase(Value *val)
{
	MAXScript_interface->SetRendFileNumberBase (val->to_int());
	return val;
}

Value*
get_RendPickupFrames()
{
	return new String (MAXScript_interface->GetRendPickFramesString());
}


Value*
set_RendPickupFrames(Value *val)
{
	TCHAR* inString = val->to_string();
	TCHAR okchars[] = "- 0123456789,";
	TCHAR badChar[] = " ";
   int n_ok = static_cast<int>(_tcslen(okchars));  
   int n_in = static_cast<int>(_tcslen(inString)); // SR DCAST64: Downcast to 2G limit.
	for (int j=0; j<n_in; j++)
		if (!strchr(okchars,inString[j]))
		{	badChar[0] = inString[j];
			throw RuntimeError (_T("Invalid RendPickupFrames string character: "), badChar);
		}

//	TSTR& frameString = MAXScript_interface->GetRendPickFramesString();
//	frameString = inString;
	MAXScript_interface->GetRendPickFramesString() = inString;
	return val;
}


Value*
get_RendSimplifyAreaLights()
{
      return MAXScript_interface->GetRendSimplifyAreaLights() ? &true_value : &false_value;
}

 

Value*
set_RendSimplifyAreaLights(Value *val)
{
      MAXScript_interface->SetRendSimplifyAreaLights(val->to_bool());
      return val;
}



Value*
renderSceneDialog_Open_cf(Value** arg_list, int count)
{
	HoldSuspend suspend;
	check_arg_count(renderSceneDialog.Open, 0, count);
	MAXScript_interface7->OpenRenderDialog();
	return &ok;
}

Value*
renderSceneDialog_Cancel_cf(Value** arg_list, int count)
{
	HoldSuspend suspend;
	check_arg_count(renderSceneDialog.Cancel, 0, count);
	MAXScript_interface7->CancelRenderDialog();
	return &ok;
}

Value*
renderSceneDialog_Close_cf(Value** arg_list, int count)
{
	HoldSuspend suspend;
	check_arg_count(renderSceneDialog.Close, 0, count);
	MAXScript_interface7->CloseRenderDialog();
	return &ok;
}

Value*
renderSceneDialog_Commit_cf(Value** arg_list, int count)
{
	HoldSuspend suspend;
	check_arg_count(renderSceneDialog.Commit, 0, count);
	MAXScript_interface7->CommitRenderDialogParameters();
	return &ok;
}

Value*
renderSceneDialog_Update_cf(Value** arg_list, int count)
{
	HoldSuspend suspend;
	check_arg_count(renderSceneDialog.Update, 0, count);
	MAXScript_interface7->UpdateRenderDialogParameters();
	return &ok;
}

Value*
renderSceneDialog_isOpen_cf(Value** arg_list, int count)
{
	HoldSuspend suspend;
	check_arg_count(renderSceneDialog.isOpen, 0, count);
	return (MAXScript_interface7->RenderDialogOpen()) ? &true_value : &false_value;
}

Value*
get_DraftRenderer()
{
	return MAXRenderer::intern(MAXScript_interface->GetDraftRenderer());
}

Value*
set_DraftRenderer(Value *val)
{
	HoldSuspend suspend;
	Renderer* renderer = val->to_renderer();
	BOOL rendDialogOpen = MAXScript_interface7->RenderDialogOpen() && (MAXScript_interface->GetCurrentRenderSetting() == RS_Draft);
	if (rendDialogOpen)
		MAXScript_interface7->CloseRenderDialog();
	MAXScript_interface->AssignDraftRenderer(renderer);
	if (rendDialogOpen)
		MAXScript_interface7->OpenRenderDialog();
	return val;
}

Value*
get_ProductionRenderer()
{
	return MAXRenderer::intern(MAXScript_interface->GetProductionRenderer());
}

Value*
set_ProductionRenderer(Value *val)
{
	HoldSuspend suspend;
	Renderer* renderer = val->to_renderer();
	BOOL rendDialogOpen = MAXScript_interface7->RenderDialogOpen() && (MAXScript_interface->GetCurrentRenderSetting() == RS_Production);
	if (rendDialogOpen)
		MAXScript_interface7->CloseRenderDialog();
	MAXScript_interface->AssignProductionRenderer(renderer);
	if (rendDialogOpen)
		MAXScript_interface7->OpenRenderDialog();
	return val;
}

Value*
get_CurrentRenderer()
{
	return MAXRenderer::intern(MAXScript_interface->GetCurrentRenderer());
}

Value*
set_CurrentRenderer(Value *val)
{
	HoldSuspend suspend;
	Renderer* renderer = val->to_renderer();
	BOOL rendDialogOpen = MAXScript_interface7->RenderDialogOpen();
	if (rendDialogOpen)
		MAXScript_interface7->CloseRenderDialog();
	MAXScript_interface->AssignCurRenderer(renderer);
	if (rendDialogOpen)
		MAXScript_interface7->OpenRenderDialog();
	return val;
}

Value*
get_MEditRenderer()
{
	return MAXRenderer::intern(MAXScript_interface->GetMEditRenderer());
}

Value*
set_MEditRenderer(Value *val)
{
	HoldSuspend suspend;
	Renderer* renderer = val->to_renderer();
	BOOL rendDialogOpen = MAXScript_interface7->RenderDialogOpen();
	if (rendDialogOpen)
		MAXScript_interface7->CloseRenderDialog();
	MAXScript_interface->AssignMEditRenderer(renderer);
	if (rendDialogOpen)
		MAXScript_interface7->OpenRenderDialog();
	return val;
}

Value*
get_ReshadeRenderer()
{
	return MAXRenderer::intern(MAXScript_interface->GetRenderer(RS_IReshade));
}

Value*
set_ReshadeRenderer(Value *val)
{
	HoldSuspend suspend;
	Renderer* renderer = val->to_renderer();
	// can only use renderers that support a reshading interface
	if ( renderer->GetInterface(I_RENDER_ID) )
	{
		BOOL rendDialogOpen = MAXScript_interface7->RenderDialogOpen() && (MAXScript_interface->GetCurrentRenderSetting() == RS_IReshade);
		if (rendDialogOpen)
			MAXScript_interface7->CloseRenderDialog();
		MAXScript_interface->AssignRenderer(RS_IReshade, renderer);
		if (rendDialogOpen)
			MAXScript_interface7->OpenRenderDialog();
	}
	return val;
}

Value* get_MEditRendererLocked()
{
	return (MAXScript_interface->GetMEditRendererLocked() ? &true_value : &false_value);
}

Value* set_MEditRendererLocked(Value* val)
{
	HoldSuspend suspend;
	BOOL b = val->to_bool();

	BOOL rendDialogOpen = MAXScript_interface7->RenderDialogOpen();
	if (rendDialogOpen)
		MAXScript_interface7->CloseRenderDialog();
	MAXScript_interface->SetMEditRendererLocked(b != 0);
	if (rendDialogOpen)
		MAXScript_interface7->OpenRenderDialog();

	return val;
}

//  =================================================
//  Controller methods
//  =================================================

Value*
getXYZControllers_cf(Value** arg_list, int count)
{
	check_arg_count(getXYZControllers, 1, count);
	Control *c = arg_list[0]->to_controller();
	if(!c) return &undefined; 	
	one_typed_value_local(Array* result);
	vl.result = new Array (3);
	Control *c1 = c->GetXController();
	vl.result->append(c1 ? MAXControl::intern(c1) : &undefined);
	c1 = c->GetYController();
	vl.result->append(c1 ? MAXControl::intern(c1) : &undefined);
	c1 = c->GetZController();
	vl.result->append(c1 ? MAXControl::intern(c1) : &undefined);
	return_value(vl.result);
}

Value*
displayControlDialog_cf(Value** arg_list, int count)
{
	check_arg_count(displayControlDialog, 2, count);
	arg_list[0]->to_controller()->EditTrackParams(MAXScript_time(), 
		((MAXControl*)arg_list[0])->dim,
		arg_list[1]->to_string(),
		MAXScript_interface->GetMAXHWnd(), 
		(IObjParam*)MAXScript_interface, 
		EDITTRACK_BUTTON | EDITTRACK_TRACK);
	return &ok;
}

//  =================================================
//  Mesh/Spline point controller methods
//  =================================================

// EditTriObject References:
#define ET_MASTER_CONTROL_REF  0
#define ET_VERT_BASE_REF 1

Value*
getPointControllers_cf(Value** arg_list, int count)
{
	check_arg_count(getPointControllers, 1, count);
	INode* node = get_valid_node(arg_list[0], getPointControllers);
   Object* obj = Get_Object_Or_XRef_BaseObject(node->GetObjectRef());

	one_typed_value_local(Array* result);

	if (obj->ClassID() == Class_ID(EDITTRIOBJ_CLASS_ID, 0)) {
		// mesh
		TriObject *em = (TriObject*)obj;
		int size = em->NumRefs()-1;
		vl.result = new Array (size);
		for (int i = 0; i < size; i++) {
			Control *c1 = (Control*)em->GetReference(i+ET_VERT_BASE_REF);
			vl.result->append(c1 ? MAXControl::intern(c1) : &undefined);
		}
	}
	else if (obj->ClassID() == splineShapeClassID || obj->ClassID() == Class_ID(SPLINE3D_CLASS_ID,0)) {
		// spline
		Tab<Control*> *cont;
		cont=&((SplineShape*)obj)->cont;
		int size = cont->Count();
		vl.result = new Array (size);
		for (int i = 0; i < size; i++) {
			Control *c1 = (*cont)[i];
			vl.result->append(c1 ? MAXControl::intern(c1) : &undefined);
		}
	}
	else 
		throw RuntimeError (GetString(IDS_CANNOT_GET_POINT_CONTROLLER), node->GetName());

	return_value(vl.result);
}

Value*
getPointController_cf(Value** arg_list, int count)
{
	if (count == 0) throw ArgCountError (_T("getPointController"), 2, count);
	INode* node = get_valid_node(arg_list[0], getPointController);
   Object* obj = Get_Object_Or_XRef_BaseObject(node->GetObjectRef());


	if (obj->ClassID() == Class_ID(EDITTRIOBJ_CLASS_ID, 0)) {
		// mesh
		check_arg_count(getPointController, 2, count);
		TriObject *em = (TriObject*)obj;
		int pointIndex=arg_list[1]->to_int();
		range_check(pointIndex, 1, em->GetMesh().getNumVerts(), GetString(IDS_MESH_VERTEX_INDEX_OUT_OF_RANGE))
		Control *c1 = (Control*)em->GetReference(pointIndex-1+ET_VERT_BASE_REF);
		return (c1 ? MAXControl::intern(c1) : &undefined);
	}
	else if (obj->ClassID() == splineShapeClassID || obj->ClassID() == Class_ID(SPLINE3D_CLASS_ID,0)) {
		// spline
		check_arg_count(getPointController, 3, count);
		Tab<Control*> *cont;
		cont=&((SplineShape*)obj)->cont;
		int splineIndex=arg_list[1]->to_int();
		int vertIndex=arg_list[2]->to_int();
		BezierShape* s = set_up_spline_access(node, splineIndex);
		Spline3D* s3d = s->splines[splineIndex-1];
		int pointCount = 3*s3d->KnotCount();
		range_check(vertIndex, 1, pointCount, GetString(IDS_SPLINE_VERTEX_INDEX_OUT_OF_RANGE))
		int pointIndex = s->GetVertIndex(splineIndex-1,vertIndex-1)+1;
		if (pointIndex <= cont->Count()) {
			Control *c1 = (*cont)[pointIndex-1];
			return (c1 ? MAXControl::intern(c1) : &undefined);
		}
		else
			return &undefined;
	}
	else 
		throw RuntimeError (GetString(IDS_CANNOT_GET_POINT_CONTROLLER), node->GetName());

}

//  =================================================
//  Value type methods
//  =================================================

Value*
isStructDef_cf(Value** arg_list, int count)
{
	check_arg_count(isStructDef, 1, count);
	return (is_structdef(arg_list[0]) ? &true_value : &false_value);
}

Value*
isStruct_cf(Value** arg_list, int count)
{
	check_arg_count(isStruct, 1, count);
	return ((arg_list[0]->tag == INTERNAL_STRUCT_TAG) ? &true_value : &false_value);
}

Value*
isController_cf(Value** arg_list, int count)
{
	check_arg_count(isController, 1, count);
	return (is_controller(arg_list[0]) ? &true_value : &false_value);
}

//  =================================================
//  System Info methods
//  =================================================

Value*
getDesktopSize()
{
//	HDC hdc = GetDC(GetDesktopWindow());
//	int wScreen = GetDeviceCaps(hdc, HORZRES);
//	int hScreen = GetDeviceCaps(hdc, VERTRES);
//	ReleaseDC(GetDesktopWindow(),hdc);
	int wScreen = GetScreenWidth();
	int hScreen = GetScreenHeight();
	return new Point2Value((float)wScreen,(float)hScreen);
}

Value*
getDesktopBPP()
{
	HDC hdc = GetDC(GetDesktopWindow());
	int bits = GetDeviceCaps(hdc,BITSPIXEL);
	ReleaseDC(GetDesktopWindow(),hdc);
	return Integer::intern(bits);
}

Value*
getSystemMemoryInfo_cf(Value** arg_list, int count)
{
	check_arg_count(getSystemMemoryInfo, 0, count);
	MEMORYSTATUS ms;
	ms.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&ms);
	one_typed_value_local(Array* result);
   vl.result = new Array (7);
   vl.result->append(Integer64::intern(ms.dwMemoryLoad));   // percent of memory in use
   vl.result->append(Integer64::intern(ms.dwTotalPhys)); // bytes of physical memory
   vl.result->append(Integer64::intern(ms.dwAvailPhys)); // free physical memory bytes
   vl.result->append(Integer64::intern(ms.dwTotalPageFile));// bytes of paging file
   vl.result->append(Integer64::intern(ms.dwAvailPageFile));// free bytes of paging file
   vl.result->append(Integer64::intern(ms.dwTotalVirtual)); // user bytes of address space
   vl.result->append(Integer64::intern(ms.dwAvailVirtual)); // free user bytes
	return_value(vl.result);
}

//-----------------------------------------------------------------------------
// Structure for GetProcessMemoryInfo()
//
// This is from psapi.h which is only included in the Platform SDK

typedef struct _PROCESS_MEMORY_COUNTERS {
	DWORD cb;
	DWORD PageFaultCount;
	SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize;
	SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage;
	SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage;
	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;

//-- This is in psapi.dll (NT4 and W2k only)

typedef BOOL (WINAPI *GetProcessMemoryInfo)(HANDLE Process,PROCESS_MEMORY_COUNTERS* ppsmemCounters,DWORD cb);

GetProcessMemoryInfo getProcessMemoryInfo;
//-----------------------------------------------------------------------------

Value*
getMAXMemoryInfo_cf(Value** arg_list, int count)
{
	check_arg_count(getMAXMemoryInfo, 0, count);

	one_typed_value_local(Array* result);
	HMODULE hPsapi = LoadLibrary("psapi.dll");
	BOOL resOK = FALSE;
	if (hPsapi) {
		getProcessMemoryInfo = (GetProcessMemoryInfo)GetProcAddress(hPsapi,"GetProcessMemoryInfo");
		if (getProcessMemoryInfo) {
			PROCESS_MEMORY_COUNTERS pmc;
			int processID = GetCurrentProcessId();
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
			if (getProcessMemoryInfo(hProcess,&pmc,sizeof(pmc))) {
				vl.result = new Array (9);
            vl.result->append(Integer64::intern(pmc.PageFaultCount));
            vl.result->append(Integer64::intern(pmc.PeakWorkingSetSize));
            vl.result->append(Integer64::intern(pmc.WorkingSetSize));
            vl.result->append(Integer64::intern(pmc.QuotaPeakPagedPoolUsage));
            vl.result->append(Integer64::intern(pmc.QuotaPagedPoolUsage));
            vl.result->append(Integer64::intern(pmc.QuotaPeakNonPagedPoolUsage));
            vl.result->append(Integer64::intern(pmc.QuotaNonPagedPoolUsage));
            vl.result->append(Integer64::intern(pmc.PagefileUsage));
            vl.result->append(Integer64::intern(pmc.PeakPagefileUsage));
				resOK=TRUE;
			}
			CloseHandle( hProcess );
		}
		FreeLibrary(hPsapi);
	}
	if (resOK) 	return_value(vl.result);
	return_value(&undefined);// LAM - 5/18/01 - was: return &undefined;

}

Value*
getMAXPriority()
{
	def_process_priority_types();
	int processID = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
	int priority=GetPriorityClass(hProcess);
	CloseHandle( hProcess );
	return GetName(processPriorityTypes, elements(processPriorityTypes), priority);
}

Value*
setMAXPriority(Value* val)
{
	def_process_priority_types();
	int type = GetID(processPriorityTypes, elements(processPriorityTypes), val);
	int processID = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, processID);
	SetPriorityClass(hProcess,type);
	CloseHandle( hProcess );
	return val;
}

Value*
getusername()
{
	TCHAR username[MAX_PATH];
	DWORD namesize = MAX_PATH;
	GetUserName(username,&namesize);
	return new String (username);
}

Value*
getcomputername()
{
	TCHAR computername[MAX_COMPUTERNAME_LENGTH+1];
	DWORD namesize = MAX_COMPUTERNAME_LENGTH+1;
	GetComputerName(computername,&namesize);
	return new String (computername);
}

Value*
getSystemDirectory()
{
	TCHAR sysDir[MAX_PATH];
	GetSystemDirectory(sysDir,MAX_PATH);
	return new String (sysDir);
}

Value*
getWinDirectory()
{
	TCHAR winDir[MAX_PATH];
	GetWindowsDirectory(winDir,MAX_PATH);
	return new String (winDir);
}

Value*
getTempDirectory()
{
	TCHAR tempDir[MAX_PATH];
	GetTempPath(MAX_PATH,tempDir);
	return new String (tempDir);
}

Value*
getCurrentDirectory()
{
	TCHAR currentDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH,currentDir);
	return new String (currentDir);
}

Value*
setCurrentDirectory(Value* val)
{
	DispInfo info;
	GetUnitDisplayInfo(&info);
	if (!SetCurrentDirectory(val->to_filename()))
		throw RuntimeError (GetString(IDS_ERROR_SETTING_CURRENT_DIRECTORY), val);
	return val;
}

Value*
getCPUcount()
{
	SYSTEM_INFO     si;
	GetSystemInfo(&si);
	return Integer::intern(si.dwNumberOfProcessors);
}


//  =================================================
//  Command Mode methods
//  =================================================

Value*
getCommandMode()
{
	int state = MAXScript_interface->GetCommandMode()->Class();
	def_commandmode_types();
	return GetName(commandmodeTypes, elements(commandmodeTypes), state, Integer::intern(state));
}

Value*
setCommandMode(Value* val)
{
	def_stdcommandmode_types();
	int state = GetID(stdcommandmodeTypes, elements(stdcommandmodeTypes), val);
	MAXScript_interface->SetStdCommandMode(state);
	return val;
}

Value*
getAxisConstraints()
{
	int state = MAXScript_interface->GetAxisConstraints();
	def_axisconstraint_types();
	return GetName(axisconstraintTypes, elements(axisconstraintTypes), state, &undefined);
}

Value*
setAxisConstraints(Value* val)
{
	def_axisconstraint_types();
	int state = GetID(axisconstraintTypes, elements(axisconstraintTypes), val);
	MAXScript_interface->SetAxisConstraints(state);
	return val;
}

Value* 
GetToolBtnState_cf(Value** arg_list, int count)
{
	check_arg_count(GetToolBtnState, 1, count);
	def_toolbtn_types();
	BOOL state = MAXScript_interface->GetToolButtonState(
			GetID(toolbtnTypes, elements(toolbtnTypes), arg_list[0]));
	return ((state) ? &true_value : &false_value);
}

//  =================================================
//  MAX Unit methods
//  =================================================

/*
units.DisplayType
units.SystemType
units.SystemScale
units.MetricType
units.USType
units.USFrac
units.CustomName
units.CustomValue
units.CustomUnit
*/

Value* 
formatValue_cf(Value** arg_list, int count)
{
	check_arg_count(formatValue, 1, count);
	TCHAR* outstring = FormatUniverseValue(arg_list[0]->to_float());
	if (outstring[0] == '\0') 
		throw RuntimeError (GetString(IDS_UNABLE_TO_FORMAT_VALUE), arg_list[0]);
	return new String(outstring);
}


Value* 
decodeValue_cf(Value** arg_list, int count)
{
	check_arg_count(decodeValue, 1, count);
	BOOL valid;
	float outval = DecodeUniverseValue(arg_list[0]->to_string(),&valid);
	if (!valid) 
		throw RuntimeError (GetString(IDS_UNABLE_TO_DECODE_VALUE), arg_list[0]);
	return Float::intern(outval);
}

Value*
getUnitDisplayType()
{
	def_displayunit_types();
	int type = GetUnitDisplayType();
	return GetName(displayunitTypes, elements(displayunitTypes), type, &undefined);
}

Value*
setUnitDisplayType(Value* val)
{
	def_displayunit_types();
	int type = GetID(displayunitTypes, elements(displayunitTypes), val);
	SetUnitDisplayType(type);
	return val;
}

#ifndef USE_HARDCODED_SYSTEM_UNIT

Value*
getUnitSystemType()
{
	def_systemunit_types();
	int type;
	float scale;
	GetMasterUnitInfo(&type, &scale);
	return GetName(systemunitTypes, elements(systemunitTypes), type, &undefined);
}

Value*
setUnitSystemType(Value* val)
{
	def_systemunit_types();
	int type;
	float scale;
	GetMasterUnitInfo(&type, &scale);
	type = GetID(systemunitTypes, elements(systemunitTypes), val);
	SetMasterUnitInfo(type, scale);
	return val;
}

Value*
getUnitSystemScale()
{
	int type;
	float scale;
	GetMasterUnitInfo(&type, &scale);
	return Float::intern(scale);
}

Value*
setUnitSystemScale(Value* val)
{
	int type;
	float scale;
	GetMasterUnitInfo(&type, &scale);
	SetMasterUnitInfo(type, val->to_float());
	return val;
}
#endif // USE_HARDCODED_SYSTEM_UNIT 

Value*
getMetricDisplay()
{
	def_metricunit_types();
	DispInfo info;
	GetUnitDisplayInfo(&info);
	return GetName(metricunitTypes, elements(metricunitTypes), info.metricDisp, &undefined);
}

Value*
setMetricDisplay(Value* val)
{
	def_metricunit_types();
	DispInfo info;
	GetUnitDisplayInfo(&info);
	info.metricDisp= GetID(metricunitTypes, elements(metricunitTypes), val);
	SetUnitDisplayInfo(&info);
	return val;
}

Value*
getUSDisplay()
{
	def_usunit_types();
	DispInfo info;
	GetUnitDisplayInfo(&info);
	return GetName(usunitTypes, elements(usunitTypes), info.usDisp, &undefined);
}

Value*
setUSDisplay(Value* val)
{
	def_usunit_types();
	DispInfo info;
	GetUnitDisplayInfo(&info);
	info.usDisp= GetID(usunitTypes, elements(usunitTypes), val);
	SetUnitDisplayInfo(&info);
	return val;
}

Value*
getUSFrac()
{
	def_usfrac_types();
	DispInfo info;
	GetUnitDisplayInfo(&info);
	return GetName(usfracTypes, elements(usfracTypes), info.usFrac, &undefined);
}

Value*
setUSFrac(Value* val)
{
	def_usfrac_types();
	DispInfo info;
	GetUnitDisplayInfo(&info);
	info.usFrac= GetID(usfracTypes, elements(usfracTypes), val);
	SetUnitDisplayInfo(&info);
	return val;
}

Value*
getCustomName()
{
	DispInfo info;
	GetUnitDisplayInfo(&info);
	return new String(info.customName);
}

Value*
setCustomName(Value* val)
{
	DispInfo info;
	GetUnitDisplayInfo(&info);
	info.customName= val->to_string();
	SetUnitDisplayInfo(&info);
	return val;
}

Value*
getCustomValue()
{
	DispInfo info;
	GetUnitDisplayInfo(&info);
	return Float::intern(info.customValue);
}

Value*
setCustomValue(Value* val)
{
	DispInfo info;
	GetUnitDisplayInfo(&info);
	info.customValue= val->to_float();
	SetUnitDisplayInfo(&info);
	return val;
}

Value*
getCustomUnit()
{
	def_systemunit_types();
	DispInfo info;
	GetUnitDisplayInfo(&info);
	return GetName(systemunitTypes, elements(systemunitTypes), info.customUnit, &undefined);
}

Value*
setCustomUnit(Value* val)
{
	def_systemunit_types();
	DispInfo info;
	GetUnitDisplayInfo(&info);
	info.customUnit= GetID(systemunitTypes, elements(systemunitTypes), val);
	SetUnitDisplayInfo(&info);
	return val;
}

//  =================================================
//  AutoBackup methods
//  =================================================

Value*
getAutoBackupEnabled()
{
	return (MAXScript_interface->AutoBackupEnabled() ? &true_value : &false_value);
}

Value*
setAutoBackupEnabled(Value* val)
{
	MAXScript_interface->EnableAutoBackup(val->to_bool());
	return val;
}

Value*
getAutoBackupTime()
{
	return Float::intern(MAXScript_interface->GetAutoBackupTime());
}

Value*
setAutoBackupTime(Value* val)
{
	float t = val->to_float();
	if (t < 0.01f) {
		t = 0.01f;
		val = Float::intern(t);
	}
	MAXScript_interface->SetAutoBackupTime(t);
	return val;
}

//  =================================================
//  Quat <-> Euler methods
//  =================================================

typedef int EAOrdering[3];
static EAOrdering EAOrderings[] = {
        {0,1,2},
        {0,2,1},
        {1,2,0},
        {1,0,2},
        {2,0,1},
        {2,1,0},
        {0,1,0},
        {1,2,1},
        {2,0,2},
        };

Value*
eulerToQuat_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(eulerToQuat, 1, count);
	Value* tmp;
	int order = int_key_arg(order, tmp, 1);
	range_check(order, 1, 12, GetString(IDS_AXIS_ORDER_INDEX_OUT_OF_RANGE));
	if (bool_key_arg(sliding, tmp, FALSE)) order += 16;
	if (!is_eulerangles(arg_list[0])) throw ConversionError (arg_list[0], _T("EulerAngles"));
//	Quat res;
//	EulerToQuat((float*)arg_list[0], res, (order-1));
	EulerAnglesValue* eulerAng = (EulerAnglesValue*)arg_list[0];
	Matrix3 tm(1);
	for (int i=0; i<3; i++) {
        switch (EAOrderings[order-1][i]) {
                case 0: tm.RotateX(eulerAng->angles[i]); break;
                case 1: tm.RotateY(eulerAng->angles[i]); break;
                case 2: tm.RotateZ(eulerAng->angles[i]); break;
                }
        }
	return new QuatValue(Quat(tm));
}

Value*
quatToEuler_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(quatToEuler, 1, count);
	Value* tmp;
	int order = int_key_arg(order, tmp, 1);
	range_check(order, 1, 12, GetString(IDS_AXIS_ORDER_INDEX_OUT_OF_RANGE));
	if (bool_key_arg(sliding, tmp, FALSE)) order += 16;
//	QuatToEuler(arg_list[0]->to_quat(), res, (order-1));
	Quat v = arg_list[0]->to_quat();
	float *res = new float [3];
	Matrix3 tm;
	v.MakeMatrix(tm);
	MatrixToEuler(tm, res, order-1);
	Value* result = new EulerAnglesValue(res[0], res[1], res[2]);
	delete [] res;
	return result;
}


//  =================================================
//  UI Items
//  =================================================


/* -------------------- MultiListBoxControl  ------------------- */
/*
rollout test "test"
(MultiListBox mlb "MultiListBox" items:#("A","B","C") selection:#(1,3)
on mlb selected val do format "selected: % - %\n" val mlb.selection[val]
on mlb doubleclicked val do format "doubleclicked: % - %\n" val mlb.selection[val]
on mlb selectionEnd do format "selectionEnd: %\n" mlb.selection
)
rof=newrolloutfloater "tester" 200 300
addrollout test rof
test.mlb.items
test.mlb.selection=1
test.mlb.selection=#(1,3)
test.mlb.selection=#{}

*/
class MultiListBoxControl;
visible_class (MultiListBoxControl)

class MultiListBoxControl : public RolloutControl
{
public:
	Array*		item_array;
	int			lastSelection;
	BitArray*	selection;

	// Constructor
	MultiListBoxControl(Value* name, Value* caption, Value** keyparms, int keyparm_count)
			: RolloutControl(name, caption, keyparms, keyparm_count)
	{
		tag = class_tag(MultiListBoxControl); 
		item_array = NULL;
		selection = new BitArray (0);
	}
	~MultiListBoxControl() { delete selection;}

	static RolloutControl* create(Value* name, Value* caption, Value** keyparms, int keyparm_count)
						{ return new MultiListBoxControl (name, caption, keyparms, keyparm_count); }


	classof_methods (MultiListBoxControl, RolloutControl);

	// Garbage collection
	void		collect() {delete this; }

	// Print out the internal name of the control to MXS
	void		sprin1(CharStream* s) { s->printf(_T("MultiListBoxControl:%s"), name->to_string()); }

	// Return the window class name 
	LPCTSTR		get_control_class() { return _T("MULTILISTBOX"); }
		
	int			num_controls() { return 2; }

	// Top-level call for changing rollout layout. We don't process this.
	void		compute_layout(Rollout *ro, layout_data* pos) { }

	void gc_trace()
	{
		RolloutControl::gc_trace();

		if (item_array && item_array->is_not_marked())
			item_array->gc_trace();
	}


		
	void compute_layout(Rollout *ro, layout_data* pos, int& current_y)
	{
		setup_layout(ro, pos, current_y);

		TCHAR*	label_text = caption->eval()->to_string();
		int label_height = (strlen(label_text) != 0) ? ro->text_height + SPACING_BEFORE - 2 : 0;
		Value* height_obj;
		int item_count = int_control_param(height, height_obj, 10);
		int lb_height = item_count * ro->text_height + 7;

		process_layout_params(ro, pos, current_y);
		pos->height = label_height + lb_height;
		current_y = pos->top + pos->height;
	}

	// Add the control itself to a rollout window
	void add_control(Rollout *ro, HWND parent, HINSTANCE hInstance, int& current_y)
	{
		HWND	label, list_box;
		int		left, top, width, height;
		SIZE	size;
		TCHAR*	label_text = caption->eval()->to_string();

		// add 2 controls for a list box: a static label & the list
		parent_rollout = ro;
		control_ID = next_id();
		WORD label_id = next_id();
		WORD list_box_id = control_ID;

		int label_height = (strlen(label_text) != 0) ? ro->text_height + SPACING_BEFORE - 2 : 0;
		Value* height_obj;
		int item_count = int_control_param(height, height_obj, 10);
		int lb_height = item_count * ro->text_height + 6;

		layout_data pos;
		compute_layout(ro, &pos, current_y);

		// place optional label
		left = pos.left;
		// LAM - defect 298613 - not creating the caption HWND was causing problems (whole screen redrawn
		// when control moved, setting caption text set wrong HWND). Now always create.
//		if (label_height != 0)
//		{
         DLGetTextExtent(ro->rollout_dc, label_text, &size);   
			width = min(size.cx, pos.width); height = ro->text_height;
			top = pos.top; 
			label = CreateWindow(_T("STATIC"),
									label_text,
									WS_VISIBLE | WS_CHILD | WS_GROUP,
									left, top, width, height,    
									parent, (HMENU)label_id, hInstance, NULL);
//		}

		// place list box
		top = pos.top + label_height;
		width = pos.width; height = lb_height;
		list_box = CreateWindowEx(WS_EX_CLIENTEDGE, 
								_T("LISTBOX"),
								_T(""),
								LBS_EXTENDEDSEL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY | WS_BORDER | WS_VSCROLL | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
								left, top, width, height,    
								parent, (HMENU)list_box_id, hInstance, NULL);

		SendMessage(label, WM_SETFONT, (WPARAM)ro->font, 0L);
		SendMessage(list_box, WM_SETFONT, (WPARAM)ro->font, 0L);

		// fill up the list
		if (ro->init_values)
		{
			one_value_local(selection);
			item_array = (Array*)control_param(items);
			if (item_array == (Array*)&unsupplied)
				item_array = new Array (0);
			type_check(item_array, Array, _T("items:"));
			vl.selection = control_param(selection);
			if (vl.selection != &unsupplied) 
				ValueToBitArray(vl.selection, *selection, -1);
			selection->SetSize(item_array->size,1);
			pop_value_locals();
		}

		// add items from array to combobox and compute widest entry & set minwidth
		int max_width = 0;
		SendMessage(list_box, LB_RESETCONTENT, 0, 0);
		for (int i = 0; i < item_array->size; i++)
		{
			TCHAR* item = item_array->data[i]->to_string();
			SendMessage(list_box, LB_ADDSTRING, 0, (LPARAM)item);
         DLGetTextExtent(ro->rollout_dc, item, &size);   
			if (size.cx > max_width) max_width = size.cx;
			SendMessage(list_box, LB_SETSEL, (*selection)[i], i);
		}
	}

	BOOL handle_message(Rollout *ro, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_COMMAND)
		{
			if (HIWORD(wParam) == LBN_SELCHANGE)
         {  
				HWND list_box = GetDlgItem(parent_rollout->page, control_ID);
				int numSel = SendMessage(list_box,LB_GETSELCOUNT,0,0);
				int* selArray = new int[numSel];
				SendMessage(list_box,LB_GETSELITEMS,numSel,(LPARAM)selArray);
				lastSelection = SendMessage(list_box,LB_GETCURSEL,0,0);
				BitArray newSelection (item_array->size);
            for (int i = 0; i < numSel; i++) 
					newSelection.Set(selArray[i]);
				one_value_local(sel);
				BOOL selChanged = FALSE;
            for (int i = 0; i < item_array->size; i++) {
					if (newSelection[i] != (*selection)[i]) {
						selection->Set(i,newSelection[i]);
						vl.sel = Integer::intern(i + 1);
						call_event_handler(ro, n_selected, &vl.sel, 1);
						selChanged = TRUE;
					}
				}
				pop_value_locals();
				delete [] selArray;
				if (selChanged) call_event_handler(ro, n_selectionEnd, NULL, 0);
			}
			else if (HIWORD(wParam) == LBN_DBLCLK)
			{
				one_value_local(sel);
				vl.sel = Integer::intern(lastSelection + 1);
				call_event_handler(ro, n_doubleClicked, &vl.sel, 1); 
				pop_value_locals();
			}
			return TRUE;
		}
		return FALSE;
	}

	Value* set_property(Value** arg_list, int count)
	{
		Value* val = arg_list[0];
		Value* prop = arg_list[1];

		if (prop == n_text || prop == n_caption)
		{
			TCHAR* text = val->to_string();
			caption = val->get_heap_ptr();
			if (parent_rollout != NULL && parent_rollout->page != NULL)
				set_text(text, GetDlgItem(parent_rollout->page, control_ID + 1), n_left);
		}
		else if (prop == n_items)
		{
			type_check(val, Array, _T("items:"));
			item_array = (Array*)val;
			selection->SetSize(item_array->size,1);
			if (parent_rollout != NULL && parent_rollout->page != NULL)
			{
				HWND list_box = GetDlgItem(parent_rollout->page, control_ID);
				SendMessage(list_box, LB_RESETCONTENT, 0, 0);
				for (int i = 0; i < item_array->size; i++) {
					SendMessage(list_box, LB_ADDSTRING, 0, (LPARAM)item_array->data[i]->to_string());
					SendMessage(list_box, LB_SETSEL, (*selection)[i], i);
				}

			}
		}
		else if (prop == n_selection)
		{
			selection->ClearAll();
			ValueToBitArray(val, *selection, -1);
			selection->SetSize(item_array->size,1);
			if (parent_rollout != NULL && parent_rollout->page != NULL)
			{
				HWND list_box = GetDlgItem(parent_rollout->page, control_ID);
				for (int i = 0; i < item_array->size; i++) 
					SendMessage(list_box, LB_SETSEL, (*selection)[i], i);
			}
		}
		else if(prop == n_height)
		{
			if (parent_rollout != NULL && parent_rollout->page != NULL)
			{
				HWND list_box = GetDlgItem(parent_rollout->page, control_ID);
				int newHeight = val->to_int(); 
				RECT rect;
				GetWindowRect(list_box, &rect);
				MapWindowPoints(NULL, parent_rollout->page, (LPPOINT)&rect, 2);
				SetWindowPos(list_box, NULL, rect.left, rect.top, rect.right-rect.left, newHeight, SWP_NOZORDER);
			}
		}
		else if(prop == n_width)
		{
			if (parent_rollout != NULL && parent_rollout->page != NULL)
			{
				HWND list_box = GetDlgItem(parent_rollout->page, control_ID);
				int newWidth = val->to_int(); 
				RECT rect;
				GetWindowRect(list_box, &rect);
				MapWindowPoints(NULL, parent_rollout->page, (LPPOINT)&rect, 2);
				SetWindowPos(list_box, NULL, rect.left, rect.top, newWidth, rect.bottom-rect.top, SWP_NOZORDER);
			}
		}
		else
			return RolloutControl::set_property(arg_list, count);

		return val;
	}

	Value* get_property(Value** arg_list, int count)
	{
		Value* prop = arg_list[0];

		if (prop == n_items)
			return item_array;
		else if (prop == n_selection)
			return new BitArrayValue(*selection);
		else if (prop == n_doubleClicked || prop == n_selected || prop == n_selectionEnd)
			return get_wrapped_event_handler(prop);
		else if(prop == n_height)
		{
			if (parent_rollout != NULL && parent_rollout->page != NULL)
			{
				HWND list_box = GetDlgItem(parent_rollout->page, control_ID);
				RECT rect;
				GetWindowRect(list_box, &rect);
				return Integer::intern(rect.bottom-rect.top);
			}
			else
				return &undefined;
		}
		else if(prop == n_width)
		{
			if (parent_rollout != NULL && parent_rollout->page != NULL)
			{
				HWND list_box = GetDlgItem(parent_rollout->page, control_ID);
				RECT rect;
				GetWindowRect(list_box, &rect);
				return Integer::intern(rect.right-rect.left);
			}
			else
				return &undefined;
		}
		else
			return RolloutControl::get_property(arg_list, count);
	}

	void set_enable()
	{
		if (parent_rollout != NULL && parent_rollout->page != NULL)
		{
			// set combox enable
			EnableWindow(GetDlgItem(parent_rollout->page, control_ID), enabled);
			// set caption enable
			HWND ctrl = GetDlgItem(parent_rollout->page, control_ID + 1);
			if (ctrl)
				EnableWindow(ctrl, enabled);
		}
	}
};

visible_class_instance (MultiListBoxControl, "MultiListBoxControl");

//  =================================================
//  Bitmap File Open/Save Filename Dialogs
//  =================================================

Value* 
getBitmapOpenFileName_cf(Value** arg_list, int count)
{
//	getBitmapOpenFileName caption:<title> filename:<seed_filename_string>
	check_arg_count_with_keys(getBitmapOpenFileName, 0, count);
	BitmapInfo bi;
	Value* cap = key_arg(caption);
	TCHAR*  cap_str = (cap != &unsupplied) ? cap->to_string() : NULL ;
	Value* fn = key_arg(filename);
	if (fn != &unsupplied) 
	{	TSTR fname = fn->to_filename();
		for (TCHAR* e = fname; *e; e++) if (*e == _T('/')) *e = _T('\\'); // xlate '/' to '\'
		bi.SetName(fname);
	}

	if (TheManager->SelectFileInput(&bi, GetActiveWindow(), cap_str))
		return new String((TCHAR*)bi.Name());
	else
		return &undefined;
}

Value* 
getBitmapSaveFileName_cf(Value** arg_list, int count)
{
//	getBitmapSaveFileName caption:<title> filename:<seed_filename_string>
	check_arg_count_with_keys(getBitmapSaveFileName, 0, count);
	BitmapInfo bi;
	Value* cap = key_arg(caption);
	TCHAR*  cap_str = (cap != &unsupplied) ? cap->to_string() : NULL ;
	Value* fn = key_arg(filename);
	if (fn != &unsupplied) 
	{	TSTR fname = fn->to_filename();
		for (TCHAR* e = fname; *e; e++) if (*e == _T('/')) *e = _T('\\'); // xlate '/' to '\'
		bi.SetName(fname);
	}
	if (TheManager->SelectFileOutput(&bi, GetActiveWindow(), cap_str))
		return new String((TCHAR*)bi.Name());
	else
		return &undefined;
}

Value* 
doesFileExist_cf(Value** arg_list, int count)
{
//	doesFileExist <filename_string>
	check_arg_count(doesFileExist, 1, count);
	return (DoesFileExist(arg_list[0]->to_filename()) ? &true_value : &false_value);
}

//  =================================================
//  MAX File Open/Save Filename Dialogs
//  =================================================

Value* 
getMAXOpenFileName_cf(Value** arg_list, int count)
{
//	getMAXOpenFileName filename:<seed_filename_string> dir:<seed_directory_string>
	check_arg_count_with_keys(getMAXOpenFileName, 0, count);
	TSTR fname;
	TSTR* defFile = NULL;
	TSTR* defDir = NULL;
	Value* dfn = key_arg(filename);
	if (dfn != &unsupplied) defFile = new TSTR (dfn->to_string());
	Value* ddir = key_arg(dir);
	if (ddir != &unsupplied) 
	{	defDir = new TSTR (ddir->to_filename());
		for (TCHAR* e = defDir->data(); *e; e++) if (*e == _T('/')) *e = _T('\\'); // xlate '/' to '\'
	}

	Value* res;
	if (MAXScript_interface7->DoMaxFileOpenDlg(fname, defDir, defFile))
		res = new String(fname);
	else
		res = &undefined;
	if (defFile) delete defFile;
	if (defDir) delete defDir;
	return res;
}


Value* 
getMAXSaveFileName_cf(Value** arg_list, int count)
{
//	getMAXSaveFileName filename:<seed_filename_string>
	check_arg_count_with_keys(getMAXSaveFileName, 0, count);
	TSTR fname;
	Value* dfn = key_arg(filename);
	if (dfn != &unsupplied) 
	{	fname = dfn->to_filename();
		for (TCHAR* e = fname; *e; e++) if (*e == _T('/')) *e = _T('\\'); // xlate '/' to '\'
	}
	if (MAXScript_interface7->DoMaxFileSaveAsDlg(fname, FALSE))
		return new String(fname);
	else
		return &undefined;
}

//  =================================================
//  Miscellaneous Stuff
//  =================================================

Value* 
getCurNameSelSet_cf(Value** arg_list, int count)
{
//	getCurNameSelSet()
	check_arg_count(getCurNameSelSet, 0, count);
	const int BUFLEN = 17;
	TCHAR buf[BUFLEN];
	GetWindowText( GetCUIFrameMgr()->GetItemHwnd(50037), buf, BUFLEN );
	return new String(buf);
}

Value* 
isDebugBuild_cf(Value** arg_list, int count)
{
//	isDebugBuild()
#ifdef _DEBUG
	return &true_value;
#endif
	return &false_value;

}

Value*
get_superclasses()
{
	one_typed_value_local(Array* result);
	vl.result = new Array (maxwrapper_class.getNumSuperclasses ());
	for (int i = 0; i < maxwrapper_class.getNumSuperclasses (); i++)
		vl.result->append(maxwrapper_class.getSuperclass (i));
	return_value(vl.result);
}

Value*
setFocus_cf(Value** arg_list, int count)
{
	check_arg_count(SetFocus, 1, count);
	if (is_rolloutcontrol(arg_list[0]))
	{
		RolloutControl* roc = (RolloutControl*)arg_list[0];
		return roc->set_focus() ? &true_value : &false_value;
//		if (roc->parent_rollout && roc->parent_rollout->page)
//			SetFocus(roc->parent_rollout->page);
	}
	else if (arg_list[0]->tag == class_tag(RolloutFloater))
	{
		RolloutFloater* rof = (RolloutFloater*)arg_list[0];
		if (rof->window)
		{
			SetFocus(rof->window);
			return &true_value;
		}
	}
	else if (arg_list[0]->tag == class_tag(Rollout))
	{
		Rollout* ro = (Rollout*)arg_list[0];
		if (ro->page)
		{
			SetFocus(ro->page);
			return &true_value;
		}
	}
	else
		throw RuntimeError (GetString(IDS_SETFOCUS_ARG_TYPE_ERROR), arg_list[0]);
	return &false_value;
}


//  =================================================
//  Bit operations
//  =================================================

union shareIntFloat
{
	int iVal;
	INT64 i64Val;
	UINT i64Valpart[2];
	INT_PTR iptrVal;
	float fVal;
	double dVal;
	byte bVal[8];
};

Value* 
bit_and_cf(Value** arg_list, int count)
{
//	and <integer> <integer>
	check_arg_count(and, 2, count);
	if (is_int64(arg_list[0]))
		return Integer64::intern(arg_list[0]->to_int64() & arg_list[1]->to_int64());
	else if (is_intptr(arg_list[0]))
		return IntegerPtr::intern(arg_list[0]->to_intptr() & arg_list[1]->to_intptr());
	else
		return Integer::intern(arg_list[0]->to_int() & arg_list[1]->to_int());
}

Value* 
bit_or_cf(Value** arg_list, int count)
{
//	or <integer> <integer>
	check_arg_count(or, 2, count);
	if (is_int64(arg_list[0]))
		return Integer64::intern(arg_list[0]->to_int64() | arg_list[1]->to_int64());
	else if (is_intptr(arg_list[0]))
		return IntegerPtr::intern(arg_list[0]->to_intptr() | arg_list[1]->to_intptr());
	else
		return Integer::intern(arg_list[0]->to_int() | arg_list[1]->to_int());
}

Value* 
bit_xor_cf(Value** arg_list, int count)
{
//	xor <integer> <integer>
	check_arg_count(xor, 2, count);
	if (is_int64(arg_list[0]))
		return Integer64::intern(arg_list[0]->to_int64() ^ arg_list[1]->to_int64());
	else if (is_intptr(arg_list[0]))
		return IntegerPtr::intern(arg_list[0]->to_intptr() ^ arg_list[1]->to_intptr());
	else
		return Integer::intern(arg_list[0]->to_int() ^ arg_list[1]->to_int());
}

Value* 
bit_not_cf(Value** arg_list, int count)
{
//	not <integer>
	check_arg_count(not, 1, count);
	if (is_int64(arg_list[0]))
		return Integer64::intern(~arg_list[0]->to_int64());
	else if (is_intptr(arg_list[0]))
		return IntegerPtr::intern(~arg_list[0]->to_intptr());
	else
		return Integer::intern(~arg_list[0]->to_int());
}

Value* 
bit_shift_cf(Value** arg_list, int count)
{
//	shift <integer> <integer> -- positive values shift to left
	check_arg_count(shift, 2, count);
	int shiftCount = arg_list[1]->to_int();
	if (is_int64(arg_list[0]))
	{
		UINT64 val = arg_list[0]->to_int64();
		int int64_num_bits = sizeof(INT64) * 8;
		range_check(shiftCount, -int64_num_bits, int64_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		if (shiftCount > 0)
			val <<= shiftCount;
		else if (shiftCount < 0)
			val >>= (UINT) abs(shiftCount);
		return Integer64::intern(val);
	}
	else if (is_intptr(arg_list[0]))
	{
		UINT_PTR val = arg_list[0]->to_intptr();
		int intptr_num_bits = sizeof(INT_PTR) * 8;
		range_check(shiftCount, -intptr_num_bits, intptr_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		if (shiftCount > 0)
			val <<= shiftCount;
		else if (shiftCount < 0)
			val >>= (UINT) abs(shiftCount);
		return IntegerPtr::intern(val);
	}
	else
	{
		UINT val = arg_list[0]->to_int();
		int int_num_bits = sizeof(int) * 8;
		range_check(shiftCount, -int_num_bits, int_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		if (shiftCount > 0)
			val <<= shiftCount;
		else if (shiftCount < 0)
			val >>= (UINT) abs(shiftCount);
		return Integer::intern(val);
	}
}

Value* 
bit_set_cf(Value** arg_list, int count)
{
//	set <integer> <integer> <bool>
	check_arg_count(set, 3, count);
	int whichBit = arg_list[1]->to_int();
	if (is_int64(arg_list[0]))
	{
		INT64 val = arg_list[0]->to_int64();
		INT64 mask = 1;
		mask <<= (whichBit-1);
		int int64_num_bits = sizeof(INT64) * 8;
		range_check(whichBit, 1, int64_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		return Integer64::intern((arg_list[2]->to_bool()) ? val | mask : val & ~mask);
	}
	else if (is_intptr(arg_list[0]))
	{
		INT_PTR val = arg_list[0]->to_intptr();
		INT_PTR mask = 1;
		mask <<= (whichBit-1);
		int intptr_num_bits = sizeof(INT_PTR) * 8;
		range_check(whichBit, 1, intptr_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		return IntegerPtr::intern((arg_list[2]->to_bool()) ? val | mask : val & ~mask);
	}
	else
	{
		int val = arg_list[0]->to_int();
		int mask = 1 << (whichBit-1);
		int int_num_bits = sizeof(int) * 8;
		range_check(whichBit, 1, int_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		return Integer::intern((arg_list[2]->to_bool()) ? val | mask : val & ~mask);
	}
}

Value* 
bit_flip_cf(Value** arg_list, int count)
{
//	flip <integer> <integer>
	check_arg_count(flip, 2, count);
	int whichBit = arg_list[1]->to_int();
	if (is_int64(arg_list[0]))
	{
		INT64 val = arg_list[0]->to_int64();
		INT64 mask = 1;
		mask <<= (whichBit-1);
		int int64_num_bits = sizeof(INT64) * 8;
		range_check(whichBit, 1, int64_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		return Integer64::intern(val ^ mask);
	}
	else if (is_intptr(arg_list[0]))
	{
		INT_PTR val = arg_list[0]->to_intptr();
		INT_PTR mask = 1;
		mask <<= (whichBit-1);
		int intptr_num_bits = sizeof(INT_PTR) * 8;
		range_check(whichBit, 1, intptr_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		return IntegerPtr::intern(val ^ mask);
	}
	else
	{
		int val = arg_list[0]->to_int();
		int mask = 1 << (whichBit-1);
		int int_num_bits = sizeof(int) * 8;
		range_check(whichBit, 1, int_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		return Integer::intern(val ^ mask);
	}
}

Value* 
bit_get_cf(Value** arg_list, int count)
{
//	get <integer> <integer>
	check_arg_count(get, 2, count);
	int whichBit = arg_list[1]->to_int();
	if (is_int64(arg_list[0]))
	{
		INT64 val = arg_list[0]->to_int64();
		INT64 mask = 1;
		mask <<= (whichBit-1);
		int int64_num_bits = sizeof(INT64) * 8;
		range_check(whichBit, 1, int64_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		return (val & mask) ? &true_value : &false_value;
	}
	else if (is_intptr(arg_list[0]))
	{
		INT_PTR val = arg_list[0]->to_intptr();
		INT_PTR mask = 1;
		mask <<= (whichBit-1);
		int intptr_num_bits = sizeof(INT_PTR) * 8;
		range_check(whichBit, 1, intptr_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		return (val & mask) ? &true_value : &false_value;
	}
	else
	{
		int val = arg_list[0]->to_int();
		int mask = 1 << (whichBit-1);
		int int_num_bits = sizeof(int) * 8;
		range_check(whichBit, 1, int_num_bits, GetString(IDS_BIT_INDEX_OR_SHIFT_COUNT_OUT_OF_RANGE));
		return (val & mask) ? &true_value : &false_value;
	}
}

Value* 
bit_intAsChar_cf(Value** arg_list, int count)
{
//	intAsChar <integer> 
	check_arg_count(intAsChar, 1, count);
	TCHAR buf[2] = {0};
	int val = arg_list[0]->to_int();
	buf[0]= val & ((sizeof(TCHAR) == 1) ? 0x0FF : 0x0FFFF);
	return new String(buf);
}

Value* 
bit_charAsInt_cf(Value** arg_list, int count)
{
//	charAsInt <string> 
	check_arg_count(charAsInt, 1, count);
	int val = (arg_list[0]->to_string())[0];
	return Integer::intern(val & ((sizeof(TCHAR) == 1) ? 0x0FF : 0x0FFFF));
}


Value* 
bit_intAsHex_cf(Value** arg_list, int count)
{
//	intAsHex <int> 
	check_arg_count(intAsHex, 1, count);
	const int BUFLEN = 32;
	TCHAR buf[BUFLEN] = {0};
	if (is_int64(arg_list[0]))
	{
		shareIntFloat ifUnion;
		ifUnion.i64Val = arg_list[0]->to_int64();
		if (ifUnion.i64Valpart[1] == 0)
			wsprintf (buf, "%xL", ifUnion.i64Valpart[0]);
		else
			wsprintf (buf, "%x%08xL", ifUnion.i64Valpart[1], ifUnion.i64Valpart[0]);
	}
	else if (is_intptr(arg_list[0]))
		wsprintf (buf, "%pP", arg_list[0]->to_intptr());
	else
		wsprintf (buf, "%x", arg_list[0]->to_int());
	return new String(buf);
}

Value* 
bit_intAsFloat_cf(Value** arg_list, int count)
{
	//	intAsFloat <integer> 
	check_arg_count(intAsFloat, 1, count);
	shareIntFloat ifUnion;
	ifUnion.iVal = arg_list[0]->to_int();
	return Float::intern(ifUnion.fVal);
}

Value* 
bit_floatAsInt_cf(Value** arg_list, int count)
{
	//	floatAsInt <float> 
	check_arg_count(floatAsInt, 1, count);
	shareIntFloat ifUnion;
	ifUnion.fVal = arg_list[0]->to_float();
	return Integer::intern(ifUnion.iVal);
}

Value* 
bit_int64AsDouble_cf(Value** arg_list, int count)
{
	//	int64AsDouble <integer64> 
	check_arg_count(int64AsDouble, 1, count);
	shareIntFloat ifUnion;
	ifUnion.i64Val = arg_list[0]->to_int64();
	return Double::intern(ifUnion.dVal);
}

Value* 
bit_doubleAsInt64_cf(Value** arg_list, int count)
{
	//	doubleAsInt64 <double> 
	check_arg_count(doubleAsInt64, 1, count);
	shareIntFloat ifUnion;
	ifUnion.dVal = arg_list[0]->to_double();
	return Integer64::intern(ifUnion.i64Val);
}

Value* 
bit_swapBytes_cf(Value** arg_list, int count)
{
	//	swapBytes <integer> <integer> <integer>
	check_arg_count(floatAsInt, 3, count);
	shareIntFloat ifUnion;
	ifUnion.i64Val = arg_list[0]->to_int64();
	int from = arg_list[1]->to_int();
	int to = arg_list[2]->to_int();
	int max_num_bytes = sizeof(int);
	if (is_int64(arg_list[0]))
		max_num_bytes = sizeof(INT64);
	else if (is_intptr(arg_list[0]))
		max_num_bytes = sizeof(INT_PTR);
	range_check(from, 1, max_num_bytes, _T("byte index out of range: "));
	range_check(to, 1, max_num_bytes, _T("byte index out of range: "));
	from--;
	to--;
	byte tmp = ifUnion.bVal[to];
	ifUnion.bVal[to] = ifUnion.bVal[from];
	ifUnion.bVal[from] = tmp;
	if (is_int64(arg_list[0]))
		return Integer64::intern(ifUnion.i64Val);
	else if (is_intptr(arg_list[0]))
		return IntegerPtr::intern(ifUnion.iptrVal);
	return Integer::intern(ifUnion.iVal);
}

Value* 
bit_isNAN_cf(Value** arg_list, int count)
{
	//	isNAN <float|double> 
	check_arg_count(isNAN, 1, count);
	if (is_double(arg_list[0]))
	{
		double dVal = arg_list[0]->to_double();
		return (_isnan(dVal)) ? &true_value : &false_value;
	}
	float fVal = arg_list[0]->to_float();
	return (_isnan(fVal)) ? &true_value : &false_value;
}

Value* 
bit_isFinite_cf(Value** arg_list, int count)
{
	//	isFinite <float|double> 
	check_arg_count(isFinite, 1, count);
	if (is_double(arg_list[0]))
	{
		double dVal = arg_list[0]->to_double();
		return (_finite(dVal)) ? &true_value : &false_value;
	}
	float fVal = arg_list[0]->to_float();
	return (_finite(fVal)) ? &true_value : &false_value;
}

Value* 
bit_hexAsInt_cf(Value** arg_list, int count)
{
	//	hexAsInt <string> 
	check_arg_count(hexAsInt, 1, count);
	// see if whether int64 or intptr by whether last character is an L or a P. If neither, its an int.
	TCHAR* buf = arg_list[0]->to_string();
	INT64 val = 0;
	int i = 0;
	int len = static_cast<int>(_tcslen(buf));
	if (len > 2 && buf[0] == _T('0') && buf[1] == _T('x')) i = 2;
	for (; i < len; i++)
	{
		TCHAR c = buf[i];
		if (isdigit(c))
			val = (val * 16) + (c - _T('0'));
		else if (c >= _T('a') && c <= _T('f'))
			val = (val * 16) + 10 + (c - _T('a'));
		else if (c >= _T('A') && c <= _T('F'))
			val = (val * 16) + 10 + (c - _T('A'));
		else if (c == _T('L'))
			return Integer64::intern(val);
		else if (c == _T('P'))
			return IntegerPtr::intern(val);
		else
			throw RuntimeError (GetString(IDS_INVALID_HEX_STRING), arg_list[0]);
	}
	return Integer::intern(val);
}

//  =================================================
//  Value hasher
//  =================================================

// methods for calculating hash value for various class values

#define HASHPRECISION 6 

// form the hash value for string s
inline void Hash(TCHAR* s, UINT& hashVal)
{
	for (; *s; s++)
		hashVal = *s + 31 * hashVal;
	return;
}

// form the hash value for integer i
inline void Hash(int i, UINT& hashVal)
{
	hashVal = i + 31 * hashVal;
	return;
}

// decompose a float into integer and exponent given precision
inline void decomposeFloat(float v, int& mantissa, int& exponent)
{	if (v == 0.0f) {
		exponent = mantissa = 0;
		return;
	}
	BOOL neg = FALSE;
	if (v < 0.0f) {
		neg = TRUE;
		v = fabsf(v);
	}
	exponent = int(log10f (v));
	if (v >= 1.0f) exponent += 1;
	mantissa = int(v*(float(10^(HASHPRECISION-exponent)) +0.5f));
	return;
}

// form the hash value for float f
inline void Hash(float f, UINT& hashVal)
{	int mantissa, exponent;
	decomposeFloat(f, mantissa, exponent);
	Hash(mantissa, hashVal);
	Hash(exponent, hashVal);
	return;
}

// form the hash value for Point2 p
inline void Hash(Point2 p, UINT& hashVal)
{	Hash(p.x, hashVal);
	Hash(p.y, hashVal);
	return;
}

// form the hash value for Point3 p
inline void Hash(Point3 p, UINT& hashVal)
{	Hash(p.x, hashVal);
	Hash(p.y, hashVal);
	Hash(p.z, hashVal);
	return;
}

// form the hash value for AColor c
inline void Hash(AColor c, UINT& hashVal)
{	Hash(c.r, hashVal);
	Hash(c.g, hashVal);
	Hash(c.b, hashVal);
	Hash(c.a, hashVal);
	return;
}

// form the hash value for Quat q
inline void Hash(Quat q, UINT& hashVal)
{	Hash(q.x, hashVal);
	Hash(q.y, hashVal);
	Hash(q.z, hashVal);
	Hash(q.w, hashVal);
	return;
}

// form the hash value for Angle Axis a
inline void Hash(AngAxis a, UINT& hashVal)
{	Hash(a.axis, hashVal);
	Hash(a.angle, hashVal);
	return;
}

// form the hash value for Matrix3 m
inline void Hash(Matrix3 m, UINT& hashVal)
{	Hash(m.GetRow(0), hashVal);
	Hash(m.GetRow(1), hashVal);
	Hash(m.GetRow(2), hashVal);
	Hash(m.GetRow(3), hashVal);
	return;
}

// form the hash value for Interval t
inline void Hash(Interval t, UINT& hashVal)
{	Hash(t.Start(), hashVal);
	Hash(t.End(), hashVal);
	return;
}

// form the hash value for Ray r
inline void Hash(Ray r, UINT& hashVal)
{	Hash(r.p, hashVal);
	Hash(r.dir, hashVal);
	return;
}

// form the hash value for BitArray b
inline void Hash(BitArray b, UINT& hashVal)
{	
	for (int i = 0; i < b.GetSize(); i++) {
		if (b[i]) Hash(i, hashVal);
	}
	return;
}

BOOL 
getHashValue(Value* val, UINT& HashValue)
{	if (is_float_number(val)) 
		Hash(val->to_float(),HashValue);
	else if (is_integer_number(val)) 
		Hash(val->to_int(),HashValue);
	else if (val->is_kind_of(class_tag(String))) 
		Hash(val->to_string(),HashValue);
	else if (val->is_kind_of(class_tag(Array))) {
		Array* theArray = (Array*)val;
		for (int i = 0; i < theArray->size; i++) {
			BOOL ok=getHashValue(theArray->data[i],HashValue);
			if (!ok) return FALSE;
		}
	}
	else if (val->is_kind_of(class_tag(BitArrayValue))) 
		Hash(val->to_bitarray(),HashValue);
	else if (val->is_kind_of(class_tag(Point3Value))) 
		Hash(val->to_point3(),HashValue);
	else if (val->is_kind_of(class_tag(RayValue))) 
		Hash(val->to_ray(),HashValue);
	else if (val->is_kind_of(class_tag(QuatValue))) 
		Hash(val->to_quat(),HashValue);
	else if (val->is_kind_of(class_tag(AngAxisValue))) 
		Hash(val->to_angaxis(),HashValue);
	else if (val->is_kind_of(class_tag(EulerAnglesValue))) {
		Hash(((EulerAnglesValue*)val)->angles[0],HashValue);
		Hash(((EulerAnglesValue*)val)->angles[1],HashValue);
		Hash(((EulerAnglesValue*)val)->angles[2],HashValue);
	}
	else if (val->is_kind_of(class_tag(Matrix3Value))) 
		Hash(val->to_matrix3(),HashValue);
	else if (val->is_kind_of(class_tag(Point2Value))) 
		Hash(val->to_point2(),HashValue);
	else if (val->is_kind_of(class_tag(ColorValue))) 
		Hash(val->to_acolor(),HashValue);
	else
		return FALSE;
	return TRUE;
}

// the MAXScript interface

Value* 
getHashValue_cf(Value** arg_list, int count)
{//	getHashValue <value> <oldHashValue> 
	check_arg_count_with_keys(getHashValue, 2, count);
	UINT HashValue = (UINT)arg_list[1]->to_int();
	return (getHashValue(arg_list[0], HashValue) ? Integer::intern((int)HashValue) : &undefined);
}

Value* 
int_cf(Value** arg_list, int count)
{
//	int <number> 
	check_arg_count(int, 1, count);
	return Integer::intern(arg_list[0]->to_int());
}

Value*
getCoreInterfaces_cf(Value** arg_list, int count)
{
	four_typed_value_locals(Array* result, Value* name, FPInterfaceValue* i, Value* g);
	vl.result = new Array (NumCOREInterfaces());
	for (int i = 0; i < NumCOREInterfaces(); i++)
	{
		// look at each interface
		FPInterface* fpi = GetCOREInterfaceAt(i);
		FPInterfaceDesc* fpid = fpi->GetDesc();
		if (!(fpid->flags & (FP_TEST_INTERFACE | FP_MIXIN)) && !fpid->internal_name.isNull())
		{
			vl.name = Name::intern(fpid->internal_name);
			vl.g = globals->get(vl.name);
			if (vl.g != NULL && !(is_thunk(vl.g) && vl.g->eval()->tag == class_tag(FPInterfaceValue)))
			{
				TSTR newName = vl.name->to_string();
				newName.append(_T("Interface"));
				vl.name = Name::intern(newName);
				vl.g = globals->get(vl.name);
			}
			if (vl.g == NULL)
			{
				vl.i = new FPInterfaceValue (fpi);
				vl.g = new ConstGlobalThunk (vl.name, vl.i);
				globals->put_new(vl.name, vl.g);
			}
			if (vl.g)
				vl.result->append(vl.g->eval());
		}
	}
	return_value(vl.result);
}

Value*
getTextExtent_cf(Value** arg_list, int count)
{
	check_arg_count(getTextExtent, 1, count);
	TCHAR* text = arg_list[0]->to_string();
	SIZE size;
	HWND hwd = MAXScript_interface->GetMAXHWnd();
	HDC hdc = GetDC(hwd);
	HFONT font = MAXScript_interface->GetAppHFont();
	SelectObject(hdc, font);
   DLGetTextExtent(hdc,text,&size);
	ReleaseDC(hwd, hdc);
	return new Point2Value((float)size.cx, (float)size.cy);
}


Value*
isValidNode_cf(Value** arg_list, int count)
{
	check_arg_count(isValidNode, 1, count);
	MAXNode* node = (MAXNode*)arg_list[0];
	return (is_node(node) && !(deletion_check_test(node))) ? &true_value : &false_value;
}


Value*
isValidObj_cf(Value** arg_list, int count)
{
	check_arg_count(isValidObj, 1, count);
	MAXWrapper* obj = (MAXWrapper*)arg_list[0];
	return (obj->is_kind_of(class_tag(MAXWrapper)) && !deletion_check_test(obj)) ? &true_value : &false_value;
}

Value*
isPB2Based_cf(Value** arg_list, int count)
{
	check_arg_count(isPB2Based, 1, count);
	return (arg_list[0]->is_kind_of(class_tag(MAXClass)) && ((MAXClass*)arg_list[0])->cd2) ? &true_value : &false_value;
}

Value*
updateToolbarButtons_cf(Value** arg_list, int count)
{
	check_arg_count(updateToolbarButtons, 0, count);
	GetCUIFrameMgr()->SetMacroButtonStates(FALSE);
	return &ok;
}

static Value* productAppID = NULL;

Value*
get_productAppID()
{
	if (productAppID == NULL)
	{
		three_typed_value_locals(StringStream* source, Parser* parser, Value* code);
		vl.source = new StringStream (_T("maxops.productAppID"));
		vl.parser = new Parser (thread_local(current_stdout));

		// loop through expr compiling & evaling all expressions,
		// keep the code of last expression as expr for this controller
		vl.source->flush_whitespace();
		vl.code = vl.parser->compile_all(vl.source);
		vl.source->close();
		productAppID = vl.code->eval()->make_heap_permanent();
		pop_value_locals();
	}
	return (productAppID) ? productAppID : &undefined;
}


Value*
okToCreate_cf(Value** arg_list, int count)
{
	// okToCreate <maxobjclass>     
	check_arg_count(okToCreate, 1, count);

	if (arg_list[0] == &undefined) return &false_value; //RK: 10/10/01, case when some plugins are missing

	MAXClass* mc = (MAXClass*)arg_list[0];

#ifdef WEBVERSION
	// If we're given a simple string, try to map it to the appropriate MAXClass
	if(is_string(arg_list[0]))
	{
		int i;
		class_table* ct;
		mc = NULL;
		for(ct = MAXClass::classes, i = 0; i < MAXClass::n_classes; i++, ct++)
			if(_tcsicmp(ct->mx_class->name->to_string(), arg_list[0]->to_string()) == 0)
			{
				mc = ct->mx_class;
				break;
			}
	}
#endif // WEBVERSION

	if (!mc->is_kind_of(class_tag(MAXClass)))
		throw RuntimeError ("okToCreate() requires a MAXClass parameter, got: ", mc);
	ClassDirectory& cdir = MAXScript_interface->GetDllDir().ClassDir();
	//RK: 10/10/01, consider the case where the plugin is missing
	ClassEntry* ce = cdir.FindClassEntry(mc->sclass_id, mc->class_id);
	if (!ce) return &false_value;
	ClassDesc* cd = ce->FullCD();
	return (cd->IsPublic() && cd->OkToCreate(GetCOREInterface())) ? &true_value : &false_value;
}

Value*
get_VptUseDualPlanes()
{
	return (MAXScript_interface7->GetDualPlanes() ? &true_value : &false_value);
}

Value*
set_VptUseDualPlanes(Value* val)
{
	MAXScript_interface7->SetDualPlanes(val->to_bool());
	return val;
}

Value*
getManipulateMode()
{
	return (MAXScript_interface7->InManipMode() ? &true_value : &false_value);
}

Value*
setManipulateMode(Value* val)
{
	if (val->to_bool()) 
	{	if (!MAXScript_interface7->InManipMode())
			MAXScript_interface7->StartManipulateMode();
	}
	else
		MAXScript_interface7->EndManipulateMode();
	return val;
}

Value*
getLastRenderedImage_cf(Value** arg_list, int count)
{
	Bitmap* image = MAXScript_interface7->GetLastRenderedImage();
	if (image == NULL) return &undefined;
	one_typed_value_local(MAXBitMap* mbm);
	vl.mbm = new MAXBitMap ();
	vl.mbm->bi.CopyImageInfo(&image->Storage()->bi);
	vl.mbm->bi.SetFirstFrame(0);
	vl.mbm->bi.SetLastFrame(0);
	vl.mbm->bi.SetName("");
	vl.mbm->bi.SetDevice("");
	if (vl.mbm->bi.Type() > BMM_TRUE_64)
		vl.mbm->bi.SetType(BMM_TRUE_64);
	vl.mbm->bm = TheManager->Create(&vl.mbm->bi);
	BMM_Color_64 black64(0,0,0,0);
	vl.mbm->bm->CopyImage(image, COPY_IMAGE_CROP, black64, &vl.mbm->bi);
	return_value(vl.mbm);
}

Value*
get_coordsys_node()
{
	INode *coordNode = MAXScript_interface7->GetRefCoordNode();
	if (coordNode == NULL) 
		return &undefined;
	else
		return MAXNode::intern(coordNode);
}

Value*
set_coordsys_node(Value* val)
{
	MAXScript_interface7->AddRefCoordNode(val->to_node());
	return val;
}

// ============================================================================
// <Point2Value> getMAXWindowSize
Value* getMAXWindowSize_cf( Value** arg_list, int count )
{
	RECT rect;
	check_arg_count( getMAXWindowSize, 0, count );
	GetWindowRect( GetCOREInterface()->GetMAXHWnd(), &rect );
	return new Point2Value( rect.right-rect.left, rect.bottom-rect.top );
}

// <Point2Value> getMAXWindowPos
Value* getMAXWindowPos_cf( Value** arg_list, int count )
{
	RECT rect;
	check_arg_count( getMAXWindowPos, 0, count );
	GetWindowRect( GetCOREInterface()->GetMAXHWnd(), &rect );
	return new Point2Value( rect.left, rect.top );
}

Value*
getSelectionLevel_cf(Value** arg_list, int count)
{
	// getSelectionLevel <maxobject>
	//                                         
	check_arg_count(getSelectionLevel, 1, count);
	IMeshSelect *ims = NULL;
	ISplineSelect *iss = NULL;
	IPatchSelect *ips = NULL;
	Animatable *target = NULL;
	int level;
	if (is_node(arg_list[0]))
      target = Get_Object_Or_XRef_BaseObject(arg_list[0]->to_node()->GetObjectRef());
//	else if (is_modifier(arg_list[0]))
//		target = arg_list[0]->to_modifier();
	else
		target = arg_list[0]->to_reftarg();

   // need to block xref object access
   if (target->GetInterface(IID_XREF_ITEM))
      target = NULL;

	if (target)
	{	
		if (ims = (IMeshSelect*)target->GetInterface(I_MESHSELECT))
			level = ims->GetSelLevel();
		else if (iss = (ISplineSelect*)target->GetInterface(I_SPLINESELECT))
			level = iss->GetSelLevel();
		else if (ips = (IPatchSelect*)target->GetInterface(I_PATCHSELECT))
			level = ips->GetSelLevel();
		else
			throw RuntimeError (GetString(IDS_NO_SELECT_INTERFACE_FOR_OBJECT), arg_list[0]);
		def_select_so_types();
		return GetName(selectsoTypes, elements(selectsoTypes),level);
	}
	else
		throw RuntimeError (GetString(IDS_NO_SELECT_INTERFACE_FOR_OBJECT), arg_list[0]);
	return &undefined;
}

Value*
setSelectionLevel_cf(Value** arg_list, int count)
{
	// setSelectionLevel <maxobject> {#object | #vertex | #edge | #face}
	//                                         
	check_arg_count(setSelectionLevel, 2, count);
	IMeshSelect *ims = NULL;
	ISplineSelect *iss = NULL;
	IPatchSelect *ips = NULL;
	Animatable *target = NULL;
	def_select_so_types();
	int level = GetID(selectsoTypes, elements(selectsoTypes),arg_list[1]);
	if (is_node(arg_list[0]))
      target = Get_Object_Or_XRef_BaseObject(arg_list[0]->to_node()->GetObjectRef());
//	else if (is_modifier(arg_list[0]))
//		target = arg_list[0]->to_modifier();
	else
		target = arg_list[0]->to_reftarg();

   // need to block xref object access
   if (target->GetInterface(IID_XREF_ITEM))
      target = NULL;

	if (target)
	{	
		if (ims = (IMeshSelect*)target->GetInterface(I_MESHSELECT))
			ims->SetSelLevel(level);
		else if (iss = (ISplineSelect*)target->GetInterface(I_SPLINESELECT))
			iss->SetSelLevel(level);
		else if (ips = (IPatchSelect*)target->GetInterface(I_PATCHSELECT))
			ips->SetSelLevel(level);
		else
			throw RuntimeError (GetString(IDS_NO_SELECT_INTERFACE_FOR_OBJECT), arg_list[0]);
	}
	else
		throw RuntimeError (GetString(IDS_NO_SELECT_INTERFACE_FOR_OBJECT), arg_list[0]);
	return &ok;
}

Value*
stricmp_cf(Value** arg_list, int count)
{
	check_arg_count(stricmp, 2, count);
	return Integer::intern(_stricmp(arg_list[0]->to_string(),arg_list[1]->to_string()));
}

Value*
internetCheckConnection_cf(Value** arg_list, int count)
{
	// Internet.CheckConnection url:<string> force:<bool>
   check_arg_count_with_keys(internet.CheckConnection, 0, count);
	Value* tmp;
	TCHAR *url = NULL;
	Value* urlVal = key_arg(url)->eval();
	if (urlVal != &unsupplied)
		url = urlVal->to_string();
	DWORD flags = bool_key_arg(force,tmp,FALSE) ? FLAG_ICC_FORCE_CONNECTION : 0;
	return (InternetCheckConnection(url,flags,0)) ? &true_value : &false_value;
}

Value*
getLanguage_cf(Value** arg_list, int count)
{
	// sysinfo.getLanguage user:true
	check_arg_count_with_keys(getLanguage, 0, count);
	one_typed_value_local(Array* result);
	vl.result = new Array (3);
	TCHAR tmpString[256];
	LANGID wLang;
	Value* tmp;
	BOOL user = bool_key_arg(user,tmp,FALSE);
	if (user)
		wLang = GetUserDefaultLangID();
	else
		wLang = GetSystemDefaultLangID();
	VerLanguageName(wLang,tmpString,256);
	vl.result->append(Integer::intern(PRIMARYLANGID(wLang)));
	vl.result->append(Integer::intern(SUBLANGID(wLang)));
	vl.result->append(new String(tmpString));
	return_value(vl.result);
}

// class Mtl:
Value*
getNumSubMtls_cf(Value** arg_list, int count)
{
	check_arg_count(getNumSubMtls, 1, count);
	Mtl *m = arg_list[0]->to_mtl();
   if (m == NULL) throw ConversionError (arg_list[0], _T("material"));
	return Integer::intern(m->NumSubMtls());
}

Value*
getSubMtl_cf(Value** arg_list, int count)
{
	check_arg_count(getSubMtl, 2, count);
	Mtl *m = arg_list[0]->to_mtl();
   if (m == NULL) throw ConversionError (arg_list[0], _T("material"));
	int which = arg_list[1]->to_int();
	range_check(which, 1, m->NumSubMtls(), GetString(IDS_SUBMTL_INDEX_OUT_OF_RANGE));
	Mtl* sm = m->GetSubMtl(which-1);
	return MAXClass::make_wrapper_for(sm);
}

Value*
setSubMtl_cf(Value** arg_list, int count)
{
	check_arg_count(setSubMtl, 3, count);
	Mtl *m = arg_list[0]->to_mtl();
   if (m == NULL) throw ConversionError (arg_list[0], _T("material"));
	int which = arg_list[1]->to_int();
	range_check(which, 1, m->NumSubMtls(), GetString(IDS_SUBMTL_INDEX_OUT_OF_RANGE));
	Mtl *sm = NULL;
	if (arg_list[2] != &undefined)
	{	sm = arg_list[2]->to_mtl();
		if (sm->TestForLoop(FOREVER,m) != REF_SUCCEED)
			return &ok;
	}
	m->SetSubMtl(which-1,sm);
	return &ok;
}

Value*
getSubMtlSlotName_cf(Value** arg_list, int count)
{
	check_arg_count(getSubMtlSlotName, 2, count);
	Mtl *m = arg_list[0]->to_mtl();
   if (m == NULL) throw ConversionError (arg_list[0], _T("material"));
	int which = arg_list[1]->to_int();
	range_check(which, 1, m->NumSubMtls(), GetString(IDS_SUBMTL_INDEX_OUT_OF_RANGE));
	TSTR s_name = m->GetSubMtlSlotName(which-1);
	return new String(s_name);
}

// class ISubMap:
Value*
getNumSubTexmaps_cf(Value** arg_list, int count)
{
	check_arg_count(getNumSubTexmaps, 1, count);
	MtlBase *m = arg_list[0]->to_mtlbase();
	return Integer::intern(m->NumSubTexmaps());
}

Value*
getSubTexmap_cf(Value** arg_list, int count)
{
	check_arg_count(getSubTexmap, 2, count);
	MtlBase *m = arg_list[0]->to_mtlbase();
	int which = arg_list[1]->to_int();
	range_check(which, 1, m->NumSubTexmaps(), GetString(IDS_SUBTEXMAP_INDEX_OUT_OF_RANGE));
	Texmap* tm = m->GetSubTexmap(which-1);
	return MAXClass::make_wrapper_for(tm);
}

Value*
setSubTexmap_cf(Value** arg_list, int count)
{
	check_arg_count(setSubTexmap, 3, count);
	MtlBase *m = arg_list[0]->to_mtlbase();
	int which = arg_list[1]->to_int();
	range_check(which, 1, m->NumSubTexmaps(), GetString(IDS_SUBTEXMAP_INDEX_OUT_OF_RANGE));
	Texmap *tm = NULL;
	if (arg_list[2] != &undefined)
	{	tm = arg_list[2]->to_texmap();
		if (tm->TestForLoop(FOREVER,m) != REF_SUCCEED)
			return &ok;
	}
	m->SetSubTexmap(which-1,tm);
	return &ok;
}

Value*
getSubTexmapSlotName_cf(Value** arg_list, int count)
{
	check_arg_count(getSubTexmapSlotName, 2, count);
	MtlBase *m = arg_list[0]->to_mtlbase();
	int which = arg_list[1]->to_int();
	range_check(which, 1, m->NumSubTexmaps(), GetString(IDS_SUBTEXMAP_INDEX_OUT_OF_RANGE));
	TSTR s_name = m->GetSubTexmapSlotName(which-1);
	return new String(s_name);
}

// ===============================================
//      hold system exposure
// ===============================================

Value*
theHold_Begin_cf(Value** arg_list, int count)
{
//	check_arg_count("Begin", 2, count);
	theHold.Begin();
	return &ok;
}

Value*
theHold_Suspend_cf(Value** arg_list, int count)
{
//	check_arg_count("Suspend", 0, count);
	theHold.Suspend();
	return &ok;
}

Value*
theHold_IsSuspended_cf(Value** arg_list, int count)
{
//	check_arg_count("IsSuspended", 0, count);
	return ((theHold.IsSuspended()) ? &true_value : &false_value);
}

Value*
theHold_Resume_cf(Value** arg_list, int count)
{
//	check_arg_count("Resume", 0, count);
	theHold.Resume();
	return &ok;
}

Value*
theHold_Holding_cf(Value** arg_list, int count)
{
//	check_arg_count("Holding", 0, count);
	return ((theHold.Holding()) ? &true_value : &false_value);
}

Value*
theHold_Restoring_cf(Value** arg_list, int count)
{
//	check_arg_count_with_keys("Restoring", 0, count);
	int isUndo;
	Value* isUndoValue = key_arg(isUndo);
	Thunk* isUndoThunk = NULL;
	if (isUndoValue != &unsupplied && isUndoValue->_is_thunk()) 
		isUndoThunk = isUndoValue->to_thunk();
	int res = theHold.Restoring(isUndo);
	if (isUndoThunk)
		isUndoThunk->assign(Integer::intern(isUndo));
	return ((res) ? &true_value : &false_value);
}

Value*
theHold_Redoing_cf(Value** arg_list, int count)
{
//	check_arg_count("Redoing", 0, count);
	return ((theHold.Redoing()) ? &true_value : &false_value);
}

Value*
theHold_RestoreOrRedoing_cf(Value** arg_list, int count)
{
//	check_arg_count("RestoreOrRedoing", 0, count);
	return ((theHold.RestoreOrRedoing()) ? &true_value : &false_value);
}

Value*
theHold_DisableUndo_cf(Value** arg_list, int count)
{
//	check_arg_count("DisableUndo", 0, count);
	theHold.DisableUndo();
	return &ok;
}

Value*
theHold_EnableUndo_cf(Value** arg_list, int count)
{
//	check_arg_count("EnableUndo", 0, count);
	theHold.EnableUndo();
	return &ok;
}

Value*
theHold_Restore_cf(Value** arg_list, int count)
{
//	check_arg_count("Restore", 0, count);
	theHold.Restore();
	return &ok;
}

Value*
theHold_Release_cf(Value** arg_list, int count)
{
//	check_arg_count("Release", 0, count); 
	theHold.Release();
	return &ok;
}

		// 3 ways to terminate the Begin()...
Value*
theHold_End_cf(Value** arg_list, int count)
{
//	check_arg_count("End", 0, count);
	theHold.End();
	return &ok;
}

Value*
theHold_Accept_cf(Value** arg_list, int count)
{
	if (count == 1 && (is_string(arg_list[0]) || is_name(arg_list[0])))
		theHold.Accept(arg_list[0]->to_string());
	else
		theHold.Accept(_T(""));
//	check_arg_count("Accept", 1, count);
//	arg_list[0]->to_string();
	return &ok;
}

Value*
theHold_Cancel_cf(Value** arg_list, int count)
{
//	check_arg_count("Cancel", 0, count);
	theHold.Cancel();
	return &ok;
}

//		
// Group several Begin-End lists into a single Super-group.
Value*
theHold_SuperBegin_cf(Value** arg_list, int count)
{
//	check_arg_count("SuperBegin", 0, count);
	theHold.SuperBegin();
	return &ok;
}

// do a SuperAccept regardless of error 
Value*
theHold_SuperAccept_cf(Value** arg_list, int count)
{
	if (count == 1 && (is_string(arg_list[0]) || is_name(arg_list[0])))
		theHold.SuperAccept(arg_list[0]->to_string());
	else
		theHold.SuperAccept(_T(""));
//	check_arg_count("SuperAccept", 1, count);
//	arg_list[0]->to_string();
	return &ok;
}

Value*
theHold_SuperCancel_cf(Value** arg_list, int count)
{
//	check_arg_count("SuperCancel", 0, count);
	theHold.SuperCancel();
	return &ok;
}


// Get the number of times Put() has been called in the current session of MAX
Value*
theHold_GetGlobalPutCount_cf(Value** arg_list, int count)
{
//	check_arg_count("GetGlobalPutCount", 0, count);
	return Integer::intern(theHold.GetGlobalPutCount());
}

// Get the superlevel depth
Value*
theHold_GetSuperLevel_cf(Value** arg_list, int count)
{
	//	check_arg_count("GetSuperLevel", 0, count);
	return Integer::intern(theHold.Execute(HOLD_SUPERLEVEL));
}

/* --------------------- Node reset transform methods --------------------------------- */

#define CLUSTOSM_CLASS_ID			0x25215824

Value*
ResetXForm_cf(Value** arg_list, int count)
{
	// ResetXForm <node>
	check_arg_count(ResetXForm, 1, count);
	INode* node = get_valid_node((MAXNode*)arg_list[0], ResetXForm);
	TimeValue t = MAXScript_time();

	Matrix3 ntm, ptm, rtm(1), piv(1), tm;

	Object*			obj = node->GetObjectRef();

	SimpleMod *mod = (SimpleMod*)MAXScript_interface->CreateInstance(OSM_CLASS_ID,Class_ID(CLUSTOSM_CLASS_ID,0));
	if (mod == NULL)
		return &false_value;

	// Get Parent and Node TMs
	ntm = node->GetNodeTM(t);
	ptm = node->GetParentTM(t);

	// Compute the relative TM
	ntm = ntm * Inverse(ptm);

	// The reset TM only inherits position
	rtm.SetTrans(ntm.GetTrans());

	// Set the node TM to the reset TM		
	tm = rtm*ptm;
	node->SetNodeTM(t, tm);

	// Compute the pivot TM
	piv.SetTrans(node->GetObjOffsetPos());
	PreRotateMatrix(piv,node->GetObjOffsetRot());
	ApplyScaling(piv,node->GetObjOffsetScale());

	// Reset the offset to 0
	node->SetObjOffsetPos(Point3(0,0,0));
	node->SetObjOffsetRot(IdentQuat());
	node->SetObjOffsetScale(ScaleValue(Point3(1,1,1)));

	// Take the position out of the matrix since we don't reset position
	ntm.NoTrans();

	// Apply the offset to the TM
	ntm = piv * ntm;

	// Apply a derived object to the node's object
	IDerivedObject *dobj = CreateDerivedObject(obj);

	// Apply the transformation to the mod.
	SetXFormPacket pckt(ntm);
	mod->tmControl->SetValue(t,&pckt);

	BOOL was_editing = MAXScript_interface->GetCommandPanelTaskMode() == TASK_MODE_MODIFY &&
		MAXScript_interface->GetSelNodeCount() > 0 ; // && node->Selected();

	// clear any problem command modes
	MAXScript_interface->StopCreating();
	MAXScript_interface->ClearPickMode();
	if (was_editing)
		MAXScript_interface7->SuspendEditing();

	// Add the bend modifier to the derived object.
	dobj->AddModifier(mod);

	// Replace the node's object
	node->SetObjectRef(dobj);

	needs_redraw_set();
	SetSaveRequiredFlag(TRUE);

	MAXScript_interface7->InvalidateObCache(node);

	dobj->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	node->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);
	node->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);

	if (was_editing)
		MAXScript_interface7->ResumeEditing();

	return &true_value;
}

Value*
SuspendEditing_cf(Value** arg_list, int count)
{
	// SuspendEditing ()
	check_arg_count_with_keys(SuspendEditing, 0, count);
	def_cpanel_types();	
	DWORD which = GetID(cpanelTypes, elements(cpanelTypes), key_arg(which),0x00FF);
	if (which != 0x00FF) which = (1<<which);
	Value* tmp;
	BOOL alwaysSuspend = bool_key_arg(alwaysSuspend, tmp, TRUE); 
	MAXScript_interface7->SuspendEditing(which, alwaysSuspend);
	return &ok;
}

Value*
ResumeEditing_cf(Value** arg_list, int count)
{
	// ResumeEditing ()
	check_arg_count_with_keys(ResumeEditing, 0, count);
	def_cpanel_types();	
	DWORD which = GetID(cpanelTypes, elements(cpanelTypes), key_arg(which),0x00FF);
	if (which != 0x00FF) which = (1<<which);
	Value* tmp;
	BOOL alwaysSuspend = bool_key_arg(alwaysSuspend, tmp, true); 
	MAXScript_interface7->ResumeEditing(which, alwaysSuspend);
	return &ok;
}


Value*
CenterPivot_cf(Value** arg_list, int count)
{
	// CenterPivot <node> 
	check_arg_count(CenterPivot, 1, count);
	INode *node = get_valid_node((MAXNode*)arg_list[0], CenterPivot);
	// Disable animation for this
	if (!(node->IsGroupMember() && !node->IsOpenGroupMember())) {
		AnimateSuspend as; // LAM - 6/27/03 - defect 505170
		node->CenterPivot(MAXScript_time(),FALSE);			
		needs_redraw_set();
		return &true_value;
	}
	return &false_value;
}

Value*
CenterObject_cf(Value** arg_list, int count)
{
	// CenterObject <node> 
	check_arg_count(CenterObject, 1, count);
	INode *node = get_valid_node((MAXNode*)arg_list[0], CenterObject);
	// Disable animation for this
	if (!(node->IsGroupMember() && !node->IsOpenGroupMember())) {
		AnimateSuspend as; // LAM - 6/27/03 - defect 505170
		node->CenterPivot(MAXScript_time(),TRUE);			
		needs_redraw_set();
		return &true_value;
	}
	return &false_value;
}

Value*
WorldAlignPivot_cf(Value** arg_list, int count)
{
	// WorldAlignPivot <node>
	check_arg_count(WorldAlignPivot, 1, count);
	INode *node = get_valid_node((MAXNode*)arg_list[0], WorldAlignPivot);
	// Disable animation for this
	if (!(node->IsGroupMember() && !node->IsOpenGroupMember())) {
		AnimateSuspend as; // LAM - 6/27/03 - defect 505170
		node->WorldAlignPivot(MAXScript_time(),FALSE);			
		needs_redraw_set();
		return &true_value;
	}
	return &false_value;
}

Value*
WorldAlignObject_cf(Value** arg_list, int count)
{
	// WorldAlignObject <node>
	check_arg_count(WorldAlignObject, 1, count);
	INode *node = get_valid_node((MAXNode*)arg_list[0], WorldAlignObject);
	// Disable animation for this
	if (!(node->IsGroupMember() && !node->IsOpenGroupMember())) {
		AnimateSuspend as; // LAM - 6/27/03 - defect 505170
		node->WorldAlignPivot(MAXScript_time(),TRUE);			
		needs_redraw_set();
		return &true_value;
	}
	return &false_value;
}

Value*
AlignPivot_cf(Value** arg_list, int count)
{
	// AlignPivot <node>
	check_arg_count(AlignPivot, 1, count);
	INode *node = get_valid_node((MAXNode*)arg_list[0], AlignPivot);
	// Disable animation for this
	if (!(node->IsGroupMember() && !node->IsOpenGroupMember())) {
		AnimateSuspend as; // LAM - 6/27/03 - defect 505170
		node->AlignPivot(MAXScript_time(),FALSE);			
		needs_redraw_set();
		return &true_value;
	}
	return &false_value;
}

Value*
AlignObject_cf(Value** arg_list, int count)
{
	// AlignObject <node>
	check_arg_count(AlignObject, 1, count);
	INode *node = get_valid_node((MAXNode*)arg_list[0], AlignObject);
	// Disable animation for this
	if (!(node->IsGroupMember() && !node->IsOpenGroupMember())) {
		AnimateSuspend as; // LAM - 6/27/03 - defect 505170
		node->AlignPivot(MAXScript_time(),TRUE);			
		needs_redraw_set();
		return &true_value;
	}
	return &false_value;
}

Value*
AlignToParent_cf(Value** arg_list, int count)
{
	// AlignToParent <node> 
	check_arg_count(AlignToParent, 1, count);
	INode *node = get_valid_node((MAXNode*)arg_list[0], AlignToParent);
	if (!(node->IsGroupMember() && !node->IsOpenGroupMember())) {
		node->AlignToParent(MAXScript_time());			
		needs_redraw_set();
		return &true_value;
	}
	return &false_value;
}

Value*
ResetTransform_cf(Value** arg_list, int count)
{
	// ResetTransform <node>
	check_arg_count(ResetTransform, 1, count);
	INode *node = get_valid_node((MAXNode*)arg_list[0], ResetTransform);
	if (!(node->IsGroupMember() && !node->IsOpenGroupMember())) {
		node->ResetTransform(MAXScript_time(),FALSE);
		needs_redraw_set();
		return &true_value;
	}
	return &false_value;
}

Value*
ResetScale_cf(Value** arg_list, int count)
{
	// ResetScale <node>
	check_arg_count(ResetScale, 1, count);
	INode *node = get_valid_node((MAXNode*)arg_list[0], ResetScale);
	if (!(node->IsGroupMember() && !node->IsOpenGroupMember())) {
		node->ResetTransform(MAXScript_time(),TRUE);
		needs_redraw_set();
		return &true_value;
	}
	return &false_value;
}

Value*
ResetPivot_cf(Value** arg_list, int count)
{
	// ResetPivot <node>
	check_arg_count(ResetPivot, 1, count);
	INode *node = get_valid_node((MAXNode*)arg_list[0], ResetPivot);
	if (!(node->IsGroupMember() && !node->IsOpenGroupMember())) {
		node->ResetPivot(MAXScript_time());
		needs_redraw_set();
		return &true_value;
	}
	return &false_value;
}

static Class_ID  noneClassID(0,0);

Value*
areMtlAndRendererCompatible_cf(Value** arg_list, int count)
{
	// areMtlAndRendererCompatible {<mtlBase*> | <classDesc>} renderer:{<renderer> | <classDesc>}
	check_arg_count_with_keys(areMtlAndRendererCompatible, 1, count);
	ClassDesc* mtlClassDesc = NULL;
	ClassDesc* rendererClassDesc = NULL;
	ClassDirectory& cdir = MAXScript_interface->GetDllDir().ClassDir();
	Value* arg0 = arg_list[0];
	if (arg0->is_kind_of(class_tag(MAXClass)))
	{	MAXClass* mc = (MAXClass*)arg0;
		ClassEntry* ce = cdir.FindClassEntry(mc->sclass_id, mc->class_id);
		if (ce) mtlClassDesc = ce->FullCD();
	}
	else
	{
		MtlBase *r = arg0->to_mtlbase();
		mtlClassDesc = cdir.FindClass(r->SuperClassID(), r->ClassID());
	}
	
	Value* arg1 = key_arg(renderer);
	if (arg1 == &unsupplied)
	{
		Renderer* currentRenderer = MAXScript_interface->GetCurrentRenderer();
		if(currentRenderer != NULL)
			rendererClassDesc = cdir.FindClass(currentRenderer->SuperClassID(), currentRenderer->ClassID());
	}
	else if (arg1->is_kind_of(class_tag(MAXClass)))
	{	MAXClass* mc = (MAXClass*)arg1;
		ClassEntry* ce = cdir.FindClassEntry(mc->sclass_id, mc->class_id);
		if (ce) rendererClassDesc = ce->FullCD();
	}
	else
	{
		Renderer *r = arg1->to_renderer();
		rendererClassDesc = cdir.FindClass(r->SuperClassID(), r->ClassID());
	}
	
	bool incompatible = ((mtlClassDesc != NULL) && (rendererClassDesc != NULL) && 
						(mtlClassDesc->ClassID() != noneClassID) && 
						(rendererClassDesc->ClassID() != noneClassID) && 
						!AreMtlAndRendererCompatible(*mtlClassDesc, *rendererClassDesc));

	return incompatible ? &false_value : &true_value;
}

Value*
getHideByCategory_geometry()
{	
	DWORD f = GetCOREInterface()->GetHideByCategoryFlags();
	return (f & HIDE_OBJECTS) ? &true_value : &false_value;
}
Value*
setHideByCategory_geometry(Value* val)
{
	DWORD f = HIDE_OBJECTS;
	DWORD f1 = GetCOREInterface()->GetHideByCategoryFlags();
	bool isHidden = (f1 & f) == 0; // currently hidden if != 0
	bool hide = val->to_bool() == 0;
	if (isHidden != hide)
		GetCOREInterface()->ExecuteMAXCommand(MAXCOM_HIDE_OBJECT_TOGGLE );
	return val;
}

Value*
getHideByCategory_shapes()
{	
	DWORD f = GetCOREInterface()->GetHideByCategoryFlags();
	return (f & HIDE_SHAPES) ? &true_value : &false_value;
}
Value*
setHideByCategory_shapes(Value* val)
{
	DWORD f = HIDE_SHAPES;
	DWORD f1 = GetCOREInterface()->GetHideByCategoryFlags();
	bool isHidden = (f1 & f) == 0; // currently hidden if != 0
	bool hide = val->to_bool() == 0;
	if (isHidden != hide)
		GetCOREInterface()->ExecuteMAXCommand(MAXCOM_HIDE_SHAPE_TOGGLE);
	return val;
}

Value*
getHideByCategory_lights()
{	
	DWORD f = GetCOREInterface()->GetHideByCategoryFlags();
	return (f & HIDE_LIGHTS) ? &true_value : &false_value;
}
Value*
setHideByCategory_lights(Value* val)
{
	DWORD f = HIDE_LIGHTS;
	DWORD f1 = GetCOREInterface()->GetHideByCategoryFlags();
	bool isHidden = (f1 & f) == 0; // currently hidden if != 0
	bool hide = val->to_bool() == 0;
	if (isHidden != hide)
		GetCOREInterface()->ExecuteMAXCommand(MAXCOM_HIDE_LIGHT_TOGGLE);
	return val;
}

Value*
getHideByCategory_cameras()
{	
	DWORD f = GetCOREInterface()->GetHideByCategoryFlags();
	return (f & HIDE_CAMERAS) ? &true_value : &false_value;
}
Value*
setHideByCategory_cameras(Value* val)
{
	DWORD f = HIDE_CAMERAS;
	DWORD f1 = GetCOREInterface()->GetHideByCategoryFlags();
	bool isHidden = (f1 & f) == 0; // currently hidden if != 0
	bool hide = val->to_bool() == 0;
	if (isHidden != hide)
		GetCOREInterface()->ExecuteMAXCommand(MAXCOM_HIDE_CAMERA_TOGGLE);
	return val;
}

Value*
getHideByCategory_helpers()
{	
	DWORD f = GetCOREInterface()->GetHideByCategoryFlags();
	return (f & HIDE_HELPERS) ? &true_value : &false_value;
}
Value*
setHideByCategory_helpers(Value* val)
{
	DWORD f = HIDE_HELPERS;
	DWORD f1 = GetCOREInterface()->GetHideByCategoryFlags();
	bool isHidden = (f1 & f) == 0; // currently hidden if != 0
	bool hide = val->to_bool() == 0;
	if (isHidden != hide)
		GetCOREInterface()->ExecuteMAXCommand(MAXCOM_HIDE_HELPER_TOGGLE);
	return val;
}

Value*
getHideByCategory_spacewarps()
{	
	DWORD f = GetCOREInterface()->GetHideByCategoryFlags();
	return (f & HIDE_WSMS) ? &true_value : &false_value;
}
Value*
setHideByCategory_spacewarps(Value* val)
{
	DWORD f = HIDE_WSMS;
	DWORD f1 = GetCOREInterface()->GetHideByCategoryFlags();
	bool isHidden = (f1 & f) == 0; // currently hidden if != 0
	bool hide = val->to_bool() == 0;
	if (isHidden != hide)
		GetCOREInterface()->ExecuteMAXCommand(MAXCOM_HIDE_WSM_TOGGLE);
	return val;
}

Value*
getHideByCategory_particles()
{	
	DWORD f = GetCOREInterface()->GetHideByCategoryFlags();
	return (f & HIDE_PARTICLES) ? &true_value : &false_value;
}
Value*
setHideByCategory_particles(Value* val)
{
	DWORD f = HIDE_PARTICLES;
	DWORD f1 = GetCOREInterface()->GetHideByCategoryFlags();
	bool isHidden = (f1 & f) == 0; // currently hidden if != 0
	bool hide = val->to_bool() == 0;
	if (isHidden != hide)
		GetCOREInterface()->ExecuteMAXCommand(MAXCOM_HIDE_SYSTEM_TOGGLE);
	return val;
}

Value*
getHideByCategory_bones()
{	
	DWORD f = GetCOREInterface()->GetHideByCategoryFlags();
	return (f & HIDE_BONEOBJECTS) ? &true_value : &false_value;
}
Value*
setHideByCategory_bones(Value* val)
{
	DWORD f = HIDE_BONEOBJECTS;
	DWORD f1 = GetCOREInterface()->GetHideByCategoryFlags();
	((val->to_bool())) ? f1 |= f : f1 &= ~f;
	GetCOREInterface()->SetHideByCategoryFlags(f1);
	GetCOREInterface()->ExecuteMAXCommand(MAXCOM_TOOL_EDGES_ONLY_TOGGLE);
	GetCOREInterface()->ExecuteMAXCommand(MAXCOM_TOOL_EDGES_ONLY_TOGGLE);
	return val;
}

Value* 
HideByCategory_all_cf(Value** arg_list, int count)
{
	//	hideByCategory.all() 
	check_arg_count(hideByCategory.all, 0, count);
	setHideByCategory_geometry(&true_value);
	setHideByCategory_shapes(&true_value);
	setHideByCategory_lights(&true_value);
	setHideByCategory_cameras(&true_value);
	setHideByCategory_bones(&true_value);
	setHideByCategory_helpers(&true_value);
	setHideByCategory_spacewarps(&true_value);
	setHideByCategory_particles(&true_value);
	return &ok;
}

Value* 
HideByCategory_none_cf(Value** arg_list, int count)
{
	//	hideByCategory.none() 
	check_arg_count(hideByCategory.none, 0, count);
	setHideByCategory_geometry(&false_value);
	setHideByCategory_shapes(&false_value);
	setHideByCategory_lights(&false_value);
	setHideByCategory_cameras(&false_value);
	setHideByCategory_bones(&false_value);
	setHideByCategory_helpers(&false_value);
	setHideByCategory_spacewarps(&false_value);
	setHideByCategory_particles(&false_value);
	return &ok;
}

Value* 
GetQuietMode_cf(Value** arg_list, int count)
{
	check_arg_count(GetQuietMode, 0, count);
	return GetCOREInterface()->GetQuietMode() ? &true_value : &false_value;	
}

Value* 
SetQuietMode_cf(Value** arg_list, int count)
{
	check_arg_count(SetQuietMode, 1, count);
	return GetCOREInterface()->SetQuietMode(arg_list[0]->to_bool()) ? &true_value : &false_value;
}

Value* 
GetMAXIniFile_cf(Value** arg_list, int count)
{
	check_arg_count(GetMAXIniFile, 0, count);
	return new String(GetCOREInterface7()->GetMAXIniFile());
}

/* --------------------- Dependency Test --------------------------------- */

Value*
DependencyLoopTest_cf(Value** arg_list, int count)
{
	// DependencyLoopTest <RefTarg_source> <RefTarg_target>
	// returns true if RefTarg1 is dependent on RefTarg2
	check_arg_count(DependencyLoopTest, 2, count);
	ReferenceTarget *source = (is_node(arg_list[0])) ? (ReferenceTarget*)((MAXNode*)arg_list[0])->node : arg_list[0]->to_reftarg();
	ReferenceTarget *target = (is_node(arg_list[1])) ? (ReferenceTarget*)((MAXNode*)arg_list[1])->node : arg_list[1]->to_reftarg();
	if (source == NULL || target == NULL) return &false_value;
	deletion_check((MAXWrapper*)arg_list[0]);
	deletion_check((MAXWrapper*)arg_list[1]);
	return (source->TestForLoop(FOREVER,target)==REF_FAIL) ? &true_value : &false_value;
}

//-----------------------------------------------------------

class GetInstancesProc : public AnimEnum
{
public:
	MAXClass* mc;
	Array* result;
	BOOL asTrackViewPick;
	GetInstancesProc(MAXClass* mc, Array* result, BOOL asTrackViewPick) : AnimEnum(SCOPE_ALL)
	{ 
		this->mc = mc;
		this->result = result;
		this->asTrackViewPick = asTrackViewPick;
	}
	int proc(Animatable *anim, Animatable *client, int subNum)
	{
		Class_ID cid = anim->ClassID();
		SClass_ID sid = anim->SuperClassID();
		if (sid == REF_MAKER_CLASS_ID && cid == CUSTATTRIB_CONTAINER_CLASS_ID)
		{
			Animatable *amaker = ((ICustAttribContainer*)anim)->GetOwner();
			if (amaker == NULL)
				return ANIM_ENUM_PROCEED;
			anim = (ReferenceMaker*)amaker->GetInterface(REFERENCE_MAKER_INTERFACE);
			if (anim == NULL)
				return ANIM_ENUM_PROCEED;
			cid = anim->ClassID();
			sid = anim->SuperClassID();
		}

		if (!asTrackViewPick && anim->TestAFlag(A_WORK3))
			return ANIM_ENUM_STOP; // already been down this path....

		MAXClass* cls = lookup_MAXClass(&cid, sid);
		MSCustAttrib* msCA;
		if (cls == mc)
		{
			Value *item;
			if (cls == &inode_object)
				item = MAXNode::intern((INode*)anim);
			else if (cls->superclass->superclass == &node_class)
				item = MAXObject::intern((Object*)anim);
			else if ((msCA = (MSCustAttrib*)anim->GetInterface(I_SCRIPTEDCUSTATTRIB)) != NULL) // handle scripted CAs 
				item = MAXCustAttrib::intern(msCA);
			else
				item = (*cls->superclass->maker)(cls, (ReferenceTarget*)anim, NULL, 0);
			if (item != &undefined) // only add if unique.
			{	
				if (asTrackViewPick)	// return all routes to the object
				{	TrackViewPick tvp;
				tvp.anim = (ReferenceTarget*)anim;
				tvp.client = (ReferenceTarget*)client;
				tvp.subNum = subNum;
				result->append(new TrackViewPickValue(tvp));
				}
				if (anim->TestAFlag(A_WORK3))
					return ANIM_ENUM_STOP; // already been down this path....
				anim->SetAFlag(A_WORK3);
				if (!asTrackViewPick)	// return only a single instance of the object
					result->append(item);
			}
		}
		else
		{	if (anim->TestAFlag(A_WORK3))
		return ANIM_ENUM_STOP; // already been down this path....
		}

		return ANIM_ENUM_PROCEED;
	}
};

Value*
getClassInstances_cf(Value** arg_list, int count)
{
	// getClassInstances <MAXClass> target:<MAXObject> asTrackViewPick:<bool>
	check_arg_count_with_keys(getClassInstances, 1, count);
	MAXClass *mc = (MAXClass*)arg_list[0];
	if (!mc->is_kind_of(class_tag(MAXClass)))
		type_check(mc, MAXClass, _T("getClassInstances"));
	Value* tmp;
	BOOL asTrackViewPick = bool_key_arg(asTrackViewPick, tmp, FALSE); 

	one_typed_value_local(Array* result);
	vl.result = new Array (0);

	// to get a handle to the scene, or use specified target
	ReferenceMaker* target = NULL;

	bool quickClear = true;
	Value* targetVal = key_arg(target)->eval();
	if (targetVal != &unsupplied)
	{
		target = targetVal->to_reftarg();
		quickClear = false;
	}
	else
		target = MAXScript_interface->GetScenePointer();

	if (target)
	{
		if (!quickClear)
		{
			ClearAnimFlagEnumProc clearProc(A_WORK3);
			clearProc.SetScope(SCOPE_ALL);
			target->EnumAnimTree(&clearProc,NULL,0);		
		}
		else
			ClearAFlagInAllAnimatables(A_WORK3);
		GetInstancesProc gip (mc, vl.result, asTrackViewPick);
		target->EnumAnimTree(&gip,NULL,0);
	}

	return_value(vl.result);
}

Value*
isMSPluginClass_cf(Value** arg_list, int count)
{
	check_arg_count(isMSPluginClass, 1, count);
	return bool_result(arg_list[0]->is_kind_of(class_tag(MSPluginClass)));
}

Value*
isMSCustAttribClass_cf(Value** arg_list, int count)
{
	check_arg_count(isMSCustAttribClass, 1, count);
	return bool_result(arg_list[0]->is_kind_of(class_tag(MSCustAttribDef)));
}

Value*
isMSPlugin_cf(Value** arg_list, int count)
{
	check_arg_count(isMSPlugin, 1, count);
	return ( arg_list[0]->is_kind_of(class_tag(MAXWrapper)) && 
			 (((MAXWrapper*)arg_list[0])->get_max_object()->GetInterface(IID_XREF_ITEM) == NULL) &&
		     ((MAXWrapper*)arg_list[0])->get_max_object()->GetInterface(I_MAXSCRIPTPLUGIN) ) ? &true_value : &false_value;
}

Value*
isMSCustAttrib_cf(Value** arg_list, int count)
{
	check_arg_count(isMSCustAttrib, 1, count);
	return (arg_list[0]->is_kind_of(class_tag(MAXWrapper)) && ((MAXWrapper*)arg_list[0])->get_max_object()->GetInterface(I_SCRIPTEDCUSTATTRIB)) ? &true_value : &false_value;
}

Value*
getClassName_cf(Value** arg_list, int count)
{
	check_arg_count(getClassName, 1, count);
	TSTR name;
	ReferenceTarget* ref = arg_list[0]->to_reftarg();
	if (ref)
		ref->GetClassName(name);
	return new String (name);
}

Value*
isMaxFile_cf(Value** arg_list, int count)
{
	check_arg_count(isMaxFile, 1, count);
	return bool_result(MAXScript_interface->IsMaxFile(arg_list[0]->to_filename()));
}

Value*
is64bitApplication_cf(Value** arg_list, int count)
{
	check_arg_count(is64bitApplication, 0, count);
#ifdef WIN64
	return &true_value;
#else
	return &false_value;
#endif
}

//  =================================================
//  Snaps 
//  =================================================

#include "omanapi.h"
#include "osnap.h"
#include "osnaphit.h"

Value*
setSnapHilite(Value* val)
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->sethilite(val->to_point3());
	return val;
}

Value*
getSnapHilite()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	Point3 p = theman->gethilite();
	return ColorValue::intern(p.x, p.y, p.z);
}

Value*
setSnapMarkSize(Value* val)
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->setmarksize(val->to_int());
	return val;
}

Value*
getSnapMarkSize()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	return Integer::intern(theman->getmarksize());
}

Value*
setSnapToFrozen(Value* val)
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->SetSnapToFrozen(val->to_bool()==TRUE);
	return val;
}

Value*
getSnapToFrozen()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	return (theman->SnapToFrozen()) ? &true_value : &false_value;
}

Value*
setSnapAxisConstraint(Value* val)
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	if (val->to_bool())
		theman->SetAFlag(XYZ_CONSTRAINT);
	else
		theman->ClearAFlag(XYZ_CONSTRAINT);
	return val;
}

Value*
getSnapAxisConstraint()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	return (theman->TestAFlag(XYZ_CONSTRAINT)) ? &true_value : &false_value;
}

Value*
setSnapDisplay(Value* val)
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	if (val->to_bool())
		theman->SetAFlag(HIGHLIGHT_POINT | HIGHLIGHT_GEOM);
	else
		theman->ClearAFlag(HIGHLIGHT_POINT | HIGHLIGHT_GEOM);
	return val;
}

Value*
getSnapDisplay()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	return (theman->TestAFlag(HIGHLIGHT_POINT | HIGHLIGHT_GEOM)) ? &true_value : &false_value;
}

Value*
setSnapStrength(Value* val)
{	
	throw RuntimeError (_T("Cannot set snap strength. Use snap preview radius instead."));
}

Value*
getSnapStrength()
{
	IOsnapManager* theman = (IOsnapManager*)GetCOREInterface()->GetOsnapManager();
	return Integer::intern(theman->GetSnapStrength());
}

Value*
setDisplaySnapRubberBand(Value* val)
{	
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->SetDisplaySnapRubberBand(val->to_bool()==TRUE);
	return val;
}

Value*
getDisplaySnapRubberBand()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	return (theman->GetDisplaySnapRubberBand()) ? &true_value : &false_value;
}

Value*
setUseAxisCenterAsStartSnapPoint(Value* val)
{	
	IOsnapManager8* theman = IOsnapManager8::GetInstance();
	theman->SetTMAxisCenterAsDefaultStartSnapPoint(val->to_bool()==TRUE);
	return val;
}

Value*
getUseAxisCenterAsStartSnapPoint()
{
	IOsnapManager8* theman = IOsnapManager8::GetInstance();
	return (theman->IsTMAxisCenterTheDefaultStartSnapPoint()) ? &true_value : &false_value;
}


Value*
setSnapHit(Value* val)
{	
	throw RuntimeError (_T("Cannot set snap hit"));
}

Value*
getSnapHit()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	return (theman->TestAFlag(BUFFER_HOLDING)) ? &true_value : &false_value;
}

Value*
getSnapNode()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	if (!theman->getactive()) return &undefined;
	INode* hit = NULL;
	OsnapHit& theHit = theman->GetHit();
	if (&theHit) hit = theHit.GetNode();
	if (hit)
		return MAXNode::intern(hit);
	else
		return &undefined;
}

Value*
setSnapNode(Value* val)
{
	throw RuntimeError (_T("Cannot set snap node"));
}

Value*
getSnapFlags()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	if (!theman->getactive()) return &undefined;
	return Integer::intern(theman->GetSnapFlags());
}

Value*
setSnapFlags(Value* val)
{
	throw RuntimeError (_T("Cannot set snap Flags"));
}

Value*
getHitPoint()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	if (!theman->getactive()) return &undefined;
	OsnapHit& theHit = theman->GetHit();
	INode* hit = NULL;
	if (&theHit) hit = theHit.GetNode();
	if (hit)
		return new Point3Value (theHit.GetHitpoint());
	else
		return &undefined;
}

Value*
setHitPoint(Value* val)
{
	throw RuntimeError (_T("Cannot set snap hitPoint"));
}

Value*
getWorldHitpoint()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	if (!theman->getactive()) return &undefined;
	OsnapHit& theHit = theman->GetHit();
	INode* hit = NULL;
	if (&theHit) hit = theHit.GetNode();
	if (hit)
		return new Point3Value (theHit.GetWorldHitpoint());
	else
		return &undefined;
}

Value*
setWorldHitpoint(Value* val)
{
	throw RuntimeError (_T("Cannot set snap worldHitPoint"));
}

Value*
getScreenHitpoint()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	if (!theman->getactive()) return &undefined;
	OsnapHit& theHit = theman->GetHit();
	INode* hit = NULL;
	if (&theHit) hit = theHit.GetNode();
	if (hit) {
		IPoint3 hitPoint = theHit.GetHitscreen();
		return new Point3Value (hitPoint.x, hitPoint.y, hitPoint.z);
	}
	else
		return &undefined;
}

Value*
setScreenHitpoint(Value* val)
{
	throw RuntimeError (_T("Cannot set snap screenHitPoint"));
}

Value*
getSnapOKForRelativeSnap()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	if (!theman->getactive()) return &undefined;
	return (theman->OKForRelativeSnap()) ? &true_value : &false_value;
}

Value*
setSnapOKForRelativeSnap(Value* val)
{
	throw RuntimeError (_T("Cannot set OKForRelativeSnap"));
}

Value*
getSnapRefPoint()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	if (!theman->getactive() || !theman->OKForRelativeSnap()) return &undefined;
	return new Point3Value (theman->GetRefPoint(FALSE));
}

Value*
setSnapRefPoint(Value* val)
{
	throw RuntimeError (_T("Cannot set snap refPoint"));
}

Value*
getSnapTopRefPoint()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	if (!theman->getactive() || !theman->OKForRelativeSnap()) return &undefined;
	return new Point3Value (theman->GetRefPoint(TRUE));
}

Value*
setSnapTopRefPoint(Value* val)
{
	throw RuntimeError (_T("Cannot set snap topRefPoint"));
}

Value*
getNumOSnaps()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->Enumerate(RESET);//start at the beginning of the table
	Osnap* thesnap;
	int nsnaps=0;
	while(thesnap = theman->Enumerate(NEXT))
		nsnaps++;
	return Integer::intern(nsnaps);
}

Value*
setNumOSnaps(Value* val)
{
	throw RuntimeError (_T("Cannot set snap NumOSnaps"));
}


Value*
getSnapRadius()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	return Integer::intern(theman->GetSnapRadius());
}

Value*
setSnapRadius(Value* val)
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->SetSnapRadius(val->to_int());
	return val;
}

Value*
setSnapPreviewRadius(Value* val)
{	
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->SetSnapPreviewRadius(val->to_int());
	return val;
}

Value*
getSnapPreviewRadius()
{
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	return Integer::intern(theman->GetSnapPreviewRadius());
}




Value*
snap_getOSnapName_cf(Value** arg_list, int count)
{
	// <string> getOSnapName <int osnap_index>
	check_arg_count(getOSnapName, 1, count);
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->Enumerate(RESET);//start at the beginning of the table
	Osnap* thesnap;
	int nsnaps=0;
	int which = arg_list[0]->to_int();
	while(thesnap = theman->Enumerate(NEXT)) {
		nsnaps++;
		if (nsnaps == which) {
			return new String (thesnap->Category());
		}
	}
	range_check(which, 1, nsnaps, _T("Object snap index out of range: "));
	return &undefined;
}

Value*
snap_getOSnapNumItems_cf(Value** arg_list, int count)
{
	// <string> getOSnapNumItems <int osnap_index>
	check_arg_count(getOSnapNumItems, 1, count);
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->Enumerate(RESET);//start at the beginning of the table
	Osnap* thesnap;
	int nsnaps=0;
	int which = arg_list[0]->to_int();
	while(thesnap = theman->Enumerate(NEXT)) {
		nsnaps++;
		if (nsnaps == which) {
			return Integer::intern(thesnap->numsubs());
		}
	}
	range_check(which, 1, nsnaps, _T("Object snap index out of range: "));
	return &undefined;
}

Value*
snap_getOSnapItemName_cf(Value** arg_list, int count)
{
	// <string> getOSnapItemName <int osnap_index> <int osnap_item_index>
	check_arg_count(getOSnapItemName, 2, count);
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->Enumerate(RESET);//start at the beginning of the table
	Osnap* thesnap;
	int nsnaps=0;
	int which = arg_list[0]->to_int();
	int item = arg_list[1]->to_int();
	while(thesnap = theman->Enumerate(NEXT)) {
		nsnaps++;
		if (nsnaps == which) {
			int nitems = thesnap->numsubs();
			range_check(item, 1, nitems, _T("Object snap item index out of range: "));
			TSTR* snapname = thesnap->snapname(item-1);
			return new String ((snapname) ? snapname->data() : _T(""));
		}
	}
	range_check(which, 1, nsnaps, _T("Object snap index out of range: "));
	return &undefined;
}

Value*
snap_getOSnapItemToolTip_cf(Value** arg_list, int count)
{
	// <string> getOSnapItemToolTip <int osnap_index> <int osnap_item_index>
	check_arg_count(getOSnapItemToolTip, 2, count);
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->Enumerate(RESET);//start at the beginning of the table
	Osnap* thesnap;
	int nsnaps=0;
	int which = arg_list[0]->to_int();
	int item = arg_list[1]->to_int();
	while(thesnap = theman->Enumerate(NEXT)) {
		nsnaps++;
		if (nsnaps == which) {
			int nitems = thesnap->numsubs();
			range_check(item, 1, nitems, _T("Object snap item index out of range: "));
			TSTR* tooltip = thesnap->tooltip(item-1);
			return new String ((tooltip) ? tooltip->data() : _T(""));
		}
	}
	range_check(which, 1, nsnaps, _T("Object snap index out of range: "));
	return &undefined;
}

class MXS_IOsnap
{
public:
	Osnap* osnap;
	MXS_IOsnap(Osnap* osnap){this->osnap=osnap;}
	~MXS_IOsnap() {};
	BOOL GetActive(int index){return osnap->GetActive(index);}
	void SetActive(int index, BOOL state){osnap->SetActive(index, state);}

};

Value*
snap_getOSnapItemActive_cf(Value** arg_list, int count)
{
	// <string> getOSnapItemActive <int osnap_index> <int osnap_item_index>
	check_arg_count(getOSnapItemActive, 2, count);
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->Enumerate(RESET);//start at the beginning of the table
	Osnap* thesnap;
	int nsnaps=0;
	int which = arg_list[0]->to_int();
	int item = arg_list[1]->to_int();
	while(thesnap = theman->Enumerate(NEXT)) {
		nsnaps++;
		if (nsnaps == which) {
			int nitems = thesnap->numsubs();
			range_check(item, 1, nitems, _T("Object snap item index out of range: "));
			MXS_IOsnap osnap (thesnap);
			return (osnap.GetActive(item-1)) ? &true_value : &false_value;
		}
	}
	range_check(which, 1, nsnaps, _T("Object snap index out of range: "));
	return &undefined;
}

Value*
snap_setOSnapItemActive_cf(Value** arg_list, int count)
{
	// <string> setOSnapItemActive <int osnap_index> <int osnap_item_index>
	check_arg_count(setOSnapItemActive, 3, count);
	IOsnapManager7* theman = IOsnapManager7::GetInstance();
	theman->Enumerate(RESET);//start at the beginning of the table
	Osnap* thesnap;
	int nsnaps=0;
	int which = arg_list[0]->to_int();
	int item = arg_list[1]->to_int();
	while(thesnap = theman->Enumerate(NEXT)) {
		nsnaps++;
		if (nsnaps == which) {
			int nitems = thesnap->numsubs();
			range_check(item, 1, nitems, _T("Object snap item index out of range: "));
			MXS_IOsnap osnap (thesnap);
			osnap.SetActive(item-1,arg_list[2]->to_bool());
			return &ok;
		}
	}
	range_check(which, 1, nsnaps, _T("Object snap index out of range: "));
	return &undefined;
}

#include "gencam.h"

#define FOV_W 0	  // width-related FOV
#define FOV_H 1   // height-related FOV
#define FOV_D 2   // diagonal-related FOV

static float GetAspect() {
	return GetCOREInterface()->GetRendImageAspect();
};

static float GetApertureWidth() {
	return GetCOREInterface()->GetRendApertureWidth();
}

Value*
cameraFOV_MMtoFOV_cf(Value** arg_list, int count)
{
	// <float> MMtoFOV <float mm>
	check_arg_count(MMtoFOV, 1, count);
	float mm = arg_list[0]->to_float();
	float fov = float(2.0f*atan(0.5f*GetApertureWidth()/mm));
	return Float::intern(RadToDeg(fov));
}

Value*
cameraFOV_FOVtoMM_cf(Value** arg_list, int count)
{
	// <float> FOVtoMM <float fov>
	check_arg_count(FOVtoMM, 1, count);
	float fov = DegToRad(arg_list[0]->to_float());
	float mm = float((0.5f*GetApertureWidth())/tan(fov/2.0f));
	return Float::intern(mm);
}

Value*
cameraFOV_CurFOVtoFOV_cf(Value** arg_list, int count)
{
	// <float> CurFOVtoWFOV <camera> <float cfov>
	check_arg_count(CurFOVtoWFOV, 2, count);
	INode* node = get_valid_node(arg_list[0], CurFOVtoWFOV);
   Object* obj = Get_Object_Or_XRef_BaseObject(node->GetObjectRef());
	Class_ID cid = obj->ClassID();
	SClass_ID sid = obj->SuperClassID();
	GenCamera *cam;
	if (sid == CAMERA_CLASS_ID && (cid == Class_ID(LOOKAT_CAM_CLASS_ID, 0) || cid == Class_ID(SIMPLE_CAM_CLASS_ID, 0)))
		cam = (GenCamera*)obj;
	else
		throw RuntimeError (GetString(IDS_NEED_CAMERA), arg_list[0]);
	int fovType = cam->GetFOVType();
	float cfov = DegToRad(arg_list[1]->to_float());
	float fov;
	switch (fovType) 
	{
		case FOV_H: 
			fov = float(2.0*atan(GetAspect()*tan(cfov/2.0f)));
			break;
		case FOV_D:
			{
				float w = GetApertureWidth();
				float h = w/GetAspect();
				float d = (float)sqrt(w*w + h*h);	
				fov = float(2.0*atan((w/d)*tan(cfov/2.0f)));
				break;
			}
		default:
			fov = cfov;
	}
	return Float::intern(RadToDeg(fov));
}

Value*
cameraFOV_FOVtoCurFOV_cf(Value** arg_list, int count)
{
	// <float> FOVtoCurFOV <camera> <float fov>
	check_arg_count(WFOVtoCurFOV, 2, count);
	INode* node = get_valid_node(arg_list[0], FOVtoCurFOV);
   Object* obj = Get_Object_Or_XRef_BaseObject(node->GetObjectRef());
	Class_ID cid = obj->ClassID();
	SClass_ID sid = obj->SuperClassID();
	GenCamera *cam;
	if (sid == CAMERA_CLASS_ID && (cid == Class_ID(LOOKAT_CAM_CLASS_ID, 0) || cid == Class_ID(SIMPLE_CAM_CLASS_ID, 0)))
		cam = (GenCamera*)obj;
	else
		throw RuntimeError (GetString(IDS_NEED_CAMERA), arg_list[0]);
	int fovType = cam->GetFOVType();
	float fov = DegToRad(arg_list[1]->to_float());
	float cfov;
	switch (fovType) 
	{
		case FOV_H:
			cfov = float(2.0*atan(tan(fov/2.0f)/GetAspect()));
			break;
		case FOV_D: 
		{
			float w = GetApertureWidth();
			float h = w/GetAspect();
			float d = (float)sqrt(w*w + h*h);	
			cfov =  float(2.0*atan((d/w)*tan(fov/2.0f)));
			break;
		}
		default:
			cfov = fov;
	}

	return Float::intern(RadToDeg(cfov));
}

static BOOL MyComparePixels(float p1, float p2, float var)
{
	float dif = fabsf(p1 - p2);
	return (dif > var);
}


Value* CompareBitmaps_cf(Value **arg_list, int count)
{
	// CompareBitmaps <filename>File1 <filename>File2 <int>tolerance(# different pixels) <int>variation(0-255) useAlpha:<boolean> errorMsg:<&string>
	// CompareBitmaps <bitmap>bitmap1 <bitmap>bitmap2 <int>tolerance(# different pixels) <int>variation(0-255) useAlpha:<boolean> errorMsg:<&string>
	check_arg_count_with_keys(CompareBitmaps, 4, count);

	TSTR myStr = _T("");
	bool res = false;

	Bitmap *bm1 = NULL, *bm2 = NULL;
	BitmapInfo *bi1 = NULL, *bi2 = NULL;
	BMMRES isbm1Loaded, isbm2Loaded;
	BOOL SuccessLoadingBitmap = TRUE;
	bool isbm1MAXBitmap=false, isbm2MAXBitmap=false;

	long int mycount = 0;

	int tolerance = arg_list[2]->to_int();
	float variance = (arg_list[3]->to_float())/255.0;

	BOOL useAlpha = key_arg_or_default(useAlpha,&false_value)->to_bool();

	Value* errorMsgValue = key_arg(errorMsg);
	Thunk* errorMsgThunk = NULL;
	if (errorMsgValue != &unsupplied && errorMsgValue->_is_thunk()) 
		errorMsgThunk = errorMsgValue->to_thunk();

	TSTR bi1Name;
	TSTR bi2Name;

	// two step it here so that if an error is thrown, we don't leak memory
	if (!is_bitmap(arg_list[0]))
		bi1Name = arg_list[0]->to_filename();
	if (!is_bitmap(arg_list[1]))
		bi2Name = arg_list[1]->to_filename();

	// from here on we won't throw
	if (is_bitmap(arg_list[0]))
	{
		MAXBitMap *mbm = (MAXBitMap*)arg_list[0];
		bi1 = &mbm->bi;
		bi1Name = bi1->Filename();
		bm1 = mbm->bm;
		isbm1Loaded = BMMRES_SUCCESS;
		isbm1MAXBitmap=true;
	}
	else
	{
		bi1 = new BitmapInfo (bi1Name);
		BOOL oldSilentMode = TheManager->SetSilentMode(TRUE); // no error messages
		bm1 = TheManager->Load(bi1,&isbm1Loaded);
		TheManager->SetSilentMode(oldSilentMode);
	}
	if (is_bitmap(arg_list[1]))
	{
		MAXBitMap *mbm = (MAXBitMap*)arg_list[1];
		bi2 = &mbm->bi;
		bi2Name = bi2->Filename();
		bm2 = mbm->bm;
		isbm2Loaded = BMMRES_SUCCESS;
		isbm2MAXBitmap=true;
	}
	else
	{
		bi2 = new BitmapInfo (bi2Name);
		BOOL oldSilentMode = TheManager->SetSilentMode(TRUE); // no error messages
		bm2 = TheManager->Load(bi2,&isbm2Loaded);
		TheManager->SetSilentMode(oldSilentMode);
	}

	if (isbm1Loaded == BMMRES_SUCCESS && isbm2Loaded == BMMRES_SUCCESS && bm1 && bm2 && bi1 && bi2)
	{

		int biWidth = bi1->Width();
		int biHeight = bi1->Height();
		int bi2Width = bi2->Width();
		int bi2Height = bi2->Height();
		if ((biWidth == bi2Width) && (biHeight == bi2Height))
		{
			// iterate through pixels

			BMM_Color_fl c1, c2;

			int max_c = useAlpha ? 4 : 3;

			for (int i = 0; i < biWidth; i++)
			{
				for (int j = 0; j < biHeight; j++)
				{
					bm1->GetPixels(i,j,1,&c1);
					bm2->GetPixels(i,j,1,&c2);

					float pi1[4] = {c1.r,c1.g,c1.b,c1.a};
					float pi2[4] = {c2.r,c2.g,c2.b,c2.a};
					for (int c = 0; c < max_c; c++)
					{
						BOOL isDiff = MyComparePixels(pi1[c], pi2[c], variance);
						if (isDiff)
						{
							mycount++;
							break;
						}
					}
				}
			}
			if (mycount > tolerance)
				myStr.printf(GetString(IDS_DIFFERENT_PIXEL_COUNT), mycount);
			else 
			{
				myStr = GetString(IDS_BITMAP_COMPARE_PASSED);
				res = true;
			}

		}
		else 
			myStr = GetString(IDS_BITMAP_COMPARE_DIFF_SIZES);
		if (!isbm1MAXBitmap)
		{
			if (bi1) delete bi1;
			if (bm1) bm1->DeleteThis();
		}
		if (!isbm2MAXBitmap)
		{
			if (bi2) delete bi2;
			if (bm2) bm2->DeleteThis();
		}

	} 
	else 
		myStr = GetString(IDS_BITMAP_COMPARE_NO_BITMAP);


	if (errorMsgThunk)
		errorMsgThunk->assign(new String(myStr));
	return bool_result(res);
}

#define WIDEPRINT_BUF_WIDTH 8192

Value*
formattedPrint_cf(Value** arg_list, int count)
{
	check_arg_count_with_keys(formattedPrint, 1, count);
	Value* arg = arg_list[0];
	TSTR format;
	TSTR defaultFloatFormat = _T("%0.8g");
	TSTR Lbracket = _T("[");
	TSTR Rbracket = _T("]");
	TSTR Lparen = _T("(");
	TSTR Rparen = _T(")");
	TSTR space = _T(" ");
	TSTR comma = _T(",");
	TSTR f = _T("f");
	TSTR p = _T("%");
	TSTR formatSpec;
	bool formatGiven = false;
	Value* formatValue = key_arg(format)->eval();
	if (formatValue != &unsupplied)
	{
		formatSpec = formatValue->to_string();
		format = p+formatSpec;
		formatGiven = true;
	}

	set_english_numerics();
	TCHAR buf[WIDEPRINT_BUF_WIDTH];
	buf[0] = '\0';
	if (is_float(arg))
	{
		if (!formatGiven) 
			format = defaultFloatFormat;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, format, EPS(arg->to_float()));
	}
	else if (is_double(arg))
	{
		if (!formatGiven) 
			format = defaultFloatFormat;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, format, EPS(arg->to_double()));
	}
	else if (is_int(arg))
	{
		if (!formatGiven) 
			format = _T("%I32d");
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, format, arg->to_int());
	}
	else if (is_int64(arg))
	{
		if (!formatGiven) 
			format = _T("%I64d");
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, format, arg->to_int64());
	}
	else if (is_intptr(arg))
	{
		if (!formatGiven) 
			format = _T("%Id");
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, format, arg->to_intptr());
	}
	else if (is_string(arg))
	{
		if (!formatGiven) 
			format = _T("%d");
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, format, arg->to_string());
	}
	else if (is_bool(arg))
	{
		if (!formatGiven) 
			format = _T("%d");
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, format, arg->to_bool());
	}
	else if (arg->tag == class_tag(RayValue))
	{
		Ray r = arg->to_ray();
		if (!formatGiven) 
			format = defaultFloatFormat;
		TSTR formatRay = _T("(ray [");
		formatRay += format+comma+format+comma+format+Rbracket+space+Lbracket+format+comma+format+comma+format+Rbracket+Rparen;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatRay, EPS(r.p.x), EPS(r.p.y), EPS(r.p.z), EPS(r.dir.x), EPS(r.dir.y), EPS(r.dir.z));
	}
	else if (is_point2(arg))
	{
		Point2 p = arg->to_point2();
		if (!formatGiven) 
			format = defaultFloatFormat;
		TSTR formatP3 = Lbracket+format+comma+format+Rbracket;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatP3, EPS(p.x), EPS(p.y));
	}
	else if (is_point3(arg))
	{	
		Point3 p = arg->to_point3();
		if (!formatGiven) 
			format = defaultFloatFormat;
		TSTR formatP3 = Lbracket+format+comma+format+comma+format+Rbracket;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatP3, EPS(p.x), EPS(p.y), EPS(p.z));
	}
	else if (is_point4(arg))
	{	
		Point4 p = arg->to_point4();
		if (!formatGiven) 
			format = defaultFloatFormat;
		TSTR formatP4 = Lbracket+format+comma+format+comma+format+comma+format+Rbracket;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatP4, EPS(p.w), EPS(p.x), EPS(p.y), EPS(p.z));
	}
	else if (is_quat(arg))
	{
		Quat q = arg->to_quat();
		if (!formatGiven) 
			format = defaultFloatFormat;
		TSTR formatQ = _T("(quat ");
		formatQ += format+space+format+space+format+space+format+Rparen;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatQ, EPS(q.x), EPS(q.y), EPS(q.z), EPS(q.w));
	}
	else if (is_angaxis(arg))
	{
		AngAxis aa = arg->to_angaxis();
		if (!formatGiven) 
			format = defaultFloatFormat;
		TSTR formatAA= _T("(angleAxis ");
		formatAA += format+space+Lbracket+format+comma+format+comma+format+Rbracket+Rparen;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatAA, EPS(RadToDeg(aa.angle)), EPS(aa.axis.x), EPS(aa.axis.y), EPS(aa.axis.z));
	}
	else if (is_eulerangles(arg))
	{
		EulerAnglesValue* eav = (EulerAnglesValue*)arg;
		if (!formatGiven) 
			format = defaultFloatFormat;
		TSTR formatEA = _T("(eulerAngles ");
		formatEA += format+space+format+space+format+Rparen;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatEA, EPS(RadToDeg(eav->angles[0])), EPS(RadToDeg(eav->angles[1])), EPS(RadToDeg(eav->angles[2])));
	}
	else if (is_matrix3(arg))
	{
		Matrix3 m = arg->to_matrix3();
		if (!formatGiven) 
			format = defaultFloatFormat;
		TSTR formatM3= _T("(matrix3");
		formatM3 += space+Lbracket+format+comma+format+comma+format+Rbracket +
			space+Lbracket+format+comma+format+comma+format+Rbracket +
			space+Lbracket+format+comma+format+comma+format+Rbracket+Rparen;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatM3, 
			EPS(m.GetRow(0).x), EPS(m.GetRow(0).y), EPS(m.GetRow(0).z),
			EPS(m.GetRow(1).x), EPS(m.GetRow(1).y), EPS(m.GetRow(1).z),
			EPS(m.GetRow(2).x), EPS(m.GetRow(2).y), EPS(m.GetRow(2).z),
			EPS(m.GetRow(3).x), EPS(m.GetRow(3).y), EPS(m.GetRow(3).z));

	}
	else if (is_box2(arg))
	{
		Box2 b = arg->to_box2();
		if (!formatGiven) 
			format = _T("%d");
		TSTR formatB2 = _T("(box2 ");
		formatB2 += format+space+format+space+format+space+format+Rparen;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatB2, b.x(), b.y(), b.w(), b.h());
	}
	else if (is_color(arg))
	{
		AColor color = arg->to_acolor();
		if (!formatGiven) 
			format = defaultFloatFormat;
		TSTR formatC = _T("(color ");
		formatC += format+space+format+space+format;
		if (color.a != 1.0f)
			formatC += space+format;
		formatC += Rparen;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatC, EPS(color.r * 255.0f), EPS(color.g * 255.0f), EPS(color.b * 255.0f), EPS(color.a * 255.0f));
	}
	else if (is_time(arg))
	{
		if (!formatGiven) 
			format = defaultFloatFormat;
		TSTR formatT = format+f;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatT, EPS(arg->to_timevalue() / (float)GetTicksPerFrame()));
	}
	else if (is_interval(arg))
	{
		Interval interval = arg->to_interval();
		if (!formatGiven) 
			format = defaultFloatFormat;
		TSTR formatT = _T("(interval ");
		formatT += format+f+space+format+f+Rparen;
		_snprintf(buf, WIDEPRINT_BUF_WIDTH, formatT, 		
			EPS(interval.Start() / (float)GetTicksPerFrame()), 
			EPS(interval.End() / (float)GetTicksPerFrame()));
	}

	reset_numerics();
	return new String(buf);
}



/* --------------------- plug-in init --------------------------------- */
// this is called by the dlx initializer, register the global vars here
void avg_init()
{

	#include "lam_glbls.h"
	
//	def_user_prop (pinNode, MAXNode, getPinNode_cf, setPinNode_cf);
//	def_user_generic(testBitArray_UG, BitArrayValue, test);
//#define def_user_propM(_prop, _getter, _setter)					\
//		MAXNode_class.add_user_prop(#_prop, _getter, _setter);	\
//		MAXObject_class.add_user_prop(#_prop, _getter, _setter);	\
//		MeshValue_class.add_user_prop(#_prop, _getter, _setter);
#define def_user_propM(_prop, _getter, _setter)				\
		def_user_prop(_prop, MAXNode,   _getter, _setter);	\
		def_user_prop(_prop, MAXObject, _getter, _setter);	\
		def_user_prop(_prop, MeshValue, _getter, _setter);
	def_user_propM (hiddenVerts,	meshop_getHiddenVerts_cf,	meshop_setHiddenVerts_cf);
	def_user_propM (hiddenFaces,	meshop_getHiddenFaces_cf,	meshop_setHiddenFaces_cf);
	def_user_propM (openEdges,		meshop_getOpenEdges_cf,		NULL);

	def_user_prop (isPB2Based,	MAXClass,	isPB2Based_cf,	NULL);
	def_user_prop (isPB2Based,	MSPluginClass,	isPB2Based_cf,	NULL);
	def_user_prop (isPB2Based,	MSCustAttribDef,	isPB2Based_cf,	NULL);

	def_user_prop (isMSPluginClass,	MAXClass,	isMSPluginClass_cf,	NULL);
	def_user_prop (isMSPluginClass,	MSPluginClass,	isMSPluginClass_cf,	NULL);
	def_user_prop (isMSPluginClass,	MSCustAttribDef,	isMSPluginClass_cf,	NULL);

	def_user_prop (isMSCustAttribClass,	MAXClass,	isMSCustAttribClass_cf,	NULL);
	def_user_prop (isMSCustAttribClass,	MSPluginClass,	isMSCustAttribClass_cf,	NULL);
	def_user_prop (isMSCustAttribClass,	MSCustAttribDef,	isMSCustAttribClass_cf,	NULL);

	install_rollout_control(n_multiListBox,		MultiListBoxControl::create);	
}

/*


mouse test script:

rollout test "test1" 
( 
 timer clock "testClock" interval:40
 label lbl1 ""  
 label lbl2 ""
 on clock tick do 
 (lbl1.text = getMouseButtonStates() as string
  lbl2.text = getMouseMode() as string
 ) 
) 
floater=newrolloutfloater "" 100 100
addrollout test floater

MtlBase test script:

--	1 - MTL_BEING_EDITED
--	2 - MTL_MEDIT_BACKGROUND
--	3 - MTL_MEDIT_BACKLIGHT
--	4 - MTL_MEDIT_VIDCHECK

s=sphere()
tm
sm=s.material=standard diffusemap:(tm=checker())
meditmaterials[1]=sm
meditmaterials[2]=tm
-- material
getMTLMEditFlags sm
setMTLMEditFlags sm  #{}
getMTLMEditFlags sm
getMTLMeditObjType sm
setMTLMeditObjType sm  2
getMTLMeditObjType sm
-- texture
getMTLMEditFlags tm
setMTLMEditFlags tm  #{1..4}
getMTLMEditFlags tm
getMTLMeditObjType tm
setMTLMeditObjType tm  3
getMTLMeditObjType tm

*/

/*
Value*
tester_cf(Value** arg_list, int count)
{
ICUIFrame *cui = GetCUIFrameMgr()->GetICUIFrame("Command Panel");
if (cui) cui->Hide(TRUE);
if (cui) cui->Hide(FALSE);
return &ok;
}
*/

/*
// This used to cause an exception, now it doesn't. No idea what is being done differently
// might have had wrong run-time library specified.

Value*
tester_cf(Value** arg_list, int count)
{
	AdjEdgeList ae(*arg_list[0]->to_mesh());
	return &ok;
}


script for running tester:

mo=mesh lengthsegs:1 widthsegs:1
tester (copy mo.mesh)

result: user breakpoint:

NTDLL! 77f76274()
NTDLL! 77f8318c()
KERNEL32! 77f12e5a()
_CrtIsValidHeapPointer(const void * 0x088049e0) line 1606
_free_dbg_lk(void * 0x088049e0, int 1) line 1011 + 9 bytes
_free_dbg(void * 0x088049e0, int 1) line 970 + 13 bytes
free(void * 0x088049e0) line 926 + 11 bytes
operator delete(void * 0x088049e0) line 7 + 9 bytes
DWORDTab::`vector deleting destructor'(unsigned int 3) + 67 bytes
AdjEdgeList::~AdjEdgeList() line 48 + 78 bytes
tester_cf(Value * * 0x0012fa24, int 1) line 235 + 16 bytes
Primitive::apply(Value * * 0x08804c60, int 1) line 270 + 14 bytes
CodeTree::eval() line 117 + 26 bytes
ListenerDlgProc(HWND__ * 0x003e07c6, unsigned int 1288, unsigned int 0, long 89718596) line 705 + 17 bytes
USER32! 77e80cbb()
USER32! 77e83451()
USER32! 77e71268()
0558ff44()
*/


/*
Value*
tester_cf(Value** arg_list, int count)
{
	int counter=GetCUIFrameMgr()->GetCount();
	TCHAR* theName;
	for (int i = 0; i < counter; i++) 
		for (int j = 0; j < 3; j++) 
			for (int k = 0; k < 3; k++) {
				ICUIFrame *cui = GetCUIFrameMgr()->GetICUIFrame(i,j,k);
				if (cui) {
					theName = cui->GetName();
					OutputDebugString(theName);
				}
	}
	return &ok;
}
// output: Main Menu, Tab Panel 
*/

/*
g=geosphere()
at time 10 with animate on g.radius = 100
a=refs.dependents g.radius.controller
pb=a[1]
tester470888 pb
*/

/*
Value*
tester470888_cf(Value** arg_list, int count)
{
	ReferenceTarget *ref = arg_list[0]->to_reftarg();
	IParamBlock2 *pblock = (IParamBlock2*)ref;

	//PDL: Reset the param and try to set a value.
	//pblock->RemoveController(eFloatParam, 0); // Uncoment this line to work around the problem
	pblock->Reset(0); 
	// This line crashes because the implementation of reset doesn't
	//properly reset the flags. In particular Reset will delete the
	//controller but not change the flag to indicate that the param
	//is now constant.
	pblock->SetValue(0, 0, 25.0f);
	return &ok;
}
*/

/*
#ifdef _DEBUG
def_visible_primitive	( test_function,		"test_function");

PolyObject* GetPolyObjectFromNode(INode *pNode, TimeValue t, int &deleteIt)
{
	deleteIt = FALSE;
	Object *pObj = pNode->EvalWorldState(t).obj;
	if (pObj->CanConvertToType(Class_ID(POLYOBJ_CLASS_ID, 0))) { 
		PolyObject *pTri = (PolyObject *)pObj->ConvertToType(t, Class_ID(POLYOBJ_CLASS_ID,0));
		if (pObj != pTri) deleteIt = TRUE;
		return pTri;
	} else {
		return NULL;
	}
}

TriObject* GetTriObjectFromNode(INode *Node, TimeValue t, int &deleteIt) 
{
	deleteIt = FALSE;
	Object *pObj = Node->EvalWorldState(t).obj;
	if (pObj && pObj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID,0))) { 
		TriObject *pTri = (TriObject *)pObj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID,0));
		if (pObj != pTri) deleteIt = TRUE;
		return pTri;
	} else {
		return NULL;
	}

}

Value* 
test_function_cf(Value** arg_list, int count)
{
	if(MAXScript_interface->GetSelNodeCount() != 0) 
	{
		INode *pNode = MAXScript_interface->GetSelNode(0);
		BOOL deleteIt;
		PolyObject *pPolyObj = GetPolyObjectFromNode(pNode, MAXScript_interface->GetTime(), deleteIt);
		if (pPolyObj)
		{
			MNMesh *pMesh = &pPolyObj->mm;
			MNNormalSpec *pNormals = pMesh->GetSpecifiedNormals();
			if(deleteIt) delete pPolyObj;
		}
		TriObject *pTriObj = GetTriObjectFromNode(pNode, MAXScript_interface->GetTime(), deleteIt);
		if (pTriObj)
		{
			Mesh *pMesh = &pTriObj->mesh;
			MeshNormalSpec *pNormals = pMesh->GetSpecifiedNormals();
			if(deleteIt) delete pTriObj;
		}
	}
	FPValue res;
	ExecuteMAXScriptScript("maxops.pivotMode", TRUE, &res);

	static FunctionID fid;
	static FPInterface* maxops = NULL;

	if (maxops == NULL)
	{
		maxops = GetCOREInterface(MAIN_MAX_INTERFACE);
		FPInterfaceDesc* maxops_desc = maxops->GetDesc();
		for (int i = 0; i < maxops_desc->props.Count(); i++)
		{
			FPPropDef *propDef = maxops_desc->props[i];
			if (_tcsicmp("pivotMode", propDef->internal_name) == 0)
			{
				fid = propDef->getter_ID;
				break;
			}
		}
	}
	FPValue result;
	maxops->Invoke(fid, result);

	return &ok;
}

def_visible_primitive	( test_RefTargContainer,		"test_RefTargContainer");
#include "IRefTargContainer.h"

// NOTE: should cause 3 asserts to fire

Value* 
test_RefTargContainer_cf(Value** arg_list, int count)
{
	ClassDirectory& cdir = MAXScript_interface->GetDllDir().ClassDir();
	ClassDesc* cd = cdir.FindClassEntry(REF_TARGET_CLASS_ID, REFTARG_CONTAINER_CLASS_ID)->FullCD();
	ReferenceTarget *rt = (ReferenceTarget *)cd->Create();
	IRefTargContainer *rtc = (IRefTargContainer*)rt;

	int n = rtc->GetNumItems();
	assert(n == 0);
	ReferenceTarget *r = rtc->GetItem(0);
	assert(r == NULL);

	ClassDesc* bend_cd = cdir.FindClassEntry(OSM_CLASS_ID, Class_ID(BENDOSM_CLASS_ID,0))->FullCD();

	ReferenceTarget *bend1 = (ReferenceTarget *)bend_cd->Create();
	rtc->AppendItem(bend1);
	n = rtc->GetNumItems();
	assert(n == 1);
	r = rtc->GetItem(0);
	assert(r == bend1);

	ReferenceTarget *bend2 = (ReferenceTarget *)bend_cd->Create();
	rtc->AppendItem(bend2);
	n = rtc->GetNumItems();
	assert(n == 2);
	r = rtc->GetItem(0);
	assert(r == bend1);
	r = rtc->GetItem(1);
	assert(r == bend2);

	ReferenceTarget *bend3 = (ReferenceTarget *)bend_cd->Create();
	rtc->SetItem(0,bend3);
	n = rtc->GetNumItems();
	assert(n == 2);
	r = rtc->GetItem(0);
	assert(r == bend3);
	r = rtc->GetItem(1);
	assert(r == bend2);

	ReferenceTarget *bend4 = (ReferenceTarget *)bend_cd->Create();
	rtc->InsertItem(1,bend4);
	n = rtc->GetNumItems();
	assert(n == 3);
	r = rtc->GetItem(0);
	assert(r == bend3);
	r = rtc->GetItem(1);
	assert(r == bend4);
	r = rtc->GetItem(2);
	assert(r == bend2);

	ReferenceTarget *bend5 = (ReferenceTarget *)bend_cd->Create();
	rtc->AppendItem(bend5);
	n = rtc->GetNumItems();
	assert(n == 4);

	rtc->RemoveItem(0);
	n = rtc->GetNumItems();
	assert(n == 3);
	r = rtc->GetItem(0);
	assert(r == bend4);
	r = rtc->GetItem(1);
	assert(r == bend2);
	r = rtc->GetItem(2);
	assert(r == bend5);

	rtc->RemoveItem(1);
	n = rtc->GetNumItems();
	assert(n == 2);
	r = rtc->GetItem(0);
	assert(r == bend4);
	r = rtc->GetItem(1);
	assert(r == bend5);

	rtc->RemoveItem(1);
	n = rtc->GetNumItems();
	assert(n == 1);
	r = rtc->GetItem(0);
	assert(r == bend4);

	mputs(_T(" assert coming...   \n"));
	rtc->RemoveItem(1); // test assert
	r = rtc->GetItem(1);
	assert(r == NULL);

	rtc->RemoveItem(0);
	n = rtc->GetNumItems();
	assert(n == 0);

	bend1 = (ReferenceTarget *)bend_cd->Create();
	rtc->AppendItem(bend1);
	bend2 = (ReferenceTarget *)bend_cd->Create();
	rtc->AppendItem(bend2);
	n = rtc->GetNumItems();
	assert(n == 2);

	rt->MaybeAutoDelete();

	theHold.Accept(_T("phase 1"));
	theHold.Begin();

	rt = (ReferenceTarget *)cd->Create();
	rtc = (IRefTargContainer*)rt;
	bend1 = (ReferenceTarget *)bend_cd->Create();
	bend2 = (ReferenceTarget *)bend_cd->Create();
	rtc->SetItem(0,bend1);

	theHold.Accept(_T("phase 2"));
	theHold.Begin();

	rtc->SetItem(2,bend2);
	n = rtc->GetNumItems();
	assert(n == 3);
	r = rtc->GetItem(0);
	assert(r == bend1);
	r = rtc->GetItem(1);
	assert(r == NULL);
	r = rtc->GetItem(2);
	assert(r == bend2);

	theHold.Accept(_T("phase 3"));

	MAXScript_interface->ExecuteMAXCommand(MAXCOM_EDIT_UNDO);
	n = rtc->GetNumItems();
	assert(n == 1);
	r = rtc->GetItem(0);
	assert(r == bend1);

	MAXScript_interface->ExecuteMAXCommand(MAXCOM_EDIT_REDO);
	n = rtc->GetNumItems();
	assert(n == 3);
	r = rtc->GetItem(0);
	assert(r == bend1);
	r = rtc->GetItem(1);
	assert(r == NULL);
	r = rtc->GetItem(2);
	assert(r == bend2);

	MAXScript_interface->ExecuteMAXCommand(MAXCOM_EDIT_UNDO);
	n = rtc->GetNumItems();
	assert(n == 1);
	r = rtc->GetItem(0);
	assert(r == bend1);

	MAXScript_interface->ExecuteMAXCommand(MAXCOM_EDIT_REDO);
	n = rtc->GetNumItems();
	assert(n == 3);
	r = rtc->GetItem(0);
	assert(r == bend1);
	r = rtc->GetItem(1);
	assert(r == NULL);
	r = rtc->GetItem(2);
	assert(r == bend2);

	theHold.Begin();

	return &ok;
}
#endif // ifdef _DEBUG

def_visible_primitive	( test_function,		"test_function");

Value* 
test_function_cf(Value** arg_list, int count)
{
	TCHAR filename1[MAX_PATH];
	TSTR filename2 = _T("C:\\test\\");
	_tcscpy(filename1, filename2);
	BMMRemoveSlash(filename1);
	BMMRemoveSlash(filename1);
	BMMAppendSlash(filename1);
	BMMAppendSlash(filename1);

	BMMRemoveSlash(filename2);
	BMMRemoveSlash(filename2);
	BMMAppendSlash(filename2);
	BMMAppendSlash(filename2);

	filename2 = _T("C:/test/");
	_tcscpy(filename1, filename2);
	BMMRemoveSlash(filename1);
	BMMRemoveSlash(filename1);
	BMMAppendSlash(filename1);
	BMMAppendSlash(filename1);

	BMMRemoveSlash(filename2);
	BMMRemoveSlash(filename2);
	BMMAppendSlash(filename2);
	BMMAppendSlash(filename2);

	return &ok;
	}

def_visible_primitive	( test_function,		"test_function");

class  TestAnimEnumProc : public Animatable::EnumAnimList
{
public:
	int index;
	Animatable *delAnim, *delAnimNext;
	ULONG numAnims, numRefMakers, numRetTargs;
	TestAnimEnumProc(int _index, Animatable *_delAnim, Animatable *_delAnimNext) : 
		index(_index), numAnims(0), numRefMakers(0), numRetTargs(0), 
		delAnim(_delAnim), delAnimNext(_delAnimNext){}
	bool proc(Animatable *theAnim)
	{
		if (theAnim == NULL) return true;
		if (!theAnim->TestFlagBit(index))
		{
			theAnim->SetFlagBit(index);
			numAnims++;
			if (theAnim->IsRefMaker()) 
			{
				numRefMakers++;
				assert (theAnim->GetInterface(REFERENCE_MAKER_INTERFACE));
				if (((ReferenceMaker*)theAnim)->IsRefTarget()) 
				{
					numRetTargs++;
					assert (theAnim->GetInterface(REFERENCE_TARGET_INTERFACE));
				}
				else
				{
					assert (theAnim->GetInterface(REFERENCE_TARGET_INTERFACE) == NULL);
				}
			}
			else
			{
				assert (theAnim->GetInterface(REFERENCE_MAKER_INTERFACE) == NULL);
			}
		}
		if (delAnim == theAnim)
		{
			delAnimNext->DeleteThis();
		}
		return true;
	}
};

Value* 
test_function_cf(Value** arg_list, int count)
{
	int index1 = Animatable::RequestFlagBit();
	Animatable::ReleaseFlagBit(index1);
	index1 = Animatable::RequestFlagBit();
	int index2 = Animatable::RequestFlagBit();
	Animatable::ClearFlagBitInAllAnimatables(index2);
	Animatable* testAnim1 = new Animatable();
	Animatable* testAnim2 = new Animatable();
	Animatable* testAnim3 = new Animatable();
	TestAnimEnumProc testAnimEnumProc1(index2, testAnim1, testAnim2);
	testAnim2 = NULL;
	Animatable::EnumerateAllAnimatables(testAnimEnumProc1);
	DebugPrint("# anims: %d; # refmakers: %d; # reftargs: %d\n", 
		testAnimEnumProc1.numAnims,
		testAnimEnumProc1.numRefMakers,
		testAnimEnumProc1.numRetTargs);
	TestAnimEnumProc testAnimEnumProc2(index2, testAnim1, testAnim3);
	testAnim3 = NULL;
	Animatable::EnumerateAllAnimatables(testAnimEnumProc2);
	Animatable::ClearFlagBitInAllAnimatables(index2);
	TestAnimEnumProc testAnimEnumProc3(index2, NULL, NULL);
	Animatable::EnumerateAllAnimatables(testAnimEnumProc3);
	testAnim1->DeleteThis();
	testAnim1 = NULL;
	Animatable::ReleaseFlagBit(index2);
	Animatable::ReleaseFlagBit(index1);
	return &ok;
}

*/


