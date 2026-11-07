// CMesh and the standard shapes' data sources.

#include "FHMeshes.h"

#include <math.h>
#include <memory>

LF_IMPLEMENT_CLASS(CMesh, LF_CLASSOF(CBuffersOwner))
LF_IMPLEMENT_CLASS(LStdGeometryDataSource, 0)

// ---------------------------------------------------------------------------
// CMesh

CMesh::CMesh()
    : _meshOptions(0), _meshPrimType(D3DPT_TRIANGLELIST), _dxMesh(0), _meshVB(0), _meshIB(0), _extra(0),
      _attributesCount(0), _attributes(0), _softwareProcessing(false), _meshDS(0), _meshDSOwner(0), _flag114(true),
      _performBoxVisibilityTest(true), _unused130(0), _currentSubset(-1), _flag138(false)
{
	_boxMin = Vector3(0, 0, 0);
	_boxMax = Vector3(0, 0, 0);
}

CMesh *CMesh::initWithDataSource(CMeshDataSource *source, CRenderProfile *profile, unsigned char retain)
{
	if (CBuffersOwner::initWithDataSource(this, profile, false) == 0)
		return 0;
	_meshDS = source;
	_meshDSOwner = retain && source ? dynamic_cast<LObject *>(source) : 0;
	LRetain(_meshDSOwner);
	return this;
}

CMesh *CMesh::initWithStdGeometry(const SStdGeometry *geometry, CRenderProfile *profile)
{
	LStdGeometryDataSource *source = LStdGeometryDataSource::sourceWithStdGeometry(geometry);
	return initWithDataSource(source, profile, true);
}

void CMesh::dealloc()
{
	if (_extra) {
		operator delete(_extra);
		_extra = 0;
	}
	if (_meshDS) {
		_meshDS->meshWillDie(this);
		_meshDS = 0;
	}
	LRelease(_meshDSOwner);
	_meshDSOwner = 0;
	LRelease(_meshVB);
	_meshVB = 0;
	LRelease(_meshIB);
	_meshIB = 0;
	CBuffersOwner::dealloc();
}

void CMesh::dataSourceWillDie()
{
	LRelease(_meshDSOwner);
	_meshDSOwner = 0;
	_meshDS = 0;
}

LClass *CMesh::modelDataClass()
{
	return LF_CLASSOF(ModelDataWithMatrix);
}

void CMesh::instantiateSelf()
{
	createMesh();
	updateBoundingParameters();
}

void CMesh::releaseSelf()
{
	if (_meshDS)
		_meshDS->meshWillRelease(this);
	if (_dxMesh) {
		_dxMesh->Release();
		_dxMesh = 0;
	}
	delete[] _attributes;
	_attributes = 0;
	LRelease(_meshVB);
	_meshVB = 0;
	LRelease(_meshIB);
	_meshIB = 0;
	invalidateBuffers();
	CBuffersOwner::releaseSelf();
}

CVertexBuffer *CMesh::vertexBufferForBufOwner(int stream, CBuffersOwner *)
{
	return stream == 0 ? _meshVB : 0;
}

void CMesh::updateBoundingParameters()
{
	if (_dxMesh == 0 || !_performBoxVisibilityTest)
		return;
	StVertexBufferLocker locker(_dxMesh, 0);
	unsigned int stride = _profile->vertexStructSize(0);
	FHComputeBoundingBox((const Vector3 *)locker.data(), _dxMesh->GetNumVertices(), stride, &_boxMin, &_boxMax);
}

void CMesh::doImmediateDrawWithData(CDrawableModelData *data, CFrameDrawer *drawer)
{
	_currentSubset = -1;
	if (_dxMesh)
		CBuffersOwner::doImmediateDrawWithData(data, drawer);
}

// Draws the attribute range of the subset.
void CMesh::drawSubset(CDrawableModelData *, long subset, long)
{
	for (unsigned int i = 0; i < _attributesCount; i++) {
		const SMeshAttributeRange *r = &_attributes[i];
		if (r->attribId == (unsigned int)subset) {
			drawMeshFragment(r->vertexStart, r->vertexCount, r->faceStart, r->faceCount);
			return;
		}
	}
}

