#include "unwrap.h"

#include "3dsmaxport.h"

/*
workflow 
user selects some faces
user hits Pelt Map
user defines the plane
user defines/refines the seams
user brings up pelt dialog

add the display
	add teh outer ring list
	add the outer ring edges

	draw the spring
	draw the tension

add the sim run button

create a seam list
add the seam point to point mode
add the seam display
add the hit seam mode

fix the selection of the faces in the dialog
add the UI
add some seam edge tools
	by selection
	by point to point

*/

PeltEdgeMode* PeltData::peltEdgeMode   = NULL;

INT_PTR CALLBACK UnwrapMapRollupWndProc(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
 	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);
	
	static BOOL inEnter = FALSE;

	switch (msg) {
		case WM_INITDIALOG:
			{
			mod = (UnwrapMod*)lParam;
			DLSetWindowLongPtr(hWnd, lParam);
			mod->hMapParams = hWnd;

			ICustButton *iPeltButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_PELT2));
			iPeltButton->SetType(CBT_CHECK);
			iPeltButton->SetHighlightColor(GREEN_WASH);
			ReleaseICustButton(iPeltButton);
			

			ICustButton *iPlanarButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_PLANAR));
			iPlanarButton->SetType(CBT_CHECK);
			iPlanarButton->SetHighlightColor(GREEN_WASH);			
			ReleaseICustButton(iPlanarButton);


			ICustButton *iCylindricalButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_CYLINDRICAL));
			iCylindricalButton->SetType(CBT_CHECK);
			iCylindricalButton->SetHighlightColor(GREEN_WASH);			
			ReleaseICustButton(iCylindricalButton);

 			ICustButton *iSphericalButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_SPHERICAL));
			iSphericalButton->SetType(CBT_CHECK);
			iSphericalButton->SetHighlightColor(GREEN_WASH);			
			ReleaseICustButton(iSphericalButton);

			ICustButton *iBoxButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_BOX));
			iBoxButton->SetType(CBT_CHECK);
			iBoxButton->SetHighlightColor(GREEN_WASH);			
			ReleaseICustButton(iBoxButton);

			CheckDlgButton(hWnd,IDC_NORMALIZEMAP_CHECK2,mod->fnGetNormalizeMap());

			
			ICustButton *iEditSeamsButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_EDITSEAMS));
			iEditSeamsButton->SetType(CBT_CHECK);
			iEditSeamsButton->SetHighlightColor(GREEN_WASH);
			ReleaseICustButton(iEditSeamsButton);


			ICustButton *iEditSeamsByPointButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_SEAMPOINTTOPOINT));
			iEditSeamsByPointButton->SetType(CBT_CHECK);
			iEditSeamsByPointButton->SetHighlightColor(GREEN_WASH);
			ReleaseICustButton(iEditSeamsByPointButton);



			ICustButton *iExpandSeamsButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_EXPANDTOSEAMS2));
			iExpandSeamsButton->SetType(CBT_PUSH);
			ReleaseICustButton(iExpandSeamsButton);


			mod->peltData.ShowUI(hWnd, FALSE);
			mod->EnableMapButtons(FALSE);
			mod->EnableAlignButtons(FALSE);

			mod->peltData.EnablePeltButtons(mod->hMapParams, FALSE);
			mod->peltData.SetPointToPointSeamsMode(mod,FALSE);

			
			int		align = mod->GetQMapAlign();
			CheckDlgButton(hWnd,IDC_RADIO1,FALSE);
			CheckDlgButton(hWnd,IDC_RADIO2,FALSE);
			CheckDlgButton(hWnd,IDC_RADIO3,FALSE);
			CheckDlgButton(hWnd,IDC_RADIO4,FALSE);
			if (align == 0)
				CheckDlgButton(hWnd,IDC_RADIO1,TRUE);
			else if (align == 1)
				CheckDlgButton(hWnd,IDC_RADIO2,TRUE);
			else if (align == 2)
				CheckDlgButton(hWnd,IDC_RADIO3,TRUE);
			else if (align == 3)
				CheckDlgButton(hWnd,IDC_RADIO4,TRUE);

			CheckDlgButton(hWnd,IDC_PREVIEW_CHECK,mod->GetQMapPreview());

			break;
			}



		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_PREVIEW_CHECK:

					macroRecorder->Disable();
					mod->SetQMapPreview(IsDlgButtonChecked(hWnd,IDC_PREVIEW_CHECK));
					macroRecorder->Enable();
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setQuickMapGizmoPreview"), 1, 0,
							mr_bool,mod->GetQMapPreview());

//					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setQuickMapGizmoPreview"), 0,1,
//							mr_int,mod->GetQMapPreview());
					if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
					break;
				case IDC_RADIO1:
				case IDC_RADIO2:
				case IDC_RADIO3:
				case IDC_RADIO4:
					{
						int		align = 0;
						if  (IsDlgButtonChecked(hWnd,IDC_RADIO1))
							align = 0;
						else if  (IsDlgButtonChecked(hWnd,IDC_RADIO2))
							align = 1;
						else if  (IsDlgButtonChecked(hWnd,IDC_RADIO3))
							align = 2;
						else if  (IsDlgButtonChecked(hWnd,IDC_RADIO4))
							align = 3;

						CheckDlgButton(hWnd,IDC_RADIO1,FALSE);
						CheckDlgButton(hWnd,IDC_RADIO2,FALSE);
						CheckDlgButton(hWnd,IDC_RADIO3,FALSE);
						CheckDlgButton(hWnd,IDC_RADIO4,FALSE);
						if (align == 0)
							CheckDlgButton(hWnd,IDC_RADIO1,TRUE);
						else if (align == 1)
							CheckDlgButton(hWnd,IDC_RADIO2,TRUE);
						else if (align == 2)
							CheckDlgButton(hWnd,IDC_RADIO3,TRUE);
						else if (align == 3)
							CheckDlgButton(hWnd,IDC_RADIO4,TRUE);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap.setProjectionType"), 1, 0,mr_int,align+1);

						macroRecorder->Disable();
						mod->SetQMapAlign(align);
						macroRecorder->Enable();
						if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
						break;
					}
				case IDC_QMAP:
					{
						mod->ApplyQMap();
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.quickPlanarMap"), 0, 0);
						
						break;
					}
				case IDC_UNWRAP_PELT2:
					{
						mod->WtExecute(ID_PELT_MAP);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingMode"), 1, 0,	mr_int,mod->fnGetMapMode());						
						break;
					}
				case IDC_UNWRAP_PLANAR:
					{
						mod->WtExecute(ID_PLANAR_MAP);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingMode"), 1, 0,	mr_int,mod->fnGetMapMode());						
						break;
					}

				case IDC_UNWRAP_CYLINDRICAL:
					{
						mod->WtExecute(ID_CYLINDRICAL_MAP);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingMode"), 1, 0,	mr_int,mod->fnGetMapMode());						
						break;
					}

				case IDC_UNWRAP_SPHERICAL:
					{
						mod->WtExecute(ID_SPHERICAL_MAP);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingMode"), 1, 0,	mr_int,mod->fnGetMapMode());						
						break;
					}

				case IDC_UNWRAP_BOX:
					{
						mod->WtExecute(ID_BOX_MAP);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingMode"), 1, 0,	mr_int,mod->fnGetMapMode());						
						break;
					}

				case IDC_UNWRAP_ALIGNX:
					{
 						mod->WtExecute(ID_MAPPING_ALIGNX);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingAlign"), 1, 0,	mr_int,0);						
					break;
					}
				case IDC_UNWRAP_ALIGNY:
					{
						mod->WtExecute(ID_MAPPING_ALIGNY);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingAlign"), 1, 0,	mr_int,1);						
					break;
					}
				case IDC_UNWRAP_ALIGNZ:
					{
						mod->WtExecute(ID_MAPPING_ALIGNZ);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingAlign"), 1, 0,	mr_int,2);						
					break;
					}
				case IDC_UNWRAP_ALIGNNORMAL:
					{
						mod->WtExecute(ID_MAPPING_NORMALALIGN);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingAlign"), 1, 0,	mr_int,3);						
					break;
					}

				case IDC_UNWRAP_FITMAP:
					{
						mod->WtExecute(ID_MAPPING_FIT);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingFit"), 0, 0);						
					break;
					}
				case IDC_UNWRAP_ALIGNTOVIEW:
					{
						mod->WtExecute(ID_MAPPING_ALIGNTOVIEW);
						Matrix3 macroTM = *mod->fnGetGizmoTM();
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setGizmoTM"), 1, 0,	mr_matrix3,&macroTM);						
					break;
					}
				case IDC_UNWRAP_CENTER:
					{
						mod->WtExecute(ID_MAPPING_CENTER);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingCenter"), 0, 0);						
					break;
					}
				case IDC_UNWRAP_RESET:
					{
						mod->WtExecute(ID_MAPPING_RESET);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.mappingReset"), 0, 0);						
					break;
					}

				case IDC_UNWRAP_EDITPELT:
					mod->WtExecute(ID_PELTDIALOG);
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialog"), 0, 0);
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialogSelectStretcher"), 0, 0);						

//					if (mod->peltData.GetPeltMapMode())
//						mod->fnPeltDialog();
					break;

				case IDC_NORMALIZEMAP_CHECK2:
					{
						//set element mode swtich 
						//					CheckDlgButton(hWnd,IDC_SELECTELEMENT_CHECK,mod->fnGetGeomElemMode());
						mod->fnSetNormalizeMap(IsDlgButtonChecked(hWnd,IDC_NORMALIZEMAP_CHECK2));
						//send macro message
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setNormalizeMap"), 1, 0,
							mr_bool,mod->fnGetNormalizeMap());
						macroRecorder->EmitScript();

						break;

					}

				case IDC_UNWRAP_EXPANDTOSEAMS2:
					{
					mod->WtExecute(ID_PELT_EXPANDSELTOSEAM);
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltExpandSelectionToSeams"), 0, 0);						
					break;
					}

				case IDC_UNWRAP_EDGESELTOSEAMS:
					{
						mod->WtExecute(ID_PW_SELTOSEAM2);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltEdgeSelToSeam"), 1, 0,	mr_bool,FALSE);												
						break;
					}

				case IDC_UNWRAP_EDITSEAMS:
					{
 					mod->WtExecute(ID_PELT_EDITSEAMS);
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltEditSeamsMode"), 1, 0,	mr_bool,mod->fnGetPeltEditSeamsMode());						

					break;
					}


				case IDC_UNWRAP_SEAMPOINTTOPOINT:
					{			
					mod->WtExecute(ID_PELT_POINTTOPOINTSEAMS);
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltPointToPointSeamsMode"), 1, 0,	mr_bool,mod->fnGetPeltPointToPointSeamsMode());						
					break;
					}


				}
			break;

		default:
			return FALSE;
		}
	return TRUE;
	}



	PeltData::~PeltData()
	{
		Free();
	}

	void PeltData::Free()
	{
//		for (int i = 0; i < faces.Count(); i++)
//		{
//		delete faces[i];
//		}
//		faces.ZeroCount();
		verts.ZeroCount();
		initialPointData.ZeroCount();
		springEdges.ZeroCount();
//		seams.ZeroCount();
	}

	void PeltData::PeltMode(UnwrapMod *mod, BOOL start)
	{
		if (inPeltMode == start) return;

		ICustButton *iPeltButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_PELT2));
		inPeltMode = start;
			
		if ((!inPeltMode) && peltDialoghWnd)
		{			
			SendMessage(peltDialoghWnd,WM_CLOSE,0,0);
		}
		if (!inPeltMode)
		{
			inPeltPickEdgeMode = FALSE;
			SetPointToPointSeamsMode(mod,FALSE);
//			inPeltPointToPointEdgeMode = FALSE;

			ICustButton *iEditSeamsButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_EDITSEAMS));
			ICustButton *iEditSeamsByPointButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_SEAMPOINTTOPOINT));

			if (iEditSeamsButton)
			{
			iEditSeamsButton->SetCheck(inPeltPickEdgeMode);
				ReleaseICustButton(iEditSeamsButton);
			}
			if (iEditSeamsByPointButton)
			{
			iEditSeamsByPointButton->SetCheck(inPeltPointToPointEdgeMode);
			ReleaseICustButton(iEditSeamsByPointButton);
		}
		}


		if (iPeltButton)
		{
		iPeltButton->SetCheck(inPeltMode);
		ShowUI(hMapParams, inPeltMode);
		ReleaseICustButton(iPeltButton);
		}
	
	}

	void PeltData::StartPeltDialog(UnwrapMod *mod)
	{

		
		HWND hWnd = mod->hMapParams;
		ICustButton *iEditSeamsButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_EDITSEAMS));
		ICustButton *iEditSeamsByPointButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_SEAMPOINTTOPOINT));
		mod->peltData.inPeltPickEdgeMode = FALSE;

		SetPointToPointSeamsMode(mod,FALSE);
