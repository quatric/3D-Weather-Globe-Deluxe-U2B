// CPolysDrawable: a polygon container drawn for one LOD. The outlines are one
// PolygonDrawableModel per vertex-buffer chunk (line lists over the BLOB's
// indices); the hit shape's highlight adds FilledPolygonDrawableModels over
// the triangulated polygons, split in buffers of at most 0xffff vertices.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CPolysDrawable, LF_CLASSOF(CShapeDrawable))

CPolysDrawable::CPolysDrawable()
    : _maxEdgeSq(0), _triCount(0), _tris(0), _buffers(0), _triangulated(false), _ibParsed(false), _modelParams(0),
      _paths(0)
{
}

void CPolysDrawable::dealloc()
{
	int count = _modelParams ? _modelParams->count() : 0;
	for (int i = 0; i < count; i++) {
		SSingleModelParams &params = _modelParams->objectAtIndex(i);
		if (params.ib) {
			params.ib->dataSourceWillDie();
			releaseAndClear(params.ib);
		}
		if (params.polygons) {
			delete params.polygons;
			params.polygons = 0;
		}
	}
	count = _buffers ? _buffers->count() : 0;
	for (int i = 0; i < count; i++) {
		SBuffersList *list = _buffers->objectAtIndex(i);
		for (SBuffersList::Node *node = list->head; node;) {
			SBuffersList::Node *next = node->next;
			delete[] node->points;
			delete[] node->indices;
			delete node;
			node = next;
		}
		// The original leaked the lists themselves.
		delete list;
	}
	if (_buffers) {
		delete _buffers;
		_buffers = 0;
	}
	if (_modelParams) {
		delete _modelParams;
		_modelParams = 0;
	}
	releaseAndClear(_paths);
	CShapeDrawable::dealloc();
}

