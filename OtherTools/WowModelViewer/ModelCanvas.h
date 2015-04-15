

#ifndef __ModelCanvas_h__
#define __ModelCanvas_h__

#include <QString>
#include <QWidget>
#include <QMouseEvent>
#include <QTimer>
#include <string>
#include "OgreMath.h"
#include "mpq_libmpq.h"
#include <OgreD3D9RenderSystem.h>
#include "enums.h"
#include "animated.h"

typedef unsigned char uint8;
typedef char int8;
typedef unsigned __int16 uint16;
typedef __int16 int16;
typedef unsigned __int32 uint32;
typedef __int32 int32;

class CModel;
class CModelCanvas;

struct CModelHeader 
{
	char id[4];
	uint8 version[4];
	uint32 nameLength;
	uint32 nameOfs;
	uint32 type;

	uint32 nGlobalSequences;
	uint32 ofsGlobalSequences;
	uint32 nAnimations;
	uint32 ofsAnimations;
	uint32 nC;
	uint32 ofsC;
	uint32 nD;
	uint32 ofsD;
	uint32 nBones;
	uint32 ofsBones;
	uint32 nF;
	uint32 ofsF;

	uint32 nVertices;
	uint32 ofsVertices;
	uint32 nViews;
	uint32 ofsViews;

	uint32 nColors;
	uint32 ofsColors;

	uint32 nTextures;
	uint32 ofsTextures;

	uint32 nTransparency; // H
	uint32 ofsTransparency;
	uint32 nI;   // always unused ?
	uint32 ofsI;
	uint32 nTexAnims;	// J
	uint32 ofsTexAnims;
	uint32 nTexReplace;
	uint32 ofsTexReplace;

	uint32 nTexFlags;
	uint32 ofsTexFlags;
	uint32 nY;
	uint32 ofsY;

	uint32 nTexLookup;
	uint32 ofsTexLookup;

	uint32 nTexUnitLookup;		// L
	uint32 ofsTexUnitLookup;
	uint32 nTransparencyLookup; // M
	uint32 ofsTransparencyLookup;
	uint32 nTexAnimLookup;
	uint32 ofsTexAnimLookup;

	float floats[14];

	uint32 nBoundingTriangles;
	uint32 ofsBoundingTriangles;
	uint32 nBoundingVertices;
	uint32 ofsBoundingVertices;
	uint32 nBoundingNormals;
	uint32 ofsBoundingNormals;

	uint32 nAttachments; // O
	uint32 ofsAttachments;
	uint32 nAttachLookup; // P
	uint32 ofsAttachLookup;
	uint32 nQ; // Q
	uint32 ofsQ;
	uint32 nLights; // R
	uint32 ofsLights;
	uint32 nCameras; // S
	uint32 ofsCameras;
	uint32 nT;
	uint32 ofsT;
	uint32 nRibbonEmitters; // U
	uint32 ofsRibbonEmitters;
	uint32 nParticleEmitters; // V
	uint32 ofsParticleEmitters;
};

struct CModelTextureDef 
{
	uint32 type;
	uint32 flags;
	uint32 nameLen;
	uint32 nameOfs;
};

struct CCharModelDetails 
{	
	void Reset()
	{
	closeRHand = false;
	closeLHand = false;
	isChar = false;
	isMounted = false;
	}

	bool closeRHand;
	bool closeLHand;
	bool isChar;
	bool isMounted;
};

struct CModelVertex 
{
	Ogre::CVector pos;
	uint8 weights[4];
	uint8 bones[4];
	Ogre::CVector normal;
	Ogre::CVector2D texcoords;
	int unk1, unk2; // always 0,0 so this is probably unused
};

struct CAnimationBlock 
{
	int16 type;		// interpolation type (0=none, 1=linear, 2=hermite)
	int16 seq;		// global sequence id or -1
	uint32 nRanges;
	uint32 ofsRanges;
	uint32 nTimes;
	uint32 ofsTimes;
	uint32 nKeys;
	uint32 ofsKeys;
};

struct CModelTransDef 
{
	CAnimationBlock trans;
};

struct CModelTransparency 
{
	AnimatedShort trans;

	void Init(CMPQFile &f, CModelTransDef &mtd, int *global);
};

// block G - color defs
struct CModelColorDef 
{
	CAnimationBlock color;
	CAnimationBlock opacity;
};

struct CModelColor 
{
	Animated<Ogre::CVector> color;
	AnimatedShort opacity;

	void Init(CMPQFile &f, CModelColorDef &mcd, int *global);
};

