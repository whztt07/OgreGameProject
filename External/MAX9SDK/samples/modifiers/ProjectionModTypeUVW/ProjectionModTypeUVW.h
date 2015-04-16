/**********************************************************************
 *<
	FILE:		    ProjectionModTypeUVW.h
	DESCRIPTION:	Projection Modifier Type UVW Header
	CREATED BY:		Michael Russo
	HISTORY:		Created 03-23-2004

 *>	Copyright (c) 2004 Discreet, All Rights Reserved.
 **********************************************************************/

#ifndef __PROJECTIONMODTYPEUVW__H
#define __PROJECTIONMODTYPEUVW__H

#include "DllEntry.h"

#include "IProjectionMod.h"
#include "ObjectWrapper.h"
#include "ProjectionHolderUVW.h"

#define PROJECTIONMODTYPEUVW_CLASS_ID	Class_ID(0x56bb7872, 0x2d4c74ef)
#define PBLOCK_REF 0

class ProjectionState;

//=============================================================================
//
//	Class ProjectionModTypeUVW
//
//=============================================================================
class ProjectionModTypeUVW : public IProjectionModType
{
	protected:
		static Interface*	mpIP;
		Interval			mValid;
		IParamBlock2		*mpPBlock;

		bool				mbEnabled;
		bool				mbEditing;
		TSTR				mName;
		int					miIndex;
		HWND				mhPanel;
		bool				mbSuspendPanelUpdate;
		bool				mbAlwaysUpdate;
		int					miSourceMapChannel;
		int					miTargetMapChannel;
		bool				mbInModifyObject;

		ISpinnerControl*	mSpinnerSourceMapChannel;
		ISpinnerControl*	mSpinnerTargetMapChannel;

	public:
		ProjectionModTypeUVW(BOOL create);
		~ProjectionModTypeUVW();

		IProjectionMod*		mpPMod;

		const TCHAR *	ClassName() { return GetString(IDS_PROJECTIONMODTYPEUVW_CLASS_NAME); }
		SClass_ID		SuperClassID() { return REF_TARGET_CLASS_ID; }
		Class_ID ClassID() { return PROJECTIONMODTYPEUVW_CLASS_ID;}

		//ParamBlock params
		enum { pb_params };
		enum {	pb_name, 
				pb_holderName, pb_holderAlwaysUpdate, pb_holderCreateNew,
				pb_sourceMapChannel,
				pb_targetSameAsSource, pb_targetMapChannel, 
				pb_projectMaterialIDs, pb_sameTopology, pb_ignoreBackfacing };

		int			NumRefs() { return 1; }
		RefTargetHandle GetReference(int i) { return mpPBlock; }
		void		SetReference(int i, RefTargetHandle rtarg) { mpPBlock=(IParamBlock2*)rtarg; }

		int			NumParamBlocks() { return 1; }
		IParamBlock2* GetParamBlock(int i) { return mpPBlock; }
		IParamBlock2* GetParamBlockByID(BlockID id) { return (mpPBlock->ID() == id) ? mpPBlock : NULL; }

		RefTargetHandle Clone( RemapDir &remap );

		//	IProjectionModType
		void		SetIProjectionMod( IProjectionMod *pPMod ) { mpPMod = pPMod; }

		bool		CanProject(Tab<INode*> &tabSourceNodes, int iSelIndex, int iNodeIndex);
		void		Project(Tab<INode*> &tabSourceNodes, int iSelIndex, int iNodeIndex);

		void		SetInitialName(int iIndex);
		TCHAR*		GetName();
		void		SetName(TCHAR *name);

		void		Enable(bool enable) { mbEnabled = enable; }
		bool		IsEditing() { return mbEditing; }
		void		EndEditing() { mbEditing = false;}
		void		EnableEditing(bool enable) { mbEditing = enable; }

		void		BeginEditParams(IObjParam  *ip, ULONG flags,Animatable *prev);
		void		EndEditParams(IObjParam *ip,ULONG flags,Animatable *next);       

		IOResult	Load(ILoad *pILoad);
		IOResult	Save(ISave *pISave);

