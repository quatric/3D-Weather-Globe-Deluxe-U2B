// MarkerMesh: the hit city's marker.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(MarkerMesh, LF_CLASSOF(CMesh))

LClass *MarkerMesh::modelDataClass()
{
	return LF_CLASSOF(MarkerModelData);
}
