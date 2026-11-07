// The atmosphere dome: drawn while its data shows.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(AtmosphereMesh, LF_CLASSOF(CMesh))

LClass *AtmosphereMesh::modelDataClass()
{
	return LF_CLASSOF(AtmosphereDrawableData);
}

bool AtmosphereMesh::evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance)
{
	if (!data)
		return false;
	AtmosphereDrawableData *atmosphere = lobj_cast<AtmosphereDrawableData>(data);
	if (!atmosphere)
		return CMesh::evalVisibilityParams(data, center, distance);
	return atmosphere->_visible;
}
