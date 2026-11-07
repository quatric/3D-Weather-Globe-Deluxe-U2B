// SphereLOD: the globe's patch grid at six levels. Level l has n rows of 2n
// patches (n = 1, 3, 9, 18, 54, 162), each spanning pi/n in latitude and
// longitude, row 0 at the south pole. The tables are filled once, when the
// library loads.

#include "AppModel.h"

int SphereLOD::sOffsetForLOD[8] = { 0, 1, 10, 91, 415, 3331, 0, 0 };
int SphereLOD::sItemsCountForLOD[8] = { 1, 3, 9, 18, 54, 162, 0, 0 };

static float sSinArrayLevel0[3], sSinArrayLevel1[7], sSinArrayLevel2[19], sSinArrayLevel3[37];
static float sSinArrayLevel4[109], sSinArrayLevel5[325];
static float sCosArrayLevel0[3], sCosArrayLevel1[7], sCosArrayLevel2[19], sCosArrayLevel3[37];
static float sCosArrayLevel4[109], sCosArrayLevel5[325];

float *SphereLOD::sSinArraysByLevel[kLevelsCount] = { sSinArrayLevel0, sSinArrayLevel1, sSinArrayLevel2,
	                                                  sSinArrayLevel3, sSinArrayLevel4, sSinArrayLevel5 };
float *SphereLOD::sCosArraysByLevel[kLevelsCount] = { sCosArrayLevel0, sCosArrayLevel1, sCosArrayLevel2,
	                                                  sCosArrayLevel3, sCosArrayLevel4, sCosArrayLevel5 };
Vector3 SphereLOD::sFaceNormals[kPatchesCount];
Vector3 SphereLOD::sMinCorners[kPatchesCount];
Vector3 SphereLOD::sMaxCorners[kPatchesCount];
int SphereLOD::sPatchIBOffsets[kPatchesCount];
Matrix SphereLOD::sMaxLevelMatrices[162 * 324];
SphereLOD SphereLOD::sInst;

static inline int rowsForLevel(int level)
{
	return level >= 0 && level < SphereLOD::kLevelsCount ? SphereLOD::sItemsCountForLOD[level] : 1;
}

// The patch's rotation from the north pole: around the pole to its column,
// then down to its face normal.
void SphereLOD::entryRotationMatrix(const STextureID &id, Matrix *m)
{
	float step = (float)M_PI / (float)sItemsCountForLOD[(int)id.level];
	const Vector3 &normal = sFaceNormals[_indexOf(id)];
	Vector3 pole(0, 0, 1);
	Quater around, down, q;
	FHQuaternionRotationAxis(&around, &pole, (float)id.x * step + step * 0.5f + (float)M_PI_2);
	Vector3 axis;
	FHVec3Cross(&axis, &pole, &normal);
	FHVec3Normalize(&axis, &axis);
	FHQuaternionRotationAxis(&down, &axis, acosf(pole.x * normal.x + pole.y * normal.y + pole.z * normal.z));
	FHQuaternionMultiply(&q, &around, &down);
	FHMatrixRotationQuaternion(m, &q);
}

