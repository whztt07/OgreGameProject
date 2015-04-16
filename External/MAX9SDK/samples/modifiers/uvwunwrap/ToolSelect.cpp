#include "unwrap.h"


void UnwrapMod::RebuildDistCache()
{
	
if (iStr==NULL) return;

float str = iStr->GetFVal();
falloffStr = str;
float sstr = str*str;
if ((str == 0.0f) || (enableSoftSelection == FALSE))
	{
	for (int i = 0; i<TVMaps.v.Count(); i++)
		{
		if (!(TVMaps.v[i].flags & FLAG_WEIGHTMODIFIED))
			TVMaps.v[i].influence = 0.0f;
		}
	return;
	}
Tab<int> Selected;

for (int i = 0; i<TVMaps.v.Count(); i++)
	{
	if (vsel[i])
		Selected.Append(1,&i,1);
	}
if (falloffSpace == 0)
	BuildObjectPoints();

BitArray useableVerts;
useableVerts.SetSize(TVMaps.v.Count());



if (limitSoftSel)
	{
	int oldMode = fnGetTVSubMode();
	fnSetTVSubMode(TVVERTMODE);
	useableVerts.ClearAll();
	BitArray holdSel;
	holdSel.SetSize(vsel.GetSize());
	holdSel = vsel;
	for (int i=0; i < limitSoftSelRange; i++)
		{
		ExpandSelection(0,FALSE,FALSE);
		}
	useableVerts = vsel;
	vsel = holdSel;
	fnSetTVSubMode(oldMode);
	}
else useableVerts.SetAll();

for (int i = 0; i<TVMaps.v.Count(); i++)
	{
	if (!(TVMaps.v[i].flags & FLAG_WEIGHTMODIFIED))
		TVMaps.v[i].influence = 0.0f;
	}


for (int i = 0; i<TVMaps.v.Count(); i++)
	{
	if ( (vsel[i] == 0) && (useableVerts[i]) && (!(TVMaps.v[i].flags & FLAG_WEIGHTMODIFIED)))
		{
		float closest_dist = BIGFLOAT;
		for (int j= 0; j < Selected.Count();j++)
			{
//use XY	Z space values
			if (falloffSpace == 0)
				{
				Point3 sp = GetObjectPoint(ip->GetTime(),i);
				Point3 rp = GetObjectPoint(ip->GetTime(),Selected[j]);
				float d = LengthSquared(sp-rp);
				if (d < closest_dist) closest_dist = d;

				}
			else
//use UVW space values
				{
				Point3 sp = GetPoint(ip->GetTime(),Selected[j]);
				Point3 rp = GetPoint(ip->GetTime(),i);
				float d = LengthSquared(sp-rp);
				if (d < closest_dist) closest_dist = d;
				}
			}
		if (closest_dist < sstr)
			{
			closest_dist = (float) sqrt(closest_dist);
			TVMaps.v[i].influence = 1.0f - closest_dist/str;
			ComputeFalloff(TVMaps.v[i].influence,falloff);
			}
		else TVMaps.v[i].influence = 0.0f;
		}
	}	

}


void UnwrapMod::ExpandSelection(int dir, BOOL rebuildCache, BOOL hold)

{
	// LAM - 6/19/04 - defect 576948
	if (hold)
	{
		theHold.Begin();
		HoldSelection();
	}

	//convert our sub selection type to vertex selection
	TransferSelectionStart();

	BitArray faceHasSelectedVert;

	faceHasSelectedVert.SetSize(TVMaps.f.Count());
	faceHasSelectedVert.ClearAll();

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
				if (dir == 0)
					vsel.Set(index,1);
				else vsel.Set(index,0);
				}

			}
		}




	SelectHandles(dir);
	if (rebuildCache)
		RebuildDistCache();
	//convert our sub selection back

	TransferSelectionEnd(FALSE,TRUE);


	if (hold)
		theHold.Accept(GetString(IDS_DS_SELECT));					
}

void	UnwrapMod::fnSelectElement()
	{
	theHold.Begin();
	HoldSelection();
	SelectElement();
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
	RebuildDistCache();
	theHold.Accept(GetString(IDS_DS_SELECT));					
	}

void UnwrapMod::SelectElement(BOOL addSelection)

{

	bool faceMode = false;
	BitArray tempFSel;
	if (fnGetTVSubMode() == TVFACEMODE)
	{
		faceMode = true;
		tempFSel = fsel;
	}

	TransferSelectionStart();

	if (faceMode && (!addSelection))
	{
		for (int i = 0; i < TVMaps.f.Count();i++)
		{
			if (!(TVMaps.f[i]->flags & FLAG_DEAD) && (!tempFSel[i]))
			{
				int pcount = 3;
				pcount = TVMaps.f[i]->count;
				int totalSelected = 0;
				for (int k = 0; k < pcount; k++)
				{
					int index = TVMaps.f[i]->t[k];
					vsel.Set(index,false);
				}
			}
		}
	}


	BitArray faceHasSelectedVert;

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
					if (addSelection)
						vsel.Set(index,1);
					else vsel.Set(index,0);
				}
			}

		}
	}

	SelectHandles(0);


	TransferSelectionEnd(FALSE,TRUE);

}


void UnwrapMod::SelectHandles(int dir)

{
//if face is selected select me
for (int j = 0; j < TVMaps.f.Count();j++)
	{
	if ( (!(TVMaps.f[j]->flags & FLAG_DEAD)) &&
	     (TVMaps.f[j]->flags & FLAG_CURVEDMAPPING) &&
		 (TVMaps.f[j]->vecs) 
	   )

		{
		int pcount = 3;
		pcount = TVMaps.f[j]->count;
		for (int k = 0; k < pcount; k++)
			{
			int id = TVMaps.f[j]->t[k];
			if (dir ==0)
				{
				if ((vsel[id]) && (!wasSelected[id]))
					{
					int vid1 = TVMaps.f[j]->vecs->handles[k*2];
					int vid2 = 0;
					if (k ==0)
						vid2 = TVMaps.f[j]->vecs->handles[pcount*2-1];
					else vid2 = TVMaps.f[j]->vecs->handles[k*2-1];
	
					if ( (IsVertVisible(vid1)) &&  (!(TVMaps.v[vid1].flags & FLAG_FROZEN)) )
						vsel.Set(vid1,1);
					if ( (IsVertVisible(vid2)) &&  (!(TVMaps.v[vid2].flags & FLAG_FROZEN)) )
						vsel.Set(vid2,1);

					if (TVMaps.f[j]->flags & FLAG_INTERIOR)
						{
						int ivid1 = TVMaps.f[j]->vecs->interiors[k];
						if ( (ivid1 >= 0) && (IsVertVisible(ivid1)) &&  (!(TVMaps.v[ivid1].flags & FLAG_FROZEN)) )
							vsel.Set(ivid1,1);
						}
					}
				}
			else
				{
				if (!vsel[id])
					{
					int vid1 = TVMaps.f[j]->vecs->handles[k*2];
					int vid2 = 0;
					if (k ==0)
						vid2 = TVMaps.f[j]->vecs->handles[pcount*2-1];
					else vid2 = TVMaps.f[j]->vecs->handles[k*2-1];
	
					if ( (IsVertVisible(vid1)) &&  (!(TVMaps.v[vid1].flags & FLAG_FROZEN)) )
						vsel.Set(vid1,0);
					if ( (IsVertVisible(vid2)) &&  (!(TVMaps.v[vid2].flags & FLAG_FROZEN)) )
						vsel.Set(vid2,0);

					if (TVMaps.f[j]->flags & FLAG_INTERIOR)
						{
						int ivid1 = TVMaps.f[j]->vecs->interiors[k];
						if ( (ivid1 >= 0) && (IsVertVisible(ivid1)) &&  (!(TVMaps.v[ivid1].flags & FLAG_FROZEN)) )
							vsel.Set(ivid1,0);
						}

					}

				}

			}
		}
	}

}





void UnwrapMod::SelectFacesByNormals( MeshTopoData *md,BitArray &sel,Point3 norm, float angle, Tab<Point3> &normList)
	{

	if (normList.Count() == 0)
		BuildNormals(md,normList);
	norm = Normalize(norm);
	angle = angle * PI/180.0f;
	if (md->mesh)
		{
//check for contigous faces
		double newAngle;
		if (sel.GetSize() != md->mesh->numFaces)
			sel.SetSize(md->mesh->numFaces);
		sel.ClearAll();
		for (int i =0; i < md->mesh->numFaces; i++)
			{
			Point3 debugNorm = Normalize(normList[i]);
			float dot = DotProd(debugNorm,norm);
			newAngle = (acos(dot));




			if ((dot == 1.0f) || (newAngle <= angle))
				sel.Set(i);
			else
				{
				sel.Set(i,0);
				}
			}
		}
	else if (md->mnMesh)
		{
//check for contigous faces
		double newAngle;
		if (sel.GetSize() != md->mnMesh->numf)
			sel.SetSize(md->mnMesh->numf);
		for (int i =0; i < md->mnMesh->numf; i++)
			{
			Point3 debugNorm = normList[i];
			float dot = DotProd(normList[i],norm);
			newAngle = (acos(dot));			
			if ((dot == 1.0f) || (newAngle <= angle))
				sel.Set(i);
			else
				{
				sel.Set(i,0);
				}
			}
		}
	else if (md->patch)
		{
//check for contigous faces
		double newAngle;
		if (sel.GetSize() != md->patch->numPatches)
			sel.SetSize(md->patch->numPatches);
		for (int i =0; i < md->patch->numPatches; i++)
			{
			Point3 debugNorm = normList[i];
			float dot = DotProd(normList[i],norm);
			newAngle = (acos(dot));
			if ((dot == 1.0f) || (newAngle <= angle))
				sel.Set(i);
			else
				{
				sel.Set(i,0);
				}
			}
		}


	}