void CMesh::drawMeshFragment(unsigned int vertexStart, unsigned int vertexCount, unsigned int faceStart,
                             unsigned int faceCount)
{
	if (_context == 0)
		return;
	CDrawCache *cache = _context->_drawCache;
	if (_meshPrimType == D3DPT_POINTLIST)
		cache->drawPrimitive(D3DPT_POINTLIST, vertexStart, vertexCount);
	else
		cache->drawIndexedPrimitive(_meshPrimType, 0, vertexStart, vertexCount, faceStart * 3, faceCount);
}

// The box test, and the box center projected.
bool CMesh::evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance)
{
	if (data == 0)
		return true;
	const Matrix *world = data->worldMatrix();
	int intersection = 2;
	if (_performBoxVisibilityTest)
		intersection = _context->boxIntersectionTest(&_boxMin, &_boxMax, world, false);
	if (center || distance) {
		Vector3 boxCenter = (_boxMax + _boxMin) * 0.5f;
		Vector3 projected;
		float w;
		_context->projectVector(center ? center : &projected, &boxCenter, world, distance ? distance : &w);
	}
	return intersection != 0;
}

// Asks the data source for the size and makes the D3DX-style mesh; the
// data source fills it.
void CMesh::createMesh()
{
	CShader *shader = _profile->_shader;
	if (shader == 0)
		return;
	if (shader->_streamsCount != 1)
		LF_RAISE(WrongConditionException());
	_softwareProcessing = shader->_useSWVP;
	bool dynamic = shader->_dynamicBuffer[0];
	if (_meshDS == 0) {
		LLog("%@ (%s) - unable to instantiate\n", this, _typeName);
		releaseContextDependedPart();
		return;
	}
	unsigned int numVertices = _meshDS->numVerticesForMesh(this);
	D3DPRIMITIVETYPE type = _meshDS->primitiveTypeForMesh(this);
	unsigned int numPrimitives = type == D3DPT_POINTLIST ? 1 : _meshDS->numPrimitivesForMesh(this);
	_managesSelf = false;
	unsigned int options = 0x110;	// system memory
	if (!_softwareProcessing) {
		options = 0x220;	// managed
		if (dynamic) {
			_managesSelf = true;
			options = 0;
		}
	}
	if (_meshOptions) {
		options = _meshOptions;
		if (_softwareProcessing)
			options |= 0x110;
	}
	if (type != D3DPT_POINTLIST && numVertices > 0xfffe)
		options |= D3DXMESH_32BIT;
	if (dynamic)
		options |= 0x80;
	_meshPrimType = type;
	unsigned int numFaces = numPrimitives;
	if (type == D3DPT_LINELIST)
		numFaces = (numPrimitives * 2 + 2) / 3;
	HRESULT hr = FHCreateMesh(numFaces, numVertices, options, shader->_decl, _context->_device, &_dxMesh);
	if (hr < 0 || _dxMesh == 0)
		return;
	VertexBufferEmu *vb = 0;
	IndexBufferEmu *ib = 0;
	_dxMesh->GetVertexBuffer(&vb);
	_dxMesh->GetIndexBuffer(&ib);
	LRelease(_meshVB);
	_meshVB = 0;
	LRelease(_meshIB);
	_meshIB = 0;
	unsigned int stride = FHGetDeclVertexSize(shader->_decl, 0);
	_meshVB = CVertexBuffer::_alloc()->initWithDeviceBuffer(vb, stride);
	_meshIB = CIndexBuffer::_alloc()->initWithDeviceBuffer(ib, _meshPrimType);
	bindBuffers();
	if (_meshDS) {
		_meshDS->meshWasInstantiated(this);
		_attributesCount = 1;
		_attributes = new SMeshAttributeRange[1];
		_attributes[0].attribId = 0;
		_attributes[0].faceStart = 0;
		_attributes[0].faceCount = numPrimitives;
		_attributes[0].vertexStart = 0;
		_attributes[0].vertexCount = numVertices;
	}
	updateBoundingParameters();
}

