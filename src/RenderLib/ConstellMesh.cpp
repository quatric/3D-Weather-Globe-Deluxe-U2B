// The constellation lines' mesh: always drawn.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(ConstellMesh, LF_CLASSOF(CMesh))

bool ConstellMesh::evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance)
{
	CMesh::evalVisibilityParams(data, center, distance);
	return true;
}