void UnwrapMod::SelectFacesByGroup( MeshTopoData *md,BitArray &sel,int seedFaceIndex, Point3 norm, float angle, BOOL relative,Tab<Point3> &normList,
								   Tab<BorderClass> &borderData,
								   AdjEdgeList *medges)
	{
	//check for type

	if (normList.Count() == 0)
		BuildNormals(md,normList);

	int ct = 0;

	angle = angle * PI/180.0f;

	sel.ClearAll();

	

	if (md->mesh)
		{
//get seed face
		if (seedFaceIndex < md->mesh->numFaces)
			{
			
			if (sel.GetSize() != md->mesh->numFaces)
				sel.SetSize(md->mesh->numFaces);
//select it
			sel.Set(seedFaceIndex);
			
//build egde list of all edges that have only one edge selected
			AdjEdgeList *edges;
			BOOL deleteEdges = FALSE;
			if (medges == NULL)
				{
				edges = new AdjEdgeList(*md->mesh);
				deleteEdges = TRUE;
				}
			else edges = medges;

			borderData.ZeroCount();

			
			int numberWorkableEdges = 1;
			while (numberWorkableEdges > 0)
				{
				numberWorkableEdges = 0;
				for (int i = 0; i < edges->edges.Count(); i++)
					{
//					if (!blockedEdges[i])
						{
						int a = edges->edges[i].f[0];
						int b = edges->edges[i].f[1];
						if ( (a>=0) && (b>=0) )
							{
							if (sel[a] && (!sel[b]))
								{
								float newAngle = 0.0f;
								if (!relative)
								{
									float dot = DotProd(normList[b],norm);
									if (dot != 1.0f)
										newAngle = (acos(dot));
									
								}
								else
								{
									float dot = DotProd(normList[b],normList[a]);
									if (dot != 1.0f)
										newAngle = (acos(dot));
								}
								if (newAngle <= angle)
									{
									sel.Set(b);
									numberWorkableEdges++;
									
									}
								else
									{
									BorderClass tempData;
									tempData.edge = i;
									tempData.innerFace = a;
									tempData.outerFace = b;
									borderData.Append(1,&tempData);
									}
								}
							else if (sel[b] && (!sel[a]))
								{
								float newAngle = 0.0f;
								if (!relative)
								{
									float dot = DotProd(normList[a],norm);
									if (dot != 1.0f)
										newAngle = (acos(dot));
								}
								else
								{
									float dot = DotProd(normList[a],normList[b]);
									if (dot != 1.0f)
										newAngle = (acos(dot));
								}
								if (newAngle <= angle)
									{
									sel.Set(a);
									numberWorkableEdges++;
									
									}
								else
									{
									BorderClass tempData;
									tempData.edge = i;
									tempData.innerFace = b;
									tempData.outerFace = a;
									borderData.Append(1,&tempData);

									}

								}
							}
						}
					}
				}
			if (deleteEdges) delete edges;
			}

		}
	else if (md->patch)
		{
		if (seedFaceIndex < md->patch->numPatches)
			{
//select it
			if (sel.GetSize() != md->patch->numPatches)
				sel.SetSize(md->patch->numPatches);

			sel.Set(seedFaceIndex);

			borderData.ZeroCount();

//build egde list of all edges that have only one edge selected
			PatchEdge *edges = md->patch->edges;
				
			int numberWorkableEdges = 1;
			while (numberWorkableEdges > 0)
				{
				numberWorkableEdges = 0;
				for (int i = 0; i < md->patch->numEdges; i++)
					{
					if (edges[i].patches.Count() ==2 )
						{
						int a = edges[i].patches[0];
						int b = edges[i].patches[1];
						if ( (a>=0) && (b>=0) )
							{
							if (sel[a] && (!sel[b]))
								{
								float newAngle;
								if (!relative)
									newAngle = (acos(DotProd(normList[b],norm)));
								else newAngle = (acos(DotProd(normList[b],normList[a])));
								if (newAngle <= angle)
									{
									sel.Set(b);
									numberWorkableEdges++;
									
									}
								else
									{
									BorderClass tempData;
									tempData.edge = i;
									tempData.innerFace = a;
									tempData.outerFace = b;
									borderData.Append(1,&tempData);

									}
								}
							else if (sel[b] && (!sel[a]))
								{
								float newAngle;
								if (!relative)
									newAngle = (acos(DotProd(normList[a],norm)));
								else newAngle = (acos(DotProd(normList[a],normList[b])));
								if (newAngle <= angle)
									{
									sel.Set(a);
									numberWorkableEdges++;
									
									}
								else
									{
									BorderClass tempData;
									tempData.edge = i;
									tempData.innerFace = b;
									tempData.outerFace = a;
									borderData.Append(1,&tempData);

									}
								}

							}
						}
					}
				}
			}

		}
	else if (md->mnMesh)
		{
//select it
		if (seedFaceIndex < md->mnMesh->numf)
			{
			if (sel.GetSize() != md->mnMesh->numf)
				sel.SetSize(md->mnMesh->numf);
			sel.Set(seedFaceIndex);
			
			borderData.ZeroCount();

//build egde list of all edges that have only one edge selected
			MNEdge *edges = md->mnMesh->E(0);
			int numberWorkableEdges = 1;
			while (numberWorkableEdges > 0)
				{
				numberWorkableEdges = 0;
				for (int i = 0; i < md->mnMesh->nume; i++)
					{
					int a = edges[i].f1;
					int b = edges[i].f2;

 					if ( (a>=0) && (b>=0) )
						{
						if (sel[a] && (!sel[b]))
							{
							float newAngle = 0.0f;
							if (!relative)
							{
								float dot = DotProd(normList[b],norm);
								if (dot < -0.99999f)
									newAngle = PI;
								else if (dot > 0.99999f)
									newAngle = 0.0f;
								else 
									newAngle = (acos(dot));
									
							}
							else
							{
								float dot = DotProd(normList[b],normList[a]);
								if (dot < -0.99999f)
									newAngle = PI;
								else if (dot > 0.99999f)
									newAngle = 0.0f;
								else 
									newAngle = (acos(dot));
							}
/*
							float newAngle;
							if (!relative)
								newAngle = (acos(DotProd(normList[b],norm)));
							else newAngle = (acos(DotProd(normList[b],normList[a])));
*/
							if (newAngle <= angle)
								{
								sel.Set(b);
								numberWorkableEdges++;
								
								}
							else
								{
								BorderClass tempData;
								tempData.edge = i;
								tempData.innerFace = a;
								tempData.outerFace = b;
								borderData.Append(1,&tempData);

								}
							}
						else if (sel[b] && (!sel[a]))
							{
/*
							float newAngle;
							if (!relative)
								newAngle = (acos(DotProd(normList[a],norm)));
							else newAngle = (acos(DotProd(normList[a],normList[b])));
*/
 							float newAngle = 0.0f;
							if (!relative)
							{
								float dot = DotProd(normList[a],norm);
								if (dot < -0.99999f)
									newAngle = PI;
								else if (dot > 0.99999f)
									newAngle = 0.0f;
								else 
									newAngle = (acos(dot));
							}
							else
							{
								float dot = DotProd(normList[a],normList[b]);
								if (dot < -0.99999f)
									newAngle = PI;
								else if (dot > 0.99999f)
									newAngle = 0.0f;
								else 
									newAngle = (acos(dot));
							}
							if (newAngle <= angle)
								{
								sel.Set(a);
								numberWorkableEdges++;
								
								}
							else
								{

								BorderClass tempData;
								tempData.edge = i;
								tempData.innerFace = b;
								tempData.outerFace = a;
								borderData.Append(1,&tempData);

								}
							}

						}
					}
				}
			}
		}


		

	}


void UnwrapMod::fnSelectPolygonsUpdate(BitArray *sel, BOOL update)
	{
	ModContextList mcList;		
	INodeTab nodes;

	MeshTopoData *md =NULL;

	if (ip) 
		{
		ip->GetModContexts(mcList,nodes);

		int objects = mcList.Count();
		md = (MeshTopoData*)mcList[0]->localData;
		}
	else md = GetModData();

	int ct = 0;
	if (md)
		{
//		MeshTopoData *md = (MeshTopoData*)mcList[0]->localData;

		if (md == NULL) 
			{
			return;
			}


		md->faceSel.ClearAll();
		for (int i =0; i < md->faceSel.GetSize(); i++)
			{
			if (i < sel->GetSize())
				{
				if ((*sel)[i]) md->faceSel.Set(i);
				}
			}
		UpdateFaceSelection(md->faceSel);
		if (update)
			{
			NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
			InvalidateView();
			}
		else
			{
			if (md->GetMesh())
				MeshUpdateGData(md->GetMesh(), md->faceSel);
			if (md->GetPatch())
				PatchUpdateGData(md->GetPatch(), md->faceSel);
			if (md->GetMNMesh())
				PolyUpdateGData(md->GetMNMesh(), md->faceSel);
			}
		}

	}

void	UnwrapMod::fnSelectFacesByNormal(Point3 normal, float angleThreshold, BOOL update)
	{
	//check for type
	ModContextList mcList;		
	INodeTab nodes;

	if (!ip) return;
	ip->GetModContexts(mcList,nodes);

	int objects = mcList.Count();


	if (objects != 0)
		{
		MeshTopoData *md = (MeshTopoData*)mcList[0]->localData;

		if (md == NULL) 
			{
			return;
			}


		BitArray sel;
		Tab<Point3> normList;
		normList.ZeroCount();
		SelectFacesByNormals( md,sel, normal, angleThreshold,normList);
		fnSelectPolygonsUpdate(&sel,update);
		}
	}

void	UnwrapMod::fnSelectClusterByNormal( float angleThreshold, int seedIndex, BOOL relative, BOOL update)
	{
	//check for type
	ModContextList mcList;		
	INodeTab nodes;

	if (!ip) return;
	ip->GetModContexts(mcList,nodes);

	int objects = mcList.Count();
	seedIndex--;


	if (objects != 0)
		{
		MeshTopoData *md = (MeshTopoData*)mcList[0]->localData;

		if (md == NULL) 
			{
			return;
			}

		BitArray sel;

		Tab<BorderClass> dummy;
		Tab<Point3> normList;
		normList.ZeroCount();
		BuildNormals(md, normList);
		if ((seedIndex >= 0) && (seedIndex <normList.Count()))
			{
			Point3 normal = normList[seedIndex];
			SelectFacesByGroup( md,sel,seedIndex, normal, angleThreshold, relative,normList,dummy);
			fnSelectPolygonsUpdate(&sel,update);
			}
		}
	}



BOOL	UnwrapMod::fnGetLimitSoftSel()
	{
	return limitSoftSel;
	}

void	UnwrapMod::fnSetLimitSoftSel(BOOL limit)
	{
	limitSoftSel = limit;
	RebuildDistCache();
	NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
	InvalidateView();

	}

int		UnwrapMod::fnGetLimitSoftSelRange()
	{
	return limitSoftSelRange;
	}

void	UnwrapMod::fnSetLimitSoftSelRange(int range)
	{
	limitSoftSelRange = range;
	RebuildDistCache();
	NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
	InvalidateView();
	}


float	UnwrapMod::fnGetVertexWeight(int index)
	{
	float v = 0.0f;
	index--;

	if ((index >=0) && (index <TVMaps.v.Count()))
		v = TVMaps.v[index].influence;
	return v;
	}
void	UnwrapMod::fnSetVertexWeight(int index,float weight)
	{
	index--;
	if ((index >=0) && (index <TVMaps.v.Count()))
		{
		TVMaps.v[index].influence = weight;
		TVMaps.v[index].flags  |= FLAG_WEIGHTMODIFIED;
		NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
		InvalidateView();
		}

	}

BOOL	UnwrapMod::fnIsWeightModified(int index)
	{
	index--;
	BOOL mod = FALSE;
	if ((index >=0) && (index <TVMaps.v.Count()))
		{

		mod = (TVMaps.v[index].flags & FLAG_WEIGHTMODIFIED);
		}
	
	return mod;

	}
void	UnwrapMod::fnModifyWeight(int index, BOOL modified)
	{
	index--;
	if ((index >=0) && (index <TVMaps.v.Count()))
		{
		if (modified)
			TVMaps.v[index].flags |= FLAG_WEIGHTMODIFIED;
		else TVMaps.v[index].flags &= ~FLAG_WEIGHTMODIFIED;
		NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
		InvalidateView();
		}


	}

BOOL	UnwrapMod::fnGetGeomElemMode()
	{
	return geomElemMode;
	}

void	UnwrapMod::fnSetGeomElemMode(BOOL elem)
	{
	geomElemMode = elem;
	CheckDlgButton(hSelRollup,IDC_SELECTELEMENT_CHECK,geomElemMode);
	}

void	UnwrapMod::SelectGeomElement(MeshTopoData* tmd, BOOL addSel, BitArray *originalSel)
{

	BitArray holdSel = tmd->faceSel;
	
	if (!addSel && (originalSel != NULL))
	{
		BitArray oSel = *originalSel;
		tmd->faceSel = (~tmd->faceSel) & oSel;
	}
	Tab<BorderClass> dummy;
	Tab<Point3> normList;
	normList.ZeroCount();
	BuildNormals(tmd, normList);
	BitArray tempSel;
	tempSel.SetSize(tmd->faceSel.GetSize());

 	tempSel = tmd->faceSel;

	for (int i =0; i < tmd->faceSel.GetSize(); i++)
		{
		if ((tempSel[i]) && (i >= 0) && (i <normList.Count()))
			{
			BitArray sel;
			Point3 normal = normList[i];
			SelectFacesByGroup( tmd,sel,i, normal, 180.0f, FALSE,normList,dummy);
			tmd->faceSel |= sel;
			for (int j = 0; j < tempSel.GetSize(); j++)
				{
				if (sel[j]) tempSel.Set(j,FALSE);
				}
			}
		}

	if (!addSel && (originalSel != NULL))
	{
		BitArray oSel = *originalSel;
		tmd->faceSel =  oSel & (~tmd->faceSel);
	}

}




void	UnwrapMod::SelectGeomFacesByAngle(MeshTopoData* tmd)
	{


	Tab<BorderClass> dummy;
	Tab<Point3> normList;
	normList.ZeroCount();
	BuildNormals(tmd, normList);
	BitArray tempSel;
	tempSel.SetSize(tmd->faceSel.GetSize());

	tempSel = tmd->faceSel;

	for (int i =0; i < tmd->faceSel.GetSize(); i++)
		{
		if ((tempSel[i]) && (i >= 0) && (i <normList.Count()))
			{
			BitArray sel;
			Point3 normal = normList[i];
			SelectFacesByGroup( tmd,sel,i, normal, planarThreshold, FALSE,normList,dummy);
			tmd->faceSel |= sel;
			for (int j = 0; j < tempSel.GetSize(); j++)
				{
				if (sel[j]) tempSel.Set(j,FALSE);
				}
			}
		}
	UpdateFaceSelection(tmd->faceSel);


	}

BOOL	UnwrapMod::fnGetGeomPlanarMode()
	{
	return planarMode ;
	}

void	UnwrapMod::fnSetGeomPlanarMode(BOOL planar)
	{
	planarMode = planar;
//update UI
	CheckDlgButton(hSelRollup,IDC_PLANARANGLE_CHECK,planarMode);
	}

float	UnwrapMod::fnGetGeomPlanarModeThreshold()
	{
	return planarThreshold;
	}

void	UnwrapMod::fnSetGeomPlanarModeThreshold(float threshold)
	{
	planarThreshold = threshold;
//update UI
	if (iPlanarThreshold)
		iPlanarThreshold->SetValue(fnGetGeomPlanarModeThreshold(),TRUE);
	}


void	UnwrapMod::fnSelectByMatID(int matID)
	{
	//check for type
	ModContextList mcList;		
	INodeTab nodes;

	if (!ip) return;
	ip->GetModContexts(mcList,nodes);

	int objects = mcList.Count();

	matID--;
	if (objects != 0)
		{

		MeshTopoData *tmd = (MeshTopoData*)mcList[0]->localData;

		if (tmd == NULL) 
			{
			return;
			}

		

		theHold.Begin();
		theHold.Put (new UnwrapSelRestore (this,tmd));
		theHold.Accept (GetString (IDS_DS_SELECT));

		tmd->faceSel.ClearAll();
		for (int i = 0; i < TVMaps.f.Count(); i++)
			{
			if ( (TVMaps.f[i]->MatID == matID) && (i < tmd->faceSel.GetSize()))
				tmd->faceSel.Set(i);
			}
		UpdateFaceSelection(tmd->faceSel);

		theHold.Suspend();
		if (fnGetSyncSelectionMode()) fnSyncTVSelection();
		theHold.Resume();

	
		ComputeSelectedFaceData();
		NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
		InvalidateView();
		ip->RedrawViews(ip->GetTime());

		}
	}