void CPolysDrawable::_subdivideTriangle(LPoint &a, LPoint &b, LPoint &c)
{
	float ab = (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
	float ac = (a.x - c.x) * (a.x - c.x) + (a.y - c.y) * (a.y - c.y);
	float bc = (c.x - b.x) * (c.x - b.x) + (c.y - b.y) * (c.y - b.y);
	LPoint mid;
	if (ab <= ac) {
		if (bc < ac) {
			if (ac > _maxEdgeSq) {
				mid = LMakePoint((a.x + c.x) * 0.5f, (a.y + c.y) * 0.5f);
				_subdivideTriangle(mid, b, c);
				_subdivideTriangle(a, b, mid);
				return;
			}
			goto store;
		}
	} else if (bc < ab) {
		if (ab > _maxEdgeSq) {
			mid = LMakePoint((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
			_subdivideTriangle(mid, b, c);
			_subdivideTriangle(a, mid, c);
			return;
		}
		goto store;
	}
	if (bc > _maxEdgeSq) {
		mid = LMakePoint((b.x + c.x) * 0.5f, (b.y + c.y) * 0.5f);
		_subdivideTriangle(a, mid, c);
		_subdivideTriangle(a, b, mid);
		return;
	}
store:
	if (_tris) {
		LPoint *t = _tris + _triCount * 3;
		t[0] = a;
		t[1] = b;
		t[2] = c;
	}
	_triCount++;
}

D3DPRIMITIVETYPE CPolysDrawable::primitiveTypeForBufOwner(CBuffersOwner *owner)
{
	return lobj_cast<PolygonDrawableModel>(owner) ? D3DPT_LINELIST : D3DPT_TRIANGLELIST;
}

void CPolysDrawable::_modelWasCreated(CBuffersOwner *model, unsigned int index)
{
	PolygonDrawableModel *polygon = lobj_cast<PolygonDrawableModel>(model);
	if (polygon)
		polygon->_index = index;
}

// One SSingleModelParams per (vertex BLOB, chunk) of the drawable's LOD,
// with its polygons.
void CPolysDrawable::_parseIBIfNeeded()
{
	if (_ibParsed)
		return;
	CPolysContainer::_polyLineIterator it(_container->_indexes);
	while (it.nextLOD()) {
		while (it.nextPoly()) {
			if (_mode != it._lod || it._count < 2)
				continue;
			if (_modelParams == 0)
				_modelParams = new TArray<SSingleModelParams>;
			int count = _modelParams->count();
			int i;
			for (i = 0; i < count; i++) {
				SSingleModelParams &params = _modelParams->objectAtIndex(i);
				if (it._chunk == params.chunk && strncmp(it._uid, params.uid, 10) == 0)
					break;
			}
			if (i == count) {
				SSingleModelParams params = { it._uid, it._chunk, 0, 0, 0 };
				_modelParams->addObject(params);
			}
			SSingleModelParams &params = _modelParams->objectAtIndex(i);
			params.count += it._count;
			if (params.polygons == 0)
				params.polygons = new TArray<SPolygonParams>;
			SPolygonParams polygon = { it._count, it._indices };
			params.polygons->addObject(polygon);
		}
	}
	_ibParsed = true;
}

int CPolysDrawable::_modelsCount()
{
	_parseIBIfNeeded();
	return _modelParams ? _modelParams->count() : 0;
}

CShader *CPolysDrawable::renderShaderForBuffer(CIndexBuffer *ib)
{
	return renderProfile(ib->holder())->_shader;
}

LClass *CPolysDrawable::_classForModel(unsigned int)
{
	return LF_CLASSOF(PolygonDrawableModel);
}

unsigned int CPolysDrawable::indsCountForBuffer(CIndexBuffer *ib)
{
	if (ib->getClass()->isSubclassOf(LF_CLASSOF(PolygonIndexBuffer)))
		return _modelParams->objectAtIndex(((PolygonIndexBuffer *)ib)->_modelIndex).count * 2;
	return _buffers->objectAtIndex(((FilledPolygonIndexBuffer *)ib)->_modelIndex)->triCount * 3;
}

// The outlines: each polygon's closed ring as line pairs (the BLOB's indices
// are big-endian). The fill: each node's triangles, offset by the vertices of
// the nodes before it.
void CPolysDrawable::fillBuffer(CIndexBuffer *ib)
{
	StIndexBufferLocker *locker = ib->detachLocker(0);
	uint16_t *out = (uint16_t *)locker->data();
	if (ib->getClass()->isSubclassOf(LF_CLASSOF(PolygonIndexBuffer))) {
		SSingleModelParams &params = _modelParams->objectAtIndex(((PolygonIndexBuffer *)ib)->_modelIndex);
		int count = params.polygons->count();
		for (int i = 0; i < count; i++) {
			SPolygonParams &polygon = params.polygons->objectAtIndex(i);
			uint16_t first = __builtin_bswap16(polygon.indices[0]);
			uint16_t index = first;
			for (int j = 1; j <= polygon.count - 1; j++) {
				out[0] = index;
				index = __builtin_bswap16(polygon.indices[j]);
				out[1] = index;
				out += 2;
			}
			out[0] = index;
			out[1] = first;
			out += 2;
		}
	} else {
		SBuffersList *list = _buffers->objectAtIndex(((FilledPolygonIndexBuffer *)ib)->_modelIndex);
		uint16_t base = 0;
		for (SBuffersList::Node *node = list->head; node; node = node->next) {
			for (int i = 0; i < node->triCount * 3; i++)
				*out++ = base + (uint16_t)(node->indices ? node->indices[i] : i);
			base += (uint16_t)node->vertCount;
		}
	}
	delete locker;
}

int CPolysDrawable::_triangulate()
{
	if (!_triangulated) {
		triangulateio in, out;
		memset(&in, 0, sizeof(in));
		memset(&out, 0, sizeof(out));
		StPointerBlock pointsBlock(1000, false);
		StPointerBlock segmentsBlock(1000, false);
		StPointerBlock markersBlock(1000, false);
		SBuffersList *list = 0;
		SBuffersList::Node *last = 0;
		CPolysContainer::_polyLineIterator it(_container->_indexes);
		while (it.nextLOD()) {
			while (it.nextPoly()) {
				if (it._lod != _mode)
					continue;
				VertexBuffersBLOB *vertices =
				    VertexBuffersBLOB::verticesWithUID(LString::stringWithCString(it._uid, 10));
				if (vertices == 0)
					continue;
				const LPoint *src = vertices->pointsForBufferAtIndex(it._chunk, _mode);
				if (src == 0)
					return 0;
				int count = it._count;
				if (pointsBlock.size() < (unsigned int)count * 8)
					pointsBlock.setSize(count * 8);
				LPoint *pts = (LPoint *)pointsBlock.bytes();
				// Drops repeated points (near ones at the coarse LODs) and
				// spikes back to the point before last.
				int n = 0;
				for (int i = 0; i < count; i++) {
					LPoint &p = pts[n];
					p = src[__builtin_bswap16(it._indices[i])];
					if (n >= 2 && pts[n - 2].x == p.x && p.y == pts[n - 2].y) {
						n -= 2;
						continue;
					}
					if (_mode < 2) {
						if (n > 0 && fabsf(pts[n - 1].x - p.x) < 0.0125f && fabsf(pts[n - 1].y - p.y) < 0.0125f)
							continue;
					} else if (n > 0 && pts[n - 1].x == p.x && p.y == pts[n - 1].y) {
						continue;
					}
					n++;
				}
				if (n > 0) {
					if (_mode < 2 && fabsf(pts[n - 1].x - pts[0].x) < 0.0125f &&
					    fabsf(pts[n - 1].y - pts[0].y) < 0.0125f)
						n--;
					else if (pts[n - 1].x == pts[0].x)
						n -= pts[n - 1].y == pts[0].y;
				}
				if (n <= 3)
					continue;
				in.pointlist = (float *)pts;
				in.pointattributelist = 0;
				in.numberofpointattributes = 0;
				in.pointmarkerlist = 0;
				in.numberofpoints = n;
				in.segmentlist = 0;
				in.segmentmarkerlist = 0;
				in.numberofsegments = n;
				if (segmentsBlock.size() < (unsigned int)n * 8)
					segmentsBlock.setSize(n * 8);
				in.segmentlist = (int *)segmentsBlock.bytes();
				if (markersBlock.size() < (unsigned int)n * 4)
					markersBlock.setSize(n * 4);
				in.segmentmarkerlist = (int *)markersBlock.bytes();
				for (int i = 0; i < n - 1; i++) {
					in.segmentlist[i * 2] = i;
					in.segmentlist[i * 2 + 1] = i + 1;
					in.segmentmarkerlist[i] = 1;
				}
				in.segmentmarkerlist[n - 1] = 1;
				in.segmentlist[n * 2 - 2] = n - 1;
				in.segmentlist[n * 2 - 1] = 0;
				in.holelist = 0;
				in.numberofholes = 0;
				in.numberofregions = 0;
				memset(&out, 0, sizeof(out));
				triangulate("QzpXY", &in, &out, 0);
				if (out.trianglelist == 0)
					continue;
				const LPoint *outPoints = (const LPoint *)out.pointlist;
				const int *tris = out.trianglelist;
				// Count the pieces, split where a batch passes 0x5555
				// triangles...
				_splitCounts.removeAllObjects();
				_triCount = 0;
				_tris = 0;
				for (int t = 0; t < out.numberoftriangles; t++) {
					int before = _triCount;
					LPoint a = outPoints[tris[t * 3]], b = outPoints[tris[t * 3 + 1]],
					       c = outPoints[tris[t * 3 + 2]];
					_subdivideTriangle(a, b, c);
					if (_triCount > 0x5555) {
						_splitCounts.addObject(before);
						_triCount -= before;
					}
				}
				if (_triCount != 0)
					_splitCounts.addObject(_triCount);
				// ...then store each batch in a node.
				int next = 0;
				int batches = _splitCounts.count();
				for (int s = 0; s < batches; s++) {
					int batch = _splitCounts[s];
					_tris = new LPoint[batch * 3];
					_triCount = 0;
					while (_triCount < batch) {
						LPoint a = outPoints[tris[next * 3]], b = outPoints[tris[next * 3 + 1]],
						       c = outPoints[tris[next * 3 + 2]];
						_subdivideTriangle(a, b, c);
						next++;
					}
					int vertCount = _triCount * 3;
					if (_buffers == 0) {
						_buffers = new TArray<SBuffersList *>;
						_buffers->preserveCapacity(2);
					}
					if (list == 0 || list->vertCount + vertCount > 0xffff) {
						list = new SBuffersList;
						_buffers->addObject(list);
						list->head = 0;
						list->vertCount = 0;
						list->triCount = 0;
					}
					SBuffersList::Node *node = new SBuffersList::Node;
					if (list->head == 0)
						list->head = node;
					else
						last->next = node;
					list->triCount += _triCount;
					list->vertCount += vertCount;
					node->indices = 0;
					node->points = _tris;
					node->vertCount = vertCount;
					node->next = 0;
					node->triCount = _triCount;
					last = node;
				}
				_tris = 0;
				free(out.pointlist);
				free(out.trianglelist);
			}
		}
		_triangulated = true;
	}
	return _buffers ? _buffers->count() : 0;
}

LString *CPolysDrawable::_renderProfileFile()
{
	return LString::stringWithCString("RENDERPROFILES\\Borders.yaf");
}

int CPolysDrawable::pathsCount()
{
	if (_paths == 0) {
		_paths = LArray::_alloc()->init();
		((CPolysContainer *)_container)->_loadPathsForLod(_mode, _paths, 0, 0, 0);
	}
	return _paths->count();
}

CVertexBuffer *CPolysDrawable::vertexBufferForBufOwner(int, CBuffersOwner *owner)
{
	PolygonDrawableModel *polygon = lobj_cast<PolygonDrawableModel>(owner);
	if (polygon) {
		_parseIBIfNeeded();
		SSingleModelParams &params = _modelParams->objectAtIndex(polygon->_index);
		VertexBuffersBLOB *vertices = VertexBuffersBLOB::verticesWithUID(LString::stringWithCString(params.uid, 10));
		if (vertices == 0)
			return 0;
		return vertices->deviceBufferAtIndex(params.chunk, _mode, _projection, owner->renderProfile());
	}
	if (_buffers == 0)
		return 0;
	CHolder *holder = owner->holder();
	if (holder)
		holder->beginAccumulationBlock();
	FilledPolygonDrawableModel *fill = lobj_cast<FilledPolygonDrawableModel>(owner);
	SBuffersList *list = _buffers->objectAtIndex(fill->_index);
	VBFilledPolygonDataSource *source = LAutorelease(VBFilledPolygonDataSource::_alloc());
	source->_profile = fill->renderProfile();
	source->_projection = _projection;
	source->_head = list->head;
	source->_vertCount = list->vertCount;
	CVertexBuffer *vb = LAutorelease(CVertexBuffer::_alloc()->initWithDataSource(source, owner->holder(), 1, 0));
	if (holder)
		holder->endAccumulationBlock();
	return vb;
}

CIndexBuffer *CPolysDrawable::indexBufferForBufOwner(CBuffersOwner *owner)
{
	_parseIBIfNeeded();
	PolygonDrawableModel *polygon = lobj_cast<PolygonDrawableModel>(owner);
	if (polygon) {
		int index = polygon->_index;
		SSingleModelParams &params = _modelParams->objectAtIndex(index);
		if (params.ib == 0) {
			LAutoreleasePool::addNewPoolToTop();
			CHolder *holder = owner->holder();
			if (holder)
				holder->beginAccumulationBlock();
			PolygonIndexBuffer *ib = (PolygonIndexBuffer *)PolygonIndexBuffer::_alloc()->initWithDataSource(
			    this, D3DPT_LINELIST, owner->holder(), 0, 0);
			params.ib = ib;
			ib->_modelIndex = index;
			if (holder)
				holder->endAccumulationBlock();
			LAutoreleasePool::removeTopPool();
		}
		return params.ib;
	}
	FilledPolygonDrawableModel *fill = lobj_cast<FilledPolygonDrawableModel>(owner);
	if (fill == 0)
		return 0;
	CHolder *holder = owner->holder();
	if (holder)
		holder->beginAccumulationBlock();
	int index = fill->_index;
	FilledPolygonIndexBuffer *ib = LAutorelease((FilledPolygonIndexBuffer *)FilledPolygonIndexBuffer::_alloc()->initWithDataSource(
	    this, D3DPT_TRIANGLELIST, owner->holder(), 0, 0));
	ib->_modelIndex = index;
	if (holder)
		holder->endAccumulationBlock();
	return ib;
}

// The hit shape's highlight also gets the fills, drawn under the outlines in
// the shape's fill color.
LArray *CPolysDrawable::drawableDatas(int index, CHolder *holder, const LString *usageKey, unsigned char *created)
{
	unsigned char made;
	LArray *datas = CShapeDrawable::drawableDatas(index, holder, usageKey, &made);
	if (created)
		*created = made;
	if (!made || !usageKey->isEqualTo(ShapeModelData::kUsageHitShapeHighlight))
		return datas;
	int count = _triangulate();
	if (count == 0)
		return datas;
	Color color(1, 1, 1, 1);
	CShapeManager::highlightFillColorForShapeID(_container->_shapeID, &color);
	if (holder)
		holder->beginAccumulationBlock();
	CRenderProfile *profile = renderProfile(holder);
	for (int i = 0; i < count; i++) {
		CBuffersOwner *model = FilledPolygonDrawableModel::_alloc()->initWithDataSource(this, profile, 0);
		lobj_cast<FilledPolygonDrawableModel>(model)->_index = i;
		CDrawableModelData *data = model->detachModelDataInstance();
		if (data && data->getClass()->isSubclassOf(LF_CLASSOF(ShapeModelData))) {
			ShapeModelData *shapeData = (ShapeModelData *)data;
			shapeData->_color = color;
			shapeData->_drawable = this;
			datas->insertObjectAtIndex(shapeData, 0);
		}
		model->release();
	}
	if (holder)
		holder->endAccumulationBlock();
	return datas;
}