struct CModelDrawVertex
{
	Ogre::CVector pos;
	Ogre::CVector normal;
	Ogre::CVector2D tex;
	uint8 weight[4];
	uint8 index[4];
};

struct CModelGeoset 
{
	uint16 id;		// mesh part id?
	uint16 d2;		// ?
	uint16 vstart;	// first vertex
	uint16 vcount;	// num vertices
	uint16 istart;	// first index
	uint16 icount;	// num indices
	uint16 d3;		// number of bone indices
	uint16 d4;		// ? always 1 to 4
	uint16 d5;		// ?
	uint16 d6;		// root bone?
	Ogre::CVector v;
	float unknown[4];	// Added in WoW 2.0?
};

struct CModelTexUnit
{
	uint16 flags;		// Flags
	uint16 order;		// ?
	uint16 op;			// Material this texture is part of (index into mat)
	uint16 op2;			// Always same as above?
	int16 colorIndex;	// color or -1
	uint16 flagsIndex;	// more flags...
	uint16 texunit;		// Texture unit (0 or 1)
	uint16 d4;			// ? (seems to be always 1)
	uint16 textureid;	// Texture id (index into global texture list)
	uint16 texunit2;	// copy of texture unit value?
	uint16 transid;		// transparency id (index into transparency list)
	uint16 texanimid;	// texture animation id
};

// block X - render flags
struct CModelRenderFlags 
{
	uint16 flags;
	//unsigned char f1;
	//unsigned char f2;
	uint16 blend;
};

struct CModelRenderPass 
{
	CModelRenderPass()
	{
		m_bRender = true;
		m_bHasSpecular = false;
		m_nSpecularPassID = -1;
	}

	uint32 indexStart;
	uint32 indexCount;
	uint32 vertexStart;
	uint32 vertexEnd;
	int tex;

	int16 opacity;
	int16 color;
	bool noZWrite;
	bool useEnvMap;
	int geoset;
	uint16 order;
	int16 blendmode;
	float p;

	bool m_bRender;
	bool m_bHasSpecular;
	int m_nSpecularPassID;

	bool Setup(CModel *pModel);
	bool IsRender(CModel *pModel);

	bool operator< (const CModelRenderPass &m) const
	{
		// This is the old sort order method which I'm pretty sure is wrong - need to try something else.
		//return !trans;
		if (order < m.order)
			return true;
		else if (order > m.order)
			return false;
		else
			return blendmode == m.blendmode ? (p < m.p) : (blendmode < m.blendmode);
	}
};

struct CModelView 
{
	uint32 nIndex, ofsIndex; // Vertices in this model (index into vertices[])
	uint32 nTris, ofsTris;	 // indices
	uint32 nProps, ofsProps; // additional vtx properties
	uint32 nSub, ofsSub;	 // materials/renderops/submeshes
	uint32 nTex, ofsTex;	 // material properties/textures
	int32 lod;				 // LOD bias?
};

// block E - bones
struct CModelBoneDef 
{
	int32 animid;
	int32 flags;
	int16 parent; // parent bone index
	int16 geoid;
	// new int added to the bone definitions.  Added in WoW 2.0
	int32 unknown;
	CAnimationBlock translation;
	CAnimationBlock rotation;
	CAnimationBlock scaling;
	Ogre::CVector pivot;
};

// block B - animations
struct CModelAnimation 
{
	uint32 animID;
	uint32 timeStart;
	uint32 timeEnd;
	float moveSpeed;
	uint32 loopType;
	uint32 flags;
	uint32 d1;
	uint32 d2;
	uint32 playSpeed;  // note: this can't be play speed because it's 0 for some models
	Ogre::CVector boxA, boxB;
	float rad;
	int16 s[2];
};

struct PACK_QUATERNION 
{  
	__int16 x,y,z,w;  
};

enum
{
	COMPRESSED_DXT1,
	COMPRESSED_DXT3,
	COMPRESSED_DXT5,
	UNCOMPRESSED_RGBA8,
};

class CMpqTexture
{
public:
	CMpqTexture(std::string name);
	~CMpqTexture();

	std::string m_strName;
	int m_nWidth;
	int m_nHeight;
	bool m_bCompressed;
};

class CMpqTextureManager
{
public:
	CMpqTextureManager();
	~CMpqTextureManager();