void SphereLOD::_initTables()
{
	static bool wasInitialized = false;
	if (wasInitialized)
		return;
	for (int level = 0; level < kLevelsCount; level++) {
		int n = sItemsCountForLOD[level];
		float fn = (float)n;
		int index = sOffsetForLOD[level] * 2;
		float step = (float)M_PI / fn;
		float edge = (float)(2.0 * sin((double)(step * 0.5f)));	// the chord of a patch side
		float *sins = sSinArraysByLevel[level];
		float *coss = sCosArraysByLevel[level];
		sins[2 * n] = 0;
		coss[2 * n] = 1;
		for (int row = 0; row < n; row++) {
			float a = step * (float)row;
			sins[2 * row] = sinf(a + a);
			coss[2 * row] = cosf(a + a);
			sins[2 * row + 1] = sinf(step + a + a);
			coss[2 * row + 1] = cosf(step + a + a);

			float lat0 = a - (float)M_PI_2;
			float sinLat0 = sinf(lat0);
			float sinLat1 = sinf(step + lat0);
			float cosLat0 = cosf(lat0);
			float cosLat1 = cosf(step + lat0);
			float minCosLat = cosLat0, maxCosLat = cosLat1;
			if (maxCosLat < minCosLat)
				std::swap(minCosLat, maxCosLat);
			int half = n >> 1;
			if (row == half && (n & 1) != 0)
				maxCosLat = 1;	// the row on the equator
			float rowScale = edge * maxCosLat;
			for (int column = 0; column < 2 * n; column++) {
				float lon = step * (float)column;
				float cosLon0 = cosf(lon), cosLon1 = cosf(step + lon);
				float minCosLon = cosLon0, maxCosLon = cosLon1;
				if (maxCosLon < minCosLon)
					std::swap(minCosLon, maxCosLon);
				float sinLon0 = sinf(lon), sinLon1 = sinf(step + lon);
				float minSinLon = sinLon0, maxSinLon = sinLon1;
				if (maxSinLon < minSinLon)
					std::swap(minSinLon, maxSinLon);
				if (column == half && (n & 1) != 0)
					maxSinLon = 1;
				if (column == n + half && (n & 1) != 0)
					minSinLon = -1;

				int i = index + column;
				Vector3 &lo = sMinCorners[i];
				lo.x = (minCosLon < 0 ? maxCosLat : minCosLat) * minCosLon;
				lo.y = (minSinLon < 0 ? maxCosLat : minCosLat) * minSinLon;
				lo.z = sinLat0;
				Vector3 &hi = sMaxCorners[i];
				hi.x = (maxCosLon > 0 ? maxCosLat : minCosLat) * maxCosLon;
				hi.y = (maxSinLon > 0 ? maxCosLat : minCosLat) * maxSinLon;
				hi.z = sinLat1;

				// The normal of the patch's plane, from the corner nearest
				// to the equator.
				Vector3 p0(cosLat0 * cosLon0, cosLat0 * sinLon0, sinLat0);
				Vector3 normal;
				if (sinLat0 <= 0) {
					Vector3 diagonal(cosLat1 * cosLon1 - p0.x, sinLon1 * cosLat1 - p0.y, sinLat1 - sinLat0);
					Vector3 up(cosLat1 * cosLon0 - p0.x, sinLon0 * cosLat1 - p0.y, sinLat1 - sinLat0);
					FHVec3Cross(&normal, &diagonal, &up);
				} else {
					Vector3 up(cosLat1 * cosLon0 - p0.x, sinLon0 * cosLat1 - p0.y, sinLat1 - sinLat0);
					Vector3 east(cosLat0 * cosLon1 - p0.x, sinLon1 * cosLat0 - p0.y, sinLat0 - sinLat0);
					FHVec3Cross(&normal, &east, &up);
				}
				UMathUtils::normalize(&normal);
				sFaceNormals[i] = normal;

				// The patch's indices in the index buffer: its place in the
				// level-4 patch it is part of, down to level 0.
				int offset = 0, r = row, c = column;
				for (int l = level; l >= 0; l--) {
					switch (l) {
					case 0:
						offset += (c % 2) * 0xb64;
						break;
					case 1:
						offset += (c % 3 + (r % 3) * 3) * 0x144;
						r /= 3;
						c /= 3;
						break;
					case 2:
						offset += (c % 3 + (r % 3) * 3) * 0x24;
						r /= 3;
						c /= 3;
						break;
					case 3:
						offset += (c % 2 + (r % 2) * 2) * 9;
						c /= 2;
						r /= 2;
						break;
					case 4:
						offset += c % 3 + (r % 3) * 3;
						r /= 3;
						c /= 3;
						break;
					}
				}
				sPatchIBOffsets[i] = offset;

				if (level == kMaxLevel) {
					STextureID id = { (short)column, (short)row, kMaxLevel };
					Matrix *m = &sMaxLevelMatrices[row * 2 * n + column];
					entryRotationMatrix(id, m);
					m->_11 *= rowScale;
					m->_12 *= rowScale;
					m->_13 *= rowScale;
					m->_21 *= edge;
					m->_22 *= edge;
					m->_23 *= edge;
					Vector3 center = earthVecFromMapPoint(LMakePoint(((float)column * 0.5f) / fn + 0.5f, (float)row / fn));
					m->_41 = center.x;
					m->_42 = center.y;
					m->_43 = center.z;
				}
			}
			index += 2 * n;
		}
	}
	wasInitialized = true;
}

