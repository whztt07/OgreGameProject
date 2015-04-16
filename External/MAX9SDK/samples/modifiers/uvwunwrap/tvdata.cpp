#include "unwrap.h"

int UVW_TVFaceClass::GetGeoVertIDFromUVVertID(int uvID)
{
	for (int i = 0; i < count; i++)
	{
		int a = t[i];
		if (a == uvID)
			return v[i];
	}
	return -1;

}

void UVW_TVFaceClass::GetConnectedUVVerts(int id, int &v1, int &v2)
{
	for (int i = 0; i < count; i++)
	{
		int a = t[i];
		if (a == id)
		{
			int next = (i + 1);
			int prev = (i - 1);

			if (next >= count) next = 0;
			if (prev < 0) prev = count-1;
			v1 = t[prev];
			v2 = t[next];
			i = count;
		}
	}
}

void UVW_TVFaceClass::GetConnectedGeoVerts(int id, int &v1, int &v2)
{
	for (int i = 0; i < count; i++)
	{
		int a = v[i];
		if (a == id)
		{
			int next = (i + 1);
			int prev = (i - 1);

			if (next >= count) next = 0;
			if (prev < 0) prev = count-1;
			v1 = v[prev];
			v2 = v[next];
			i = count;
		}
	}
}


UVW_TVFaceClass* UVW_TVFaceClass::Clone()
	{
	UVW_TVFaceClass *f = new UVW_TVFaceClass;
	f->FaceIndex = FaceIndex;
	f->MatID = MatID;
	f->flags = flags;
	f->count = count;
	f->t = new int[count];
	f->v = new int[count];
	for (int i =0; i < count; i++)
		{
		f->t[i] = t[i];
		f->v[i] = v[i];
		}
	f->vecs = NULL;
	if (vecs)
		{
		UVW_TVVectorClass *vect = new UVW_TVVectorClass;
		f->vecs = vect;

		for (int i =0; i < count; i++)
			{
			vect->interiors[i] =  vecs->interiors[i];
			vect->handles[i*2] =  vecs->handles[i*2];
			vect->handles[i*2+1] =  vecs->handles[i*2+1];

			vect->vinteriors[i] =  vecs->vinteriors[i];
			vect->vhandles[i*2] =  vecs->vhandles[i*2];
			vect->vhandles[i*2+1] =  vecs->vhandles[i*2+1];
			}
		}
	return f;
	}

void UVW_TVFaceClass::DeleteVec()
	{
	if (vecs) delete vecs;
	vecs = NULL;
	}

ULONG UVW_TVFaceClass::SaveFace(ISave *isave)
	{
	ULONG nb = 0;
	isave->Write(&count, sizeof(int), &nb);
	isave->Write(t, sizeof(int)*count, &nb);
	isave->Write(&FaceIndex, sizeof(int), &nb);
	isave->Write(&MatID, sizeof(int), &nb);
	isave->Write(&flags, sizeof(int), &nb);
	isave->Write(v, sizeof(int)*count, &nb);
	BOOL useVecs = TRUE;
	if ( (vecs) && (flags & FLAG_CURVEDMAPPING))
		{
		isave->Write(vecs->handles, sizeof(int)*count*2, &nb);
		isave->Write(vecs->interiors, sizeof(int)*count, &nb);
		isave->Write(vecs->vhandles, sizeof(int)*count*2, &nb);
		isave->Write(vecs->vinteriors, sizeof(int)*count, &nb);

		}
	return nb;
	}

ULONG UVW_TVFaceClass::SaveFace(FILE *file)
	{
	ULONG nb = 1;
	fwrite(&count, sizeof(int), 1,file);
	fwrite(t, sizeof(int)*count, 1,file);
	fwrite(&FaceIndex, sizeof(int), 1,file);
	fwrite(&MatID, sizeof(int), 1,file);
	fwrite(&flags, sizeof(int), 1,file);
	fwrite(v, sizeof(int)*count, 1,file);
	BOOL useVecs = TRUE;
	if ( (vecs) && (flags & FLAG_CURVEDMAPPING))
		{
		fwrite(vecs->handles, sizeof(int)*count*2, 1,file);
		fwrite(vecs->interiors, sizeof(int)*count, 1,file);
		fwrite(vecs->vhandles, sizeof(int)*count*2, 1,file);
		fwrite(vecs->vinteriors, sizeof(int)*count, 1,file);
		}
	return nb;
	}


ULONG UVW_TVFaceClass::LoadFace(ILoad *iload)
	{
	ULONG nb = 0;
	iload->Read(&count, sizeof(int), &nb);

	if (t==NULL)
		t = new int[count];
	else
		{
		delete [] t;
		t = new int[count];
		}

	if (v==NULL)
		v = new int[count];
	else
		{
		delete [] v;
		v = new int[count];
		}


	iload->Read(t, sizeof(int)*count, &nb);
	iload->Read(&FaceIndex, sizeof(int), &nb);
	iload->Read(&MatID, sizeof(int), &nb);
	iload->Read(&flags, sizeof(int), &nb);
	iload->Read(v, sizeof(int)*count, &nb);
	vecs = NULL;

	if (flags & FLAG_CURVEDMAPPING)
		{
		vecs = new UVW_TVVectorClass;
		iload->Read(vecs->handles, sizeof(int)*count*2, &nb);
		iload->Read(vecs->interiors, sizeof(int)*count, &nb);
		iload->Read(vecs->vhandles, sizeof(int)*count*2, &nb);
		iload->Read(vecs->vinteriors, sizeof(int)*count, &nb);
		}
	return nb;
	}

ULONG UVW_TVFaceClass::LoadFace(FILE *file)
	{
	ULONG nb = 0;
	fread(&count, sizeof(int), 1, file);

	if (t==NULL)
		t = new int[count];
	else
		{
		delete [] t;
		t = new int[count];
		}

	if (v==NULL)
		v = new int[count];
	else
		{
		delete [] v;
		v = new int[count];
		}

	fread(t, sizeof(int)*count, 1, file);
	fread(&FaceIndex, sizeof(int), 1, file);
	fread(&MatID, sizeof(int), 1, file);
	fread(&flags, sizeof(int), 1, file);
	fread(v, sizeof(int)*count, 1, file);
	vecs = NULL;

	if (flags & FLAG_CURVEDMAPPING)
		{
		vecs = new UVW_TVVectorClass;
		fread(vecs->handles, sizeof(int)*count*2, 1, file);
		fread(vecs->interiors, sizeof(int)*count, 1, file);
		fread(vecs->vhandles, sizeof(int)*count*2, 1, file);
		fread(vecs->vinteriors, sizeof(int)*count, 1, file);
		}
	return nb;
	}


int UVW_TVFaceClass::FindGeomEdge(int a, int b)
{
	for (int i = 0; i < count; i++)
	{
		int fa = v[i];
		int fb = v[(i+1)%count];
		if ( (fa == a) && (fb == b) )
			return i;
		if ( (fa == b) && (fb == a) )
			return i;

	}
	return -1;
}
int UVW_TVFaceClass::FindUVEdge(int a, int b)
{
	for (int i = 0; i < count; i++)
	{
		int fa = t[i];
		int fb = t[(i+1)%count];
		if ( (fa == a) && (fb == b) )
			return i;
		if ( (fa == b) && (fb == a) )
			return i;

	}
	return -1;
}


ULONG UVW_ChannelClass::LoadFaces(ILoad *iload)
	{
	ULONG nb;
	for (int i=0; i < f.Count(); i++)
		{
		nb = f[i]->LoadFace(iload);
		}
	return nb;
	}

