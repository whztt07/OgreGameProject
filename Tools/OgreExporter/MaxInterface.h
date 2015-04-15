#pragma once

#include "max.h"
#include "cs/bipexp.h"
#include "cs/phyexp.h"
#include "iparamb2.h"
#include "iskin.h"
#include "stdmat.h"
#include <string>
#include <vector>
#include "OgreMath.h"
#include "OgreDataChunkWrite.h"
using namespace Ogre;

enum
{
	MODIFIER_NONE,
	MODIFIER_SKIN,
	MODIFIER_PHYSIQUE
};

class CMaxInterface
{
public:
	CMaxInterface(void);
	~CMaxInterface(void);

	struct CMdxColorKey
	{
		float	dr, dg, db;	// diffuse color
		float	ar, ag, ab;	// ambiant color
		float	sr, sg, sb;	// specular color
		float	alpha;		// alpha
		float	shinstr;		// shiness
		float	selfillum;	// self illumination
		float	uoffset, voffset; // uv offset...
	};

	struct CColorTrack
	{
		CColorTrack()
		{
			bTiling = FALSE;
			nUTile = 1;
			nVTile = 1;
		}

		int bTiling;		// ÊÇ·ñÓÐtile;
		int nUTile, nVTile;	// u,v¿éÊý
		std::vector<CMdxColorKey> vectorColorKey;
	};

	struct CInfluence
	{
		CInfluence()
		{
			szBoneName[0] = 0;
			nBoneId = -1;
			fWeight = 0.0f;
		}

		char szBoneName[64];
		int nBoneId;
		float fWeight;
	};

	struct CBoneGroup
	{
		void AddInfluence( CInfluence& infl );
		std::vector<CInfluence> vectorInfl;
	};

	struct CTrack
	{
		std::vector<CMatrix> vectorMatrix;
		std::vector<BOOL> vectorVisible;
		BOOL bMirror;
	};

	void Create(ExpInterface *pExpInterface, Interface *pInterface);
	INode* GetSelectedNode();
	INode *GetRootNode();
	INode* GetNode( int i );
	Mesh* GetMesh( INode* pNode );
	Modifier* FindModifier(INode *pINode, Class_ID id);
	bool GetMtlAnim( StdMat* pStdMtl, CColorTrack& track );
	bool IsBone(INode *pNode);
	bool IsBipedBone(INode *pNode);
	bool IsMesh(INode *pNode);
	bool IsRenderable( INode* node );
	bool IsDummy(INode *pNode);
	void GetNodeTree( INode* pNode );
	void ClearNodeTree();
	int GetNodeCount();
	int GetFps();
	int GetStartTick();
	int GetEndTick();
	int GetFrameCount();
	int GetTickPerFrame();
	void StartProgressInfo(const std::string& strText);
	void SetProgressInfo(int percentage);
	void StopProgressInfo();
	bool GetStdMtlChannelBitmapFileName(StdMat* pStdMat, int nChannel, char *szFileName);
	void GetTracks( int nNodeCount, INode** pNodes, CTrack** tracks, BOOL bOnlyFirstFrame = FALSE );
	void GetBoneGroup( Modifier *pModifier, int nModifierType, INode* pNode, 
		Mesh* pMesh, int nVertexId, CMaxInterface::CBoneGroup& boneGroup );
	static DWORD WINAPI ProgressFunction(LPVOID arg);
protected:
	ExpInterface *m_pExpInterface;
	Interface *m_pInterface;
	std::vector<INode*> m_vecNode;
};

CMaxInterface* GetMaxIP();