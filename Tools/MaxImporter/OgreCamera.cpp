

#include "OgreCamera.h"
#include <d3dx9.h>

namespace Ogre
{
	CCamera::CCamera()
	{
		m_vEye = CVector(0.0f, -1.0f, 1.0f);
		
		m_vViewDir = CVector(0.0f, 1.0f, 0.0f);
		m_vRightVec = CVector(1.0f, 0.0f, 0.0f);
		m_vUpVec = CVector(0.0f, 0.0f, 1.0f);
	}

	CCamera::~CCamera()
	{

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
	}

	CVector CCamera::GetEye()
	{
		return m_vEye;
	}

	CVector CCamera::GetViewDir()
	{
		return m_vViewDir;
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
	}

	void CCamera::Pitch(float fAngle)
	{

	}
}