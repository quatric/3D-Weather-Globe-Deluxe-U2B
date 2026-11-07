// UMathUtils
//
// Frustum and box tests, matrix decomposition, splines and gradient noise.

#include "FHMath.h"
#include "LFBase.h"

#include <stdlib.h>

// The noise tables. The permutations hold indices into the gradients; the 1D
// one repeats its first entry after the last.
static int _k1DTableSize = 16, _k2DTableSize = 16, _k3DTableSize = 16;
static float _kf1DTableSize = 16, _kf2DTableSize = 16, _kf3DTableSize = 16;
static int _sIdxTable_1D[257], _sIdxTable_2D[257], _sIdxTable_3D[257];
static float _sGradsTable_1D[256], _sGradsTable_2D[256 * 2], _sGradsTable_3D[256 * 3];
static bool _s1dNoiseWasInitializedOnce, _s2dNoiseWasInitializedOnce, _s3dNoiseWasInitializedOnce;

void UMathUtils::evalBoxZRange(const Vector3 &min, const Vector3 &max, const Matrix *m, float &zMin, float &zMax)
{
	for (int i = 0; i < 3; i++) {
		float a = m->m[i][2] * min[i];
		float b = m->m[i][2] * max[i];
		float lo = b <= a ? b : a;
		float hi = b <= a ? a : b;
		if (i == 0) {
			zMin = lo;
			zMax = hi;
		}
		else {
			zMin = lo + zMin;
			zMax = hi + zMax;
		}
	}
	zMin = zMin + m->_43;
	zMax = zMax + m->_43;
}

int UMathUtils::boxIntersectionTest(const Vector3 *min, const Vector3 *max, const SViewFrustum *frustum)
{
	int inside = 0;
	for (int i = 0; i < 6; i++) {
		const Plane &p = frustum->planes[i];
		int count = 0;
		for (int corner = 0; corner < 8; corner++) {
			Vector3 v(corner & 1 ? max->x : min->x, corner & 2 ? max->y : min->y, corner & 4 ? max->z : min->z);
			if (p.dotCoord(v) > 0)
				count++;
		}
		if (count == 0)
			return 0;
		if (count == 8)
			inside++;
	}
	return inside == 6 ? 2 : 1;
}

// The box min..max transformed by m.
int UMathUtils::boxIntersectionTest(const Vector3 *min, const Vector3 *max, const Matrix *m, const SViewFrustum *frustum)
{
	_SVector4 origin;
	FHVec3Transform(&origin, min, m);
	Vector3 ex = Vector3(m->_11, m->_12, m->_13) * (max->x - min->x);
	Vector3 ey = Vector3(m->_21, m->_22, m->_23) * (max->y - min->y);
	Vector3 ez = Vector3(m->_31, m->_32, m->_33) * (max->z - min->z);
	Vector3 o(origin.x, origin.y, origin.z);
	int inside = 0;
	for (int i = 0; i < 6; i++) {
		const Plane &p = frustum->planes[i];
		float threshold = -p.dotCoord(o);
		float dx = p.dotNormal(ex), dy = p.dotNormal(ey), dz = p.dotNormal(ez);
		int count = 0;
		for (int corner = 0; corner < 8; corner++) {
			float sum = (corner & 1 ? dx : 0) + (corner & 2 ? dy : 0);
			float t = corner & 4 ? threshold - dz : threshold;
			if (t <= sum)
				count++;
		}
		if (count == 0)
			return 0;
		if (count == 8)
			inside++;
	}
	return inside == 6 ? 2 : 1;
}

bool UMathUtils::sphereIntersectsViewFrustum(const Vector3 *center, float radius, const SViewFrustum *frustum)
{
	for (int i = 0; i < 6; i++)
		if (frustum->planes[i].dotCoord(*center) < -radius)
			return false;
	return true;
}