// Only the polar rows of the two top levels have no high resolution images.
bool SphereLOD::doAllowHiResDrawForID(const STextureID &id)
{
	if (id.level == 4)
		return (unsigned short)(id.y - 3) <= 0x2f;
	if (id.level == 5)
		return (unsigned short)(id.y - 9) <= 0x8f;
	return true;
}

float SphereLOD::farClipDistanceForAltitude(float altitude)
{
	return sqrtf((altitude + 2.0f) * altitude);
}

// The patch's width at its southern and northern borders, as a trapezoid:
// x is the difference, y the southern one, relative to the wider.
void SphereLOD::entryTrapetionParams(const STextureID &id, Vector2 *params)
{
	float n = (float)rowsForLevel(id.level);
	const float *sins = sSinArraysByLevel[(int)id.level];
	float edge = (float)(2.0 * sin((double)((float)M_PI_2 / n)));
	float south = edge * sins[id.y];
	float north = edge * sins[id.y + 1];
	float wider = south <= north ? north : south;
	params->y = south / wider;
	params->x = (north - south) / wider;
}

// The patch's indices are `count` from `offset` in the level's index buffer.
// (Level 5 uses level 4's; `count` is left alone for it.)
void SphereLOD::getPatchIBParameters(const STextureID &id, int *count, int *offset)
{
	*offset = sPatchIBOffsets[_indexOf(id)];
	switch (id.level) {
	case 0:
		*count = 0xb64;
		break;
	case 1:
		*count = 0x144;
		break;
	case 2:
		*count = 0x24;
		break;
	case 3:
		*count = 9;
		break;
	case 4:
		*count = 1;
		break;
	}
}

const Vector3 *SphereLOD::faceNormal(const STextureID &id)
{
	return &sFaceNormals[_indexOf(id)];
}

// How far the camera sees past the globe's limb.
float SphereLOD::farClipValueForCameraParams(const Vector3 *camera, const Vector3 *direction, float altitude)
{
	Vector3 d = *camera;
	UMathUtils::normalize(&d);
	float a = acosf(-(d.x * direction->x + d.y * direction->y + d.z * direction->z));
	float b = (float)asin((double)(-1.0f / (altitude + 1.0f)));
	float c = (float)cos((double)(a + b));
	float far = sqrtf(altitude * altitude + altitude + altitude) * c;
	return far <= altitude ? altitude : far;
}

SphereLOD::SCameraParams::SCameraParams(const Vector3 *p)
{
	textureID.x = textureID.y = 0;
	textureID.level = 0;
	position = *p;
	distance = sqrtf(p->x * p->x + p->y * p->y + p->z * p->z);
	float k = 1.0f / distance;
	direction.x = k * p->x;
	direction.y = k * p->y;
	direction.z = k * p->z;
	sinLatitude = direction.z;
	cosLatitude = sqrtf(1.0f - direction.z * direction.z);
	latitude = asinf(direction.z);
	float r = sqrtf(direction.x * direction.x + direction.y * direction.y);
	float s = r != 0 ? direction.y / r : 0;
	if (s > 1)
		s = 1;
	if (!(-1 <= s))
		s = -1;
	sinLongitude = s;
	longitude = asinf(s);
	if (direction.x < 0)
		longitude = (float)M_PI - longitude;
	if (longitude < 0)
		longitude += (float)(2 * M_PI);
	cosLongitude = cosf(longitude);
	for (int level = 0; level < kLevelsCount; level++) {
		float step = (float)M_PI / (float)sItemsCountForLOD[level];
		columns[level] = (int)(longitude / step);
		columnHalf[level] = ((int)((longitude + longitude) / step) & 1) != 0;
		rows[level] = (int)((latitude + (float)M_PI_2) / step);
		rowHalf[level] = ((int)((latitude + (float)M_PI_2) / (step * 0.5f)) & 1) != 0;
	}
}

void SphereLOD::fillVisibleEntries(LPoint geoPoint, float altitude, TArray<STextureID> *entries)
{
	entries->removeAllObjects();
	int n = sItemsCountForLOD[kMaxLevel];
	Vector3 camera = earthVecFromGeoPoint(geoPoint) * ((altitude < 0.00015f ? 0.00015f : altitude) + 1.0f);
	int index = sOffsetForLOD[kMaxLevel] * 2;
	for (int row = 0; row < n; row++) {
		for (int column = 0; column < 2 * n; column++, index++) {
			const Vector3 &normal = sFaceNormals[index];
			float c = camera.x * normal.x + camera.y * normal.y + camera.z * normal.z;
			Vector3 d = camera - normal;
			if (0.05f <= (c - 1.0f) / sqrtf(d.x * d.x + d.y * d.y + d.z * d.z)) {
				STextureID id = { (short)column, (short)row, kMaxLevel };
				entries->addObject(id);
			}
		}
	}
}

