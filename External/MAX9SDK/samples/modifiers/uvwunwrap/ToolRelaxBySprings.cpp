#include "unwrap.h"

#include "3dsmaxport.h"


/*

find the best guess edge lengths


*/

int UnwrapMod::fnGetRelaxBySpringIteration()
{
	return relaxBySpringIteration;
}
void UnwrapMod::fnSetRelaxBySpringIteration(int iteration)
{
	relaxBySpringIteration = iteration;
	if (relaxBySpringIteration < 1) relaxBySpringIteration = 1;
}

float UnwrapMod::fnGetRelaxBySpringStretch()
{
	return relaxBySpringStretch;
}
void UnwrapMod::fnSetRelaxBySpringStretch(float stretch)
{
	relaxBySpringStretch = stretch;
}

BOOL UnwrapMod::fnGetRelaxBySpringVEdges()
{
	return relaxBySpringUseOnlyVEdges;
}
void UnwrapMod::fnSetRelaxBySpringVEdges(BOOL useVEdges)
{
	relaxBySpringUseOnlyVEdges = useVEdges;
}


float UnwrapMod::AngleFromVectors(Point3 vec1, Point3 vec2)
{
	float iangle = 0.0f;
	float dot = DotProd(vec1,vec2);
	Point3 crossProd = CrossProd(vec2,vec1);
	if (dot >= 0.9999f)
		iangle = 0.0f;
	else if (dot == 0.0f)
		iangle = PI*0.5f;
	else if (dot <= -0.9999f)
		iangle = PI;
	else iangle = acos(dot);
	if (crossProd.z < 0.0)
		iangle *= -1.0f;
	return iangle;
}

void UnwrapMod::fnRelaxByFaceAngleNoDialog(int frames, float stretch, float str, BOOL lockOuterVerts)
{
	fnRelaxByFaceAngle(frames, stretch, str, lockOuterVerts);
}
void UnwrapMod::fnRelaxByEdgeAngleNoDialog(int frames, float stretch,  float str, BOOL lockOuterVerts)
{
	fnRelaxByEdgeAngle(frames, stretch, str, lockOuterVerts);
}


void UnwrapMod::fnRelaxByFaceAngle(int frames, float stretch, float str, BOOL lockEdges,HWND statusHWND)

{

	//	stretch = (1.0f-(stretch*0.01f));
	stretch = (1.0f-(stretch));
	TimeValue t = GetCOREInterface()->GetTime();

	//    int frames = 10;
	//	float stretch = 0.87f;
	float reducer = 1.0f;

	/*
	if (!theHold.Holding())
	{
	theHold.Begin();
	HoldPoints();
	theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));
	}
	*/
	TransferSelectionStart(); 

	BitArray rigPoint;
	rigPoint.SetSize(TVMaps.v.Count());
	rigPoint.ClearAll();
	if (peltData.peltDialog.hWnd)
	{
		for (int i = 0; i < peltData.rigPoints.Count(); i++)
		{
			rigPoint.Set(peltData.rigPoints[i].lookupIndex);
		}
	}

	BitArray edgeVerts;
	edgeVerts.SetSize(TVMaps.v.Count());
	edgeVerts.ClearAll();

	float uvEdgeDist = 0.0f;
	float geomEdgeDist = 0.0f;

	Tab<float> originalWs;
	originalWs.SetCount( TVMaps.v.Count());
	for (int i = 0; i < originalWs.Count(); i++)
	{
		originalWs[i] = TVMaps.v[i].p.z;
		TVMaps.v[i].p.z = 0.0f;
	}




	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;

		if (TVMaps.ePtrList[i]->faceList.Count() == 1)
		{
			edgeVerts.Set(a,TRUE);
			edgeVerts.Set(b,TRUE);
		}
	}

	int ct = 0;
	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		int ga = TVMaps.ePtrList[i]->ga;
		int gb = TVMaps.ePtrList[i]->gb;

		float wa = 0.0f;
		float wb = 0.0f;

		if (vsel[a]) wa = 1.0f;
		else wa = TVMaps.v[a].influence;
		if (vsel[b]) wb = 1.0f;
		else wb = TVMaps.v[b].influence;
		if ((wa > 0.0f) && (wb == 0.0f))
			edgeVerts.Set(a,TRUE);
		if ((wb > 0.0f) && (wa == 0.0f))
			edgeVerts.Set(b,TRUE);

		if ((wa > 0.0f) && (wb > 0.0f))
		{

			uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
			geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
			ct++;
		}

	}

	if (ct)
	{

		uvEdgeDist = uvEdgeDist/(ct);
		geomEdgeDist = geomEdgeDist/(ct);
		float edgeScale = (uvEdgeDist/geomEdgeDist) *reducer;


		//get the normals at each vertex

		Tab<Point3> geomFaceNormals;
		geomFaceNormals.SetCount(TVMaps.f.Count());
		for (int i =0; i < TVMaps.f.Count(); i++)
		{
			geomFaceNormals[i] = TVMaps.GeomFaceNormal(i);
		}




		Tab<Matrix3> tmList;
		tmList.SetCount(TVMaps.f.Count());
		for (int i = 0; i < tmList.Count(); i++)
		{
			MatrixFromNormal(geomFaceNormals[i],tmList[i]);
			tmList[i] = Inverse(tmList[i]);
		}

		Tab<MyPoint3Tab*> gFacePoints;


		gFacePoints.SetCount(TVMaps.f.Count());

		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			gFacePoints[i] = new MyPoint3Tab();
			int deg = TVMaps.f[i]->count;
			gFacePoints[i]->p.SetCount(deg);

			Point3 p(0.0f,0.0f,0.0f);
			Point3 up(0.0f,0.0f,0.0f);
			for (int j = 0; j < deg; j++)
			{
				int index = TVMaps.f[i]->v[j];
				int uindex = TVMaps.f[i]->t[j];



				gFacePoints[i]->p[j] = TVMaps.geomPoints[index] * tmList[i];
				gFacePoints[i]->p[j].z = 0.0f;
				p += gFacePoints[i]->p[j];
			}
			Point3 gCenter = p/deg;


			for (int j = 0; j < deg; j++)
			{
				gFacePoints[i]->p[j] -= gCenter;
				gFacePoints[i]->p[j] *= edgeScale;
				Point3 p = gFacePoints[i]->p[j];
			}
		}

		Tab<Point3> deltaList;
		Tab<int> deltaCount;

		deltaCount.SetCount(TVMaps.v.Count());
		deltaList.SetCount(TVMaps.v.Count());

		BitArray useFace;
		useFace.SetSize(TVMaps.f.Count());
		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			int deg = TVMaps.f[i]->count;

			BOOL sel = FALSE;
			for (int j = 0; j < deg; j++)
			{
				int index = TVMaps.f[i]->t[j];
				float w = 0.0f;
				if (vsel[index]) w = 1.0f;
				else w = TVMaps.v[index].influence;
				if (w > 0.f) sel = TRUE;

			}
			if (sel) useFace.Set(i,TRUE);
		}

		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			int deg = TVMaps.f[i]->count;

			for (int j = 0; j < deg; j++)
			{
				int index = TVMaps.f[i]->t[j];
				if (!edgeVerts[index])
					gFacePoints[i]->p[j] *= stretch;				
			}
		}


		for (int cframe = 0; cframe < frames; cframe++)
		{

			SHORT iret = GetAsyncKeyState (VK_ESCAPE);
			if (iret==-32767)
			{
				cframe = frames;
			}

			if (statusHWND != NULL)
			{
				TSTR progress;
				progress.printf("%s %d(%d)",GetString(IDS_PROCESS),cframe,frames);
				SetWindowText(statusHWND,progress);
			}

			for (int i = 0; i < TVMaps.v.Count(); i++)
			{
				deltaList[i] = Point3(0.0f,0.0f,0.0f);
				deltaCount[i] = 0;
			}

			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
				//line up the face
				if (useFace[i])
				{
					int deg =  TVMaps.f[i]->count;
					Tab<Point3> uvPoints;
					uvPoints.SetCount(deg);
					Point3 center(0.0f,0.0f,0.0f);
					for (int j = 0; j < deg; j++)
					{
						int uindex = TVMaps.f[i]->t[j];
						uvPoints[j] = TVMaps.v[uindex].p;
						center += uvPoints[j];
					}
					center = center/(float)deg;
					for (int j = 0; j < deg; j++)
					{
						uvPoints[j] -= center;
					}


					//add the deltas


					float fangle = 0.0f;
					for (int j = 0; j < deg; j++)
					{
						Point3 gvec = gFacePoints[i]->p[j];
						Point3 uvec = uvPoints[j];
						uvec = Normalize(uvec);
						gvec = Normalize(gvec);
						float ang = AngleFromVectors(gvec,uvec);
						fangle += ang;
					}
					float fangle2 = -fangle/(deg);

					Matrix3 tm(1);


					Point3 rvec(0.0f,0.0f,0.0f);
					Point3 ovec = Normalize(gFacePoints[i]->p[0]);
					for (int j = 0; j < deg; j++)
					{
						Point3 gvec = gFacePoints[i]->p[j];
						Point3 uvec = uvPoints[j];
						uvec = Normalize(uvec);
						gvec = Normalize(gvec);
						float ang = AngleFromVectors(gvec,uvec);
						Matrix3 rtm(1);
						rtm.RotateZ(ang);
						rvec += ovec * rtm;					 
					}
					rvec = Normalize(rvec);
					fangle = AngleFromVectors(ovec,rvec);
					//DebugPrint("%f %f \n",fangle2,fangle);

					tm.RotateZ(fangle);


					for (int j = 0; j < deg; j++)
					{
						Point3 gvec = gFacePoints[i]->p[j] * tm;
						Point3 delta = gvec - uvPoints[j];

						int uindex = TVMaps.f[i]->t[j];
						deltaList[uindex] += delta;
						deltaCount[uindex] += 1;				
					}
				}
			}
			for (int i = 0; i < TVMaps.v.Count(); i++)
			{
				float w = 0.0f;
				if (vsel[i]) w = 1.0f;
				else w = TVMaps.v[i].influence;
				if ((w > 0.0f) && (!(TVMaps.v[i].flags & FLAG_FROZEN)) && (!rigPoint[i]))
				{
					if (lockEdges)
					{

						if (!edgeVerts[i])
							TVMaps.v[i].p += (deltaList[i]/(float) deltaCount[i]) * w * str;			 
					}
					else TVMaps.v[i].p += (deltaList[i]/(float) deltaCount[i]) * w * str;			 
				}
			}
			/*
			for (int i = 0; i < TVMaps.f.Count(); i++)
			{
			int deg = TVMaps.f[i]->count;

			for (int j = 0; j < deg; j++)
			{
			int index = TVMaps.f[i]->t[j];
			if (!edgeVerts[index])
			gFacePoints[i]->p[j] *= stretch;				
			}
			}
			*/
			if ((cframe%10) == 0)
			{
				peltData.ResolvePatchHandles(this);
				NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
				InvalidateView();
				UpdateWindow(hWnd);
				if (ip) ip->RedrawViews(t);
			}
		}

		for (int i = 0; i < TVMaps.f.Count(); i++)
		{
			delete gFacePoints[i];
		}


		TSTR progress;
		progress.printf("    ");
		SetWindowText(statusHWND,progress);

		for (int i = 0; i < originalWs.Count(); i++)
		{
			TVMaps.v[i].p.z = originalWs[i];

		}

		if (peltData.peltDialog.hWnd)
			peltData.RelaxRig(frames,str,lockEdges,this);

		//	TimeValue t = GetCOREInterface()->GetTime();
		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
			float w = 0.0f;
			if (vsel[i]) w = 1.0f;
			else w = TVMaps.v[i].influence;
			if ( (w > 0.0f) && (!(TVMaps.v[i].flags & FLAG_FROZEN)))
			{			
				if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);

			}
		}

		peltData.ResolvePatchHandles(this);

	}
	TransferSelectionEnd(FALSE,FALSE); 


	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
	if (ip) ip->RedrawViews(ip->GetTime());
}

