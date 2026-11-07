// FilledPolygonDrawableModel: a fill's triangles of one buffer, at the origin.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(FilledPolygonDrawableModel, LF_CLASSOF(CBuffersOwner))

LClass *FilledPolygonDrawableModel::modelDataClass()
{
	return LF_CLASSOF(FilledPolygonDrawableModelData);
}

bool FilledPolygonDrawableModel::evalVisibilityParams(CDrawableModelData *, Vector3 *center, float *distance)
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