// The parameter range [outMin, outMax] within [tMin, tMax] where the line
// k * t + b lies between lo and hi; false when it never does.
bool check_line_intersection(float k, float b, float tMin, float tMax, float lo, float hi, float &outMin, float &outMax)
{
	float y1 = k * tMin + b;
	float y2 = k * tMax + b;
	if ((y1 <= lo && y2 <= lo) || (hi <= y1 && hi <= y2))
		return false;
	if (k > 0 || k < 0) {
		outMin = (lo - b) / k;
		outMax = (hi - b) / k;
	}
	else {
		outMin = tMin;
		outMax = tMax;
	}
	if (outMin < tMin)
		outMin = tMin;
	if (tMax < outMax)
		outMax = tMax;
	return true;
}

// The range of axis 0 over which the ray o + d * t is inside the box on axes 1
// and 2.
bool check_prepared_box_intersection(float d0, float d1, float d2, float o0, float o1, float o2,
                                     float min0, float min1, float min2, float max0, float max1, float max2,
                                     float &tMin, float &tMax)
{
	if (!check_line_intersection(d1 / d0, o1 - (o0 * d1) / d0, min0, max0, min1, max1, tMin, tMax))
		return false;
	return check_line_intersection(d2 / d0, o2 - (o0 * d2) / d0, tMin, tMax, min2, max2, tMin, tMax);
}

// Where the ray origin + dir * t enters and leaves the box. distance gets the
// distance from origin to the farther of the two points when the box is ahead,
// or to the exit when origin is inside.
bool UMathUtils::boxRayIntersectionTest(const Vector3 &min, const Vector3 &max, const Vector3 &origin, const Vector3 &dir,
                                        float *distance, Vector3 *enter, Vector3 *exit)
{
	float ax = fabsf(dir.x), ay = fabsf(dir.y), az = fabsf(dir.z);
	int major;
	if (ax < ay)
		major = ay < az ? 2 : 1;
	else
		major = ax < az ? 2 : 0;
	int a1 = (major + 1) % 3, a2 = (major + 2) % 3;

	float t0, t1;
	bool hit = check_prepared_box_intersection(dir[major], dir[a1], dir[a2], origin[major], origin[a1], origin[a2],
	                                           min[major], min[a1], min[a2], max[major], max[a1], max[a2], t0, t1);
	if (!hit)
		return false;

	Vector3 in, out;
	in[major] = t0;
	in[a1] = ((t0 - origin[major]) * dir[a1]) / dir[major] + origin[a1];
	in[a2] = ((t0 - origin[major]) * dir[a2]) / dir[major] + origin[a2];
	out[major] = t1;
	out[a1] = ((t1 - origin[major]) * dir[a1]) / dir[major] + origin[a1];
	out[a2] = ((t1 - origin[major]) * dir[a2]) / dir[major] + origin[a2];
	if (enter)
		*enter = in;
	if (exit)
		*exit = out;

	Vector3 toIn = in - origin;
	float along = toIn.dot(dir);
	if (along >= 0 || isnan(along)) {
		if (distance) {
			float d = toIn.length();
			float dOut = (out - origin).length();
			*distance = d <= dOut ? dOut : d;
		}
	}
	else {
		Vector3 toOut = out - origin;
		if (toOut.dot(dir) >= 0) {
			if (distance)
				*distance = toOut.length();
		}
		else
			hit = false;
	}
	return hit;
}

