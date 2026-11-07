// FHMath
//
// The FH* math functions and the out-of-line parts of the math wrappers.
//
// Corrected from the original, none of which the app reached: the Lerps
// returned v2 + (v2 - v1) * s, FHQuaternionNormalize never wrote w,
// FHPlaneNormalize never wrote d (so FHMatrixReflect read an unset d),
// FHQuaternionToAxisAngle returned a quarter of the angle, and
// FHQuaternionSlerp only interpolated angles above e (2.718...).

#include "FHMath.h"

// Vectors

_SVector2 *FHVec2Lerp(_SVector2 *out, const _SVector2 *v1, const _SVector2 *v2, float s)
{
	out->x = v1->x + (v2->x - v1->x) * s;
	out->y = v1->y + (v2->y - v1->y) * s;
	return out;
}

_SVector3 *FHVec3Lerp(_SVector3 *out, const _SVector3 *v1, const _SVector3 *v2, float s)
{
	out->x = v1->x + (v2->x - v1->x) * s;
	out->y = v1->y + (v2->y - v1->y) * s;
	out->z = v1->z + (v2->z - v1->z) * s;
	return out;
}

_SVector4 *FHVec3Lerp(_SVector4 *out, const _SVector4 *v1, const _SVector4 *v2, float s)
{
	out->x = v1->x + (v2->x - v1->x) * s;
	out->y = v1->y + (v2->y - v1->y) * s;
	out->z = v1->z + (v2->z - v1->z) * s;
	out->w = v1->w + (v2->w - v1->w) * s;
	return out;
}

_SDVector2 *FHDVec2Lerp(_SDVector2 *out, const _SDVector2 *v1, const _SDVector2 *v2, double s)
{
	out->x = v1->x + (v2->x - v1->x) * s;
	out->y = v1->y + (v2->y - v1->y) * s;
	return out;
}

_SDVector3 *FHDVec3Lerp(_SDVector3 *out, const _SDVector3 *v1, const _SDVector3 *v2, double s)
{
	out->x = v1->x + (v2->x - v1->x) * s;
	out->y = v1->y + (v2->y - v1->y) * s;
	out->z = v1->z + (v2->z - v1->z) * s;
	return out;
}

_SDVector4 *FHDVec3Lerp(_SDVector4 *out, const _SDVector4 *v1, const _SDVector4 *v2, double s)
{
	out->x = v1->x + (v2->x - v1->x) * s;
	out->y = v1->y + (v2->y - v1->y) * s;
	out->z = v1->z + (v2->z - v1->z) * s;
	out->w = v1->w + (v2->w - v1->w) * s;
	return out;
}

_SVector3 *FHVec3Cross(_SVector3 *out, const _SVector3 *v1, const _SVector3 *v2)
{
	float x = v1->y * v2->z - v1->z * v2->y;
	float y = v1->z * v2->x - v2->z * v1->x;
	float z = v2->y * v1->x - v1->y * v2->x;
	out->x = x;
	out->y = y;
	out->z = z;
	return out;
}

_SDVector3 *FHDVec3Cross(_SDVector3 *out, const _SDVector3 *v1, const _SDVector3 *v2)
{
	double x = v1->y * v2->z - v1->z * v2->y;
	double y = v1->z * v2->x - v2->z * v1->x;
	double z = v2->y * v1->x - v1->y * v2->x;
	out->x = x;
	out->y = y;
	out->z = z;
	return out;
}

// The Hermite basis for s.
template <class T>
static inline void hermiteBasis(T s, T &h1, T &h2, T &h3, T &h4)
{
	T s2 = s * s;
	T s3 = s * s2;
	h1 = s3 + s3 + s2 * -3 + 1;
	h2 = s3 * -2 + s2 * 3;
	h3 = s2 * -2 + s3 + s;
	h4 = s3 - s2;
}

