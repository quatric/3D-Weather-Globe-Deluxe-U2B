// The globe's projection.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(ProjectionPerspScene, LF_CLASSOF(CProjection))

void ProjectionPerspScene::scenePointFromGeoPoint(Vector3 *out, const LPoint *geoPoint)
{
	float radius = HeightsManager::quickEarthRadiusFromMapPoint(mapPointFromGeoPoint(*geoPoint));
	*out = earthVecFromGeoPoint(*geoPoint) * radius;
}