void UMathUtils::calcViewFrustum(SViewFrustum *frustum, const Matrix *invViewProj)
{
	Vector3 *c = frustum->corners;
	for (int i = 0; i < 8; i++) {
		c[i] = Vector3(i & 1 ? 1.0f : -1.0f, i & 2 ? 1.0f : -1.0f, i & 4 ? 1.0f : -1.0f);
		FHVec3TransformCoord(&c[i], &c[i], invViewProj);
	}

	// Each plane from three corners of one face, turned to face a corner
	// on the opposite face.
	static const int kFaces[6][4] = {
		{ 0, 1, 2, 4 }, // near
		{ 6, 7, 5, 0 }, // far
		{ 2, 6, 4, 7 }, // left
		{ 7, 3, 5, 6 }, // right
		{ 2, 3, 6, 5 }, // top
		{ 1, 0, 4, 7 }, // bottom
	};
	for (int i = 0; i < 6; i++) {
		Plane &p = frustum->planes[i];
		FHPlaneFromPoints(&p, &c[kFaces[i][0]], &c[kFaces[i][1]], &c[kFaces[i][2]]);
		if (p.dotCoord(c[kFaces[i][3]]) < 0)
			p = -p;
	}
}

void UMathUtils::calcViewFrustum(SViewFrustum *frustum, const Matrix *view, const Matrix *proj)
{
	Matrix m;
	FHMatrixMultiply(&m, view, proj);
	FHMatrixInverse(&m, 0, &m);
	calcViewFrustum(frustum, &m);
}

// An orthographic light view-projection looking along dir that covers the
// frustum's corners.
void UMathUtils::shadowMapViewProjMatrixForFrustumDir(Matrix *out, const Vector3 *center, const Vector3 *dir, const SViewFrustum *frustum)
{
	Vector3 d(*dir);
	FHVec3Normalize(&d, &d);
	Vector3 up = *center - d * d.dot(*center);
	up.y += 0.002f;
	FHVec3Normalize(&up, &up);
	Vector3 right;
	FHVec3Cross(&right, &up, &d);

	Vector3 eye(0, 0, 0);
	Vector3 at = eye + *dir;
	Matrix view;
	FHMatrixLookAtRH(&view, &eye, &at, &up);

	_SVector4 v;
	float minX = 1e10f, minY = 1e10f, minZ = 1e10f;
	float maxX = -1e10f, maxY = -1e10f;
	for (int i = 0; i < 8; i++) {
		FHVec3Transform(&v, &frustum->corners[i], &view);
		maxX = v.x > maxX ? v.x : maxX;
		minX = v.x < minX ? v.x : minX;
		minY = v.y < minY ? v.y : minY;
		maxY = v.y > maxY ? v.y : maxY;
		minZ = v.z < minZ ? v.z : minZ;
	}

	eye = right * 0.5f * (maxX + minX) + up * 0.5f * (maxY + minY) + d * minZ;
	at = eye + *dir;
	FHMatrixLookAtRH(&view, &eye, &at, &up);

	float maxZ = -1e10f;
	minX = minY = minZ = 1e10f;
	maxX = maxY = -1e10f;
	for (int i = 0; i < 8; i++) {
		FHVec3Transform(&v, &frustum->corners[i], &view);
		maxX = v.x > maxX ? v.x : maxX;
		maxY = v.y > maxY ? v.y : maxY;
		maxZ = v.z > maxZ ? v.z : maxZ;
		minX = v.x < minX ? v.x : minX;
		minY = v.y < minY ? v.y : minY;
		minZ = v.z < minZ ? v.z : minZ;
	}

	Matrix proj;
	FHMatrixOrthoRH(&proj, maxX - minX, maxY - minY, (maxZ - minZ) * -2.0f + minZ, maxZ);
	FHMatrixMultiply(out, &view, &proj);
}

void UMathUtils::fillExp2FogParamsConstant(float *out, const Matrix *view, float fogStart, float fogEnd, float density)
{
	float k = -density / (fogEnd - fogStart);
	out[0] = k * view->_31;
	out[1] = k * view->_32;
	out[2] = k * view->_33;
	out[3] = k * (fogStart + view->_34);
}

void UMathUtils::scalingFromMatrix(const Matrix *m, _SVector3 *scaling)
{
	scaling->x = sqrtf(m->_11 * m->_11 + m->_12 * m->_12 + m->_13 * m->_13);
	scaling->y = sqrtf(m->_21 * m->_21 + m->_22 * m->_22 + m->_23 * m->_23);
	scaling->z = sqrtf(m->_31 * m->_31 + m->_32 * m->_32 + m->_33 * m->_33);
}