void UnwrapMod::fnRelaxByEdgeAngle(int frames, float stretch,float str, BOOL lockEdges,HWND statusHWND)

{
// 	stretch = (1.0f-(stretch*0.01f));
 	stretch = (1.0f-(stretch));
//    int frames = 10;
 //	float stretch = 0.87f;
 	float reducer = 1.0f;
/*
	if (!theHold.Holding())
	{
		theHold.Begin();
		HoldPoints();
		theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));
	}
*/
	TransferSelectionStart(); 

	BitArray rigPoint;
	rigPoint.SetSize(TVMaps.v.Count());
	rigPoint.ClearAll();
	if (peltData.peltDialog.hWnd)
	{
		for (int i = 0; i < peltData.rigPoints.Count(); i++)
		{
			rigPoint.Set(peltData.rigPoints[i].lookupIndex);
		}
	}

	BitArray edgeVerts;
	edgeVerts.SetSize(TVMaps.v.Count());
	edgeVerts.ClearAll();

	BitArray useVerts;
	useVerts.SetSize(TVMaps.v.Count());
	useVerts.ClearAll();



	float uvEdgeDist = 0.0f;
	float geomEdgeDist = 0.0f;

	TimeValue t = GetCOREInterface()->GetTime();

	Tab<float> originalWs;
	originalWs.SetCount( TVMaps.v.Count());
	for (int i = 0; i < originalWs.Count(); i++)
	{
		originalWs[i] = TVMaps.v[i].p.z;
		TVMaps.v[i].p.z = 0.0f;
	}


	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		int a = i;
		float wa = 0.0f;
		if (vsel[a]) wa = 1.0f;
		else wa = TVMaps.v[a].influence;
		if (wa > 0.0f) useVerts.Set(a,TRUE);
	}

	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;

		if (TVMaps.ePtrList[i]->faceList.Count() == 1)
		{
			edgeVerts.Set(a,TRUE);
			edgeVerts.Set(b,TRUE);
		}

		float wa = 0.0f;
		if (vsel[a]) wa = 1.0f;
		else wa = TVMaps.v[a].influence;

		float wb = 0.0f;
		if (vsel[b]) wb = 1.0f;
		else wb = TVMaps.v[b].influence;

		if (useVerts[a] && (!useVerts[b]))
			edgeVerts.Set(a,TRUE);
		if (useVerts[b] && (!useVerts[a]))
			edgeVerts.Set(b,TRUE);
	}

	int ct = 0;
	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		int ga = TVMaps.ePtrList[i]->ga;
		int gb = TVMaps.ePtrList[i]->gb;

		if (useVerts[a] || useVerts[b])
		{
			uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
			geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
			ct++;
		}

	}

   	uvEdgeDist = uvEdgeDist/(ct);
	geomEdgeDist = geomEdgeDist/(ct);
	float edgeScale = (uvEdgeDist/geomEdgeDist) *reducer;



	//get the normals at each vertex
	
	Tab<Point3> geomFaceNormals;
	geomFaceNormals.SetCount(TVMaps.f.Count());
	for (int i =0; i < TVMaps.f.Count(); i++)
	{
		geomFaceNormals[i] = TVMaps.GeomFaceNormal(i);
	}

	Tab<AdjacentItem*> facesAtVert;
 	TVMaps.BuildAdjacentUVFacesToVerts(facesAtVert);

	Tab<ClusterInfo*> clusterInfo;


	clusterInfo.SetCount(TVMaps.v.Count());

	//loop through our verts
  	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		clusterInfo[i] = NULL;
	//get the faces attached to this vert
		if (useVerts[i] && facesAtVert[i]->index.Count() )
		{						
			int ct =facesAtVert[i]->index.Count();
			clusterInfo[i] = new ClusterInfo();

			Tab<EdgeVertInfo> edgeVerts;
			edgeVerts.SetCount(ct);
			BitArray usedEdge;
			usedEdge.SetSize(ct);
			usedEdge.ClearAll();

			
 			for (int j = 0; j < ct; j++)
			{
				edgeVerts[j].faceIndex = facesAtVert[i]->index[j];
				int faceIndex = edgeVerts[j].faceIndex;
				int uvVertID = i;
				int geoVertID = TVMaps.f[faceIndex]->GetGeoVertIDFromUVVertID(uvVertID);

				int geoP,geoN;
				int uvN,uvP;
				TVMaps.f[faceIndex]->GetConnectedGeoVerts(geoVertID,geoP,geoN);
				edgeVerts[j].geoIndexA = geoVertID;				
				edgeVerts[j].geoIndexP = geoP;
				edgeVerts[j].geoIndexN = geoN;

				TVMaps.f[faceIndex]->GetConnectedUVVerts(uvVertID,uvP,uvN);
				edgeVerts[j].uvIndexA = uvVertID;
				edgeVerts[j].uvIndexP = uvP;
				edgeVerts[j].uvIndexN = uvN;				
			}
			//now sort them
			usedEdge.Set(0);

			clusterInfo[i]->edgeVerts.Append(1,&edgeVerts[0]);
			int n = edgeVerts[0].uvIndexN;
			int p = edgeVerts[0].uvIndexP;
			
			BOOL done = FALSE;
			while (!done)
			{
				BOOL hit = FALSE;
				for (int j = 0; j < ct; j++)
				{
					if (!usedEdge[j])
					{
						int testN,testP;
						testN = edgeVerts[j].uvIndexN;
						testP = edgeVerts[j].uvIndexP;
						if (testN == p)
						{
							usedEdge.Set(j,TRUE);
							clusterInfo[i]->edgeVerts.Append(1,&edgeVerts[j]);
							p = testP;
							hit = TRUE;
						}
						else if (testP == n)
						{
							usedEdge.Set(j,TRUE);
							clusterInfo[i]->edgeVerts.Insert(0,1,&edgeVerts[j]);
							n = testN;
							hit = TRUE;
						}
					}
				}
				if (!hit) done = TRUE;
			}			
		}
	}

	//now create our ideal cluster info
  	for (int i = 0; i < TVMaps.v.Count(); i++)
	{	
		if (clusterInfo[i])
		{
			BOOL open = TRUE;
			int startID, endID;
			int ct = clusterInfo[i]->edgeVerts.Count();
			startID = clusterInfo[i]->edgeVerts[0].uvIndexN;
			endID = clusterInfo[i]->edgeVerts[ct-1].uvIndexP;
			if (startID == endID)
				open = FALSE;
			float totalAngle = 0.0f;
			
			for (int j = 0; j < ct; j++)
			{
				int nID = clusterInfo[i]->edgeVerts[j].geoIndexN;
				int pID = clusterInfo[i]->edgeVerts[j].geoIndexP;
				
				int centerID = clusterInfo[i]->edgeVerts[j].geoIndexA;
				Point3 center = TVMaps.geomPoints[centerID];

				Point3 vec1 = TVMaps.geomPoints[nID] - center;
				Point3 vec2 = TVMaps.geomPoints[pID] - center;
				vec1 = Normalize(vec1);
				vec2 = Normalize(vec2);
				float angle = fabs(AngleFromVectors(vec2,vec1));
				clusterInfo[i]->edgeVerts[j].angle = angle;
				totalAngle += angle;
			}
			for (int j = 0; j < ct; j++)
			{
				if (open && (totalAngle < (PI*2.0f)))
				{
				}
				else
				{
					float angle = clusterInfo[i]->edgeVerts[j].angle;
					angle = (angle/totalAngle)*(PI*2.0f);
					clusterInfo[i]->edgeVerts[j].angle = angle;
				}				
			}
			float currentAngle = 0.0f;
			for (int j = 0; j < ct; j++)
			{
				int centerID = clusterInfo[i]->edgeVerts[j].geoIndexA;
				Point3 center = TVMaps.geomPoints[centerID];
				Matrix3 tm(1);
				tm.RotateZ(currentAngle);
				int nID = clusterInfo[i]->edgeVerts[j].geoIndexN;
				Point3 vec1 = TVMaps.geomPoints[nID] - center;
				float l = Length(vec1)*edgeScale;
				Point3 yaxis(0.0f,l,0.0f);
				Point3 v = yaxis * tm;
				clusterInfo[i]->edgeVerts[j].idealPos = v;
				clusterInfo[i]->edgeVerts[j].idealPosNormalized = Normalize(v);
				currentAngle += clusterInfo[i]->edgeVerts[j].angle;
			}
			if (open)
			{
				EdgeVertInfo eInfo;
				int centerID = clusterInfo[i]->edgeVerts[ct-1].geoIndexA;
				Point3 center = TVMaps.geomPoints[centerID];

				Matrix3 tm(1);
				tm.RotateZ(currentAngle);
				int nID = clusterInfo[i]->edgeVerts[ct-1].geoIndexP;
				Point3 vec1 = TVMaps.geomPoints[nID] - center;
				float l = Length(vec1)*edgeScale;
				Point3 yaxis(0.0f,l,0.0f);
				Point3 v = yaxis * tm;
				eInfo.idealPos = v;
				eInfo.idealPosNormalized = Normalize(v);
				eInfo.geoIndexN = nID;
				eInfo.uvIndexN = clusterInfo[i]->edgeVerts[ct-1].uvIndexP;
				clusterInfo[i]->edgeVerts.Append(1,&eInfo);
			}
		}
	}

 	for (int i = 0; i < facesAtVert.Count(); i++)
	{
		delete facesAtVert[i];
	}

	Tab<Point3> deltaList;
	Tab<int> deltaCount;

	deltaCount.SetCount(TVMaps.v.Count());
	deltaList.SetCount(TVMaps.v.Count());

	for (int i = 0; i < clusterInfo.Count(); i++)
	{

		if (clusterInfo[i])
		{
			int deg = clusterInfo[i]->edgeVerts.Count();

			for (int j = 0; j < deg; j++)
			{
				int index = clusterInfo[i]->edgeVerts[j].uvIndexN;
				if (!edgeVerts[index])
					clusterInfo[i]->edgeVerts[j].idealPos *= stretch;				
			}
		}
	}

 	for (int cframe = 0; cframe < frames; cframe++)
	{

		if (statusHWND != NULL)
		{
			TSTR progress;
			progress.printf("%s %d(%d)",GetString(IDS_PROCESS),cframe,frames);
			SetWindowText(statusHWND,progress);
		}

		SHORT iret = GetAsyncKeyState (VK_ESCAPE);
		if (iret==-32767)
		{
			cframe = frames;
		}

		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
			deltaList[i] = Point3(0.0f,0.0f,0.0f);
			deltaCount[i] = 0;
		}

		for (int i = 0; i < clusterInfo.Count(); i++)
		{
			//line up the face
			if (clusterInfo[i])
			{
				int deg = clusterInfo[i]->edgeVerts.Count();
				Tab<Point3> uvPoints;
				uvPoints.SetCount(deg);
				Point3 center(0.0f,0.0f,0.0f);
				for (int j = 0; j < deg; j++)
				{
					int uindex = clusterInfo[i]->edgeVerts[j].uvIndexN;
					uvPoints[j] = TVMaps.v[uindex].p;
					center += uvPoints[j];
				}
//				center = center/(float)deg;
				center = TVMaps.v[i].p;
				for (int j = 0; j < deg; j++)
				{
					uvPoints[j] -= center;
				}


				//add the deltas

				
  				float fangle = 0.0f;

				float angle = 0.0f;
				float iangle = 0.0f;
				int ct = clusterInfo[i]->edgeVerts.Count();
				
				for (int j = 0; j < ct; j++)
				{
					if (j == 0)
					{
						
						Point3 geomVec1 = clusterInfo[i]->edgeVerts[j].idealPosNormalized;
						geomVec1 = Normalize(geomVec1);

						int uvIndex = clusterInfo[i]->edgeVerts[j].uvIndexN;
						Point3 uvVec1 = TVMaps.v[uvIndex].p - center;

						uvVec1 = Normalize(uvVec1);

						float dot = DotProd(uvVec1,geomVec1);
						Point3 crossProd = CrossProd(uvVec1,geomVec1);
						if (dot >= 0.9999f)
							iangle = 0.0f;
						else if (dot == 0.0f)
							iangle = PI*0.5f;
						else if (dot <= -0.9999f)
							iangle = PI;
						else iangle = acos(dot);
						clusterInfo[i]->angle = iangle;
						if (crossProd.z > 0.0)
							iangle *= -1.0f;
						
					}
					else
					{
						Point3 geomVec1 = clusterInfo[i]->edgeVerts[j].idealPosNormalized;
						geomVec1 = Normalize(geomVec1);
						Matrix3 tm(1);
						tm.RotateZ(iangle);
						geomVec1 = geomVec1 * tm;

						int uvIndex = clusterInfo[i]->edgeVerts[j].uvIndexN;
						Point3 uvVec1 = TVMaps.v[uvIndex].p - center;

						uvVec1 = Normalize(uvVec1);

						float uangle = 0.0f;
						float dot = DotProd(uvVec1,geomVec1);
						if (dot >= 0.9999f)
							uangle += 0.0f;
						else if (dot == 0.0f)
							uangle += PI*0.5f;
						else if (dot <= -0.999f)
							uangle += PI;
						else uangle += acos(dot);

						Point3 crossProd = CrossProd(uvVec1,geomVec1);
						if (crossProd.z > 0.0)
							uangle *= -1.0f;
						angle += uangle;

					}


				}
    				angle = (angle)/(float)(ct-1);
				clusterInfo[i]->counterAngle = angle;
  				fangle = angle;


 				Point3 rvec(0.0f,0.0f,0.0f);
 				Point3 ovec = Normalize(clusterInfo[i]->edgeVerts[0].idealPos);
				BOOL centerIsEdge = FALSE;
				if (edgeVerts[i]) centerIsEdge = TRUE;
				for (int j = 1; j < deg; j++)
				{
					Point3 gvec = clusterInfo[i]->edgeVerts[j].idealPos;
					Point3 uvec = uvPoints[j];
					uvec = Normalize(uvec);
					gvec = Normalize(gvec);
					float ang = AngleFromVectors(gvec,uvec);
					Matrix3 rtm(1);
					rtm.RotateZ(ang);
					if (centerIsEdge)
					{
						int uindex = clusterInfo[i]->edgeVerts[j].uvIndexN;
						if (edgeVerts[uindex])
							rvec += ovec * rtm;					 
					}
					else rvec += ovec * rtm;					 
				}
 				rvec = Normalize(rvec);
				fangle = AngleFromVectors(ovec,rvec);
//DebugPrint("%f %f \n",fangle2,fangle);

				Matrix3 tm(1);
				tm.RotateZ(fangle);


				for (int j = 0; j < deg; j++)
				{
					Point3 gvec =  clusterInfo[i]->edgeVerts[j].idealPos * tm;
					clusterInfo[i]->edgeVerts[j].tempPos = gvec;
					Point3 delta = gvec - uvPoints[j];
					delta *= 0.5f;

					int uindex = clusterInfo[i]->edgeVerts[j].uvIndexN;
					deltaList[uindex] += delta;
					deltaCount[uindex] += 1;				

					deltaList[i] -= delta;
					deltaCount[i] += 1;	
				}
			}
		}
 		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
			float w = 0.0f;
			if (vsel[i]) w = 1.0f;
			else w = TVMaps.v[i].influence;
			if ((w > 0.0f) && (!(TVMaps.v[i].flags & FLAG_FROZEN)) && (!rigPoint[i]))
			{
				if (lockEdges)
				{
					if (!edgeVerts[i])
						TVMaps.v[i].p += (deltaList[i]/(float) deltaCount[i]) * w * str;			 
				}
				else TVMaps.v[i].p += (deltaList[i]/(float) deltaCount[i]) * w * str;			 
			}
		}