template <class V, class T>
static V *TFHVec2Hermite(V *out, const V *v1, const V *t1, const V *v2, const V *t2, T s)
{
	T h1, h2, h3, h4;
	hermiteBasis(s, h1, h2, h3, h4);
	T x = h1 * v1->x + h2 * v2->x + h3 * t1->x + h4 * t2->x;
	T y = h1 * v1->y + h2 * v2->y + h3 * t1->y + h4 * t2->y;
	out->x = x;
	out->y = y;
	return out;
}

template <class V, class T>
static V *TFHVec3Hermite(V *out, const V *v1, const V *t1, const V *v2, const V *t2, T s)
{
	T h1, h2, h3, h4;
	hermiteBasis(s, h1, h2, h3, h4);
	T x = h1 * v1->x + h2 * v2->x + h3 * t1->x + h4 * t2->x;
	T y = h1 * v1->y + h2 * v2->y + h3 * t1->y + h4 * t2->y;
	T z = h1 * v1->z + h2 * v2->z + h3 * t1->z + h4 * t2->z;
	out->x = x;
	out->y = y;
	out->z = z;
	return out;
}

template <class V, class T>
static V *TFHVec4Hermite(V *out, const V *v1, const V *t1, const V *v2, const V *t2, T s)
{
	T h1, h2, h3, h4;
	hermiteBasis(s, h1, h2, h3, h4);
	T x = h1 * v1->x + h2 * v2->x + h3 * t1->x + h4 * t2->x;
	T y = h1 * v1->y + h2 * v2->y + h3 * t1->y + h4 * t2->y;
	T z = h1 * v1->z + h2 * v2->z + h3 * t1->z + h4 * t2->z;
	T w = h1 * v1->w + h2 * v2->w + h3 * t1->w + h4 * t2->w;
	out->x = x;
	out->y = y;
	out->z = z;
	out->w = w;
	return out;
}

_SVector2 *FHVec2Hermite(_SVector2 *out, const _SVector2 *v1, const _SVector2 *t1, const _SVector2 *v2, const _SVector2 *t2, float s)
{
	return TFHVec2Hermite(out, v1, t1, v2, t2, s);
}

_SVector3 *FHVec3Hermite(_SVector3 *out, const _SVector3 *v1, const _SVector3 *t1, const _SVector3 *v2, const _SVector3 *t2, float s)
{
	return TFHVec3Hermite(out, v1, t1, v2, t2, s);
}

_SVector4 *FHVec4Hermite(_SVector4 *out, const _SVector4 *v1, const _SVector4 *t1, const _SVector4 *v2, const _SVector4 *t2, float s)
{
	return TFHVec4Hermite(out, v1, t1, v2, t2, s);
}

_SDVector2 *FHDVec2Hermite(_SDVector2 *out, const _SDVector2 *v1, const _SDVector2 *t1, const _SDVector2 *v2, const _SDVector2 *t2, double s)
{
	return TFHVec2Hermite(out, v1, t1, v2, t2, s);
}

_SDVector3 *FHDVec3Hermite(_SDVector3 *out, const _SDVector3 *v1, const _SDVector3 *t1, const _SDVector3 *v2, const _SDVector3 *t2, double s)
{
	return TFHVec3Hermite(out, v1, t1, v2, t2, s);
}

_SDVector4 *FHDVec4Hermite(_SDVector4 *out, const _SDVector4 *v1, const _SDVector4 *t1, const _SDVector4 *v2, const _SDVector4 *t2, double s)
{
	return TFHVec4Hermite(out, v1, t1, v2, t2, s);
}

// 1 / length, or 0 for a zero vector.
template <class T>
static inline T invLength(T lengthSq)
{
	T length = sqrt(lengthSq);
	return length != 0 ? 1 / length : length;
}

template <class V, class T>
static V *TFHVec2Normalize(V *out, const V *v)
{
	T k = invLength<T>(v->x * v->x + v->y * v->y);
	out->x = v->x * k;
	out->y = v->y * k;
	return out;
}

template <class V, class T>
static V *TFHVec3Normalize(V *out, const V *v)
{
	T k = invLength<T>(v->x * v->x + v->y * v->y + v->z * v->z);
	out->x = v->x * k;
	out->y = v->y * k;
	out->z = v->z * k;
	return out;
}

