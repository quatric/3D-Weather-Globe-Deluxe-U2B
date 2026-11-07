// PolygonDrawableModel: an outline's lines of one buffer, at the origin.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PolygonDrawableModel, LF_CLASSOF(CBuffersOwner))

LClass *PolygonDrawableModel::modelDataClass()
{
	return LF_CLASSOF(PolygonDrawableModelData);
}

bool PolygonDrawableModel::evalVisibilityParams(CDrawableModelData *, Vector3 *center, float *distance)
{
	if (center) {
		center->z = 0.5f;
		center->y = 0;
		center->x = 0;
	}
	if (distance)
		*distance = 1.0f;
	return true;
}
