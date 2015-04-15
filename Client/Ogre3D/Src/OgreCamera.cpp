

#include "OgreCamera.h"
#include <d3dx9.h>

namespace Ogre
{
	CCamera::CCamera()
	{
		m_vEye = CVector(0.0f, 1.0f, -1.0f);
		m_vViewDir = m_vLookat - m_vEye;
		m_vRightVec = CVector(1.0f, 0.0f, 0.0f);
		m_vUpVec = CVector(0.0f, 1.0f, 0.0f);
		m_vViewDir.Normalize();
		m_fProjFovy = 1.0f;
		m_fProjAspect = 1.0f;
		m_fProjNearDist = 0.1f;
		m_fProjFarDist = 100.0f;
	}

	CCamera::~CCamera()
	{

	}

	void CCamera::SetViewMatrix(CMatrix *pmView)
	{
		m_mView = *pmView;
	}

	void CCamera::SetProjMatrix(CMatrix *pmProj)
	{
		m_mProj = *pmProj;
	}

	void CCamera::SetViewParam(CVector eye, CVector lookat, CVector up)
	{
		m_vEye = eye;
		m_vViewDir = lookat - eye;
		m_vViewDir.Normalize();
		m_vUpVec = up;
		m_vUpVec.Normalize();
		m_vRightVec = m_vUpVec.Cross(m_vViewDir);
		m_vRightVec.Normalize();

		m_vUpVec = m_vViewDir.Cross(m_vRightVec);
		m_vUpVec.Normalize();
	}

	void CCamera::SetProjParam(float fFovy, float fAspect, float fNearDist, float fFarDist)
	{
		m_fProjFovy = fFovy;
		m_fProjAspect = fAspect;
		m_fProjNearDist = fNearDist;
		m_fProjFarDist = fFarDist;
	}

	void CCamera::MoveForward(float Distance)
	{
		m_vEye += (m_vViewDir * Distance);
		m_vLookat += (m_vViewDir * Distance);
	}

	void CCamera::MoveUpward(float Distance)
	{
		m_vEye += (m_vUpVec * Distance);
		m_vLookat += (m_vUpVec * Distance);
	}

	void CCamera::Strafe(float Distance)
	{
		m_vEye += (m_vRightVec * Distance);
		m_vLookat += (m_vRightVec * Distance);
	}

	void CCamera::Yaw(float fAngle)
	{
		CMatrix matRot;
		RotationAxis(&matRot, &Ogre::CVector(0.0f, 0.0f, 1.0f), fAngle);

		TransformNormal(&m_vRightVec, &m_vRightVec, &matRot);
		TransformNormal(&m_vViewDir, &m_vViewDir, &matRot);
		m_vRightVec.Normalize();
		m_vViewDir.Normalize();

		m_vUpVec = m_vViewDir.Cross(m_vRightVec);
		m_vUpVec.Normalize();
	}

	void CCamera::Pitch(float fAngle)
	{
		CMatrix matRot;
		RotationAxis(&matRot, &m_vRightVec, fAngle);

		TransformNormal(&m_vUpVec, &m_vUpVec, &matRot);
		TransformNormal(&m_vViewDir, &m_vViewDir, &matRot);
		m_vUpVec.Normalize();
		m_vViewDir.Normalize();
	}
}