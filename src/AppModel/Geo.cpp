// Units, geographic coordinates and the positions of the earth, sun, moon
// and stars for a date.

#include "AppModel.h"
#include <math.h>

static const Vector3 kEarthRotationVector(0.0f, 0.0f, 1.0f);

float celsiusFromFahrenheit(float f) { return (f - 32.0f) * 0.5555556f; }

float fahrenheitFromCelsius(float c) { return c * 1.8f + 32.0f; }

float feetsFromKilometers(float km) { return km * 3280.8398f; }

float milesFromKilometers(float km) { return km / 1.609f; }

float inchesFromMiliBars(float mb) { return (mb * 29.92f) / 1013.0f; }

float millimetersFromMiliBars(float mb) { return (mb * 29.92f * 25.4f) / 1013.0f; }

TDegrees degreesFromFloat(float value)
{
	TDegrees d;
	float a = fabsf(value);
	d.degrees = floorf(a);
	float minutes = (a - d.degrees) * 60.0f;
	d.seconds = (minutes - floorf(minutes)) * 60.0f;
	d.minutes = floorf(minutes);
	d.isNegative = value < 0.0f;
	return d;
}

float floatFromDegrees(TDegrees d)
{
	float value = d.minutes / 60.0f + d.degrees + d.seconds / 3600.0f;
	return d.isNegative ? -value : value;
}

TDegrees clampDegreesWithPeriod(TDegrees d, float period)
{
	float carry = floorf(d.seconds / 60.0f);
	float minutes = d.minutes + carry;
	float seconds = carry * -60.0f + d.seconds;
	carry = floorf(minutes / 60.0f);
	minutes = carry * -60.0f + minutes;
	float degrees = carry + d.degrees;
	float value = minutes / 60.0f + degrees + seconds / 3600.0f;
	if (d.isNegative)
		value = -value;
	if (period < value)
		degrees = degrees - period * floorf(degrees / period);
	TDegrees r;
	r.degrees = degrees;
	r.minutes = minutes;
	r.seconds = seconds;
	r.isNegative = d.isNegative;
	return r;
}

float deg2radf(float deg) { return deg * 0.017453292f; }

float rad2degf(float rad) { return rad * 57.29578f; }

float windAngleFromWindDir(Vector2 dir)
{
	dir = -dir;
	float angle;
	if (dir.y == 0.0f) {
		angle = dir.x < 0.0f ? 4.712389f : 1.5707964f;
	}
	else {
		angle = atanf(dir.x / dir.y);
		if (dir.y < 0.0f)
			angle += 3.1415927f;
		if (angle < 0.0f)
			angle += 6.2831855f;
	}
	return angle * 57.29578f;
}

Vector2 windDirFromWindAngle(float angle)
{
	float c = cosf(angle * 0.017453292f);
	float s = sinf(angle * 0.017453292f);
	return Vector2(-s, -c);
}

// Longitudes wrap into -180..180.
static inline float wrapLongitude(float lon)
{
	if (lon > 180.0f)
		lon -= 360.0f;
	else if (lon < -180.0f)
		lon += 360.0f;
	return lon;
}

LPoint geoPointFromMapPoint(LPoint p)
{
	LPoint g = {wrapLongitude((p.x - 0.5f) * 360.0f), (p.y - 0.5f) * 180.0f};
	return g;
}

LPoint mapPointFromGeoPoint(LPoint p)
{
	LPoint m = {p.x * 0.0027777778f + 0.5f, p.y * 0.0055555557f + 0.5f};
	return m;
}

LPoint mapPointFromEarthVec(const Vector3 *v)
{
	float z = v->z;
	if (z < -1.0f)
		z = -1.0f;
	else if (z > 1.0f)
		z = 1.0f;
	float x = v->x, y = v->y;
	double s = (double)y / sqrt((double)x * x + (double)y * y);
	if (s < -1.0)
		s = -1.0;
	else if (s > 1.0)
		s = 1.0;
	float lon = (float)asin(s);
	float lat = asinf(z);
	if (x < 0.0f)
		lon = (y <= 0.0f ? -3.1415927f : 3.1415927f) - lon;
	LPoint m = {lon * 0.5f * 0.31830987f + 0.5f, lat * 0.31830987f + 0.5f};
	return m;
}

