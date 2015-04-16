
#ifndef __IUNWRAP8__H
#define __IUNWRAP8__H

#include "iFnPub.h"

#define UNWRAP_INTERFACE5 Interface_ID(0x53b3409b, 0x18ff7ad2)

enum {  unwrap_pelt_getmapmode,unwrap_pelt_setmapmode, 
		unwrap_pelt_geteditseamsmode,unwrap_pelt_seteditseamsmode, 
		unwrap_pelt_setseamselection,unwrap_pelt_getseamselection, 

		unwrap_pelt_getpointtopointseamsmode,unwrap_pelt_setpointtopointseamsmode, 

		unwrap_pelt_expandtoseams,
		unwrap_pelt_dialog,
		
		unwrap_peltdialog_resetrig,
		unwrap_peltdialog_selectrig,
		unwrap_peltdialog_selectpelt,
		unwrap_peltdialog_snaprigtoedges,
		unwrap_peltdialog_mirrorrig,

		unwrap_peltdialog_setstraightenmode,unwrap_peltdialog_getstraightenmode,
		unwrap_peltdialog_straighten,
		unwrap_peltdialog_run,
		unwrap_peltdialog_relax1,
		unwrap_peltdialog_relax2,

		unwrap_peltdialog_getframes,unwrap_peltdialog_setframes,
		unwrap_peltdialog_getsamples,unwrap_peltdialog_setsamples,

		unwrap_peltdialog_getrigstrength,unwrap_peltdialog_setrigstrength,
		unwrap_peltdialog_getstiffness,unwrap_peltdialog_setstiffness,

		unwrap_peltdialog_getdampening,unwrap_peltdialog_setdampening,
		unwrap_peltdialog_getdecay,unwrap_peltdialog_setdecay,
		unwrap_peltdialog_getmirroraxis,unwrap_peltdialog_setmirroraxis,

		unwrap_mapping_align,
		unwrap_mappingmode,
		unwrap_setgizmotm,unwrap_getgizmotm,

		unwrap_gizmofit,
		unwrap_gizmocenter,
		unwrap_gizmoaligntoview,

		unwrap_geomedgeselectionexpand,unwrap_geomedgeselectioncontract,

		unwrap_geomedgeloop,
		unwrap_geomedgering,

		unwrap_geomvertexselectionexpand,unwrap_geomvertexselectioncontract,

		unwrap_setgeomvertexselection,unwrap_getgeomvertexselection, 
		unwrap_setgeomedgeselection,unwrap_getgeomedgeselection, 

		unwrap_pelt_getshowseam,unwrap_pelt_setshowseam,

		unwrap_pelt_seamtosel,	unwrap_pelt_seltoseam,
		unwrap_getnormalizemap,unwrap_setnormalizemap,
		
		unwrap_getshowedgedistortion,unwrap_setshowedgedistortion,

		unwrap_getlockedgesprings,unwrap_setlockedgesprings,

		unwrap_selectoverlap,

		unwrap_gizmoreset,

		unwrap_getedgedistortionscale,unwrap_setedgedistortionscale,

		unwrap_relaxspring,
		unwrap_relaxspringdialog,
		unwrap_getrelaxspringiteration,unwrap_setrelaxspringiteration,
		unwrap_getrelaxspringstretch,unwrap_setrelaxspringstretch,

		unwrap_getrelaxspringvedges,unwrap_setrelaxspringvedges,


		unwrap_getrelaxstretch,unwrap_setrelaxstretch,
		unwrap_getrelaxtype,unwrap_setrelaxtype,

		unwrap_relaxbyfaceangle,unwrap_relaxbyedgeangle,

		unwrap_setwindowxoffset,
		unwrap_setwindowyoffset,
		unwrap_frameselectedelement,

		unwrap_setshowcounter,	unwrap_getshowcounter,
		
		unwrap_renderuv_dialog,
		unwrap_renderuv,
		unwrap_ismesh,
		unwrap_qmap,

		unwrap_qmap_setpreview,unwrap_qmap_getpreview,		

		unwrap_setshowmapseams,unwrap_getshowmapseams,		

};