void	UnwrapMod::fnSelectBySG(int sg)
	{

	//check for type
	ModContextList mcList;		
	INodeTab nodes;

	if (!ip) return;
	ip->GetModContexts(mcList,nodes);

	int objects = mcList.Count();

	sg--;

	if (objects != 0)
		{

		MeshTopoData *tmd = (MeshTopoData*)mcList[0]->localData;

		if (tmd == NULL) 
			{
			return;
			}

		theHold.Begin();

		theHold.Put (new UnwrapSelRestore (this,tmd));

		tmd->faceSel.ClearAll();

		if ((objType == IS_MESH) && (!tmd->mesh))
			{
			updateCache = TRUE;
			NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
			}
		else if ((objType == IS_PATCH) && (!tmd->patch))
			{
			updateCache = TRUE;
			NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
			}
		else if ((objType == IS_MNMESH) && (!tmd->mnMesh))
			{
			updateCache = TRUE;
			NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
			}

		sg = 1 << sg;
		if(tmd->mesh)
			{
			for (int i =0; i < tmd->mesh->numFaces; i++)
				{
				if ( (tmd->mesh->faces[i].getSmGroup() & sg) && (i < tmd->faceSel.GetSize()) )
					tmd->faceSel.Set(i);
				}
			}
		else if (tmd->mnMesh)
			{
			for (int i =0; i < tmd->mnMesh->numf; i++)
				{
				if ( (tmd->mnMesh->f[i].smGroup & sg) && (i < tmd->faceSel.GetSize()) )
					tmd->faceSel.Set(i);
				}
			}
		else if (tmd->patch)
			{
			for (int i =0; i < tmd->patch->numPatches; i++)
				{
				if ( (tmd->patch->patches[i].smGroup & sg) && (i < tmd->faceSel.GetSize()) )
					tmd->faceSel.Set(i);
				}
			}
		


		UpdateFaceSelection(tmd->faceSel);

		if (fnGetSyncSelectionMode()) fnSyncTVSelection();
		
		theHold.Accept (GetString (IDS_DS_SELECT));
		ComputeSelectedFaceData();
		NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
		InvalidateView();
		ip->RedrawViews(ip->GetTime());
		}


	}



void	UnwrapMod::GeomExpandFaceSel(MeshTopoData* tmd)
	{

	BitArray selectedVerts;
	
	if ((objType == IS_MESH) && (tmd->mesh))
		{
		selectedVerts.SetSize(tmd->mesh->getNumVerts());
		selectedVerts.ClearAll();
		for (int i =0; i < tmd->mesh->getNumFaces(); i++)
			{
			if (tmd->faceSel[i])
				{
				for (int j = 0;j < 3; j++)
					{
					int index = tmd->mesh->faces[i].v[j];
					selectedVerts.Set(index);
					}
				}
			}
		for (int i =0; i < tmd->mesh->getNumFaces(); i++)
			{
			if (!tmd->faceSel[i])
				{
				for (int j = 0;j < 3; j++)
					{
					int index = tmd->mesh->faces[i].v[j];
					if (selectedVerts[index])
						tmd->faceSel.Set(i);

					}
				}
			}

		}
	else if ((objType == IS_PATCH) && (tmd->patch))
		{
		selectedVerts.SetSize(tmd->patch->getNumVerts());
		selectedVerts.ClearAll();
		for (int i =0; i < tmd->patch->getNumPatches(); i++)
			{
			if (tmd->faceSel[i])
				{
				int ct = 4;
				if (tmd->patch->patches[i].type == PATCH_TRI) ct = 3;
				for (int j = 0;j < ct; j++)
					{
					int index = tmd->patch->patches[i].v[j];
					selectedVerts.Set(index);
					}
				}
			}
		for (int i =0; i < tmd->patch->getNumPatches(); i++)
			{
			if (!tmd->faceSel[i])
				{
				int ct = 4;
				if (tmd->patch->patches[i].type == PATCH_TRI) ct = 3;
				for (int j = 0;j < ct; j++)
					{
					int index = tmd->patch->patches[i].v[j];
					if (selectedVerts[index])
						tmd->faceSel.Set(i);

					}
				}
			}
		}
	else if ((objType == IS_MNMESH) && (tmd->mnMesh))
		{
		selectedVerts.SetSize(tmd->mnMesh->numv);
		selectedVerts.ClearAll();
		for (int i =0; i < tmd->mnMesh->numf; i++)
			{
			if (tmd->faceSel[i])
				{
				int ct = tmd->mnMesh->f[i].deg;
				for (int j = 0;j < ct; j++)
					{
					int index = tmd->mnMesh->f[i].vtx[j];
					selectedVerts.Set(index);
					}
				}
			}
		for (int i =0; i < tmd->mnMesh->numf; i++)
			{
			if (!tmd->faceSel[i])
				{
				int ct = tmd->mnMesh->f[i].deg;
				for (int j = 0;j < ct; j++)
					{
					int index = tmd->mnMesh->f[i].vtx[j];
					if (selectedVerts[index])
						tmd->faceSel.Set(i);

					}
				}
			}
		}

	}
void	UnwrapMod::GeomContractFaceSel(MeshTopoData* tmd)
	{
	BitArray unselectedVerts;
	
	if ((objType == IS_MESH) && (tmd->mesh))
		{
		unselectedVerts.SetSize(tmd->mesh->getNumVerts());
		unselectedVerts.ClearAll();
		for (int i =0; i < tmd->mesh->getNumFaces(); i++)
			{
			if (!tmd->faceSel[i])
				{
				for (int j = 0;j < 3; j++)
					{
					int index = tmd->mesh->faces[i].v[j];
					unselectedVerts.Set(index);
					}
				}
			}
		for (int i =0; i < tmd->mesh->getNumFaces(); i++)
			{
			if (tmd->faceSel[i])
				{
				for (int j = 0;j < 3; j++)
					{
					int index = tmd->mesh->faces[i].v[j];
					if (unselectedVerts[index])
						tmd->faceSel.Set(i,FALSE);

					}
				}
			}

		}
	else if ((objType == IS_PATCH) && (tmd->patch))
		{
		unselectedVerts.SetSize(tmd->patch->getNumVerts());
		unselectedVerts.ClearAll();
		for (int i =0; i < tmd->patch->getNumPatches(); i++)
			{
			if (!tmd->faceSel[i])
				{
				int ct = 4;
				if (tmd->patch->patches[i].type == PATCH_TRI) ct = 3;
				for (int j = 0;j < ct; j++)
					{
					int index = tmd->patch->patches[i].v[j];
					unselectedVerts.Set(index);
					}
				}
			}
		for (int i =0; i < tmd->patch->getNumPatches(); i++)
			{
			if (tmd->faceSel[i])
				{
				int ct = 4;
				if (tmd->patch->patches[i].type == PATCH_TRI) ct = 3;
				for (int j = 0;j < ct; j++)
					{
					int index = tmd->patch->patches[i].v[j];
					if (unselectedVerts[index])
						tmd->faceSel.Set(i,FALSE);

					}
				}
			}
		}
	else if ((objType == IS_MNMESH) && (tmd->mnMesh))
		{
		unselectedVerts.SetSize(tmd->mnMesh->numv);
		unselectedVerts.ClearAll();
		for (int i =0; i < tmd->mnMesh->numf; i++)
			{
			if (!tmd->faceSel[i])
				{
				int ct = tmd->mnMesh->f[i].deg;
				for (int j = 0;j < ct; j++)
					{
					int index = tmd->mnMesh->f[i].vtx[j];
					unselectedVerts.Set(index);
					}
				}
			}
		for (int i =0; i < tmd->mnMesh->numf; i++)
			{
			if (tmd->faceSel[i])
				{
				int ct = tmd->mnMesh->f[i].deg;
				for (int j = 0;j < ct; j++)
					{
					int index = tmd->mnMesh->f[i].vtx[j];
					if (unselectedVerts[index])
						tmd->faceSel.Set(i,FALSE);

					}
				}
			}
		}

	}
void	UnwrapMod::fnGeomExpandFaceSel()
	{
	//check for type
	ModContextList mcList;		
	INodeTab nodes;

	if (!ip) return;
	ip->GetModContexts(mcList,nodes);

	int objects = mcList.Count();

	if (objects != 0)
		{

		MeshTopoData *tmd = (MeshTopoData*)mcList[0]->localData;

		if (tmd == NULL) 
			{
			return;
			}

		theHold.Begin();

		theHold.Put (new UnwrapSelRestore (this,tmd));


		if ((objType == IS_MESH) && (!tmd->mesh))
			{
			updateCache = TRUE;
			NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
			}
		else if ((objType == IS_PATCH) && (!tmd->patch))
			{
			updateCache = TRUE;
			NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
			}
		else if ((objType == IS_MNMESH) && (!tmd->mnMesh))
			{
			updateCache = TRUE;
			NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
			}

		GeomExpandFaceSel(tmd);

		if (fnGetSyncSelectionMode()) fnSyncTVSelection();

		UpdateFaceSelection(tmd->faceSel);

		theHold.Accept (GetString (IDS_DS_SELECT));
		ComputeSelectedFaceData();
		NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
		InvalidateView();
		ip->RedrawViews(ip->GetTime());
		}
	}
void	UnwrapMod::fnGeomContractFaceSel()
	{
	//check for type
	ModContextList mcList;		
	INodeTab nodes;

	if (!ip) return;
	ip->GetModContexts(mcList,nodes);

	int objects = mcList.Count();

	if (objects != 0)
		{

		MeshTopoData *tmd = (MeshTopoData*)mcList[0]->localData;

		if (tmd == NULL) 
			{
			return;
			}

		theHold.Begin();

		theHold.Put (new UnwrapSelRestore (this,tmd));

		if ((objType == IS_MESH) && (!tmd->mesh))
			{
			updateCache = TRUE;
			NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
			}
		else if ((objType == IS_PATCH) && (!tmd->patch))
			{
			updateCache = TRUE;
			NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
			}
		else if ((objType == IS_MNMESH) && (!tmd->mnMesh))
			{
			updateCache = TRUE;
			NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
			}

		GeomContractFaceSel(tmd);
	
		if (fnGetSyncSelectionMode()) fnSyncTVSelection();

		UpdateFaceSelection(tmd->faceSel);

		theHold.Accept (GetString (IDS_DS_SELECT));
		ComputeSelectedFaceData();
		NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
		InvalidateView();
		ip->RedrawViews(ip->GetTime());
		}
	}



BitArray* UnwrapMod::fnGetSelectedVerts()
//int fnGetSelectedVerts()
	{
	return &vsel;
	}

void UnwrapMod::fnSelectVerts(BitArray *sel)
	{
	vsel.ClearAll();
	for (int i =0; i < vsel.GetSize(); i++)
		{
		if (i < sel->GetSize())
			{
			if ((*sel)[i]) vsel.Set(i);
			}
		}

	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

	InvalidateView();
	}

BOOL UnwrapMod::fnIsVertexSelected(int index)
	{
	index--;
	if (index < vsel.GetSize())
		return vsel[index];

	return FALSE;
	}



BitArray* UnwrapMod::fnGetSelectedFaces()
	{
	return &fsel;
	}

void	UnwrapMod::fnSelectFaces(BitArray *sel)
	{
	fsel.ClearAll();
	for (int i =0; i < fsel.GetSize(); i++)
		{
		if (i < sel->GetSize())
			{
			if ((*sel)[i]) fsel.Set(i);
			}
		}
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
	InvalidateView();
	}

BOOL	UnwrapMod::fnIsFaceSelected(int index)
	{
	index--;
	if (index < fsel.GetSize())
		return fsel[index];

	return FALSE;
	}

void    UnwrapMod::GetVertSelFromFace(BitArray &sel)
	{
	sel.SetSize(TVMaps.v.Count());
	sel.ClearAll();
	for (int i = 0; i < TVMaps.f.Count(); i++)
		{
		if (fsel[i])
			{
			int pcount = 3;
			pcount = TVMaps.f[i]->count;
			for (int k = 0; k < pcount; k++)
				{
				int index = TVMaps.f[i]->t[k];
				sel.Set(index);
				if ((TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[i]->vecs))
					{
					index = TVMaps.f[i]->vecs->handles[k*2];
					if ((index >= 0) && (index < sel.GetSize()))
						sel.Set(index);

					index = TVMaps.f[i]->vecs->handles[k*2+1];
					if ((index >= 0) && (index < sel.GetSize()))
						sel.Set(index);
	
					if (TVMaps.f[i]->flags & FLAG_INTERIOR) 
						{
						index = TVMaps.f[i]->vecs->interiors[k];
						if ((index >= 0) && (index < sel.GetSize()))
							sel.Set(index);
						}
					}

				}
			}

		}
	}

void    UnwrapMod::GetFaceSelFromVert(BitArray &sel, BOOL partialSelect)
	{
	sel.SetSize(TVMaps.f.Count());
	sel.ClearAll();	
	for (int i = 0; i < TVMaps.f.Count(); i++)
		{
		int pcount = 3;
		pcount = TVMaps.f[i]->count;
		int total = 0;
		for (int k = 0; k < pcount; k++)
			{
			int index = TVMaps.f[i]->t[k];
			if (vsel[index]) total++;
			if ((TVMaps.f[i]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[i]->vecs))
				{
				index = TVMaps.f[i]->vecs->handles[k*2];
				if ((index >= 0) && (index < sel.GetSize()))
					{
					if (vsel[index]) total++;
					}

				index = TVMaps.f[i]->vecs->handles[k*2+1];
				if ((index >= 0) && (index < sel.GetSize()))
					{
					if (vsel[index]) total++;
					}
	
				if (TVMaps.f[i]->flags & FLAG_INTERIOR) 
					{
					index = TVMaps.f[i]->vecs->interiors[k];
					if ((index >= 0) && (index < sel.GetSize()))
						{
						if (vsel[index]) 
							total++;
						}
					}

				}
			}
		if ((partialSelect) && (total))
			sel.Set(i);
		else if ((!partialSelect) && (total >= pcount))
			sel.Set(i);

		}
	}