LPoint geoPointFromEarthVec(const Vector3 *v) { return geoPointFromMapPoint(mapPointFromEarthVec(v)); }

Vector3 earthVecFromMapPoint(LPoint p)
{
	float lat = (p.y - 0.5f) * 3.1415927f;
	double lon = (double)(((p.x - 0.5f) + (p.x - 0.5f)) * 3.1415927f);
	float c = cosf(lat);
	return Vector3(c * (float)cos(lon), (float)sin(lon) * c, sinf(lat));
}

Vector3 earthVecFromGeoPoint(LPoint p)
{
	float c = cosf(p.y * 0.017453292f);
	double lon = (double)(p.x * 0.017453292f);
	return Vector3(c * (float)cos(lon), (float)sin(lon) * c, sinf(p.y * 0.017453292f));
}

Vector3 earthVecFromGeoPoint_e(LPoint p)
{
	float lon2 = p.x * 0.017453292f * p.x * 0.017453292f;
	float lat2 = p.y * 0.017453292f * p.y * 0.017453292f;
	float cosLat = lat2 * -0.5f + 1.0f + (lat2 * lat2) / 24.0f + (lat2 * lat2 * lat2) / -720.0f;
	float cosLon = lon2 * -0.5f + 1.0f + (lon2 * lon2) / 24.0f + (lon2 * lon2 * lon2) / -720.0f;
	return Vector3(cosLat * cosLon, sqrtf(1.0f - cosLon * cosLon) * cosLat, sqrtf(1.0f - cosLat * cosLat));
}

LRect mapRectFromGeoRect(LRect r)
{
	LRect m = {{r.origin.x * 0.0027777778f + 0.5f, r.origin.y * 0.0055555557f + 0.5f},
	           {r.size.width * 0.0027777778f, r.size.height * 0.0055555557f}};
	return m;
}

LRect geoRectFromMapRect(LRect r)
{
	LRect g = {{wrapLongitude((r.origin.x - 0.5f) * 360.0f), (r.origin.y - 0.5f) * 180.0f},
	           {r.size.width * 360.0f, r.size.height * 180.0f}};
	return g;
}

bool pointInRect(LPoint p, LRect r)
{
	return !(p.x < r.origin.x || r.origin.x + r.size.width < p.x || p.y < r.origin.y ||
	         r.origin.y + r.size.height < p.y);
}

void planeFromPointAndNormal(Plane *plane, const Vector3 *point, const Vector3 *normal)
{
	plane->a = normal->x;
	plane->b = normal->y;
	plane->c = normal->z;
	plane->d = point->x * normal->x + point->y * normal->y + point->z * normal->z;
}

LSize radiansFromDegreesSize(LSize s)
{
	LSize r = {s.width * 0.017453292f, s.height * 0.017453292f};
	return r;
}

LPoint radiansFromDegreesPoint(LPoint p)
{
	LPoint r = {p.x * 0.017453292f, p.y * 0.017453292f};
	return r;
}

LPoint degreesFromRadiansPoint(LPoint p)
{
	LPoint r = {p.x * 57.29578f, p.y * 57.29578f};
	return r;
}

float distEstimationFromEarthVecs(const Vector3 *a, const Vector3 *b)
{
	return -(a->x * b->x + a->y * b->y + a->z * b->z);
}

float distanceOnEarthFromEarthVecs(const Vector3 *a, const Vector3 *b)
{
	_SDVector3 da = {a->x, a->y, a->z};
	_SDVector3 db = {b->x, b->y, b->z};
	FHDVec3Normalize(&da, &da);
	FHDVec3Normalize(&db, &db);
	double c = da.x * db.x + da.y * db.y + da.z * db.z;
	if (c < -1.0)
		c = -1.0;
	else if (c > 1.0)
		c = 1.0;
	return (float)(acos(c) * 6371.10009765625);
}

