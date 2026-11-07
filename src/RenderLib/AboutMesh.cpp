// AboutMesh: the about box's logo drawn over the globe.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(AboutMesh, LF_CLASSOF(CMesh))

LClass *AboutMesh::modelDataClass()
{
	return LF_CLASSOF(AboutMeshData);
}