void	UnwrapMod::TransferSelectionStart()
	{
	
	originalSel.SetSize(vsel.GetSize());
	originalSel = vsel;

	holdFSel.SetSize(fsel.GetSize());
	holdFSel = fsel;

	holdESel.SetSize(esel.GetSize());
	holdESel = esel;


	if (fnGetTVSubMode() == TVVERTMODE)
		{
		}
	else if (fnGetTVSubMode() == TVEDGEMODE)
		{
		GetVertSelFromEdge(vsel);
		}
	else if (fnGetTVSubMode() == TVFACEMODE)
		{
		GetVertSelFromFace(vsel);
		}
	}	

		//this transfer our vertex selection into our curren selection
void	UnwrapMod::TransferSelectionEnd(BOOL partial,BOOL recomputeSelection)
	{


	if (fnGetTVSubMode() == TVVERTMODE)
		{
		//is vertex selection do not need to do anything
		}
	else if (fnGetTVSubMode() == TVEDGEMODE) //face mode
		{
		//need to convert our vertex selection to face
		if ( (recomputeSelection)  || (holdESel.GetSize() != TVMaps.ePtrList.Count()))
			GetEdgeSelFromVert(esel,partial);
		else esel = holdESel;
		//now we need to restore the vertex selection
		vsel = originalSel;
		}
	else if (fnGetTVSubMode() == TVFACEMODE) //face mode
		{
		//need to convert our vertex selection to face
		if (recomputeSelection) GetFaceSelFromVert(fsel,partial);
		else fsel = holdFSel;
		//now we need to restore the vertex selection as long as we have not changed topo
		if (vsel.GetSize() == originalSel.GetSize())
			vsel = originalSel;
		}


	}


BitArray* UnwrapMod::fnGetSelectedEdges()
	{
	return &esel;
	}

void	UnwrapMod::fnSelectEdges(BitArray *sel)
	{
	esel.ClearAll();
	for (int i =0; i < esel.GetSize(); i++)
		{
		if (i < sel->GetSize())
			{
			if ((*sel)[i]) esel.Set(i);
			}
		}
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
	InvalidateView();
	}

BOOL	UnwrapMod::fnIsEdgeSelected(int index)
	{
	index--;
	if (index < esel.GetSize())
		return esel[index];

	return FALSE;
	}


		//Converts the edge selection into a vertex selection set
void    UnwrapMod::GetVertSelFromEdge(BitArray &sel)
	{
	sel.SetSize(TVMaps.v.Count());
	sel.ClearAll();
	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
		{
		if (esel[i])
			{
			int index = TVMaps.ePtrList[i]->a;
			if ((index >= 0) && (index < sel.GetSize()))
				sel.Set(index);

			index = TVMaps.ePtrList[i]->b;
			if ((index >= 0) && (index < sel.GetSize()))
				sel.Set(index);

			index = TVMaps.ePtrList[i]->avec;
			if ((index >= 0) && (index < sel.GetSize()))
				sel.Set(index);

			index = TVMaps.ePtrList[i]->bvec;
			if ((index >= 0) && (index < sel.GetSize()))
				sel.Set(index);


			}

		}
	}
		//Converts the vertex selection into a edge selection set
		//PartialSelect determines whether all the vertices of a edge need to be selected for that edge to be selected
void    UnwrapMod::GetEdgeSelFromVert(BitArray &sel,BOOL partialSelect)
	{
	sel.SetSize(esel.GetSize());
	sel.ClearAll();
	for (int i =0; i < TVMaps.ePtrList.Count(); i++)
		{
		int a,b;
		a = TVMaps.ePtrList[i]->a;
		b = TVMaps.ePtrList[i]->b;
		if (partialSelect)
			{
			if (vsel[a] || vsel[b])
				sel.Set(i);
			}
		else
			{
			if (vsel[a] && vsel[b])
				sel.Set(i);
			}

		}
	}


BOOL	UnwrapMod::fnGetUVEdgeMode()
	{
	return uvEdgeMode;
	}
void	UnwrapMod::fnSetUVEdgeMode(BOOL uvmode)	
	{
	if (uvmode)
		{
		tvElementMode = FALSE;
		openEdgeMode = FALSE;
		}
	 uvEdgeMode = uvmode;
	}

BOOL	UnwrapMod::fnGetTVElementMode()
	{
	return tvElementMode;
	}
void	UnwrapMod::fnSetTVElementMode(BOOL mode)
	{
	if (mode)
		{
		uvEdgeMode = FALSE;
		openEdgeMode = FALSE;
		}
	tvElementMode =mode;
	MoveScriptUI();
	if ( (ip) &&(hWnd) )
		{	
		IMenuBarContext* pContext = (IMenuBarContext*) GetCOREInterface()->GetMenuManager()->GetContext(kUnwrapMenuBar);
		if (pContext)
			pContext->UpdateWindowsMenu();
		}
	}

void	UnwrapMod::SelectUVEdge()
	{		
	BitArray originalESel;
	originalESel.SetSize(esel.GetSize());
	originalESel = esel;

//look for open edges first
	int edgeCount = esel.GetSize();
	
		
	int eselSet = 0;

	while (eselSet!= esel.NumberSet())
		{
		eselSet =  esel.NumberSet();
		GrowSelectUVEdge();
				//get connecting a edge
		}
	}


void	UnwrapMod::SelectOpenEdge()
	{		
	BitArray originalESel;
	originalESel.SetSize(esel.GetSize());
	originalESel = esel;

//look for open edges first
	int edgeCount = esel.GetSize();
	
		
	int eselSet = 0;

	while (eselSet!= esel.NumberSet())
		{
		eselSet =  esel.NumberSet();
		GrowSelectOpenEdge();
				//get connecting a edge
		}
	}


void	UnwrapMod::GrowSelectOpenEdge()
	{

	int edgeCount = esel.GetSize();

	Tab<int> edgeCounts;
	edgeCounts.SetCount(TVMaps.v.Count());
	for (int i = 0; i < TVMaps.v.Count(); i++)
		edgeCounts[i] = 0;
	for (int i = 0; i < edgeCount; i++)
		{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		if (esel[i])
			{
			edgeCounts[a]++;
			edgeCounts[b]++;
			}
		}
	for (int i = 0; i < edgeCount; i++)
		{
		if ( (!esel[i]) && (TVMaps.ePtrList[i]->faceList.Count() == 1) )
			{
			int a = TVMaps.ePtrList[i]->a;
			int b = TVMaps.ePtrList[i]->b;
			int aCount = edgeCounts[a];
			int bCount = edgeCounts[b];
			if ( ( ( aCount == 0) && (bCount >= 1) ) ||
				 ( (bCount == 0) && (aCount >= 1) ) ||
				 ( (bCount >= 1) && (aCount >= 1) ) )
				esel.Set(i,TRUE);
			}
		}
	}

void	UnwrapMod::ShrinkSelectOpenEdge()
	{


	int edgeCount = esel.GetSize();

	Tab<int> edgeCounts;
	edgeCounts.SetCount(TVMaps.v.Count());
	for (int i = 0; i < TVMaps.v.Count(); i++)
		edgeCounts[i] = 0;
	for (int i = 0; i < edgeCount; i++)
		{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		if (esel[i])
			{
			edgeCounts[a]++;
			edgeCounts[b]++;
			}
		}
	for (int i = 0; i < edgeCount; i++)
		{
		if ( (esel[i])  && (TVMaps.ePtrList[i]->faceList.Count() == 1) )
			{
			int a = TVMaps.ePtrList[i]->a;
			int b = TVMaps.ePtrList[i]->b;
			if ( (edgeCounts[a] == 1) || (edgeCounts[b] == 1) )
				esel.Set(i,FALSE);
			}
		}
	}


void	UnwrapMod::GrowSelectUVEdge()
 {
// get current edgesel
	Tab<int> edgeConnectedCount;
	Tab<int> numberEdgesAtVert;
	int edgeCount = esel.GetSize();
	int vertCount = vsel.GetSize();  edgeConnectedCount.SetCount(vertCount);
	numberEdgesAtVert.SetCount(vertCount);

	for (int i = 0; i < vertCount; i++)
		{
		edgeConnectedCount[i] = 0;
		numberEdgesAtVert[i] = 0;
		}

//find all the vertices that touch a selected edge
// and keep a count of the number of selected edges that touch that //vertex  
	for (int i = 0; i < edgeCount; i++)
		{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		if (a!=b)
			{

			if (!(TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA))
				{
				numberEdgesAtVert[a]++;
				numberEdgesAtVert[b]++;
				}
			if (esel[i])
				{
				edgeConnectedCount[a]++;
				edgeConnectedCount[b]++;
				}
			}
		}


	BitArray edgesToExpand;
	edgesToExpand.SetSize(edgeCount);
	edgesToExpand.ClearAll();

//tag any edge that has only one vertex count since that will be an end edge  
	for (int i = 0; i < edgeCount; i++)
		{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		if (a!=b)
			{
			if (esel[i])
				{
				if ((edgeConnectedCount[a] == 1) || (edgeConnectedCount[b] == 1))
					edgesToExpand.Set(i,TRUE);
				}
			}
		}

	for (int i = 0; i < edgeCount; i++)
		{
		if (edgesToExpand[i])
			{
//make sure we have an even number of edges at this vert
//if odd then we can not go further
//   if ((numberEdgesAtVert[i] % 2) == 0)
//now need to find our most opposing edge
//find all edges connected to the vertex

		    for (int k = 0; k < 2; k++)
				{
			    int a = 0;
			    if (k==0) a = TVMaps.ePtrList[i]->a;
				else a = TVMaps.ePtrList[i]->b;
		
				if ( (edgeConnectedCount[a] == 1) && ((numberEdgesAtVert[a] % 2) == 0))
					{
					int centerVert = a;
				    Tab<int> edgesConnectedToVert;
					for (int j = 0; j < edgeCount; j++)
						{
						if (j!=i)  //make sure we dont add our selected vertex
							{
							int a = TVMaps.ePtrList[j]->a;
							int b = TVMaps.ePtrList[j]->b;

							if (a!=b)
								{
								if ((a == centerVert) || (b==centerVert))
									{
									edgesConnectedToVert.Append(1,&j);
									}
								}
							}

						}
//get a face connected to our oririnal egd
					int faceIndex = TVMaps.ePtrList[i]->faceList[0];
				    int count = numberEdgesAtVert[centerVert]/2;
					int tally = 0;
					BOOL done = FALSE;
					while (!done)
						{
						int lastEdge = -1;


						for (int m = 0; m < edgesConnectedToVert.Count(); m++)
							{
							int edgeIndex = edgesConnectedToVert[m];
							for (int n = 0; n < TVMaps.ePtrList[edgeIndex]->faceList.Count(); n++)
								{
								if (faceIndex == TVMaps.ePtrList[edgeIndex]->faceList[n])
									{
									for (int p = 0; p < TVMaps.ePtrList[edgeIndex]->faceList.Count(); p++)
										{
										if (faceIndex != TVMaps.ePtrList[edgeIndex]->faceList[p])
											{
											faceIndex = TVMaps.ePtrList[edgeIndex]->faceList[p];
											p = TVMaps.ePtrList[edgeIndex]->faceList.Count();
											}
										}
									if (!(TVMaps.ePtrList[edgeIndex]->flags& FLAG_HIDDENEDGEA))
										tally++;
									edgesConnectedToVert.Delete(m,1);
									m = edgesConnectedToVert.Count();
									n = TVMaps.ePtrList[edgeIndex]->faceList.Count();

									lastEdge = edgeIndex;
									}
								}
							}
						if (lastEdge == -1)
							{
//					        assert(0);
					        done = TRUE;
							}
						if (tally >= count)
							{
							done = TRUE;
							if (lastEdge != -1)
								esel.Set(lastEdge,TRUE);
							}

						}
					}
				}
			}
		}	
	}
  


void	UnwrapMod::ShrinkSelectUVEdge()
	{


	int edgeCount = esel.GetSize();

	Tab<int> edgeCounts;
	edgeCounts.SetCount(TVMaps.v.Count());
	for (int i = 0; i < TVMaps.v.Count(); i++)
		edgeCounts[i] = 0;
	for (int i = 0; i < edgeCount; i++)
		{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		if (esel[i])
			{
			edgeCounts[a]++;
			edgeCounts[b]++;
			}
		}
	for (int i = 0; i < edgeCount; i++)
		{
		if ( (esel[i])  && (TVMaps.ePtrList[i]->faceList.Count() == 1) )
			{
			int a = TVMaps.ePtrList[i]->a;
			int b = TVMaps.ePtrList[i]->b;
			if ( (edgeCounts[a] == 1) || (edgeCounts[b] == 1) )
				esel.Set(i,FALSE);
			}
		}
	}


BOOL	UnwrapMod::fnGetOpenEdgeMode()
	{
	return openEdgeMode;
	}

void	UnwrapMod::fnSetOpenEdgeMode(BOOL mode)
	{
	if (mode)
		{
		uvEdgeMode = FALSE;
		tvElementMode =FALSE;
		}

	openEdgeMode = mode;
	}

