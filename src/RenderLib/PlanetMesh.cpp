// The sphere of a planet.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PlanetMesh, LF_CLASSOF(CMesh))

LClass *PlanetMesh::modelDataClass()
{
	return LF_CLASSOF(PlanetDrawableData);
}
