// The flat map's projection.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CProjectionFlatScene, LF_CLASSOF(CProjection))

void CProjectionFlatScene::scenePointFromGeoPoint(Vector3 *out, const LPoint *geoPoint)
{
	LPoint p = mapPointFromGeoPoint(*geoPoint);
	out->z = 0.0f;
	out->x = p.x;
	out->y = p.y;
}
