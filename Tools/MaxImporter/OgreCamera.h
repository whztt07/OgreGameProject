

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

		void SetViewParam(CVector eye, CVector lookat, CVector up);
		CVector GetEye();
		CVector GetViewDir();

		void MoveForward(float Distance);
		void MoveUpward(float Distance);
		void Strafe(float Distance);

		void Yaw(float fAngle);
		void Pitch(float fAngle);
	private:
		CVector m_vEye;
		CVector m_vLookat;

		CVector m_vRightVec;
		CVector m_vViewDir;
		CVector m_vUpVec;
	};
}