//		mod->peltData.inPeltPointToPointEdgeMode = FALSE;

		iEditSeamsButton->SetCheck(mod->peltData.inPeltPickEdgeMode);
		iEditSeamsByPointButton->SetCheck(mod->peltData.inPeltPointToPointEdgeMode);
		
		iEditSeamsButton->Enable(FALSE);
		iEditSeamsByPointButton->Enable(FALSE);
		ReleaseICustButton(iEditSeamsButton);
		ReleaseICustButton(iEditSeamsByPointButton);

		ICustButton *iGetSeamsFromSelButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_EDGESELTOSEAMS));
		iGetSeamsFromSelButton->Enable(FALSE);
		ReleaseICustButton(iGetSeamsFromSelButton);


		ICustButton *iExpandToSeamsButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_EXPANDTOSEAMS2));
		iExpandToSeamsButton->Enable(FALSE);
		ReleaseICustButton(iExpandToSeamsButton);


	}
	void PeltData::EndPeltDialog(UnwrapMod *mod)
	{

		for (int i = 0; i < mod->vsel.GetSize(); i++)
		{
			if ((i < hiddenVerts.GetSize()) && (!hiddenVerts[i]))
			{
				mod->TVMaps.v[i].flags &= ~FLAG_HIDDEN;				
			}
		}
		//restore selection
		ModContextList mcList;		
		INodeTab nodes;

  		MeshTopoData *md =NULL;

		GetCOREInterface()->GetModContexts(mcList,nodes);

		int objects = mcList.Count();
		if (objects == 0) return;

 		md = (MeshTopoData*)mcList[0]->localData;

		int ns = mod->peltData.peltFaces.NumberSet();
		md->faceSel = mod->peltData.peltFaces;
		mod->SyncTVToGeomSelection(md);
		mod->SyncGeomToTVSelection(md);
		ns = md->faceSel.NumberSet();

		mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
		if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());

		HWND hWnd = mod->hMapParams;
		ICustButton *iEditSeamsButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_EDITSEAMS));
		ICustButton *iEditSeamsByPointButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_SEAMPOINTTOPOINT));
		mod->peltData.inPeltPickEdgeMode = FALSE;

		SetPointToPointSeamsMode(mod,FALSE);
