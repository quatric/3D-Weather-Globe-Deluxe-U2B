// FHMath
//
// FishHook's math library: the D3DX-style vector, matrix, quaternion and plane
// structs with their FH* functions, and the C++ wrappers Vector2/3/4, Matrix,
// Quater, Plane and Color that the engine and the app use. Matrices are row
// major and vectors are rows (v * M), as in Direct3D.

#ifndef FHMATH_H
#define FHMATH_H

#include <math.h>

struct _SVector2 { float x, y; };
struct _SVector3 { float x, y, z; };
struct _SVector4 { float x, y, z, w; };
struct _SDVector2 { double x, y; };
struct _SDVector3 { double x, y, z; };
struct _SDVector4 { double x, y, z, w; };
struct _SQuater { float x, y, z, w; };
struct _SPlane { float a, b, c, d; };

struct _SMatrix {
	union {
		struct {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
	};
};

// Vectors

_SVector2 *FHVec2Lerp(_SVector2 *out, const _SVector2 *v1, const _SVector2 *v2, float s);
_SVector3 *FHVec3Lerp(_SVector3 *out, const _SVector3 *v1, const _SVector3 *v2, float s);
_SVector4 *FHVec3Lerp(_SVector4 *out, const _SVector4 *v1, const _SVector4 *v2, float s);
_SDVector2 *FHDVec2Lerp(_SDVector2 *out, const _SDVector2 *v1, const _SDVector2 *v2, double s);
_SDVector3 *FHDVec3Lerp(_SDVector3 *out, const _SDVector3 *v1, const _SDVector3 *v2, double s);
_SDVector4 *FHDVec3Lerp(_SDVector4 *out, const _SDVector4 *v1, const _SDVector4 *v2, double s);

_SVector3 *FHVec3Cross(_SVector3 *out, const _SVector3 *v1, const _SVector3 *v2);
_SDVector3 *FHDVec3Cross(_SDVector3 *out, const _SDVector3 *v1, const _SDVector3 *v2);

// Hermite(out, v1, t1, v2, t2, s): the cubic from v1 (tangent t1) to v2 (tangent t2).
_SVector2 *FHVec2Hermite(_SVector2 *out, const _SVector2 *v1, const _SVector2 *t1, const _SVector2 *v2, const _SVector2 *t2, float s);
_SVector3 *FHVec3Hermite(_SVector3 *out, const _SVector3 *v1, const _SVector3 *t1, const _SVector3 *v2, const _SVector3 *t2, float s);
_SVector4 *FHVec4Hermite(_SVector4 *out, const _SVector4 *v1, const _SVector4 *t1, const _SVector4 *v2, const _SVector4 *t2, float s);
_SDVector2 *FHDVec2Hermite(_SDVector2 *out, const _SDVector2 *v1, const _SDVector2 *t1, const _SDVector2 *v2, const _SDVector2 *t2, double s);
_SDVector3 *FHDVec3Hermite(_SDVector3 *out, const _SDVector3 *v1, const _SDVector3 *t1, const _SDVector3 *v2, const _SDVector3 *t2, double s);
_SDVector4 *FHDVec4Hermite(_SDVector4 *out, const _SDVector4 *v1, const _SDVector4 *t1, const _SDVector4 *v2, const _SDVector4 *t2, double s);

_SVector2 *FHVec2Normalize(_SVector2 *out, const _SVector2 *v);
_SVector3 *FHVec3Normalize(_SVector3 *out, const _SVector3 *v);
_SVector4 *FHVec4Normalize(_SVector4 *out, const _SVector4 *v);
_SDVector2 *FHDVec2Normalize(_SDVector2 *out, const _SDVector2 *v);
_SDVector3 *FHDVec3Normalize(_SDVector3 *out, const _SDVector3 *v);
_SDVector4 *FHDVec4Normalize(_SDVector4 *out, const _SDVector4 *v);

_SVector4 *FHVec3Transform(_SVector4 *out, const _SVector3 *v, const _SMatrix *m);
_SVector3 *FHVec3TransformCoord(_SVector3 *out, const _SVector3 *v, const _SMatrix *m);
_SVector4 *FHVec4Transform(_SVector4 *out, const _SVector4 *v, const _SMatrix *m);

// Planes

_SPlane *FHPlaneFromPointNormal(_SPlane *out, const _SVector3 *point, const _SVector3 *normal);
_SPlane *FHPlaneFromPoints(_SPlane *out, const _SVector3 *v1, const _SVector3 *v2, const _SVector3 *v3);
_SPlane *FHPlaneNormalize(_SPlane *out, const _SPlane *p);
_SPlane *FHPlaneTransform(_SPlane *out, const _SPlane *p, const _SMatrix *m);

// Quaternions

_SQuater *FHQuaternionMultiply(_SQuater *out, const _SQuater *q1, const _SQuater *q2);
_SQuater *FHQuaternionConjugate(_SQuater *out, const _SQuater *q);
_SQuater *FHQuaternionNormalize(_SQuater *out, const _SQuater *q);
void FHQuaternionToAxisAngle(const _SQuater *q, _SVector3 *axis, float *angle);
_SQuater *FHQuaternionRotationMatrix(_SQuater *out, const _SMatrix *m);
_SQuater *FHQuaternionRotationAxis(_SQuater *out, const _SVector3 *axis, float angle);
_SQuater *FHQuaternionSlerp(_SQuater *out, const _SQuater *q1, const _SQuater *q2, float t);

// Matrices

float FHDet3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3);
_SMatrix *FHMatrixMultiply(_SMatrix *out, const _SMatrix *m1, const _SMatrix *m2);
_SMatrix *FHMatrixTranspose(_SMatrix *out, const _SMatrix *m);
static inline _SMatrix *FHMatrixIdentity(_SMatrix *out)
{
	for (int i = 0; i < 16; i++)
		(&out->_11)[i] = 0.0f;
	out->_11 = out->_22 = out->_33 = out->_44 = 1.0f;
	return out;
}
_SMatrix *FHMatrixInverse(_SMatrix *out, float *determinant, const _SMatrix *m);
_SMatrix *FHMatrixScaling(_SMatrix *out, float sx, float sy, float sz);
_SMatrix *FHMatrixScalingEx(_SMatrix *out, const _SVector3 *axis, const _SVector3 *center, float scale);
_SMatrix *FHMatrixTranslation(_SMatrix *out, float x, float y, float z);
_SMatrix *FHMatrixRotationX(_SMatrix *out, float angle);
_SMatrix *FHMatrixRotationY(_SMatrix *out, float angle);
_SMatrix *FHMatrixRotationZ(_SMatrix *out, float angle);
_SMatrix *FHMatrixRotationAxis(_SMatrix *out, const _SVector3 *axis, float angle);
_SMatrix *FHMatrixRotationQuaternion(_SMatrix *out, const _SQuater *q);
_SMatrix *FHMatrixRotationVectors(_SMatrix *out, const _SVector3 *from, const _SVector3 *to);
_SMatrix *FHMatrixReflect(_SMatrix *out, const _SPlane *plane);
_SMatrix *FHMatrixLookAtRH(_SMatrix *out, const _SVector3 *eye, const _SVector3 *at, const _SVector3 *up);
_SMatrix *FHMatrixPerspectiveFovRH(_SMatrix *out, float fovy, float aspect, float zn, float zf);
_SMatrix *FHMatrixOrthoRH(_SMatrix *out, float w, float h, float zn, float zf);