ULONG UVW_ChannelClass::LoadFaces(FILE *file)
	{
	ULONG nb;
	for (int i=0; i < f.Count(); i++)
		{
		nb = f[i]->LoadFace(file);
		}
	return nb;
	}


ULONG UVW_ChannelClass::SaveFaces(ISave *isave)
	{
	ULONG nb;
	for (int i=0; i < f.Count(); i++)
		{
		nb = f[i]->SaveFace(isave);
		}
	return nb;
	}

ULONG UVW_ChannelClass::SaveFaces(FILE *file)
	{
	ULONG nb;
	for (int i=0; i < f.Count(); i++)
		{
		nb = f[i]->SaveFace(file);
		}
	return nb;
	}


void UVW_ChannelClass::SetCountFaces(int newct)
	{
//delete existing data
	int ct = f.Count();
	for (int i =0; i < ct; i++)
		{
		if (f[i]->vecs) delete f[i]->vecs;
		if (f[i]->t) delete [] f[i]->t;
		if (f[i]->v) delete [] f[i]->v;
		f[i]->vecs = NULL;
		f[i]->t = NULL;
		f[i]->v = NULL;

		delete f[i];
		f[i] = NULL;
		}

	f.SetCount(newct);
	for (int i =0; i < newct; i++)
		{
		f[i] = new UVW_TVFaceClass;
		f[i]->vecs = NULL;
		f[i]->t = NULL;
		f[i]->v = NULL;
		}


	}

void UVW_ChannelClass::CloneFaces(Tab<UVW_TVFaceClass*> &t)
	{
	int ct = f.Count();
	t.SetCount(ct);
	for (int i =0; i < ct; i++)
		t[i] = f[i]->Clone();
	}

void UVW_ChannelClass::AssignFaces(Tab<UVW_TVFaceClass*> &t)
	{
	//nuke old data and cassign clone of new
	int ct = f.Count();
	for (int i =0; i < ct; i++)
		{
		if (f[i]->vecs) delete f[i]->vecs;
		if (f[i]->t) delete [] f[i]->t;
		if (f[i]->v) delete [] f[i]->v;
		f[i]->vecs = NULL;
		f[i]->t = NULL;
		f[i]->v = NULL;

		delete f[i];
		f[i] = NULL;
		}
	ct = t.Count();
	f.SetCount(ct);
	for (int i =0; i < ct; i++)
		f[i] = t[i]->Clone();
	}

void UVW_ChannelClass::FreeFaces()
	{
	int ct = f.Count();
	for (int i =0; i < ct; i++)
		{
		if (f[i]->vecs) delete f[i]->vecs;
		if (f[i]->t) delete [] f[i]->t;
		if (f[i]->v) delete [] f[i]->v;
		f[i]->vecs = NULL;
		f[i]->t = NULL;
		f[i]->v = NULL;

		delete f[i];
		f[i] = NULL;
		}

	}

void UVW_ChannelClass::Dump()
	{
	for (int i = 0; i < v.Count(); i++)
		{
		DebugPrint("Vert %d pt %f %f %f\n",i,v[i].p.x,v[i].p.y,v[i].p.z);
		}
	for (int i = 0; i < f.Count(); i++)
		{
		DebugPrint("face %d t %d %d %d %d\n",i,f[i]->t[0],f[i]->t[1],f[i]->t[2],f[i]->t[3]);

		if (f[i]->vecs)
			DebugPrint("     int  %d %d %d %d\n",f[i]->vecs->interiors[0],f[i]->vecs->interiors[1],
										  f[i]->vecs->interiors[2],f[i]->vecs->interiors[3]
				   );
		
		if (f[i]->vecs)
			DebugPrint("     vec  %d,%d %d,%d %d,%d %d,%d\n",
										  f[i]->vecs->handles[0],f[i]->vecs->handles[1],
										  f[i]->vecs->handles[2],f[i]->vecs->handles[3],
										  f[i]->vecs->handles[4],f[i]->vecs->handles[5],
										  f[i]->vecs->handles[6],f[i]->vecs->handles[7]
				   );
		}
	}	

void UVW_ChannelClass::MarkDeadVertices()
	{
	BitArray usedVerts;
	usedVerts.SetSize(v.Count());
	usedVerts.ClearAll();

	for (int i =0; i < f.Count(); i++)
		{
		if (!(f[i]->flags & FLAG_DEAD))
			{
			for (int j=0; j < f[i]->count; j++)
				{
				int id = f[i]->t[j];
				if (id < usedVerts.GetSize()) usedVerts.Set(id);
				if ((f[i]->flags & FLAG_CURVEDMAPPING) && (f[i]->vecs))
					{
					id = f[i]->vecs->handles[j*2];
					if (id < usedVerts.GetSize()) usedVerts.Set(id);
					id = f[i]->vecs->handles[j*2+1];
					if (id < usedVerts.GetSize()) usedVerts.Set(id);
					if (f[i]->flags & FLAG_INTERIOR)
						{
						id = f[i]->vecs->interiors[j];
						if (id < usedVerts.GetSize()) usedVerts.Set(id);
						}
					}
	
				}
			}
		}

	for (int i =0; i < v.Count(); i++)
		{
		if (i < usedVerts.GetSize())
			{
			if (!usedVerts[i])
				{
				v[i].flags |= FLAG_DEAD;
				}
			}
		
		}
	

	}






void VertexLookUpListClass::addPoint(int a_index, Point3 a)
	{	
	BOOL found = FALSE;
	if (sel[a_index]) found = TRUE;
	if (!found)
		{
		VertexLookUpDataClass t;
		t.index = a_index;
		t.newindex = a_index;
		t.p = a;
		d[a_index] = t;
		sel.Set(a_index);
		}
	};


void UVW_ChannelClass::FreeEdges()
	{
	for (int i =0; i < e.Count(); i++)
		{
		for (int j =0; j < e[i]->data.Count(); j++)
			delete e[i]->data[j];
		delete e[i];
		}
	e.ZeroCount();
	ePtrList.ZeroCount();
	}

void UVW_ChannelClass::BuildEdges()
	{
	FreeEdges();
	if (v.Count() != 0)
		edgesValid = TRUE;
	e.SetCount(v.Count());
	


	for (int i = 0; i < v.Count();i++)
		{
		e[i] = new UVW_TVEdgeClass();
		}

	for (int i = 0; i < f.Count();i++)
		{
		if (!(f[i]->flags & FLAG_DEAD))
			{
			int pcount = 3;
			pcount = f[i]->count;
			int totalSelected = 0;
			for (int k = 0; k < pcount; k++)
				{
				int gv1 = f[i]->v[k];
				int gv2 = f[i]->v[(k+1)%pcount];
				int index1 = f[i]->t[k];
 
				int index2 = 0;
				if (k == (pcount-1))
					index2 = f[i]->t[0];
				else index2 = f[i]->t[k+1];
				int vec1=-1, vec2=-1;
				if ((f[i]->flags & FLAG_CURVEDMAPPING) &&(f[i]->vecs) )
					{
					vec1 = f[i]->vecs->handles[k*2];

					vec2 = f[i]->vecs->handles[k*2+1];
					}
				if (index2 < index1) 
					{
					int temp = index1;
					index1 = index2;
					index2 = temp;
					temp = vec1;
					vec1 = vec2;
					vec2 = temp;
					temp = gv1;
					gv1 = gv2;
					gv2 = temp;
					}
				BOOL hidden = FALSE;
				if (k==0)
					{
					if (f[i]->flags & FLAG_HIDDENEDGEA)
						hidden = TRUE;
					}
				else if (k==1)
					{
					if (f[i]->flags & FLAG_HIDDENEDGEB)
						hidden = TRUE;
					}
				else if (k==2)
					{
					if (f[i]->flags & FLAG_HIDDENEDGEC)
						hidden = TRUE;

					}
				AppendEdge(e,index1,vec1,index2,vec2,i,hidden,gv1,gv2);
				}	

			}
		}
	int ePtrCount = 0;
	for (int i =0; i < e.Count(); i++)
		{
		ePtrCount += e[i]->data.Count();
		}
	ePtrList.SetCount(ePtrCount);
	int ct = 0;
	for (int i =0; i < e.Count(); i++)
		{
		for (int j =0; j < e[i]->data.Count(); j++)
			ePtrList[ct++] = e[i]->data[j];
		}
//PELT
	for (int i = 0; i < ePtrList.Count(); i++)
	{
		ePtrList[i]->lookupIndex = i;
	}
	for (int i = 0; i < gePtrList.Count(); i++)
	{
		gePtrList[i]->lookupIndex = i;
	}

}