template <class V, class T>
static V *TFHVec4Normalize(V *out, const V *v)
{
	T k = invLength<T>(v->x * v->x + v->y * v->y + v->z * v->z + v->w * v->w);
	out->x = v->x * k;
	out->y = v->y * k;
	out->z = v->z * k;
	out->w = v->w * k;
	return out;
}

_SVector2 *FHVec2Normalize(_SVector2 *out, const _SVector2 *v) { return TFHVec2Normalize<_SVector2, float>(out, v); }
_SVector3 *FHVec3Normalize(_SVector3 *out, const _SVector3 *v) { return TFHVec3Normalize<_SVector3, float>(out, v); }
_SVector4 *FHVec4Normalize(_SVector4 *out, const _SVector4 *v) { return TFHVec4Normalize<_SVector4, float>(out, v); }
_SDVector2 *FHDVec2Normalize(_SDVector2 *out, const _SDVector2 *v) { return TFHVec2Normalize<_SDVector2, double>(out, v); }
_SDVector3 *FHDVec3Normalize(_SDVector3 *out, const _SDVector3 *v) { return TFHVec3Normalize<_SDVector3, double>(out, v); }
_SDVector4 *FHDVec4Normalize(_SDVector4 *out, const _SDVector4 *v) { return TFHVec4Normalize<_SDVector4, double>(out, v); }

_SVector4 *FHVec3Transform(_SVector4 *out, const _SVector3 *v, const _SMatrix *m)
{
	float x = v->x, y = v->y, z = v->z;
	out->x = x * m->_11 + y * m->_21 + z * m->_31 + m->_41;
	out->y = x * m->_12 + y * m->_22 + z * m->_32 + m->_42;
	out->z = x * m->_13 + y * m->_23 + z * m->_33 + m->_43;
	out->w = x * m->_14 + y * m->_24 + z * m->_34 + m->_44;
	return out;
}

_SVector3 *FHVec3TransformCoord(_SVector3 *out, const _SVector3 *v, const _SMatrix *m)
{
	float x = v->x, y = v->y, z = v->z;
	float w = x * m->_14 + y * m->_24 + z * m->_34 + m->_44;
	if (w != 0)
		w = 1 / w;
	out->x = (x * m->_11 + y * m->_21 + z * m->_31 + m->_41) * w;
	out->y = (x * m->_12 + y * m->_22 + z * m->_32 + m->_42) * w;
	out->z = (x * m->_13 + y * m->_23 + z * m->_33 + m->_43) * w;
	return out;
}

_SVector4 *FHVec4Transform(_SVector4 *out, const _SVector4 *v, const _SMatrix *m)
{
	float x = v->x, y = v->y, z = v->z, w = v->w;
	out->x = x * m->_11 + y * m->_21 + z * m->_31 + w * m->_41;
	out->y = x * m->_12 + y * m->_22 + z * m->_32 + w * m->_42;
	out->z = x * m->_13 + y * m->_23 + z * m->_33 + w * m->_43;
	out->w = x * m->_14 + y * m->_24 + z * m->_34 + w * m->_44;
	return out;
}

// Planes

_SPlane *FHPlaneFromPointNormal(_SPlane *out, const _SVector3 *point, const _SVector3 *normal)
{
	out->a = normal->x;
	out->b = normal->y;
	out->c = normal->z;
	out->d = -(normal->x * point->x + normal->y * point->y + normal->z * point->z);
	return out;
}

_SPlane *FHPlaneFromPoints(_SPlane *out, const _SVector3 *v1, const _SVector3 *v2, const _SVector3 *v3)
{
	float e1x = v2->x - v1->x, e1y = v2->y - v1->y, e1z = v2->z - v1->z;
	float e2x = v3->x - v1->x, e2y = v3->y - v1->y, e2z = v3->z - v1->z;
	float nx = e1y * e2z - e1z * e2y;
	float ny = e1z * e2x - e2z * e1x;
	float nz = e1x * e2y - e1y * e2x;
	float k = invLength<float>(nx * nx + ny * ny + nz * nz);
	out->a = k * nx;
	out->b = k * ny;
	out->c = k * nz;
	out->d = -(k * nx * v1->x + k * ny * v1->y + k * nz * v1->z);
	return out;
}