//		mod->peltData.inPeltPointToPointEdgeMode = FALSE;

		iEditSeamsButton->SetCheck(mod->peltData.inPeltPickEdgeMode);
		iEditSeamsByPointButton->SetCheck(mod->peltData.inPeltPointToPointEdgeMode);
		iEditSeamsButton->Enable(TRUE);
		iEditSeamsByPointButton->Enable(TRUE);
		ReleaseICustButton(iEditSeamsButton);
		ReleaseICustButton(iEditSeamsByPointButton);

		ICustButton *iGetSeamsFromSelButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_EDGESELTOSEAMS));
		iGetSeamsFromSelButton->Enable(TRUE);
		ReleaseICustButton(iGetSeamsFromSelButton);


		ICustButton *iExpandToSeamsButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_EXPANDTOSEAMS2));
		iExpandToSeamsButton->Enable(TRUE);
		ReleaseICustButton(iExpandToSeamsButton);
		mod->peltData.SetPeltDialogHandle(NULL);


	}


	void PeltData::EnablePeltButtons(HWND hWnd, BOOL enable)
	{

		ICustButton *iEditSeamsButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_EDITSEAMS));
		ICustButton *iEditSeamsByPointButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_SEAMPOINTTOPOINT));

		if (!enable)
		{
			inPeltPickEdgeMode = FALSE;	
			
	}

		
		iEditSeamsButton->Enable(enable);
		iEditSeamsByPointButton->Enable(enable);
		ReleaseICustButton(iEditSeamsButton);
		ReleaseICustButton(iEditSeamsByPointButton);

		ICustButton *iGetSeamsFromSelButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_EDGESELTOSEAMS));
		iGetSeamsFromSelButton->Enable(enable);
		ReleaseICustButton(iGetSeamsFromSelButton);


		ICustButton *iExpandToSeamsButton = GetICustButton(GetDlgItem(hWnd, IDC_UNWRAP_EXPANDTOSEAMS2));
		iExpandToSeamsButton->Enable(enable);
		ReleaseICustButton(iExpandToSeamsButton);
	}

	void PeltData::ShowUI(HWND hWnd, BOOL show)
	{
		//get the edit button

		HWND hButton = GetDlgItem(hWnd, IDC_UNWRAP_EDITSEAMS);
		ShowWindow(hButton,TRUE);

		hButton = GetDlgItem(hWnd, IDC_UNWRAP_EDITPELT);
		ShowWindow(hButton,show);

		hButton = GetDlgItem(hWnd, IDC_UNWRAP_SEAMPOINTTOPOINT);
		ShowWindow(hButton,TRUE);

		hButton = GetDlgItem(hWnd, IDC_UNWRAP_EDGESELTOSEAMS);
		ShowWindow(hButton,TRUE);


		hButton = GetDlgItem(hWnd, IDC_UNWRAP_EXPANDTOSEAMS2);
		ShowWindow(hButton,TRUE);

		hButton = GetDlgItem(hParams, IDC_UNWRAP_RESET);
		EnableWindow(hButton,!show);

		hButton = GetDlgItem(hParams, IDC_UNWRAP_SAVE);
		EnableWindow(hButton,!show);

		hButton = GetDlgItem(hParams, IDC_UNWRAP_LOAD);
		EnableWindow(hButton,!show);

		hButton = GetDlgItem(hParams, IDC_MAP_CHAN1);
		EnableWindow(hButton,!show);

		hButton = GetDlgItem(hParams, IDC_MAP_CHAN2);
		EnableWindow(hButton,!show);

		hButton = GetDlgItem(hParams, IDC_MAP_CHAN);
		EnableWindow(hButton,!show);

		hButton = GetDlgItem(hParams, IDC_MAP_CHAN_SPIN);
		EnableWindow(hButton,!show);

	}

	void PeltData::NukeRig(UnwrapMod *mod )
	{
		for (int i = 0; i < rigPoints.Count(); i++)
		{
			int vIndex = rigPoints[i].lookupIndex;
			if ((vIndex >= 0) && (vIndex < mod->TVMaps.v.Count()))
			{
				mod->TVMaps.v[vIndex].flags |= FLAG_DEAD;
			}
		}
		verts.ZeroCount();
		springEdges.ZeroCount();
		rigPoints.ZeroCount();
	}

	void PeltData::SetRigStrength(float str)
	{
		rigStrength = str;
		for (int i = 0; i < rigPoints.Count(); i++)
		{
			int springIndex = rigPoints[i].springIndex;
			if ((springIndex >= 0) && (springIndex < springEdges.Count()))
			{
				springEdges[springIndex].str = rigStrength;
			}
		}
	}


	float PeltData::GetRigStrength()
	{
		return rigStrength;
	}

	void PeltData::SetSamples(int samp)
	{
		samples = samp;
	}
	int PeltData::GetSamples()
	{
		return samples;
	}
	void PeltData::SetFrames(int fr)
	{
		frames = fr;
	}
	int PeltData::GetFrames()
	{
		return frames;
	}

	void PeltData::SetStiffness(float stiff) { stiffness = stiff; }
	float PeltData::GetStiffness() { return stiffness; }

	void PeltData::SetDampening(float damp) { dampening = damp; }
	float PeltData::GetDampening() { return dampening; }

	void PeltData::SetDecay(float dec) { decay = dec; }
	float PeltData::GetDecay() { return decay; }

	void PeltData::SetMirrorAngle(float ang) { rigMirrorAngle = ang; }
	float PeltData::GetMirrorAngle() { return rigMirrorAngle; }
	
	Point3  PeltData::GetMirrorCenter() {return rigCenter;}

	BOOL PeltData::GetEditSeamsMode()
	{
		return inPeltPickEdgeMode;
	}
	void PeltData::SetEditSeamsMode(UnwrapMod *mod,BOOL mode)
	{
		if (inPeltPickEdgeMode == mode) return;

		inPeltPickEdgeMode = mode;

//		if (inPeltMode)
		{
			ICustButton *iEditSeamsButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_EDITSEAMS));
			ICustButton *iEditSeamsByPointButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_SEAMPOINTTOPOINT));
			if (!inPeltPickEdgeMode)
				inPeltPickEdgeMode = FALSE;
			else
			{
				inPeltPickEdgeMode = TRUE;
				SetPointToPointSeamsMode(mod,FALSE);
//				inPeltPointToPointEdgeMode = FALSE;

			}
			iEditSeamsButton->SetCheck(inPeltPickEdgeMode);
			iEditSeamsByPointButton->SetCheck(inPeltPointToPointEdgeMode);
			ReleaseICustButton(iEditSeamsButton);
			ReleaseICustButton(iEditSeamsByPointButton);
		}
	}

	BOOL PeltData::GetPointToPointSeamsMode()
	{
		return inPeltPointToPointEdgeMode;
	}
	void PeltData::SetPointToPointSeamsMode(UnwrapMod *mod,BOOL mode)
	{
//		if (mode == inPeltPointToPointEdgeMode) return;
		inPeltPointToPointEdgeMode = mode;
/*
		ICustButton *iEditSeamsButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_EDITSEAMS));
	 	ICustButton *iEditSeamsByPointButton = GetICustButton(GetDlgItem(hMapParams, IDC_UNWRAP_SEAMPOINTTOPOINT));
*/
		if (inPeltPointToPointEdgeMode)
		{
			SetEditSeamsMode(mod,FALSE);
		}
		currentPointHit = -1;
		previousPointHit = -1;
/*
		if (iEditSeamsButton)
		{
		iEditSeamsButton->SetCheck(GetEditSeamsMode());
			ReleaseICustButton(iEditSeamsButton);
		}
		if (iEditSeamsByPointButton)
		{
		iEditSeamsByPointButton->SetCheck(GetPointToPointSeamsMode());
		ReleaseICustButton(iEditSeamsByPointButton);
		}
*/
		if (inPeltPointToPointEdgeMode)
		{
			
			GetCOREInterface()->PushCommandMode(mod->peltPointToPointMode);
		}
		else
		{
			if (GetCOREInterface()->GetCommandMode() == mod->peltPointToPointMode)
			{
				if (GetCOREInterface()->GetCommandStackSize() == 1)
					GetCOREInterface()->SetStdCommandMode(CID_OBJMOVE);			
				else GetCOREInterface()->PopCommandMode();
			}
		}

	}


	BOOL PeltData::GetPeltMapMode()
	{
		return inPeltMode;
	}
	void PeltData::SetPeltMapMode(UnwrapMod *mod,BOOL mode)
	{
		if (mode == inPeltMode) return;
		PeltMode(mod,mode);
	}



	void PeltData::SnapRig(UnwrapMod *mod)
	{
		for (int i = 0; i < rigPoints.Count(); i++)
		{
			int selfIndex = rigPoints[i].lookupIndex;
			int targetIndex = rigPoints[i].index;
			if ((selfIndex >= 0) && (selfIndex < mod->TVMaps.v.Count()) && 
				(targetIndex >= 0) && (targetIndex < mod->TVMaps.v.Count()) )
			{
				Point3 targetP =  mod->TVMaps.v[targetIndex].p;
				mod->TVMaps.v[selfIndex].p = targetP;
				if (mod->TVMaps.cont[selfIndex]) mod->TVMaps.cont[selfIndex]->SetValue(0,&mod->TVMaps.v[selfIndex].p);

				
			}
		}
		mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
		mod->InvalidateView();
	
	}

	void PeltData::ValidateSeams(UnwrapMod *mod,BitArray &newSeams)
	{
		//loop through our spring list
		newSeams = seamEdges;
		int s = peltFaces.NumberSet();
		
		for (int i = 0; i < newSeams.GetSize(); i++)
		{
			if (newSeams[i])
			{
				if (i < mod->TVMaps.gePtrList.Count())
				{
					int faceCount = mod->TVMaps.gePtrList[i]->faceList.Count();
					BOOL sel = FALSE;
					for (int j = 0; j< faceCount; j++)
					{
		//make sure each spring edge is part of selection
						int faceIndex = mod->TVMaps.gePtrList[i]->faceList[j];
						if ((faceIndex < peltFaces.GetSize()) && peltFaces[faceIndex])
							sel = TRUE;
	
					}
					//if not remove it
					if (!sel)
					{
						newSeams.Set(i,FALSE);
					}
					//check to see if this is a degenerate seams
					if (mod->TVMaps.gePtrList[i]->a == mod->TVMaps.gePtrList[i]->b)
						newSeams.Set(i,FALSE);
				}
			}
		}

	}

	void PeltData::SetupSprings(UnwrapMod *mod )
	{
 		UVW_ChannelClass *tvData = &mod->TVMaps;
		tvData = &mod->TVMaps;
		//get our face selection

 		theHold.Begin();
		mod->HoldPointsAndFaces(true);	
		theHold.Accept("Run");

		ModContextList mcList;		
		INodeTab nodes;

		MeshTopoData *md =NULL;

		GetCOREInterface()->GetModContexts(mcList,nodes);

		int objects = mcList.Count();
		if (objects == 0) return;

		md = (MeshTopoData*)mcList[0]->localData;

		peltFaces = md->faceSel;

		for (int i = 0; i < tvData->f.Count(); i++)
		{

			if (peltFaces[i] )
				tvData->f[i]->flags |= FLAG_SELECTED;
			else tvData->f[i]->flags &= ~FLAG_SELECTED;
		}

		BitArray tempSeams; 
		ValidateSeams(mod,tempSeams);

		theHold.Suspend();
		//fix up x,y,z so they are square 

		Matrix3 tm(1);
		TimeValue t = GetCOREInterface()->GetTime();

		mod->tmControl->GetValue(t,&tm,FOREVER,CTRL_RELATIVE);

		if (Length(tm.GetRow(2)) == 0.0f)
		{
			tm.SetRow(2,Point3(0.0f,0.0f,1.0f));
		}


		float len = Length(tm.GetRow(0));

		tm.SetRow(1,Normalize(tm.GetRow(1))*len);
		tm.SetRow(2,Normalize(tm.GetRow(2))*len);


/*  		tm.SetRow(0,Point3(0.0f,-scale.y,0.0f));
		tm.SetRow(1,Point3(0.0f,0.0f,scale.z));
		tm.SetRow(2,Point3(scale.x,0.0f,0.0f));
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == PELTMAP)  || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			tm.SetRow(3,center);
		else if (fnGetMapMode() == CYLINDRICALMAP)
		{
			center.x = bounds.pmin.x;
			tm.SetRow(3,center);
		}		
*/

		Matrix3 ptm(1), id(1);
		SetXFormPacket tmpck(tm,ptm);
		mod->tmControl->SetValue(t,&tmpck,TRUE,CTRL_RELATIVE);

		mod->fnPlanarMap();
		theHold.Resume();
		
 		tvData->edgesValid = FALSE;
 		mod->RebuildEdges();

 		CutSeams(mod,tempSeams);
		tvData->edgesValid = FALSE;
		
		mod->RebuildEdges();




		for (int i = 0; i < tvData->f.Count(); i++)
		{
			if (peltFaces[i] )
				tvData->f[i]->flags |= FLAG_SELECTED;
			else tvData->f[i]->flags &= ~FLAG_SELECTED;
		}


		

//		DebugPrint("initial sel count %d\n",peltFaces.NumberSet());
 		
		
		//get our fsel asnd transfer the selection






		for (int i = 0; i < tvData->v.Count(); i++)
		{			
			if (tvData->cont[i]) 
				tvData->cont[i]->SetValue(0,&tvData->v[i].p);
		}


		
//get our face selection
//for now we are building all of them
		//copy our geom data to the uvw data
		Tab<int> uvToGeom;
		uvToGeom.SetCount(tvData->v.Count());
		Box3 bounds;
		bounds.Init();
		springEdges.ZeroCount();

		verts.SetCount(tvData->v.Count());
		for (int i = 0; i < tvData->v.Count(); i++)
		{
			verts[i].pos = tvData->v[i].p;
			verts[i].vel = Point3(0.0f,0.0f,0.0f);
		}

		bounds.Init();
		for (int i = 0; i < tvData->f.Count(); i++)
		{
			if ((!(tvData->f[i]->flags & FLAG_DEAD)) && (tvData->f[i]->flags & FLAG_SELECTED))
			{
				int deg = tvData->f[i]->count;
				for (int j = 0; j < deg; j++)
				{
					int tvA = tvData->f[i]->t[j];
					bounds += tvData->v[tvA].p;
				}
			}
		}

		Point3 center = bounds.Center();
//build our spring list
		for (int i = 0; i < tvData->ePtrList.Count(); i++)
		{
		//loop the through the edges
			int a = tvData->ePtrList[i]->a;
			int b = tvData->ePtrList[i]->b;
			int veca = tvData->ePtrList[i]->avec;
			int vecb = tvData->ePtrList[i]->bvec;
			BOOL isHidden = tvData->ePtrList[i]->flags & FLAG_HIDDENEDGEA;

			BOOL faceSelected = FALSE;
			for (int j = 0; j < tvData->ePtrList[i]->faceList.Count(); j++)
			{
				int faceIndex = tvData->ePtrList[i]->faceList[j];
				if ((!(tvData->f[faceIndex]->flags & FLAG_DEAD)) && (tvData->f[faceIndex]->flags & FLAG_SELECTED))
					faceSelected = TRUE;
			}

			if (faceSelected)
			{
				EdgeBondage sp;
				sp.v1 = a;
				sp.v2 = b;
				sp.vec1 = veca;
				sp.vec2 = vecb;
				float dist = Length(tvData->v[a].p-tvData->v[b].p);
				sp.dist = dist;
				sp.str = 1.0f;
				sp.distPer = 1.0f;
				sp.isEdge = FALSE;
				sp.edgeIndex = i;
				springEdges.Append(1,&sp,5000);
				
			//add a spring for each edge
			//if edge is not visible find cross edge
				if ((isHidden) && (tvData->ePtrList[i]->faceList.Count() > 1))
				{
					//get face 1
					int a1,b1;
					a1 = -1;
					b1 = -1;

					int faceIndex = tvData->ePtrList[i]->faceList[0];

					if ((!(tvData->f[faceIndex]->flags & FLAG_DEAD)) && (tvData->f[faceIndex]->flags & FLAG_SELECTED))
					{
						int deg = tvData->f[faceIndex]->count;
						for (int j = 0; j < deg; j++)
						{
							int tvA = tvData->f[faceIndex]->t[j];
							if ((tvA != a) && (tvA != b))
								a1 = tvA;

						}
					
						//get face 2
						faceIndex = tvData->ePtrList[i]->faceList[1];
						deg = tvData->f[faceIndex]->count;
						for (int j = 0; j < deg; j++)
						{
							int tvA = tvData->f[faceIndex]->t[j];
							if ((tvA != a) && (tvA != b))
								b1 = tvA;
						}

						if ((a1 != -1) && (b1 != -1))
						{
							EdgeBondage sp;
							sp.v1 = a1;
							sp.v2 = b1;
							sp.vec1 = -1;
							sp.vec2 = -1;
							float dist = Length(tvData->v[a1].p-tvData->v[b1].p);
							sp.dist = dist;
							sp.str = 1.0f;
							sp.distPer = 1.0f;
							sp.isEdge = FALSE;
							sp.edgeIndex = -1;
							springEdges.Append(1,&sp,5000);
						}
					}
				}
			}
		}

//build our initial rig
		
		//find our edge verts
		//loop through our seams
		//build our outeredge list
		//check for multiple holes
		BitArray masterSeamList;
		masterSeamList.SetSize(tvData->ePtrList.Count());
		masterSeamList.ClearAll();




		BitArray tempEdges;
		tempEdges.SetSize(tvData->ePtrList.Count());
		tempEdges.ClearAll();

		Tab<int> edgeVerts;
 		float d = 0;
		for (int i = 0; i <  tvData->ePtrList.Count(); i++)
		{
			Point3 a,b;
			int ia,ib;
			if (tvData->ePtrList[i]->faceList.Count() == 1)
			{
				int faceIndex = tvData->ePtrList[i]->faceList[0];

				if ((!(tvData->f[faceIndex]->flags & FLAG_DEAD)) && (tvData->f[faceIndex]->flags & FLAG_SELECTED))
				{
					ia = tvData->ePtrList[i]->a;
					ib = tvData->ePtrList[i]->b;
					if (ia != ib)
					{
						a = tvData->v[ia].p;
						b = tvData->v[ib].p;
						d += Length(a-b);
						edgeVerts.Append(1,&i,400);
						tempEdges.Set(i,TRUE);
					}
				}
			}
		}





//		Tab<float> edgeAngle;
//		edgeAngle.SetCount(edgeVerts.Count());

		Tab<int> vConnects;
		vConnects.SetCount(tvData->v.Count());
		for (int i = 0; i < tvData->v.Count(); i++)
			vConnects[i] = -1;
		
		//build our connection list
		float sum = 0.0f;
		BitArray tempVerts;
		tempVerts.SetSize(tvData->v.Count());
		tempVerts.ClearAll();

		float rigSize = 0.0f;
		for (int i = 0; i < edgeVerts.Count(); i++)
		{
			Point3 a,b;
			int ia,ib;
//			edgeAngle[i] = 0.0f;
			int edgeIndex = edgeVerts[i];
			ia = tvData->ePtrList[edgeIndex]->a;
			ib = tvData->ePtrList[edgeIndex]->b;
			a = tvData->v[ia].p;
			b = tvData->v[ib].p;
			//only connect the clowckwise vert
			int faceIndex = tvData->ePtrList[edgeIndex]->faceList[0];
			int deg = tvData->f[faceIndex]->count;
			for (int j = 0; j < deg; j++)
			{
				int fa = tvData->f[faceIndex]->t[j];
				int fb = tvData->f[faceIndex]->t[(j+1) % deg];
				if (fa != fb)
				{
					if ((fa == ia) && (fb == ib))
					{
						vConnects[fa] = fb;
						tempVerts.Set(fa,TRUE);
					}
					else if ((fa == ib) && (fb == ia))
					{
						vConnects[fa] = fb;
						tempVerts.Set(fa,TRUE);
					}
				}
			}
			float l = Length(a-b);
			float per = l/d;
			sum += per;
			float angle = PI * 2.0f * per;

			if (Length(center-a) > rigSize ) rigSize = Length(center-a);
			if (Length(center-b) > rigSize ) rigSize = Length(center-b);
//			edgeAngle[i] = angle;			
		}


		Tab<int> seamStarts;
		Tab<float> seamDistance;
		while (tempVerts.NumberSet()!= 0)
		{
			//find a seed vert
			int seedVert = -1;
			for (int i = 0; i < vConnects.Count(); i++)
			{
				if (tempVerts[i])
				{
					seedVert = i;
					i = vConnects.Count();
				}
			}
			if (seedVert != -1)
			{
				int currentVert = seedVert;
				tempVerts.Set(currentVert,FALSE);

				float d = 0.0f;
				int prevVert = currentVert;
				int nextVert = -1;
				//loop through its connections getting distance 
				BOOL bail = FALSE;
				while ((nextVert != seedVert) && (!bail))
				{
					prevVert = currentVert;
					currentVert = vConnects[currentVert];
					if (currentVert == -1)
						bail = TRUE;
					else if (!tempVerts[currentVert])
						bail = TRUE;
					else
					{

						tempVerts.Set(currentVert,FALSE);
						Point3 a, b;
						a = tvData->v[currentVert].p;
						b = tvData->v[prevVert].p;
						d += Length(a-b);					
						nextVert = currentVert;
					}
				}
				Point3 a, b;
				a = tvData->v[currentVert].p;
				b = tvData->v[nextVert].p;
				d += Length(a-b);					
				nextVert = currentVert;
				
				seamStarts.Append(1,&seedVert);
				seamDistance.Append(1,&d);
			}
		}

		//find the longest d
		int seed = -1;
		float seedDistance = 0.0f;
		for (int i = 0; i < seamDistance.Count(); i++)
		{
			int id = seamStarts[i];
			float td = seamDistance[i];
			if ((seed == -1) || (seamDistance[i] > seedDistance))
			{
				seed = seamStarts[i];
				seedDistance = seamDistance[i];
			}
		}
		
		d = seedDistance;


		//build the rig
		float currentAngle = 0.0f;
		float r = 2.0f;
		BOOL done = FALSE;
		if (edgeVerts.Count() == 0) return;
		int currentVert = seed;//tvData->ePtrList[edgeVerts[0]]->a;
		
		BOOL first = TRUE;
		Point3 v = Point3(0.0f,1.0f,0.0f);
		int fid = -1;

		BitArray processedVerts;
		processedVerts.SetSize(tvData->v.Count());
		processedVerts.ClearAll();

		int startVert = -1;
		Point3 initialVec = Point3(0.0f,0.0f,0.0f);
		rigCenter = center;
		while (!done)
		{

			int startCount = processedVerts.NumberSet();

			processedVerts.Set(currentVert,TRUE);

			
			int nextVertA = vConnects[currentVert];

			int prevVert = currentVert;
			currentVert = nextVertA;
			//need to make sure we are going the right direction

			Point3 a = tvData->v[prevVert].p;
			Point3 b = tvData->v[currentVert].p;

			if (startVert != prevVert)
			{
				float l = Length(a-b);
				float per = l/d;
				float angle = PI * 2.0f * per;
				

				//rotate our current
 				Matrix3 rtm(1);
  				rtm.RotateZ(currentAngle);
				if (first)
				{
					first = FALSE;
					fid = prevVert;
					startVert = prevVert;
					Point3 a = tvData->v[fid].p;
					a.z = 0.0f;
					Point3 b = center;
					b.z = 0.0f;

					initialVec = Normalize(a-b);
					
				}
				v = initialVec;
				v = v * rtm;

				RigPoint rp;
				
				rp.p = v * rigSize * 1.5f+center;//Point3(0.5f,0.5f,0.0f);
				rp.d = Length(rp.p - tvData->v[prevVert].p);
				rp.index = prevVert;
				rp.neighbor = currentVert;
				rp.elen = l;
				rp.springIndex = springEdges.Count();
				rp.angle = currentAngle;
				
				EdgeBondage sp;
				sp.v1 = prevVert;

				int found = -1;
				BOOL append = TRUE;
				for (int m= 0; m <tvData->v.Count();m++)
				{
					if (tvData->v[m].flags & FLAG_DEAD)
						{
						found =m;
						append = FALSE;
						m = tvData->v.Count();
						}
				}
				int newVertIndex = found;
				if (found == -1)
					newVertIndex = tvData->v.Count();
				

				UVW_TVVertClass newVert;
				newVert.p =rp.p;
				newVert.influence = 0.0f;
				newVert.flags = 0;
				rp.lookupIndex = newVertIndex;
				if (append)
				{
					tvData->v.Append(1,&newVert);
					Control* c;
					c = NULL;
					tvData->cont.Append(1,&c,1);
				}
				else
				{
					tvData->v[newVertIndex].p = newVert.p;
					tvData->v[newVertIndex].influence = newVert.influence;
					tvData->v[newVertIndex].flags = newVert.flags;
					if (tvData->cont[newVertIndex]) 
						tvData->cont[newVertIndex]->SetValue(0,&tvData->v[newVertIndex].p);
				}

				rigPoints.Append(1,&rp,5000);

				SpringClass vd;
				vd.pos = tvData->v[newVertIndex].p;
				vd.vel = Point3(0.0f,0.0f,0.0f);
				vd.weight = 1.0f;

				if (append)
					verts.Append(1,&vd,5000);
				else
				{
					verts[newVertIndex] = vd;
				}


 				sp.v2 = newVertIndex;
 				sp.vec1 = -1;
				sp.vec2 = -1;
	//build our rig springs			
				sp.dist = rp.d * 0.1f;
//				sp.dist = rigSize*0.1f;
				sp.str = rigStrength;
				sp.edgePos = rp.p;
				sp.edgeIndex = -1;
				sp.originalVertPos = tvData->v[prevVert].p;
				
				sp.distPer = 1.0f;
				sp.isEdge = TRUE;
 				springEdges.Append(1,&sp,5000);
//	DebugPrint("Vert %d TAngle %f CAngle %f \n",prevVert,currentAngle*180.0f/PI,angle*180.0f/PI);
				currentAngle += angle;
			}
			else done = TRUE;

			
 			int endCount = processedVerts.NumberSet();
	
			if (startCount == endCount) 
				done = TRUE;
			if (currentVert == -1)
				done = TRUE;

		}


		initialPointData.SetCount(verts.Count());
		for (int i=0;i<verts.Count();i++)
		{
			verts[i].vel = Point3(0.0f,0.0f,0.0f);
			for (int j = 0; j < 6; j++)
			{
				verts[i].tempVel[j] = Point3(0.0f,0.0f,0.0f);
				verts[i].tempPos[j] = Point3(0.0f,0.0f,0.0f);
			}
			initialPointData[i] = tvData->v[i].p;

			
		}

		mod->RebuildEdges();

		mod->vsel.SetSize(mod->TVMaps.v.Count());
		
		md->faceSel = peltFaces;
		mod->SyncTVToGeomSelection(md);
		tvData->edgesValid = TRUE;

		mod->vsel.SetSize(tvData->v.Count());
		mod->vsel.ClearAll();
		mod->RebuildDistCache();

		SelectRig(mod);
		SelectPelt(mod);

		hiddenVerts = mod->vsel;
		for (int i = 0; i < mod->vsel.GetSize(); i++)
		{
			if (!hiddenVerts[i])
			{
				mod->TVMaps.v[i].flags |= FLAG_HIDDEN;				
			}
		}
		

 		mod->vsel.ClearAll();
		for (int i = 0; i < rigPoints.Count(); i++)
		{
			int index = rigPoints[i].lookupIndex;
			mod->vsel.Set(index,TRUE);
		}

		if (seamEdges.GetSize() != mod->TVMaps.gePtrList.Count())
		{
			seamEdges.SetSize(mod->TVMaps.gePtrList.Count());
			seamEdges.ClearAll();
		}
		for (int i = 0; i < mod->TVMaps.gePtrList.Count(); i++)
		{
				int numberSelected = 0;
				int ct = mod->TVMaps.gePtrList[i]->faceList.Count();
				
				for (int j = 0; j < ct; j++)
				{
					int faceIndex = mod->TVMaps.gePtrList[i]->faceList[j];
					//draw this edge
					if (mod->TVMaps.f[faceIndex]->flags & FLAG_SELECTED)
					{
						numberSelected++;
					}
				}
				if ((numberSelected==1) )
				{
					seamEdges.Set(i,TRUE);
				}
		}

	}