	int Add(std::string name);
	void Del(int nTexID);
	LPDIRECT3DTEXTURE9 FindD3D9Texture(int nID);
	CMpqTexture* FindTexture(int nID);
	void GetPixel(unsigned char *buf, int nTexID);
	void ComposePixel(unsigned char *buf, int width, int height, int& nTexID);
private:
	bool LoadBLP(CMpqTexture *tex);
	void CreateD3D9Texture(int width, int height, int numMip, int format, std::vector<int> , std::vector<unsigned char*> vecBuf);
	void CreateD3D9Texture2(int width, int height, int numMip, int format, std::vector<int> , std::vector<unsigned int*> vecBuf);

	typedef std::map<std::string, int> NameMap;
	typedef std::map<int, LPDIRECT3DTEXTURE9> TextureIDMap;
	typedef std::map<int, CMpqTexture*> TextureMap;
	NameMap m_mapName;
	TextureIDMap m_mapTextureID;
	TextureMap m_mapTexture;
	int nIDCount;
};

class CAttachment
{
public:
	CAttachment();
	CAttachment(CAttachment *pParent, CModel *pModel, int id, int slot);
	~CAttachment();

	void Clear();
	void Setup();
	void Draw(CModelCanvas *canvas);
	CAttachment* AddChild(CModel *pModel, int id = -1, int slot = -1);
	CAttachment* AddChild(const char *modelfn, int id, int slot, int elseParam);
	void DelSlot(int slot);

	CAttachment *m_pParent;
	CModel *m_pModel;
	Ogre::CMatrix m_matAtt;
	int m_nId;
	int m_nSlot;
	
	typedef std::vector<CAttachment*> AttachmentList;
	AttachmentList m_vecChildren;
};

class CModelAttachment;
class CBone
{
	friend class CModel;
	friend class CModelAttachment;
public:
	CBone();
	~CBone();

	void Init(CMPQFile &f, CModelBoneDef &b, int *global);
	void CalcMatrix(CBone* allbones, int anim, int time, bool rotate=true);
public:
	Ogre::CVector GetTrans(unsigned int anim, unsigned int time);
	Ogre::CVector GetScale(unsigned int anim, unsigned int time);
	Ogre::CQuaternion GetQuat(unsigned int anim, unsigned int time);

	CModelBoneDef m_BoneDef;
	Ogre::CVector m_vPivot;
	Ogre::CMatrix m_matBone;

	typedef std::vector<Ogre::CVector> TranslationList;
	typedef std::vector<Ogre::CQuaternion> QuatList;
	typedef std::vector<Ogre::CVector> ScaleList;
	typedef std::pair<size_t, size_t> AnimRange;
	TranslationList m_vecTrans;
	QuatList m_vecQuat;
	ScaleList m_vecScale;

	std::vector<uint32> m_vecTransTime;
	std::vector<uint32> m_vecScaleTime;
	std::vector<uint32> m_vecQuatTime;
	std::vector<AnimRange> m_vecTransRange;
	std::vector<AnimRange> m_vecScaleRange;
	std::vector<AnimRange> m_vecQuatRange;
	int m_nTransType;
	int m_nScaleType;
	int m_nQuatType;

	int m_nTransSeq;
	int m_nScaleSeq;
	int m_nQuatSeq;
	int *m_pnGlobals;

	int m_nParent;
	bool m_bUsedTrans;
	bool m_bUsedScale;
	bool m_bUsedQuat;
	bool m_bCalc;
};

class CAnimManager
{
public:
	CAnimManager(CModelAnimation *anim);
	~CAnimManager();

	int GetFrame();
private:
	CModelAnimation *m_pAnim;
	int m_nFrame;
};

struct CTextureGroup 
{
	static const int num = 3;
	int base, count;
	std::string tex[num];
	
	CTextureGroup()
	{
		for (int i=0; i<num; i++) 
		{
			tex[i] = "";
		}
	}

	// default copy constr
	CTextureGroup(const CTextureGroup &grp)
	{
		for (int i=0; i<num; i++) 
		{
			tex[i] = grp.tex[i];
		}

		base = grp.base;
		count = grp.count;
	}

	const bool operator < (const CTextureGroup &grp) const
	{
		for (int i = 0; i < num; i++) 
		{
			if (tex[i] < grp.tex[i]) 
				return true;
		}

		return false;
	}
};
typedef std::set<CTextureGroup> TextureSet;

struct CModelAttachmentDef 
{
	int32 id;
	int32 bone;
	Ogre::CVector pos;
	CAnimationBlock unk;
};

struct CModelAttachment
{
	int id;
	Ogre::CVector pos;
	int bone;
	CModel *model;
	Ogre::CMatrix m_matAtt;

