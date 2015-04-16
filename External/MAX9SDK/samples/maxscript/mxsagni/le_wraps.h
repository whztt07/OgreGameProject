
/*Interface class and miscellaneous functions*/

def_visible_primitive			( ConfigureBitmapPaths,		"ConfigureBitmapPaths");
#ifndef NO_ATMOSPHERICS	// russom - 04/11/02
def_visible_primitive			( EditAtmosphere,			"EditAtmosphere");
#endif
def_visible_primitive_debug_ok	( CheckForSave,				"CheckForSave");


/* Object Class */ 
def_visible_primitive_debug_ok	( GetPolygonCount,			"GetPolygonCount");
def_visible_primitive_debug_ok	( GetTriMeshFaceCount,		"GetTriMeshFaceCount");
def_visible_primitive_debug_ok	( IsPointSelected,			"IsPointSelected");
def_visible_primitive_debug_ok	( NumMapsUsed,				"NumMapsUsed");
def_visible_primitive_debug_ok	( IsShapeObject,			"IsShapeObject");
def_visible_primitive_debug_ok	( PointSelection,			"PointSelection");
def_visible_primitive_debug_ok	( NumSurfaces,				"NumSurfaces");
def_visible_primitive_debug_ok	( IsSurfaceUVClosed,		"IsSurfaceUVClosed");

/* Miscellaneous functions */
def_visible_primitive			( DeselectHiddenEdges,		"DeselectHiddenEdges");
def_visible_primitive			( DeselectHiddenFaces,		"DeselectHiddenFaces");
def_visible_primitive_debug_ok	( AverageSelVertCenter,		"AverageSelVertCenter");
def_visible_primitive_debug_ok	( AverageSelVertNormal,		"AverageSelVertNormal");
def_visible_primitive_debug_ok	( MatrixFromNormal,			"MatrixFromNormal");

/* Patch Objects */
#ifndef NO_PATCHES
def_visible_primitive			( SetPatchSteps,			"SetPatchSteps");
def_visible_primitive_debug_ok	( GetPatchSteps,			"GetPatchSteps");
#endif

/* Euler Angles */
def_visible_primitive_debug_ok	( GetEulerQuatAngleRatio,	"GetEulerQuatAngleRatio");
def_visible_primitive_debug_ok	( GetEulerMatAngleRatio,	"GetEulerMatAngleRatio");

/* IK controller stuff */

#ifndef WEBVERSION
def_struct_primitive_debug_ok	( GetStartTime,		ik,		"GetStartTime");
def_struct_primitive			( SetStartTime,		ik,		"SetStartTime");
def_struct_primitive_debug_ok	( GetEndTime,		ik,		"GetEndTime");
def_struct_primitive			( SetEndTime,		ik,		"SetEndTime");
def_struct_primitive			( SetPosThreshold,	ik,		"SetPosThreshold");
def_struct_primitive_debug_ok	( GetPosThreshold,	ik,		"GetPosThreshold");
def_struct_primitive			( SetRotThreshold,	ik,		"SetRotThreshold");
def_struct_primitive_debug_ok	( GetRotThreshold,	ik,		"GetRotThreshold");
#endif //WEBVERSION
def_struct_primitive_debug_ok	( GetIterations,	ik,		"GetIterations");
def_struct_primitive			( SetIterations,	ik,		"SetIterations");


/*SystemTools  -  Utility functions to pool the state of the system
  Added by AF (09/28/00)                                         */
def_struct_primitive_debug_ok	( IsDebugging,			systemTools,	"IsDebugging");
def_struct_primitive_debug_ok	( NumberOfProcessors,	systemTools,	"NumberOfProcessors");
def_struct_primitive_debug_ok	( IsWindows9x,			systemTools,	"IsWindows9x");
def_struct_primitive_debug_ok	( IsWindows98or2000,	systemTools,	"IsWindows98or2000");
def_struct_primitive_debug_ok	( GetScreenWidth,		systemTools,	"GetScreenWidth");
def_struct_primitive_debug_ok	( GetScreenHeight,		systemTools,	"GetScreenHeight");