void PeltData::SelectRig(UnwrapMod *mod, BOOL replace)
{
	
	//loopt through the rigs
	if (replace) mod->vsel.ClearAll();
	for (int i = 0; i < rigPoints.Count(); i++)
	{
		int index = rigPoints[i].lookupIndex;
		if ((index >= 0) && (index < mod->vsel.GetSize()))
			mod->vsel.Set(index,TRUE);
	}

	mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	mod->InvalidateView();

	if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());


}
void PeltData::SelectPelt(UnwrapMod *mod, BOOL replace)
{

	if (replace) mod->vsel.ClearAll();
	//loopt through the rigs
	BitArray rigPointsSel;
	rigPointsSel.SetSize(mod->vsel.GetSize());
	rigPointsSel.ClearAll();

	for (int i = 0; i < rigPoints.Count(); i++)
	{
		int index = rigPoints[i].lookupIndex;
		rigPointsSel.Set(index,TRUE);
	}
	for (int i = 0; i < springEdges.Count(); i++)
	{
		int index = springEdges[i].v1;
		if (index >= 0)
		{
			if ( (index < rigPointsSel.GetSize()) && (index < mod->vsel.GetSize()))
			{
				if (!rigPointsSel[index])
					mod->vsel.Set(index,TRUE);
			}
		}

		index = springEdges[i].v2;
		if (index >= 0)
		{
			if ( (index < rigPointsSel.GetSize()) && (index < mod->vsel.GetSize()) )
			{
				if (!rigPointsSel[index])
					mod->vsel.Set(index,TRUE);
			}
		}
	}


	mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	mod->InvalidateView();
	if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
}


void PeltData::ResetRig(UnwrapMod *mod)
{
	theHold.Begin();
	mod->HoldPointsAndFaces();	
	theHold.Accept(GetString(IDS_PELTDIALOG_RESETRIG));

	for (int i=0;i<verts.Count();i++)
	{
		verts[i].vel = Point3(0.0f,0.0f,0.0f);
		for (int j = 0; j < 6; j++)
		{
			verts[i].tempVel[j] = Point3(0.0f,0.0f,0.0f);
			verts[i].tempPos[j] = Point3(0.0f,0.0f,0.0f);
		}			
	}
	for (int i = 0; i < springEdges.Count(); i++)
	{
		int index = springEdges[i].v1;
		if (index >= 0)
		{
			if (( index < mod->TVMaps.v.Count()) && (index < initialPointData.Count()))
			{
				mod->TVMaps.v[index].p = initialPointData[index];
			}
		}

		index = springEdges[i].v2;
		if (index >= 0)
		{
			if (( index < mod->TVMaps.v.Count()) && (index < initialPointData.Count()))
			{
				mod->TVMaps.v[index].p = initialPointData[index];
			}
		}

		index = springEdges[i].vec1;
		if (index >= 0)
		{
			if (( index < mod->TVMaps.v.Count()) && (index < initialPointData.Count()))
			{
				mod->TVMaps.v[index].p = initialPointData[index];
			}
		}

		index = springEdges[i].vec2;
		if (index >= 0)
		{
			if (( index < mod->TVMaps.v.Count()) && (index < initialPointData.Count()))
			{
				mod->TVMaps.v[index].p = initialPointData[index];
			}
		}

	}
	mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	mod->InvalidateView();
	if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());
}

void PeltData::RunRelax(UnwrapMod *mod, int relaxLevel)
{
	float  holdStr = rigStrength;
	int holdFrames = GetFrames();
	int holdSamples = GetSamples();
	
	if (relaxLevel == 0)
		SetRigStrength(0.005f);
	else if (relaxLevel == 1)
		SetRigStrength(0.0005f);

	SetSamples(5);
	SetFrames(5);

	Run(mod);
	SetRigStrength(holdStr);

	SetSamples(holdSamples);
	SetFrames(holdFrames);

}



