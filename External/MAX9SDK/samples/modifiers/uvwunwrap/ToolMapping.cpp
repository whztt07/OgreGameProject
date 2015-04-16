
#include "unwrap.h"


void UnwrapMod::DrawGizmo(TimeValue t, INode* inode, ModContext *mc, GraphicsWindow *gw)
{
	 	ComputeSelectedFaceData();

		Matrix3 vtm(1);
		Interval iv;
		if (inode) 
			vtm = inode->GetObjectTM(t,&iv);
		Point3 a(-0.5f,-0.5f,0.0f),b(0.5f,-0.5f,0.0f),c(0.5f,0.5f,0.0f),d(-0.5f,0.5f,0.0f);
		
		Matrix3 modmat, ntm = inode->GetObjectTM(t);

/*
		Matrix3 ctm(1);
		if (tmControl) 
		{
			tmControl->GetValue(t,&ctm,FOREVER,CTRL_RELATIVE);
		}
		modmat = ctm * ntm;
*/

//		 ntm = inode->GetObjectTM(t);
		modmat = CompMatrix(t,mc,&ntm);

		gw->setTransform(modmat);	

		if ( (fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == PELTMAP) )
		{
			Point3 line[5];
			line[0] =   a;
			line[1] =   b;
			line[2] =   c;
			line[3] =   d;
			line[4] = line[0];
			gw->setColor(LINE_COLOR,GetUIColor(COLOR_SEL_GIZMOS));
			gw->polyline(5, line, NULL, NULL, 0);
		}
		else if (fnGetMapMode() == CYLINDRICALMAP)
		{
			//draw the bottom circle
			int segs = 32;
			float angle = 0.0f;
			float inc = 1.0f/(float)segs * 2 * PI;
			
			Point3 prevVec;
			gw->startSegments();
	 		Point3 pl[3];
			gw->setColor(LINE_COLOR,GetUIColor(COLOR_SEL_GIZMOS));
			for (int i = 0; i < (segs+1); i++)
			{
				
				Matrix3 tm(1);
				tm.RotateZ(angle);
				Point3 vec (0.50f,0.0f,0.0f);
				
 				vec = vec * tm;
				if ( i >= 1)
				{
					pl[0] = vec;
					pl[1] = prevVec;
					gw->segment(pl,1);


					pl[0].z = 1.0f;
					pl[1].z = 1.0f;
					gw->segment(pl,1);

					if (((i%4) == 0) && (i != segs))
					{
						pl[0] = vec;
						pl[1] = vec;
						pl[1].z = 1.0f;
						gw->segment(pl,1);
					}

				}
				prevVec = vec;
				angle += inc;
			}


			Color c(openEdgeColor);
			gw->setColor(LINE_COLOR,c);
			
			pl[0] = Point3(0.50f,0.0f,0.0f);
			pl[1] = Point3(0.50f,0.0f,0.0f);
			pl[1].z = 1.0f;
			gw->segment(pl,1);

			gw->endSegments();
		}
		else if (fnGetMapMode() == SPHERICALMAP)
		{
			//draw the bottom circle
			int segs = 32;

			float inc = 1.0f/(float)segs * 2 * PI;
			
			
			gw->startSegments();
	 		Point3 pl[3];
 			Color c(openEdgeColor);
			gw->setColor(LINE_COLOR,GetUIColor(COLOR_SEL_GIZMOS));
			pl[0] = Point3(0.0f,0.0f,.6f);
			pl[1] = Point3(0.0f,0.0f,-.5f);
			gw->segment(pl,1);
			
			for (int j = 0; j < 3; j++)
			{
				float angle = 0.0f;
				Point3 prevVec;

 				gw->setColor(LINE_COLOR,GetUIColor(COLOR_SEL_GIZMOS));
				for (int i = 0; i < (segs+1); i++)
				{
					
					Matrix3 tm(1);
					Point3 vec (0.50f,0.0f,0.0f);
					if (j == 0)
						tm.RotateZ(angle);
					if (j == 1)
					{
						vec = Point3(0.0f,0.5f,0.0f);
						tm.RotateX(angle);
					}
 					if (j == 2)
					{
						vec = Point3(0.0f,0.0f,-0.5f);
						if (i < ((segs+2)/2))
							gw->setColor(LINE_COLOR,GetUIColor(COLOR_SEL_GIZMOS));
						else gw->setColor(LINE_COLOR,c);
						tm.RotateY(angle);
					}
					
					
 					vec = vec * tm;
					if ( i >= 1)
					{
						pl[0] = vec;
						pl[1] = prevVec;
						gw->segment(pl,1);

					}
					prevVec = vec;
					angle += inc;
				}
			}
			gw->endSegments();
		}
		else if (fnGetMapMode() == BOXMAP) 
		{
 			Point3 line[3];
			gw->setColor(LINE_COLOR,GetUIColor(COLOR_SEL_GIZMOS));
			gw->startSegments();

			line[0] =  Point3(-0.5f,-0.5f,-0.5f);
			line[1] =  Point3(0.5f,-0.5f,-0.5f);
			gw->segment(line,1);

			line[0] =  Point3(-0.5f,0.5f,-0.5f);
			line[1] =  Point3(0.5f,0.5f,-0.5f);
			gw->segment(line,1);

			line[0] =  Point3(0.5f,0.5f,-0.5f);
			line[1] =  Point3(0.5f,-0.5f,-0.5f);
			gw->segment(line,1);

			line[0] =  Point3(-0.5f,0.5f,-0.5f);
			line[1] =  Point3(-0.5f,-0.5f,-0.5f);
			gw->segment(line,1);




			line[0] =  Point3(-0.5f,-0.5f,0.5f);
		 	line[1] =  Point3(0.5f,-0.5f,0.5f);
			gw->segment(line,1);

			line[0] =  Point3(-0.5f,0.5f,0.5f);
			line[1] =  Point3(0.5f,0.5f,0.5f);
			gw->segment(line,1);

			line[0] =  Point3(0.5f,0.5f,0.5f);
			line[1] =  Point3(0.5f,-0.5f,0.5f);
			gw->segment(line,1);

			line[0] =  Point3(-0.5f,0.5f,0.5f);
			line[1] =  Point3(-0.5f,-0.5f,0.5f);
			gw->segment(line,1);



			line[0] =  Point3(-0.5f,-0.5f,0.5f);
			line[1] =  Point3(-0.5f,-0.5f,-0.5f);			
			gw->segment(line,1);

			line[0] =  Point3(0.5f,-0.5f,0.5f);
			line[1] =  Point3(0.5f,-0.5f,-0.5f);			
			gw->segment(line,1);

			line[0] =  Point3(-0.5f,0.5f,0.5f);
			line[1] =  Point3(-0.5f,0.5f,-0.5f);			
			gw->segment(line,1);

			line[0] =  Point3(0.5f,0.5f,0.5f);
			line[1] =  Point3(0.5f,0.5f,-0.5f);			
			gw->segment(line,1);





			

			gw->endSegments();
		}

}

