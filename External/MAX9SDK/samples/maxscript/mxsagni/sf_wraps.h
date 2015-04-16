/* ViewExp Class */
def_struct_primitive			( VP_SetType,			viewport,		"SetType");
def_struct_primitive_debug_ok	( VP_GetType,			viewport,		"GetType");
def_struct_primitive			( VP_SetTM,				viewport,		"SetTM");
def_struct_primitive_debug_ok	( VP_GetTM,				viewport,		"GetTM");
def_struct_primitive			( VP_SetCamera,			viewport,		"SetCamera");
def_struct_primitive_debug_ok	( VP_GetCamera,			viewport,		"GetCamera");
def_struct_primitive			( VP_SetLayout,			viewport,		"SetLayout");
def_struct_primitive_debug_ok	( VP_GetLayout,			viewport,		"GetLayout");

def_struct_primitive			( VP_SetRenderLevel,	viewport,		"SetRenderLevel");
def_struct_primitive_debug_ok	( VP_GetRenderLevel,	viewport,		"GetRenderLevel");

def_struct_primitive			( VP_SetShowEdgeFaces,	viewport,		"SetShowEdgeFaces");
def_struct_primitive_debug_ok	( VP_GetShowEdgeFaces,	viewport,		"GetShowEdgeFaces");

def_struct_primitive			( VP_SetTransparencyLevel, viewport,	"SetTransparencyLevel");
def_struct_primitive_debug_ok	( VP_GetTransparencyLevel, viewport,	"GetTransparencyLevel");

// LAM: Added these
#ifndef NO_REGIONS
def_struct_primitive			( VP_SetRegionRect,		viewport,		"SetRegionRect");
def_struct_primitive_debug_ok	( VP_GetRegionRect,		viewport,		"GetRegionRect");
def_struct_primitive			( VP_SetBlowupRect,		viewport,		"SetBlowupRect");
def_struct_primitive_debug_ok	( VP_GetBlowupRect,		viewport,		"GetBlowupRect");
#endif // NO_REGIONS

def_struct_primitive			( VP_SetGridVisibility,	viewport,		"SetGridVisibility");
def_struct_primitive_debug_ok	( VP_GetGridVisibility,	viewport,		"GetGridVisibility");

// RK: Added this
def_struct_primitive			( VP_ResetAllViews,		viewport,		"resetAllViews");
//watje
def_struct_primitive			( VP_ZoomToBounds,		viewport,		"ZoomToBounds");

// David Cunnigham: 
def_struct_primitive_debug_ok	( VP_CanSetToViewport,	viewport,		"CanSetToViewport");

def_struct_primitive_debug_ok	( VP_IsEnabled,			viewport,		"IsEnabled");

def_struct_primitive_debug_ok	( GetFOV,				viewport,		"GetFOV");
def_struct_primitive_debug_ok	( GetScreenScaleFactor,	viewport,		"GetScreenScaleFactor");

def_struct_primitive_debug_ok	( VP_IsPerspView,		viewport,		"IsPerspView");

def_struct_primitive_debug_ok	( VP_IsWire,			viewport,		"IsWire");

def_struct_primitive_debug_ok	( VP_GetFPS,		viewport,		"GetFPS");