float distanceOnEarthFromGeoPoints(LPoint a, LPoint b)
{
	Vector3 va = earthVecFromGeoPoint(a);
	Vector3 vb = earthVecFromGeoPoint(b);
	return distanceOnEarthFromEarthVecs(&va, &vb);
}

// Star colors by B-V color index, -0.4 to 2.0 in steps of 0.05, with the
// black body temperatures they stand for.
struct SBVColor {
	float bv;
	int temperature;
	unsigned int rgb;
};

static const SBVColor *s_bv(int *count)
{
	static const SBVColor s_bv_dat[] = {
	    {-0.4f, 113017, 0xffb29b}, {-0.35f, 56701, 0xffb59e}, {-0.3f, 33605, 0xffb9a3}, {-0.25f, 22695, 0xffbfaa},
	    {-0.2f, 16954, 0xffc5b2},  {-0.15f, 13674, 0xffccbb}, {-0.1f, 11677, 0xffd2c4}, {-0.05f, 10395, 0xffd8cc},
	    {0.0f, 9531, 0xffddd3},    {0.05f, 8917, 0xffe2da},   {0.1f, 8455, 0xffe5df},   {0.15f, 8084, 0xffe9e4},
	    {0.2f, 7767, 0xffece9},    {0.25f, 7483, 0xffefee},   {0.3f, 7218, 0xfff2f3},   {0.35f, 6967, 0xfff6f8},
	    {0.4f, 6728, 0xfff9fe},    {0.45f, 6500, 0xfbf9ff},   {0.5f, 6285, 0xf5f7ff},   {0.55f, 6082, 0xeff5ff},
	    {0.6f, 5895, 0xeaf3ff},    {0.65f, 5722, 0xe5f1ff},   {0.7f, 5563, 0xe0efff},   {0.75f, 5418, 0xdbedff},
	    {0.8f, 5286, 0xd6ebff},    {0.85f, 5164, 0xd2e9ff},   {0.9f, 5052, 0xcee8ff},   {0.95f, 4948, 0xcae6ff},
	    {1.0f, 4849, 0xc6e5ff},    {1.05f, 4755, 0xc3e3ff},   {1.1f, 4664, 0xbfe2ff},   {1.15f, 4576, 0xbbe0ff},
	    {1.2f, 4489, 0xb8dfff},    {1.25f, 4405, 0xb4ddff},   {1.3f, 4322, 0xb0dbff},   {1.35f, 4241, 0xaddaff},
	    {1.4f, 4159, 0xa9d8ff},    {1.45f, 4076, 0xa5d6ff},   {1.5f, 3989, 0xa1d5ff},   {1.55f, 3892, 0x9cd2ff},
	    {1.6f, 3779, 0x96d0ff},    {1.65f, 3640, 0x8fccff},   {1.7f, 3463, 0x85c8ff},   {1.75f, 3234, 0x78c1ff},
	    {1.8f, 2942, 0x65b7ff},    {1.85f, 2579, 0x4ba9ff},   {1.9f, 2150, 0x2395ff},   {1.95f, 1675, 0x007bff},
	    {2.0f, 1195, 0x0052ff},
	};
	*count = sizeof(s_bv_dat) / sizeof(s_bv_dat[0]);
	return s_bv_dat;
}

unsigned int colorFromBV(float bv, float brightness)
{
	int count;
	const SBVColor *table = s_bv(&count);
	if (!table)
		return 0xffffffff;
	int i = (int)((bv + 0.4f) * 20.0f);
	if (i < 0)
		i = 0;
	else if (i >= count)
		i = count - 1;
	if (brightness < 0.0f)
		brightness = 0.0f;
	else if (brightness > 1.0f)
		brightness = 1.0f;
	float alpha = brightness * 255.1f;
	if (alpha <= 0.0f)
		alpha = 0.0f;
	return (table[i].rgb & 0xffffff) | ((unsigned int)alpha << 24);
}

