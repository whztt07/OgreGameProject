

#ifndef __OgreMdxEntity_h__
#define __OgreMdxEntity_h__

#include "OgrePrerequisites.h"
#include "OgreMath.h"
#include "OgreMdxMesh.h"

namespace Ogre
{
	class OgreExport CMdxEntity
	{
	public:
		CMdxEntity(CString szName, CMdxMesh *pMesh);
		~CMdxEntity();

		void Destroy(){}
		void Update(float fElapsedTime);
		void Render();

		CMatrix GetWorldMatrix(){return m_mWorld;}
		void SetRotate(float yaw, float roll, float pitch);
	private:
		void RenderSubset(int nChunk);

		CString m_szName;
		CMdxMesh *m_pMdxMesh;
		CMatrix m_mWorld;
		CVector m_vRotate;
	};

	//class OgreExport CMdxAnim
	//{
	//public:
	//	CMdxAnim();
	//	~CMdxAnim();

	//	bool CreateMdxAnim(char *filename);
	//	void SetRotate(float yaw, float roll, float pitch);
	//	void SetCurAnimId(int nId);
	//	void SetCurAnimFrame(int nFrame);
	//	int GetAnimNum();
	//	void SetSingleFrame(bool bSingleFrame);
	//	CMdxMesh* GetMdxMesh();
	//	CVector GetRotate();
	//private:
	//	void UpdateSubset(int nSubset);
	//	void RenderSubset(int nSubset);

	//	CMdxMesh *m_pMdx;
	//	bool m_bSingleFrame;
	//	int m_nCurAnimId;
	//	int m_nCurAnimFrame;
	//	CMatrix m_matWorld;
	//	CVector m_vRotate;
	//};
}

#endif