void UnwrapMod::GetOpenEdges(Tab<int> &openEdges, Tab<int> &results)
{
	if (openEdges.Count() == 0) return;

	int seedEdgeA = openEdges[0];

	if (openEdges.Count() > 0)
	{
		int initialEdge = seedEdgeA;
		results.Append(1,&seedEdgeA,5000);
		openEdges.Delete(0,1);

		int seedVert = TVMaps.gePtrList[seedEdgeA]->a;
		BOOL done = FALSE;
		int ct = openEdges.Count();
		while (!done)
		{
			for (int i = 0; i < openEdges.Count(); i++)
			{
				int edgeIndex = openEdges[i];

				if (edgeIndex != seedEdgeA)
				{
					int a = TVMaps.gePtrList[edgeIndex]->a;
					int b = TVMaps.gePtrList[edgeIndex]->b;
					if (a == seedVert)
					{
						seedVert = b;
						seedEdgeA = edgeIndex;
						results.Append(1,&seedEdgeA,5000);
						openEdges.Delete(i,1);
						
						i = openEdges.Count();
					}
					else if  (b == seedVert)
					{
						seedVert = a;
						seedEdgeA = edgeIndex;
						results.Append(1,&seedEdgeA,5000);
						openEdges.Delete(i,1);
						i = openEdges.Count();
					}
				}
			}					
			if (ct == openEdges.Count())
				done = TRUE;
			ct = openEdges.Count();
		}


		seedEdgeA = initialEdge;

		seedVert = TVMaps.gePtrList[seedEdgeA]->b;
		done = FALSE;
		ct = openEdges.Count();
		while (!done)
		{
			for (int i = 0; i < openEdges.Count(); i++)
			{
				int edgeIndex = openEdges[i];

				if (edgeIndex != seedEdgeA)
				{
					int a = TVMaps.gePtrList[edgeIndex]->a;
					int b = TVMaps.gePtrList[edgeIndex]->b;
					if (a == seedVert)
					{
						seedVert = b;
						seedEdgeA = edgeIndex;
						results.Append(1,&seedEdgeA,5000);
						openEdges.Delete(seedEdgeA,1);
						i = openEdges.Count();
					}
					else if  (b == seedVert)
					{
						seedVert = a;
						seedEdgeA = edgeIndex;
						results.Append(1,&seedEdgeA,5000);
						openEdges.Delete(seedEdgeA,1);
						i = openEdges.Count();
					}
				}
			}					
			if (ct == openEdges.Count())
				done = TRUE;
			ct = openEdges.Count();
		}
	}

}


void UnwrapMod::fnGizmoReset()
{
	if (gfaces.Count() == 0) return;

	theHold.Begin();
	SuspendAnimate();
	AnimateOff();
	TimeValue t = GetCOREInterface()->GetTime();

	ModContextList mcList;
	INodeTab nodeList;
	ip->GetModContexts(mcList,nodeList);

	// MC tm
	Matrix3 mctm(1);
	if (mcList[0]->tm) mctm = *mcList[0]->tm;

	
	//get our selection
	Box3 bounds;
	bounds.Init();
	//get the bounding box
	
	for (int k=0; k<gfaces.Count(); k++) 
	{
			// Grap the three points, xformed
		int pcount = 3;
			//				if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
		pcount = gfaces[k]->count;

		Point3 temp_point[4];
		for (int j=0; j<pcount; j++) 
		{
			int index = gfaces[k]->t[j];
			bounds += gverts.d[index].p;
		}
	}	
	
	Matrix3 tm(1);
	
	//if just a primary axis set the tm;
	Point3 center = bounds.Center();
		// build the scale
 	Point3 scale(bounds.Width().x ,bounds.Width().y , bounds.Width().z);
	if (scale.x == 0.0f) scale.x = 1.0f;
	if (scale.y == 0.0f) scale.y = 1.0f;
 	if (scale.z == 0.0f) scale.z = 1.0f;
	float scl = scale.x;
	if (scale.y > scl) scl = scale.y;
	if (scale.z > scl) scl = scale.z;
	scale.x = scl;
	scale.y = scl;
	scale.z = scl;
	
	tm.SetRow(0,Point3(scale.x,0.0f,0.0f));
	tm.SetRow(1,Point3(0.0f,scale.y,0.0f));
	tm.SetRow(2,Point3(0.0f,0.0f,scale.z));
	if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == PELTMAP)|| (fnGetMapMode() == SPHERICALMAP))
		tm.SetRow(3,center);
	else if (fnGetMapMode() == CYLINDRICALMAP)
	{
		center.z = bounds.pmin.z;
		tm.SetRow(3,center);
	}
		

	Matrix3 ptm(1), id(1);
	tm = tm * mctm;
	SetXFormPacket tmpck(tm,ptm);
	tmControl->SetValue(t,&tmpck,TRUE,CTRL_RELATIVE);
	ResumeAnimate();

	if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
		ApplyGizmo();

	theHold.Accept(GetString(IDS_MAPPING_RESET));
	if (ip) ip->RedrawViews(ip->GetTime());
}

