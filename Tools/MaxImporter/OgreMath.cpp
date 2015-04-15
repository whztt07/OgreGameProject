

#include "OgreMath.h"

namespace Ogre
{
	//--------------------------------
	//-----------CVector--------------
	//--------------------------------
	CVector::CVector()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}

	CVector::CVector(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	CVector::CVector(const CVector& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}

	void CVector::Normalize()
	{
		float fFactor = 1.0f / Length();
		x *= fFactor;
		y *= fFactor;
		z *= fFactor;
	}

	float CVector::Length()
	{
		return sqrtf(x * x + y * y + z * z);
	}

	float CVector::Dot(const CVector& vec) const
	{
		return x * vec.x + y * vec.y + z * vec.z;
	}

	CVector CVector::Cross( const CVector& rkVector ) const
	{
		return CVector(
			y * rkVector.z - z * rkVector.y,
			z * rkVector.x - x * rkVector.z,
			x * rkVector.y - y * rkVector.x);
	}

	CVector& CVector::operator -()
	{
		x = -x;
		y = -y;
		z = -z;
		return *this;
	}

	CVector CVector::operator + (const CVector &v) const
	{
		CVector r(x + v.x, y + v.y, z + v.z);
		return r;
	}

	CVector CVector::operator - (const CVector &v) const
	{
		CVector r(x - v.x, y - v.y, z - v.z);
		return r;
	}

	CVector& CVector::operator = (const CVector &v) 
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	CVector& CVector::operator += (const CVector &v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	CVector& CVector::operator -= (const CVector &v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	CVector CVector::operator * (float d) const
	{
		CVector r(x * d, y * d, z * d);
		return r;
	}

	CVector::operator D3DXVECTOR3()
	{
		D3DXVECTOR3 v;
		v.x = x;
		v.y = y;
		v.z = z;
		return v;
	}

	//--------------------------------
	//-----------CVector2D------------
	//--------------------------------
	CVector2D::CVector2D()
	{
		x = 0.0f;
		y = 0.0f;
	}

	//--------------------------------
	//-----------CVector4-------------
	//--------------------------------
	CVector4::CVector4()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 0.0f;
	}

	//--------------------------------
	//---------CQuaternion------------
	//--------------------------------
	CQuaternion::CQuaternion(float x, float y, float z, float w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	const CQuaternion CQuaternion::operator * (float d) const
	{
		CQuaternion r(x * d, y * d, z * d, w * d);
		return r;
	}

	const CQuaternion CQuaternion::operator + (const CQuaternion &v) const
	{
		CQuaternion r(x + v.x, y + v.y, z + v.z, w + v.w);
		return r;
	}

	//--------------------------------
	//-----------CMatrix--------------
	//--------------------------------
	CMatrix::CMatrix()
	{

	}

	CMatrix::CMatrix(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33 )
	{
		m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
		m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
		m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
		m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
	}

	CMatrix CMatrix::Inverse()
	{
		float m00 = m[0][0], m01 = m[0][1], m02 = m[0][2], m03 = m[0][3];
		float m10 = m[1][0], m11 = m[1][1], m12 = m[1][2], m13 = m[1][3];
		float m20 = m[2][0], m21 = m[2][1], m22 = m[2][2], m23 = m[2][3];
		float m30 = m[3][0], m31 = m[3][1], m32 = m[3][2], m33 = m[3][3];

		float v0 = m20 * m31 - m21 * m30;
		float v1 = m20 * m32 - m22 * m30;
		float v2 = m20 * m33 - m23 * m30;
		float v3 = m21 * m32 - m22 * m31;
		float v4 = m21 * m33 - m23 * m31;
		float v5 = m22 * m33 - m23 * m32;

		float t00 = + (v5 * m11 - v4 * m12 + v3 * m13);
		float t10 = - (v5 * m10 - v2 * m12 + v1 * m13);
		float t20 = + (v4 * m10 - v2 * m11 + v0 * m13);
		float t30 = - (v3 * m10 - v1 * m11 + v0 * m12);

		float invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

		float d00 = t00 * invDet;
		float d10 = t10 * invDet;
		float d20 = t20 * invDet;
		float d30 = t30 * invDet;

		float d01 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		float d11 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		float d21 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		float d31 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		v0 = m10 * m31 - m11 * m30;
		v1 = m10 * m32 - m12 * m30;
		v2 = m10 * m33 - m13 * m30;
		v3 = m11 * m32 - m12 * m31;
		v4 = m11 * m33 - m13 * m31;
		v5 = m12 * m33 - m13 * m32;

		float d02 = + (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		float d12 = - (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		float d22 = + (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		float d32 = - (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		v0 = m21 * m10 - m20 * m11;
		v1 = m22 * m10 - m20 * m12;
		v2 = m23 * m10 - m20 * m13;
		v3 = m22 * m11 - m21 * m12;
		v4 = m23 * m11 - m21 * m13;
		v5 = m23 * m12 - m22 * m13;

		float d03 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		float d13 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		float d23 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		float d33 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		return CMatrix(
			d00, d01, d02, d03,
			d10, d11, d12, d13,
			d20, d21, d22, d23,
			d30, d31, d32, d33);
	}

	CMatrix CMatrix::operator * (CMatrix &m2)
	{
		CMatrix r;
		r.m[0][0] = m[0][0] * m2.m[0][0] + m[0][1] * m2.m[1][0] + m[0][2] * m2.m[2][0] + m[0][3] * m2.m[3][0];
		r.m[0][1] = m[0][0] * m2.m[0][1] + m[0][1] * m2.m[1][1] + m[0][2] * m2.m[2][1] + m[0][3] * m2.m[3][1];
		r.m[0][2] = m[0][0] * m2.m[0][2] + m[0][1] * m2.m[1][2] + m[0][2] * m2.m[2][2] + m[0][3] * m2.m[3][2];
		r.m[0][3] = m[0][0] * m2.m[0][3] + m[0][1] * m2.m[1][3] + m[0][2] * m2.m[2][3] + m[0][3] * m2.m[3][3];

		r.m[1][0] = m[1][0] * m2.m[0][0] + m[1][1] * m2.m[1][0] + m[1][2] * m2.m[2][0] + m[1][3] * m2.m[3][0];
		r.m[1][1] = m[1][0] * m2.m[0][1] + m[1][1] * m2.m[1][1] + m[1][2] * m2.m[2][1] + m[1][3] * m2.m[3][1];
		r.m[1][2] = m[1][0] * m2.m[0][2] + m[1][1] * m2.m[1][2] + m[1][2] * m2.m[2][2] + m[1][3] * m2.m[3][2];
		r.m[1][3] = m[1][0] * m2.m[0][3] + m[1][1] * m2.m[1][3] + m[1][2] * m2.m[2][3] + m[1][3] * m2.m[3][3];

		r.m[2][0] = m[2][0] * m2.m[0][0] + m[2][1] * m2.m[1][0] + m[2][2] * m2.m[2][0] + m[2][3] * m2.m[3][0];
		r.m[2][1] = m[2][0] * m2.m[0][1] + m[2][1] * m2.m[1][1] + m[2][2] * m2.m[2][1] + m[2][3] * m2.m[3][1];
		r.m[2][2] = m[2][0] * m2.m[0][2] + m[2][1] * m2.m[1][2] + m[2][2] * m2.m[2][2] + m[2][3] * m2.m[3][2];
		r.m[2][3] = m[2][0] * m2.m[0][3] + m[2][1] * m2.m[1][3] + m[2][2] * m2.m[2][3] + m[2][3] * m2.m[3][3];

		r.m[3][0] = m[3][0] * m2.m[0][0] + m[3][1] * m2.m[1][0] + m[3][2] * m2.m[2][0] + m[3][3] * m2.m[3][0];
		r.m[3][1] = m[3][0] * m2.m[0][1] + m[3][1] * m2.m[1][1] + m[3][2] * m2.m[2][1] + m[3][3] * m2.m[3][1];
		r.m[3][2] = m[3][0] * m2.m[0][2] + m[3][1] * m2.m[1][2] + m[3][2] * m2.m[2][2] + m[3][3] * m2.m[3][2];
		r.m[3][3] = m[3][0] * m2.m[0][3] + m[3][1] * m2.m[1][3] + m[3][2] * m2.m[2][3] + m[3][3] * m2.m[3][3];

		return r;
	}

	CMatrix CMatrix::operator *= (CMatrix m2)
	{
		return *this = this->operator * (m2);
	}

	CVector CMatrix::operator * (const CVector& v) const
	{
		CVector o;
		o.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3];
		o.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3];
		o.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3];
		
		return o;
	}

	CMatrix::operator D3DXMATRIX()
	{
		D3DXMATRIX m;
		memcpy((void*)&m, (void*)this, sizeof(CMatrix));
		return m;
	}

	void CMatrix::Zero()
	{
		for (size_t j = 0; j < 4; j++) 
		{
			for (size_t i = 0; i < 4; i++) 
			{
				m[j][i] = 0;
			}
		}
	}

	void CMatrix::Unit()
	{
		Zero();
		m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
	}

	void CMatrix::Translation(const CVector& tr)
	{
		Unit();
		m[0][3]=tr.x;
		m[1][3]=tr.y;
		m[2][3]=tr.z;
	}

	void CMatrix::Scale(const CVector& sc)
	{
		Zero();

		m[0][0]=sc.x;
		m[1][1]=sc.y;
		m[2][2]=sc.z;
		m[3][3]=1.0f;
	}

	void CMatrix::QuaternionRotate(const CQuaternion& q)
	{
		/*
			###0
			###0
			###0
			0001
		*/
		
		m[0][0] = 1.0f - 2.0f * q.y * q.y - 2.0f * q.z * q.z;
		m[0][1] = 2.0f * q.x * q.y + 2.0f * q.w * q.z;
		m[0][2] = 2.0f * q.x * q.z - 2.0f * q.w * q.y;
		m[1][0] = 2.0f * q.x * q.y - 2.0f * q.w * q.z;
		m[1][1] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.z * q.z;
		m[1][2] = 2.0f * q.y * q.z + 2.0f * q.w * q.x;
		m[2][0] = 2.0f * q.x * q.z + 2.0f * q.w * q.y;
		m[2][1] = 2.0f * q.y * q.z - 2.0f * q.w * q.x;
		m[2][2] = 1.0f - 2.0f * q.x * q.x - 2.0f * q.y * q.y;
		m[0][3] = m[1][3] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0;
		m[3][3] = 1.0f;
	}

	void CMatrix::Transpose()
	{
		for (size_t j=1; j<4; j++) 
		{
			for (size_t i=0; i<j; i++) 
			{
				float f = m[j][i];
				m[j][i] = m[i][j];
				m[i][j] = f;
			}
		}
	}

	const CMatrix CMatrix::NewTranslation(const CVector& tr)
	{
		CMatrix t;
		t.Translation(tr);
		return t;
	}

	const CMatrix CMatrix::NewQuatRotate(const CQuaternion& qr)
	{
		CMatrix t;
		t.QuaternionRotate(qr);
		return t;
	}

	const CMatrix CMatrix::NewScale(const CVector& sc)
	{
		CMatrix t;
		t.Scale(sc);
		return t;
	}

	//--------------------------------
	//-----------Common---------------
	//--------------------------------
	void Quat2Mat( CQuaternion& q, CMatrix& mat )
	{
		float s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;
		s = 2.0f / sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
		xs = q.x * s; ys = q.y * s; zs = q.z * s;
		wx = q.w * xs; wy = q.w * ys; wz = q.w * zs;
		xx = q.x * xs; xy = q.x * ys; xz = q.x * zs;
		yy = q.y * ys; yz = q.y * zs; zz = q.z * zs;

		memset( mat.m, 0, sizeof(mat.m) );
		mat.m[0][0] = 1.0f - (yy + zz);
		mat.m[0][1] = xy - wz;
		mat.m[0][2] = xz + wy;
		mat.m[1][0] = xy + wz;
		mat.m[1][1] = 1.0f - (xx + zz);
		mat.m[1][2] = yz - wx;
		mat.m[2][0] = xz - wy;
		mat.m[2][1] = yz + wx;
		mat.m[2][2] = 1.0f - (xx + yy);
		mat.m[3][3] = 1.0f;
	}

	void MakeMatrix( CVector& v, CQuaternion& q, CMatrix& m )
	{
		Quat2Mat( q, m );
		
		m.m[3][0] = v.x;
		m.m[3][1] = v.y;
		m.m[3][2] = v.z;
	}

	CMatrix* CMatrix::RotationAxis(CMatrix *pOut, const CVector *vec3, float angle)
	{
		memset(pOut, 0, sizeof(CMatrix));

		pOut->_44 = 1.0f;

		pOut->_11 = cos(angle) + vec3->x * vec3->x * (1 - cos(angle));
		pOut->_12 = vec3->z * sin(angle) + vec3->x * vec3->y * (1 - cos(angle));
		pOut->_13 = -vec3->y * sin(angle) + vec3->x * vec3->z * (1 - cos(angle));

		pOut->_21 = -vec3->z * sin(angle) + vec3->x * vec3->y * (1 - cos(angle));
		pOut->_22 = cos(angle) + vec3->y * vec3->y * (1 - cos(angle));
		pOut->_23 = vec3->x * sin(angle) + vec3->y * vec3->z * (1 - cos(angle));

		pOut->_31 = vec3->y * sin(angle) + vec3->x * vec3->z * (1 - cos(angle));
		pOut->_32 = -vec3->x * sin(angle) + vec3->y * vec3->z * (1 - cos(angle));
		pOut->_33 = cos(angle) + vec3->z * vec3->z * (1 - cos(angle));

		return pOut;
	}

	CVector* TransformNormal(CVector *pOut, const CVector *vec3, const CMatrix *mat)
	{
		CVector tempVec3;

		tempVec3.x = vec3->x * mat->_11 + vec3->y * mat->_21 + vec3->z * mat->_31;
		tempVec3.y = vec3->x * mat->_12 + vec3->y * mat->_22 + vec3->z * mat->_32;
		tempVec3.z = vec3->x * mat->_13 + vec3->y * mat->_23 + vec3->z * mat->_33;

		*pOut = tempVec3;
		return pOut;
	}
}