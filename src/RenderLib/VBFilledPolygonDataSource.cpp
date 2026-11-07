// VBFilledPolygonDataSource: the vertices of a fill's buffer list, its
// triangles' geographic points through the projection.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(VBFilledPolygonDataSource, 0)

VBFilledPolygonDataSource::VBFilledPolygonDataSource() : _profile(0), _projection(0), _head(0), _vertCount(0)
{
}

CShader *VBFilledPolygonDataSource::renderShaderForBuffer(CVertexBuffer *)
{
	return _profile ? _profile->_shader : 0;
}

void VBFilledPolygonDataSource::fillBuffer(CVertexBuffer *vb)
{
	StVertexBufferLocker *locker = vb->detachLocker(0);
	Vector3 *out = (Vector3 *)locker->data();
	for (SBuffersList::Node *node = _head; node; node = node->next) {
		const LPoint *p = node->points;
		for (int i = 0; i < node->vertCount; i++)
			_projection->scenePointFromGeoPoint(out++, p++);
	}
	delete locker;
}