_SPlane *FHPlaneNormalize(_SPlane *out, const _SPlane *p)
{
	float k = invLength<float>(p->a * p->a + p->b * p->b + p->c * p->c);
	out->a = p->a * k;
	out->b = p->b * k;
	out->c = p->c * k;
	out->d = p->d * k;
	return out;
}

_SPlane *FHPlaneTransform(_SPlane *out, const _SPlane *p, const _SMatrix *m)
{
	float a = p->a, b = p->b, c = p->c, d = p->d;
	out->a = m->_11 * a + m->_21 * b + m->_31 * c + m->_41 * d;
	out->b = m->_12 * a + m->_22 * b + m->_32 * c + m->_42 * d;
	out->c = m->_13 * a + m->_23 * b + m->_33 * c + m->_43 * d;
	out->d = m->_14 * a + m->_24 * b + m->_34 * c + m->_44 * d;
	return out;
}

// Quaternions

// As D3DX: the rotation q1 followed by q2.
_SQuater *FHQuaternionMultiply(_SQuater *out, const _SQuater *q1, const _SQuater *q2)
{
	float x1 = q1->x, y1 = q1->y, z1 = q1->z, w1 = q1->w;
	float x2 = q2->x, y2 = q2->y, z2 = q2->z, w2 = q2->w;
	out->x = ((w1 * x2 + x1 * w2) - y1 * z2) + z1 * y2;
	out->y = ((w1 * y2 + w2 * y1) - z1 * x2) + x1 * z2;
	out->z = (w1 * z2 + z1 * w2 + x1 * y2) - x2 * y1;
	out->w = ((w1 * w2 - x2 * x1) - y1 * y2) - z2 * z1;
	return out;
}

_SQuater *FHQuaternionConjugate(_SQuater *out, const _SQuater *q)
{
	out->x = -q->x;
	out->y = -q->y;
	out->z = -q->z;
	out->w = q->w;
	return out;
}

_SQuater *FHQuaternionNormalize(_SQuater *out, const _SQuater *q)
{
	float k = sqrtf(q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w);
	k = k != 0 ? 1 / k : 0;
	out->x = q->x * k;
	out->y = q->y * k;
	out->z = q->z * k;
	out->w = q->w * k;
	return out;
}

void FHQuaternionToAxisAngle(const _SQuater *q, _SVector3 *axis, float *angle)
{
	_SQuater n;
	FHQuaternionNormalize(&n, q);
	float half = acosf(n.w);
	float s = sinf(half);
	float k = s != 0 ? 1 / s : 0;
	axis->x = k * n.x;
	axis->y = k * n.y;
	axis->z = k * n.z;
	*angle = half * 2;
}

_SQuater *FHQuaternionRotationMatrix(_SQuater *out, const _SMatrix *m)
{
	float trace = m->_11 + m->_22 + m->_33;
	if (trace > 0) {
		float s = sqrtf(trace + 1);
		out->w = s * 0.5f;
		s = 0.5f / s;
		out->x = (m->_23 - m->_32) * s;
		out->y = (m->_31 - m->_13) * s;
		out->z = (m->_12 - m->_21) * s;
		return out;
	}
	if (m->_11 >= (m->_22 > m->_33 ? m->_22 : m->_33)) {
		float s = sqrtf(((m->_11 - m->_22) - m->_33) + 1);
		out->x = s * 0.5f;
		s = 0.5f / s;
		out->w = (m->_23 - m->_32) * s;
		out->y = (m->_12 + m->_21) * s;
		out->z = (m->_13 + m->_31) * s;
	}
	else if (m->_22 >= (m->_11 > m->_33 ? m->_11 : m->_33)) {
		float s = sqrtf(((m->_22 - m->_11) - m->_33) + 1);
		out->y = s * 0.5f;
		s = 0.5f / s;
		out->w = (m->_31 - m->_13) * s;
		out->x = (m->_12 + m->_21) * s;
		out->z = (m->_23 + m->_32) * s;
	}
	else {
		float s = sqrtf(((m->_33 - m->_11) - m->_22) + 1);
		out->z = s * 0.5f;
		s = 0.5f / s;
		out->w = (m->_12 - m->_21) * s;
		out->x = (m->_13 + m->_31) * s;
		out->y = (m->_23 + m->_32) * s;
	}
	return out;
}