//PELT
void UVW_ChannelClass::FreeGeomEdges()
{
	for (int i =0; i < ge.Count(); i++)
	{
		for (int j =0; j < ge[i]->data.Count(); j++)
			delete ge[i]->data[j];
		delete ge[i];
	}
	ge.ZeroCount();
	gePtrList.ZeroCount();
}

void UVW_ChannelClass::BuildGeomEdges()
{
	FreeGeomEdges();
	if (v.Count() != 0)
		edgesValid = TRUE;
	ge.SetCount(geomPoints.Count());



	for (int i = 0; i < geomPoints.Count();i++)
	{
		ge[i] = new UVW_TVEdgeClass();
	}

	for (int i = 0; i < f.Count();i++)
	{
		if (!(f[i]->flags & FLAG_DEAD))
		{
			int pcount = 3;
			pcount = f[i]->count;
			int totalSelected = 0;
			for (int k = 0; k < pcount; k++)
			{

				int index1 = f[i]->v[k];

				int index2 = 0;
				if (k == (pcount-1))
					index2 = f[i]->v[0];
				else index2 = f[i]->v[k+1];
				int vec1=-1, vec2=-1;
				if ((f[i]->flags & FLAG_CURVEDMAPPING) &&(f[i]->vecs) )
				{
					vec1 = f[i]->vecs->vhandles[k*2];

					vec2 = f[i]->vecs->vhandles[k*2+1];
				}
				if (index2 < index1) 
				{
					int temp = index1;
					index1 = index2;
					index2 = temp;
					temp = vec1;
					vec1 = vec2;
					vec2 = temp;
				}
				BOOL hidden = FALSE;
				if (k==0)
				{
					if (f[i]->flags & FLAG_HIDDENEDGEA)
						hidden = TRUE;
				}
				else if (k==1)
				{
					if (f[i]->flags & FLAG_HIDDENEDGEB)
						hidden = TRUE;
				}
				else if (k==2)
				{
					if (f[i]->flags & FLAG_HIDDENEDGEC)
						hidden = TRUE;

	}
				AppendEdge(ge,index1,vec1,index2,vec2,i,hidden);
			}	

		}
	}
	int ePtrCount = 0;
	for (int i =0; i < ge.Count(); i++)
	{
		ePtrCount += ge[i]->data.Count();
	}
	gePtrList.SetCount(ePtrCount);
	int ct = 0;
	for (int i =0; i < ge.Count(); i++)
	{
		for (int j =0; j < ge[i]->data.Count(); j++)
			gePtrList[ct++] = ge[i]->data[j];
	}

	for (int i = 0; i < gePtrList.Count(); i++)
	{
		gePtrList[i]->lookupIndex = i;
	}

}


void UVW_ChannelClass::AppendEdge(Tab<UVW_TVEdgeClass*> &elist,int index1,int vec1, int index2,int vec2, int face, BOOL hidden, int gva, int gvb)
{
	if (elist[index1]->data.Count() == 0)
		{
		UVW_TVEdgeDataClass *temp = new UVW_TVEdgeDataClass();
		temp->a = index1;
		temp->avec = vec1;
		temp->b = index2;
		temp->bvec = vec2;
		temp->flags = 0;
		temp->ga = gva;
		temp->gb = gvb;
		if (hidden)
			temp->flags |= FLAG_HIDDENEDGEA;
		temp->faceList.Append(1,&face,4);
		elist[index1]->data.Append(1,&temp,4);
		}
	else
		{
		BOOL found = FALSE;
		for (int i =0; i < elist[index1]->data.Count(); i++)
			{
			if ( (elist[index1]->data[i]->b == index2) && (elist[index1]->data[i]->bvec == vec2))
				{
				found = TRUE;
				elist[index1]->data[i]->faceList.Append(1,&face,4);

				if ((!hidden) && (elist[index1]->data[i]->flags & FLAG_HIDDENEDGEA) )
					elist[index1]->data[i]->flags &= ~FLAG_HIDDENEDGEA;

				i = elist[index1]->data.Count();
				}
			}
		if (!found)
			{
			UVW_TVEdgeDataClass *temp = new UVW_TVEdgeDataClass();
			temp->a = index1;
			temp->avec = vec1;
			temp->b = index2;
			temp->bvec = vec2;
			temp->flags = 0;
			temp->ga = gva;
			temp->gb = gvb;
			if (hidden)
				temp->flags |= FLAG_HIDDENEDGEA;
			temp->faceList.Append(1,&face,4);
			elist[index1]->data.Append(1,&temp,4);
			}

		}

	}


float UVW_ChannelClass::LineToPoint(Point3 p1, Point3 l1, Point3 l2)
{
Point3 VectorA,VectorB,VectorC;
double Angle;
double dist = 0.0f;
VectorA = l2-l1;
VectorB = p1-l1;
float dot = DotProd(Normalize(VectorA),Normalize(VectorB));
if (dot == 1.0f) dot = 0.99f;
Angle =  acos(dot);
if (Angle > (3.14/2.0))
	{
	dist = Length(p1-l1);
	}
else
	{
	VectorA = l1-l2;
	VectorB = p1-l2;
	dot = DotProd(Normalize(VectorA),Normalize(VectorB));
	if (dot == 1.0f) dot = 0.99f;
	Angle = acos(dot);
	if (Angle > (3.14/2.0))
		{
		dist = Length(p1-l2);
		}
	else
		{
		double hyp;
		hyp = Length(VectorB);
		dist =  sin(Angle) * hyp;

		}

	}

return (float) dist;

}


int UVW_ChannelClass::EdgeIntersect(Point3 p, float threshold, int i1,int i2)
{

 static int startEdge = 0;
 if (startEdge >= ePtrList.Count()) startEdge = 0;
 if (ePtrList.Count() == 0) return -1;

 int ct = 0;
 BOOL done = FALSE;


 int hitEdge = -1;
 while (!done) 
	{
 //check bounding volumes

	Box3 bounds;
	bounds.Init();

	int index1 = ePtrList[startEdge]->a;
	int index2 = ePtrList[startEdge]->b;
	if ((v[index1].flags & FLAG_HIDDEN) &&  (v[index2].flags & FLAG_HIDDEN))
		{
		}
	else if ((v[index1].flags & FLAG_FROZEN) &&  (v[index1].flags & FLAG_FROZEN)) 
		{
		}
	else
		{
		Point3 p1(0.0f,0.0f,0.0f);
		p1[i1] = v[index1].p[i1];
		p1[i2] = v[index1].p[i2];
//		p1.z = 0.0f;
		bounds += p1;
		Point3 p2(0.0f,0.0f,0.0f);
		p2[i1] = v[index2].p[i1];
		p2[i2] = v[index2].p[i2];
//		p2.z = 0.0f;
		bounds += p2;
		bounds.EnlargeBy(threshold);
		if (bounds.Contains(p))
			{
//check edge distance
			if (LineToPoint(p, p1, p2) < threshold)
				{
				hitEdge = startEdge;
				done = TRUE;
//				LineToPoint(p, p1, p2);
				}
			}
		}
 	ct++;
	startEdge++;
	if (ct == ePtrList.Count()) done = TRUE;
	if (startEdge >= ePtrList.Count()) startEdge = 0;
	}
 
 return hitEdge;
}