/*
		for (int i = 0; i < clusterInfo.Count(); i++)
		{

			if (clusterInfo[i])
			{
				int deg = clusterInfo[i]->edgeVerts.Count();
			
				for (int j = 0; j < deg; j++)
				{
					int index = clusterInfo[i]->edgeVerts[j].uvIndexN;
					if (!edgeVerts[index])
						clusterInfo[i]->edgeVerts[j].idealPos *= stretch;				
				}
			}
		}
*/
		if ((cframe%10) == 0)
		{
			peltData.ResolvePatchHandles(this);
			NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
			InvalidateView();
			UpdateWindow(hWnd);
			if (ip) ip->RedrawViews(t);
		}

	}


	peltData.ResolvePatchHandles(this);

	for (int i = 0; i < originalWs.Count(); i++)
	{
		TVMaps.v[i].p.z = originalWs[i];
		
	}

	if (peltData.peltDialog.hWnd)
		peltData.RelaxRig(frames,str,lockEdges,this);


//	TimeValue t = GetCOREInterface()->GetTime();
	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		float w = 0.0f;
		if (vsel[i]) w = 1.0f;
		else w = TVMaps.v[i].influence;
		if ((w > 0.0f) && (!(TVMaps.v[i].flags & FLAG_FROZEN)))
		{			
			if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
				
		}
	}


	//delete the clusters
	for (int i = 0; i < clusterInfo.Count(); i++)
	{
//if cluster
		if (clusterInfo[i])
			delete clusterInfo[i];
	}


	TransferSelectionEnd(FALSE,FALSE); 

	TSTR progress;
	progress.printf("    ");
	SetWindowText(statusHWND,progress);

	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
	if (ip) ip->RedrawViews(ip->GetTime());
}