void	UnwrapMod::fnUVEdgeSelect()
	{
	theHold.Begin();
	HoldSelection();

	SelectUVEdge();
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));	
	InvalidateView();			
	}

void	UnwrapMod::fnOpenEdgeSelect()
	{
	theHold.Begin();
	HoldSelection();

	SelectOpenEdge();
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));					
	InvalidateView();
	}


void	UnwrapMod::fnVertToEdgeSelect()
	{
	theHold.Begin();
	HoldSelection();

	GetEdgeSelFromVert(esel,FALSE);
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();
	theHold.Accept(GetString(IDS_DS_SELECT));	
	InvalidateView();
	}
void	UnwrapMod::fnVertToFaceSelect()
	{
	theHold.Begin();
	HoldSelection();

	GetFaceSelFromVert(fsel,FALSE);

	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

	theHold.Accept(GetString(IDS_DS_SELECT));	
	InvalidateView();
	}

void	UnwrapMod::fnEdgeToVertSelect()
	{
	theHold.Begin();
	HoldSelection();

	GetVertSelFromEdge(vsel);

	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

	theHold.Accept(GetString(IDS_DS_SELECT));	
	InvalidateView();	
	}
void	UnwrapMod::fnEdgeToFaceSelect()
	{
	theHold.Begin();
	HoldSelection();

	BitArray tempSel;
	tempSel.SetSize(vsel.GetSize());
	tempSel = vsel;
	GetVertSelFromEdge(vsel);
	GetFaceSelFromVert(fsel,FALSE);

	vsel = tempSel;

	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

	theHold.Accept(GetString(IDS_DS_SELECT));	
	InvalidateView();
	}

void	UnwrapMod::fnFaceToVertSelect()
	{
	theHold.Begin();
	HoldSelection();

	GetVertSelFromFace(vsel);

	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

	theHold.Accept(GetString(IDS_DS_SELECT));	
	InvalidateView();
	}
void	UnwrapMod::fnFaceToEdgeSelect()
	{
	theHold.Begin();
	HoldSelection();

	ConvertFaceToEdgeSel();
/*
	BitArray tempSel;
	tempSel.SetSize(vsel.GetSize());
	tempSel = vsel;
	GetVertSelFromFace(vsel);
	GetEdgeSelFromVert(esel,FALSE);

	vsel = tempSel;
*/
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

	theHold.Accept(GetString(IDS_DS_SELECT));	
	InvalidateView();
	}


void UnwrapMod::InitReverseSoftData()
	{
	RebuildDistCache();
	BitArray originalVSel(vsel);

	sketchBelongsToList.SetCount(TVMaps.v.Count());
	originalPos.SetCount(TVMaps.v.Count());

	for (int i = 0; i < TVMaps.v.Count(); i++)
		{
		sketchBelongsToList[i] = -1;
		originalPos[i] = TVMaps.v[i].p;
		}
	
	for (int i = 0; i < TVMaps.v.Count(); i++)
		{
		if ((!originalVSel[i]) && (TVMaps.v[i].influence > 0.0f))
			{
			int closest = -1;
			float closestDist = 0.0f;
			Point3 a = TVMaps.v[i].p;
			for (int j = 0; j < TVMaps.v.Count(); j++)
				{
				if (vsel[j])
					{
					Point3 b = TVMaps.v[j].p;
					float dist = Length(a-b);
					if ((dist < closestDist) || (closest == -1))
						{
						closest = j;
						closestDist = dist;
						}
					}
				}
			if (closest != -1)
				{
				sketchBelongsToList[i] = closest;
				}
			}
		}
	vsel = originalVSel;

	}
void UnwrapMod::ApplyReverseSoftData()
	{
	for (int i = 0; i < TVMaps.v.Count(); i++)
		{
		if (sketchBelongsToList[i] >= 0)
			{
			Point3 accumVec(0.0f,0.0f,0.0f);
			int index = sketchBelongsToList[i];
			Point3 vec = TVMaps.v[index].p - originalPos[index];
			accumVec += vec * TVMaps.v[i].influence;
	
			Point3 p = TVMaps.v[i].p + accumVec;
			TVMaps.v[i].p = p;
			if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(0,&TVMaps.v[i].p);
			}
		}
	}


int		UnwrapMod::fnGetHitSize()
	{
	return hitSize;
	}
void	UnwrapMod::fnSetHitSize(int size)
	{
	hitSize = size;
	}


BOOL	UnwrapMod::fnGetPolyMode()
	{
	return polyMode;
	}
void	UnwrapMod::fnSetPolyMode(BOOL pmode)
	{
	polyMode = pmode;
	}

void	UnwrapMod::ConvertFaceToEdgeSel()
	{
	esel.ClearAll();
	for (int i = 0; i < TVMaps.ePtrList.Count();i++)
		{
		for (int j = 0; j < TVMaps.ePtrList[i]->faceList.Count();j++)
			{
			int index = TVMaps.ePtrList[i]->faceList[j];
			if (fsel[index])
				{
				esel.Set(i);
				j = TVMaps.ePtrList[i]->faceList.Count();
				}
			}
		
		}

	}

void	UnwrapMod::fnPolySelect()
{
	fnPolySelect2(TRUE);
}

void	UnwrapMod::fnPolySelect2(BOOL add)
{

	BitArray originalESel(esel);
	BitArray originalVSel(vsel);

	BitArray tempSel;
	//convert to edge sel
	ConvertFaceToEdgeSel();

	//repeat until selection  not done
	int selCount = fsel.NumberSet();
	BOOL done= FALSE;
	int eSelCount = esel.GetSize();
	while (!done)
	{
		for (int i =0; i < eSelCount; i++)
		{
			if ( (esel[i])  && (TVMaps.ePtrList[i]->flags&FLAG_HIDDENEDGEA))
			{
				int ct = TVMaps.ePtrList[i]->faceList.Count();
				BOOL unselFace = FALSE;
				for (int j = 0; j < ct; j++)
				{
					int index = TVMaps.ePtrList[i]->faceList[j];
					if (add)
						fsel.Set(index);				
					else						
					{
						if (!fsel[index])
							unselFace = TRUE;

					}
				}
				if (unselFace && (!add))
				{
					for (int j = 0; j < ct; j++)
					{
						int index = TVMaps.ePtrList[i]->faceList[j];

						fsel.Set(index,FALSE);				
					}
				}
			}
		}

		if (selCount == fsel.NumberSet()) 
			done = TRUE;
		else
		{
			selCount = fsel.NumberSet();
			ConvertFaceToEdgeSel();

		}
	}

	esel = originalESel;
	vsel = originalVSel;

	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

	InvalidateView();

}


BOOL	UnwrapMod::fnGetSyncSelectionMode()
	{
		return TRUE;
//	return syncSelection;
	}

void	UnwrapMod::fnSetSyncSelectionMode(BOOL sync)
	{
	syncSelection = sync;
	}


void	UnwrapMod::SyncGeomToTVSelection(MeshTopoData *md)
	
	{
//get our geom face sel
	if (fnGetTVSubMode() == TVVERTMODE)
		{
			gvsel.ClearAll();
			//loop through our faces
			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
			//iterate through the faces
				int deg = TVMaps.f[i]->count;
				for (int j  = 0; j < deg; j++)
				{
			//get our geom index
					int geomIndex = TVMaps.f[i]->v[j];
			//get our tv index
					int tvIndex = TVMaps.f[i]->t[j];
			//if geom index is selected select the tv index
					if (vsel[tvIndex])
						gvsel.Set(geomIndex,TRUE);
				}
			}

		}
	else if (fnGetTVSubMode() == TVEDGEMODE)
		{
			Tab<GeomToTVEdges*> edgeInfo;
			edgeInfo.SetCount(TVMaps.f.Count());
			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				edgeInfo[i] = new GeomToTVEdges();
				int deg = TVMaps.f[i]->count;
				edgeInfo[i]->edgeInfo.SetCount(deg);
				for (int j = 0; j < deg; j++)
				{
					edgeInfo[i]->edgeInfo[j].gIndex = -1;
					edgeInfo[i]->edgeInfo[j].tIndex = -1;
				}
			}
			//loop through the geo edges
			for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
			{
				int numberOfFaces =  TVMaps.gePtrList[i]->faceList.Count();
				for (int j = 0; j < numberOfFaces; j++)
				{
					int faceIndex = TVMaps.gePtrList[i]->faceList[j];
					int a = TVMaps.gePtrList[i]->a;
					int b = TVMaps.gePtrList[i]->b;
					int ithEdge = TVMaps.f[faceIndex]->FindGeomEdge(a, b);
					edgeInfo[faceIndex]->edgeInfo[ithEdge].gIndex = i;

				}
			}

			//loop through the uv edges
			for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
			{
				int numberOfFaces =  TVMaps.ePtrList[i]->faceList.Count();
				for (int j = 0; j < numberOfFaces; j++)
				{
					int faceIndex = TVMaps.ePtrList[i]->faceList[j];
					int a = TVMaps.ePtrList[i]->a;
					int b = TVMaps.ePtrList[i]->b;
					int ithEdge = TVMaps.f[faceIndex]->FindUVEdge(a, b);
					edgeInfo[faceIndex]->edgeInfo[ithEdge].tIndex = i;

				}
			}

			gesel.ClearAll();
			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				
				int deg = TVMaps.f[i]->count;
				for (int j = 0; j < deg; j++)
				{
					int gIndex = edgeInfo[i]->edgeInfo[j].gIndex;
					int tIndex = edgeInfo[i]->edgeInfo[j].tIndex;
					if ( (gIndex>=0) && (gIndex < gesel.GetSize()) && 
						 (tIndex>=0) && (tIndex < esel.GetSize()) && 
						 (esel[tIndex]) )
						gesel.Set(gIndex,TRUE);

				}

			}

			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				delete edgeInfo[i];
			}
/*
		BitArray holdFaceSel(fsel);
		fsel = md->faceSel;

		BitArray tempSel(vsel);
		GetVertSelFromEdge(vsel);
		GetFaceSelFromVert(fsel,FALSE);

		vsel = tempSel;
		md->faceSel = fsel;
		fsel = holdFaceSel;
*/
		}
	else if (fnGetTVSubMode() == TVFACEMODE)
		{
		md->faceSel = fsel;
		}
	}

void	UnwrapMod::SyncTVToGeomSelection(MeshTopoData *md)
	{

	if (ip == NULL) return;
//get our geom face sel
	if (fnGetTVSubMode() == TVVERTMODE)
		{
			vsel.ClearAll();
			//loop through our faces
			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
			//iterate through the faces
				int deg = TVMaps.f[i]->count;
				for (int j  = 0; j < deg; j++)
				{
			//get our geom index
					int geomIndex = TVMaps.f[i]->v[j];
			//get our tv index
					int tvIndex = TVMaps.f[i]->t[j];
			//if geom index is selected select the tv index
					if (gvsel[geomIndex])
					{
						vsel.Set(tvIndex,TRUE);
						if (TVMaps.v[tvIndex].flags & FLAG_FROZEN)
						{
							vsel.Set(tvIndex,FALSE);
							gvsel.Set(geomIndex,FALSE);
						}
					}
				}
			}

/*
		BitArray holdFaceSel(fsel);
		fsel = md->faceSel;
		GetVertSelFromFace(vsel);
		fsel = holdFaceSel;
*/
		}
	else if (fnGetTVSubMode() == TVEDGEMODE)
		{
			Tab<GeomToTVEdges*> edgeInfo;
			edgeInfo.SetCount(TVMaps.f.Count());
			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				edgeInfo[i] = new GeomToTVEdges();
				int deg = TVMaps.f[i]->count;
				edgeInfo[i]->edgeInfo.SetCount(deg);
				for (int j = 0; j < deg; j++)
				{
					edgeInfo[i]->edgeInfo[j].gIndex = -1;
					edgeInfo[i]->edgeInfo[j].tIndex = -1;
				}
			}
			//loop through the geo edges
			for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
			{
				int numberOfFaces =  TVMaps.gePtrList[i]->faceList.Count();
				for (int j = 0; j < numberOfFaces; j++)
				{
					int faceIndex = TVMaps.gePtrList[i]->faceList[j];
					int a = TVMaps.gePtrList[i]->a;
					int b = TVMaps.gePtrList[i]->b;
					int ithEdge = TVMaps.f[faceIndex]->FindGeomEdge(a, b);
					edgeInfo[faceIndex]->edgeInfo[ithEdge].gIndex = i;

				}
			}

			//loop through the uv edges
			for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
			{
				int numberOfFaces =  TVMaps.ePtrList[i]->faceList.Count();
				for (int j = 0; j < numberOfFaces; j++)
				{
					int faceIndex = TVMaps.ePtrList[i]->faceList[j];
					int a = TVMaps.ePtrList[i]->a;
					int b = TVMaps.ePtrList[i]->b;
					int ithEdge = TVMaps.f[faceIndex]->FindUVEdge(a, b);
					edgeInfo[faceIndex]->edgeInfo[ithEdge].tIndex = i;

				}
			}

			esel.ClearAll();
			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				
				int deg = TVMaps.f[i]->count;
				for (int j = 0; j < deg; j++)
				{
					int gIndex = edgeInfo[i]->edgeInfo[j].gIndex;
					int tIndex = edgeInfo[i]->edgeInfo[j].tIndex;
					if ((gIndex != -1) &&(tIndex != -1))
					{
						if (gesel[gIndex])
						{
							int tva,tvb;
							tva = TVMaps.ePtrList[tIndex]->a;
							tvb = TVMaps.ePtrList[tIndex]->b;

							if ( (TVMaps.v[tva].flags & FLAG_FROZEN) ||
								(TVMaps.v[tvb].flags & FLAG_FROZEN) )
							{
								esel.Set(tIndex,FALSE);
								gesel.Set(gIndex,FALSE);
							}
							else esel.Set(tIndex,TRUE);
						}
					}

				}

			}

			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				delete edgeInfo[i];
			}