_SQuater *FHQuaternionRotationAxis(_SQuater *out, const _SVector3 *axis, float angle)
{
	_SVector3 n;
	FHVec3Normalize(&n, axis);
	float s = sinf(angle * 0.5f);
	out->x = s * n.x;
	out->y = s * n.y;
	out->z = s * n.z;
	out->w = cosf(angle * 0.5f);
	return out;
}

_SQuater *FHQuaternionSlerp(_SQuater *out, const _SQuater *q1, const _SQuater *q2, float t)
{
	_SQuater a, b;
	FHQuaternionNormalize(&a, q1);
	FHQuaternionNormalize(&b, q2);
	float cosTheta = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	if (cosTheta < -1)
		cosTheta = -1;
	else if (cosTheta > 1)
		cosTheta = 1;
	float theta = acosf(cosTheta);
	if (fabsf(theta) < 1e-5f) {
		*out = a;
		return out;
	}
	float invSin = 1 / sinf(theta);
	float ka = (float)sin((1.0 - t) * theta) * invSin;
	float kb = (float)sin((double)(theta * t)) * invSin;
	out->x = a.x * ka + b.x * kb;
	out->y = a.y * ka + b.y * kb;
	out->z = a.z * ka + b.z * kb;
	out->w = a.w * ka + b.w * kb;
	return out;
}

// Matrices

float FHDet3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3)
{
	return ((b2 * c3 - b3 * c2) * a1 - (c3 * b1 - b3 * c1) * a2) + (c2 * b1 - b2 * c1) * a3;
}

_SMatrix *FHMatrixMultiply(_SMatrix *out, const _SMatrix *m1, const _SMatrix *m2)
{
	_SMatrix r;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			r.m[i][j] = m1->m[i][0] * m2->m[0][j] + m1->m[i][1] * m2->m[1][j] +
			            m1->m[i][2] * m2->m[2][j] + m1->m[i][3] * m2->m[3][j];
	*out = r;
	return out;
}

_SMatrix *FHMatrixTranspose(_SMatrix *out, const _SMatrix *m)
{
	_SMatrix r;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			r.m[i][j] = m->m[j][i];
	*out = r;
	return out;
}

// The determinant of m without row and column.
static float minor3x3(const _SMatrix *m, int row, int col)
{
	float e[9];
	int n = 0;
	for (int i = 0; i < 4; i++) {
		if (i == row)
			continue;
		for (int j = 0; j < 4; j++)
			if (j != col)
				e[n++] = m->m[i][j];
	}
	return FHDet3x3(e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8]);
}

// A singular matrix gives all zeros.
_SMatrix *FHMatrixInverse(_SMatrix *out, float *determinant, const _SMatrix *m)
{
	float minors[4][4];
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			minors[i][j] = minor3x3(m, i, j);
	float det = minors[0][0] * m->_11 + -minors[0][1] * m->_12 + minors[0][2] * m->_13 + -minors[0][3] * m->_14;
	float k = det != 0 ? 1 / det : 0;
	if (determinant)
		*determinant = det;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			out->m[i][j] = ((i + j) & 1 ? -minors[j][i] : minors[j][i]) * k;
	return out;
}

static _SMatrix *setIdentity(_SMatrix *out)
{
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			out->m[i][j] = i == j ? 1.0f : 0.0f;
	return out;
}