/*
void UnwrapMod::fnRelaxBySprings(int frames, float stretch, BOOL lockEdges)

{
//    int frames = 10;
 //	float stretch = 0.87f;
	float reducer = 1.0f;

	if (!theHold.Holding())
	{
		theHold.Begin();
		HoldPoints();
		theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));
	}
	TransferSelectionStart(); 

	//find our boundary edges
	BitArray edgeSprings;
	BitArray springs;
	springs.SetSize(TVMaps.ePtrList.Count());
	springs.ClearAll();
	edgeSprings.SetSize(TVMaps.ePtrList.Count());
	edgeSprings.ClearAll();

	BitArray edgeVerts;
	edgeVerts.SetSize(TVMaps.v.Count());
	edgeVerts.ClearAll();



	BitArray springVerts;
	springVerts.SetSize(TVMaps.v.Count());
	springVerts.ClearAll();

	float uvEdgeDist = 0.0f;
	float geomEdgeDist = 0.0f;

	TimeValue t = GetCOREInterface()->GetTime();

	for (int i = 0; i < TVMaps.v.Count(); i++)
	{

		float wa = 0.0f;
		if (vsel[i]) wa = 1.0f;
		else wa = TVMaps.v[i].influence;
		if (wa > 0.0f)
		{
			Point3 p = TVMaps.v[i].p;		
			p.z = 0.0f;
			TVMaps.v[i].p = p;
			if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
			springVerts.Set(i,TRUE);
		}
		
	}
	

	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		int deg = TVMaps.f[i]->count;
		int ct = 0;
		for (int j = 0; j < deg; j++)
		{
			int a = TVMaps.f[i]->t[j];
			float wa = 0.0f;
			if (vsel[a]) wa = 1.0f;
			else wa = TVMaps.v[a].influence;
			if (wa > 0.0f) ct++;
		}
		if (ct != deg)
		{
			for (int j = 0; j < deg; j++)
			{
				int a = TVMaps.f[i]->t[j];

				float wa = 0.0f;
				if (vsel[a]) wa = 1.0f;
				else wa = TVMaps.v[a].influence;
				if (wa > 0.0f)
					edgeVerts.Set(a,TRUE);
			}

		}
	}

	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;

		if (TVMaps.ePtrList[i]->faceList.Count() == 1)
		{
			edgeVerts.Set(a,TRUE);
			edgeVerts.Set(b,TRUE);
		}
	}

	int ct = 0;
	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		int ga = TVMaps.ePtrList[i]->ga;
		int gb = TVMaps.ePtrList[i]->gb;

		float wa = 0.0f;
		float wb = 0.0f;

		if (vsel[a]) wa = 1.0f;
		else wa = TVMaps.v[a].influence;
		if (vsel[b]) wb = 1.0f;
		else wb = TVMaps.v[b].influence;
		if ((wa > 0.0f) && (wb > 0.0f))
		{
			if (lockEdges)
			{
				if (edgeVerts[a] && edgeVerts[b])
				{

				}
				else if ((!edgeVerts[a]) && (!edgeVerts[b]))
				{
					springs.Set(i,TRUE);
					uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
					geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
					ct++;
				}
				else if ( ((edgeVerts[a]) && (!edgeVerts[b])) ||
					      ((edgeVerts[b]) && (!edgeVerts[a])) )
				{
					springs.Set(i,TRUE);
					edgeSprings.Set(i,TRUE);
					uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
					geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
					ct++;
				}

			}
			else
			{
  				if (edgeVerts[a] && edgeVerts[b])
				{
					springs.Set(i,TRUE);
					uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
					geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
					ct++;
				}
				else if ((!edgeVerts[a]) && (!edgeVerts[b]))
				{
					springs.Set(i,TRUE);
					uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
					geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
					ct++;
				}
				else if ( ((edgeVerts[a]) && (!edgeVerts[b])) ||
					      ((edgeVerts[b]) && (!edgeVerts[a])) )
				{
					springs.Set(i,TRUE);
					uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
					geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
					ct++;
				}
			}
		}

	}

   	uvEdgeDist = uvEdgeDist/(ct);
	geomEdgeDist = geomEdgeDist/(ct);
	float edgeScale = (uvEdgeDist/geomEdgeDist) *reducer;






	//get the normals at each vertex
	
	Tab<Point3> geomFaceNormals;
	geomFaceNormals.SetCount(TVMaps.f.Count());
	for (int i =0; i < TVMaps.f.Count(); i++)
	{
		geomFaceNormals[i] = TVMaps.GeomFaceNormal(i);
	}

	Tab<Point3> geomVertexNormals;
	Tab<int> geomVertexNormalsCT;
	geomVertexNormals.SetCount(TVMaps.geomPoints.Count());
	geomVertexNormalsCT.SetCount(TVMaps.geomPoints.Count());

	for (int i =0; i < TVMaps.geomPoints.Count(); i++)
	{
		geomVertexNormals[i] = Point3(0.0f,0.0f,0.0f);
		geomVertexNormalsCT[i] = 0;
	}
	for (int i =0; i < TVMaps.f.Count(); i++)
	{
		int deg = TVMaps.f[i]->count;
		for (int j = 0; j < deg; j++)
		{
			int index = TVMaps.f[i]->v[j];
			geomVertexNormalsCT[index] = geomVertexNormalsCT[index] + 1;
			geomVertexNormals[index] += geomFaceNormals[i];
		}
	}

	for (int i =0; i < TVMaps.geomPoints.Count(); i++)
	{
		if (geomVertexNormalsCT[i] != 0)
			geomVertexNormals[i] = geomVertexNormals[i]/(float)geomVertexNormalsCT[i];
		geomVertexNormals[i] = Normalize(geomVertexNormals[i]);

	}

	Tab<EdgeBondage> springEdges;
	Tab<AdjacentItem*> facesAtVert;
 	TVMaps.BuildAdjacentUVFacesToVerts(facesAtVert);


	//delete the clusters
	for (int i = 0; i < clusterInfo.Count(); i++)
	{
//if cluster
		if (clusterInfo[i])
			delete clusterInfo[i];
	}

	clusterInfo.SetCount(TVMaps.v.Count());

	//loop through our verts
  	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		clusterInfo[i] = NULL;
	//get the faces attached to this vert
		if (springVerts[i])
		{
			clusterInfo[i] = new ClusterInfo();
			clusterInfo[i]->uvCenterIndex = i;
			Tab<int> connectedFaces;
			connectedFaces.SetCount(facesAtVert[i]->index.Count());
			for (int j = 0; j < facesAtVert[i]->index.Count(); j++)
			{
				connectedFaces[j] = facesAtVert[i]->index[j];
			}
		//get the uv/geom verts attached to this vert
			Tab<int> connectedUVVerts;
			Tab<int> connectedGeomVerts;
			int centerGIndex = -1;
			for (int j = 0; j < connectedFaces.Count(); j++)
			{
				
				int faceIndex = connectedFaces[j];
				int deg = TVMaps.f[faceIndex]->count;
				
				for (int k = 0; k < deg; k++)
				{
					BOOL visibleEdge = TRUE;
					if (deg == 3)
					{
						if (k == 0)
							visibleEdge = (!(TVMaps.f[faceIndex]->flags & FLAG_HIDDENEDGEA));
						if (k == 1)
							visibleEdge = (!(TVMaps.f[faceIndex]->flags & FLAG_HIDDENEDGEB));
						if (k == 2)
							visibleEdge = (!(TVMaps.f[faceIndex]->flags & FLAG_HIDDENEDGEC));
					}
					if (visibleEdge)
					{
						int a = TVMaps.f[faceIndex]->t[k];
						int b = TVMaps.f[faceIndex]->t[(k+1)%deg];

						int ga = TVMaps.f[faceIndex]->v[k];
						int gb = TVMaps.f[faceIndex]->v[(k+1)%deg];

						if (a == i)
						{
							connectedUVVerts.Append(1,&b,5);
							connectedGeomVerts.Append(1,&gb,5);
							centerGIndex = ga;
						}
						if (b == i)
						{
							connectedUVVerts.Append(1,&a,5);
							connectedGeomVerts.Append(1,&ga,5);
							centerGIndex = gb;
						}
					}

				}
			}

		//remove the duplicates
	 		BitArray processedVerts;
			processedVerts.SetSize(TVMaps.v.Count());
			processedVerts.ClearAll();

			Tab<int> cUVVerts;
			Tab<int> cGeomVerts;
			for (int j = 0; j < connectedUVVerts.Count(); j++)
			{
				int index = connectedUVVerts[j];
				int gindex = connectedGeomVerts[j];
				if (!processedVerts[index])
				{
					cUVVerts.Append(1,&index,5);
					cGeomVerts.Append(1,&gindex,5);
					processedVerts.Set(index,TRUE);
				}
			}

			Point3 center = TVMaps.geomPoints[centerGIndex];
			Tab<Point3> edgePoints;
			edgePoints.SetCount(cGeomVerts.Count());

			for (int j = 0; j < cGeomVerts.Count(); j++)
			{
				int uindex = cUVVerts[j];

				int index = cGeomVerts[j];
				edgePoints[j] = TVMaps.geomPoints[index];
			}
		//flatten the verts 
			Point3 norm = geomVertexNormals[centerGIndex];

  	 		for (int j = 0; j < cGeomVerts.Count(); j++)
			{
				Point3 vec = edgePoints[j] - center;
				float l = Length(vec) *edgeScale;
				vec = Normalize(vec);


				BOOL aEdge,bEdge;
 				aEdge = edgeVerts[i];
				bEdge = edgeVerts[cUVVerts[j]];
				if (aEdge && bEdge)
				{
				}
				else l *= 0.85f;

				Matrix3 tm2(1);
				MatrixFromNormal(norm,tm2);
				tm2 = Inverse(tm2);
				vec = vec *tm2;
				vec.z = 0.f;
				vec = Normalize(vec) *l;
				edgePoints[j] = vec;

			}

			clusterInfo[i]->edgeVerts.SetCount(cGeomVerts.Count());

//DebugPrint("vert %d\n",i);			
			for (int j = 0; j < cGeomVerts.Count(); j++)
			{
				clusterInfo[i]->edgeVerts[j].uvIndex = cUVVerts[j];
				clusterInfo[i]->edgeVerts[j].geoIndex = cGeomVerts[j];
				clusterInfo[i]->edgeVerts[j].idealPos = edgePoints[j];//*tm;
				clusterInfo[i]->edgeVerts[j].idealPosNormalized = Normalize(clusterInfo[i]->edgeVerts[j].idealPos);
//DebugPrint("	%f %f %f\n",clusterInfo[i]->edgeVerts[j].idealPos.x,clusterInfo[i]->edgeVerts[j].idealPos.y,clusterInfo[i]->edgeVerts[j].idealPos.z);		
			}
		}
	}

 	for (int i = 0; i < facesAtVert.Count(); i++)
	{
		delete facesAtVert[i];
	}

		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
//if cluster
			int a = i;
			if (clusterInfo[i])
			{
	
	//get a tm to go from geo space to uv space
				//get our angle bw

				int ct = clusterInfo[i]->edgeVerts.Count();
				for (int j = 0; j < ct; j++)
				{
//get our average rotation angle
					int b = clusterInfo[i]->edgeVerts[j].uvIndex;
					if (edgeVerts[a] && edgeVerts[b])
					{
					}
//					else
//						clusterInfo[i]->edgeVerts[j].idealPos *= stretch;

				}
			}
		}

	//loop through the verts
 	Tab<Point3> deltaList;
	deltaList.SetCount(TVMaps.v.Count());

 	Tab<int> deltaCount;
	
  	deltaCount.SetCount(TVMaps.v.Count());
//	frames = 1;
	dverts = edgeVerts;

	Tab<Point3> op;
	op.SetCount(TVMaps.v.Count());

	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
//if cluster
		if (clusterInfo[i])
		{
			clusterInfo[i]->delta = Point3(0.0f,0.0f,0.0f);
		}
		op[i] = TVMaps.v[i].p;
	}

	for (int iframes = 0; iframes < frames; iframes++)
	{
		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
			deltaList[i] = Point3(0.0f,0.0f,0.0f);
			deltaCount[i] = 0;
		}
  		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
//if cluster
			if (i == 16)
				DebugPrint("Wait\n");

			if (clusterInfo[i])
			{
	
	//get a tm to go from geo space to uv space
				//get our angle bw

				float angle = 0.0f;
				float iangle = 0.0f;
				int ct = clusterInfo[i]->edgeVerts.Count();
				Point3 center = op[i];
				for (int j = 0; j < ct; j++)
				{
					if (j == 0)
					{
						
						Point3 geomVec1 = clusterInfo[i]->edgeVerts[j].idealPosNormalized;
						geomVec1 = Normalize(geomVec1);

						int uvIndex = clusterInfo[i]->edgeVerts[j].uvIndex;
						Point3 uvVec1 =op[uvIndex] - center;

						uvVec1 = Normalize(uvVec1);

						float dot = DotProd(uvVec1,geomVec1);
						Point3 crossProd = CrossProd(uvVec1,geomVec1);
						if (dot >= 0.9999f)
							iangle = 0.0f;
						else if (dot == 0.0f)
							iangle = PI*0.5f;
						else if (dot <= -0.9999f)
							iangle = PI;
						else iangle = acos(dot);
						clusterInfo[i]->angle = iangle;
						if (crossProd.z > 0.0)
							iangle *= -1.0f;
						
					}
					else
					{
						Point3 geomVec1 = clusterInfo[i]->edgeVerts[j].idealPosNormalized;
						geomVec1 = Normalize(geomVec1);
						Matrix3 tm(1);
						tm.RotateZ(iangle);
						geomVec1 = geomVec1 * tm;

						int uvIndex = clusterInfo[i]->edgeVerts[j].uvIndex;
						Point3 uvVec1 = op[uvIndex] - center;

						uvVec1 = Normalize(uvVec1);

						float uangle = 0.0f;
						float dot = DotProd(uvVec1,geomVec1);
						if (dot >= 0.9999f)
							uangle += 0.0f;
						else if (dot == 0.0f)
							uangle += PI*0.5f;
						else if (dot <= -0.999f)
							uangle += PI;
						else uangle += acos(dot);

						Point3 crossProd = CrossProd(uvVec1,geomVec1);
						if (crossProd.z > 0.0)
							uangle *= -1.0f;
						angle += uangle;

					}



				}
  				angle = (angle)/(float)(ct-1);
				clusterInfo[i]->counterAngle = angle;
 				float fangle = angle + iangle;

				Matrix3 tm(1);
				tm.RotateZ(iangle);

 				Matrix3 tm2(1);
				tm2.RotateZ(fangle);

	//rotate the ideal points
				//apply any scaling
				BOOL centerIsEdge = edgeVerts[i];

  				for (int j = 0; j < ct; j++)
				{
				
					Point3 target = clusterInfo[i]->edgeVerts[j].idealPos * tm2;
					clusterInfo[i]->edgeVerts[j].tempPos = target;

//					Point3 target2 = clusterInfo[i]->edgeVerts[j].idealPos * tm;
//					clusterInfo[i]->edgeVerts[j].tempPos2 = target2;

					target += center;
					int uvIndex = clusterInfo[i]->edgeVerts[j].uvIndex;
					BOOL isEdge = edgeVerts[uvIndex];
					BOOL useVert = TRUE;
 					if (lockEdges)
					{
//						if (edgeVerts[uvIndex])
//							useVert = FALSE;
					}


  	 				if (useVert)
					{
 						Point3 delta = target - op[uvIndex];
						deltaList[uvIndex] += delta *0.1f;
						deltaCount[uvIndex] += 1;
					}
				}
			}
		}

 		for (int i = 0; i < TVMaps.v.Count(); i++)
		{
			if ((deltaCount[i] > 0) && (clusterInfo[i]))
			{
				clusterInfo[i]->delta += deltaList[i]/(float)deltaCount[i];
				op[i] += deltaList[i]/(float)deltaCount[i];
			}
		}	
	//deltas * influence and add back to our list
	}

 	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		TVMaps.v[i].p = op[i];
	}

	peltData.ResolvePatchHandles(this);

	TransferSelectionEnd(FALSE,FALSE); 


	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
	if (ip) ip->RedrawViews(ip->GetTime());
}
*/
void UnwrapMod::fnRelaxBySprings(int frames, float stretch, BOOL lockEdges)