bool SphereLOD::getEntryViewParams(const STextureID &id, float height, const SCameraParams *camera, float *nearDistance,
                                   float *farDistance, float *nearCos, float *farCos)
{
	int level = id.level;
	int cameraColumn = camera->columns[level];
	int cameraRow = camera->rows[level];
	int n = rowsForLevel(level);
	const float *sins = sSinArraysByLevel[level];
	const float *coss = sCosArraysByLevel[level];
	Vector3 nearest(0, 0, 1);
	int column = id.x, row = id.y;
	int farRow;
	if (cameraColumn == column && cameraRow == row) {
		// The camera is above the patch.
		nearest = camera->direction;
		farRow = camera->rowHalf[level] ? row : row + 1;
	} else {
		// The nearest and the farthest border of the patch.
		int nearRow;
		int dx = cameraColumn - column;
		if (dx < -dx)
			dx = -dx;
		if (n <= dx)
			dx = n * 2 - dx;
		bool northern;
		if ((n + 1) >> 1 < dx)
			northern = row > (n >> 1) || (row == (n >> 1) && row < cameraRow);
		else
			northern = row < cameraRow;
		if (northern) {
			nearRow = row + 1;
			farRow = row;
		} else {
			nearRow = row;
			farRow = row + 1;
		}
		float sinNear = sins[nearRow], cosNear = coss[nearRow];
		if (cameraColumn == column) {
			nearest = Vector3(sinNear * camera->cosLongitude, sinNear * camera->sinLongitude, -cosNear);
		} else {
			// The nearest of the patch's meridians.
			int dc = column - cameraColumn;
			if (dc < 0)
				dc += n * 2;
			bool east = dc == n ? camera->columnHalf[level] : dc < n;
			int meridian = column + (east ? 0 : 1);
			if (n * 2 <= meridian)
				meridian -= n * 2;
			float cl = coss[meridian], sl = sins[meridian];
			float z = camera->direction.z;
			float h = cl * camera->direction.x + sl * camera->direction.y;
			float k = 1.0f / sqrtf(h * h + z * z);
			z *= k;
			h *= k;
			if (z > 1)
				z = 1;
			if (!(-1 <= z))
				z = -1;
			bool outside = row != (int)((double)n * (asin((double)z) / 3.1415927410125732 + 0.5));
			if (cameraRow == row) {
				if (outside) {
					h = camera->cosLatitude;
					z = camera->sinLatitude;
				}
				nearest = Vector3(cl * h, sl * h, z);
			} else if (outside) {
				nearest = Vector3(sinNear * cl, sinNear * sl, -cosNear);
			} else {
				nearest = Vector3(cl * h, sl * h, z);
			}
		}
	}
	float d = camera->distance;
	Vector3 p = nearest * (height + 1.0f) - camera->position;
	*nearDistance = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
	float c = nearest.x * camera->position.x + nearest.y * camera->position.y + nearest.z * camera->position.z;
	*nearCos = (float)((double)(c - 1.0f) / sqrt((double)(d * d + 1.0f + c * -1.9999f)));
	// As in the original, the far point is (s*c, s*s, -c) of the far border.
	float s = sins[farRow], co = coss[farRow];
	Vector3 q(s * co, s * s, -co);
	Vector3 e = q - camera->position;
	*farDistance = sqrtf(e.x * e.x + e.y * e.y + e.z * e.z);
	c = q.x * camera->position.x + q.y * camera->position.y + q.z * camera->position.z;
	*farCos = (float)((double)(c - 1.0f) / sqrt((double)(d * d + 1.0f + c * -1.9999f)));
	return 0 < *nearCos;
}

bool SphereLOD::getEntryViewParams(const STextureID &id, float height, const Vector3 *camera, float *nearDistance,
                                   float *farDistance, float *nearCos, float *farCos)
{
	SCameraParams params(camera);
	return getEntryViewParams(id, height, &params, nearDistance, farDistance, nearCos, farCos);
}
