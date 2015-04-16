#include "unwrap.h"



BOOL Solver::Solve(int startFrame, int endFrame, int samples,
				   Tab<EdgeBondage> &springs, Tab<SpringClass> &vertexData,
				   float stiffness, float dampening, float decay,
				   UnwrapMod *mod)
{
	int tps = GetTicksPerFrame();
	int sampleInc = tps/samples;

	BitArray lockedVerts;
	lockedVerts.SetSize(vertexData.Count());
	lockedVerts.ClearAll();
	Box3 boundsVert;
	boundsVert.Init();


	for (int i = 0; i < springs.Count(); i++)
	{
		if (springs[i].isEdge)
		{
			int a= springs[i].v2;
			if ((a < 0) || (a>=lockedVerts.GetSize()))
			{
				DebugPrint("error\n");
			}
			else
			{
				boundsVert += vertexData[a].pos;
				lockedVerts.Set(a);
			}
		}		
	}

	Point3 boxVec = boundsVert.pmax-boundsVert.pmin;
	maxVelocity = Length(boxVec)*0.05f; 
	

	for (int i = 0; i < vertexData.Count(); i++)
	{
		if (vertexData[i].weight == 0.0f)
			lockedVerts.Set(i);
	}

	vertsToProcess.ZeroCount();
	for (int i = 0; i < vertexData.Count(); i++)
	{
		if (!lockedVerts[i])
			vertsToProcess.Append(1,&i,10000);
	}

	holdHoldVertexData = vertexData;

	HWND hWnd = GetDlgItem(mod->peltData.peltDialog.hWnd, IDC_PELTLABEL);

	int frames = endFrame-startFrame;
	BOOL biret = TRUE;
	for (int i = startFrame; i < (frames*tps); i+=tps)
	{
		SHORT iret = GetAsyncKeyState (VK_ESCAPE);
		if (iret==-32767)
		{
			i = (frames*tps);
			biret = FALSE;
		}

		TimeValue sampleTime = i;

		TSTR progress;
		progress.printf("%s %d",GetString(IDS_ITERATION),(int)i/tps);
		SetWindowText(hWnd,progress);

		for (int k =0; k < samples; k++)
		{
			sampleTime += sampleInc;
			Evaluate(springs, vertexData,
				i,  
				samples,
				stiffness, dampening);
			//			Evaluate(lmd, sampleTime,t, nv, os,samples,1.0f);
			for (int k=0;k<vertsToProcess.Count();k++)
			{
				int index = vertsToProcess[k];
				vertexData[index].vel *= decay;
			}

		}

		if (mod && ((i%4) == 0))
		{
			TimeValue t = GetCOREInterface()->GetTime();

			UVW_ChannelClass *tvData = &mod->TVMaps;
			for (int j = 0; j < vertsToProcess.Count(); j++)
			{
				int index = vertsToProcess[j];
				Point3 p = vertexData[index].pos;

				tvData->v[index].p = p;
				if (tvData->cont[index]) tvData->cont[index]->SetValue(t,&tvData->v[index].p);

			}
			mod->peltData.ResolvePatchHandles(mod);
			mod->NotifyDependents(FOREVER,TEXMAP_CHANNEL,REFMSG_CHANGE);
			mod->InvalidateView();
			UpdateWindow(mod->hWnd);
			if (mod->ip) mod->ip->RedrawViews(t);
		}
	}


	TSTR progress;
	progress.printf("    ");
	SetWindowText(hWnd,progress);
	return biret;

}