{
//    int frames = 10;
 //	float stretch = 0.87f;
	float reducer = 1.0f;

	if (!theHold.Holding())
	{
		theHold.Begin();
		HoldPoints();
		theHold.Accept(_T(GetString(IDS_PW_MOVE_UVW)));
	}
	TransferSelectionStart(); 

	//find our boundary edges
	BitArray edgeSprings;
	BitArray springs;
	springs.SetSize(TVMaps.ePtrList.Count());
	springs.ClearAll();
	edgeSprings.SetSize(TVMaps.ePtrList.Count());
	edgeSprings.ClearAll();

	BitArray edgeVerts;
	edgeVerts.SetSize(TVMaps.v.Count());
	edgeVerts.ClearAll();



	BitArray springVerts;
	springVerts.SetSize(TVMaps.v.Count());
	springVerts.ClearAll();

	float uvEdgeDist = 0.0f;
	float geomEdgeDist = 0.0f;

	TimeValue t = GetCOREInterface()->GetTime();

	for (int i = 0; i < TVMaps.v.Count(); i++)
	{

		float wa = 0.0f;
		if (vsel[i]) wa = 1.0f;
		else wa = TVMaps.v[i].influence;
		if (wa > 0.0f)
		{
			Point3 p = TVMaps.v[i].p;		
			p.z = 0.0f;
			TVMaps.v[i].p = p;
			if (TVMaps.cont[i]) TVMaps.cont[i]->SetValue(t,&TVMaps.v[i].p);
			springVerts.Set(i,TRUE);
		}
		
	}
	

	for (int i = 0; i < TVMaps.f.Count(); i++)
	{
		int deg = TVMaps.f[i]->count;
		int ct = 0;
		for (int j = 0; j < deg; j++)
		{
			int a = TVMaps.f[i]->t[j];
			float wa = 0.0f;
			if (vsel[a]) wa = 1.0f;
			else wa = TVMaps.v[a].influence;
			if (wa > 0.0f) ct++;
		}
		if (ct != deg)
		{
			for (int j = 0; j < deg; j++)
			{
				int a = TVMaps.f[i]->t[j];

				float wa = 0.0f;
				if (vsel[a]) wa = 1.0f;
				else wa = TVMaps.v[a].influence;
				if (wa > 0.0f)
					edgeVerts.Set(a,TRUE);
			}

		}
	}

	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;

		if (TVMaps.ePtrList[i]->faceList.Count() == 1)
		{
			edgeVerts.Set(a,TRUE);
			edgeVerts.Set(b,TRUE);
		}
	}

	int ct = 0;
	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		int ga = TVMaps.ePtrList[i]->ga;
		int gb = TVMaps.ePtrList[i]->gb;

		float wa = 0.0f;
		float wb = 0.0f;

		if (vsel[a]) wa = 1.0f;
		else wa = TVMaps.v[a].influence;
		if (vsel[b]) wb = 1.0f;
		else wb = TVMaps.v[b].influence;
		if ((wa > 0.0f) && (wb > 0.0f))
		{
			if (lockEdges)
			{
				if (edgeVerts[a] && edgeVerts[b])
				{

				}
				else if ((!edgeVerts[a]) && (!edgeVerts[b]))
				{
					springs.Set(i,TRUE);
					uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
					geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
					ct++;
				}
				else if ( ((edgeVerts[a]) && (!edgeVerts[b])) ||
					      ((edgeVerts[b]) && (!edgeVerts[a])) )
				{
					springs.Set(i,TRUE);
					edgeSprings.Set(i,TRUE);
					uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
					geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
					ct++;
				}

			}
			else
			{
  				if (edgeVerts[a] && edgeVerts[b])
				{
					springs.Set(i,TRUE);
					uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
					geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
					ct++;
				}
				else if ((!edgeVerts[a]) && (!edgeVerts[b]))
				{
					springs.Set(i,TRUE);
					uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
					geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
					ct++;
				}
				else if ( ((edgeVerts[a]) && (!edgeVerts[b])) ||
					      ((edgeVerts[b]) && (!edgeVerts[a])) )
				{
					springs.Set(i,TRUE);
					uvEdgeDist += Length(TVMaps.v[a].p - TVMaps.v[b].p );
					geomEdgeDist += Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
					ct++;
				}
			}
		}

	}

   	uvEdgeDist = uvEdgeDist/(ct);
	geomEdgeDist = geomEdgeDist/(ct);
	float edgeScale = (uvEdgeDist/geomEdgeDist) *reducer;






	//get the normals at each vertex
	
	Tab<Point3> geomFaceNormals;
	geomFaceNormals.SetCount(TVMaps.f.Count());
	for (int i =0; i < TVMaps.f.Count(); i++)
	{
		geomFaceNormals[i] = TVMaps.GeomFaceNormal(i);
	}

	Tab<Point3> geomVertexNormals;
	Tab<int> geomVertexNormalsCT;
	geomVertexNormals.SetCount(TVMaps.geomPoints.Count());
	geomVertexNormalsCT.SetCount(TVMaps.geomPoints.Count());

	for (int i =0; i < TVMaps.geomPoints.Count(); i++)
	{
		geomVertexNormals[i] = Point3(0.0f,0.0f,0.0f);
		geomVertexNormalsCT[i] = 0;
	}
	for (int i =0; i < TVMaps.f.Count(); i++)
	{
		int deg = TVMaps.f[i]->count;
		for (int j = 0; j < deg; j++)
		{
			int index = TVMaps.f[i]->v[j];
			geomVertexNormalsCT[index] = geomVertexNormalsCT[index] + 1;
			geomVertexNormals[index] += geomFaceNormals[i];
		}
	}

	for (int i =0; i < TVMaps.geomPoints.Count(); i++)
	{
		if (geomVertexNormalsCT[i] != 0)
			geomVertexNormals[i] = geomVertexNormals[i]/(float)geomVertexNormalsCT[i];
		geomVertexNormals[i] = Normalize(geomVertexNormals[i]);

	}

	Tab<EdgeBondage> springEdges;
	Tab<AdjacentItem*> facesAtVert;
 	TVMaps.BuildAdjacentUVFacesToVerts(facesAtVert);

	//loop through our verts
	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
	//get the faces attached to this vert
		if (springVerts[i])
		{
			Tab<int> connectedFaces;
			connectedFaces.SetCount(facesAtVert[i]->index.Count());
			for (int j = 0; j < facesAtVert[i]->index.Count(); j++)
			{
				connectedFaces[j] = facesAtVert[i]->index[j];
			}
		//get the uv/geom verts attached to this vert
			Tab<int> connectedUVVerts;
			Tab<int> connectedGeomVerts;
			int centerGIndex = -1;
			for (int j = 0; j < connectedFaces.Count(); j++)
			{
				
				int faceIndex = connectedFaces[j];
				int deg = TVMaps.f[faceIndex]->count;
				
				for (int k = 0; k < deg; k++)
				{
					BOOL visibleEdge = TRUE;
					if (deg == 3)
					{
						if (k == 0)
							visibleEdge = (!(TVMaps.f[faceIndex]->flags & FLAG_HIDDENEDGEA));
						if (k == 1)
							visibleEdge = (!(TVMaps.f[faceIndex]->flags & FLAG_HIDDENEDGEB));
						if (k == 2)
							visibleEdge = (!(TVMaps.f[faceIndex]->flags & FLAG_HIDDENEDGEC));
					}
					if (visibleEdge)
					{
						int a = TVMaps.f[faceIndex]->t[k];
						int b = TVMaps.f[faceIndex]->t[(k+1)%deg];

						int ga = TVMaps.f[faceIndex]->v[k];
						int gb = TVMaps.f[faceIndex]->v[(k+1)%deg];

						if (a == i)
						{
							connectedUVVerts.Append(1,&b,5);
							connectedGeomVerts.Append(1,&gb,5);
							centerGIndex = ga;
						}
						if (b == i)
						{
							connectedUVVerts.Append(1,&a,5);
							connectedGeomVerts.Append(1,&ga,5);
							centerGIndex = gb;
						}
					}

				}
			}

		//remove the duplicates
	 		BitArray processedVerts;
			processedVerts.SetSize(TVMaps.v.Count());
			processedVerts.ClearAll();

			Tab<int> cUVVerts;
			Tab<int> cGeomVerts;
			for (int j = 0; j < connectedUVVerts.Count(); j++)
			{
				int index = connectedUVVerts[j];
				int gindex = connectedGeomVerts[j];
				if (!processedVerts[index])
				{
					cUVVerts.Append(1,&index,5);
					cGeomVerts.Append(1,&gindex,5);
					processedVerts.Set(index,TRUE);
				}
			}

			Point3 center = TVMaps.geomPoints[centerGIndex];
			Tab<Point3> edgePoints;
			edgePoints.SetCount(cGeomVerts.Count());
//DebugPrint("Center Point %d\n",i);
//DebugPrint("GCenter Point %d\n",centerGIndex);

			for (int j = 0; j < cGeomVerts.Count(); j++)
			{
				int uindex = cUVVerts[j];
//DebugPrint("   UVEPoint %d\n",uindex);  

				int index = cGeomVerts[j];
//DebugPrint("   GeoEPoint %d\n",index);  
				edgePoints[j] = TVMaps.geomPoints[index];
			}
		//flatten the verts 
			Point3 norm = geomVertexNormals[centerGIndex];

			for (int j = 0; j < cGeomVerts.Count(); j++)
			{
				Point3 vec = edgePoints[j] - center;
				float l = Length(vec);
				vec = Normalize(vec);
				Point3 axis = Normalize(CrossProd(norm,vec));


				Quat q = QFromAngAxis(PI*0.5f,axis);
				Matrix3 tm(1);
				
				q.MakeMatrix(tm);
				Point3 ep = norm * tm;

				edgePoints[j] = vec * l;
			}

		//build the springs


		//do the edge to edge spings
			for (int j = 0; j < cGeomVerts.Count(); j++)
			{
				int a = cUVVerts[j];
				int crossIndex = (j+1)%cGeomVerts.Count();
				int b = cUVVerts[crossIndex];
				int ga = cGeomVerts[j];
				int gb = cGeomVerts[crossIndex];
				float d = Length(edgePoints[j] - edgePoints[crossIndex]);
				d = d * edgeScale;

 				EdgeBondage sp;
				sp.v1 = a;
				sp.v2 = b;
				sp.vec1 = -1;
				sp.vec2 = -1;
				sp.dist = d;
				sp.str = 0.9f;
if ( ((a == 7) && ( b == 15)) ||
	 ((a == 15) && ( b == 7)) )
	 DebugPrint("Wait\n");
				sp.distPer = 1.0f;
				sp.isEdge = FALSE;
				sp.edgeIndex = -1;
				springEdges.Append(1,&sp,5000);
			}
		//do the cross springs
 			if (cGeomVerts.Count()> 3)
			{
				for (int j = 0; j < cGeomVerts.Count(); j++)
				{
					int a = cUVVerts[j];
					int crossIndex = (j+(cGeomVerts.Count()/2))%cGeomVerts.Count();
					int b = cUVVerts[crossIndex];
					int ga = cGeomVerts[j];
					int gb = cGeomVerts[crossIndex];
					float d = Length(edgePoints[j] - edgePoints[crossIndex]);
					d = d * edgeScale;

if ( ((a == 7) && ( b == 15)) ||
	 ((a == 15) && ( b == 7)) )
	 DebugPrint("Wait\n");

 					EdgeBondage sp;
					sp.v1 = a;
					sp.v2 = b;
					sp.vec1 = -1;
					sp.vec2 = -1;
					sp.dist = d;
					sp.str =0.90f;

					sp.distPer = 1.0f;
					sp.isEdge = FALSE;
					sp.edgeIndex = -1;
					springEdges.Append(1,&sp,5000);
				}
			}

		}

	}
  	for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
	{
			//loop the through the edges
		int a = TVMaps.ePtrList[i]->a;
		int b = TVMaps.ePtrList[i]->b;
		if (springVerts[a]&&springVerts[b])
		{
			int ga = TVMaps.ePtrList[i]->ga;
			int gb = TVMaps.ePtrList[i]->gb;

			int veca = TVMaps.ePtrList[i]->avec;
			int vecb = TVMaps.ePtrList[i]->bvec;
			BOOL isHidden = TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA;




			EdgeBondage sp;
			sp.v1 = a;
			sp.v2 = b;
			sp.vec1 = veca;
			sp.vec2 = vecb;
			float dist = Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
			dist = dist * edgeScale;
			sp.dist = dist;
			sp.str = 1.0f;

			sp.distPer = 1.0f;
			sp.isEdge = FALSE;
			sp.edgeIndex = i;
			springEdges.Append(1,&sp,5000);

			if ((isHidden) && (TVMaps.ePtrList[i]->faceList.Count() > 1))
			{
				//get face 1
				int a1,b1;
				a1 = -1;
				b1 = -1;
				int ga1,gb1;

				int faceIndex = TVMaps.ePtrList[i]->faceList[0];

				int deg = TVMaps.f[faceIndex]->count;
				for (int j = 0; j < deg; j++)
				{
					int tvA = TVMaps.f[faceIndex]->t[j];
					if ((tvA != a) && (tvA != b))
					{
						a1 = tvA;
						ga1 = TVMaps.f[faceIndex]->v[j];
					}

				}

				//get face 2
				faceIndex = TVMaps.ePtrList[i]->faceList[1];
				deg = TVMaps.f[faceIndex]->count;
				for (int j = 0; j < deg; j++)
				{
					int tvA = TVMaps.f[faceIndex]->t[j];
					if ((tvA != a) && (tvA != b))
					{
						b1 = tvA;
						gb1 = TVMaps.f[faceIndex]->v[j];
					}
				}

				if ((a1 != -1) && (b1 != -1))
				{

					if (edgeVerts[a1])
					{
						int ta = a1;
						a1 = b1;
						b1 = ta;

						ta = ga1;
						ga1 = gb1;
						gb1 = ta;


					}


 					EdgeBondage sp;
					sp.v1 = a1;
					sp.v2 = b1;
					sp.vec1 = -1;
					sp.vec2 = -1;
					float dist = Length(TVMaps.geomPoints[ga1]-TVMaps.geomPoints[gb1]);
					sp.dist = dist * edgeScale;
					sp.str = 1.0f;
//					if (lockEdges)
//						sp.str = 0.0f;

					sp.distPer = 1.0f;

					float wa = 0.0f;
					float wb = 0.0f;

					if (vsel[a]) wa = 1.0f;
					else wa = TVMaps.v[a].influence;
					if (vsel[b]) wb = 1.0f;
					else wb = TVMaps.v[b].influence;

					sp.isEdge = FALSE;


					sp.edgeIndex = -1;
					springEdges.Append(1,&sp,5000);
				}
			}
			
		}
	}



	for (int i = 0; i < facesAtVert.Count(); i++)
	{
		delete facesAtVert[i];
	}


	Solver solver;