void UnwrapMod::fnAlignAndFit(int axis)
{
	if (gfaces.Count() == 0) 
		return;
	
	theHold.Begin();
	SuspendAnimate();
	AnimateOff();
	TimeValue t = GetCOREInterface()->GetTime();

	ModContextList mcList;
	INodeTab nodeList;
	ip->GetModContexts(mcList,nodeList);

	// MC tm
	Matrix3 mctm(1);
	if (mcList[0]->tm) mctm = *mcList[0]->tm;


	//get our selection
	Box3 bounds;
	bounds.Init();
	//get the bounding box
	Point3 pnorm(0.0f,0.0f,0.0f);
	for (int k=0; k<gfaces.Count(); k++) 
	{
			// Grap the three points, xformed
		int pcount = 3;
			//				if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
		pcount = gfaces[k]->count;

		Point3 temp_point[4];
		for (int j=0; j<pcount; j++) 
		{
			int index = gfaces[k]->t[j];
			bounds += gverts.d[index].p;
			if (j < 4)
				temp_point[j] = gverts.d[index].p;
		}
		pnorm += Normalize(temp_point[1]-temp_point[0]^temp_point[2]-temp_point[1]);
	}	
	
	pnorm = pnorm / (float) gfaces.Count();
	Matrix3 tm(1);
	
	//if just a primary axis set the tm;
	Point3 center = bounds.Center();
		// build the scale
	Point3 scale(bounds.Width().x ,bounds.Width().y , bounds.Width().z);
	if (scale.x == 0.0f) scale.x = 1.0f;
	if (scale.y == 0.0f) scale.y = 1.0f;
 	if (scale.z == 0.0f) scale.z = 1.0f;
	
 	if (axis == 0) // x axi
	{

  		tm.SetRow(0,Point3(0.0f,-scale.y,0.0f));
		tm.SetRow(1,Point3(0.0f,0.0f,scale.z));
		tm.SetRow(2,Point3(scale.x,0.0f,0.0f));
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == PELTMAP)  || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			tm.SetRow(3,center);
		else if (fnGetMapMode() == CYLINDRICALMAP)
		{
			center.x = bounds.pmin.x;
			tm.SetRow(3,center);
		}		

		Matrix3 ptm(1), id(1);
		tm = tm * mctm;
		SetXFormPacket tmpck(tm,ptm);
		tmControl->SetValue(t,&tmpck,TRUE,CTRL_RELATIVE);
	}
	else if (axis == 1) // y axi
	{
  		tm.SetRow(0,Point3(scale.x,0.0f,0.0f));
 		tm.SetRow(1,Point3(0.0f,0.0f,scale.z));
		tm.SetRow(2,Point3(0.0f,scale.y,0.0f));
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == PELTMAP)|| (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			tm.SetRow(3,center);
		else if (fnGetMapMode() == CYLINDRICALMAP)
		{
			center.y = bounds.pmin.y;
			tm.SetRow(3,center);
		}
		

		Matrix3 ptm(1), id(1);
		tm = tm * mctm;
		SetXFormPacket tmpck(tm,ptm);
		tmControl->SetValue(t,&tmpck,TRUE,CTRL_RELATIVE);
	}
	else if (axis == 2) //z axi
	{
		tm.SetRow(0,Point3(scale.x,0.0f,0.0f));
		tm.SetRow(1,Point3(0.0f,scale.y,0.0f));
		tm.SetRow(2,Point3(0.0f,0.0f,scale.z));
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == PELTMAP)|| (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			tm.SetRow(3,center);
		else if (fnGetMapMode() == CYLINDRICALMAP)
		{
			center.z = bounds.pmin.z;
			tm.SetRow(3,center);
		}
		

		Matrix3 ptm(1), id(1);
		tm = tm * mctm;
		SetXFormPacket tmpck(tm,ptm);
		tmControl->SetValue(t,&tmpck,TRUE,CTRL_RELATIVE);
	}
	else if (axis == 3) // normal
	{
		if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == PELTMAP))
		{
			//get our tm
			Matrix3 tm;
			UnwrapMatrixFromNormal(pnorm,tm);
 			Matrix3 itm = Inverse(tm);
			//find our x and y scale
			float xmax = 0.0f;
			float ymax = 0.0f;
			float zmax = 0.0f;
			Box3 localBounds;
			localBounds.Init();
			for (int k=0; k<gfaces.Count(); k++) 
			{
					// Grap the three points, xformed
				int pcount = 3;
					//				if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
				pcount = gfaces[k]->count;

				Point3 temp_point[4];
				for (int j=0; j<pcount; j++) 
				{
					int index = gfaces[k]->t[j];
					Point3 p = gverts.d[index].p * itm;
					localBounds += p;
				}
			}

			center = localBounds.Center() * tm;
			xmax = localBounds.pmax.x - localBounds.pmin.x;
			ymax = localBounds.pmax.y - localBounds.pmin.y;
			zmax = localBounds.pmax.z - localBounds.pmin.z;

			Point3 vec;
			vec = Normalize(tm.GetRow(0)) * xmax;
			tm.SetRow(0,vec);

			vec = Normalize(tm.GetRow(1)) * ymax;
			tm.SetRow(1,vec);

			vec = Normalize(tm.GetRow(2)) * zmax;
			tm.SetRow(2,vec);


			tm.SetRow(3,center);
			

			Matrix3 ptm(1), id(1);
			tm = tm * mctm;
			SetXFormPacket tmpck(tm,ptm);
			tmControl->SetValue(t,&tmpck,TRUE,CTRL_RELATIVE);
		}
		else if ((fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP)|| (fnGetMapMode() == BOXMAP))
		{
			//get our first 2 rings
			Tab<int> openEdges;
			Tab<int> startRing;
			Tab<int> endRing;

			
			for (int i = 0; i < TVMaps.gePtrList.Count(); i++)
			{
				int numberSelectedFaces = 0;
				int ct = TVMaps.gePtrList[i]->faceList.Count();
				for (int j = 0; j < ct; j++)
				{
					int faceIndex = TVMaps.gePtrList[i]->faceList[j];
					if (fsel[faceIndex])
						numberSelectedFaces++;
				}
				if (numberSelectedFaces == 1)
				{
					openEdges.Append(1,&i,1000);
					
				}
			}

			GetOpenEdges(openEdges, startRing);
			GetOpenEdges(openEdges, endRing);
			Point3 zVec = pnorm;

			Point3 centerS(0.0f,0.0f,0.0f), centerE;
			if ((startRing.Count() != 0) && (endRing.Count() != 0))
			{
				//get the center start
				Box3 BoundsS, BoundsE;
				BoundsS.Init();
				BoundsE.Init();

				//get the center end
				for (int i = 0; i < startRing.Count(); i++)
				{
					int eIndex = startRing[i];
					int a = TVMaps.gePtrList[eIndex]->a;
					int b = TVMaps.gePtrList[eIndex]->b;

					BoundsS += TVMaps.geomPoints[a];
					BoundsS += TVMaps.geomPoints[b];
				}


				for (int i = 0; i < endRing.Count(); i++)
				{
					int eIndex = endRing[i];
					int a = TVMaps.gePtrList[eIndex]->a;
					int b = TVMaps.gePtrList[eIndex]->b;

					BoundsE += TVMaps.geomPoints[a];
					BoundsE += TVMaps.geomPoints[b];
				}

				
				centerS = BoundsS.Center();
				centerE = BoundsE.Center();
				//create the vec
				zVec = centerE - centerS;

			}
			else if ((startRing.Count() != 0) && (endRing.Count() == 0))
			{
				//get the center start
				Box3 BoundsS;
				BoundsS.Init();
				

				//get the center end
				for (int i = 0; i < startRing.Count(); i++)
				{
					int eIndex = startRing[i];
					int a = TVMaps.gePtrList[eIndex]->a;
					int b = TVMaps.gePtrList[eIndex]->b;

					BoundsS += TVMaps.geomPoints[a];
					BoundsS += TVMaps.geomPoints[b];
				}

				centerS = BoundsS.Center();
				

				int farthestPoint= -1;
				Point3 fp;
				float farthestDist= 0.0f;
				for (int k=0; k<gfaces.Count(); k++) 
				{
						// Grap the three points, xformed
					int pcount = 3;
						//				if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
					pcount = gfaces[k]->count;

					
					for (int j=0; j<pcount; j++) 
					{
						int index = gfaces[k]->t[j];
						
						Point3 p = gverts.d[index].p;
						float d = LengthSquared(p-centerS);
						if ((d > farthestDist) || (farthestPoint == -1))
						{
							farthestDist = d;
							farthestPoint = index;
							fp = p;
						}
						
					}
				}

				
				
				centerE = fp;
				//create the vec
				zVec = centerE - centerS;

			}
			else
			{
				zVec = Point3(0.0f,0.0f,1.0f);
			}


			//get our tm
			Matrix3 tm;
			UnwrapMatrixFromNormal(zVec,tm);
			tm.SetRow(3,centerS);
 			Matrix3 itm = Inverse(tm);
			//find our x and y scale
			float xmax = 0.0f;
			float ymax = 0.0f;
			float zmax = 0.0f;
			Box3 localBounds;
			localBounds.Init();
			for (int k=0; k<gfaces.Count(); k++) 
			{
					// Grap the three points, xformed
				int pcount = 3;
					//				if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
				pcount = gfaces[k]->count;

				Point3 temp_point[4];
				for (int j=0; j<pcount; j++) 
				{
					int index = gfaces[k]->t[j];
					Point3 p = gverts.d[index].p * itm;
					localBounds += p;
				}
			}

			center = localBounds.Center() * tm;

			if (fnGetMapMode() == CYLINDRICALMAP)
			{
				if ((startRing.Count() == 0) && (endRing.Count() == 0))
				{
					centerS = center;
					centerS.z = localBounds.pmin.z;					
				}
				else
				{

					centerS = centerS * itm;
					centerS.z = localBounds.pmin.z;
					centerS = centerS * tm;
				}
			}
			else if ((fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
			{
				centerS = center;
			}

			Point3 bc = localBounds.Center();
			bc.z = localBounds.pmin.z;
			bc = bc * tm;

			xmax = localBounds.pmax.x - localBounds.pmin.x;
			ymax = localBounds.pmax.y - localBounds.pmin.y;
			zmax = localBounds.pmax.z - localBounds.pmin.z;

			Point3 vec;
			vec = Normalize(tm.GetRow(0)) * xmax;
			tm.SetRow(0,vec);

			vec = Normalize(tm.GetRow(1)) * ymax;
			tm.SetRow(1,vec);

			vec = Normalize(tm.GetRow(2)) * zmax;
			tm.SetRow(2,vec);


			
			tm.SetRow(3,centerS);
			

			Matrix3 ptm(1), id(1);
			tm = tm * mctm;
			SetXFormPacket tmpck(tm,ptm);
			tmControl->SetValue(t,&tmpck,TRUE,CTRL_RELATIVE);
			
		}
	}
	ResumeAnimate();

	if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
		ApplyGizmo();

	theHold.Accept(GetString(IDS_MAPPING_ALIGN));
	if (ip) ip->RedrawViews(ip->GetTime());

}


