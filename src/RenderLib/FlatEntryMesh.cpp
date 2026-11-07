// The flat map's tile mesh.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(FlatEntryMesh, LF_CLASSOF(CMesh))

LClass *FlatEntryMesh::modelDataClass()
{
	return FlatEntryObjectData::classObject();
}

bool FlatEntryMesh::evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance)
{
	CMesh::evalVisibilityParams(data, center, distance);
	return true;
}