// Wrappers

class Vector2 : public _SVector2 {
public:
	Vector2() {}
	Vector2(float ax, float ay) { x = ax; y = ay; }
	Vector2(const _SVector2 &v) { x = v.x; y = v.y; }
	explicit Vector2(const float *f) { x = f[0]; y = f[1]; }

	operator float *() { return &x; }
	operator const float *() const { return &x; }

	Vector2 &operator+=(const _SVector2 &v) { x += v.x; y += v.y; return *this; }
	Vector2 &operator-=(const _SVector2 &v) { x -= v.x; y -= v.y; return *this; }
	Vector2 &operator*=(float f) { x *= f; y *= f; return *this; }
	Vector2 &operator/=(float f) { x /= f; y /= f; return *this; }

	Vector2 operator-() const { return Vector2(-x, -y); }
	Vector2 operator+(const _SVector2 &v) const { return Vector2(x + v.x, y + v.y); }
	Vector2 operator-(const _SVector2 &v) const { return Vector2(x - v.x, y - v.y); }
	Vector2 operator*(float f) const { return Vector2(x * f, y * f); }
	Vector2 operator/(float f) const { return Vector2(x / f, y / f); }

	bool operator==(const _SVector2 &v) const { return x == v.x && y == v.y; }
	bool operator!=(const _SVector2 &v) const { return !(*this == v); }