void PeltData::Run(UnwrapMod *mod)
{



	UVW_ChannelClass *tvData = &mod->TVMaps;
	//see if we have selected verts
	BOOL selectedVerts = FALSE;

	for (int i = 0; i < tvData->f.Count(); i++)
	{
		int deg = tvData->f[i]->count;
		for (int j = 0; j < deg; j++)
		{

			int id = tvData->f[i]->t[j];
			if (mod->vsel[id])
			{
				selectedVerts = TRUE;
			}
			else if (tvData->v[id].influence > 0.0f)
			{
				selectedVerts = TRUE;
			}
		}
	}

	//update our vertex data pos/vel
 	for (int j = 0; j < verts.Count(); j++)
		{
			Point3 p = tvData->v[j].p;		
			verts[j].pos = p;
			if (!selectedVerts)
				verts[j].weight = 1.0f;

			else
			{
				verts[j].weight = 0.0f;
				if (mod->vsel[j])
					verts[j].weight = 1.0f;					
				else verts[j].weight = tvData->v[j].influence;
					selectedVerts = TRUE;
			}

			if (!mod->IsVertVisible(j))
				verts[j].weight = 0.0f;			
			
		}

	Solver solver;

	
	//create our solver
	
 	Tab<EdgeBondage> tempSpringEdges;
	for (int i = 0; i < springEdges.Count(); i++)
	{
		//copy rig springs
		if (springEdges[i].isEdge)
		{
			tempSpringEdges.Append(1,&springEdges[i],10000);
		}
		else
		{
			int a = springEdges[i].v1;
			int b = springEdges[i].v2;
			if (mod->fnGetLockSpringEdges())
			{
				if ((verts[a].weight > 0.0f) || (verts[b].weight > 0.0f))
					tempSpringEdges.Append(1,&springEdges[i],10000);
			}
			else
			{
				if ((verts[a].weight > 0.0f) && (verts[b].weight > 0.0f))
					tempSpringEdges.Append(1,&springEdges[i],10000);
			}

		}
	}
	//solve
	solver.Solve(0, frames, samples,
				tempSpringEdges, verts,
			   GetStiffness(),GetDampening(),GetDecay(),mod);

	
	if (mod)
	{
	//put the data back
			TimeValue t = GetCOREInterface()->GetTime();

			UVW_ChannelClass *tvData = &mod->TVMaps;
			for (int j = 0; j < verts.Count(); j++)
			{
					Point3 p = verts[j].pos	;		
					
					tvData->v[j].p = p;
					if (tvData->cont[j]) tvData->cont[j]->SetValue(t,&tvData->v[j].p);
					
			}
			ResolvePatchHandles(mod);
			mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
			mod->InvalidateView();
			if (mod->ip) mod->ip->RedrawViews(t);

	}

	
}


void PeltData::CutSeams(UnwrapMod *mod, BitArray seams)
{
 	
 	UVW_ChannelClass *tvData = &mod->TVMaps;

	//loop through our seams and remove any that are open edges

	for (int i = 0; i < seams.GetSize(); i++)
	{
		if (seams[i])
		{
			if (tvData->gePtrList[i]->faceList.Count() == 1)
			{
				seams.Set(i,FALSE);
			}
		}
	}
	

	
	Tab<int> geomToUVVerts;
	geomToUVVerts.SetCount(tvData->geomPoints.Count());
	for (int i = 0; i < tvData->geomPoints.Count(); i++)
		geomToUVVerts[i] = -1;


	for (int i = 0; i < tvData->f.Count(); i++)
	{
		if (tvData->f[i]->flags & FLAG_SELECTED)
		{
			int deg = tvData->f[i]->count;
			for (int j = 0; j < deg; j++)
			{
				int tvA = tvData->f[i]->t[j];
				int geoA = tvData->f[i]->v[j];
				geomToUVVerts[geoA] = tvA;
			}
		}
	}
	for (int i = 0; i < tvData->ePtrList.Count(); i++)
	{
		tvData->ePtrList[i]->lookupIndex = i;
	}
	//convert our geom edges to uv edges
	BitArray newUVEdgeSel;
	newUVEdgeSel.SetSize(tvData->ePtrList.Count());
	newUVEdgeSel.ClearAll();

	int numberSeams = seams.NumberSet();

	for (int i = 0; i < seams.GetSize(); i++)
	{
		if (seams[i])
		{
			int ga = tvData->gePtrList[i]->a;
			int gb = tvData->gePtrList[i]->b;
			//convert to uv indices
			int uvA = geomToUVVerts[ga];
			int uvB = geomToUVVerts[gb];
			//find matching UV edge 
			int uvEdge = -1;
			int ct = tvData->e[uvA]->data.Count();
			for (int j = 0; j < ct; j++)
			{
				int matchA = tvData->e[uvA]->data[j]->a;
				int matchB = tvData->e[uvA]->data[j]->b;
				if ( ((matchA == uvA) && (matchB == uvB)) ||
					 ((matchA == uvB) && (matchB == uvA)) )
				{
					BOOL selected = FALSE;

					uvEdge = tvData->e[uvA]->data[j]->lookupIndex;
				}
			}
			if (uvEdge != -1)
			{
				ct = tvData->e[uvB]->data.Count();
				for (int j = 0; j < ct; j++)
				{
					int matchA = tvData->e[uvB]->data[j]->a;
					int matchB = tvData->e[uvB]->data[j]->b;
					if ( ((matchA == uvA) && (matchB == uvB)) ||
						((matchA == uvB) && (matchB == uvA)) )
					{
						uvEdge = tvData->e[uvB]->data[j]->lookupIndex;
					}
				}
			}
			if (uvEdge != -1) 
			{
				newUVEdgeSel.Set(uvEdge,TRUE);
			}
			else
			{
				DebugPrint("Miss\n");
			}
		}
	}

	//do a  break now


	mod->TVMaps.SplitUVEdges(newUVEdgeSel);
	

	mod->esel.SetSize(mod->TVMaps.ePtrList.Count());
	mod->esel.ClearAll();
	


	
	

}


INT_PTR CALLBACK UnwrapPeltFloaterDlgProc(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);
	//POINTS p = MAKEPOINTS(lParam);	commented out by sca 10/7/98 -- causing warning since unused.
	

	switch (msg) {
		case WM_INITDIALOG:
			{


			mod = (UnwrapMod*)lParam;

			DLSetWindowLongPtr(hWnd, lParam);


			mod->peltData.peltDialog.SetUpDialog(hWnd);
			mod->peltData.SetPeltDialogHandle(hWnd);
			mod->peltData.StartPeltDialog(mod);
			
			CheckDlgButton(hWnd,IDC_LOCKOPENEDGESCHECK,mod->fnGetLockSpringEdges());

			
//			mod->peltData.inPeltMode = TRUE;

			break;
			}

		case CC_SPINNER_CHANGE:
			mod->peltData.peltDialog.SpinnerChange(mod->peltData,LOWORD(wParam));
			mod->InvalidateView();
			break;
		case WM_DESTROY:
 			mod->peltData.peltDialog.DestroyDialog(hWnd);
			mod->peltData.EndPeltDialog(mod);
			mod->peltData.NukeRig(mod );
			mod->peltData.SetPeltDialogHandle( NULL);
			break;
		case WM_CLOSE:
 			mod->peltData.peltDialog.DestroyDialog(hWnd);
			mod->peltData.EndPeltDialog(mod);

			mod->peltData.NukeRig(mod );
			if (!mod->peltData.IsSubObjectUpdateSuspended())
				mod->fnSetTVSubMode(TVFACEMODE);

			mod->InvalidateView();
			EndDialog(hWnd,1);
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialog"), 0, 0);
			mod->peltData.SetPeltDialogHandle( NULL);

			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_LOCKOPENEDGESCHECK:
					{
						//set element mode swtich 
						//					CheckDlgButton(hWnd,IDC_SELECTELEMENT_CHECK,mod->fnGetGeomElemMode());
						mod->fnSetLockSpringEdges(IsDlgButtonChecked(hWnd,IDC_LOCKOPENEDGESCHECK));
						//send macro message
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltLockOpenEdges"), 1, 0,
							mr_bool,mod->fnGetLockSpringEdges());
						macroRecorder->EmitScript();

						break;

					}
				case IDC_RELAX1_BUTTON:
					{
						mod->WtExecute(ID_PELTDIALOG_RELAX1);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialogRelaxLight"), 0, 0);						
						macroRecorder->EmitScript();

//					mod->fnPeltDialogRelax1();
					break;
					}
				case IDC_RELAX2_BUTTON:
					{
						mod->WtExecute(ID_PELTDIALOG_RELAX2);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialogRelaxHeavy"), 0, 0);						
						macroRecorder->EmitScript();
//					mod->fnPeltDialogRelax2();
					break;
					}
				case IDC_RUN_BUTTON:
					{
						mod->WtExecute(ID_PELTDIALOG_RUN);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialogRun"), 0, 0);						
						macroRecorder->EmitScript();
//					mod->fnPeltDialogRun();
					break;
					}
				case IDC_SELECTRIG_BUTTON:
					{
						mod->WtExecute(ID_PELTDIALOG_SELECTRIG);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialogSelectStretcher"), 0, 0);						

//					mod->fnPeltDialogSelectRig();
					break;
					}
				case IDC_SELECTPELT_BUTTON:
					{
						mod->WtExecute(ID_PELTDIALOG_SELECTPELT);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialogSelectPelt"), 0, 0);						

//					mod->fnPeltDialogSelectPelt();
					break;
					}
				case IDC_SNAPRIG_BUTTON:
					{
						mod->WtExecute(ID_PELTDIALOG_SNAPRIG);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialogSnapToSeams"), 0, 0);						

//						mod->fnPeltDialogSnapRig();
						break;
					}
				case IDC_RESETRIG_BUTTON:
					{
						mod->WtExecute(ID_PELTDIALOG_RESETRIG);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialogResetStretcher"), 0, 0);						
//						mod->fnPeltDialogResetRig();
						break;
					}
				case IDC_MIRRORRIG_BUTTON:
					{
						mod->WtExecute(ID_PELTDIALOG_MIRRORRIG);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialogMirrorStretcher"), 0, 0);						

//						mod->fnPeltDialogMirrorRig();
						break;
					}

				case IDC_STRAIGHTEN_BUTTON2:
					{
						mod->WtExecute(ID_PELTDIALOG_STRAIGHTENSEAMS);
						macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltDialogStraightenMode"), 1, 0,mr_bool,mod->peltData.peltDialog.GetStraightenMode());					
/*						if (mod->peltData.peltDialog.GetStraightenMode())
							mod->peltData.peltDialog.SetStraightenMode(FALSE);
						else mod->peltData.peltDialog.SetStraightenMode(TRUE);											
*/
						break;
					}


/*
				case IDC_APPLYOK:
					{

					 EndDialog(hWnd,1);
					
					break;
					}

				case IDCANCEL:
					{

					EndDialog(hWnd,0);
					break;
					}
*/
				}
			break;

		default:
			return FALSE;
		}
	return TRUE;
	}


void UnwrapMod::fnPeltDialog()
{

 	if ((GetPeltMapMode())&&(peltData.peltDialog.hWnd == NULL))
	{
		
		fnEdit();
		peltData.peltDialog.mod = this;
		peltData.peltDialog.hWnd = CreateDialogParam(	hInstance,
							MAKEINTRESOURCE(IDD_UNWRAP_PELTDIALOG),
							hWnd,
							UnwrapPeltFloaterDlgProc,
							(LPARAM)this );
		ShowWindow(peltData.peltDialog.hWnd ,SW_SHOW);

		peltData.SetupSprings(this);

		int oCount = vsel.GetSize();
		peltData.startPoint = oCount;
		fnSetTVSubMode(TVVERTMODE);
		TVMaps.edgesValid= TRUE;
		fnFit();
		NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);

	}
	else
	{
		SendMessage(peltData.peltDialog.hWnd,WM_CLOSE,0,0);
	}
/*
	for (int i = oCount; i < TVMaps.v.Count(); i++)
	{
		vsel.Set(i,TRUE);
		Control* c;
		c = NULL;
		TVMaps.cont.Append(1,&c,1);
	}
*/


}