class IUnwrapMod5 : public FPMixinInterface  //interface for R8
	{
	public:

		//Function Publishing System
		//Function Map For Mixin Interface
		//*************************************************
		BEGIN_FUNCTION_MAP

//			VFN_1(unwrap_pelt_setmapmode, fnSetPeltMapMode,TYPE_BOOL);
//			FN_0(unwrap_pelt_getmapmode, TYPE_BOOL, fnGetPeltMapMode);

			VFN_1(unwrap_pelt_seteditseamsmode, fnSetPeltEditSeamsMode,TYPE_BOOL);
			FN_0(unwrap_pelt_geteditseamsmode, TYPE_BOOL, fnGetPeltEditSeamsMode);

			VFN_1(unwrap_pelt_setseamselection, fnSetSeamSelection,TYPE_BITARRAY);
			FN_0(unwrap_pelt_getseamselection,TYPE_BITARRAY, fnGetSeamSelection);

			VFN_1(unwrap_pelt_setpointtopointseamsmode, fnSetPeltPointToPointSeamsMode,TYPE_BOOL);
			FN_0(unwrap_pelt_getpointtopointseamsmode, TYPE_BOOL, fnGetPeltPointToPointSeamsMode);

			VFN_0(unwrap_pelt_expandtoseams, fnPeltExpandSelectionToSeams);
			VFN_0(unwrap_pelt_dialog, fnPeltDialog);

			VFN_0(unwrap_peltdialog_resetrig, fnPeltDialogResetRig);
			VFN_0(unwrap_peltdialog_selectrig, fnPeltDialogSelectRig);
			VFN_0(unwrap_peltdialog_selectpelt, fnPeltDialogSelectPelt);

			VFN_0(unwrap_peltdialog_snaprigtoedges, fnPeltDialogSnapRig);
			VFN_0(unwrap_peltdialog_mirrorrig, fnPeltDialogMirrorRig);

			VFN_1(unwrap_peltdialog_setstraightenmode, fnSetPeltDialogStraightenSeamsMode,TYPE_BOOL);
			FN_0(unwrap_peltdialog_getstraightenmode, TYPE_BOOL, fnGetPeltDialogStraightenSeamsMode);

			VFN_2(unwrap_peltdialog_straighten,fnPeltDialogStraighten,TYPE_INT,TYPE_INT);

			VFN_0(unwrap_peltdialog_run, fnPeltDialogRun);
			VFN_0(unwrap_peltdialog_relax1, fnPeltDialogRelax1);
			VFN_0(unwrap_peltdialog_relax2, fnPeltDialogRelax2);

			VFN_1(unwrap_peltdialog_setframes, fnSetPeltDialogFrames,TYPE_INT);
			FN_0(unwrap_peltdialog_getframes, TYPE_INT, fnGetPeltDialogFrames);

			VFN_1(unwrap_peltdialog_setsamples, fnSetPeltDialogSamples,TYPE_INT);
			FN_0(unwrap_peltdialog_getsamples, TYPE_INT, fnGetPeltDialogSamples);

			VFN_1(unwrap_peltdialog_setrigstrength, fnSetPeltDialogRigStrength,TYPE_FLOAT);
			FN_0(unwrap_peltdialog_getrigstrength, TYPE_FLOAT, fnGetPeltDialogRigStrength);

			VFN_1(unwrap_peltdialog_setstiffness, fnSetPeltDialogStiffness,TYPE_FLOAT);
			FN_0(unwrap_peltdialog_getstiffness, TYPE_FLOAT, fnGetPeltDialogStiffness);

			VFN_1(unwrap_peltdialog_setdampening, fnSetPeltDialogDampening,TYPE_FLOAT);
			FN_0(unwrap_peltdialog_getdampening, TYPE_FLOAT, fnGetPeltDialogDampening);

			VFN_1(unwrap_peltdialog_setdecay, fnSetPeltDialogDecay,TYPE_FLOAT);
			FN_0(unwrap_peltdialog_getdecay, TYPE_FLOAT, fnGetPeltDialogDecay);

			VFN_1(unwrap_peltdialog_setmirroraxis, fnSetPeltDialogMirrorAxis,TYPE_FLOAT);
			FN_0(unwrap_peltdialog_getmirroraxis, TYPE_FLOAT, fnGetPeltDialogMirrorAxis);


			VFN_1(unwrap_mapping_align, fnAlignAndFit,TYPE_INT);
			VFN_1(unwrap_mappingmode, fnSetMapMode,TYPE_INT);

			VFN_1(unwrap_setgizmotm, fnSetGizmoTM,TYPE_MATRIX3);
			FN_0(unwrap_getgizmotm, TYPE_MATRIX3, fnGetGizmoTM);
			
			VFN_0(unwrap_gizmofit, fnGizmoFit);
			VFN_0(unwrap_gizmocenter, fnGizmoCenter);
			
			VFN_0(unwrap_gizmoaligntoview, fnGizmoAlignToView);

			VFN_0(unwrap_geomedgeselectionexpand, fnGeomExpandEdgeSel);
			VFN_0(unwrap_geomedgeselectioncontract, fnGeomContractEdgeSel);

			VFN_0(unwrap_geomedgeloop, fnGeomLoopSelect);
			VFN_0(unwrap_geomedgering, fnGeomRingSelect);

			VFN_0(unwrap_geomvertexselectionexpand, fnGeomExpandVertexSel);
			VFN_0(unwrap_geomvertexselectioncontract, fnGeomContractVertexSel);

			VFN_1(unwrap_setgeomvertexselection, fnSetGeomVertexSelection,TYPE_BITARRAY);
			FN_0(unwrap_getgeomvertexselection,TYPE_BITARRAY, fnGetGeomVertexSelection);

			VFN_1(unwrap_setgeomedgeselection, fnSetGeomEdgeSelection,TYPE_BITARRAY);
			FN_0(unwrap_getgeomedgeselection,TYPE_BITARRAY, fnGetGeomEdgeSelection);


			FN_0(unwrap_pelt_getshowseam, TYPE_BOOL, fnGetAlwayShowPeltSeams);
			VFN_1(unwrap_pelt_setshowseam, fnSetAlwayShowPeltSeams,TYPE_BOOL);

			VFN_1(unwrap_pelt_seamtosel, fnPeltSeamsToSel,TYPE_BOOL);
			VFN_1(unwrap_pelt_seltoseam, fnPeltSelToSeams,TYPE_BOOL);

			VFN_1(unwrap_setnormalizemap, fnSetNormalizeMap,TYPE_BOOL);
			FN_0(unwrap_getnormalizemap, TYPE_BOOL, fnGetNormalizeMap);

			VFN_1(unwrap_setshowedgedistortion, fnSetShowEdgeDistortion,TYPE_BOOL);
			FN_0(unwrap_getshowedgedistortion, TYPE_BOOL, fnGetShowEdgeDistortion);

			VFN_1(unwrap_setlockedgesprings, fnSetLockSpringEdges,TYPE_BOOL);
			FN_0(unwrap_getlockedgesprings, TYPE_BOOL, fnGetLockSpringEdges);

			VFN_0(unwrap_selectoverlap, fnSelectOverlap);

			VFN_0(unwrap_gizmoreset, fnGizmoReset);
			
			VFN_1(unwrap_setedgedistortionscale, fnSetEdgeDistortionScale,TYPE_FLOAT);
			FN_0(unwrap_getedgedistortionscale, TYPE_FLOAT, fnGetEdgeDistortionScale);

			VFN_3(unwrap_relaxspring, fnRelaxBySprings,TYPE_INT,TYPE_FLOAT,TYPE_BOOL);
			VFN_0(unwrap_relaxspringdialog, fnRelaxBySpringsDialog);


			VFN_1(unwrap_setrelaxspringiteration, fnSetRelaxBySpringIteration,TYPE_INT);
			FN_0(unwrap_getrelaxspringiteration, TYPE_INT, fnGetRelaxBySpringIteration);

			VFN_1(unwrap_setrelaxspringstretch, fnSetRelaxBySpringStretch,TYPE_FLOAT);
			FN_0(unwrap_getrelaxspringstretch, TYPE_FLOAT, fnGetRelaxBySpringStretch);

			VFN_1(unwrap_setrelaxspringvedges, fnSetRelaxBySpringVEdges,TYPE_BOOL);
			FN_0(unwrap_getrelaxspringvedges, TYPE_BOOL, fnGetRelaxBySpringVEdges);

			VFN_1(unwrap_setrelaxstretch, fnSetRelaxStretch,TYPE_FLOAT);
			FN_0(unwrap_getrelaxstretch, TYPE_FLOAT, fnGetRelaxStretch);

			VFN_1(unwrap_setrelaxtype, fnSetRelaxType,TYPE_INT);
			FN_0(unwrap_getrelaxtype, TYPE_INT, fnGetRelaxType);

			VFN_4(unwrap_relaxbyfaceangle, fnRelaxByFaceAngleNoDialog,TYPE_INT,TYPE_FLOAT,TYPE_FLOAT,TYPE_BOOL);
			VFN_4(unwrap_relaxbyedgeangle, fnRelaxByEdgeAngleNoDialog,TYPE_INT,TYPE_FLOAT,TYPE_FLOAT,TYPE_BOOL);

			VFN_1(unwrap_setwindowxoffset, fnSetWindowXOffset,TYPE_INT);
			VFN_1(unwrap_setwindowyoffset, fnSetWindowYOffset,TYPE_INT);

			VFN_0(unwrap_frameselectedelement, fnFrameSelectedElement);
			
			VFN_1(unwrap_setshowcounter, fnSetShowCounter,TYPE_BOOL);
			FN_0(unwrap_getshowcounter, TYPE_BOOL, fnGetShowCounter);

			VFN_0(unwrap_renderuv_dialog, fnRenderUVDialog);

			VFN_1(unwrap_renderuv, fnRenderUV, TYPE_FILENAME);
			FN_0(unwrap_ismesh, TYPE_BOOL,fnIsMesh);

			VFN_0(unwrap_qmap, fnQMap);

			VFN_1(unwrap_qmap_setpreview, fnSetQMapPreview,TYPE_BOOL);
			FN_0(unwrap_qmap_getpreview, TYPE_BOOL, fnGetQMapPreview);

			VFN_1(unwrap_setshowmapseams, fnSetShowMapSeams,TYPE_BOOL);
			FN_0(unwrap_getshowmapseams, TYPE_BOOL, fnGetShowMapSeams);



		END_FUNCTION_MAP

		FPInterfaceDesc* GetDesc();    // <-- must implement 

//		virtual BOOL fnGetPeltMapMode() = 0;
//		virtual void fnSetPeltMapMode(BOOL mode) = 0;
				
		virtual BOOL fnGetPeltEditSeamsMode() = 0;
		virtual void fnSetPeltEditSeamsMode(BOOL mode) = 0;

		virtual BitArray* fnGetSeamSelection()=0;
		virtual void fnSetSeamSelection(BitArray *sel)=0;

		virtual BOOL fnGetPeltPointToPointSeamsMode() = 0;
		virtual void fnSetPeltPointToPointSeamsMode(BOOL mode) = 0;

		virtual void fnPeltExpandSelectionToSeams() = 0;
		virtual void fnPeltDialog() = 0;

		virtual void fnPeltDialogResetRig() = 0;

		virtual void fnPeltDialogSelectRig() = 0;
		virtual void fnPeltDialogSelectPelt() = 0;

		virtual void fnPeltDialogSnapRig() = 0;
		virtual void fnPeltDialogMirrorRig() = 0;

		virtual BOOL fnGetPeltDialogStraightenSeamsMode() = 0;
		virtual void fnSetPeltDialogStraightenSeamsMode(BOOL mode) = 0;

		virtual void fnPeltDialogStraighten(int a, int b) = 0;

		virtual void fnPeltDialogRun() = 0;
		virtual void fnPeltDialogRelax1() = 0;
		virtual void fnPeltDialogRelax2() = 0;

		virtual int fnGetPeltDialogFrames()=0;
		virtual void fnSetPeltDialogFrames(int frames)=0;

		virtual int fnGetPeltDialogSamples()=0;
		virtual void fnSetPeltDialogSamples(int samples)=0;

		virtual float fnGetPeltDialogRigStrength()=0;
		virtual void fnSetPeltDialogRigStrength(float strength)=0;

		virtual float fnGetPeltDialogStiffness()=0;
		virtual void fnSetPeltDialogStiffness(float stiffness)=0;

		virtual float fnGetPeltDialogDampening()=0;
		virtual void fnSetPeltDialogDampening(float dampening)=0;

		virtual float fnGetPeltDialogDecay()=0;
		virtual void fnSetPeltDialogDecay(float decay)=0;

		virtual float fnGetPeltDialogMirrorAxis()=0;
		virtual void fnSetPeltDialogMirrorAxis(float axis)=0;

		virtual void fnAlignAndFit(int axis) = 0;

		virtual void fnSetMapMode(int mode) = 0;


		virtual void fnSetGizmoTM(Matrix3 tm) = 0;
		virtual Matrix3 *fnGetGizmoTM() = 0;

		virtual void fnGizmoFit() = 0;
		virtual void fnGizmoCenter() = 0;

		virtual void fnGizmoAlignToView() = 0;
		
		virtual void	fnGeomExpandEdgeSel() = 0;
		virtual void	fnGeomContractEdgeSel() = 0;

		virtual void	fnGeomLoopSelect() = 0;
		virtual void	fnGeomRingSelect() = 0;

		virtual void	fnGeomExpandVertexSel() = 0;
		virtual void	fnGeomContractVertexSel() = 0;

		virtual BitArray* fnGetGeomVertexSelection()=0;
		virtual void fnSetGeomVertexSelection(BitArray *sel)=0;

		virtual BitArray* fnGetGeomEdgeSelection()=0;
		virtual void fnSetGeomEdgeSelection(BitArray *sel)=0;

		virtual BOOL fnGetAlwayShowPeltSeams()=0;
		virtual void fnSetAlwayShowPeltSeams(BOOL show)=0;

		virtual void fnPeltSeamsToSel(BOOL replace)= 0;
		virtual void fnPeltSelToSeams(BOOL replace)= 0;
		
		virtual void fnSetNormalizeMap(BOOL normalize) = 0;
		virtual BOOL fnGetNormalizeMap() = 0;


		virtual void fnSetShowEdgeDistortion(BOOL show) = 0;
		virtual BOOL fnGetShowEdgeDistortion() = 0;

		virtual void fnSetLockSpringEdges(BOOL lock) = 0;
		virtual BOOL fnGetLockSpringEdges() = 0;

		virtual void fnSelectOverlap() = 0;

		virtual void fnGizmoReset() = 0;

		
		virtual void fnSetEdgeDistortionScale(float scale) = 0;
		virtual float fnGetEdgeDistortionScale() = 0;

		virtual void fnRelaxBySprings(int frames, float stretch, BOOL vEdgesOnly) = 0;
		virtual void fnRelaxBySpringsDialog() = 0;
	
		virtual int fnGetRelaxBySpringIteration() = 0;
		virtual void fnSetRelaxBySpringIteration(int iteration) = 0;

		virtual float fnGetRelaxBySpringStretch() = 0;
		virtual void fnSetRelaxBySpringStretch(float stretch) = 0;


		virtual BOOL fnGetRelaxBySpringVEdges() = 0;
		virtual void fnSetRelaxBySpringVEdges(BOOL useVEdges) = 0;

		virtual float fnGetRelaxStretch() = 0;
		virtual void fnSetRelaxStretch(float stretch) = 0;

		virtual int fnGetRelaxType() = 0;
		virtual void fnSetRelaxType(int type) = 0;

		virtual void fnRelaxByFaceAngleNoDialog(int frames, float stretch, float str, BOOL lockOuterVerts) = 0;;
		virtual void fnRelaxByEdgeAngleNoDialog(int frames, float stretch,  float str, BOOL lockOuterVerts) = 0;;


		virtual void fnSetWindowXOffset(int offset) = 0;
		virtual void fnSetWindowYOffset(int offset) = 0;

		virtual void fnFrameSelectedElement() = 0;

		virtual void fnSetShowCounter(BOOL show) = 0;
		virtual BOOL fnGetShowCounter() = 0;

		virtual void fnRenderUVDialog() = 0;
		virtual void fnRenderUV(TCHAR *name) = 0;
		virtual BOOL fnIsMesh() = 0;

		virtual void fnQMap() = 0;

		virtual void fnSetQMapPreview(BOOL preview) = 0;
		virtual BOOL fnGetQMapPreview() = 0;

		virtual void fnSetShowMapSeams(BOOL show) = 0;
		virtual BOOL fnGetShowMapSeams() = 0;

		
		
	};
#endif