_SQuater *UMathUtils::rotationFromMatrix(const Matrix *m, _SQuater *rotation)
{
	Matrix r(*m);
	_SVector3 s;
	scalingFromMatrix(m, &s);
	for (int j = 0; j < 3; j++) {
		r.m[0][j] /= s.x;
		r.m[1][j] /= s.y;
		r.m[2][j] /= s.z;
	}
	FHQuaternionRotationMatrix(rotation, &r);
	return rotation;
}

STransformParts *UMathUtils::transformsFromMatrix(const Matrix *m, STransformParts *parts)
{
	rotationFromMatrix(m, &parts->rotation);
	scalingFromMatrix(m, &parts->scaling);
	parts->translation = Vector3(m->_41, m->_42, m->_43);
	return parts;
}

Matrix *UMathUtils::matrixRotationAxis(Matrix *out, const Vector3 *axis, float angle)
{
	Vector3 n;
	FHVec3Normalize(&n, axis);
	float c = cosf(angle), s = sinf(angle);
	float xy = n.y * n.x - c * n.y * n.x;
	float xz = n.x * n.z - c * n.x * n.z;
	float yz = n.y * n.z - c * n.y * n.z;
	out->_11 = n.x * n.x + (1 - n.x * n.x) * c;
	out->_12 = xy - n.z * s;
	out->_13 = s * n.y + xz;
	out->_14 = 0;
	out->_21 = xy + s * n.z;
	out->_22 = (1 - n.y * n.y) * c + n.y * n.y;
	out->_23 = yz - n.x * s;
	out->_24 = 0;
	out->_31 = xz - n.y * s;
	out->_32 = yz + s * n.x;
	out->_33 = (1 - n.z * n.z) * c + n.z * n.z;
	out->_34 = 0;
	out->_41 = out->_42 = out->_43 = 0;
	out->_44 = 1;
	return out;
}

// The rotation taking the unit vector from onto the unit vector to.
Matrix *UMathUtils::matrixRotationVectors(Matrix *out, const Vector3 *from, const Vector3 *to)
{
	Vector3 v;
	FHVec3Cross(&v, to, from);
	float e = from->dot(*to);
	float h = v.lengthSq();
	if (h > 1e-6f)
		h = (1 - e) / h;
	float hxy = v.x * v.y * h, hxz = v.x * v.z * h, hyz = v.y * v.z * h;
	out->_11 = v.x * v.x * h + e;
	out->_12 = hxy - v.z;
	out->_13 = hxz + v.y;
	out->_14 = 0;
	out->_21 = hxy + v.z;
	out->_22 = v.y * v.y * h + e;
	out->_23 = hyz - v.x;
	out->_24 = 0;
	out->_31 = hxz - v.y;
	out->_32 = hyz + v.x;
	out->_33 = e + h * v.z * v.z;
	out->_34 = 0;
	out->_41 = out->_42 = out->_43 = 0;
	out->_44 = 1;
	return out;
}

// A Catmull-Rom style curve through p1 and p2 with tangents p2 - p0 and p3 - p1.
void UMathUtils::vec3SplineInterpolation(Vector3 *out, float t, const Vector3 &p0, const Vector3 &p1, const Vector3 &p2, const Vector3 &p3)
{
	Vector3 t1 = p2 - p0;
	Vector3 t2 = p3 - p1;
	FHVec3Hermite(out, &p1, &t1, &p2, &t2, t);
}