void	UnwrapMod::RebuildEdges()
	{
	if (mode == ID_SKETCHMODE)
		SetMode(ID_UNWRAP_MOVE);

	BitArray holdVSel(vsel);

	BOOL holdSyncMode = fnGetSyncSelectionMode();
	fnSetSyncSelectionMode(FALSE);
/*
	if (esel.GetSize() > 0)
		{
		theHold.Suspend();
		fnEdgeToVertSelect();
		theHold.Resume();
		}
*/

	TVMaps.BuildEdges(  );
	//PELT
	TVMaps.BuildGeomEdges(  );

	if (esel.GetSize() != TVMaps.ePtrList.Count())
		{
		esel.SetSize(TVMaps.ePtrList.Count());
		esel.ClearAll();
		}

	if (gesel.GetSize() != TVMaps.gePtrList.Count())
		{
		gesel.SetSize(TVMaps.gePtrList.Count());
		gesel.ClearAll();
		}

	//FP 05/26/06 : PELT Mapping - CER Bucket #370291
	//When the topology of the object changes, the seams bits array need to be updated.  
	//By clearing it there, the update will be done each time the topology changes
	if (peltData.seamEdges.GetSize() != TVMaps.gePtrList.Count())
		{
		peltData.seamEdges.SetSize(TVMaps.gePtrList.Count());
		peltData.seamEdges.ClearAll();
		}

/*
	if (esel.GetSize() > 0)
		{
		theHold.Suspend();
		fnVertToEdgeSelect();
		theHold.Resume();
		}
*/


	vsel = holdVSel;

	fnSetSyncSelectionMode(holdSyncMode);

	usedVertices.SetSize(TVMaps.v.Count());
	usedVertices.ClearAll();

	for (int i = 0; i < TVMaps.f.Count(); i++)
		{
		int faceIndex = i;
		for (int k = 0; k < TVMaps.f[faceIndex]->count; k++)
			{
			if (!(TVMaps.f[faceIndex]->flags & FLAG_DEAD))
				{
				int vertIndex = TVMaps.f[faceIndex]->t[k];
				usedVertices.Set(vertIndex);
				if (objType == IS_PATCH)
					{
					if ((TVMaps.f[faceIndex]->flags & FLAG_CURVEDMAPPING) && (TVMaps.f[faceIndex]->vecs))
						{
						if (TVMaps.f[faceIndex]->flags & FLAG_INTERIOR)
							{
							vertIndex = TVMaps.f[faceIndex]->vecs->interiors[k];
							if ((vertIndex >=0) && (vertIndex < usedVertices.GetSize()))
								usedVertices.Set(vertIndex);
							}
						vertIndex = TVMaps.f[faceIndex]->vecs->handles[k*2];
						if ((vertIndex >=0) && (vertIndex < usedVertices.GetSize()))
							usedVertices.Set(vertIndex);
						vertIndex = TVMaps.f[faceIndex]->vecs->handles[k*2+1];
						if ((vertIndex >=0) && (vertIndex < usedVertices.GetSize()))
							usedVertices.Set(vertIndex);
						}
					}
				}
			}
		}

	//PELT
	for (int i = 0; i < peltData.springEdges.Count(); i++)
	{
		int vertIndex = peltData.springEdges[i].v1;
		if (vertIndex != -1)
			usedVertices.Set(vertIndex);
		vertIndex = peltData.springEdges[i].v2;
		if (vertIndex != -1)
			usedVertices.Set(vertIndex);

		vertIndex = peltData.springEdges[i].vec1;
		if ( (vertIndex != -1) && (vertIndex < usedVertices.GetSize()))
			usedVertices.Set(vertIndex);
		vertIndex = peltData.springEdges[i].vec2;
		if ( (vertIndex != -1) && (vertIndex < usedVertices.GetSize()))
			usedVertices.Set(vertIndex);

	}
	BuildEdgeDistortionData();

}