void PeltDialog::SetUpDialog(HWND hWnd)
{



	iSpinRigStrength = SetupFloatSpinner(
				hWnd,IDC_PELT_RIGSTRENGTHSPIN,IDC_PELT_RIGSTRENGTH,
				0.0f,0.5f,mod->peltData.GetRigStrength());	
	iSpinRigStrength->SetScale(0.1f);

	iSpinSamples = SetupIntSpinner(
				hWnd,IDC_PELT_SAMPLESSPIN,IDC_PELT_SAMPLES,
				1,50,mod->peltData.GetSamples());	
	iSpinSamples->SetScale(1.0f);

	iSpinFrames = SetupIntSpinner(
				hWnd,IDC_PELT_FRAMESSPIN,IDC_PELT_FRAMES,
				1,100,mod->peltData.GetFrames());	
	iSpinFrames->SetScale(1.0f);

	
	iSpinStiffness = SetupFloatSpinner(
				hWnd,IDC_PELT_STIFFNESSSPIN,IDC_PELT_STIFFNESS,
				0.0f,0.5f,mod->peltData.GetStiffness());	
	iSpinStiffness->SetScale(0.1f);

	iSpinDampening = SetupFloatSpinner(
				hWnd,IDC_PELT_DAMPENINGSPIN,IDC_PELT_DAMPENING,
				0.0f,0.5f,mod->peltData.GetDampening());	
	iSpinDampening->SetScale(0.1f);

	iSpinDecay = SetupFloatSpinner(
				hWnd,IDC_PELT_DECAYSPIN,IDC_PELT_DECAY,
				0.0f,0.5f,mod->peltData.GetDecay());	
	iSpinDecay->SetScale(0.1f);

	iSpinMirrorAngle = SetupFloatSpinner(
				hWnd,IDC_PELT_MIRRORAXISSPIN,IDC_PELT_MIRRORAXIS,
				0.0f,360.0f,mod->peltData.GetMirrorAngle()*180.0f/PI);	
	iSpinMirrorAngle->SetScale(1.0f);

	ICustButton *iPeltStraightenButton = GetICustButton(GetDlgItem(hWnd, IDC_STRAIGHTEN_BUTTON2));
	iPeltStraightenButton->SetType(CBT_CHECK);
	iPeltStraightenButton->SetHighlightColor(GREEN_WASH);
	ReleaseICustButton(iPeltStraightenButton);


	this->hWnd = hWnd;

}

void PeltDialog::DestroyDialog(HWND hWnd)
{
	
	ReleaseISpinner(iSpinRigStrength);
	iSpinRigStrength = NULL;

	ReleaseISpinner(iSpinSamples);
	iSpinSamples = NULL;

	ReleaseISpinner(iSpinFrames);
	iSpinFrames = NULL;

	ReleaseISpinner(iSpinStiffness);
	iSpinStiffness = NULL;

	ReleaseISpinner(iSpinDampening);
	iSpinDampening = NULL;

	ReleaseISpinner(iSpinDecay);
	iSpinDecay = NULL;

	ReleaseISpinner(iSpinMirrorAngle);
	iSpinMirrorAngle = NULL;

	this->hWnd = NULL;

}

void PeltDialog::SetStraightenMode(BOOL on)
{
	if (!on)
	{
		if (mod->oldMode == ID_TOOL_PELTSTRAIGHTEN)
			mod->SetMode(ID_TOOL_MOVE);
		else mod->SetMode(mod->oldMode);		
	}
	else
	{
		mod->peltData.currentPointHit = -1;
		mod->peltData.previousPointHit = -1;
		mod->SetMode(ID_TOOL_PELTSTRAIGHTEN);
		if (mod->fnGetTVSubMode() == TVVERTMODE)
			mod->vsel.ClearAll();
		
	}

	ICustButton *iPeltStraightenButton = GetICustButton(GetDlgItem(hWnd, IDC_STRAIGHTEN_BUTTON2));
	if (on)
		iPeltStraightenButton->SetCheck(TRUE);
	else iPeltStraightenButton->SetCheck(FALSE);
	ReleaseICustButton(iPeltStraightenButton);

}

void PeltDialog::SetStraightenButton(BOOL on)
{
	ICustButton *iPeltStraightenButton = GetICustButton(GetDlgItem(hWnd, IDC_STRAIGHTEN_BUTTON2));
	if (iPeltStraightenButton)
	{
		if (on)
			iPeltStraightenButton->SetCheck(TRUE);
		else iPeltStraightenButton->SetCheck(FALSE);
		ReleaseICustButton(iPeltStraightenButton);
	}
}

BOOL PeltDialog::GetStraightenMode()
{
	if (mod->mode == ID_TOOL_PELTSTRAIGHTEN)
		return TRUE;
	else return FALSE;
}

void PeltDialog::UpdateSpinner(int id, int value)
{
	if ((id == IDC_PELT_FRAMESSPIN) && iSpinFrames)
		iSpinFrames-> SetValue(value, 0);
	else if ((id == IDC_PELT_SAMPLESSPIN) && iSpinSamples)
		iSpinSamples-> SetValue(value, 0);
}

void PeltDialog::UpdateSpinner(int id, float value)
{
	if ((id == IDC_PELT_RIGSTRENGTHSPIN) && iSpinRigStrength)
		iSpinRigStrength-> SetValue(value, 0);
	else if ((id == IDC_PELT_STIFFNESSSPIN) && iSpinStiffness)
		iSpinStiffness-> SetValue(value, 0);
	else if ((id == IDC_PELT_DAMPENINGSPIN) && iSpinDampening)
		iSpinDampening-> SetValue(value, 0);
	else if ((id == IDC_PELT_DECAYSPIN) && iSpinDecay)
		iSpinDecay-> SetValue(value, 0);
	else if ((id == IDC_PELT_MIRRORAXISSPIN) && iSpinMirrorAngle)
		iSpinMirrorAngle-> SetValue(value, 0);

}

void PeltDialog::SpinnerChange(PeltData &peltData, int controlID)
{
	if (controlID == IDC_PELT_RIGSTRENGTHSPIN)
	{
		if (iSpinRigStrength)
		{
			peltData.SetRigStrength(iSpinRigStrength->GetFVal());
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltDialogStretcherStrength"), 1, 0,
				mr_float,iSpinRigStrength->GetFVal());		
		}
	}
	else if (controlID == IDC_PELT_SAMPLESSPIN)
	{
		if (iSpinSamples)
		{
			peltData.SetSamples(iSpinSamples->GetIVal());
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltDialogSamples"), 1, 0,
				mr_int,iSpinSamples->GetIVal());						
		}
	}
	else if (controlID == IDC_PELT_FRAMESSPIN)
	{
		if (iSpinFrames)
		{
			peltData.SetFrames(iSpinFrames->GetIVal());
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltDialogFrames"), 1, 0,
				mr_int,iSpinFrames->GetIVal());						

		}
	}
	else if (controlID == IDC_PELT_STIFFNESSSPIN)
	{
		if (iSpinStiffness)
		{
			peltData.SetStiffness(iSpinStiffness->GetFVal());
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltDialogStiffness"), 1, 0,
				mr_float,iSpinStiffness->GetFVal());		

		}
	}
	else if (controlID == IDC_PELT_DAMPENINGSPIN)
	{
		if (iSpinDampening)
		{
			peltData.SetDampening(iSpinDampening->GetFVal());
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltDialogDampening"), 1, 0,
				mr_float,iSpinDampening->GetFVal());
		}
	}
	else if (controlID == IDC_PELT_DECAYSPIN)
	{
		if (iSpinDecay)
		{
			peltData.SetDecay(iSpinDecay->GetFVal());
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltDialogDecay"), 1, 0,
				mr_float,iSpinDecay->GetFVal());

		}
	}
	else if (controlID == IDC_PELT_MIRRORAXISSPIN)
	{
		if (iSpinMirrorAngle)
		{
			peltData.SetMirrorAngle(iSpinMirrorAngle->GetFVal()*PI/180.0f);
			macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.setPeltDialogMirrorAxis"), 1, 0,
				mr_float,iSpinMirrorAngle->GetFVal());
		}
	}


}


UnwrapPeltSeamRestore::UnwrapPeltSeamRestore(UnwrapMod *m, Point3 *anchorPoint)
{
	mod = m;
	useams = mod->peltData.seamEdges;
	up1 = mod->peltData.previousPointHit;
	up2 = mod->peltData.currentPointHit;
	ucurrentP = mod->peltData.currentMouseClick;
	ulastP = mod->peltData.lastMouseClick;

	//watje 685880 might need to restore the anchor point
	mRestoreAnchorPoint = FALSE;
	if (anchorPoint)
	{
		mUAnchorPoint = *anchorPoint;
		mRestoreAnchorPoint = TRUE;
	}

}
	
void UnwrapPeltSeamRestore::Restore(int isUndo)
{
	if (isUndo)
	{
		rseams = mod->peltData.seamEdges;
		rp1 = mod->peltData.previousPointHit;
		rp2 = mod->peltData.currentPointHit;

		rcurrentP = mod->peltData.currentMouseClick;
		rlastP = mod->peltData.lastMouseClick;

		//watje 685880 might need to restore the anchor point
		if (mRestoreAnchorPoint)
			mRAnchorPoint = mod->peltData.pointToPointAnchorPoint;


	}
	mod->peltData.basep = ulastP;
	mod->peltData.seamEdges = useams;
	mod->peltData.previousPointHit = -1;
	mod->peltData.currentPointHit = up1;
	mod->peltData.basep = ulastP;

	//watje 685880 might need to restore the anchor point
	if (mRestoreAnchorPoint)
		mod->peltData.pointToPointAnchorPoint = mUAnchorPoint;


	mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
}
void UnwrapPeltSeamRestore::Redo()
{
	mod->peltData.previousPointHit = -1;
	mod->peltData.currentPointHit = rp1;
	mod->peltData.basep = rlastP;

	mod->peltData.seamEdges = rseams;

	//watje 685880 might need to restore the anchor point
	if (mRestoreAnchorPoint)
		mod->peltData.pointToPointAnchorPoint = mRAnchorPoint;

	mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
}

UnwrapPeltVertVelocityRestore::UnwrapPeltVertVelocityRestore(UnwrapMod *m)
{
	mod = m;
	uverts = mod->peltData.verts;

}
	
void UnwrapPeltVertVelocityRestore::Restore(int isUndo)
{
	if (isUndo)
	{
		rverts = mod->peltData.verts;
	}
	mod->peltData.verts = uverts;
}
void UnwrapPeltVertVelocityRestore::Redo()
{
	mod->peltData.verts = rverts;
}