void UMathUtils::quaternionSplineInterpolation(Quater *out, float t, const Quater &q0, const Quater &q1, const Quater &q2, const Quater &q3)
{
	Vector3 axis0, axis1, axis2, axis3, axis;
	float a0, a1, a2, a3;
	FHQuaternionToAxisAngle(&q0, &axis0, &a0);
	FHQuaternionToAxisAngle(&q1, &axis1, &a1);
	FHQuaternionToAxisAngle(&q2, &axis2, &a2);
	FHQuaternionToAxisAngle(&q3, &axis3, &a3);
	vec3SplineInterpolation(&axis, t, axis0, axis1, axis2, axis3);
	float t2 = t * t, t3 = t * t2;
	FHQuaternionRotationAxis(out, &axis,
	                         (a3 - a1) * (t3 - t2) * 0.5f + (a2 - a0) * (t2 * -2 + t3 + t) * 0.5f +
	                         a1 * (t3 + t3 + t2 * -3 + 1) + a2 * (t3 * -2 + t2 * 3));
}

// Noise

static int clampTableSize(int size)
{
	if (size <= 1)
		return 2;
	return size > 0x100 ? 0x100 : size;
}

static void shuffle(int *table, int size)
{
	for (int i = 0; i < size; i++)
		table[i] = i;
	for (int i = size - 1; i >= 0; i--) {
		int t = table[i];
		int j = (int)(((float)size - 0.01f) * (float)rand() * 4.656613e-10f + 0.0f);
		table[i] = table[j];
		table[j] = t;
	}
}

// A random value in -1..1.
static float randomGradient()
{
	float r = (float)rand();
	return (r + r) * 4.656613e-10f - 1.0f;
}

void UMathUtils::initNoise1D(int size)
{
	size = clampTableSize(size);
	_kf1DTableSize = (float)size;
	_k1DTableSize = size;
	shuffle(_sIdxTable_1D, size);
	_sIdxTable_1D[size] = _sIdxTable_1D[0];
	for (int i = 0; i < size; i++)
		_sGradsTable_1D[i] = randomGradient();
	_s1dNoiseWasInitializedOnce = true;
}

void UMathUtils::initNoise2D(int size)
{
	size = clampTableSize(size);
	_kf2DTableSize = (float)size;
	_k2DTableSize = size;
	shuffle(_sIdxTable_2D, size);
	for (int i = 0; i < size; i++) {
		_sGradsTable_2D[i * 2] = randomGradient();
		_sGradsTable_2D[i * 2 + 1] = randomGradient();
	}
	_s2dNoiseWasInitializedOnce = true;
}

void UMathUtils::initNoise3D(int size)
{
	size = clampTableSize(size);
	_kf3DTableSize = (float)size;
	_k3DTableSize = size;
	shuffle(_sIdxTable_3D, size);
	for (int i = 0; i < size; i++) {
		_sGradsTable_3D[i * 3] = randomGradient();
		_sGradsTable_3D[i * 3 + 1] = randomGradient();
		_sGradsTable_3D[i * 3 + 2] = randomGradient();
	}
	_s3dNoiseWasInitializedOnce = true;
}

static inline float smoothStep(float f)
{
	return f * f * (f * -2 + 3);
}

// x must not be negative; the noise repeats every 1.
float UMathUtils::noise1D(float x)
{
	if (x < 0)
		LF_RAISE(LF::WrongConditionException());
	if (!_s1dNoiseWasInitializedOnce)
		initNoise1D(_k1DTableSize);
	int i = (int)(x * _kf1DTableSize);
	float f = x * _kf1DTableSize - (float)i;
	float s = smoothStep(f);
	i %= _k1DTableSize;
	return f * _sGradsTable_1D[_sIdxTable_1D[i]] * (1 - s) +
	       s * (f - 1) * _sGradsTable_1D[_sIdxTable_1D[i + 1]];
}