/*
		BitArray holdFaceSel(fsel);
		fsel = md->faceSel;
		ConvertFaceToEdgeSel();
		fsel = holdFaceSel;
*/
		}
	else if (fnGetTVSubMode() == TVFACEMODE)
		{
		fsel = md->faceSel;
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
				md->faceSel.Set(i,FALSE);
			}
		}
		}

	}


void	UnwrapMod::fnSyncTVSelection()
	{
	if (!ip) return;

	// LAM - 6/19/04 - defect 576948 
	BOOL wasHolding = theHold.Holding();
	if (!wasHolding )
		theHold.Begin();

	HoldSelection();



		//check for type
	ModContextList mcList;		
	INodeTab nodes;

	ip->GetModContexts(mcList,nodes);

	int objects = mcList.Count();


	if (objects != 0)
		{
		MeshTopoData *md = (MeshTopoData*)mcList[0]->localData;

		if (md == NULL) 
			{
			if (!wasHolding )
				theHold.Cancel();
			return;
			}


		SyncTVToGeomSelection(md);

		if (!wasHolding )
			theHold.Accept(GetString(IDS_DS_SELECT));					

		InvalidateView();
		}
	else 
		if (!wasHolding )
			theHold.Cancel();
	}

void	UnwrapMod::fnSyncGeomSelection()
	{

	if (!ip) return;

	// LAM - 6/19/04 - defect 576948 
	BOOL wasHolding = theHold.Holding();
	if (!wasHolding )
		theHold.Begin();

	HoldSelection();
		//check for type
	ModContextList mcList;		
	INodeTab nodes;

	ip->GetModContexts(mcList,nodes);

	int objects = mcList.Count();


	if (objects != 0)
		{
		MeshTopoData *md = (MeshTopoData*)mcList[0]->localData;

		if (md)
			{
			SyncGeomToTVSelection(md);

			if (!wasHolding )
				theHold.Accept(GetString(IDS_DS_SELECT));					
			InvalidateView();
			NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
			if (ip)	ip->RedrawViews(ip->GetTime());
			}
		else 
			if (!wasHolding )
				theHold.Cancel();
		}
	else 
		if (!wasHolding )
			theHold.Cancel();

	}


BOOL	UnwrapMod::fnGetPaintMode()
	{
	if (mode==ID_PAINTSELECTMODE)
		return TRUE;
	else return FALSE;
	}
void	UnwrapMod::fnSetPaintMode(BOOL paint)
	{
	if (paint)
		{
		if (ip) ip->ReplacePrompt( GetString(IDS_PW_PAINTSELECTPROMPT));
		SetMode(ID_PAINTSELECTMODE);
		paintSelectMode->first = TRUE;
		}
	else SetMode(oldMode);
	
	}


int		UnwrapMod::fnGetPaintSize()
	{
	return paintSize ;
	}

void	UnwrapMod::fnSetPaintSize(int size)
	{
	
	paintSize = size;

	if (paintSize < 1) paintSize = 1;
	

	}

void	UnwrapMod::fnIncPaintSize()
	{
	paintSize++;
	if (paintSize < 1) paintSize = 1;
	
	}
void	UnwrapMod::fnDecPaintSize()
	{
	paintSize--;
	if (paintSize < 1) paintSize = 1;
	
	}


void	UnwrapMod::fnSelectInvertedFaces()
{
	//see if face mode if not bail
	if (fnGetTVSubMode() != TVFACEMODE) return;

	theHold.Begin();
	HoldSelection();

	//clear our selection
	fsel.ClearAll();
	//loop through our faces
	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
	//get the normal of that face
		int deg = TVMaps.f[i]->count;
		
		BOOL hidden = FALSE;
		int hiddenCT = 0;
		for (int j = 0; j < deg; j++)
		{
			int index = TVMaps.f[i]->t[j];
			if ((TVMaps.v[index].flags & FLAG_FROZEN) || (!IsVertVisible(index)) )
				hiddenCT++;
		}

		if (hiddenCT == deg) hidden = TRUE;

		if (IsFaceVisible(i) && (!hidden))
		{
			
			Point3 vecA,vecB;
			int a,b;

			BOOL validFace = FALSE;
			for (int j = 0; j < deg; j++)
			{
				int a1,a2,a3;
				a1 = j - 1;
				a2 = j;
				a3 = j + 1;

				if (j == 0)
					a1 = deg-1;
				if (j == (deg-1))
					a3 = 0;

				a = TVMaps.f[i]->t[a2];
				b = TVMaps.f[i]->t[a1];
				vecA = Normalize(TVMaps.v[b].p - TVMaps.v[a].p);

				a = TVMaps.f[i]->t[a2];
				b = TVMaps.f[i]->t[a3];
				vecB = Normalize(TVMaps.v[b].p - TVMaps.v[a].p);
				float dot = DotProd(vecA,vecB);
				if (dot != 1.0f)
				{
					j = deg;				
					validFace = TRUE;
				}
				else
				{
					int c = 0;
				}
			}
			if (validFace)
			{
		//if it is negative flip it
				Point3 vec = CrossProd(vecA,vecB);
				if (vec.z >= 0.0f)
					fsel.Set(i,TRUE);
			}
		}
	}

	//put back the selection
	if (fnGetSyncSelectionMode()) fnSyncGeomSelection();

	theHold.Accept(_T(GetString(IDS_DS_SELECT)));
	InvalidateView();

}



void	UnwrapMod::fnGeomExpandEdgeSel()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(_T(GetString(IDS_DS_SELECT)));

	//get the an empty vertex selection
	BitArray tempVSel;
	tempVSel.SetSize(TVMaps.v.Count());
	tempVSel.ClearAll();
	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{
		if (gesel[i] && (!(TVMaps.gePtrList[i]->flags & FLAG_HIDDENEDGEA)))
		{
			int a = TVMaps.gePtrList[i]->a;
			tempVSel.Set(a,TRUE);
			a = TVMaps.gePtrList[i]->b;
			tempVSel.Set(a,TRUE);
		}
	}

	BitArray tempESel;
	tempESel.SetSize(TVMaps.gePtrList.Count());
	tempESel.ClearAll();
	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{
		if (!(TVMaps.gePtrList[i]->flags & FLAG_HIDDENEDGEA))
		{
			int a = TVMaps.gePtrList[i]->a;
			int b = TVMaps.gePtrList[i]->b;
			if (tempVSel[a] || tempVSel[b])
				tempESel.Set(i);
		}
	}
	gesel = tempESel;

	fnSyncTVSelection();

	InvalidateView();
	NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}
void	UnwrapMod::fnGeomContractEdgeSel()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(_T(GetString(IDS_DS_SELECT)));

	BitArray tempVSel;
	tempVSel.SetSize(TVMaps.v.Count());
	tempVSel.ClearAll();
	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{
		if ((gesel[i]) && (!(TVMaps.gePtrList[i]->flags & FLAG_HIDDENEDGEA)))
		{
			int a = TVMaps.gePtrList[i]->a;
			tempVSel.Set(a,TRUE);
			a = TVMaps.gePtrList[i]->b;
			tempVSel.Set(a,TRUE);
		}
	}

	BitArray tempVBorderSel;
	tempVBorderSel.SetSize(TVMaps.v.Count());
	tempVBorderSel.ClearAll();
	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{
		if (!(TVMaps.gePtrList[i]->flags & FLAG_HIDDENEDGEA))
		{
			int a = TVMaps.gePtrList[i]->a;
			int b = TVMaps.gePtrList[i]->b;
			if (tempVSel[a] && !tempVSel[b])
				tempVBorderSel.Set(a,TRUE);
			if (tempVSel[b] && !tempVSel[a])
				tempVBorderSel.Set(b,TRUE);			
		}
	}

	BitArray tempESel;
	tempESel.SetSize(TVMaps.gePtrList.Count());
	tempESel= gesel;
	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{
		int a = TVMaps.gePtrList[i]->a;
		int b = TVMaps.gePtrList[i]->b;
		if (tempVBorderSel[a] || tempVBorderSel[b])
			tempESel.Set(i,FALSE);
	}

	gesel = tempESel;

	fnSyncTVSelection();

	InvalidateView();
	NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}

void	UnwrapMod::fnGeomExpandVertexSel()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(_T(GetString(IDS_DS_SELECT)));

	//get the an empty vertex selection
	BitArray tempVSel;
	tempVSel.SetSize(TVMaps.geomPoints.Count());
	tempVSel.ClearAll();
	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{

			int a = TVMaps.gePtrList[i]->a;
			int b = TVMaps.gePtrList[i]->b;

			if (gvsel[a] || gvsel[b])
			{
				tempVSel.Set(a,TRUE);
				tempVSel.Set(b,TRUE);
			}
	}


	gvsel = gvsel | tempVSel;

	fnSyncTVSelection();

	InvalidateView();
	NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}
void	UnwrapMod::fnGeomContractVertexSel()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(_T(GetString(IDS_DS_SELECT)));

	//get the an empty vertex selection
	BitArray tempVSel = gvsel;
	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{

			int a = TVMaps.gePtrList[i]->a;
			int b = TVMaps.gePtrList[i]->b;

			if (gvsel[a] && (!gvsel[b]))
			{
				tempVSel.Set(a,FALSE);				
			}
			else if (gvsel[b] && (!gvsel[a]))
			{
				tempVSel.Set(b,FALSE);				
			}
	}


	gvsel =  tempVSel;

	fnSyncTVSelection();

	InvalidateView();
	NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}


void UnwrapMod::fnGeomLoopSelect()
{

	theHold.Begin();
	HoldSelection();
	theHold.Accept(_T(GetString(IDS_EDGELOOPSELECTION)));

	//get our selection
	//build a vertex connection list
	Tab<VEdges*> edgesAtVertex;
	edgesAtVertex.SetCount(TVMaps.geomPoints.Count());
	for (int i = 0; i < TVMaps.geomPoints.Count(); i++)
		edgesAtVertex[i] = NULL;

	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{
		
		{
			int a = TVMaps.gePtrList[i]->a;
	//			if (pointSel[a])
			{
				if (edgesAtVertex[a] == NULL)
					edgesAtVertex[a] = new VEdges();

					
				edgesAtVertex[a]->edgeIndex.Append(1,&i,5);
			}

			a = TVMaps.gePtrList[i]->b;
	//			if (pointSel[a])
			{
				if (edgesAtVertex[a] == NULL)
					edgesAtVertex[a] = new VEdges();

				edgesAtVertex[a]->edgeIndex.Append(1,&i,5);
			}

		}

	}



	//loop through our selection 
	BitArray tempGeSel;
	tempGeSel = gesel;
	tempGeSel.ClearAll();
	//get our start and end point
	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{
	//find the mid edge repeat until hit self or no mid edge
		if (gesel[i])
		{
			for (int j = 0; j < 2; j++)
			{
				int starVert = TVMaps.gePtrList[i]->a;
				if (j==1) starVert = TVMaps.gePtrList[i]->b;
				int startEdge = i;
				int currentEdge = i;
				BOOL done = FALSE;
				while (!done)
				{
					//get the number of visible edges at this vert
					int ct = 0;
					BOOL openEdge = FALSE;
					BOOL degenFound = FALSE;
					for (int k = 0; k < edgesAtVertex[starVert]->edgeIndex.Count(); k++)
					{
						int eindex = edgesAtVertex[starVert]->edgeIndex[k];
						if (TVMaps.gePtrList[eindex]->a == TVMaps.gePtrList[eindex]->b)
							degenFound = TRUE;
						if (TVMaps.gePtrList[eindex]->faceList.Count() == 1)
							openEdge = TRUE;
						if (!( TVMaps.gePtrList[eindex]->flags & FLAG_HIDDENEDGEA))
							ct++;
					}
					
					//if odd bail
					//if there is an open edge bail
					if (((ct%2) == 1) || openEdge || degenFound)
					{
						done = TRUE;
					}					
					else
					{
						int goalEdge = ct/2;
						int goalCount = 0;
						//now find our opposite edge 

						int currentFace = TVMaps.gePtrList[currentEdge]->faceList[0];
						while (goalCount != goalEdge)
						{
							//loop through our edges find the one that is connected to this face
							
							for (int k = 0; k < edgesAtVertex[starVert]->edgeIndex.Count(); k++)
							{
								int eindex = edgesAtVertex[starVert]->edgeIndex[k];
								if (eindex != currentEdge)
								{
									for (int m = 0; m < TVMaps.gePtrList[eindex]->faceList.Count(); m++)
									{
										if (TVMaps.gePtrList[eindex]->faceList[m] == currentFace)
										{
											currentEdge = eindex;
											if (!( TVMaps.gePtrList[eindex]->flags & FLAG_HIDDENEDGEA))
											{
												goalCount++;
											}
											m = TVMaps.gePtrList[eindex]->faceList.Count();
											k = edgesAtVertex[starVert]->edgeIndex.Count();
										}
									}
								}
							}
							//find our next face
							for (int m = 0; m < TVMaps.gePtrList[currentEdge]->faceList.Count(); m++)
							{
								if (TVMaps.gePtrList[currentEdge]->faceList[m]!=currentFace)
								{
									currentFace = TVMaps.gePtrList[currentEdge]->faceList[m];
									 m = TVMaps.gePtrList[currentEdge]->faceList.Count();
								}
							}
						}
						//set new edge
						//set the new vert
					}
					int a = TVMaps.gePtrList[currentEdge]->a;
					if (a == starVert)
						a = TVMaps.gePtrList[currentEdge]->b;
					starVert = a;

					tempGeSel.Set(currentEdge,TRUE);
					if (currentEdge == startEdge)
						done = TRUE;
					
				}
			}
		}
	}

	gesel = gesel | tempGeSel;

	for (int i = 0; i < edgesAtVertex.Count(); i++)
	{
		if (edgesAtVertex[i])
			delete edgesAtVertex[i];
		
	}

	theHold.Suspend();
	fnSyncTVSelection();
	theHold.Resume();

	InvalidateView();
	NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
}