void PeltData::MirrorRig(UnwrapMod *mod)
{

	if (rigPoints.Count() == 0) return;
	//get our mirror angle
	//build our total rig initial distance
	float d= 0.0f;
	for (int i = 0; i < rigPoints.Count(); i++)
	{
		d += rigPoints[i].elen;
	}
	float runningDist = 0.0f;
	float mirrorAngle = rigMirrorAngle;
	

	Tab<float> mAngles;
 	mAngles.SetCount(rigPoints.Count());
	for (int i = 0; i < rigPoints.Count(); i++)
	{
		float rigAngle = (rigPoints[i].angle - mirrorAngle);
		if (rigAngle < 0.0f)
			rigAngle = (PI*2.0f) + rigAngle;
		mAngles[i] = rigAngle;
//DebugPrint("%d  angle %f\n",i,rigAngle*180.0f/PI);

	}
	//build our mirror tm
	Matrix3 tm(1);
	tm.RotateZ(mirrorAngle);
	tm.SetRow(3,rigCenter);

	Matrix3 itm = Inverse(tm);
	

	//now start mirroring the data
	int currentMirrorPoint = -1;
	int nextMirrorPoint = -1;

	//get our start source point list
	int startSource;
	int startMirror;
	//get our start source mirror list
	Point3 lastPoint = Point3(0.0f,0.0f,0.0f);
	lastPoint = mod->TVMaps.v[rigPoints[0].lookupIndex].p;
	lastPoint = lastPoint * itm;
	Point3 nextPoint;
	for (int i = 1; i < rigPoints.Count(); i++)
	{		
		int rigIndex = rigPoints[i].lookupIndex;
		Point3 testPoint = mod->TVMaps.v[rigIndex].p;
		testPoint = testPoint * itm;
		if ((testPoint.x < 0.0f) && (lastPoint.x > 0.0f))
		{
			startSource = i - 1;
			startMirror = i;
		}
		lastPoint = testPoint;
		
	}
	//build our mirror list
	nextPoint = Point3(1.0f,0.0f,0.0f);
  	int currentSourcePoint = startSource;
	while (nextPoint.x > 0.0f)
	{	
		int id = rigPoints[currentSourcePoint].lookupIndex;
//DebugPrint("Mirror source %d\n",id);
		Point3 p = mod->TVMaps.v[id].p;
		p = p * itm;
		nextPoint = p;

		currentSourcePoint--;
		if (currentSourcePoint < 0)
			currentSourcePoint = rigPoints.Count()-1;			
	}

//now walk down the target verts

	currentSourcePoint = startSource;
	int prevSourcePoint = startSource;
	int currentTargetPoint = startMirror;
	//set the first one then walk the path
	int id =  rigPoints[currentTargetPoint].lookupIndex;
	int sid = rigPoints[currentSourcePoint].lookupIndex;
	Point3 p = mod->TVMaps.v[sid].p;
	
	p = p*itm;
	p.x *= -1.0f;
	mod->TVMaps.v[id].p = p*tm;
	float runningSourceD = 0.0f;
	float runningLastSourceD = 0.0f;
	float runningTargetD = 0.0f;
	Point3 lastMP = Point3(1.0f,0.0f,0.0f);
	while (lastMP.x > 0.0f)
	{	
		//get the next target point
		currentTargetPoint++;
		if (currentTargetPoint>=rigPoints.Count())
			currentTargetPoint = 0;
		runningTargetD += rigPoints[currentTargetPoint].elen;
		//now find the spot on the mirror
		while (runningSourceD < runningTargetD)
		{
			prevSourcePoint = currentSourcePoint;
			runningLastSourceD = runningSourceD;
			currentSourcePoint--;
			if (currentSourcePoint < 0)
				currentSourcePoint = rigPoints.Count()-1;	
			runningSourceD += rigPoints[currentSourcePoint].elen;
		}
		float remainder = runningTargetD - runningLastSourceD;
		float per = remainder/(runningSourceD-runningLastSourceD);

		Point3 prevp = mod->TVMaps.v[rigPoints[prevSourcePoint].lookupIndex].p;
		Point3 currentp = mod->TVMaps.v[rigPoints[currentSourcePoint].lookupIndex].p;
		Point3 vec = (currentp - prevp) * per;
		Point3 p = prevp + vec;
		p = p * itm;
		lastMP = p;
		p.x *= -1.0f;		
		p = p * tm;
		
//DebugPrint(" Source %d mirror %d\n",rigPoints[prevSourcePoint].lookupIndex,rigPoints[currentTargetPoint].lookupIndex);
		mod->TVMaps.v[rigPoints[currentTargetPoint].lookupIndex].p = p;
	}

	


/*

	for (int i = 0; i < rigPoints.Count(); i++)
	{
		float rigAngle = mAngles[i];
		int rigIndex = rigPoints[i].lookupIndex;
		
			
		Point3 testPoint = mod->TVMaps.v[rigIndex].p;
		testPoint = testPoint * itm;
		if (testPoint.x < 0.0f)
		{
DebugPrint("source index %d\n",rigIndex);
//			if (currentMirrorPoint == -1)
			{
				currentMirrorPoint = 0;//rigPoints.Count()-1;
				nextMirrorPoint = 0;//rigPoints.Count()-1;
			}

			//mirror this point
			float matchAngle = (PI*2.0f) - rigAngle;


			BOOL done = FALSE;
			while (!done)
			{
				float cAngle,nAngle;
				cAngle = mAngles[currentMirrorPoint];
				nAngle = mAngles[nextMirrorPoint];
				if  ( (matchAngle >= mAngles[currentMirrorPoint]) && 
					  (matchAngle <= mAngles[nextMirrorPoint]) )
				{
				// mirror to this point
					done = TRUE;
					
					float tangle = (mAngles[nextMirrorPoint]-mAngles[currentMirrorPoint]);
					float angle = matchAngle-mAngles[currentMirrorPoint];
					float per = angle/tangle;
					Point3 vec;
					int cp,np;
					cp = rigPoints[currentMirrorPoint].lookupIndex;
					np = rigPoints[nextMirrorPoint].lookupIndex;
					vec  = mod->TVMaps.v[np].p - mod->TVMaps.v[cp].p;
					vec *= 1.0f-per;
					Point3 goal =  mod->TVMaps.v[cp].p + vec;
					//mirror it and assign it
					int id = rigPoints[i].lookupIndex;
					Point3 p = mod->TVMaps.v[cp].p;
					goal = goal * itm;
					goal.x *= -1.0f;
					goal = goal * tm;

//					mod->TVMaps.v[id].p = goal;
DebugPrint("target index %d to %d\n",cp,np);


				}
				else
				{
					currentMirrorPoint = nextMirrorPoint;
					nextMirrorPoint++;
		 			if (nextMirrorPoint >= rigPoints.Count()) 
						done = TRUE;
				}
			}
			

		}
	}
*/
	mod->InvalidateView();
}



void PeltData::StraightenSeam(UnwrapMod *mod, int a, int b)

{

	//get our sel


	BitArray seamPoints;
	seamPoints.SetSize(mod->vsel.GetSize());
	seamPoints.ClearAll();
	for (int i = 0; i < rigPoints.Count(); i++)
	{
		seamPoints.Set(rigPoints[i].lookupIndex,TRUE);
	}


	if (a == b) return;
	int ra = -1;
	int rb = -1;

	for (int i = 0; i < rigPoints.Count(); i++)
	{
		if ((a == rigPoints[i].lookupIndex) && (ra == -1))
			ra = i;
		if ((b == rigPoints[i].lookupIndex) && (rb == -1))
			rb = i;
	}

	//make sure we have 2 rig points
	if ((ra == -1) || (rb == -1))
		return;

	int aDist;
	int bDist;
	if (rb < ra)
	{
		int temp = ra;
		ra = rb;
		rb = temp;
	}

	aDist = rb - ra;
	bDist = ra;
	bDist += rigPoints.Count() - rb;

	int startPoint = ra;
	int endPoint = rb;

	if (bDist < aDist)
	{
		startPoint = rb;
		endPoint = ra;
	}

	int currentPoint = startPoint;
	float goalDist = 0.0f;
	float seamDist = 0.0f;

	Tab<int> edgeList;

	while (currentPoint != endPoint)
	{
		edgeList.Append(1,&currentPoint);
		currentPoint++;
		if (currentPoint >= rigPoints.Count())
			currentPoint = 0;
	}

	edgeList.Append(1,&endPoint);
	Point3 pa,pb;

	pa = mod->TVMaps.v[rigPoints[startPoint].lookupIndex].p;
	pb = mod->TVMaps.v[rigPoints[endPoint].lookupIndex].p;

	goalDist = Length(pb-pa);
	Point3 vec = (pb-pa);

	for (int i = 0; i < (edgeList.Count()-1); i++)
	{
		seamDist += rigPoints[i].elen;
	}


	float runningDist = rigPoints[0].elen;
	for (int i = 1; i < (edgeList.Count()-1); i++)
	{
		int id = edgeList[i];
		float per = runningDist/seamDist;
		int currentPoint = rigPoints[id].lookupIndex;
		mod->TVMaps.v[currentPoint].p = pa + (vec*per);
		runningDist += rigPoints[i].elen; 
	} 

}



HCURSOR PeltStraightenMode::GetXFormCur() 
	{
	return mod->selCur;
	}

int PeltStraightenMode::subproc(HWND hWnd, int msg, int point, int flags, IPoint2 m)
	{
	switch (msg) {
		case MOUSE_POINT:
			if (point==0) {
 				theHold.SuperBegin();
		 		mod->PlugControllers();
				theHold.Begin();
				mod->HoldPoints();
				om = m;
				mod->tempVert = Point3(0.0f,0.0f,0.0f);


			} else {
				macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap2.MoveSelected"), 1, 0,
					mr_point3,&mod->tempVert);

				if (mod->peltData.previousPointHit!= -1)
				{
					mod->peltData.StraightenSeam(mod,mod->peltData.previousPointHit,mod->peltData.currentPointHit);
					macroRecorder->FunctionCall(_T("$.modifiers[#unwrap_uvw].unwrap5.peltDialogStraighten"), 2, 0,
						mr_int,mod->peltData.previousPointHit+1,
						mr_int,mod->peltData.currentPointHit+1
						);
				}
				mod->peltData.previousPointHit = mod->peltData.currentPointHit;
			 	mod->ip->RedrawViews(mod->ip->GetTime());

				theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));
				theHold.SuperAccept(_T(GetString(IDS_PW_MOVE_UVW)));
				om = m;
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
			
			for (int i = 0; i < mod->vsel.GetSize(); i++)
			{
				if (mod->vsel[i])
					mod->peltData.currentPointHit = i;
			}


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
			
			mod->tempVert.x = mv.x;
			mod->tempVert.y = mv.y;
			mod->tempVert.z = 0.0f;

			mod->TransferSelectionStart();
			mod->MovePoints(mv);
			mod->TransferSelectionEnd(FALSE,FALSE);

	 	 	if (mod->peltData.previousPointHit!= -1)
				mod->peltData.StraightenSeam(mod,mod->peltData.previousPointHit,mod->peltData.currentPointHit);

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


class FConnnections
{
public:
	Tab<int> connectedEdge;
};

void PeltData::ExpandSelectionToSeams(UnwrapMod *mod)
{
	UVW_ChannelClass *tvData = &mod->TVMaps;
	tvData = &mod->TVMaps;

	for (int i = 0; i < tvData->ePtrList.Count(); i++)
	{
		tvData->ePtrList[i]->lookupIndex = i;
	}
	for (int i = 0; i < tvData->gePtrList.Count(); i++)
	{
		tvData->gePtrList[i]->lookupIndex = i;
	}

	ModContextList mcList;		
	INodeTab nodes;

  	MeshTopoData *md =NULL;

	GetCOREInterface()->GetModContexts(mcList,nodes);

	int objects = mcList.Count();
	if (objects == 0) return;	

 	md = (MeshTopoData*)mcList[0]->localData;
	int seedFace = -1;

	for (int i = 0; i < md->faceSel.GetSize(); i++)
	{
		if (md->faceSel[i])
			 seedFace = i;
	}
	if (seedFace == -1) return;



	Tab<FConnnections*> face;
	face.SetCount(tvData->f.Count());
	for (int i = 0; i < tvData->f.Count(); i++)
		face[i] = new FConnnections();

	for (int i = 0; i < tvData->f.Count(); i++)
	{
		int deg = tvData->f[i]->count;
		for (int j = 0; j < deg; j++)
		{
			int a = tvData->f[i]->v[j];
			int b = tvData->f[i]->v[(j+1)%deg];
			int edge = tvData->FindGeoEdge(a,b);

			face[i]->connectedEdge.Append(1,&edge,4);
		}
	}


	// start adding edges
	BOOL done = FALSE;
	int currentFace = seedFace;
	
	BitArray selEdges;
	selEdges.SetSize(tvData->gePtrList.Count());
	selEdges.ClearAll();

	BitArray processedFaces;
	processedFaces.SetSize(tvData->f.Count());
	processedFaces.ClearAll();
 	Tab<int> faceStack;

	if (seamEdges.GetSize() != tvData->gePtrList.Count())
	{
		seamEdges.SetSize(tvData->gePtrList.Count());
		seamEdges.ClearAll();
	}

	while (!done)
	{
		//loop through the current face edges
		int deg = tvData->f[currentFace]->count;
		//mark this face as processed
		processedFaces.Set(currentFace,TRUE);
		for (int j = 0; j < deg; j++)
		{
			int a = tvData->f[currentFace]->v[j];
			int b = tvData->f[currentFace]->v[(j+1)%deg];
			int edge = tvData->FindGeoEdge(a,b);
		//add the edges if they are not a seam
			if (!seamEdges[edge])
			{
				selEdges.Set(edge,TRUE);				

				//find the connected faces to this edge
				int numberConnectedFaces = tvData->gePtrList[edge]->faceList.Count();
				for (int k = 0; k < numberConnectedFaces; k++)
				{
				//add them to the stack if they have not been processed
					int potentialFaceIndex = tvData->gePtrList[edge]->faceList[k];

					if (!processedFaces[potentialFaceIndex])
						faceStack.Append(1,&potentialFaceIndex,100);
				}

			}
		}
		
		//if stack == empty
		if (faceStack.Count() == 0)
			done = TRUE;
		else
		{
		//else current face = pop top of the stack		
			currentFace = faceStack[faceStack.Count()-1];
			faceStack.Delete(faceStack.Count()-1,1);
		}

	}

	processedFaces.ClearAll();
	for (int i = 0; i < tvData->gePtrList.Count(); i++)
	{
		if (selEdges[i] )
		{
			int numberConnectedFaces = tvData->gePtrList[i]->faceList.Count();
			for (int k = 0; k < numberConnectedFaces; k++)
			{
				//add them to the stack if they have not been processed
				int potentialFaceIndex = tvData->gePtrList[i]->faceList[k];

				processedFaces.Set(potentialFaceIndex,TRUE);						
			}
		}
	}

 	for (int i = 0; i < processedFaces.GetSize(); i++)
	{
		if (processedFaces[i])
		{
			tvData->f[i]->flags |= FLAG_SELECTED;
//DebugPrint("Selected face %d\n",i);
		}
		else tvData->f[i]->flags &= ~FLAG_SELECTED;
			
	}


 	md->faceSel = processedFaces;
	mod->SyncTVToGeomSelection(md);


	for (int i = 0; i < tvData->f.Count(); i++)
		delete face[i];

	mod->InvalidateView();
	mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	if (mod->ip) mod->ip->RedrawViews(mod->ip->GetTime());


}

