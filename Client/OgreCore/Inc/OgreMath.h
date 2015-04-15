

#ifndef __OgreMath_h__
#define __OgreMath_h__

#include "OgrePrerequisites.h"
#include <d3d9.h>
#include <d3dx9.h>

namespace Ogre
{
	class CMatrix;

	class OgreExport CVector
	{
	public:
		CVector(float x, float y, float z);
		CVector(const CVector& v);
		CVector();

		void Normalize();
		float Length();
		float Dot(const CVector& vec) const;
		CVector Cross( const CVector& rkVector ) const;

		CVector& operator -();
		CVector operator + (const CVector &v) const;
		CVector operator - (const CVector &v) const;
		CVector& operator = (const CVector &v);
		CVector& operator += (const CVector &v);
		CVector& operator -= (const CVector &v);
		CVector operator * (float d) const;
		CVector operator / (float d) const;
		operator D3DXVECTOR3();

		float x;
		float y;
		float z;
	};

	class OgreExport CVector2D
	{
	public:
		CVector2D();
		CVector2D(float x, float y);

		float x;
		float y;
	};

	class OgreExport CVector4
	{
	public:
		CVector4();

		float x;
		float y;
		float z;
		float w;
	};

	class OgreExport CQuaternion
	{
	public:
		CQuaternion(float x, float y, float z, float w);
		
		const CQuaternion operator+ (const CQuaternion &v) const;
		const CQuaternion operator * (float d) const;

		float x;
		float y;
		float z;
		float w;
	};

	class OgreExport CMatrix
	{
	public:
		CMatrix();
		CMatrix(
			float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33 );

		operator D3DXMATRIX();
		CMatrix Inverse();
		CMatrix operator * (CMatrix &m2);
		CMatrix operator *= (CMatrix m2);
		CVector operator * (const CVector& v) const;

		void Translation(const CVector& tr);
		void Scale(const CVector& sc);
		void QuaternionRotate(const CQuaternion& q);
		void Transpose();

		void Unit();
		void Zero();

		static CMatrix* RotationAxis(CMatrix *pOut, const CVector *vec3, float angle);
		static const CMatrix NewTranslation(const CVector& tr);
		static const CMatrix NewQuatRotate(const CQuaternion& qr);
		static const CMatrix NewScale(const CVector& sc);
	public:
		union
		{
			struct
			{
				float _11, _12, _13, _14;
				float _21, _22, _23, _24;
				float _31, _32, _33, _34;
				float _41, _42, _43, _44;
			};
			float m[4][4];
		};
	};

	OgreExport void Quat2Mat( CQuaternion& q, CMatrix& mat );
	OgreExport void MakeMatrix( CVector& v, CQuaternion& q, CMatrix& m );
	OgreExport CMatrix* RotationAxis(CMatrix *pOut, const CVector *vec3, float angle);
	OgreExport CVector* TransformNormal(CVector *pOut, const CVector *vec3, const CMatrix *mat);
}

#endif