// The clouds layer mesh: drawn while its data shows.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CCloudsMesh, LF_CLASSOF(CMesh))

LClass *CCloudsMesh::modelDataClass()
{
	return LF_CLASSOF(CCloudsMeshData);
}

bool CCloudsMesh::evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance)
{
	if (!CMesh::evalVisibilityParams(data, center, distance))
		return false;
	return ((CCloudsMeshData *)data)->_visibility > 0.01f;
}