		void		ModifyObject(TimeValue t, ModContext &mc, ObjectState * os, INode *inode, IProjectionModData *pPModData);

		void		DeleteThis() { delete this; }

		//	ProjectModTypeUVW
		void		MainPanelInitDialog( HWND hWnd );
		void		MainPanelDestroy( HWND hWnd );
		void		MainPanelUpdateUI();
		void		MainPanelUpdateMapChannels();

		int			GetSourceMapChannel() { return miSourceMapChannel; };
		void		SetSourceMapChannel(int iMapChannel) { miSourceMapChannel = iMapChannel; };
		int			GetTargetMapChannel() { return miTargetMapChannel; };
		void		SetTargetMapChannel(int iMapChannel) { miTargetMapChannel = iMapChannel; };
		bool		GetProjectMaterialsIDs();
		bool		GetAlwaysUpdate();

		void		ProjectToTarget(INode *pNodeSrc, ObjectState * os, INode *pNodeTarget, BitArray *pSOSel, IProjectionModData *pPModData );
		ProjectionHolderUVW *GetProjectionHolderUVW( Object* obj );
		bool		GetSOSelData( INode *pSourceNode, int iSelIndex, BitArray &sel );
		GenFace		GetChannelFace( ObjectWrapper *object, int iFace, int iMapChannel );
		Point3*		GetChannelVert( ObjectWrapper *object, int iVert, int iMapChannel );

		bool		InitProjectionState( ProjectionState &projState, INode *pNodeSrc, ObjectState * os, INode *pNodeTarget, ProjectionHolderUVW *pMod, BitArray *pSOSel, IProjectionModData *pPModData );
		void		FillInHoldDataOptions( ProjectionHolderUVWData *pData );
		void		FillInHoldDataSameTopology( ProjectionState &projState );
		void		FillInHoldDataProjection( ProjectionState &projState );
		void		FillInHoldDataClusterProjection( ProjectionState &projState );
		int			FindMostHitFace(Tab<int> &tabFaceHits);
		void		CreateClusterData( ProjectionState &projState );
		bool		CreateClusterEdges( ProjectionState &projState );
		int			ExpandClusterFaces( ProjectionState &projState, Tab<int> &clusterFaceList, int iNewFaceIndex, BitArray &faceVisited );
		bool		CreateClusterTraverseEdges( ProjectionState &projState, int iFaceIndex, int iClusterNum, BitArray &faceVisited, Box3 &bBox);
		void		FindClosestFaceAndEdgeByCluster( ProjectionState &projState, Point3 &p3Tar, int iMainCluster, int &iClosestEdgeIndex, int &iClosestFaceIndex );
		void		ProjectToEdgeToFindUV( ProjectionState &projState, Point3 &p3Tar, int iEdgeIndex, int iFaceIndex, Point3 &UV );
		void		StoreMapFaceData( ProjectionState &projState, int iTargetFace, Tab<int> &tabFaceHits, int iCenterFaceHit, Point3 &ptCenter, Point3 &ptCenterUVW );
		int			GetUVQuadrant( ProjectionState &projState, int iCluster, Point3 &ptUVW );
};

//=============================================================================
//
//	Class ProjectionState
//
//=============================================================================
class ProjectionState 
{
public:

	ProjectionState(): objectSource(NULL), objectTarget(NULL), 
		pPInter(NULL), pData(NULL), iProjType(pt_sameTopology), pSOSel(NULL) {};

	enum { pt_sameTopology, pt_projection, pt_projectionCluster };

	int							iProjType;
	ObjectWrapper*				objectSource;
	ObjectWrapper*				objectTarget;
	IProjectionIntersector*		pPInter;
	Matrix3						mat;
	ProjectionHolderUVWData*	pData;
	Tab<int>					tabCluster;
	BitArray					bitClusterEdges;
	Tab<Point2>					tabClusterCenter;
	BitArray*					pSOSel;

	void Cleanup() {
		if( objectTarget ) {
			delete objectTarget;
			objectTarget = NULL;
		}
		if( pPInter ) {
			pPInter->DeleteThis();
			pPInter = NULL;
		}
	}
};

#endif // __PROJECTIONMODTYPEUVW__H