_SMatrix *FHMatrixScaling(_SMatrix *out, float sx, float sy, float sz)
{
	setIdentity(out);
	out->_11 = sx;
	out->_22 = sy;
	out->_33 = sz;
	return out;
}

// Scales by scale along axis (a unit vector), keeping center fixed.
_SMatrix *FHMatrixScalingEx(_SMatrix *out, const _SVector3 *axis, const _SVector3 *center, float scale)
{
	float x = axis->x, y = axis->y, z = axis->z;
	float k = scale - 1;
	out->_11 = x * x * k + 1;
	out->_22 = y * y * k + 1;
	out->_33 = z * z * k + 1;
	out->_12 = out->_21 = x * y * k;
	out->_13 = out->_31 = x * z * k;
	out->_23 = out->_32 = y * z * k;
	out->_14 = out->_24 = out->_34 = 0;
	float t = -k * (x * center->x + y * center->y + z * center->z);
	out->_41 = x * t;
	out->_42 = t * y;
	out->_43 = t * z;
	out->_44 = 1;
	return out;
}

_SMatrix *FHMatrixTranslation(_SMatrix *out, float x, float y, float z)
{
	setIdentity(out);
	out->_41 = x;
	out->_42 = y;
	out->_43 = z;
	return out;
}

_SMatrix *FHMatrixRotationX(_SMatrix *out, float angle)
{
	setIdentity(out);
	float c = cosf(angle), s = sinf(angle);
	out->_22 = out->_33 = c;
	out->_23 = s;
	out->_32 = -s;
	return out;
}

_SMatrix *FHMatrixRotationY(_SMatrix *out, float angle)
{
	setIdentity(out);
	float c = cosf(angle), s = sinf(angle);
	out->_11 = out->_33 = c;
	out->_31 = s;
	out->_13 = -s;
	return out;
}

_SMatrix *FHMatrixRotationZ(_SMatrix *out, float angle)
{
	setIdentity(out);
	float c = cosf(angle), s = sinf(angle);
	out->_11 = out->_22 = c;
	out->_12 = s;
	out->_21 = -s;
	return out;
}

_SMatrix *FHMatrixRotationAxis(_SMatrix *out, const _SVector3 *axis, float angle)
{
	_SQuater q;
	FHQuaternionRotationAxis(&q, axis, angle);
	return FHMatrixRotationQuaternion(out, &q);
}

_SMatrix *FHMatrixRotationQuaternion(_SMatrix *out, const _SQuater *q)
{
	float x = q->x, y = q->y, z = q->z, w = q->w;
	float y2 = y + y, z2 = z + z;
	float xw2 = (x + x) * w, xx2 = (x + x) * x;
	out->_11 = 1 - (y2 * y + z * z2);
	out->_12 = z2 * w + x * y2;
	out->_13 = x * z2 - y2 * w;
	out->_14 = 0;
	out->_21 = x * y2 - z2 * w;
	out->_22 = 1 - (z * z2 + xx2);
	out->_23 = y * z2 + xw2;
	out->_24 = 0;
	out->_31 = y2 * w + x * z2;
	out->_32 = y * z2 - xw2;
	out->_33 = 1 - (xx2 + y2 * y);
	out->_34 = 0;
	out->_41 = out->_42 = out->_43 = 0;
	out->_44 = 1;
	return out;
}

// The rotation taking the direction from onto to.
_SMatrix *FHMatrixRotationVectors(_SMatrix *out, const _SVector3 *from, const _SVector3 *to)
{
	Vector3 a(*from), b(*to);
	float lengthSq = a.lengthSq();
	if (lengthSq - 1 >= 1e-5f || lengthSq - 1 <= -1e-5f)
		a /= sqrtf(lengthSq);
	lengthSq = b.lengthSq();
	if (lengthSq - 1 >= 1e-5f || lengthSq - 1 <= -1e-5f)
		b /= sqrtf(lengthSq);
	Vector3 axis(a.y * b.z - b.y * a.z, a.z * b.x - b.z * a.x, b.y * a.x - a.y * b.x);
	lengthSq = axis.lengthSq();
	if (!(lengthSq >= 1e-9f || isnan(lengthSq)))
		return setIdentity(out);
	axis /= sqrtf(lengthSq);
	float c = a.dot(b);
	if (c < -1)
		c = -1;
	else if (c > 1)
		c = 1;
	float s = sqrtf((1 - c) * 0.5f);
	_SQuater q = { s * axis.x, s * axis.y, s * axis.z, sqrtf((c + 1) * 0.5f) };
	return FHMatrixRotationQuaternion(out, &q);
}