/*
	int ne = edgeVerts.NumberSet();
	//create our solver

 	


	{
		for (int i = 0; i < TVMaps.ePtrList.Count(); i++)
		{
			if ((edgeSprings[i] || springs[i]) )
			{
			//loop the through the edges
			int a = TVMaps.ePtrList[i]->a;
			int b = TVMaps.ePtrList[i]->b;
			int ga = TVMaps.ePtrList[i]->ga;
			int gb = TVMaps.ePtrList[i]->gb;

			int veca = TVMaps.ePtrList[i]->avec;
			int vecb = TVMaps.ePtrList[i]->bvec;
			BOOL isHidden = TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA;


			if (edgeVerts[a])
			{
				int ta = a;
				a = b;
				b = ta;

				ta = ga;
				ga = gb;
				gb = ta;

				ta = veca;
				veca = vecb;
				vecb = ta;
			}


 			EdgeBondage sp;
			sp.v1 = a;
			sp.v2 = b;
			sp.vec1 = veca;
			sp.vec2 = vecb;
			float dist = Length(TVMaps.geomPoints[ga]-TVMaps.geomPoints[gb]);
			dist = dist * edgeScale;
			sp.dist = dist;
			sp.str = 1.0f;

			sp.distPer = 1.0f;
			sp.isEdge = edgeSprings[i] || edgeVerts[a] || edgeVerts[b];
			if (!lockEdges)
				sp.isEdge = FALSE;
			sp.edgeIndex = i;
			springEdges.Append(1,&sp,5000);

			//add a spring for each edge
			//if edge is not visible find cross edge
			if ((isHidden) && (TVMaps.ePtrList[i]->faceList.Count() > 1))
			{
				//get face 1
				int a1,b1;
				a1 = -1;
				b1 = -1;
				int ga1,gb1;

				int faceIndex = TVMaps.ePtrList[i]->faceList[0];

				int deg = TVMaps.f[faceIndex]->count;
				for (int j = 0; j < deg; j++)
				{
					int tvA = TVMaps.f[faceIndex]->t[j];
					if ((tvA != a) && (tvA != b))
					{
						a1 = tvA;
						ga1 = TVMaps.f[faceIndex]->v[j];
					}

				}

				//get face 2
				faceIndex = TVMaps.ePtrList[i]->faceList[1];
				deg = TVMaps.f[faceIndex]->count;
				for (int j = 0; j < deg; j++)
				{
					int tvA = TVMaps.f[faceIndex]->t[j];
					if ((tvA != a) && (tvA != b))
					{
						b1 = tvA;
						gb1 = TVMaps.f[faceIndex]->v[j];
					}
				}

				if ((a1 != -1) && (b1 != -1))
				{

					if (edgeVerts[a1])
					{
						int ta = a1;
						a1 = b1;
						b1 = ta;

						ta = ga1;
						ga1 = gb1;
						gb1 = ta;


					}


 					EdgeBondage sp;
					sp.v1 = a1;
					sp.v2 = b1;
					sp.vec1 = -1;
					sp.vec2 = -1;
					float dist = Length(TVMaps.geomPoints[ga1]-TVMaps.geomPoints[gb1]);
					sp.dist = dist * edgeScale;
					sp.str = 1.0f;
//					if (lockEdges)
//						sp.str = 0.0f;

					sp.distPer = 1.0f;

					float wa = 0.0f;
					float wb = 0.0f;

					if (vsel[a]) wa = 1.0f;
					else wa = TVMaps.v[a].influence;
					if (vsel[b]) wb = 1.0f;
					else wb = TVMaps.v[b].influence;

					if ((wa > 0.0f) && (wb > 0.0f))
					{
						if (edgeVerts[a1] || edgeVerts[b1])
							sp.isEdge = TRUE;
						else sp.isEdge = FALSE;
					}
					else sp.isEdge = TRUE;

					if (!lockEdges)
						sp.isEdge = FALSE;

					sp.edgeIndex = -1;
					springEdges.Append(1,&sp,5000);
				}
			}
			}
		}
	}

*/
	
	Tab<SpringClass> verts;  //this is a list off all our verts, there positions and velocities

	verts.SetCount(TVMaps.v.Count());
	for (int i = 0; i < TVMaps.v.Count(); i++)
	{
		verts[i].pos = TVMaps.v[i].p;
		verts[i].vel = Point3(0.0f,0.0f,0.0f);

		float wa = 0.0f;
		if (vsel[i]) wa = 1.0f;
		else wa = TVMaps.v[i].influence;
		verts[i].weight = wa;
	}

	//build our distance from edge

	BitArray processedVerts;
	processedVerts = edgeVerts;
	int distLevel = 0;
