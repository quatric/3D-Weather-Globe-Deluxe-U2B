// The eclipse shadow's mesh: drawn while the shadow falls on the Earth.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CEclipseMesh, LF_CLASSOF(CMesh))

LClass *CEclipseMesh::modelDataClass()
{
	return LF_CLASSOF(CEclipseMeshData);
}

bool CEclipseMesh::evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance)
{
	return CMesh::evalVisibilityParams(data, center, distance) && ((CEclipseMeshData *)data)->_inArea;
}
