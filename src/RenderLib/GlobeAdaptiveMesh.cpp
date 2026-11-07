// An adaptive mesh over a sphere patch; always visible (the patches are
// culled before they are drawn).

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(GlobeAdaptiveMesh, LF_CLASSOF(CAdaptiveFlatMesh))

bool GlobeAdaptiveMesh::evalVisibilityParams(CDrawableModelData *, Vector3 *, float *)
{
	return true;
}

LClass *GlobeAdaptiveMesh::modelDataClass()
{
	return LF_CLASSOF(GlobeAdaptiveMeshData);
}