void UnwrapMod::fnGeomRingSelect()
{

	theHold.Begin();
	HoldSelection();
	theHold.Accept(_T(GetString(IDS_EDGERINGSELECTION)));

	BitArray tempGeSel;
	tempGeSel = gesel;
	tempGeSel.ClearAll();


	//get the selected edge
	for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
	{
		if (gesel[i])
		{
	//get the face atatched to that edge
			for (int j = 0; j < TVMaps.gePtrList[i]->faceList.Count(); j++)
			{
	//get all the visible edges attached to that face
				int currentFace = TVMaps.gePtrList[i]->faceList[j];
				
				int currentEdge = i;

				BOOL done = FALSE;
				int startEdge = currentEdge;
				BitArray edgesForThisFace;
				edgesForThisFace.SetSize(TVMaps.gePtrList.Count());
				

				Tab<int> facesToProcess;
				BitArray processedFaces;
				processedFaces.SetSize(TVMaps.f.Count());
				processedFaces.ClearAll();


				while (!done)
				{
	
					
					edgesForThisFace.ClearAll();
					facesToProcess.Append(1,&currentFace,100);
					while (facesToProcess.Count() > 0)
					{
						//pop the stack
						currentFace = facesToProcess[0];
						facesToProcess.Delete(0,1);

						processedFaces.Set(currentFace,TRUE);
						int numberOfEdges = TVMaps.f[currentFace]->count;
						//loop through the edges
						for (int k = 0; k < numberOfEdges; k++)
						{
						//if edge is invisisble add the edges of the cross face
							int a = TVMaps.f[currentFace]->v[k];
							int b = TVMaps.f[currentFace]->v[(k+1)%numberOfEdges];
							if (a!=b)
							{
								int eindex = TVMaps.FindGeoEdge(a,b);
								if (!( TVMaps.gePtrList[eindex]->flags & FLAG_HIDDENEDGEA))
								{
									edgesForThisFace.Set(eindex,TRUE);
								}
								else
								{
									for (int m = 0; m < TVMaps.gePtrList[eindex]->faceList.Count(); m++)
									{
										int faceIndex = TVMaps.gePtrList[eindex]->faceList[m];
										if (!processedFaces[faceIndex])
										{
											facesToProcess.Append(1,&faceIndex,100);
										}
									}
								}
							}

						}
					}
/*
DebugPrint("Current face %d\n",currentFace);
DebugPrint("Current edge %d\n",currentEdge);
for (int m = 0; m < edgesForThisFace.GetSize(); m++)
{
	if (edgesForThisFace[m])
	{
		DebugPrint("   egde %d  vert a %d vert b %d\n",m, TVMaps.gePtrList[m]->a, TVMaps.gePtrList[m]->b);
	}
}
*/
					//if odd edge count we are done
					int numberSet = edgesForThisFace.NumberSet();
					if ( ((edgesForThisFace.NumberSet()%2) == 1) || (edgesForThisFace.NumberSet() <= 2))
						done = TRUE;
					else
					{
					//get the mid edge 
						//start at the seed
						int goal = edgesForThisFace.NumberSet()/2;
						int currentGoal = 0;
						int vertIndex = TVMaps.gePtrList[currentEdge]->a;
						Tab<int> edgeList;
						for (int m = 0; m < edgesForThisFace.GetSize(); m++)
						{
							if (edgesForThisFace[m])
								edgeList.Append(1,&m,100);

						}
						while (currentGoal != goal)
						{
							//find next edge
							for (int i = 0; i < edgeList.Count(); i++)
							{
								int potentialEdge = edgeList[i];
								if (potentialEdge != currentEdge)
								{
									int a = TVMaps.gePtrList[potentialEdge]->a;
									int b = TVMaps.gePtrList[potentialEdge]->b;
									if (a == vertIndex)
									{
										vertIndex = b;
										currentEdge = potentialEdge;
										i = edgeList.Count();

							//increment current
										currentGoal++;
									}
									else if (b == vertIndex)
									{
										vertIndex = a;
										currentEdge = potentialEdge;
										i = edgeList.Count();

									//increment current
										currentGoal++;
									}
								}
							}


						}

					}

					if (tempGeSel[currentEdge])
						done = TRUE;
					tempGeSel.Set(currentEdge,TRUE);

					for (int m = 0; m < TVMaps.gePtrList[currentEdge]->faceList.Count(); m++)
					{
						int faceIndex = TVMaps.gePtrList[currentEdge]->faceList[m];
						if ((faceIndex != currentFace) && (!processedFaces[faceIndex]))
						{
							currentFace = faceIndex;
							m = TVMaps.gePtrList[currentEdge]->faceList.Count();
						}
					}
					if (TVMaps.gePtrList[currentEdge]->faceList.Count() == 1)
						done = TRUE;
					//if we hit the start egde we are done
					if (currentEdge == startEdge) 
						done = TRUE;

					
				}

	
			}
		}
	}

	gesel = gesel | tempGeSel;

	theHold.Suspend();
	fnSyncTVSelection();
	theHold.Resume();

	InvalidateView();
	NotifyDependents(FOREVER,PART_SELECT,REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
}


void	UnwrapMod::SelectGeomElement(BOOL addSel, BitArray *originalSel)
	{

		if (ip)
		{
			if (ip->GetSubObjectLevel() == 1)
			{
				if (!addSel && (originalSel != NULL))
				{
					BitArray oSel = *originalSel;
					gvsel = (~gvsel) & oSel;
				}
				//loop through our edges finding ones with selected vertices
				//get the an empty vertex selection
				BitArray tempVSel;
				tempVSel.SetSize(TVMaps.geomPoints.Count());
				tempVSel.ClearAll();
				int selCount = -1;
				while (selCount != tempVSel.NumberSet())
				{
					selCount = tempVSel.NumberSet();
					for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
					{

							int a = TVMaps.gePtrList[i]->a;
							int b = TVMaps.gePtrList[i]->b;

							if (gvsel[a] || gvsel[b])
							{
								tempVSel.Set(a,TRUE);
								tempVSel.Set(b,TRUE);
							}
					}
					gvsel = gvsel | tempVSel;
				}

				if (!addSel && (originalSel != NULL))
				{
					BitArray oSel = *originalSel;
					gvsel =  oSel & (~gvsel);
				}
			}
			else if (ip->GetSubObjectLevel() == 2)
			{
				if (!addSel && (originalSel != NULL))
				{
					BitArray oSel = *originalSel;
					gesel = (~gesel) & oSel;
				}

				//get the an empty vertex selection
				int selCount = -1;
				while (selCount != gesel.NumberSet())
				{
					selCount = gesel.NumberSet();
					BitArray tempVSel;
					tempVSel.SetSize(TVMaps.geomPoints.Count());
					tempVSel.ClearAll();
					for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
					{
						if (gesel[i])
						{
							int a = TVMaps.gePtrList[i]->a;
							tempVSel.Set(a,TRUE);
							a = TVMaps.gePtrList[i]->b;
							tempVSel.Set(a,TRUE);
						}
					}

					BitArray tempESel;
					tempESel.SetSize(TVMaps.gePtrList.Count());
					tempESel.ClearAll();
					for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
					{
						int a = TVMaps.gePtrList[i]->a;
						int b = TVMaps.gePtrList[i]->b;
						if (tempVSel[a] || tempVSel[b])
							tempESel.Set(i);
					}
					gesel = tempESel;
				}

				if (!addSel && (originalSel != NULL))
				{
					BitArray oSel = *originalSel;
					gesel =  oSel & (~gesel);
				}
			}

		}

	}


BitArray* UnwrapMod::fnGetGeomVertexSelection()
{
	return &gvsel;
}
void UnwrapMod::fnSetGeomVertexSelection(BitArray *sel)
{

  	gvsel.ClearAll();
	for (int i = 0 ; i < (*sel).GetSize(); i++)
	{
		if ((i < gvsel.GetSize()) && ((*sel)[i]))
			gvsel.Set(i,TRUE);
	}
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
}

BitArray* UnwrapMod::fnGetGeomEdgeSelection()
{
	return &gesel;
}
void UnwrapMod::fnSetGeomEdgeSelection(BitArray *sel)
{

  	gesel.ClearAll();
	for (int i = 0 ; i < (*sel).GetSize(); i++)
	{
		if ((i < gesel.GetSize()) && ((*sel)[i]))
			gesel.Set(i,TRUE);
	}
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());
}


void UnwrapMod::fnPeltSeamsToSel(BOOL replace)
{
		
	theHold.Begin();
	HoldSelection();
	if (replace)
		theHold.Accept(GetString(IDS_PW_SEAMTOSEL));	
	else theHold.Accept(GetString(IDS_PW_SEAMTOSEL2));	
	if (replace)
		gesel = peltData.seamEdges;
	else
	{
		for (int i = 0; i < peltData.seamEdges.GetSize(); i++)
		{
			if (peltData.seamEdges[i])
				gesel.Set(i,TRUE);

		}
	}
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

	
}
void UnwrapMod::fnPeltSelToSeams(BOOL replace)
{

	theHold.Begin();
	theHold.Put (new UnwrapPeltSeamRestore (this));
	if (replace)
		theHold.Accept(GetString(IDS_PW_SELTOSEAM));
	else theHold.Accept(GetString(IDS_PW_SELTOSEAM2));

	if (replace)
		peltData.seamEdges = gesel;
	else
	{
		if (peltData.seamEdges.GetSize() != gesel.GetSize())
		{
			peltData.seamEdges.SetSize(gesel.GetSize());
			peltData.seamEdges.ClearAll();
		}

		for (int i = 0; i < gesel.GetSize(); i++)
		{
			if (gesel[i])
				peltData.seamEdges.Set(i,TRUE);

		}
	}
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}

class CellInfo
{
public:
	Tab<int> facesInThisCell;
};

class Line
{
	Tab<CellInfo*> cells;
public:
	int gridSize;
	float min, max;
	float len;
	
	void InitializeGrid(int cellSize, float min, float max);
	void AddLine(int index, float a, float b);
	void HitLine(float a, float b);
	void FreeLine();
	int ct;
	BitArray bHitFaces;
	Tab<int> hitFaces;

};

void Line::InitializeGrid(int size, float min, float max)
{
	this->min = min;
	this->max = max;
	gridSize = size;
	len = max - min;
	cells.SetCount(size);
	for (int i = 0; i < size; i++)
		cells[i] = NULL;
	ct = 0;

}
void Line::AddLine(int index, float a, float b)
{
	a -= min;
	b -= min;
	if (a > b)
	{
		float temp = a;
		a = b;
		b = temp;
	}
	//get the start cell
	int startIndex = (a/len) * gridSize;
	//get the end cell
	int endIndex = (b/len) * gridSize;

	if (startIndex >= gridSize) startIndex = gridSize-1;
	if (endIndex >= gridSize) endIndex = gridSize-1;

	//if null create it
	for (int i = startIndex; i <= endIndex; i++)
	{
		if (cells[i] == NULL)
			cells[i] = new CellInfo();
//add the primitive
		cells[i]->facesInThisCell.Append(1,&index,10);	
	}
	if (index >= ct)
		ct = index+1;
}
void Line::HitLine(float a, float b)
{
	a -= min;
	b -= min;

	if (bHitFaces.GetSize() != ct)
		bHitFaces.SetSize(ct);
	bHitFaces.ClearAll();
	hitFaces.SetCount(0);

	if (a > b)
	{
		float temp = a;
		a = b;
		b = temp;
	}
	//get the start cell
	int startIndex = (a/len) * gridSize;
	//get the end cell
	int endIndex = (b/len) * gridSize;

	if (startIndex >= gridSize) startIndex = gridSize-1;
	if (endIndex >= gridSize) endIndex = gridSize-1;
	for (int i = startIndex; i <= endIndex; i++)
	{
		if (cells[i] != NULL)
		{
			int numberOfFaces = cells[i]->facesInThisCell.Count();
			for (int j = 0; j < numberOfFaces; j++)
			{
				int index = cells[i]->facesInThisCell[j];
				if (!bHitFaces[index])
				{
					hitFaces.Append(1,&index, 500);
					bHitFaces.Set(index,TRUE);
				}
			}
		}
	}

}
void Line::FreeLine()
{
	
	for (int i = 0; i < cells.Count(); i++)
	{
		if (cells[i])
			delete cells[i];
		cells[i] = NULL;
	}
}