float planetMagnitude(Vector3 &planet, Vector3 &observer, float absMagnitude)
{
	Vector3 d = planet - observer;
	double distances = (double)(observer.x * observer.x + observer.y * observer.y + observer.z * observer.z) *
	                   (double)(d.x * d.x + d.y * d.y + d.z * d.z);
	double c = (double)(d.x * observer.x + d.y * observer.y + d.z * observer.z) / sqrt(distances);
	if (c < -1.0)
		c = -1.0;
	else if (c > 1.0)
		c = 1.0;
	// The phase function of a lambertian sphere.
	double phase = log((sqrt(1.0 - c * c) + (3.141592653589793 - acos(c)) * c) / distances);
	return (float)((double)absMagnitude - phase / log(2.512));
}

Quater equatorToSphereRotation(LDate *date)
{
	int offset = date->timeZone()->secondsFromGMTForDate(date);
	double dayPhase = (double)date->hour() / 24.0 + (double)date->minute() / 1440.0 +
	                  (double)(int)date->second() / 86400.0 + (double)offset / -86400.0;
	double yearPhase = date->timeIntervalSince1970() / UPlanets::period(UPlanets::kEarth) + 0.0272;
	double angle = (dayPhase - floor(dayPhase)) + (yearPhase - floor(yearPhase)) + 0.25;
	Quater q;
	FHQuaternionRotationAxis(&q, &kEarthRotationVector, -(float)((angle + angle) * 3.141592653589793));
	return q;
}

// The precession of the equinoxes turns the axis of the ecliptic's tilt once
// in 25770 years.
Quater eclipticToEquatorRotation(LDate *date)
{
	double precession = (UPlanets::j2000Time(date) / 31557600.0) / 25770.0;
	precession = (precession + precession) * 3.141592653589793;
	Vector3 axis((float)cos(precession), -(float)sin(precession), 0.0f);
	Quater q;
	FHQuaternionRotationAxis(&q, &axis, 0.41015238f);
	return q;
}

Quater planetsRotation_EarthSystem(LDate *date)
{
	Quater ecliptic = eclipticToEquatorRotation(date);
	Quater equator = equatorToSphereRotation(date);
	Quater q;
	FHQuaternionMultiply(&q, &ecliptic, &equator);
	return q;
}

Vector3 earthVecFromEclipticVec(const Vector3 *v, LDate *date)
{
	Quater q = planetsRotation_EarthSystem(date);
	_SMatrix m;
	FHMatrixRotationQuaternion(&m, &q);
	_SVector4 r;
	FHVec3Transform(&r, v, &m);
	return Vector3(r.x, r.y, r.z);
}

Vector3 moonPosition_EarthSystem(LDate *date)
{
	double x, y, z;
	UPlanets::moonPosition(date, &x, &y, &z);
	Vector3 v((float)x, (float)y, (float)z);
	return earthVecFromEclipticVec(&v, date);
}

Vector3 sunPosition_EarthSystem(LDate *date)
{
	double x, y, z;
	UPlanets::planetPosition(UPlanets::kEarth, date, &x, &y, &z);
	Vector3 v((float)x, (float)y, (float)z);
	return earthVecFromEclipticVec(&v, date);
}

Quater starsRotation_EarthSystem(LDate *date)
{
	Quater q = eclipticToEquatorRotation(date);
	FHQuaternionConjugate(&q, &q);
	Quater ecliptic = eclipticToEquatorRotation(date);
	Quater equator = equatorToSphereRotation(date);
	Quater t;
	FHQuaternionMultiply(&t, &q, &ecliptic);
	FHQuaternionMultiply(&q, &t, &equator);
	return q;
}