	void init(CMPQFile &f, CModelAttachmentDef &mad, int *global);
	void setup();
};

class CModel
{
	friend class CModelRenderPass;
	friend class CAnimControlWidget;
	friend class CCharControlWidget;
	friend class CWowModelViewer;
	friend class CModelAttachment;
public:
	CModel(std::string name, bool forceAnim = false);
	~CModel();

	void Draw(Ogre::CMatrix &matAtt);
	float GetRadius();
	Ogre::CMatrix SetupAtt(int id);

	bool m_bOK;
	bool m_bForceAnim;
public:
	void CreateD3D9Res();
	void InitStatic(CMPQFile &f);
	void InitAnimated(CMPQFile &f);
	void InitCommon(CMPQFile &f);
	bool IsAnimated(CMPQFile &f);
	void DrawModel();
	int GetTextureID(int nChunk);
	int GetOutputTexID(int nChunk);
	void Animate(int anim, Ogre::CMatrix &matAtt);
	void CalcBones(int anim, int time, Ogre::CMatrix &matAtt);

	std::string m_strName;

	LPDIRECT3DVERTEXBUFFER9 m_pVertexBuffer;
	LPDIRECT3DINDEXBUFFER9 m_pIndexBuffer;
	LPDIRECT3DVERTEXDECLARATION9 m_pVertexDecl;

	typedef std::vector<CModelRenderPass> PassList;
	typedef std::vector<std::string> AnimNameList;
	PassList m_vecPass;
	PassList m_vecOutputPass;
	AnimNameList m_vecAnimName;

	CModelHeader m_ModelHeader;
	CCharModelDetails m_CharModelDetails;
	CModelVertex *m_pOrigVertices;
	Ogre::CVector *m_pVertices;
	Ogre::CVector *m_pNormals;
	Ogre::CVector2D *m_pTexcoord;
	uint16 *m_pIndices;
	int *m_pTextureID;
	
	CAnimManager *m_pAnimManager;
	CModelAnimation *m_pAnim;
	int	*m_pnGlobalSequences;
	int m_BoneLookup[BONE_ROOT + 1];
	CBone *m_pBone;
	
	float m_fRadius;
	int m_nNumVertex;
	int m_nNumIndex;
	int m_nNumTexture;
	bool m_bAnimated;
	bool m_bAnimBones;
	bool m_bAnimGeometry;
	int m_nCurrentAnim;

	int m_nSpecialTextures[32];
	int m_nReplaceTextures[32];
	bool m_bUseReplaceTextures[32];
	CModelTransparency *m_pTransparency;
	CModelColor *m_pColors;
	DWORD dwLastTime;
	int m_nFrame;

	std::vector<CModelAttachment> atts;
	int attLookup[40];

	std::vector<CModelGeoset> m_vecGeoset;
	bool *m_pbShowGeosets;
	ModelType m_nModelType;
	bool m_bChar;
};

class CModelCanvas
{
	friend class CAnimControlWidget;
	friend class CWowModelViewer;
public:
	CModelCanvas();
	~CModelCanvas();

	CAttachment* LoadCharModel(QString path);
	void LoadModel(QString path);
	void SetRotate(float fYaw, float fPitch);
	void Render();
private:
	void RenderObjects();

	CModel *m_pModel;
	CAttachment *m_pRoot;
	CAttachment *m_pCurAtt;
	Ogre::CVector m_vRotate;
};

class CD3D9Widget : public QWidget
{
	Q_OBJECT
public:
	CD3D9Widget( QWidget *parent = 0, Qt::WFlags flags = 0 );
	virtual ~CD3D9Widget();

	void SetModelCanvas(CModelCanvas *canvas);
	void InitD3D9Device();
	void SetViewport();
	void BeginScene();
	void EndScene();
public slots:
	void Idle();
protected:
	QPaintEngine *paintEngine() const { return 0; }
	virtual void paintEvent( QPaintEvent *paintE );
	virtual void resizeEvent( QResizeEvent *event );
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void wheelEvent(QWheelEvent *event);
	void Render();

	IDirect3D9 *m_pD3D;
	IDirect3DDevice9 *m_pDevice;
	D3DPRESENT_PARAMETERS m_D3Dpp;
	CModelCanvas *m_pModelCanvas;
	QTimer	m_Timer;
	bool m_bMidBtnPressed;
	bool m_bLeftBtnPressed;
	bool m_bRightBtnPressed;
	QPoint m_ptPreMouse;
	QPoint m_ptCurMouse;
};

#endif