void UnwrapMod::fnGizmoFit()
{
	//get our tm
	//set the tm scale
	if (gfaces.Count() == 0) return;

	theHold.Begin();
	SuspendAnimate();
	AnimateOff();
	TimeValue t = GetCOREInterface()->GetTime();


	ModContextList mcList;
	INodeTab nodeList;
	ip->GetModContexts(mcList,nodeList);

	// MC tm
	Matrix3 mctm(1);
	if (mcList[0]->tm) mctm = *mcList[0]->tm;


	
	//get our selection
	Box3 bounds;
	bounds.Init();
	//get the bounding box
	Point3 pnorm(0.0f,0.0f,0.0f);
	for (int k=0; k<gfaces.Count(); k++) 
	{
		// Grap the three points, xformed
		int pcount = 3;
		//				if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
		pcount = gfaces[k]->count;

		Point3 temp_point[4];
		for (int j=0; j<pcount; j++) 
		{
			int index = gfaces[k]->t[j];
			bounds += gverts.d[index].p;
			if (j < 4)
				temp_point[j] = gverts.d[index].p;
		}
		pnorm += Normalize(temp_point[1]-temp_point[0]^temp_point[2]-temp_point[1]);
	}	

	pnorm = pnorm / (float) gfaces.Count();
	

	//if just a primary axis set the tm;
	Point3 center = bounds.Center();
	// build the scale

	//get our tm
	Matrix3 tm(1);
	tm = *fnGetGizmoTM();






	Point3 vec2;
	vec2 = Normalize(tm.GetRow(0)); 
	tm.SetRow(0,vec2);

	vec2 = Normalize(tm.GetRow(1)) ;
	tm.SetRow(1,vec2);

	vec2 = Normalize(tm.GetRow(2)); 
	tm.SetRow(2,vec2);

	tm.SetRow(3,center);


 	Matrix3 itm = Inverse(tm);
	//find our x and y scale
	float xmax = 0.0f;
	float ymax = 0.0f;
	float zmax = 0.0f;
	Box3 localBounds;
	localBounds.Init();
	for (int k=0; k<gfaces.Count(); k++) 
	{
		// Grap the three points, xformed
		int pcount = 3;
		//				if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
		pcount = gfaces[k]->count;

		Point3 temp_point[4];
		for (int j=0; j<pcount; j++) 
		{
			int index = gfaces[k]->t[j];
			Point3 p = gverts.d[index].p * itm;
			localBounds += p;

//			if (fabs(p.x) > xmax) xmax = fabs(p.x);
//			if (fabs(p.y) > ymax) ymax = fabs(p.y);
//			if (fabs(p.z) > zmax) zmax = fabs(p.z);
		}
	}

	center = localBounds.Center() * tm;



	xmax = localBounds.pmax.x - localBounds.pmin.x;
	ymax = localBounds.pmax.y - localBounds.pmin.y;
	zmax = localBounds.pmax.z - localBounds.pmin.z;
	Point3 vec;
	vec = Normalize(tm.GetRow(0)) * xmax;
	tm.SetRow(0,vec);

	vec = Normalize(tm.GetRow(1)) * ymax;
	tm.SetRow(1,vec);
 
	vec = Normalize(tm.GetRow(2)) * zmax;
	tm.SetRow(2,vec);

	if (fnGetMapMode() == CYLINDRICALMAP)
	{
//		Point3 zvec = tm.GetRow(2);
		center -= (vec * 0.5f);
	}

	tm.SetRow(3,center);


	Matrix3 ptm(1), id(1);
	tm = tm * mctm ;
	SetXFormPacket tmpck(tm,ptm);
	tmControl->SetValue(t,&tmpck,TRUE,CTRL_RELATIVE);

	ResumeAnimate();

	if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == BOXMAP) || (fnGetMapMode() == SPHERICALMAP))
		ApplyGizmo();

	theHold.Accept(GetString(IDS_MAPPING_FIT));
	if (ip) ip->RedrawViews(ip->GetTime());

}



void UnwrapMod::fnGizmoCenter()
{
	//get our tm
	//set the tm scale
	if (gfaces.Count() == 0) return;

	theHold.Begin();
	SuspendAnimate();
	AnimateOff();
	TimeValue t = GetCOREInterface()->GetTime();


	ModContextList mcList;
	INodeTab nodeList;
	ip->GetModContexts(mcList,nodeList);

	// MC tm
	Matrix3 mctm(1);
	if (mcList[0]->tm) mctm = *mcList[0]->tm;

	
	//get our selection
	Box3 bounds;
	bounds.Init();
	//get the bounding box
	Point3 pnorm(0.0f,0.0f,0.0f);
	for (int k=0; k<gfaces.Count(); k++) 
	{
		// Grap the three points, xformed
		int pcount = 3;
		//				if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
		pcount = gfaces[k]->count;

		Point3 temp_point[4];
		for (int j=0; j<pcount; j++) 
		{
			int index = gfaces[k]->t[j];
			bounds += gverts.d[index].p;
			if (j < 4)
				temp_point[j] = gverts.d[index].p;
		}
		pnorm += Normalize(temp_point[1]-temp_point[0]^temp_point[2]-temp_point[1]);
	}	

	pnorm = pnorm / (float) gfaces.Count();
	

	//if just a primary axis set the tm;
	Point3 center = bounds.Center();
	// build the scale



	//get our tm
	Matrix3 tm(1);
	tm = *fnGetGizmoTM();
	Matrix3 initialTM = tm;

	Point3 vec2;
	vec2 = Normalize(tm.GetRow(0)); 
	tm.SetRow(0,vec2);

	vec2 = Normalize(tm.GetRow(1)) ;
	tm.SetRow(1,vec2);

	vec2 = Normalize(tm.GetRow(2)); 
	tm.SetRow(2,vec2);

	tm.SetRow(3,center);


 	Matrix3 itm = Inverse(tm);
	//find our x and y scale
	float xmax = 0.0f;
	float ymax = 0.0f;
	float zmax = 0.0f;
	Box3 localBounds;
	localBounds.Init();
	for (int k=0; k<gfaces.Count(); k++) 
	{
		// Grap the three points, xformed
		int pcount = 3;
		//				if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
		pcount = gfaces[k]->count;

		Point3 temp_point[4];
		for (int j=0; j<pcount; j++) 
		{
			int index = gfaces[k]->t[j];
			Point3 p = gverts.d[index].p * itm;
			localBounds += p;

//			if (fabs(p.x) > xmax) xmax = fabs(p.x);
//			if (fabs(p.y) > ymax) ymax = fabs(p.y);
//			if (fabs(p.z) > zmax) zmax = fabs(p.z);
		}
	}

	if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == PELTMAP))
		center = localBounds.Center() * tm;
	else if (fnGetMapMode() == CYLINDRICALMAP)
	{
		Point3 zvec = initialTM.GetRow(2);
		
		center = center * tm;

		center = localBounds.Center() - (zvec * 0.5f);
		

	}


	initialTM.SetRow(3,center);


	Matrix3 ptm(1), id(1);
	initialTM = initialTM * mctm;
	SetXFormPacket tmpck(initialTM,ptm);
	tmControl->SetValue(t,&tmpck,TRUE,CTRL_RELATIVE);

	ResumeAnimate();

	if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP) || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
		ApplyGizmo();

	theHold.Accept(GetString(IDS_MAPPING_FIT));
	if (ip) ip->RedrawViews(ip->GetTime());

}


void UnwrapMod::fnGizmoAlignToView()
{
	if (gfaces.Count() == 0) return;
	if (ip == NULL) return;

	ViewExp *vpt = ip->GetActiveViewport();
	if (!vpt) return;

	//get our tm
	//set the tm scale
	theHold.Begin();
	SuspendAnimate();
	AnimateOff();
	TimeValue t = GetCOREInterface()->GetTime();




	

	ModContextList mcList;
	INodeTab nodeList;
	ip->GetModContexts(mcList,nodeList);


	// MC tm
	Matrix3 mctm(1);
	if (mcList[0]->tm) mctm = *mcList[0]->tm;

	// Viewport tm
	Matrix3 vtm;
	vpt->GetAffineTM(vtm);
	vtm = Inverse(vtm);

	// Node tm
	Matrix3 ntm = nodeList[0]->GetObjectTM(ip->GetTime());

	Matrix3 destTM = vtm * Inverse(ntm);
	//get our tm
	
	
	Matrix3 initialTM = *fnGetGizmoTM();


	

	Point3 center = Normalize(initialTM.GetRow(3)); 
	initialTM.SetRow(3,center);

	for (int i = 0; i < 3; i++)
	{
		Point3 vec = destTM.GetRow(i);
		float l  = Length(initialTM.GetRow(i));
		vec = Normalize(vec) * l;
		initialTM.SetRow(i,vec);
	}



	initialTM.SetRow(3,center);


	Matrix3 ptm(1), id(1);
	initialTM = initialTM * mctm;
	SetXFormPacket tmpck(initialTM,ptm);
	tmControl->SetValue(t,&tmpck,TRUE,CTRL_RELATIVE);

	ResumeAnimate();

	fnGizmoFit();

	if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == CYLINDRICALMAP)  || (fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == BOXMAP))
		ApplyGizmo();

	
	theHold.Accept(GetString(IDS_MAPPING_ALIGNTOVIEW));
	if (ip) ip->RedrawViews(ip->GetTime());
}