//pelt
void UVW_ChannelClass::EdgeListFromPoints(Tab<int> &selEdges, int source, int target, Point3 vec)
{
	//make sure point a and b are on the same element if not bail
	Tab<VConnections*> ourConnects;
	BOOL del = FALSE;

	BitArray selVerts;
	selVerts.SetSize(geomPoints.Count());
	selVerts.ClearAll();

	if (TRUE)
	{
		//loop through the edges
		Tab<int> numberOfConnections;
		numberOfConnections.SetCount(geomPoints.Count());
		for (int i = 0; i < geomPoints.Count(); i++)
		{
			numberOfConnections[i] = 0;
		}
		//count the number of vertxs connected
		for (int i = 0; i < gePtrList.Count(); i++)
		{
			if (!(gePtrList[i]->flags & FLAG_HIDDENEDGEA))
			{
				int a = gePtrList[i]->a;
				numberOfConnections[a] +=1;
				int b = gePtrList[i]->b;
				numberOfConnections[b] +=1;
			}
		}
		//allocate our connections now

		ourConnects.SetCount(geomPoints.Count());
		for (int i = 0; i < geomPoints.Count(); i++)
		{
			ourConnects[i] = new VConnections();
			ourConnects[i]->closestNode = NULL;
			ourConnects[i]->linkedListChild = NULL;
			ourConnects[i]->linkedListParent = NULL;
			ourConnects[i]->accumDist = 1.0e+9f;
			ourConnects[i]->solved = FALSE;
			ourConnects[i]->vid = i;

			ourConnects[i]->connectingVerts.SetCount(numberOfConnections[i]);
		}

		for (int i = 0; i < geomPoints.Count(); i++)
		{
			numberOfConnections[i] = 0;
		}

		//build our vconnection data
		for (int i = 0; i < gePtrList.Count(); i++)
		{
			if (!(gePtrList[i]->flags & FLAG_HIDDENEDGEA))
			{
				int a = gePtrList[i]->a;
				int b = gePtrList[i]->b;

				int index = numberOfConnections[a];
				ourConnects[a]->connectingVerts[index].vertexIndex = b;
				ourConnects[a]->connectingVerts[index].edgeIndex = i;
				numberOfConnections[a] +=1;

				index = numberOfConnections[b];
				ourConnects[b]->connectingVerts[index].vertexIndex = a;
				ourConnects[b]->connectingVerts[index].edgeIndex = i;
				numberOfConnections[b] +=1;
			}
		}


		del = TRUE;
	}
	//	else ourConnects = vConnects;

	//spider out till hit our target or no more left
	BOOL done = FALSE;
	BOOL hit = FALSE;
	Tab<int> vertsToDo;
	BitArray processedVerts;
	processedVerts.SetSize(ourConnects.Count());
	processedVerts.ClearAll();

	//if no more left bail
	int currentVert = source;
	while (!done)
	{
		//	int startingNumberProcessed = processedVerts.NumberSet();

		int ct = ourConnects[currentVert]->connectingVerts.Count();
		processedVerts.Set(currentVert, TRUE);
		for (int j = 0; j < ct; j++)
		{
			int index = ourConnects[currentVert]->connectingVerts[j].vertexIndex;
			if (index == target) 
			{
				done = TRUE;
				hit = TRUE;
			}
			if (!processedVerts[index])
				vertsToDo.Append(1,&index, 10000);
		}

		if (vertsToDo.Count())
		{
			currentVert = vertsToDo[vertsToDo.Count()-1];
			vertsToDo.Delete(vertsToDo.Count()-1,1);
		}

		if (vertsToDo.Count() == 0) done = TRUE;
		//		int endingNumberProcessed = processedVerts.NumberSet();

		if (currentVert == target) 
		{
			done = TRUE;
			hit = TRUE;
		}
		//		if (startingNumberProcessed == endingNumberProcessed)
		//			done = TRUE;
	}
	vertsToDo.ZeroCount();

 	if (hit)
	{
		Tab<VConnections*> solvedNodes;

		ourConnects[source]->accumDist = 0;

		VConnections* unsolvedNodeHead = ourConnects[source];
		VConnections* unsolvedNodeCurrent = unsolvedNodeHead;
		
		//put all our vertices in the unsolved list
		
		for (int i = 0; i < ourConnects.Count(); i++)
		{			
			if (ourConnects[i] != unsolvedNodeHead)
			{
				unsolvedNodeCurrent->linkedListChild = ourConnects[i];
				VConnections *parent = unsolvedNodeCurrent;
				unsolvedNodeCurrent = ourConnects[i];
				unsolvedNodeCurrent->linkedListParent = parent;
			}
		}

		//build our edge distances
		Tab<float> edgeDistances;
		edgeDistances.SetCount(gePtrList.Count());
		for (int i = 0 ; i < gePtrList.Count(); i++)
		{
			int a = gePtrList[i]->a;
			int b = gePtrList[i]->b;
			float d = Length(geomPoints[a] - geomPoints[b]);
			edgeDistances[i] = d;

		}


		BOOL done = FALSE;
		while (!done)
		{
			//pop the top unsolved
			VConnections *top = unsolvedNodeHead;


//DebugPrint("added verts %d\n",top->vid);
//if (top->vid == 187)
//DebugPrint("Break\n");
			unsolvedNodeHead = unsolvedNodeHead->linkedListChild;
			top->linkedListChild = NULL;
			top->linkedListParent = NULL;
			if (unsolvedNodeHead != NULL)
			{
				unsolvedNodeHead->linkedListParent = NULL;
				//mark it as processed
				top->solved = TRUE;
				
				//put it in our solved list
				solvedNodes.Append(1,&top,5000);

				int neighborCount = top->connectingVerts.Count();
				//loop through the neighbors
				for (int i = 0; i < neighborCount; i++)
				{
					int index  = top->connectingVerts[i].vertexIndex;
					int eindex  = top->connectingVerts[i].edgeIndex;
					VConnections *neighbor = ourConnects[index];
					//make sure it is not procssedd
					if (!neighbor->solved)
					{
						//find the distance from the top to this neighbor
						float d = neighbor->accumDist;
						float testAccumDistance = top->accumDist + edgeDistances[eindex];
						//see if it accum dist needs to be relaxed
						if (testAccumDistance<d)
						{
							float originalDist = neighbor->accumDist;
							neighbor->accumDist = testAccumDistance;
							neighbor->closestNode = top;
							//sort this node
							float dist = neighbor->accumDist;
if (0)
{
	DebugPrint("Node list\n");
	VConnections *cn = unsolvedNodeHead;
	
	while(cn != NULL)
	{
		if (cn->accumDist < 10000.0f)
			DebugPrint(" %d %f, ", cn->vid,cn->accumDist);
		cn = cn->linkedListChild;
	}
	DebugPrint("\nSort dist \n",dist);
}

							if (originalDist == 1.0e+9f)
							{
								//start at the top and look down
								VConnections *currentNode = unsolvedNodeHead;
								if (neighbor == currentNode)
								{
									currentNode = currentNode->linkedListChild;
									unsolvedNodeHead = currentNode;
								}
								VConnections *prevNode = NULL;
								//unhook node
								VConnections *parent = neighbor->linkedListParent;
								VConnections *child = neighbor->linkedListChild;
								if (parent)
									parent->linkedListChild = child;
								if (child)
									child->linkedListParent = parent;
									

								while ((currentNode!= NULL) && (currentNode->accumDist < dist))
								{
								
									prevNode = currentNode;
									currentNode = currentNode->linkedListChild;
									SHORT iret = GetAsyncKeyState (VK_ESCAPE);
									if (iret==-32767)
									{
										done = TRUE;
										currentNode= NULL;
									}
								}
								//empty list
								if ((prevNode==NULL) && (currentNode== NULL))
								{
									neighbor->linkedListChild = NULL;
									neighbor->linkedListParent = NULL;
									unsolvedNodeHead = neighbor;
								}
								//at top
								else if (currentNode && (prevNode == NULL))
								{
									unsolvedNodeHead->linkedListParent = neighbor;
									neighbor->linkedListParent = NULL;
									neighbor->linkedListChild =unsolvedNodeHead;
									unsolvedNodeHead = neighbor;

								}
								//at bottom
								else if (currentNode == NULL)
								{
									prevNode->linkedListChild = neighbor;
									neighbor->linkedListParent = currentNode;
									neighbor->linkedListChild = NULL;
								}

								else if (currentNode)
								{
									//insert
									VConnections *parent = currentNode->linkedListParent;
									VConnections *child = currentNode;
		
									parent->linkedListChild = neighbor;
									child->linkedListParent = neighbor;

									neighbor->linkedListParent = parent;
									neighbor->linkedListChild = child;

								}



							}
							else
							{
								//sort smallest to largest
								BOOL moveUp = FALSE;

								if (neighbor->linkedListParent && neighbor->linkedListChild)
								{
									float parentDist = neighbor->linkedListParent->accumDist;
									float childDist = neighbor->linkedListChild->accumDist;
									//walkup up or down the list till find a spot an unlink and relink
									
									if ((dist >= parentDist) && (dist <= childDist))
									{
										//done dont need to move
									}
									else if (dist < parentDist)
										moveUp = FALSE;
								}
								else if (neighbor->linkedListParent && (neighbor->linkedListChild==NULL))
								{
									moveUp = TRUE;
								}
								else if ((neighbor->linkedListParent==NULL) && (neighbor->linkedListChild))
								{
									moveUp = FALSE;
								}

								//unlink the node
								//unhook node
								VConnections *parent = neighbor->linkedListParent;
								VConnections *child = neighbor->linkedListChild;
								if (parent)
									parent->linkedListChild = child;
								else unsolvedNodeHead = child;
								if (child)
									child->linkedListParent = parent;

								VConnections *currentNode = NULL;
								if (moveUp)
									currentNode = neighbor->linkedListParent;
								else currentNode = neighbor->linkedListChild;
								VConnections *prevNode = NULL;

								while ((currentNode!= NULL) && (currentNode->accumDist < dist))
								{							
									prevNode = currentNode;
									if (moveUp)
										currentNode = currentNode->linkedListParent;
									else currentNode = currentNode->linkedListChild;

									SHORT iret = GetAsyncKeyState (VK_ESCAPE);
									if (iret==-32767)
									{
										done = TRUE;
										currentNode= NULL;
									}

								}

								//empty list
								if ((prevNode==NULL) && (currentNode== NULL))
								{
									neighbor->linkedListChild = NULL;
									neighbor->linkedListParent = NULL;
									unsolvedNodeHead = neighbor;
								}
								//at top
								else if (currentNode && (prevNode == NULL))
								{
									unsolvedNodeHead->linkedListParent = neighbor;
									neighbor->linkedListParent = NULL;
									neighbor->linkedListChild =unsolvedNodeHead;
									unsolvedNodeHead = neighbor;

								}
								//at bottom
								else if (currentNode == NULL)
								{
									prevNode->linkedListChild = neighbor;
									neighbor->linkedListParent = currentNode;
									neighbor->linkedListChild = NULL;
								}
								else if (currentNode)
								{
									//insert
									VConnections *parent = currentNode->linkedListParent;
									VConnections *child = currentNode;
		
									parent->linkedListChild = neighbor;
									child->linkedListParent = neighbor;

									neighbor->linkedListParent = parent;
									neighbor->linkedListChild = child;

								}


							}

						}					
					}
				}
			}
			

			if (unsolvedNodeHead == NULL)
				done = TRUE;
			if ((solvedNodes[solvedNodes.Count()-1]) == ourConnects[target])
				done = TRUE;
		}
		//now get our edge list
		selEdges.ZeroCount();
		VConnections *cNode = ourConnects[target];
		while ((cNode->closestNode != NULL) && (cNode != ourConnects[source]))
		{
			VConnections *pNode = cNode->closestNode;
			int ct = cNode->connectingVerts.Count();
			int edgeIndex = -1;
			for (int i = 0; i < ct; i++)
			{
				int vindex = cNode->connectingVerts[i].vertexIndex;
				int eindex = cNode->connectingVerts[i].edgeIndex;
				if (ourConnects[vindex] == pNode)
				{
					edgeIndex = eindex;
					i = ct;
				}
			}
			if (edgeIndex != -1)
				selEdges.Append(1,&edgeIndex,500);

			cNode = pNode;
		}

	}

/*
	if (hit)
	{
		//build a tm from our 2 poitns and vec
		Point3 veca = geomPoints[source]- geomPoints[target];
		veca = Normalize(veca);
		vec = Normalize(vec);
		Point3 cross = veca^vec;
		cross = Normalize(cross);

		Matrix3 tm;
		MatrixFromNormal(cross, tm);
		tm.SetRow(3, geomPoints[source]);
		Matrix3 itm = Inverse(tm);
		Tab<Point3> faceNorms;
		faceNorms.SetCount(f.Count());
		//build our transition verts, these are verts that are connected to both front and back facing faces
		//build a normal list for faces
		for (int i = 0; i < f.Count(); i++)
		{
			Point3 vecA,vecB;
			int a,b,c;
			a = f[i]->v[0];
			b = f[i]->v[1];
			c = f[i]->v[2];
			vecA  = geomPoints[b]-geomPoints[a];
			vecB  = geomPoints[b]-geomPoints[b];
			faceNorms[i] = vecA^vecB;
			faceNorms[i] = VectorTransform(faceNorms[i],itm);
		}

		//loop through the edge looking for an edge that has both pos, nega tive normals
		BitArray flipEdge;
		flipEdge.SetSize(gePtrList.Count());
		flipEdge.ClearAll();

		for (int i = 0; i < gePtrList.Count(); i++)
		{
			int ct = gePtrList[i]->faceList.Count();
			int faceIndex = gePtrList[i]->faceList[0];

			float startFaceZ = faceNorms[faceIndex].z;
			BOOL flip = FALSE;


			for (int j = 1; j < ct; j++)
			{
				int faceIndex = gePtrList[i]->faceList[j];
				float nextFaceZ = faceNorms[faceIndex].z;
				if ((startFaceZ < 0.0f)  && ( nextFaceZ > 0.0f))
					flip = TRUE;
				else if ((startFaceZ > 0.0f)  && ( nextFaceZ < 0.0f))
					flip = TRUE;
			}


			//tag that edge
			if (flip)
				flipEdge.Set(i,TRUE);

		}
		//loop through the edges and tag the vertices that are on the edges
		BitArray transitionVerts;
		transitionVerts.SetSize(geomPoints.Count());
		transitionVerts.ClearAll();

		for (int i = 0; i < gePtrList.Count(); i++)
		{
			if (flipEdge[i])
			{
				int a = gePtrList[i]->a;
				int b = gePtrList[i]->b;
				transitionVerts.Set(a,TRUE);
				transitionVerts.Set(b,TRUE);
			}
		}



		//while we dont hit out target do
		BOOL done = FALSE;
		int currentVert = source;
		BOOL goingTowards = TRUE;
		processedVerts.ClearAll();
		processedVerts.Set(currentVert,TRUE);
		Tab<int> verts;
		verts.Append(1,&currentVert,10000);
		selVerts.Set(currentVert,TRUE);
		Point3 targetP = geomPoints[target];
		while (!done)
		{

			//get all the edges that go from this point
			int ct = ourConnects[currentVert]->connectingVerts.Count();
			//do our first vert;
			float dist = -1.0f;
			float zDist = -1.0f;
			int potentialNextVert = -1;


			float cDist = Length(targetP-geomPoints[currentVert]);

			for (int j = 0; j < ct; j++)
			{
				//make sure the vert is not a processed one
				int vertIndex = ourConnects[currentVert]->connectingVerts[j];
				if (!processedVerts[vertIndex])
				{
					Point3 p = geomPoints[vertIndex];
					Point3 tp = p * itm;
					float z = fabs(tp.z);

					float d = Length(p-targetP);
					if (goingTowards)
					{
						if (d < cDist)
						{
							if ((z < zDist) || (zDist == -1.0f))
							{
								zDist = z;
								potentialNextVert = vertIndex;
							}
						}
					}
					else
					{
						if (d > cDist)
						{
							if ((z < zDist) || (zDist == -1.0f))
							{
								zDist = z;
								potentialNextVert = vertIndex;
							}
						}

					}
					//if going towards find the closet
					//otherwise we are going away find the farther
				}
			}
			if (potentialNextVert == -1)
			{
				for (int j = 0; j < ct; j++)
				{
					//make sure the vert is not a processed one
					int vertIndex = ourConnects[currentVert]->connectingVerts[j];
					if (!processedVerts[vertIndex])
					{
						Point3 p = geomPoints[vertIndex];
						Point3 tp = p * itm;
						float z = fabs(tp.z);

						float d = Length(p-targetP);
						if (goingTowards)
						{
							//							if (d < cDist)
							{
								if ((z > zDist) || (zDist == -1.0f))
								{
									zDist = z;
									potentialNextVert = vertIndex;
								}
							}
						}
						else
						{
							//							if (d > cDist)
							{
								if ((z > zDist) || (zDist == -1.0f))
								{
									zDist = z;
									potentialNextVert = vertIndex;
								}
							}

						}
						//if going towards find the closet
						//otherwise we are going away find the farther
					}
				}

			}
			//make sure we are not deadlocked
			if (potentialNextVert != -1)
			{

				if (processedVerts[potentialNextVert])
					done = TRUE;
				currentVert = potentialNextVert;
				//if new vert is a transition one flip goingtowards
				if (transitionVerts[currentVert])
					goingTowards = FALSE;
				processedVerts.Set(currentVert,TRUE);
				verts.Append(1,&currentVert,10000);
				selVerts.Set(currentVert,TRUE);
			}
			else done = TRUE;
			//add escape bail
			SHORT iret = GetAsyncKeyState (VK_ESCAPE);
			if (iret==-32767)
			{
				done = TRUE;
			}
			if (currentVert == target)
				done = TRUE;
		}

		//convert to an edge selection
		//iterate through our verts
		if (verts.Count() > 0)
		{
			int currentVert = verts[0];
			for (int i = 1; i < verts.Count(); i++)
			{
				//get the next point
				int nextVert = verts[i];
				//get edge a look to see if there is point in both
				int ct = gePtrList.Count();
				int hitEdge = -1;
				for (int j = 0; j < ct; j++)
				{
					int a = gePtrList[j]->a;
					int b = gePtrList[j]->b;
					if ( ((a == currentVert) && (b == nextVert)) ||
						((b == currentVert) && (a == nextVert)) )
					{
						selEdges.Append(1,&j,1000);					
						j = ct;
					}
				}
				currentVert = nextVert;
				//get edge b look to if tehre is point in both
			}
		}
	}

*/

	if (del)
	{
		for (int i = 0; i < geomPoints.Count(); i++)
		{
			delete ourConnects[i];
		}
	}
}