void UnwrapMod::fnSelectOverlap()
{
	theHold.Begin();
	HoldSelection();
	theHold.Accept(_T(GetString(IDS_SELECT_OVERLAP)));

	if (fnGetSyncSelectionMode()) 
		fnSyncGeomSelection();

	if (fsel.NumberSet() == 0)
		SelectOverlap(FALSE);
	else 
	{
		BitArray holdSel = fsel;
		SelectOverlap(FALSE);
		if (fsel.NumberSet() == 0)
		{
			fsel = holdSel;
			SelectOverlap(TRUE);
		}
	}
	
	InvalidateView();
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	if (ip) ip->RedrawViews(ip->GetTime());

}
void UnwrapMod::SelectOverlap(BOOL limitSel)
{
 

	BitArray useVerts;
	useVerts.SetSize(TVMaps.v.Count());
	useVerts.SetAll();

	if ((fsel.NumberSet() != 0) && (!limitSel))
	{
		useVerts.ClearAll();
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			if (fsel[i])
			{
				int deg = TVMaps.f[i]->count;
				for (int j = 0; j < deg; j++)
				{
					int index = TVMaps.f[i]->t[j];
					useVerts.Set(index,TRUE);

				}
			}
		}
	}
	

	BitArray tempEdgeSel;
	tempEdgeSel.SetSize(TVMaps.ePtrList.Count());
	tempEdgeSel.ClearAll();
	if (limitSel)
	{
		for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
		{
			int ct = TVMaps.ePtrList[i]->faceList.Count();
			for (int j = 0; j < ct; j++)
			{
				int faceIndex = TVMaps.ePtrList[i]->faceList[j];
				if (fsel[faceIndex])
					tempEdgeSel.Set(i,TRUE);

			}
		}
	}

	Tab<int> edgeStack;

	Line lineX;
	Line lineY;
	
	//get our edge list and add only those that have a visible face
	Box3 bounds;
	bounds.Init();
	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
			int ia,ib;
			ia = TVMaps.ePtrList[i]->a;
			ib = TVMaps.ePtrList[i ]->b;
		    if ( (IsVertVisible(ia) || IsVertVisible(ib)) && (useVerts[ib] && useVerts[ia]) ) 
			{
				if (!(TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA))
				{
					edgeStack.Append(1,&i,1000);
					Point3 p1 = TVMaps.v[ia].p;
					bounds += p1;
					Point3 p2 = TVMaps.v[ib].p;
					bounds += p2;
				}
			}		
	}


	lineX.InitializeGrid(500, bounds.pmin.x, bounds.pmax.x);
	lineY.InitializeGrid(500, bounds.pmin.y, bounds.pmax.y);

	Tab<Point3> transformPoints;

	transformPoints.SetCount(TVMaps.v.Count());
	float w = bounds.pmax.x - bounds.pmin.x;
	float h = bounds.pmax.y - bounds.pmin.y;
	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		transformPoints[i] = TVMaps.v[i].p;
		transformPoints[i] -= bounds.pmin;
		transformPoints[i].x = transformPoints[i].x/w;
		transformPoints[i].y = transformPoints[i].y/h;
	}

	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
			int ia,ib;
			ia = TVMaps.ePtrList[i]->a;
			ib = TVMaps.ePtrList[i ]->b;
		    if ((IsVertVisible(ia) || IsVertVisible(ib)) && (useVerts[ib] && useVerts[ia]) ) 
			{
				if (!(TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA))
				{
//					edgeStack.Append(1,&i,1000);
					Point3 p1 = TVMaps.v[ia].p;
					Point3 p2 = TVMaps.v[ib].p;

					lineX.AddLine(i,p1.x,p2.x);
					lineY.AddLine(i,p1.y,p2.y);
				}				
			}
		
	}


	Tab<int> finalEdges;
	Tab<int> map;
	map.SetCount(1000*1000);
	for (int i = 0; i < 1000*1000; i++)
		map[i] = 0;

 	BitArray hitEdges;
	hitEdges.SetSize(TVMaps.ePtrList.Count());
 	hitEdges.ClearAll();

   	BailStart();int hitct =0;
	for (int i = 0; i < edgeStack.Count(); i++)
	{

		int edgeIndex = edgeStack[i];		
		int ia,ib;
		ia = TVMaps.ePtrList[edgeIndex]->a;
		ib = TVMaps.ePtrList[edgeIndex]->b;

		Point3 p1 = TVMaps.v[ia].p;
		Point3 p2 = TVMaps.v[ib].p;

		Point3 tp1 = transformPoints[ia];
		Point3 tp2 = transformPoints[ib];

		long x1 = (long)(tp1.x*(1000.0f-1));
		long y1 = (long)(tp1.y*(1000.0f-1));

 		long x2 = (long)(tp2.x*(1000.0f-1));
		long y2 = (long)(tp2.y*(1000.0f-1));

		BOOL potentialHit = TRUE;
		potentialHit = BXPLine(x1,y1,x2,y2,
						1000,1000,	edgeIndex+1,		   
					    map, TRUE);

		if (potentialHit)
		{

			hitct++;
			lineX.HitLine(p1.x,p2.x);
			lineY.HitLine(p1.y,p2.y);
			BOOL first = TRUE;
			
			for (int j = 0; j < lineX.hitFaces.Count(); j++)
			{
				int edgeIndex2 = lineX.hitFaces[j];
				if (lineY.bHitFaces[edgeIndex2])
				{
					if (edgeIndex2 != edgeIndex)
					{
						int ia,ib;
						ia = TVMaps.ePtrList[edgeIndex]->a;
						ib = TVMaps.ePtrList[edgeIndex]->b;
						Point3 a = TVMaps.v[ia].p;
						a.z = 0.0f;
						Point3 b = TVMaps.v[ib].p;
						b.z = 0.0f;

						int ia2,ib2;
						ia2 = TVMaps.ePtrList[edgeIndex2]->a;
						ib2 = TVMaps.ePtrList[edgeIndex2]->b;

						Point3 a2 = TVMaps.v[ia2].p;
						Point3 b2 = TVMaps.v[ib2].p;
						
						a2.z = 0.0f;
						b2.z = 0.0f;

						if ((ia == ia2) || (ia == ib2) || 
							(ib == ia2) || (ib == ib2) )
						{
						}
						else
						{
				//remove those edges
							if (LineIntersect(a,b,a2,b2))
							{
								if (!limitSel)
								{
									hitEdges.Set(edgeIndex);
									hitEdges.Set(edgeIndex2);
								}
								else
								{
									//have to have one sel and one non sel edge
									BOOL sela = tempEdgeSel[edgeIndex];
									BOOL selb = tempEdgeSel[edgeIndex2];
									//mark the unsel edge
									if (sela && (!selb))
										hitEdges.Set(edgeIndex2);
									if (selb && (!sela))
										hitEdges.Set(edgeIndex);
								}

							}
						}	

					}
				}
			}
				
		}
		int per = (int)((float)i/ (float)edgeStack.Count()*100.0f);
		TSTR statusMessage;
		statusMessage.printf("%s %d ",GetString(IDS_PW_STATUS_OVERLAP),per  );
		if ( Bail(ip , statusMessage , 100))
			i = edgeStack.Count();

	}

	//select the faces from our edge list that 
	BitArray overlappedFaces;
	overlappedFaces.SetSize(TVMaps.f.Count());
	overlappedFaces.ClearAll();
	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		int edgeIndex = i;
		if (hitEdges[edgeIndex])
		{
			int ct = TVMaps.ePtrList[edgeIndex]->faceList.Count();
			for (int j = 0; j < ct; j++)
			{
				int faceIndex = TVMaps.ePtrList[edgeIndex]->faceList[j];
				overlappedFaces.Set(faceIndex,TRUE);
			}
		}
	
	}


	lineX.FreeLine();
	lineY.FreeLine();

	fsel = overlappedFaces;


}

BOOL UnwrapMod::BXPLine(long x1,long y1,long x2,long y2,
						int width, int height,	int id,				   
					   Tab<int> &map, BOOL clearEnds)



{
	long i,px,py,x,y;
	long dx,dy,dxabs,dyabs,sdx,sdy;
	long count;


	if (clearEnds)
	{
		map[x1+y1*(width)] = -1;
		map[x2+y2*(width)] = -1;
	}


	dx = x2-x1;
	dy = y2-y1;

	if (dx >0)
		sdx = 1;
	else
		sdx = -1;

	if (dy >0)
		sdy = 1;
	else
		sdy = -1;


	dxabs = abs(dx);
	dyabs = abs(dy);

	x=0;
	y=0;
	px=x1;
	py=y1;

	count = 0;
	BOOL iret = FALSE;


	if (dxabs >= dyabs)
		for (i=0; i<=dxabs; i++)
		{
			y +=dyabs;
			if (y>=dxabs)
			{
				y-=dxabs;
				py+=sdy;
			}


			if ( (px>=0) && (px<width) &&
				(py>=0) && (py<height)    )
			{
				int tid = map[px+py*width];
				if (tid != -1)
				{
					if ((tid != 0) && (tid != id))
						iret = TRUE;
					map[px+py*width] = id;
				}	
			}


			px+=sdx;
		}


	else

		for (i=0; i<=dyabs; i++)
		{
			x+=dxabs;
			if (x>=dyabs)
			{
				x-=dyabs;
				px+=sdx;
			}


			if ( (px>=0) && (px<width) &&
				(py>=0) && (py<height)    )
			{
				int tid = map[px+py*width];
				if (tid != -1)
				{

					if ((tid != 0) &&(tid != id))
						iret = TRUE;
					map[px+py*width] = id;
				}
			}
			


			py+=sdy;
		}
		(count)--;
		return iret;
}

//VSNAP
void UnwrapMod::BuildSnapBuffer()
{
	//put all our verts in screen space
	TimeValue t = GetCOREInterface()->GetTime();
	Tab<IPoint2> transformedPoints;
	transformedPoints.SetCount(TVMaps.v.Count());

	TransferSelectionStart();
	//get our window width height
	float xzoom, yzoom;
	int width,height;
	ComputeZooms(hView,xzoom,yzoom,width,height);


	//build the vertex bufffer list
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

	
//clear out the buffers
	int s = width*height;
	if (vertexSnapBuffer.Count() != s)
		vertexSnapBuffer.SetCount(s);
	if (edgeSnapBuffer.Count() != s)
		edgeSnapBuffer.SetCount(s);

	for (int i = 0; i < s; i++)
	{
		vertexSnapBuffer[i] = -1;
		edgeSnapBuffer[i] = -2;
	}
	
	BOOL vSnap,eSnap;
	pblock->GetValue(unwrap_vertexsnap,0,vSnap,FOREVER);
	pblock->GetValue(unwrap_edgesnap,0,eSnap,FOREVER);
	//loop through our verts 
	if (vSnap)
	{
		for (int i = 0; i < TVMaps.v.Count(); i++) 
		{
	//if in window add it
			if ( (transformedPoints[i].x >= 0) && (transformedPoints[i].x < width) &&
				(transformedPoints[i].y >= 0) && (transformedPoints[i].y < height) )
			{
				int x = transformedPoints[i].x;
				int y = transformedPoints[i].y;

				int index = y * width + x;
				if (index < vertexSnapBuffer.Count() && (!vsel[i]) && IsVertVisible(i))
					vertexSnapBuffer[index] = i;
			}
		}
	}
	

	
	//loop through the edges
	edgesConnectedToSnapvert.SetSize(TVMaps.ePtrList.Count());
	edgesConnectedToSnapvert.ClearAll();

	if (eSnap)
	{
		for (int i =0; i < TVMaps.ePtrList.Count(); i++)
		{
		//add them to the edge buffer
			if (!(TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA))
			{
				int a = TVMaps.ePtrList[i]->a;
				int b = TVMaps.ePtrList[i]->b;
				BOOL aHidden = (!IsVertVisible(a));
				BOOL bHidden = (!IsVertVisible(b));
				if ((a == mouseHitVert) || (b == mouseHitVert) || vsel[a] || vsel[b] || aHidden || bHidden)
				{

					
						edgesConnectedToSnapvert.Set(i);
				}
				IPoint2 pa,pb;
				pa = transformedPoints[a];
				pb = transformedPoints[b];

				long x1,y1,x2,y2;
				
				x1 = (long)pa.x;
				y1 = (long)pa.y;

				x2 = (long)pb.x;
				y2 = (long)pb.y;

				if (!edgesConnectedToSnapvert[i])
					BXPLine(x1,y1,x2,y2,
						width, height,i,				   
						edgeSnapBuffer, FALSE);
			}

		}
	}
	TransferSelectionEnd(FALSE,FALSE);

}


BOOL UnwrapMod::GetGridSnap()
{
	BOOL snap; 
	pblock->GetValue(unwrap_gridsnap,0,snap,FOREVER);
	return snap;
}

void UnwrapMod::SetGridSnap(BOOL snap)
{
	pblock->SetValue(unwrap_gridsnap,0,snap);
}

BOOL UnwrapMod::GetVertexSnap()
{
	BOOL snap; 
	pblock->GetValue(unwrap_vertexsnap,0,snap,FOREVER);
	return snap;

}
void UnwrapMod::SetVertexSnap(BOOL snap)
{
	pblock->SetValue(unwrap_vertexsnap,0,snap);
}
BOOL UnwrapMod::GetEdgeSnap()
{
	BOOL snap; 
	pblock->GetValue(unwrap_edgesnap,0,snap,FOREVER);
	return snap;

}
void UnwrapMod::SetEdgeSnap(BOOL snap)
{
	pblock->SetValue(unwrap_edgesnap,0,snap);
}
