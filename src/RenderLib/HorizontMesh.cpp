// The horizon strip: drawn while its data shows.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(HorizontMesh, LF_CLASSOF(CMesh))

LClass *HorizontMesh::modelDataClass()
{
	return LF_CLASSOF(HorizontDrawableData);
}

bool HorizontMesh::evalVisibilityParams(CDrawableModelData *data, Vector3 *, float *)
{
	return data && ((HorizontDrawableData *)data)->_visibility > 0.0f;
}
