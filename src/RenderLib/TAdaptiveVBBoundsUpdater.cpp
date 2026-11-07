// The bounds of each patch of an adaptive mesh LOD: the leaves (4 x 4 cells)
// from their vertices, each parent from its four children.

#include "RenderLib.h"

template <unsigned char XY, unsigned char Z>
static inline void includeVertex(const char *xy, const char *z, int i, Vector3 *min, Vector3 *max)
{
	if (XY) {
		float x = *(const float *)(xy + i);
		float y = *(const float *)(xy + i + 4);
		if (x < min->x)
			min->x = x;
		if (y < min->y)
			min->y = y;
		if (x > max->x)
			max->x = x;
		if (y > max->y)
			max->y = y;
	}
	if (Z) {
		float v = *(const float *)(z + i);
		if (v < min->z)
			min->z = v;
		if (v > max->z)
			max->z = v;
	}
}

template <unsigned char XY, unsigned char Z>
static inline void includeBounds(Vector3 *min, Vector3 *max, const Vector3 &cmin, const Vector3 &cmax)
{
	if (XY) {
		min->x = min->x <= cmin.x ? min->x : cmin.x;
		min->y = min->y <= cmin.y ? min->y : cmin.y;
		max->x = cmax.x <= max->x ? max->x : cmax.x;
		max->y = cmax.y <= max->y ? max->y : cmax.y;
	}
	if (Z) {
		min->z = min->z <= cmin.z ? min->z : cmin.z;
		max->z = cmax.z <= max->z ? max->z : cmax.z;
	}
}

template <unsigned char XY, unsigned char Z>
void TAdaptiveVBBoundsUpdater<XY, Z>::updateBounds(SBoundCalcParams *p, char *data, long level, long x, long y,
                                                   Vector3 *min, Vector3 *max)
{
	int index = x + UAdaptiveMeshUtils::_offsetForSubEntries[level] + (y << level);
	if (level + 2 < p->lod >> 1) {
		long next = level + 1;
		int half = p->size >> next;
		Vector3 cmin, cmax;
		updateBounds(p, data, next, x * 2, y * 2, min, max);
		updateBounds(p, data + half * p->stride, next, x * 2 + 1, y * 2, &cmin, &cmax);
		includeBounds<XY, Z>(min, max, cmin, cmax);
		updateBounds(p, data + half * p->rowStride * p->stride, next, x * 2, y * 2 + 1, &cmin, &cmax);
		includeBounds<XY, Z>(min, max, cmin, cmax);
		updateBounds(p, data + (half * p->rowStride + half) * p->stride, next, x * 2 + 1, y * 2 + 1, &cmin, &cmax);
		includeBounds<XY, Z>(min, max, cmin, cmax);
	} else {
		const char *xy = data + p->xyOffset;
		const char *z = data + p->zOffset;
		if (XY) {
			min->x = max->x = *(const float *)xy;
			min->y = max->y = *(const float *)(xy + 4);
		}
		if (Z)
			min->z = max->z = *(const float *)z;
		for (int r = 0; r < 5; r++)
			for (int c = 0; c < 5; c++)
				includeVertex<XY, Z>(xy, z, (r * p->rowStride + c) * p->stride, min, max);
		if (p->centers)
			for (int r = 0; r < 4; r++)
				for (int c = 0; c < 4; c++)
					includeVertex<XY, Z>(xy, z, (r * p->rowStride + c + p->size + 1) * p->stride, min, max);
	}
	if (XY) {
		p->mins[index] = Vector2(min->x, min->y);
		p->maxs[index] = Vector2(max->x, max->y);
	}
	if (Z) {
		p->minZ[index] = min->z;
		p->maxZ[index] = max->z;
	}
}

void updateBoundsWhole(int lod, char *data, int stride, int xyOffset, int zOffset, Vector2 *mins, Vector2 *maxs,
                       float *minZ, float *maxZ)
{
	SBoundCalcParams p;
	p.stride = stride;
	p.xyOffset = xyOffset;
	p.zOffset = zOffset;
	p.mins = mins;
	p.maxs = maxs;
	p.minZ = minZ;
	p.maxZ = maxZ;
	p.centers = lod & 1;
	p.size = 1 << (lod >> 1);
	p.rowStride = p.size + 1 + (p.centers ? p.size : 0);
	p.lod = lod;
	Vector3 min(0.0f, 0.0f, 0.0f), max(0.0f, 0.0f, 0.0f);
	bool hasXY = mins && maxs;
	bool hasZ = minZ && maxZ;
	if (hasXY && hasZ)
		TAdaptiveVBBoundsUpdater<1, 1>::updateBounds(&p, data, 0, 0, 0, &min, &max);
	else if (hasXY)
		TAdaptiveVBBoundsUpdater<1, 0>::updateBounds(&p, data, 0, 0, 0, &min, &max);
	else if (hasZ)
		TAdaptiveVBBoundsUpdater<0, 1>::updateBounds(&p, data, 0, 0, 0, &min, &max);
}