/*
	for (int i = 0; i < springEdges.Count(); i++)
	{
		springEdges[i].distanceFromEdge = -1;
	}

	BOOL done = FALSE;
	while (!done)
	{
		int numberSet = processedVerts.NumberSet();

		BitArray holdVerts = processedVerts;
		for (int i = 0; i < springEdges.Count(); i++)
		{
			int a = springEdges[i].v1;
			int b = springEdges[i].v2;

			if (holdVerts[a] && (!holdVerts[b]))
			{
				springEdges[i].distanceFromEdge = distLevel;
				processedVerts.Set(b,TRUE);
			}
			if (holdVerts[b] && (!holdVerts[a]))
			{
				springEdges[i].distanceFromEdge = distLevel;
				processedVerts.Set(a,TRUE);
			}
			if (holdVerts[b] && (holdVerts[a]) && springEdges[i].distanceFromEdge == -1)
			{
				springEdges[i].distanceFromEdge = distLevel;
				processedVerts.Set(a,TRUE);
			}

		}

		distLevel++;
		if (numberSet == processedVerts.NumberSet())
			done = TRUE;
	}
*/

	edgeWeight.SetCount(TVMaps.ePtrList.Count());
  	for (int i = 0; i < edgeWeight.Count(); i++)
		edgeWeight[i] = -1.0f;