float UMathUtils::noise2D(const Vector2 &v)
{
	if (!_s2dNoiseWasInitializedOnce)
		initNoise2D(_k2DTableSize);
	const int n = _k2DTableSize;
	int i = (int)(v.x * _kf2DTableSize);
	int j = (int)(v.y * _kf2DTableSize);
	float fx = v.x * _kf2DTableSize - (float)i;
	float fy = v.y * _kf2DTableSize - (float)j;
	float sx = smoothStep(fx), sy = smoothStep(fy);
	int r0 = _sIdxTable_2D[i % n], r1 = _sIdxTable_2D[(i + 1) % n];
	const float *g00 = &_sGradsTable_2D[_sIdxTable_2D[(r0 + j) % n] * 2];
	const float *g10 = &_sGradsTable_2D[_sIdxTable_2D[(j + r1) % n] * 2];
	const float *g01 = &_sGradsTable_2D[_sIdxTable_2D[(r0 + j + 1) % n] * 2];
	const float *g11 = &_sGradsTable_2D[_sIdxTable_2D[(j + r1 + 1) % n] * 2];
	float n00 = fx * g00[0] + fy * g00[1];
	float n10 = (fx - 1) * g10[0] + fy * g10[1];
	float n01 = fx * g01[0] + (fy - 1) * g01[1];
	float n11 = (fx - 1) * g11[0] + (fy - 1) * g11[1];
	return (n00 * (1 - sx) + n10 * sx) * (1 - sy) + sy * ((1 - sx) * n01 + sx * n11);
}

float UMathUtils::noise3D(const Vector3 &v)
{
	if (!_s3dNoiseWasInitializedOnce)
		initNoise3D(_k3DTableSize);
	const int n = _k3DTableSize;
	int i = (int)(v.x * _kf3DTableSize);
	int j = (int)(v.y * _kf3DTableSize);
	int k = (int)(v.z * _kf3DTableSize);
	float fx = v.x * _kf3DTableSize - (float)i;
	float fy = v.y * _kf3DTableSize - (float)j;
	float fz = v.z * _kf3DTableSize - (float)k;
	float sx = smoothStep(fx), sy = smoothStep(fy), sz = smoothStep(fz);

	// The gradient at corner (i + di, j + dj, k + dk), dotted with the offset
	// from that corner.
	auto corner = [&](int di, int dj, int dk) {
		int h = (k + _sIdxTable_3D[(j + _sIdxTable_3D[(i + di) % n] + dj) % n] + dk) % n;
		const float *g = &_sGradsTable_3D[h * 3];
		return (fx - di) * g[0] + (fy - dj) * g[1] + (fz - dk) * g[2];
	};
	float n000 = corner(0, 0, 0), n100 = corner(1, 0, 0), n010 = corner(0, 1, 0), n110 = corner(1, 1, 0);
	float n001 = corner(0, 0, 1), n101 = corner(1, 0, 1), n011 = corner(0, 1, 1), n111 = corner(1, 1, 1);
	return ((n000 * (1 - sx) + n100 * sx) * (1 - sy) + (n010 * (1 - sx) + n110 * sx) * sy) * (1 - sz) +
	       ((sx * n111 + n011 * (1 - sx)) * sy + (n001 * (1 - sx) + n101 * sx) * (1 - sy)) * sz;
}

// The sum of harmonics octaves, each at twice the frequency and persistence
// times the amplitude of the one before.
float UMathUtils::noise2D_harm(const Vector2 &v, int harmonics, float persistence)
{
	Vector2 p(v);
	float sum = 0, amplitude = 1;
	for (int i = 0; i < harmonics; i++) {
		sum = noise2D(p) * amplitude + sum;
		p.x = p.x + p.x;
		p.y = p.y + p.y;
		amplitude = amplitude * persistence;
	}
	return sum;
}

float UMathUtils::noise3D_harm(const Vector3 &v, int harmonics, float persistence)
{
	Vector3 p(v);
	float sum = 0, amplitude = 1;
	for (int i = 0; i < harmonics; i++) {
		sum = noise3D(p) * amplitude + sum;
		p.x = p.x + p.x;
		p.y = p.y + p.y;
		p.z = p.z + p.z;
		amplitude = amplitude * persistence;
	}
	return sum;
}