int PeltData::HitTestPointToPointMode(UnwrapMod *mod, ViewExp *vpt,GraphicsWindow *gw,IPoint2 *p,HitRegion hr, INode* inode,ModContext* mc)
{
	int res = 0;
	static IPoint2 basep;
	static IPoint2 cp,pp;

	if (currentPointHit == -1) 
	{
		basep = p[0];
	}
	pp = cp;
	cp = p[0];
	//xor our last line
	//draw our new one
/*
	if (currentPointHit != -1) 
	{


		IPoint2 points[3];
		HWND hWnd = gw->getHWnd();

		points[0] = basep;
		points[1] = pp;
		XORDottedPolyline(hWnd, 2, points);

		points[0] = basep;
		points[1] = cp;
		XORDottedPolyline(hWnd, 2, points);

	}
*/

	gw->setRndLimits(gw->getRndLimits() |  GW_BACKCULL);
	BitArray visibleFaces;
	mod->BuildVisibleFaces(gw, mc, visibleFaces);

	Tab<UVWHitData> hitVerts;
	mod->HitGeomVertData(hitVerts,gw,  hr);
	res = 0;

	TimeValue t = GetCOREInterface()->GetTime();
	Matrix3 ntm = inode->GetObjectTM(t);

	Matrix3 vtm; 
	vpt->GetAffineTM(vtm );
	vtm = Inverse(vtm);

	viewZVec = vtm.GetRow(2);
	ntm = Inverse(ntm);
	viewZVec = VectorTransform(ntm,viewZVec);


	Tab<int> lookup;
	lookup.SetCount(mod->TVMaps.geomPoints.Count());
	for (int i = 0;  i < mod->TVMaps.geomPoints.Count(); i++)
	{
		lookup[i] = -1;
	}

	for (int i = 0;  i < hitVerts.Count(); i++)
	{
		lookup[hitVerts[i].index] = i;
	}
	float closest = -1.0f;
	int closestIndex = -1;

	Matrix3 toView(1);
	vpt->GetAffineTM(toView);
	Matrix3 mat = inode->GetObjectTM(GetCOREInterface()->GetTime());
	toView = mat * toView;

	for (int i = 0;  i < visibleFaces.GetSize(); i++)
	{

		if (visibleFaces[i]/*&& (i < peltFaces.GetSize()) && (peltData.peltFaces[i])*/)
		{
			int deg = mod->TVMaps.f[i]->count;
			for (int j = 0; j < deg; j++)
			{
				int index = mod->TVMaps.f[i]->v[j];

				if (lookup[index]!=-1)
				{
					

					Point3 p = mod->TVMaps.geomPoints[index] * toView;
					//DebugPrint(" eindex %d z %f\n",eindex,p.z);
					if ((p.z > closest) || (closestIndex==-1))
					{
						closest = p.z ;
						closestIndex = index;
					}
/*
					int id = lookup[index];
					float d = hitVerts[id].dist;
					if ((d > closest) || (closestIndex == -1))
					{
						closest = d;
						closestIndex = index;
						//									res = 1;
						//									vpt->LogHit(inode,mc,0.0f,index,NULL);
					}
*/
				}
			}

		}
	}
	if (closestIndex != -1)
	{
		res = 1;
		vpt->LogHit(inode,mc,0.0f,closestIndex,NULL);
	}

	return closestIndex;
}


int PeltData::HitTestEdgeMode(UnwrapMod *mod, ViewExp *vpt,GraphicsWindow *gw,IPoint2 *p,HitRegion hr,INode* inode,ModContext* mc,int flags, int type)
{

	//build our visible face
	BitArray visibleFaces;
	mod->BuildVisibleFaces(gw, mc, visibleFaces);
	//hit test our geom edges
	Tab<UVWHitData> hitEdges;
	mod->HitGeomEdgeData(hitEdges,gw,  hr);
	int res = 0;
	res = hitEdges.Count();
	
	if (type == HITTYPE_POINT)
	{
		int closestIndex = -1;
		float closest=  0.0f;
		Matrix3 toView(1);
		vpt->GetAffineTM(toView);
		Matrix3 mat = inode->GetObjectTM(GetCOREInterface()->GetTime());
		toView = mat * toView;

		//DebugPrint(" ct %d\n",hitEdges.Count());				
		for (int hi = 0; hi < hitEdges.Count(); hi++)
		{
			int eindex = hitEdges[hi].index;
			int a = mod->TVMaps.gePtrList[eindex]->a;

			Point3 p = mod->TVMaps.geomPoints[a] * toView;
			//DebugPrint(" eindex %d z %f\n",eindex,p.z);
			if ((p.z > closest) || (closestIndex==-1))
			{
				closest = p.z ;
				closestIndex = hi;
			}

			a = mod->TVMaps.gePtrList[eindex]->b;

			p = mod->TVMaps.geomPoints[a] * toView;
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


	if (seamEdges.GetSize() != mod->TVMaps.gePtrList.Count())
	{
		seamEdges.SetSize(mod->TVMaps.gePtrList.Count());
		seamEdges.ClearAll();
	}
	for (int hi = 0; hi < hitEdges.Count(); hi++)
	{
		int i = hitEdges[hi].index;
		//						if (hitEdges[i])
		{
			BOOL visibleFace = FALSE;
			BOOL selFace = TRUE;
			int ct = mod->TVMaps.gePtrList[i]->faceList.Count();
			for (int j = 0; j < ct; j++)
			{
				int faceIndex = mod->TVMaps.gePtrList[i]->faceList[j];
//				if ((faceIndex < peltFaces.GetSize())/* && (peltData.peltFaces[faceIndex])*/)
//					selFace = TRUE;
				if ((faceIndex < visibleFaces.GetSize()) && (visibleFaces[faceIndex]))
					visibleFace = TRUE;

			}
			BOOL selOnly = flags&HIT_SELONLY;
			BOOL unselOnly = flags&HIT_UNSELONLY;

			if (mod->fnGetBackFaceCull())
			{
				if (selFace && visibleFace)
				{
					if ( (seamEdges[i] && (flags&HIT_SELONLY)) ||
						!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
						vpt->LogHit(inode,mc,0.0f,i,NULL);
					else if ( (!seamEdges[i] && (flags&HIT_UNSELONLY)) ||
						!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
						vpt->LogHit(inode,mc,0.0f,i,NULL);

				}
			}
			else
			{
				if (selFace )
				{
					if ( (seamEdges[i] && (flags&HIT_SELONLY)) ||
						!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
						vpt->LogHit(inode,mc,0.0f,i,NULL);
					else if ( (!seamEdges[i] && (flags&HIT_UNSELONLY)) ||
						!(flags&(HIT_UNSELONLY|HIT_SELONLY)))
						vpt->LogHit(inode,mc,0.0f,i,NULL);

				}
			}
		}
	}
	return res;

}


void PeltData::ResolvePatchHandles(UnwrapMod *mod)
{
	//loop through our edge pointer list
	for (int i = 0; i < mod->TVMaps.ePtrList.Count(); i++)
	{
	//find ones with handles 
		int va = mod->TVMaps.ePtrList[i]->avec;
		int vb = mod->TVMaps.ePtrList[i]->bvec;
		int a = mod->TVMaps.ePtrList[i]->a;
		int b = mod->TVMaps.ePtrList[i]->b;

//make them 1/3 way between the end points
		if (mod->vsel[a] || mod->vsel[b])
		{
			Point3 vec = (mod->TVMaps.v[b].p - mod->TVMaps.v[a].p)*0.33f;
			if (va != -1)
			{
				mod->TVMaps.v[va].p = mod->TVMaps.v[a].p + vec;
			}
			if (vb != -1)
			{
				mod->TVMaps.v[vb].p = mod->TVMaps.v[b].p - vec;
			}
		}
	
	}
}



BOOL UnwrapMod::fnGetAlwayShowPeltSeams()
{
	return alwaysShowSeams;
}
void UnwrapMod::fnSetAlwayShowPeltSeams(BOOL show)
{
	alwaysShowSeams = show;
	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
}

void  UnwrapMod::BuildEdgeDistortionData()
{

	//loop through our geo/uvw edges
	float sumUVLengths = 0;
	float sumGLengths = 0;
	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
	//find ones with handles 

		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		int ga = TVMaps.ePtrList[i]->ga;
		int gb = TVMaps.ePtrList[i]->gb;

		Point3 pa = TVMaps.v[a].p;
		Point3 pb = TVMaps.v[b].p;

		Point3 pga = TVMaps.geomPoints[ga];
		Point3 pgb = TVMaps.geomPoints[gb];

		sumUVLengths += Length(pa-pb);
		sumGLengths += Length(pga-pgb);

	}

	sumUVLengths = sumUVLengths/(float)TVMaps.ePtrList.Count();
	sumGLengths = sumGLengths/(float)TVMaps.ePtrList.Count();


	edgeScale = sumUVLengths/sumGLengths;
	//get there average lengths

}


void PeltData::RelaxRig(int iterations, float str, BOOL lockBoundaries, UnwrapMod *mod)
{
	if (mod == NULL) return;
	Tab<Point3> op;

 	op.SetCount(mod->TVMaps.v.Count());
	str = str * 0.05f;
	

	for (int i = 0; i < iterations; i++)
	{
		for (int j = 0; j < op.Count(); j++)
			op[j] = mod->TVMaps.v[j].p;
		for (int j = 0; j < rigPoints.Count(); j++)
		{
			int prevID = j - 1;
			int currentID = j;
			int nextID = j + 1;
			if (prevID < 0) prevID = rigPoints.Count()-1;
			if (nextID >= rigPoints.Count()) nextID = 0;


			Point3 pp,cp,np;
			pp = op[rigPoints[prevID].lookupIndex];
			cp = op[rigPoints[currentID].lookupIndex];
			np = op[rigPoints[nextID].lookupIndex];

			Point3 newPoint = (pp + np) *0.5f;
			newPoint = cp + (newPoint - cp) * str;

			int selID = rigPoints[currentID].lookupIndex;

			if (mod->vsel[selID])
			{
				if (!lockBoundaries)
					mod->TVMaps.v[selID].p = newPoint;
				else
				{
					if (mod->vsel[rigPoints[prevID].lookupIndex] && mod->vsel[rigPoints[nextID].lookupIndex])
					{
						mod->TVMaps.v[selID].p = newPoint;
					}
				}
			}
		}

	}

	TimeValue t = GetCOREInterface()->GetTime();
	for (int i = 0; i < mod->TVMaps.v.Count(); i++)
	{
		if (mod->TVMaps.cont[i]) mod->TVMaps.cont[i]->SetValue(t,&mod->TVMaps.v[i].p);
	}
}