void Solver::Evaluate(Tab<EdgeBondage> &springs, Tab<SpringClass> &vertexData,
					  TimeValue i, 
					  int samples,
					  float &stiffness, float &dampening)
{

	for (int j = 0; j < vertsToProcess.Count(); j++)
	{
		int index = vertsToProcess[j];
		holdHoldVertexData[index] = vertexData[index];
	}

	SolveFrame(0, springs, vertexData,
		i, 
		samples,
		stiffness, dampening);
	//	Solve(0, lmd, i, t,  nv, os,samples);

	int solver = 4;
	int tps = GetTicksPerFrame();
	float time = 1.0f;//( 1.0f/(float)samples); //put back with a multiplier in 4+


	int nv = vertexData.Count();

	TimeValue tempT;
	if (solver >= 1)
	{
		for (int id=0;id<vertsToProcess.Count();id++)
			{
			int j =  vertsToProcess[id];
				vertexData[j].tempPos[0] =  vertexData[j].pos;
				vertexData[j].pos = vertexData[j].pos + vertexData[j].tempVel[0] * (0.5f*time);
			}
		tempT = i + tps/2;
		SolveFrame(1, springs, vertexData,
			i, 
			samples,
			stiffness, dampening);
		//		Solve(1, lmd, tempT, t,  nv, os,samples);
	}

	if (solver > 1)
	{
		for (int id=0;id<vertsToProcess.Count();id++)
		{
			int j =  vertsToProcess[id];			
				vertexData[j].pos = vertexData[j].tempPos[0] + vertexData[j].tempVel[1] * 0.5f*time;
		}
		SolveFrame(2, springs, vertexData,
			i, 
			samples,
			stiffness, dampening);
		//		Solve(2, lmd, tempT, t,  nv, os,samples);

		for (int id=0;id<vertsToProcess.Count();id++)
		{
			int j =  vertsToProcess[id];
				vertexData[j].pos = vertexData[j].tempPos[0] + vertexData[j].tempVel[2] *0.5f*time;
		}
		SolveFrame(3, springs, vertexData,
			i, 
			samples,
			stiffness, dampening);
		//		Solve(3, lmd, tempT, t,  nv, os,samples);


		for (int id=0;id<vertsToProcess.Count();id++)
		{
			int j =  vertsToProcess[id];
				vertexData[j].pos = vertexData[j].tempPos[0] + vertexData[j].tempVel[3]*time;
		}
		tempT += tps/2;
		SolveFrame(4, springs, vertexData,
			i, 
			samples,
			stiffness, dampening);
		//		Solve(4, lmd, tempT, t,  nv, os,samples);
	}



	float largestVelocityChange = 0.0f;
	for (int id=0;id<vertsToProcess.Count();id++)
	{
		int j =  vertsToProcess[id];


			vertexData[j].vel += vertexData[j].tempVel[0]/6.0f + 
				vertexData[j].tempVel[1]/3.0f + 
				vertexData[j].tempVel[2]/3.0f + 
				vertexData[j].tempVel[3]/6.0f ;

			float w = vertexData[j].weight;
			vertexData[j].vel *= w;

			//DebugPrint("%d Vel %f\n",j,vertexData[j].vel);

			vertexData[j].pos = vertexData[j].tempPos[0];

			Point3 v = vertexData[j].vel*time;
			float velMag = Length(vertexData[j].vel);

			if (velMag > largestVelocityChange)
				largestVelocityChange = velMag;
			//DebugPrint("%d  %f %f %f\n",j,v.x,v.y,v.z);
			vertexData[j].pos += v;
		
	}


	BOOL error = FALSE;

	BOOL backStep = FALSE;
	if (largestVelocityChange > (maxVelocity*0.1f))
		error = TRUE;

	if (largestVelocityChange > (maxVelocity))
		backStep = TRUE;

 	if (error)
	{
		stiffness *= 0.9f;
		dampening *= 0.9f;		
		if (backStep)
		{
			for (int i = 0; i < vertsToProcess.Count(); i++)
			{
				int index = vertsToProcess[i];
				vertexData[index] = holdHoldVertexData[index];
			}
		}
	}

 }

 void Solver::SolveFrame(int level, 
	 Tab<EdgeBondage> &springs, Tab<SpringClass> &vertexData,
	 TimeValue i, 
	 int samples,
	 float strength, float dampening)

 {

	 int nv = vertexData.Count();
	 int tnv = nv;

	 float time = 1.0f;//( 1.0f/(float)samples);  put back with a multiplier in 4+
	 float timeSquared = time*time;
	 //do springs

	for (int id=0;id<vertsToProcess.Count();id++)
	 {
		int j =  vertsToProcess[id];
		 vertexData[j].tempVel[level].x = 0.0f;
		 vertexData[j].tempVel[level].y = 0.0f;
		 vertexData[j].tempVel[level].z = 0.0f;
	 }

	 for (int j=0; j<springs.Count();j++)
	 {
		 int a = springs[j].v1;	
		 int b = springs[j].v2;	

		 if ((a>=0) && (a<nv) && (b>=0) && (b<nv))
		 {
			 Point3 p1,p2;
			 Point3 v1(0.0f,0.0f,0.0f),v2(0.0f,0.0f,0.0f);
			 BOOL lockedEnd=FALSE;


			 p1 = vertexData[a].pos;
			 v1 = vertexData[a].vel;

			 p2 = vertexData[b].pos;
			 v2 = vertexData[b].vel;

			 float wa = vertexData[a].weight;
			 float wb = vertexData[b].weight;
			 if ( (wa == 0.0f) && (wb == 0.0f))
			 {
				 //				DebugPrint("locked spring\n");
			 }
			 else
			 {


				 //if (flag !=0) DebugPrint("got here\n");

				 Point3 l = p1-p2;
				 float len = 0.0f;
				 float restLen = 0.0f;
				 float str = strength;// * springs[j].extraStiffness;
				 float damp = dampening;// * springs[j].extraStiffness;

				 if (springs[j].cornerIndex == -1)
				 {
					 len = Length(p2-p1);
					 restLen = springs[j].dist;// * springs[j].distPer;
				 }
				 else
				 {
					 Point3 cp = vertexData[springs[j].cornerIndex].pos;

					 p1.z = 0.0f;
					 p2.z = 0.0f;
					 cp.z = 0.0f;

					 Point3 vecA, vecB;
					 vecA = Normalize(p1 - cp);
					 vecB = Normalize(p2 - cp);

					 float dot = DotProd(vecA,vecB);
					 float angle = 0.0f;
					 if (dot == -1.0)
						 angle = 0.0f;
					 else if (dot == 0.0)
						 angle = PI*0.5f;
					 else if (dot == 1.0)
						 angle = PI;
					 else
					 {
						 angle = acos(dot);
					 }

					 len = angle;
					 restLen = springs[j].dist;// * springs[j].distPer;
					 str *= 0.01f;
					 damp *= 0.01f;

				 }


				 Point3 dvel = v1-v2;
				 if (len < 0.0001f)
				 {
					 //				DebugPrint("Illegal Spring Length %d\n",j);
				 }
				 else 
				 {
					 if (restLen != len)
					 {
						 Point3 v;

						 if (springs[j].cornerIndex == -1)
							 v = ((str)*(len-restLen)) * l/len;
						 else v = ((str)*(len-restLen)+(damp)*((DotProd(dvel,l))/len)) * l/len;
						 if (springs[j].isEdge) v *= 2.0f;
						 v *= time;
						 v *= springs[j].str;


						 vertexData[a].tempVel[level] -= v;
						 vertexData[b].tempVel[level] += v;

						 //						vertexData[a].tempVel[level].z += 0.000001f;
						 //						vertexData[b].tempVel[level].z += 0.000001f;

					 }
				 }
			 }
		 }
	 }		

 }