_SMatrix *FHMatrixReflect(_SMatrix *out, const _SPlane *plane)
{
	_SPlane p;
	FHPlaneNormalize(&p, plane);
	float a = p.a * -2, b = p.b * -2, c = p.c * -2;
	out->_11 = p.a * a + 1;
	out->_12 = b * p.a;
	out->_13 = p.a * c;
	out->_14 = 0;
	out->_21 = p.b * a;
	out->_22 = p.b * b + 1;
	out->_23 = p.b * c;
	out->_24 = 0;
	out->_31 = p.c * a;
	out->_32 = p.c * b;
	out->_33 = p.c * c + 1;
	out->_34 = 0;
	out->_41 = a * p.d;
	out->_42 = b * p.d;
	out->_43 = c * p.d;
	out->_44 = 1;
	return out;
}

_SMatrix *FHMatrixLookAtRH(_SMatrix *out, const _SVector3 *eye, const _SVector3 *at, const _SVector3 *up)
{
	_SVector3 d = { eye->x - at->x, eye->y - at->y, eye->z - at->z };
	_SVector3 zaxis, xaxis, yaxis;
	FHVec3Normalize(&zaxis, &d);
	FHVec3Cross(&xaxis, up, &zaxis);
	FHVec3Normalize(&xaxis, &xaxis);
	FHVec3Cross(&yaxis, &zaxis, &xaxis);
	out->_11 = xaxis.x;
	out->_12 = yaxis.x;
	out->_13 = zaxis.x;
	out->_14 = 0;
	out->_21 = xaxis.y;
	out->_22 = yaxis.y;
	out->_23 = zaxis.y;
	out->_24 = 0;
	out->_31 = xaxis.z;
	out->_32 = yaxis.z;
	out->_33 = zaxis.z;
	out->_34 = 0;
	out->_41 = -(xaxis.x * eye->x + xaxis.y * eye->y + xaxis.z * eye->z);
	out->_42 = -(yaxis.x * eye->x + yaxis.y * eye->y + yaxis.z * eye->z);
	out->_43 = -(zaxis.x * eye->x + zaxis.y * eye->y + zaxis.z * eye->z);
	out->_44 = 1;
	return out;
}

// OpenGL depth range: z maps to -1..1.
_SMatrix *FHMatrixPerspectiveFovRH(_SMatrix *out, float fovy, float aspect, float zn, float zf)
{
	double yScale = 1.0 / tan((double)(fovy * 0.5f));
	setIdentity(out);
	out->_22 = (float)yScale;
	out->_11 = (float)yScale / aspect;
	out->_33 = (zf + zn) / (zn - zf);
	out->_34 = -1;
	out->_43 = ((zn + zn) * zf) / (zn - zf);
	out->_44 = 0;
	return out;
}

_SMatrix *FHMatrixOrthoRH(_SMatrix *out, float w, float h, float zn, float zf)
{
	setIdentity(out);
	out->_11 = 2 / w;
	out->_22 = 2 / h;
	out->_33 = 2 / (zn - zf);
	out->_43 = (zn + zf) / (zn - zf);
	return out;
}

// Wrappers

static unsigned int colorByte(float f)
{
	if (!(f < 1))
		return 0xff;
	if (!(0 < f))
		return 0;
	return (unsigned int)(f * 255 + 0.5f);
}

Color::operator unsigned int() const
{
	return colorByte(r) | colorByte(a) << 24 | colorByte(b) << 16 | colorByte(g) << 8;
}