int UVW_ChannelClass::GetNextEdge(int currentEdgeIndex, int cornerVert, int currentFace)
{
	//get the a b points
	int a = ePtrList[currentEdgeIndex]->a;
	int b = ePtrList[currentEdgeIndex]->b;

	//loop through the face looking for the corner
	int deg = f[currentFace]->count;
	int opposingVert = -1;
	for (int j = 0; j < deg; j++)
	{
		int fa = f[currentFace]->t[j];
		int fb = f[currentFace]->t[(j+1)%deg];

		if (fa == cornerVert)
		{
			//make sure it is not our initial edge
			if ( ((fa!= a) && (fb != b)) || ((fa!= b) && (fb != a)) )
				opposingVert = fb;
		}
		if (fb == cornerVert)
		{
			//make sure it is not our initial edge
			if ( ((fa!= a) && (fb != b)) || ((fa!= b) && (fb != a)) )
				opposingVert = fa;
		}
	}

//find out the edge
	int nextEdge = -1;
	if (opposingVert != -1)
	{
		int ct = e[cornerVert]->data.Count();
		
		for (int i = 0; i < ct; i++)
		{
			int ea = e[cornerVert]->data[i]->a;
			int eb = e[cornerVert]->data[i]->b;
			if ( ( (ea == cornerVert) && (eb == opposingVert) ) ||
				 ( (eb == cornerVert) && (ea == opposingVert) ) )
			{
				nextEdge =  e[cornerVert]->data[i]->lookupIndex;
				i = ct; 
			}
		}
		if (nextEdge != -1)
		{
			ct = e[opposingVert]->data.Count();
					
			for (int i = 0; i < ct; i++)
			{
				int ea = e[opposingVert]->data[i]->a;
				int eb = e[opposingVert]->data[i]->b;
				if ( ( (ea == cornerVert) && (eb == opposingVert) ) ||
					( (eb == cornerVert) && (ea == opposingVert) ) )
				{
					nextEdge =  e[opposingVert]->data[i]->lookupIndex;
					i = ct; 
				}
			}
		}
	}
	return nextEdge;
}