// ---------------------------------------------------------------------------
// Standard shapes.

unsigned int CStdGeometryDataSource::numPrimitivesForMesh(CMesh *)
{
	const SStdGeometry &g = _geometry;
	switch (g.type) {
	case kStdGeometrySpherePart:
		return (((unsigned int)g.cols * 2) / g.partsX) * g.rows / g.partsY;
	case kStdGeometrySphere:
	case kStdGeometryCylinder:
	case kStdGeometryPlaneXY:
		return (unsigned int)g.cols * g.rows * 2;
	}
	return 0;
}

unsigned int CStdGeometryDataSource::numVerticesForMesh(CMesh *)
{
	const SStdGeometry &g = _geometry;
	switch (g.type) {
	case kStdGeometrySpherePart:
		return (g.cols / g.partsX + 1) * (g.rows / g.partsY + 1);
	case kStdGeometrySphere:
	case kStdGeometryCylinder:
	case kStdGeometryPlaneXY:
		return (g.cols + 1) * (g.rows + 1);
	}
	return 0;
}

// Two triangles a cell of a cols x rows grid: a a+1 b+1, a b+1 b.
static void _fillGridIndices(CMesh *mesh, unsigned int cols, unsigned int rows)
{
	std::unique_ptr<StIndexBufferLocker> locker(mesh->detachIBLocker(0));
	bool is32 = locker->_desc.Format == D3DFMT_INDEX32;
	if (locker->_desc.Size / (is32 ? 4 : 2) / 3 != cols * rows * 2)
		LF_RAISE(WrongConditionException());
	if (is32) {
		uint32_t *p = (uint32_t *)locker->data();
		for (unsigned int r = 0; r < rows; r++) {
			uint32_t a = r * (cols + 1), b = a + cols + 1;
			for (unsigned int c = 0; c < cols; c++, a++, b++, p += 6) {
				p[0] = a;
				p[1] = a + 1;
				p[2] = b + 1;
				p[3] = a;
				p[4] = b + 1;
				p[5] = b;
			}
		}
	} else {
		uint16_t *p = (uint16_t *)locker->data();
		for (unsigned int r = 0; r < rows; r++) {
			uint16_t a = (uint16_t)(r * (cols + 1)), b = (uint16_t)(a + cols + 1);
			for (unsigned int c = 0; c < cols; c++, a++, b++, p += 6) {
				p[0] = a;
				p[1] = a + 1;
				p[2] = b + 1;
				p[3] = a;
				p[4] = b + 1;
				p[5] = b;
			}
		}
	}
}

// Fills a (cols+1) x (rows+1) grid of vertices; `vertex` gives the position
// and normal of grid point (u, v) in [0, 1].
template <typename F>
static void _fillGridVertices(CStdGeometryDataSource *ds, CMesh *mesh, CRenderProfile *profile, unsigned int cols,
                              unsigned int rows, F vertex)
{
	std::unique_ptr<StVertexBufferLocker> locker(mesh->detachVBLocker(0, 0));
	if (ds->numVerticesForMesh(mesh) != (cols + 1) * (rows + 1))
		LF_RAISE(WrongConditionException());
	char *p = (char *)locker->data();
	unsigned int stride = profile->vertexStructSize(0);
	int pos = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	int normal = profile->drawableElementOffset(D3DDECLUSAGE_NORMAL, 0, 0);
	int uv = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
	for (unsigned int r = 0; r <= rows; r++) {
		for (unsigned int c = 0; c <= cols; c++, p += stride) {
			float u = (float)c / (float)cols, v = (float)r / (float)rows;
			Vector3 position, n;
			vertex(u, v, &position, &n);
			if (pos >= 0)
				memcpy(p + pos, &position, sizeof(Vector3));
			if (normal >= 0)
				memcpy(p + normal, &n, sizeof(Vector3));
			if (uv >= 0) {
				float t[2] = {u, v};
				memcpy(p + uv, t, sizeof(t));
			}
		}
	}
}