	float length() const { return sqrtf(x * x + y * y); }
};

class Vector3 : public _SVector3 {
public:
	Vector3() {}
	Vector3(float ax, float ay, float az) { x = ax; y = ay; z = az; }
	Vector3(const _SVector3 &v) { x = v.x; y = v.y; z = v.z; }
	explicit Vector3(const float *f) { x = f[0]; y = f[1]; z = f[2]; }

	operator float *() { return &x; }
	operator const float *() const { return &x; }

	Vector3 &operator+=(const _SVector3 &v) { x += v.x; y += v.y; z += v.z; return *this; }
	Vector3 &operator-=(const _SVector3 &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	Vector3 &operator*=(float f) { x *= f; y *= f; z *= f; return *this; }
	Vector3 &operator/=(float f) { x /= f; y /= f; z /= f; return *this; }

	Vector3 operator-() const { return Vector3(-x, -y, -z); }
	Vector3 operator+(const _SVector3 &v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
	Vector3 operator-(const _SVector3 &v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
	Vector3 operator*(float f) const { return Vector3(x * f, y * f, z * f); }
	Vector3 operator/(float f) const { return Vector3(x / f, y / f, z / f); }

	bool operator==(const _SVector3 &v) const { return x == v.x && y == v.y && z == v.z; }
	bool operator!=(const _SVector3 &v) const { return !(*this == v); }

	float dot(const _SVector3 &v) const { return x * v.x + y * v.y + z * v.z; }
	float lengthSq() const { return x * x + y * y + z * z; }
	float length() const { return sqrtf(lengthSq()); }
};

inline Vector3 operator*(float f, const Vector3 &v) { return v * f; }

class Vector4 : public _SVector4 {
public:
	Vector4() {}
	Vector4(float ax, float ay, float az, float aw) { x = ax; y = ay; z = az; w = aw; }
	Vector4(const _SVector4 &v) { x = v.x; y = v.y; z = v.z; w = v.w; }
	Vector4(const _SVector3 &v, float aw) { x = v.x; y = v.y; z = v.z; w = aw; }
	explicit Vector4(const float *f) { x = f[0]; y = f[1]; z = f[2]; w = f[3]; }

	operator float *() { return &x; }
	operator const float *() const { return &x; }

	Vector4 &operator+=(const _SVector4 &v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	Vector4 &operator-=(const _SVector4 &v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	Vector4 &operator*=(float f) { x *= f; y *= f; z *= f; w *= f; return *this; }
	Vector4 &operator/=(float f) { x /= f; y /= f; z /= f; w /= f; return *this; }

	Vector4 operator-() const { return Vector4(-x, -y, -z, -w); }
	Vector4 operator+(const _SVector4 &v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
	Vector4 operator-(const _SVector4 &v) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }
	Vector4 operator*(float f) const { return Vector4(x * f, y * f, z * f, w * f); }
	Vector4 operator/(float f) const { return Vector4(x / f, y / f, z / f, w / f); }

	bool operator==(const _SVector4 &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
	bool operator!=(const _SVector4 &v) const { return !(*this == v); }
};

class Quater : public _SQuater {
public:
	Quater() {}
	Quater(float ax, float ay, float az, float aw) { x = ax; y = ay; z = az; w = aw; }
	Quater(const _SQuater &q) { x = q.x; y = q.y; z = q.z; w = q.w; }

	operator float *() { return &x; }
	operator const float *() const { return &x; }

	Quater operator*(const _SQuater &q) const { Quater r; FHQuaternionMultiply(&r, this, &q); return r; }
	Quater &operator*=(const _SQuater &q) { FHQuaternionMultiply(this, this, &q); return *this; }
	bool operator==(const _SQuater &q) const { return x == q.x && y == q.y && z == q.z && w == q.w; }
	bool operator!=(const _SQuater &q) const { return !(*this == q); }
};

class Plane : public _SPlane {
public:
	Plane() {}
	Plane(float aa, float ab, float ac, float ad) { a = aa; b = ab; c = ac; d = ad; }
	Plane(const _SPlane &p) { a = p.a; b = p.b; c = p.c; d = p.d; }

	operator float *() { return &a; }
	operator const float *() const { return &a; }

	Plane operator-() const { return Plane(-a, -b, -c, -d); }
	bool operator==(const _SPlane &p) const { return a == p.a && b == p.b && c == p.c && d == p.d; }
	bool operator!=(const _SPlane &p) const { return !(*this == p); }

	// Signed distance of a point for a normalized plane.
	float dotCoord(const _SVector3 &v) const { return a * v.x + b * v.y + c * v.z + d; }
	float dotNormal(const _SVector3 &v) const { return a * v.x + b * v.y + c * v.z; }
};

class Matrix : public _SMatrix {
public:
	Matrix() {}
	Matrix(const _SMatrix &mat) { *(_SMatrix *)this = mat; }
	explicit Matrix(const float *f) { for (int i = 0; i < 16; i++) (&_11)[i] = f[i]; }

	operator float *() { return &_11; }
	operator const float *() const { return &_11; }
	float &operator()(int row, int col) { return m[row][col]; }
	float operator()(int row, int col) const { return m[row][col]; }

	Matrix operator*(const _SMatrix &mat) const { Matrix r; FHMatrixMultiply(&r, this, &mat); return r; }
	Matrix &operator*=(const _SMatrix &mat) { FHMatrixMultiply(this, this, &mat); return *this; }
	bool operator==(const _SMatrix &mat) const
	{
		for (int i = 0; i < 16; i++)
			if ((&_11)[i] != (&mat._11)[i])
				return false;
		return true;
	}
	bool operator!=(const _SMatrix &mat) const { return !(*this == mat); }

	Matrix &setIdentity()
	{
		for (int i = 0; i < 16; i++)
			(&_11)[i] = 0.0f;
		_11 = _22 = _33 = _44 = 1.0f;
		return *this;
	}
	static Matrix identity() { Matrix r; return r.setIdentity(); }
};

// An RGBA color with float components; converting it to unsigned int packs it
// as 0xAABBGGRR, each component clamped to 0..1.
class Color {
public:
	float r, g, b, a;

	Color() {}
	Color(float ar, float ag, float ab, float aa = 1.0f) { r = ar; g = ag; b = ab; a = aa; }
	explicit Color(unsigned int packed)
	{
		r = (packed & 0xff) / 255.0f;
		g = ((packed >> 8) & 0xff) / 255.0f;
		b = ((packed >> 16) & 0xff) / 255.0f;
		a = (packed >> 24) / 255.0f;
	}

	operator float *() { return &r; }
	operator const float *() const { return &r; }
	operator unsigned int() const;

	// Component-wise, alpha included.
	Color operator+(const Color &c) const { return Color(r + c.r, g + c.g, b + c.b, a + c.a); }
	Color operator-(const Color &c) const { return Color(r - c.r, g - c.g, b - c.b, a - c.a); }
	Color operator*(float f) const { return Color(r * f, g * f, b * f, a * f); }

	bool operator==(const Color &c) const { return r == c.r && g == c.g && b == c.b && a == c.a; }
	bool operator!=(const Color &c) const { return !(*this == c); }
};

// The eight corners of the view volume (near z -1 then far z +1 in clip
// space, unprojected) and its six planes, all facing inwards.
struct SViewFrustum {
	Vector3 corners[8];
	Plane planes[6];
};

struct STransformParts {
	Quater rotation;
	Vector3 scaling;
	Vector3 translation;
};

class UMathUtils {
public:
	static void normalize(Vector3 *v)
	{
		float k = 1.0f / sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
		v->x *= k;
		v->y *= k;
		v->z *= k;
	}
	static void evalBoxZRange(const Vector3 &min, const Vector3 &max, const Matrix *m, float &zMin, float &zMax);
	// 0 outside, 1 intersecting, 2 inside.
	static int boxIntersectionTest(const Vector3 *min, const Vector3 *max, const SViewFrustum *frustum);
	static int boxIntersectionTest(const Vector3 *min, const Vector3 *max, const Matrix *m, const SViewFrustum *frustum);
	static bool sphereIntersectsViewFrustum(const Vector3 *center, float radius, const SViewFrustum *frustum);
	static bool boxRayIntersectionTest(const Vector3 &min, const Vector3 &max, const Vector3 &origin, const Vector3 &dir,
	                                   float *distance, Vector3 *enter, Vector3 *exit);

	static void calcViewFrustum(SViewFrustum *frustum, const Matrix *invViewProj);
	static void calcViewFrustum(SViewFrustum *frustum, const Matrix *view, const Matrix *proj);
	static void shadowMapViewProjMatrixForFrustumDir(Matrix *out, const Vector3 *center, const Vector3 *dir, const SViewFrustum *frustum);
	static void fillExp2FogParamsConstant(float *out, const Matrix *view, float fogStart, float fogEnd, float density);

	static void scalingFromMatrix(const Matrix *m, _SVector3 *scaling);
	static _SQuater *rotationFromMatrix(const Matrix *m, _SQuater *rotation);
	static STransformParts *transformsFromMatrix(const Matrix *m, STransformParts *parts);
	static Matrix *matrixRotationAxis(Matrix *out, const Vector3 *axis, float angle);
	static Matrix *matrixRotationVectors(Matrix *out, const Vector3 *from, const Vector3 *to);

	static void vec3SplineInterpolation(Vector3 *out, float t, const Vector3 &p0, const Vector3 &p1, const Vector3 &p2, const Vector3 &p3);
	static void quaternionSplineInterpolation(Quater *out, float t, const Quater &q0, const Quater &q1, const Quater &q2, const Quater &q3);

	static void initNoise1D(int size);
	static void initNoise2D(int size);
	static void initNoise3D(int size);
	static float noise1D(float x);
	static float noise2D(const Vector2 &v);
	static float noise3D(const Vector3 &v);
	static float noise2D_harm(const Vector2 &v, int harmonics, float persistence);
	static float noise3D_harm(const Vector3 &v, int harmonics, float persistence);
};

bool check_line_intersection(float k, float b, float tMin, float tMax, float lo, float hi, float &outMin, float &outMax);
bool check_prepared_box_intersection(float d0, float d1, float d2, float o0, float o1, float o2,
                                     float min0, float min1, float min2, float max0, float max1, float max2,
                                     float &tMin, float &tMax);

#endif
