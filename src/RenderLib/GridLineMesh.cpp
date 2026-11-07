// The grid line mesh.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(GridLineMesh, LF_CLASSOF(CMesh))

LClass *GridLineMesh::modelDataClass()
{
	return GridLineMeshData::classObject();
}

// Always drawn.
bool GridLineMesh::evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance)
{
	CMesh::evalVisibilityParams(data, center, distance);
	return true;
}

// Only the lines with a matrix set.
void GridLineMesh::drawMeshFragment(unsigned int vertexStart, unsigned int vertexCount, unsigned int faceStart,
                                    unsigned int faceCount)
{
	unsigned int lines = static_cast<GridLineMeshData *>(_currentData)->_linesRenderCount;
	CMesh::drawMeshFragment(vertexStart, vertexCount, faceStart, (faceCount * lines) >> 3);
}