void CStdGeometryDataSource::_fillAsCylinder(CMesh *mesh, CRenderProfile *profile, const SStdGeometry *g)
{
	float radius = g->radius, height = g->height;
	_fillGridVertices(this, mesh, profile, g->cols, g->rows, [=](float u, float v, Vector3 *p, Vector3 *n) {
		float a = u * 6.2831855f;
		float c = cosf(a), s = sinf(a);
		*p = Vector3(radius * c, radius * s, height * v);
		*n = Vector3(c, s, 0);
	});
	_fillGridIndices(mesh, g->cols, g->rows);
}

void CStdGeometryDataSource::_fillAsSphere(CMesh *mesh, CRenderProfile *profile, const SStdGeometry *g)
{
	float radius = g->radius;
	_fillGridVertices(this, mesh, profile, g->cols, g->rows, [=](float u, float v, Vector3 *p, Vector3 *n) {
		float lat = (v - 0.5f) * 3.1415927f, lon = u * 6.2831855f;
		float cl = cosf(lat), sl = sinf(lat);
		*n = Vector3(cosf(lon) * cl, sinf(lon) * cl, sl);
		*p = Vector3(n->x * radius, n->y * radius, sl * radius);
	});
	_fillGridIndices(mesh, g->cols, g->rows);
}

void CStdGeometryDataSource::_fillAsSpherePart(CMesh *mesh, CRenderProfile *profile, const SStdGeometry *g)
{
	float radius = g->radius;
	unsigned int cols = g->cols / g->partsX, rows = g->rows / g->partsY;
	float partsX = g->partsX, partsY = g->partsY, partX = g->partX, partY = g->partY;
	_fillGridVertices(this, mesh, profile, cols, rows, [=](float u, float v, Vector3 *p, Vector3 *n) {
		float lat = ((partY + v) * 3.1415927f) / partsY;
		float lon = ((partX + u) * 6.2831855f) / partsX;
		float cl = cosf(lat), sl = sinf(lat);
		*n = Vector3(cosf(lon) * sl, sinf(lon) * sl, -cl);
		*p = Vector3(n->x * radius, n->y * radius, -cl * radius);
	});
	_fillGridIndices(mesh, cols, rows);
}

void CStdGeometryDataSource::_fillAsPlaneXY(CMesh *mesh, CRenderProfile *profile, const SStdGeometry *g)
{
	float width = g->radius, height = g->height;
	_fillGridVertices(this, mesh, profile, g->cols, g->rows, [=](float u, float v, Vector3 *p, Vector3 *n) {
		*p = Vector3(width * u, height * v, 0);
		*n = Vector3(0, 0, 1);
	});
	_fillGridIndices(mesh, g->cols, g->rows);
}

void CStdGeometryDataSource::meshWasInstantiated(CMesh *mesh)
{
	CRenderProfile *profile = mesh->renderProfile();
	if (profile->_context == 0)
		LF_RAISE(WrongConditionException());
	switch (_geometry.type) {
	case kStdGeometrySphere:
		_fillAsSphere(mesh, profile, &_geometry);
		break;
	case kStdGeometrySpherePart:
		_fillAsSpherePart(mesh, profile, &_geometry);
		break;
	case kStdGeometryCylinder:
		_fillAsCylinder(mesh, profile, &_geometry);
		break;
	case kStdGeometryPlaneXY:
		_fillAsPlaneXY(mesh, profile, &_geometry);
		break;
	}
}

LStdGeometryDataSource *LStdGeometryDataSource::sourceWithStdGeometry(const SStdGeometry *geometry)
{
	LStdGeometryDataSource *source = (LStdGeometryDataSource *)LAutorelease(_alloc());
	if (source)
		source->_geometry = *geometry;
	return source;
}