void UnwrapMod::ApplyGizmo()
{

	if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == PELTMAP) ||
		(fnGetMapMode() == SPHERICALMAP) || (fnGetMapMode() == CYLINDRICALMAP))
	{
		ApplyGizmoPrivate();
	}
	else
	{
		//get our normal list
		Tab<Point3> fnorms;
		fnorms.SetCount(gfaces.Count());
		for (int k=0; k< gfaces.Count(); k++) 
			fnorms[k] = Point3(0.0f,0.0f,0.0f);
		
		for (int k=0; k< gfaces.Count(); k++) 
		{
				// Grap the three points, xformed
			int pcount = 3;
				//				if (gfaces[k].flags & FLAG_QUAD) pcount = 4;
			pcount = gfaces[k]->count;

			Point3 temp_point[4];
			for (int j=0; j<pcount; j++) 
			{
				int index = gfaces[k]->t[j];
				
				if (j < 4)
					temp_point[j] = gverts.d[index].p;
			}
		
			fnorms[k] = Normalize(temp_point[1]-temp_point[0]^temp_point[2]-temp_point[1]);
		}
		
		Tab<UVW_TVFaceClass*> front,back,left,right,top,bottom;
		Tab<Point3> norms;

		Matrix3 gtm(1);
		TimeValue t = 0;
		if (ip) t = ip->GetTime();
		if (tmControl)
			tmControl->GetValue(t,&gtm,FOREVER,CTRL_RELATIVE);

		norms.SetCount(6);
		for (int i = 0; i < 3; i++)
		{
			Point3 v = gtm.GetRow(i);
			norms[i*2] = Normalize(v);
			norms[i*2+1] = norms[i*2] * -1.0f;
		}
		
		for (int k=0; k< gfaces.Count(); k++) 
		{
			int closestFace = -1;
			float closestAngle = -10.0f;
			for (int j = 0; j < 6; j++)
			{
				float dot = DotProd(norms[j],fnorms[k]);
				if (dot > closestAngle)
				{
					closestAngle = dot;
					closestFace = j;
				}
			}
			if (closestFace == 0)
				front.Append(1,&gfaces[k],1000);
			else if (closestFace == 1)
				back.Append(1,&gfaces[k],1000);
			else if (closestFace == 2)
				left.Append(1,&gfaces[k],1000);
			else if (closestFace == 3)
				right.Append(1,&gfaces[k],1000);
			else if (closestFace == 4)
				top.Append(1,&gfaces[k],1000);
			else if (closestFace == 5)
				bottom.Append(1,&gfaces[k],1000);


		}

		Tab<UVW_TVFaceClass*> hold = gfaces;

		gtm.IdentityMatrix();
		if (tmControl)
			tmControl->GetValue(t,&gtm,FOREVER,CTRL_RELATIVE);

		Point3 xvec,yvec,zvec;
		xvec = gtm.GetRow(0);
		yvec = gtm.GetRow(1);
		zvec = gtm.GetRow(2);
  		Point3 center = gtm.GetRow(3);


  		theHold.Begin();
		for (int k=0; k < 6; k++) 
		{
			Matrix3 tm(1);
			if (k == 0)
			{
				tm.SetRow(0,yvec);
				tm.SetRow(1,zvec);
				tm.SetRow(2,xvec);
				gfaces = front;
			}
			else if (k == 1)
			{
				tm.SetRow(0,yvec);
				tm.SetRow(1,zvec);
				tm.SetRow(2,(xvec*-1.0f));

				gfaces = back;
			}
			else if (k == 2)
			{
				tm.SetRow(0,xvec);
				tm.SetRow(1,zvec);
				tm.SetRow(2,yvec);

				gfaces = left;
			}
			else if (k == 3)
			{
				tm.SetRow(0,xvec);
				tm.SetRow(1,zvec);
				tm.SetRow(2,(yvec *-1.0f));

				gfaces = right;
			}
			else if (k == 4)
			{
				tm.SetRow(0,xvec);
				tm.SetRow(1,yvec);
				tm.SetRow(2,zvec);

				gfaces = top;
			}
			else if (k == 5)
			{
				tm.SetRow(0,xvec);
				tm.SetRow(1,yvec);
				tm.SetRow(2,(zvec*-1.0f));

				gfaces = bottom;
			}

			

			tm.SetRow(3,center);

			ApplyGizmoPrivate(&tm);
		}
		gfaces = hold;
		theHold.Accept(_T(GetString(IDS_PW_PLANARMAP)));
	}
}
void UnwrapMod::ApplyGizmoPrivate(Matrix3 *defaultTM)
{

	BOOL wasHolding = FALSE;
 	if (theHold.Holding())
		wasHolding = TRUE;


	if (!theHold.Holding())
	{
		theHold.SuperBegin();
		theHold.Begin();
	}
	HoldPointsAndFaces();	


	//add vertices to our internal vertex list filling in dead spots where appropriate
	int ct = 0;  

	//get align normal
	//get fit data


	Matrix3 gtm(1);
	TimeValue t = 0;
	if (ip) t = ip->GetTime();
	if (defaultTM)
		gtm = *defaultTM;
	else
	{
		if (tmControl)
		{
			

			ModContextList mcList;
			INodeTab nodeList;
			ip->GetModContexts(mcList,nodeList);

			// MC tm
			Matrix3 mctm(1);
			if (mcList[0]->tm) mctm = *mcList[0]->tm;
			Matrix3 ntm = nodeList[0]->GetObjectTM(t);


			gtm = CompMatrix(t,mcList[0],NULL);

//			tmControl->GetValue(t,&gtm,FOREVER,CTRL_RELATIVE);
		}

	}
	if (Length(gtm.GetRow(2)) == 0.0f)
	{
		gtm.SetRow(2,Point3(0.0f,0.0f,1.0f));
	}


	float circ = 1.0f;
 	if (!normalizeMap)
	{

		for (int i = 0; i < 3; i++)
		{
			Point3 vec = gtm.GetRow(i);
			vec = Normalize(vec);
			gtm.SetRow(i,vec);
		}

	}

	gtm = Inverse(gtm);

	//nuke any verts previously held in the faces
	//for (int i = 0; i < gfaces.Count(); i++)
	//	{
 	DeleteVertsFromFace(gfaces);
	//	}
	

	//unselect all verts
	for (int j=0;j<TVMaps.v.Count();j++)
	{
		if (vsel[j])
		{
			vsel.Clear(j);
		}
	}

	//build available list
	Tab<int> alist;
	alist.ZeroCount();

	for (int j=0;j<TVMaps.v.Count();j++)
	{
		if (TVMaps.v[j].flags & FLAG_DEAD)
			//dead veretx found copy new vert in and note the place
		{
			alist.Append(1,&j,10000);
		}
	}



 	Tab<int> extraSel;

	if (fnGetMapMode() == SPHERICALMAP)
	{
		Tab<int> quads;
		quads.SetCount(gverts.d.Count());

		float longestR = 0.0f;
		for (int i = 0; i < gverts.d.Count(); i++)
		{
			BOOL found = FALSE;
			quads[i] = -1;
			if (gverts.sel[i])
			{

				Point3 tp = gverts.d[i].p * gtm;
					//z our y

				int quad;
				if ((tp.x >= 0) && (tp.y >= 0))
					quad = 0;
				else if ((tp.x < 0) && (tp.y >= 0))
					quad = 1;
				else if ((tp.x < 0) && (tp.y < 0))
					quad = 2;
				else if ((tp.x >= 0) && (tp.y < 0))
					quad = 3;

				quads[i] = quad;

				//find the quad the point is in
			 	Point3 xvec(1.0f,0.0f,0.0f);
				Point3 zvec(0.0f,0.0f,-1.0f);

				float x= 0.0f;
				Point3 zp = tp;
				zp.z = 0.0f;
				
				float dot = DotProd(xvec,Normalize(zp));
				float angle = 0.0f;
				if (dot == 1.0f)
					angle = 0.0f;
				else angle = acos(dot);

				x = angle/(PI*2.0f);
				if (quad > 1)
					x = (0.5f - x) + 0.5f;

				dot = DotProd(zvec,Normalize(tp));
				float yangle = 0.0f;
				if (dot == 1.0f)
					yangle = 0.0f;
				else yangle = acos(dot);

				float y = yangle/(PI);
								
				tp.x = x;	
				tp.y = y;//tp.z;	
				float d = Length(tp);	
				tp.z = d;
				if (d > longestR)
					longestR = d;				

				if (ct < alist.Count() )
				{
					int j = alist[ct];
					TVMaps.v[j].flags = 0;
					TVMaps.v[j].influence = 0.0f;					


					TVMaps.v[j].p = tp;
					vsel.Set(j);

					if (TVMaps.cont[j]) TVMaps.cont[j]->SetValue(0,&tp,CTRL_ABSOLUTE);
					gverts.d[i].newindex = j;
					ct++;

				}
				else
				{
					UVW_TVVertClass tempv;

					tempv.p = tp;

					tempv.flags = 0;
					tempv.influence = 0.0f;
					gverts.d[i].newindex = TVMaps.v.Count();
					TVMaps.v.Append(1,&tempv,5000);

					int vct = TVMaps.v.Count()-1;
					extraSel.Append(1,&vct,5000);


					Control* c;
					c = NULL;
					TVMaps.cont.Append(1,&c,5000);
					if (TVMaps.cont[TVMaps.v.Count()-1]) 
						TVMaps.cont[TVMaps.v.Count()-1]->SetValue(0,&TVMaps.v[TVMaps.v.Count()-1].p,CTRL_ABSOLUTE);
				}
			}
		}


		//now copy our face data over
		Tab<int> faceAcrossSeam;
		for (int i = 0; i < gfaces.Count(); i++)
		{			
			{
				//		ActiveAddFaces.Append(1,&i,1);
				int ct = gfaces[i]->FaceIndex;
				TVMaps.f[ct]->flags = gfaces[i]->flags;
				TVMaps.f[ct]->flags |= FLAG_SELECTED;
				int pcount = 3;
				//		if (gfaces[i].flags & FLAG_QUAD) pcount = 4;
				pcount = gfaces[i]->count;
				int quad0 = 0;
				int quad3 = 0;
				for (int j = 0; j < pcount; j++)
				{
					int index = gfaces[i]->t[j];
					if (quads[index] == 0) quad0++;
					if (quads[index] == 3) quad3++;
					//find spot in our list
					TVMaps.f[ct]->t[j] = gverts.d[index].newindex;
					if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
					{
						index = gfaces[i]->vecs->handles[j*2];

						if (quads[index] == 0) quad0++;
						if (quads[index] == 3) quad3++;
						//find spot in our list
						TVMaps.f[ct]->vecs->handles[j*2] = gverts.d[index].newindex;

						index = gfaces[i]->vecs->handles[j*2+1];
						if (quads[index] == 0) quad0++;
						if (quads[index] == 3) quad3++;
						//find spot in our list
						TVMaps.f[ct]->vecs->handles[j*2+1] = gverts.d[index].newindex;

						if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
						{
							index = gfaces[i]->vecs->interiors[j];
							if (quads[index] == 0) quad0++;
							if (quads[index] == 3) quad3++;
							//find spot in our list
							TVMaps.f[ct]->vecs->interiors[j] = gverts.d[index].newindex;
						}

					}
				}

				if ((quad3 > 0) && (quad0 > 0))
				{
 					for (int j = 0; j < pcount; j++)
					{
						//find spot in our list
						int index = TVMaps.f[ct]->t[j];
						if (TVMaps.v[index].p.x <= 0.25f)
						{
							UVW_TVVertClass tempv = TVMaps.v[index];
							tempv.p.x += 1.0f;
							tempv.flags = 0;
							tempv.influence = 0.0f;
							TVMaps.v.Append(1,&tempv,5000);
							int vct = TVMaps.v.Count()-1;
							TVMaps.f[ct]->t[j] = vct;
							extraSel.Append(1,&vct,5000);
							Control* c;
							c = NULL;
							TVMaps.cont.Append(1,&c,5000);
							if (TVMaps.cont[TVMaps.v.Count()-1]) 
								TVMaps.cont[TVMaps.v.Count()-1]->SetValue(0,&TVMaps.v[TVMaps.v.Count()-1].p,CTRL_ABSOLUTE);
						}

						if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
						{
							

							//find spot in our list
							int index = TVMaps.f[ct]->vecs->handles[j*2];

							if (TVMaps.v[index].p.x <= 0.25f)
							{
								UVW_TVVertClass tempv = TVMaps.v[index];
								tempv.p.x += 1.0f;
								tempv.flags = 0;
								tempv.influence = 0.0f;
								TVMaps.v.Append(1,&tempv,5000);
								int vct = TVMaps.v.Count()-1;
								TVMaps.f[ct]->vecs->handles[j*2] = vct;
								extraSel.Append(1,&vct,5000);
								Control* c;
								c = NULL;
								TVMaps.cont.Append(1,&c,5000);
								if (TVMaps.cont[TVMaps.v.Count()-1]) 
									TVMaps.cont[TVMaps.v.Count()-1]->SetValue(0,&TVMaps.v[TVMaps.v.Count()-1].p,CTRL_ABSOLUTE);
							}
							//find spot in our list
							index = TVMaps.f[ct]->vecs->handles[j*2+1];
							if (TVMaps.v[index].p.x <= 0.25f)
							{
								UVW_TVVertClass tempv = TVMaps.v[index];
								tempv.p.x += 1.0f;
								tempv.flags = 0;
								tempv.influence = 0.0f;
								TVMaps.v.Append(1,&tempv,5000);
								int vct = TVMaps.v.Count()-1;
								TVMaps.f[ct]->vecs->handles[j*2+1] = vct;
								extraSel.Append(1,&vct,5000);
								Control* c;
								c = NULL;
								TVMaps.cont.Append(1,&c,5000);
								if (TVMaps.cont[TVMaps.v.Count()-1]) 
									TVMaps.cont[TVMaps.v.Count()-1]->SetValue(0,&TVMaps.v[TVMaps.v.Count()-1].p,CTRL_ABSOLUTE);
							}
							if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
							{
								index = TVMaps.f[ct]->vecs->interiors[j];
								if (TVMaps.v[index].p.x <= 0.25f)
								{
									UVW_TVVertClass tempv = TVMaps.v[index];
									tempv.p.x += 1.0f;
									tempv.flags = 0;
									tempv.influence = 0.0f;
									TVMaps.v.Append(1,&tempv,5000);
									int vct = TVMaps.v.Count()-1;
									TVMaps.f[ct]->vecs->interiors[j] = vct;
									extraSel.Append(1,&vct,5000);
									Control* c;
									c = NULL;
									TVMaps.cont.Append(1,&c,5000);
									if (TVMaps.cont[TVMaps.v.Count()-1]) 
										TVMaps.cont[TVMaps.v.Count()-1]->SetValue(0,&TVMaps.v[TVMaps.v.Count()-1].p,CTRL_ABSOLUTE);
								}
							}

						}
					}
				}

			}
		}

		if (!normalizeMap)
		{
			BitArray processedVerts;
			processedVerts.SetSize(TVMaps.v.Count());
			processedVerts.ClearAll();
			circ = circ * PI * longestR * 2.0f;
			for (int i = 0; i < gfaces.Count(); i++)
			{	
				int pcount = 3;
				//		if (gfaces[i].flags & FLAG_QUAD) pcount = 4;
				pcount = gfaces[i]->count;
 				int ct = gfaces[i]->FaceIndex;
					
					for (int j = 0; j < pcount; j++)
					{
						//find spot in our list
						int index = TVMaps.f[ct]->t[j];
						if (!processedVerts[index])
						{
							TVMaps.v[index].p.x *= circ;
							TVMaps.v[index].p.y *= circ;
							processedVerts.Set(index,TRUE);
						}

						if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
						{
							

							//find spot in our list
							int index = TVMaps.f[ct]->vecs->handles[j*2];
							if (!processedVerts[index])
							{
								TVMaps.v[index].p.x *= circ;
								TVMaps.v[index].p.y *= circ;
								processedVerts.Set(index,TRUE);
							}
							//find spot in our list
							index = TVMaps.f[ct]->vecs->handles[j*2+1];
							if (!processedVerts[index])
							{
								TVMaps.v[index].p.x *= circ;
								TVMaps.v[index].p.y *= circ;
								processedVerts.Set(index,TRUE);
							}
							if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
							{
								index = TVMaps.f[ct]->vecs->interiors[j];
								if (!processedVerts[index])
								{
									TVMaps.v[index].p.x *= circ;
									TVMaps.v[index].p.y *= circ;
									processedVerts.Set(index,TRUE);
								}
							}

						}
					
				}
			}
		}

		
		//now find the seam
		vsel.SetSize(TVMaps.v.Count(), 1);
		vsel.Set(TVMaps.v.Count()-1);
		for (int i = 0; i < extraSel.Count(); i++)
			vsel.Set(extraSel[i],TRUE);

	}
	else if (fnGetMapMode() == CYLINDRICALMAP)
	{
		Tab<int> quads;
		quads.SetCount(gverts.d.Count());

		float longestR = 0.0f;
		for (int i = 0; i < gverts.d.Count(); i++)
		{
			BOOL found = FALSE;
			quads[i] = -1;
			if (gverts.sel[i])
			{

				Point3 gp = gverts.d[i].p;
				Point3 tp = gp * gtm;
					//z our y

				int quad;
				if ((tp.x >= 0) && (tp.y >= 0))
					quad = 0;
				else if ((tp.x < 0) && (tp.y >= 0))
					quad = 1;
				else if ((tp.x < 0) && (tp.y < 0))
					quad = 2;
				else if ((tp.x >= 0) && (tp.y < 0))
					quad = 3;

				quads[i] = quad;

				//find the quad the point is in
			 	Point3 xvec(1.0f,0.0f,0.0f);
				
				float x= 0.0f;
				Point3 zp = tp;
				zp.z = 0.0f;
				
				float dot = DotProd(xvec,Normalize(zp));
				float angle = 0.0f;
				if (dot == 1.0f)
					angle = 0.0f;
				else angle = acos(dot);

				x = angle/(PI*2.0f);
				if (quad > 1)
					x = (0.5f - x) + 0.5f;
								
				

				if (ct < alist.Count() )
				{
					int j = alist[ct];
					TVMaps.v[j].flags = 0;
					TVMaps.v[j].influence = 0.0f;



					
					TVMaps.v[j].p.x = x;	
					TVMaps.v[j].p.y = tp.z;	
					float d = Length(zp);	
					TVMaps.v[j].p.z = d;
					if (d > longestR)
						longestR = d;

					vsel.Set(j);


					if (TVMaps.cont[j]) TVMaps.cont[j]->SetValue(0,&TVMaps.v[j].p,CTRL_ABSOLUTE);
					gverts.d[i].newindex = j;
					ct++;

				}
				else
				{
					UVW_TVVertClass tempv;

					tp.x = x;	
					tp.y = tp.z;	
					float d = Length(zp);	
					tp.z = d;
					if (d > longestR)
						longestR = d;

					tempv.p = tp;

					tempv.flags = 0;
					tempv.influence = 0.0f;
					gverts.d[i].newindex = TVMaps.v.Count();
					TVMaps.v.Append(1,&tempv,5000);

					int vct = TVMaps.v.Count()-1;
					extraSel.Append(1,&vct,5000);


					Control* c;
					c = NULL;
					TVMaps.cont.Append(1,&c,5000);
					if (TVMaps.cont[TVMaps.v.Count()-1]) 
						TVMaps.cont[TVMaps.v.Count()-1]->SetValue(0,&TVMaps.v[TVMaps.v.Count()-1].p,CTRL_ABSOLUTE);
				}
			}
		}


		//now copy our face data over
		Tab<int> faceAcrossSeam;
		for (int i = 0; i < gfaces.Count(); i++)
		{			
			{
				//		ActiveAddFaces.Append(1,&i,1);
				int ct = gfaces[i]->FaceIndex;
				TVMaps.f[ct]->flags = gfaces[i]->flags;
				TVMaps.f[ct]->flags |= FLAG_SELECTED;
				int pcount = 3;
				//		if (gfaces[i].flags & FLAG_QUAD) pcount = 4;
				pcount = gfaces[i]->count;
				int quad0 = 0;
				int quad3 = 0;
				for (int j = 0; j < pcount; j++)
				{
					int index = gfaces[i]->t[j];
					if (quads[index] == 0) quad0++;
					if (quads[index] == 3) quad3++;
					//find spot in our list
					TVMaps.f[ct]->t[j] = gverts.d[index].newindex;
					if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
					{
						index = gfaces[i]->vecs->handles[j*2];

						if (quads[index] == 0) quad0++;
						if (quads[index] == 3) quad3++;
						//find spot in our list
						TVMaps.f[ct]->vecs->handles[j*2] = gverts.d[index].newindex;

						index = gfaces[i]->vecs->handles[j*2+1];
						if (quads[index] == 0) quad0++;
						if (quads[index] == 3) quad3++;
						//find spot in our list
						TVMaps.f[ct]->vecs->handles[j*2+1] = gverts.d[index].newindex;

						if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
						{
							index = gfaces[i]->vecs->interiors[j];
							if (quads[index] == 0) quad0++;
							if (quads[index] == 3) quad3++;
							//find spot in our list
							TVMaps.f[ct]->vecs->interiors[j] = gverts.d[index].newindex;
						}

					}
				}

				if ((quad3 > 0) && (quad0 > 0))
				{
 					for (int j = 0; j < pcount; j++)
					{
						//find spot in our list
						int index = TVMaps.f[ct]->t[j];
						if (TVMaps.v[index].p.x <= 0.25f)
						{
							UVW_TVVertClass tempv = TVMaps.v[index];
							tempv.p.x += 1.0f;
							tempv.flags = 0;
							tempv.influence = 0.0f;
							TVMaps.v.Append(1,&tempv,5000);
							int vct = TVMaps.v.Count()-1;
							TVMaps.f[ct]->t[j] = vct;
							extraSel.Append(1,&vct,5000);
							Control* c;
							c = NULL;
							TVMaps.cont.Append(1,&c,5000);
							if (TVMaps.cont[TVMaps.v.Count()-1]) 
								TVMaps.cont[TVMaps.v.Count()-1]->SetValue(0,&TVMaps.v[TVMaps.v.Count()-1].p,CTRL_ABSOLUTE);
						}

						if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
						{
							

							//find spot in our list
							int index = TVMaps.f[ct]->vecs->handles[j*2];

							if (TVMaps.v[index].p.x <= 0.25f)
							{
								UVW_TVVertClass tempv = TVMaps.v[index];
								tempv.p.x += 1.0f;
								tempv.flags = 0;
								tempv.influence = 0.0f;
								TVMaps.v.Append(1,&tempv,5000);
								int vct = TVMaps.v.Count()-1;
								TVMaps.f[ct]->vecs->handles[j*2] = vct;
								extraSel.Append(1,&vct,5000);
								Control* c;
								c = NULL;
								TVMaps.cont.Append(1,&c,5000);
								if (TVMaps.cont[TVMaps.v.Count()-1]) 
									TVMaps.cont[TVMaps.v.Count()-1]->SetValue(0,&TVMaps.v[TVMaps.v.Count()-1].p,CTRL_ABSOLUTE);
							}
							//find spot in our list
							index = TVMaps.f[ct]->vecs->handles[j*2+1];
							if (TVMaps.v[index].p.x <= 0.25f)
							{
								UVW_TVVertClass tempv = TVMaps.v[index];
								tempv.p.x += 1.0f;
								tempv.flags = 0;
								tempv.influence = 0.0f;
								TVMaps.v.Append(1,&tempv,5000);
								int vct = TVMaps.v.Count()-1;
								TVMaps.f[ct]->vecs->handles[j*2+1] = vct;
								extraSel.Append(1,&vct,5000);
								Control* c;
								c = NULL;
								TVMaps.cont.Append(1,&c,5000);
								if (TVMaps.cont[TVMaps.v.Count()-1]) 
									TVMaps.cont[TVMaps.v.Count()-1]->SetValue(0,&TVMaps.v[TVMaps.v.Count()-1].p,CTRL_ABSOLUTE);
							}
							if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
							{
								index = TVMaps.f[ct]->vecs->interiors[j];
								if (TVMaps.v[index].p.x <= 0.25f)
								{
									UVW_TVVertClass tempv = TVMaps.v[index];
									tempv.p.x += 1.0f;
									tempv.flags = 0;
									tempv.influence = 0.0f;
									TVMaps.v.Append(1,&tempv,5000);
									int vct = TVMaps.v.Count()-1;
									TVMaps.f[ct]->vecs->interiors[j] = vct;
									extraSel.Append(1,&vct,5000);
									Control* c;
									c = NULL;
									TVMaps.cont.Append(1,&c,5000);
									if (TVMaps.cont[TVMaps.v.Count()-1]) 
										TVMaps.cont[TVMaps.v.Count()-1]->SetValue(0,&TVMaps.v[TVMaps.v.Count()-1].p,CTRL_ABSOLUTE);
								}
							}

						}
					}
				}

			}
		}

		if (!normalizeMap)
		{
			BitArray processedVerts;
			processedVerts.SetSize(TVMaps.v.Count());
			processedVerts.ClearAll();
			circ = circ * PI * longestR * 2.0f;
			for (int i = 0; i < gfaces.Count(); i++)
			{	
				int pcount = 3;
				//		if (gfaces[i].flags & FLAG_QUAD) pcount = 4;
				pcount = gfaces[i]->count;
 				int ct = gfaces[i]->FaceIndex;
					
					for (int j = 0; j < pcount; j++)
					{
						//find spot in our list
						int index = TVMaps.f[ct]->t[j];
						if (!processedVerts[index])
						{
							TVMaps.v[index].p.x *= circ;
							processedVerts.Set(index,TRUE);
						}

						if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
						{
							

							//find spot in our list
							int index = TVMaps.f[ct]->vecs->handles[j*2];
							if (!processedVerts[index])
							{
								TVMaps.v[index].p.x *= circ;
								processedVerts.Set(index,TRUE);
							}
							//find spot in our list
							index = TVMaps.f[ct]->vecs->handles[j*2+1];
							if (!processedVerts[index])
							{
								TVMaps.v[index].p.x *= circ;
								processedVerts.Set(index,TRUE);
							}
							if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
							{
								index = TVMaps.f[ct]->vecs->interiors[j];
								if (!processedVerts[index])
								{
									TVMaps.v[index].p.x *= circ;
									processedVerts.Set(index,TRUE);
								}
							}

						}
					
				}
			}
		}

		
		//now find the seam
		vsel.SetSize(TVMaps.v.Count(), 1);
		vsel.Set(TVMaps.v.Count()-1);
		for (int i = 0; i < extraSel.Count(); i++)
			vsel.Set(extraSel[i],TRUE);

	}
	else if ((fnGetMapMode() == PLANARMAP) || (fnGetMapMode() == PELTMAP)|| (fnGetMapMode() == BOXMAP))
	{
 		for (int i = 0; i < gverts.d.Count(); i++)
		{
			BOOL found = FALSE;
			if (gverts.sel[i])
			{
				if (ct < alist.Count() )
				{
					int j = alist[ct];
					TVMaps.v[j].flags = 0;
					TVMaps.v[j].influence = 0.0f;


					Point3 tp = gverts.d[i].p * gtm;
					tp.x += 0.5f;
					tp.y += 0.5f;
					TVMaps.v[j].p = tp;
	//				int vcount = vsel.GetSize();
					vsel.Set(j);

					if (TVMaps.cont[j]) TVMaps.cont[j]->SetValue(0,&tp,CTRL_ABSOLUTE);
					gverts.d[i].newindex = j;
					ct++;

				}
				else
				{
					UVW_TVVertClass tempv;

					Point3 tp = gverts.d[i].p * gtm;
					tp.x += 0.5f;
					tp.y += 0.5f;

					tempv.p = tp;

					tempv.flags = 0;
					tempv.influence = 0.0f;
					gverts.d[i].newindex = TVMaps.v.Count();
					TVMaps.v.Append(1,&tempv,5000);

					int vct = TVMaps.v.Count()-1;
					extraSel.Append(1,&vct,5000);


					Control* c;
					c = NULL;
					TVMaps.cont.Append(1,&c,5000);
					if (TVMaps.cont[TVMaps.v.Count()-1]) 
						TVMaps.cont[TVMaps.v.Count()-1]->SetValue(0,&TVMaps.v[TVMaps.v.Count()-1].p,CTRL_ABSOLUTE);
				}
			}
		}

		vsel.SetSize(TVMaps.v.Count(), 1);
		vsel.Set(TVMaps.v.Count()-1);
		for (int i = 0; i < extraSel.Count(); i++)
			vsel.Set(extraSel[i],TRUE);

		//now copy our face data over
		for (int i = 0; i < gfaces.Count(); i++)
		{
			//	if (fs[i])
			{
				//		ActiveAddFaces.Append(1,&i,1);
				int ct = gfaces[i]->FaceIndex;
				TVMaps.f[ct]->flags = gfaces[i]->flags;
				TVMaps.f[ct]->flags |= FLAG_SELECTED;
				int pcount = 3;
				//		if (gfaces[i].flags & FLAG_QUAD) pcount = 4;
				pcount = gfaces[i]->count;
				for (int j = 0; j < pcount; j++)
				{
					int index = gfaces[i]->t[j];
					//find spot in our list
					TVMaps.f[ct]->t[j] = gverts.d[index].newindex;
					if ((TVMaps.f[ct]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[ct]->vecs))
					{
						index = gfaces[i]->vecs->handles[j*2];
						//find spot in our list
						TVMaps.f[ct]->vecs->handles[j*2] = gverts.d[index].newindex;

						index = gfaces[i]->vecs->handles[j*2+1];
						//find spot in our list
						TVMaps.f[ct]->vecs->handles[j*2+1] = gverts.d[index].newindex;

						if (TVMaps.f[ct]->flags & FLAG_INTERIOR)
						{
							index = gfaces[i]->vecs->interiors[j];
							//find spot in our list
							TVMaps.f[ct]->vecs->interiors[j] = gverts.d[index].newindex;
						}

					}
				}
			}
		}
	}

	
/*
	//make a named selection for new face selection
	TSTR namePre(GetString(IDS_PW_PLANARMAP));
	TSTR name;
	int ctn = 0;
	while (1) {				
		char num[10];
		wsprintf(num,"%d",ctn);
		name = namePre+num;
		Tab<TSTR*> &setList = namedSel;

		BOOL unique = TRUE;
		for (int i=0; i<setList.Count(); i++) {
			if (name==*setList[i]) {
				unique = FALSE;
				break;
			}
		}
		ctn++;
		if (unique) break;

	}

	NewSetFromCurSel(name); 
	SetupNamedSelDropDown();
*/

	if (!wasHolding)
	{
		theHold.Accept(_T(GetString(IDS_PW_PLANARMAP)));
		theHold.SuperAccept(_T(GetString(IDS_PW_PLANARMAP)));
	}

	RebuildEdges();
	theHold.Suspend();
	fnFaceToEdgeSelect();
	theHold.Resume();
//	ConvertFaceToEdgeSel();
//	TVMaps.edgesValid= FALSE;
	//update our views to show new faces
	InvalidateView();
}