//	dverts = vsel;
//	dverts.ClearAll();

    for (int i = 0; i < springEdges.Count(); i++)
	{
		if (!lockEdges)
			springEdges[i].isEdge = FALSE;
		else
		{
			int a = springEdges[i].v1;
			int b = springEdges[i].v2;
			//look for an egde with 2 edge verts delete it
			if (edgeVerts[a] && edgeVerts[b])
			{
				springEdges.Delete(i,1);
				i--;
			}
			else if ((!edgeVerts[a]) && edgeVerts[b])
			{
				springEdges[i].isEdge = TRUE;
//				dverts.Set(a);
			}
			else if ((!edgeVerts[b]) && edgeVerts[a])
			{
//				dverts.Set(b);
				springEdges[i].isEdge = TRUE;
				int temp;
				
				temp = springEdges[i].v1;
				springEdges[i].v1 = springEdges[i].v2;
				springEdges[i].v2 = temp;
			//look for one
			//swap a and b if need be
			}
		}
	}



//	solver.Solve(0, frames, 5,
//		springEdges, verts,
//		0.16f,0.16f,0.25f,this);

// 	dspringEdges = springEdges;

   	for (int k = 0; k < frames; k++)
	{
		BOOL iret = solver.Solve(0, frames, 2,
			springEdges, verts,
 			0.001f,0.001f,0.25f,this);

		if (iret == FALSE)
			k = frames;


		for (int j = 0; j < springEdges.Count(); j++)
		{
			float d = springEdges[j].dist;
			int a = springEdges[j].v1;
			int b = springEdges[j].v2;
			if (edgeVerts[a] && edgeVerts[b])
			{
			}
			else
			{
				
				springEdges[j].dist *= stretch; 
			}
		}
	}


	//reduce our spring lengths and repeat

	//put the data back

	UVW_ChannelClass *tvData = &TVMaps;
	for (int j = 0; j < verts.Count(); j++)
	{
		Point3 p = verts[j].pos	;		

		tvData->v[j].p = p;
		if (tvData->cont[j]) tvData->cont[j]->SetValue(t,&tvData->v[j].p);

	}
	
	peltData.ResolvePatchHandles(this);



	TransferSelectionEnd(FALSE,FALSE); 


	NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
	InvalidateView();
	if (ip) ip->RedrawViews(ip->GetTime());
}

void	UnwrapMod::SetRelaxBySpringDialogPos()
	{
	if (relaxWindowPos.length != 0) 
		SetWindowPlacement(relaxBySpringHWND,&relaxWindowPos);
	}

void	UnwrapMod::SaveRelaxBySpringDialogPos()
	{
	relaxWindowPos.length = sizeof(WINDOWPLACEMENT); 
	GetWindowPlacement(relaxBySpringHWND,&relaxWindowPos);
	}





INT_PTR CALLBACK UnwrapRelaxBySpringFloaterDlgProc(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
	UnwrapMod *mod = DLGetWindowLongPtr<UnwrapMod*>(hWnd);
	//POINTS p = MAKEPOINTS(lParam);	commented out by sca 10/7/98 -- causing warning since unused.
	
	static ISpinnerControl *iStretch = NULL;
	static ISpinnerControl *iIterations = NULL;

	static float stretch = 0.0f;
	static int iterations = 10;
	static BOOL useOnlyVEdges = FALSE;

	switch (msg) {
		case WM_INITDIALOG:
			{


			mod = (UnwrapMod*)lParam;
			mod->relaxBySpringHWND = hWnd;

			DLSetWindowLongPtr(hWnd, lParam);

//save our points

//create relax amount spinner and set value
			iStretch = SetupFloatSpinner(
				hWnd,IDC_RELAX_STRETCHSPIN,IDC_RELAX_STRETCH,
				0.0f,1.0f,mod->fnGetRelaxBySpringStretch());	
			iStretch->SetScale(0.01f);
			stretch = mod->fnGetRelaxBySpringStretch();

			iIterations = SetupIntSpinner(
				hWnd,IDC_RELAX_ITERATIONSSPIN,IDC_RELAX_ITERATIONS,
				0,1000,mod->fnGetRelaxBySpringIteration());	
			iIterations->SetScale(1.f);
			iterations = mod->fnGetRelaxBySpringIteration();

			
//restore window pos
			mod->SetRelaxBySpringDialogPos();
//start the hold begin
			if (!theHold.Holding())
				{
				theHold.SuperBegin();
				theHold.Begin();
				}
//hold the points 
			mod->HoldPoints();

			float str = 1.0f - (stretch*0.01f);
			mod->fnRelaxBySprings(iterations, str,mod->fnGetRelaxBySpringVEdges());
//stitch initial selection
//			mod->RelaxVerts2(amount,iterations,bBoundary,bCorner);

			mod->InvalidateView();
			TimeValue t = GetCOREInterface()->GetTime();
			mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
			GetCOREInterface()->RedrawViews(t);

			break;
			}



		case CC_SPINNER_CHANGE:
			if ( (LOWORD(wParam) == IDC_RELAX_STRETCHSPIN) ||
				 (LOWORD(wParam) == IDC_RELAX_ITERATIONSSPIN) )
				{
				stretch = iStretch->GetFVal();
				iterations = iIterations->GetIVal();
				theHold.Restore();

				float str = 1.0f - (stretch);
				mod->fnRelaxBySprings(iterations, str,mod->fnGetRelaxBySpringVEdges());

//				mod->RelaxVerts2(amount,iterations,bBoundary,bCorner);

				mod->InvalidateView();
				TimeValue t = GetCOREInterface()->GetTime();
				mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
				GetCOREInterface()->RedrawViews(t);
				}
			break;

		case WM_CUSTEDIT_ENTER:
		case CC_SPINNER_BUTTONUP:
			if ( (LOWORD(wParam) == IDC_RELAX_STRETCHSPIN) || (LOWORD(wParam) == IDC_RELAX_ITERATIONSSPIN) )
				{
				mod->InvalidateView();
				TimeValue t = GetCOREInterface()->GetTime();
				mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
				GetCOREInterface()->RedrawViews(t);

				}
			break;


		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_APPLY:
					{
					theHold.Accept(_T(GetString(IDS_PW_RELAX)));
					theHold.SuperAccept(_T(GetString(IDS_PW_RELAX)));

					theHold.SuperBegin();
					theHold.Begin();
					mod->HoldPoints();

					float str = 1.0f - (stretch*0.01f);
					mod->fnRelaxBySprings(iterations, str,mod->fnGetRelaxBySpringVEdges());

//					mod->RelaxVerts2(amount,iterations,bBoundary,bCorner);
					mod->InvalidateView();
					TimeValue t = GetCOREInterface()->GetTime();
					mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
					GetCOREInterface()->RedrawViews(t);

					break;
					}
				case IDC_APPLYOK:
					{
					theHold.Accept(_T(GetString(IDS_PW_RELAX)));
					theHold.SuperAccept(_T(GetString(IDS_PW_RELAX)));
					mod->SaveRelaxBySpringDialogPos();

					ReleaseISpinner(iStretch);
					iStretch = NULL;
					ReleaseISpinner(iIterations);
					iIterations = NULL;

					 EndDialog(hWnd,1);
					
					break;
					}
				case IDC_REVERT:
					{
					theHold.Restore();
					theHold.Cancel();
					theHold.SuperCancel();
				
					mod->InvalidateView();

					ReleaseISpinner(iStretch);
					iStretch = NULL;
					ReleaseISpinner(iIterations);
					iIterations = NULL;
					EndDialog(hWnd,0);

					break;
					}
				case IDC_DEFAULT:
					{
//get bias
					stretch = iStretch->GetFVal();
					iterations = iIterations->GetIVal();

					mod->fnSetRelaxBySpringStretch(stretch);
					mod->fnSetRelaxBySpringIteration(iterations);

					break;
					}

				}
			break;

		default:
			return FALSE;
		}
	return TRUE;
	}


void UnwrapMod::fnRelaxBySpringsDialog()
{

//bring up the dialog
	DialogBoxParam(	hInstance,
							MAKEINTRESOURCE(IDD_RELAXBYSPRINGDIALOG),
							GetCOREInterface()->GetMAXHWnd(),
//							hWnd,
							UnwrapRelaxBySpringFloaterDlgProc,
							(LPARAM)this );


}