//PELT

void UVW_ChannelClass::SplitUVEdges(BitArray esel)
{

	
	//get our point selection from the edges
	BitArray pointSel;
	pointSel.SetSize(v.Count());
	pointSel.ClearAll();
	for (int i = 0; i < ePtrList.Count(); i++)
	{
		if (esel[i])
		{
			int a = ePtrList[i]->a;
			int b = ePtrList[i]->b;
			pointSel.Set(a,TRUE);
			pointSel.Set(b,TRUE);
		}
	}

//build a lis of egdes for each vert
	Tab<VEdges*> edgesAtVertex;
	edgesAtVertex.SetCount(v.Count());
	for (int i = 0; i < v.Count(); i++)
		edgesAtVertex[i] = NULL;

	for (int i = 0; i < ePtrList.Count(); i++)
	{
		int a = ePtrList[i]->a;
		if (pointSel[a])
		{
			if (edgesAtVertex[a] == NULL)
				edgesAtVertex[a] = new VEdges();

			edgesAtVertex[a]->edgeIndex.Append(1,&i,5);
		}

		a = ePtrList[i]->b;
		if (pointSel[a])
		{
			if (edgesAtVertex[a] == NULL)
				edgesAtVertex[a] = new VEdges();

			edgesAtVertex[a]->edgeIndex.Append(1,&i,5);
		}

	}

	BitArray processedFaces;
	processedFaces.SetSize(f.Count());

	BitArray processedEdges;
	processedEdges.SetSize(ePtrList.Count());

	int originalCount = v.Count();
	for (int i = 0; i < originalCount; i++)
	{
		if (pointSel[i])
		{
			processedFaces.ClearAll();
			processedEdges.ClearAll();

			int ct = edgesAtVertex[i]->edgeIndex.Count();
			BOOL first = TRUE;


			for (int j = 0; j < ct; j++)
			{
				int currrentEdgeIndex =  edgesAtVertex[i]->edgeIndex[j];
				//find a selected edge
				if (esel[currrentEdgeIndex])
				{

					int numFaces = ePtrList[currrentEdgeIndex]->faceList.Count();

					for (int m = 0; m < numFaces; m++)
					{
						int newVertIndex = -1;
						if (first)
						{
							//just set the index
							newVertIndex = i;
							first = FALSE;
						}
						else
						{
							//create a new vertex 
							UVW_TVVertClass newVert;
							newVert.p = v[i].p;
							newVert.influence = 0.0f;
							newVert.flags = 0;
							//create a new handle if we need one
							v.Append(1,&newVert);
							Control* c;
							c = NULL;
							cont.Append(1,&c,1);
							newVertIndex = v.Count()-1;
						}
						int currentFaceIndex = ePtrList[currrentEdgeIndex]->faceList[m];
						//DebugPrint("New Cluster\n");
						if (!processedFaces[currentFaceIndex])
						{
							BOOL done = FALSE;
							int lastEdge = j;
							while (!done)
							{
								//loop through this face looking for matching vert
								int deg = f[currentFaceIndex]->count;
								for (int n = 0; n < deg; n++)
								{
									//replace it
									if (f[currentFaceIndex]->t[n] == i)
									{
										f[currentFaceIndex]->t[n] = newVertIndex;
										//										DebugPrint("Replaceing face %d\n",currentFaceIndex);
									}
								}
								processedFaces.Set(currentFaceIndex,TRUE);

								//loop til we find another selected face or an opent edge
								int nextEdge = -1;								
								for (int n = 0; n < ct; n++)
								{
									//mkae sure we are not looking at the current edge
									int potentialEdge = edgesAtVertex[i]->edgeIndex[n];
									if (n != lastEdge)
									{										
										int nfaces = ePtrList[potentialEdge]->faceList.Count();
										if (nfaces != -1)
										{
											for (int p = 0; p < nfaces; p++)
											{
												if (ePtrList[potentialEdge]->faceList[p] == currentFaceIndex)
												{
													nextEdge = potentialEdge;
													lastEdge = n;
													p = nfaces;
													n = ct; 
												}
											}
										}
									}
								}
								//if we hit an edge we are done with this cluster
								if (nextEdge==-1)
									done = TRUE;
								else if (ePtrList[nextEdge]->a == ePtrList[nextEdge]->b)
									done = TRUE;
								else if (esel[nextEdge])
									done = TRUE;
								else if (nextEdge != -1)
								{
									//get the next face
									int nfaces = ePtrList[nextEdge]->faceList.Count();
									BOOL hit = FALSE;
									for (int p = 0; p < nfaces; p++)
									{
										if (ePtrList[nextEdge]->faceList[p]!= currentFaceIndex)
										{
											currentFaceIndex = ePtrList[nextEdge]->faceList[p];
											hit = TRUE;
											p = nfaces;
										}
									}
									if (!hit) done = TRUE;

								}



							}
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < originalCount; i++)
	{
		if (edgesAtVertex[i])
			delete edgesAtVertex[i];
		
	}


	BuildEdges();
	BuildGeomEdges();

	Tab<int> numberConnectedEdges;
	numberConnectedEdges.SetCount(v.Count());
	for (int i = 0; i < v.Count(); i++)
	{
		numberConnectedEdges[i] = 0;
	}
//loop through our egdes
	for (int i = 0; i < ePtrList.Count(); i++)
	{
		int veca = ePtrList[i]->avec;
		int vecb = ePtrList[i]->bvec;
		if (veca != -1)
			numberConnectedEdges[veca] += 1;
		if (vecb != -1)
			numberConnectedEdges[vecb] += 1;
	}
/*
	for (int i = 0; i < v.Count(); i++)
	{
		DebugPrint("%d  ct %d\n",i,numberConnectedEdges[i]);
	}
*/

	for (int i = 0; i < f.Count(); i++)
	{
		if (f[i]->vecs)
		{
			int deg = f[i]->count;
			for (int j = 0; j < deg*2; j++)
			{
				int va;
				va = f[i]->vecs->handles[j];
				if (numberConnectedEdges[va] > 1)
				{
					//clone the vert
					UVW_TVVertClass newVert;
					newVert.p = v[va].p;
					newVert.influence = 0.0f;
					newVert.flags = 0;
					//create a new handle if we need one
					v.Append(1,&newVert);
					Control* c;
					c = NULL;
					cont.Append(1,&c,1);
					int newVertIndex = v.Count()-1;
					//assign it
					f[i]->vecs->handles[j] = newVertIndex;
					//dec our counrt
					numberConnectedEdges[va] -= 1;
				}
			}
		}
	}
	BuildEdges();
	BuildGeomEdges();

	

}


int UVW_ChannelClass::FindGeoEdge(int a, int b)
{
	int edgeIndex = -1;
	int ct = ge[a]->data.Count();
	for (int i = 0; i < ct; i++)
	{
		int ea = ge[a]->data[i]->a;
		int eb = ge[a]->data[i]->b;

		if ( ((ea == a) && (eb == b)) ||
			 ((ea == b) && (eb == a)) )
			 return ge[a]->data[i]->lookupIndex;
	}

	ct = ge[b]->data.Count();
	for (int i = 0; i < ct; i++)
	{
		int ea = ge[b]->data[i]->a;
		int eb = ge[b]->data[i]->b;

		if ( ((ea == a) && (eb == b)) ||
			 ((ea == b) && (eb == a)) )
			 return ge[b]->data[i]->lookupIndex;
	}

	return edgeIndex;
}
void UVW_ChannelClass::BuildAdjacentUVEdgesToVerts(Tab<AdjacentItem*> &verts)
{
	verts.SetCount(v.Count());
	for (int i = 0; i < v.Count(); i++)
	{
		verts[i] = new AdjacentItem();
	}

	for (int i = 0; i < ePtrList.Count(); i++)
	{
		int a = ePtrList[i]->a;
		verts[a]->index.Append(1,&i,5);
		a = ePtrList[i]->b;
		verts[a]->index.Append(1,&i,5);
	}

}


void UVW_ChannelClass::BuildAdjacentGeomEdgesToVerts(Tab<AdjacentItem*> &verts)
{
	verts.SetCount(geomPoints.Count());
	for (int i = 0; i < geomPoints.Count(); i++)
	{
		verts[i] = new AdjacentItem();
	}

	for (int i = 0; i < gePtrList.Count(); i++)
	{
		int a = gePtrList[i]->a;
		verts[a]->index.Append(1,&i,5);
		a = gePtrList[i]->b;
		verts[a]->index.Append(1,&i,5);
	}
}

void UVW_ChannelClass::BuildAdjacentUVFacesToVerts(Tab<AdjacentItem*> &verts)
{
	verts.SetCount(v.Count());
	for (int i = 0; i < v.Count(); i++)
	{
		verts[i] = new AdjacentItem();
	}

	for (int i = 0; i < f.Count(); i++)
	{
		if (!(f[i]->flags & FLAG_DEAD ))
		{
			int deg = f[i]->count;
			for (int j = 0; j < deg; j++)
			{
				int a = f[i]->t[j];

				verts[a]->index.Append(1,&i,5);
			}
		}
	}
}


Point3 UVW_ChannelClass::UVFaceNormal(int index)
{
	if (index < 0) return Point3(0.0f,0.0f,1.0f);
	if (index >= f.Count()) return Point3(0.0f,0.0f,1.0f);

	int ct = f[index]->count;
	if (f[index]->count < 3) 
		return Point3(0.0f,0.0f,1.0f);



	Point3 vec1,vec2;
	if (f[index]->count == 3)
	{
		int a = f[index]->t[0];
		int b = f[index]->t[1];
		int c = f[index]->t[2];
		vec1 = Normalize(v[b].p-v[a].p);
		vec2 = Normalize(v[c].p-v[a].p);

	}
	else
	{
		int a = f[index]->t[0];
		int b = f[index]->t[1];
		
		vec1 = Normalize(v[b].p-v[a].p);
		for (int i = 2; i < f[index]->count; i++)
		{
			b = f[index]->t[i];
			vec2 = Normalize(v[b].p-v[a].p);
			float dot = DotProd(vec1,vec2);
			if (fabs(dot) != 1.0f) 
				i = f[index]->count;
		}

	}

	Point3 norm = CrossProd(vec1,vec2);
	return Normalize(norm);

}

Point3 UVW_ChannelClass::GeomFaceNormal(int index)
{
	if (index < 0) return Point3(0.0f,0.0f,1.0f);
	if (index >= f.Count()) return Point3(0.0f,0.0f,1.0f);

	if (f[index]->count < 3) 
		return Point3(0.0f,0.0f,1.0f);

	Point3 vec1,vec2;
	if (f[index]->count == 3)
	{
		int a = f[index]->v[0];
		int b = f[index]->v[1];
		int c = f[index]->v[2];
		vec1 = Normalize(geomPoints[b]-geomPoints[a]);
		vec2 = Normalize(geomPoints[c]-geomPoints[a]);

	}
	else
	{
		int a = f[index]->v[0];
		int b = f[index]->v[1];
		
		vec1 = Normalize(geomPoints[b]-geomPoints[a]);
		for (int i = 2; i < f[index]->count; i++)
		{
			b = f[index]->v[i];
			vec2 = Normalize(geomPoints[b]-geomPoints[a]);
			float dot = DotProd(vec1,vec2);
			if (fabs(dot) != 1.0f) 
				i = f[index]->count;
		}

	}

	Point3 norm = CrossProd(vec1,vec2);
	return Normalize(norm);

}


Matrix3 UVW_ChannelClass::MatrixFromGeoFace(int index)
{

	if (f[index]->count < 3) 
		return Matrix3(1);

	Matrix3 tm(1);
	Point3 xvec,yvec,zvec;
	zvec = GeomFaceNormal(index);
	int a,b;
	a = f[index]->v[0];
	b = f[index]->v[1];
	xvec = Normalize(geomPoints[b]-geomPoints[a]);
	yvec = Normalize(CrossProd(xvec,zvec));

	tm.SetRow(0,xvec);
	tm.SetRow(1,yvec);
	tm.SetRow(2,zvec);
	tm.SetRow(3,geomPoints[a]);

	return tm;
}
Matrix3 UVW_ChannelClass::MatrixFromUVFace(int index)
{
	Matrix3 tm(1);

	if (f[index]->count < 3) 
		return Matrix3(1);

	Point3 xvec,yvec,zvec;
	zvec = UVFaceNormal(index);
	int a,b;
	a = f[index]->t[0];
	b = f[index]->t[1];
	xvec = Normalize(v[b].p-v[a].p);
	yvec = Normalize(CrossProd(xvec,zvec));

	tm.SetRow(0,xvec);
	tm.SetRow(1,yvec);
	tm.SetRow(2,zvec);
	tm.SetRow(3,v[a].p);

	return tm;
}
