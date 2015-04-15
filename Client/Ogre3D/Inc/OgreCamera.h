
#ifndef __OgreCamera_h__
#define __OgreCamera_h__

#include "OgrePrerequisites.h"
#include "OgreUseful.h"
#include "OgreMath.h"

namespace Ogre
{
	class OgreExport CCamera
	{
	public:
		CCamera();
		~CCamera();

		void SetViewMatrix(CMatrix *pmView);
		void SetProjMatrix(CMatrix *pmProj);
		void SetViewParam(CVector eye, CVector lookat, CVector up);
		void SetProjParam(float fFovy, float fAspect, float fNearDist, float fFarDist);
		CVector GetEye(){return m_vEye;}
		CVector GetLookat(){return m_vLookat;}
		CVector GetViewDir(){return m_vViewDir;}
		float GetProjFovy(){return m_fProjFovy;}
		float GetProjAspect(){return m_fProjAspect;}
		float GetProjNearDist(){return m_fProjNearDist;}
		float GetProjFarDist(){return m_fProjFarDist;}
		CMatrix GetViewMatrix(){return m_mView;}
		CMatrix GetProjMatrix(){return m_mProj;}

		void MoveForward(float Distance);
		void MoveUpward(float Distance);
		void Strafe(float Distance);
		void Yaw(float fAngle);
		void Pitch(float fAngle);
	private:
		CMatrix m_mView;
		CMatrix m_mProj;
		CVector m_vEye;
		CVector m_vLookat;
		CVector m_vRightVec;
		CVector m_vViewDir;
		CVector m_vUpVec;
		float m_fProjFovy;
		float m_fProjAspect;
		float m_fProjNearDist;
		float m_fProjFarDist;
	};
}

#endif