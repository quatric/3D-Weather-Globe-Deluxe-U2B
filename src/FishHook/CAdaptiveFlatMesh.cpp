// Adaptive flat meshes: patches drawn at the level of detail their distance
// asks for, from index and LOD buffers shared by LOD.

#include "FHMeshes.h"

#include <memory>

LF_IMPLEMENT_CLASS(CAdaptiveFlatMesh, LF_CLASSOF(CBuffersOwner))
LF_IMPLEMENT_CLASS(CMeshLODEntry, 0)
LF_IMPLEMENT_CLASS(CADMeshDataSource, 0)

// ---------------------------------------------------------------------------
// Patch layout: level L has 4^L patches, numbered from
// _offsetForSubEntries[L], row by row.

int UAdaptiveMeshUtils::_offsetForSubEntries[11] = {0, 1, 5, 21, 85, 341, 1365, 5461, 21845, 87381, 349525};

int IAdaptiveMeshDataSource::boundsArrayElementsCount(int levels)
{
	return UAdaptiveMeshUtils::_offsetForSubEntries[levels - 1];
}

namespace {
struct SPrecalcSubpatchInfos {
	int *startIndices;	// a patch
	int *primCounts;	// a level
};
}

static SPrecalcSubpatchInfos __precalcSubpatchInfosForEveryLOD[10];
static CSharableResource __localUAMUMutex;

UAdaptiveMeshUtils::UAdaptiveMeshUtils()
{
	for (int lod = 0; lod < 10; lod++)
		subpatchInfoForLevelAtLOD(0, 0, 0, lod);
}

static UAdaptiveMeshUtils _sAdaptiveMeshUtils;

// The cells of a LOD are indexed in Morton order (x in the even bits, y in
// the odd ones), six indices a cell; odd LODs have twice the triangles.
SSubpatchInfo UAdaptiveMeshUtils::subpatchInfoForLevelAtLOD(int x, int y, int level, int lod)
{
	int n = lod >> 1;
	SPrecalcSubpatchInfos *infos = &__precalcSubpatchInfosForEveryLOD[n];
	if (infos->startIndices == 0) {
		int *starts = new int[_offsetForSubEntries[n + 1]];
		int *counts = new int[n + 1];
		for (int l = 0; l <= n; l++) {
			int side = 1 << l;
			int cells = (1 << n) >> l;
			cells *= cells;
			for (unsigned int py = 0; (int)py < side; py++) {
				for (unsigned int px = 0; (int)px < side; px++) {
					unsigned int morton = 0, bit = 1;
					for (unsigned int bx = px, by = py; bx || by; bx >>= 1, by >>= 1, bit <<= 2) {
						if (bx & 1)
							morton |= bit;
						if (by & 1)
							morton |= bit << 1;
					}
					starts[px + py * side + _offsetForSubEntries[l]] = morton * cells * 6;
				}
			}
			counts[l] = cells * 2;
		}
		infos->startIndices = starts;
		infos->primCounts = counts;
	}
	SSubpatchInfo info;
	info.startIndex = infos->startIndices[(y << level) + _offsetForSubEntries[level] + x] << (lod & 1);
	info.primCount = infos->primCounts[level] << (lod & 1);
	return info;
}

// ---------------------------------------------------------------------------
// The LOD stream: lodBase - 1 at the corners of the cells, lodBase + 1 at
// the centers of odd LODs' cells, lodBase at the edge midpoints; the border
// vertices are lodBase - 1.

void CAdaptiveMeshRLODStreamFiller::fillRLODStream_2n(long size, char *data, long stride, long lodBase)
{
	float base = (float)lodBase;
	for (unsigned int y = 0; (long)y <= size; y++) {
		for (unsigned int x = 0; (long)x <= size; x++, data += stride) {
			float v;
			if ((x & 1) == (y & 1))
				v = (x & y & 1) ? base + 1.0f : base - 1.0f;
			else
				v = base + 0.0f;
			*(float *)data = v;
		}
	}
	for (long i = 0; i < size * 4; i++, data += stride)
		*(float *)data = base - 1.0f;
}

void CAdaptiveMeshRLODStreamFiller::fillRLODStream_2n_plus_1(long size, char *data, long stride, long lodBase)
{
	float base = (float)lodBase;
	for (unsigned int y = 0; (long)y <= size; y++) {
		for (unsigned int x = 0; (long)x <= size; x++, data += stride)
			*(float *)data = (x & 1) == (y & 1) ? base - 1.0f : base + 1.0f;
		if ((long)y < size)
			for (long i = 0; i < size; i++, data += stride)
				*(float *)data = base + 0.0f;
	}
	for (long i = 0; i < size * 4; i++, data += stride)
		*(float *)data = base - 1.0f;
}

// ---------------------------------------------------------------------------
// Index buffers: the cells in Morton order, then the skirt joining the grid
// edges to the border vertices from `base` on.

template <typename T>
void TAdaptiveIBFiller<T>::fillBorders(T *p, T rowStride, T base, int size)
{
	int n = size;
	// The first row.
	for (int i = 0; i < n; i++, p += 6) {
		p[0] = (T)(base + i);
		p[1] = (T)(i + 1);
		p[2] = (T)(base + i + 1);
		p[3] = (T)(base + i);
		p[4] = (T)i;
		p[5] = (T)(i + 1);
	}
	// The first column.
	T left = (T)(base + n + 1);
	p[0] = base;
	p[1] = rowStride;
	p[2] = 0;
	p[3] = base;
	p[4] = left;
	p[5] = rowStride;
	for (int k = 1; k < n; k++, left += 2) {
		p += 6;
		p[0] = left;
		p[1] = (T)((k + 1) * rowStride);
		p[2] = (T)(k * rowStride);
		p[3] = left;
		p[4] = (T)(left + 2);
		p[5] = (T)((k + 1) * rowStride);
	}
	p += 6;
	// The last column.
	T right = (T)(base + n);
	for (int j = 0; j < n; j++, right += 2, p += 6) {
		p[0] = right;
		p[1] = (T)(n + j * rowStride);
		p[2] = (T)(n + (j + 1) * rowStride);
		p[3] = right;
		p[4] = (T)(n + (j + 1) * rowStride);
		p[5] = j < n - 1 ? (T)(right + 2) : (T)(base + 2 * n + 1 + 2 * j);
	}
	// The last row.
	T bottom = (T)(base - 1 + n * 3);
	for (int m = 0; m < n; m++, bottom++, p += 6) {
		p[0] = bottom;
		p[1] = (T)(n * rowStride + m + 1);
		p[2] = (T)(n * rowStride + m);
		p[3] = bottom;
		p[4] = (T)(bottom + 1);
		p[5] = (T)(n * rowStride + m + 1);
	}
}

template <typename T>
void TAdaptiveIBFiller<T>::fillIndices_LOD_2n(T *p, long size, long x, long y, long rowStride, unsigned char borders)
{
	if (size < 2) {
		T v0 = (T)(y * rowStride + x), v1 = (T)(x + (y + 1) * rowStride);
		// The diagonal alternates.
		if ((x & 1) == (y & 1)) {
			p[0] = v0;
			p[1] = v1;
			p[2] = (T)(v1 + 1);
			p[3] = v0;
			p[4] = (T)(v1 + 1);
			p[5] = (T)(v0 + 1);
		} else {
			p[0] = v0;
			p[1] = v1;
			p[2] = (T)(v0 + 1);
			p[3] = (T)(v0 + 1);
			p[4] = v1;
			p[5] = (T)(v1 + 1);
		}
		return;
	}
	long h = size >> 1, q = h * h;
	fillIndices_LOD_2n(p, h, x, y, rowStride, false);
	fillIndices_LOD_2n(p + q * 6, h, x + h, y, rowStride, false);
	fillIndices_LOD_2n(p + q * 12, h, x, y + h, rowStride, false);
	fillIndices_LOD_2n(p + q * 18, h, x + h, y + h, rowStride, false);
	if (borders)
		fillBorders(p + size * size * 6, (T)(size + 1), (T)((size + 1) * (size + 1)), size);
}

template <typename T>
void TAdaptiveIBFiller<T>::fillIndices_LOD_2n_plus_1(T *p, long size, long x, long y, long rowStride,
                                                    unsigned char borders)
{
	if (size < 2) {
		// Rows of corners and of centers alternate.
		long s = rowStride * 2 - 1;
		T v0 = (T)(y * s + x), v1 = (T)((y + 1) * s + x), c = (T)(v0 + rowStride);
		p[0] = v0;
		p[1] = v1;
		p[2] = c;
		p[3] = v0;
		p[4] = c;
		p[5] = (T)(v0 + 1);
		p[6] = v1;
		p[7] = (T)(v1 + 1);
		p[8] = c;
		p[9] = c;
		p[10] = (T)(v1 + 1);
		p[11] = (T)(v0 + 1);
		return;
	}
	long h = size >> 1, q = h * h;
	fillIndices_LOD_2n_plus_1(p, h, x, y, rowStride, false);
	fillIndices_LOD_2n_plus_1(p + q * 12, h, x + h, y, rowStride, false);
	fillIndices_LOD_2n_plus_1(p + q * 24, h, x, y + h, rowStride, false);
	fillIndices_LOD_2n_plus_1(p + q * 36, h, x + h, y + h, rowStride, false);
	if (borders)
		fillBorders(p + size * size * 12, (T)(size * 2 + 1), (T)((size + 1) * (size + 1) + size * size), size);
}

template class TAdaptiveIBFiller<uint16_t>;
template class TAdaptiveIBFiller<uint32_t>;

// ---------------------------------------------------------------------------
// CADMeshDataSource

CADMeshDataSource *CADMeshDataSource::initWithLOD(int lod, CRenderProfile *profile)
{
	_lod = lod;
	_profile = profile;
	LRetain(profile);
	int n = 1 << (_lod >> 1);
	bool odd = _lod & 1;
	_primCount = (odd ? n * n * 4 : n * n * 2) + n * 8;
	_vertCount = (n + 1) * (n + 1) + (odd ? n * n : 0) + n * 4;
	return this;
}

void CADMeshDataSource::dealloc()
{
	LRelease(_profile);
	_profile = 0;
	LObject::dealloc();
}

unsigned int CADMeshDataSource::streamNumberForBuffer(CVertexBuffer *)
{
	unsigned short stream = 0xffff;
	_profile->drawableElementOffset(D3DDECLUSAGE_PSIZE, 0, &stream);
	return stream < 9 ? stream : 0;
}

// Vertex texture fetch was done in software on some NVidia cards before
// Mac OS X 10.4; every system this runs on is newer.
bool CADMeshDataSource::isSoftwareVertexBuffer(CVertexBuffer *)
{
	return false;
}

void CADMeshDataSource::fillBuffer(CIndexBuffer *ib)
{
	bool is32 = ib->_is32bit;
	long n = 1 << (_lod >> 1);
	std::unique_ptr<StIndexBufferLocker> locker(ib->detachLocker(0));
	void *p = locker->data();
	if (!is32) {
		if (_lod & 1)
			TAdaptiveIBFiller<uint16_t>::fillIndices_LOD_2n_plus_1((uint16_t *)p, n, 0, 0, n + 1, true);
		else
			TAdaptiveIBFiller<uint16_t>::fillIndices_LOD_2n((uint16_t *)p, n, 0, 0, n + 1, true);
	} else if (_lod & 1)
		TAdaptiveIBFiller<uint32_t>::fillIndices_LOD_2n_plus_1((uint32_t *)p, n, 0, 0, n + 1, true);
	else
		TAdaptiveIBFiller<uint32_t>::fillIndices_LOD_2n((uint32_t *)p, n, 0, 0, n + 1, true);
}

void CADMeshDataSource::fillBuffer(CVertexBuffer *vb)
{
	long n = 1 << (_lod >> 1);
	std::unique_ptr<StVertexBufferLocker> locker(vb->detachLocker(0));
	if (_lod & 1)
		CAdaptiveMeshRLODStreamFiller::fillRLODStream_2n_plus_1(n, (char *)locker->data(), 4, 24 - _lod);
	else
		CAdaptiveMeshRLODStreamFiller::fillRLODStream_2n(n, (char *)locker->data(), 4, 24 - _lod);
}

// ---------------------------------------------------------------------------
// CMeshLODEntry

CMeshLODEntry *CMeshLODEntry::sPrevDrawnEntry = 0;
LString *CMeshLODEntry::sIBKeys[20];
LString *CMeshLODEntry::sRLODKeys[20];
static bool _sMeshLODKeysArraysAreValid = false;

class _LODMEshEntryCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		for (int i = 0; i < 20; i++) {
			LRelease(CMeshLODEntry::sIBKeys[i]);
			CMeshLODEntry::sIBKeys[i] = 0;
			LRelease(CMeshLODEntry::sRLODKeys[i]);
			CMeshLODEntry::sRLODKeys[i] = 0;
		}
	}
};

static _LODMEshEntryCleanup ___meshLODEntryCleaner;

CMeshLODEntry *CMeshLODEntry::initWithParams(int lod, CAdaptiveFlatMesh *mesh, IAdaptiveMeshDataSource *ds)
{
	if (LObject::init() == 0)
		return 0;
	_mesh = mesh;
	_lod = lod;
	_holder = mesh->_holder;
	_ds = ds;
	int n = 1 << (lod >> 1);
	int count = (n + 1) * (n + 1);
	if (lod & 1)
		count += n * n;
	_numVertices = count + n * 4;
	return this;
}

void CMeshLODEntry::dealloc()
{
	LRelease(_stateBlock);
	_stateBlock = 0;
	LObject::dealloc();
}

void CMeshLODEntry::invalidateShaderDependencies()
{
	LRelease(_stateBlock);
	_stateBlock = 0;
}

// Draws the patch, and the skirt along the edges of the mesh it touches.
void CMeshLODEntry::immediateDrawPatch(int level, int x, int y, unsigned char borders)
{
	SSubpatchInfo info = UAdaptiveMeshUtils::subpatchInfoForLevelAtLOD(x, y, level, _lod);
	if (this != sPrevDrawnEntry) {
		setupBuffers();
		sPrevDrawnEntry = this;
	}
	CDrawCache *cache = _holder->_context->_drawCache;
	cache->drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, _numVertices, info.startIndex, info.primCount);
	if (!borders)
		return;
	int last = (1 << level) - 1;
	if (y != 0 && x != 0 && x != last && y != last)
		return;
	int count = 2 << ((_lod >> 1) - level);
	int n = 1 << (_lod >> 1);
	int cells = (_lod & 1) ? n * n * 4 : n * n * 2;
	if (y == 0)
		cache->drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, _numVertices, (x * count + cells) * 3, count);
	if (x == 0)
		cache->drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, _numVertices, (y * count + n * 2 + cells) * 3, count);
	if (x == last)
		cache->drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, _numVertices, (y * count + n * 4 + cells) * 3, count);
	if (y == last)
		cache->drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, _numVertices, (x * count + n * 6 + cells) * 3, count);
}

CIndexBuffer *CMeshLODEntry::sharedIndexBuffer()
{
	LObject *obj = _holder->objectForKey(sIBKeys[_lod]);
	if (obj && obj->getClass()->isSubclassOf(LF_CLASSOF(CIndexBuffer)))
		return (CIndexBuffer *)obj;
	CHolder *holder = _holder;
	if (holder)
		holder->beginAccumulationBlock();
	CRenderProfile *profile = _ds->renderProfile();
	CADMeshDataSource *source = CADMeshDataSource::_alloc()->initWithLOD(_lod, profile);
	CIndexBuffer *ib = CIndexBuffer::_alloc()->initWithDataSource(source, D3DPT_TRIANGLELIST, _holder, 1, 0);
	if (ib) {
		_holder->setObjectForKey(ib, sIBKeys[_lod]);
		ib->release();
	}
	source->release();
	if (holder)
		holder->endAccumulationBlock();
	return ib;
}

CVertexBuffer *CMeshLODEntry::sharedRLODBuffer()
{
	LObject *obj = _holder->objectForKey(sRLODKeys[_lod]);
	if (obj && obj->getClass()->isSubclassOf(LF_CLASSOF(CVertexBuffer)))
		return (CVertexBuffer *)obj;
	CHolder *holder = _holder;
	if (holder)
		holder->beginAccumulationBlock();
	CRenderProfile *profile = _ds->renderProfile();
	CADMeshDataSource *source = CADMeshDataSource::_alloc()->initWithLOD(_lod, profile);
	CVertexBuffer *vb = CVertexBuffer::_alloc()->initWithDataSource(source, _holder, 1, 0);
	if (vb) {
		_holder->setObjectForKey(vb, sRLODKeys[_lod]);
		vb->release();
	}
	source->release();
	if (holder)
		holder->endAccumulationBlock();
	return vb;
}

// Records the streams and indices of this LOD into a state block the first
// time, then applies it.
void CMeshLODEntry::setupBuffers()
{
	CDrawCache *cache = _holder->_context->_drawCache;
	if (_stateBlock == 0) {
		if (!_sMeshLODKeysArraysAreValid) {
			for (int i = 0; i < 20; i++) {
				sIBKeys[i] = LString::_alloc()->initWithFormat("%d_CMeshLODEntry_IB", i);
				sRLODKeys[i] = LString::_alloc()->initWithFormat("%d_CMeshLODEntry_RLOD", i);
			}
			_sMeshLODKeysArraysAreValid = true;
		}
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_stateBlock = CStateBlock::_alloc()->initWithHolder(_holder);
		if (holder)
			holder->endAccumulationBlock();
		{
			StStateRecordZone zone(_stateBlock);
			CRenderProfile *profile = _ds->renderProfile();
			unsigned int streams = profile->_shader->_streamsCount;
			unsigned short rlodStream = 0xffff;
			profile->drawableElementOffset(D3DDECLUSAGE_PSIZE, 0, &rlodStream);
			DeviceEmu *device = _holder->_context->_device;
			for (unsigned int stream = 0; stream < streams; stream++) {
				unsigned int stride = profile->vertexStructSize(stream);
				CVertexBuffer *vb =
				    stream == rlodStream ? sharedRLODBuffer() : _ds->vertexBufferForLOD(stream, _lod);
				device->SetStreamSource(stream, vb->_vb, 0, stride);
			}
			device->SetIndices(sharedIndexBuffer()->_ib);
		}
		if (_stateBlock == 0)
			return;
	}
	cache->applyStateBlock(_stateBlock);
}

// ---------------------------------------------------------------------------
// CAdaptiveFlatMesh

CAdaptiveFlatMesh::CAdaptiveFlatMesh()
    : _ads(0), _adsOwner(0), _lodBias(1.0f), _minLOD(4), _loadedLOD(3), _drawBorders(true), _maxLOD(0), _entries(0),
      _maxDrawnLOD(0), _minDrawnLOD(0), _transformCallback(0), _shaderBound(0)
{
}

void CAdaptiveFlatMesh::dealloc()
{
	LRelease(_entries);
	_entries = 0;
	LRelease(_adsOwner);
	_adsOwner = 0;
	CBuffersOwner::dealloc();
}

// A new shader drops the buffers.
void CAdaptiveFlatMesh::instantiateSelf()
{
	CShader *shader = renderProfile()->_shader;
	if (shader != _shaderBound) {
		_shaderBound = shader;
		invalidateBuffers();
	}
	CBuffersOwner::instantiateSelf();
}

void CAdaptiveFlatMesh::releaseSelf()
{
	invalidateShaderDependencies();
	CBuffersOwner::releaseSelf();
}

static ShaderConstantDef _shaderConstDef_WorldViewZWithBias(0x57565a42, "WorldViewZWithBias", 1, kShaderConstDynamic, 0);

// The 'WVZB' constant: the world-view z column scaled to the LOD scale.
const void *CAdaptiveFlatMesh::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != 0x57565a42)
		return CBuffersOwner::_dataPtrForConstant(descr, buffer);
	CContext *context = _context;
	const Matrix *view = context->_currentRenderTarget->viewMatrix();
	float scale = (_lodBias * 600.0f) / _holder->fullFrameSize().height;
	const Matrix &w = context->_world;
	float *out = (float *)buffer;
	for (int i = 0; i < 4; i++)
		out[i] = (w.m[i][0] * view->_13 + w.m[i][1] * view->_23 + w.m[i][2] * view->_33 + w.m[i][3] * view->_43) *
		         scale;
	return buffer;
}

LClass *CAdaptiveFlatMesh::meshLODEntryClass()
{
	return LF_CLASSOF(CMeshLODEntry);
}

void CAdaptiveFlatMesh::invalidateShaderDependencies()
{
	if (_entries)
		for (int i = 0, n = _entries->count(); i < n; i++)
			((CMeshLODEntry *)_entries->objectAtIndex(i))->invalidateShaderDependencies();
}

CMeshLODEntry *CAdaptiveFlatMesh::entryForLOD(int lod)
{
	int index = lod - 4;
	return index >= 0 && index < (int)_entries->count() ? (CMeshLODEntry *)_entries->objectAtIndex(index) : 0;
}

// floor(log2(v)) for v > 0, from the float's exponent.
static signed char _log2Floor(float v)
{
	int32_t bits;
	memcpy(&bits, &v, 4);
	return (signed char)((bits >> 23) - 127);
}

// Draws the patch with the LOD its z range asks for, or its four children
// when they need different ones.
void CAdaptiveFlatMesh::drawLOD(const SDrawLodParams *params, int level, int x, int y, int intersection)
{
	int index = x + UAdaptiveMeshUtils::_offsetForSubEntries[level] + (y << level);
	_ads->boundsWillBeRequestedForLevel(level);
	Vector3 min(params->mins[index].x, params->mins[index].y, params->minZ[index]);
	Vector3 max(params->maxs[index].x, params->maxs[index].y, params->maxZ[index]);
	if (_transformCallback)
		_transformCallback(&min, &max, params->data);
	if (intersection == 1 && (_loadedLOD >> 1) - level >= 4) {
		if (!params->axisAligned)
			intersection = _context->boxIntersectionTest(&min, &max, params->world, false);
		else {
			const Matrix *m = params->world;
			Vector3 tmin(min.x * m->_11 + m->_41, min.y * m->_22 + m->_42, min.z * m->_33 + m->_43);
			Vector3 tmax(max.x * m->_11 + m->_41, max.y * m->_22 + m->_42, max.z * m->_33 + m->_43);
			intersection = _context->boxIntersectionTest(&tmin, &tmax, false);
		}
		if (intersection == 0)
			return;
	}
	float zA, zB;
	UMathUtils::evalBoxZRange(min, max, params->worldView, zA, zB);
	float a = -zA * params->lodScale * _lodBias;
	signed char expA = 0;
	if (a > 0)
		expA = _log2Floor(a * a);
	float b = -zB * params->lodScale * _lodBias;
	signed char expB = 0;
	if (b > 0)
		expB = _log2Floor(b * b);
	if (expA < _minLOD)
		expA = _minLOD;
	if (expB < _minLOD)
		expB = _minLOD;
	signed char lodA = 24 - expA;
	if (_maxLOD < lodA)
		lodA = _maxLOD;
	signed char lodB = 24 - expB;
	if (_maxLOD < lodB)
		lodB = _maxLOD;
	if (lodB > 4 && lodB > _loadedLOD)
		preloadLODBeforeDraw(lodB);
	signed char lo = _minLOD <= lodA ? (lodA < _loadedLOD ? lodA : _loadedLOD) : _minLOD;
	signed char hi = _minLOD <= lodB ? (lodB <= _loadedLOD ? lodB : _loadedLOD) : _minLOD;
	if ((level < 3 || lo != hi) && level < hi / 2 - 3 && (intersection == 1 || lo < hi)) {
		level++;
		x *= 2;
		y *= 2;
		drawLOD(params, level, x, y, intersection);
		drawLOD(params, level, x + 1, y, intersection);
		drawLOD(params, level, x, y + 1, intersection);
		drawLOD(params, level, x + 1, y + 1, intersection);
		return;
	}
	CMeshLODEntry *entry = (CMeshLODEntry *)_entries->objectAtIndex(hi - 4);
	if (level <= hi >> 1 && entry) {
		if (lodB < _maxDrawnLOD)
			lodB = _maxDrawnLOD;
		_maxDrawnLOD = lodB;
		if (_minDrawnLOD < lodA)
			lodA = _minDrawnLOD;
		_minDrawnLOD = lodA;
		entry->immediateDrawPatch(level, x, y, _drawBorders);
	}
}

// The first draw index draws the skirts too.
void CAdaptiveFlatMesh::drawSubset(CDrawableModelData *data, long, long index)
{
	const Matrix *world = data->worldMatrix();
	_drawBorders = index == 0;
	SDrawLodParams params;
	params.data = data;
	_ads->getMinMaxCorners(&params.mins, &params.maxs, &params.minZ, &params.maxZ);
	_ads->boundsWillBeRequestedForLevel(0);
	CSharableResource *mutex = _ads->boundsMutex();
	std::unique_ptr<StSharableLocker> locker(mutex ? new StSharableLocker(mutex, true) : 0);
	Vector3 min(params.mins[0].x, params.mins[0].y, params.minZ[0]);
	Vector3 max(params.maxs[0].x, params.maxs[0].y, params.maxZ[0]);
	if (_transformCallback)
		_transformCallback(&min, &max, data);
	int intersection = _context->boxIntersectionTest(&min, &max, world, false);
	if (intersection == 0)
		return;
	Matrix worldView;
	FHMatrixMultiply(&worldView, world, _context->_currentRenderTarget->viewMatrix());
	params.axisAligned = world->_12 == 0 && world->_21 == 0 && world->_13 == 0 && world->_31 == 0 &&
	                     world->_23 == 0 && world->_32 == 0;
	params.world = world;
	params.worldView = &worldView;
	params.lodScale = 600.0f / _holder->fullFrameSize().height;
	CMeshLODEntry::sPrevDrawnEntry = 0;
	_maxDrawnLOD = 0;
	_minDrawnLOD = 100;
	drawLOD(&params, 0, 0, 0, intersection);
	analyzeLODRangeAfterDraw(_minDrawnLOD, _maxDrawnLOD);
}

// Keeps the entries of the LODs from 4 to `lod`.
void CAdaptiveFlatMesh::changeCurrentLoadedLODTo(int lod)
{
	if (lod < _minLOD)
		lod = _minLOD;
	if (lod > _maxLOD)
		lod = _maxLOD;
	if (lod == _loadedLOD)
		return;
	if (lod < _loadedLOD) {
		if (lod < 5)
			_entries->removeAllObjects();
		else
			_entries->removeAllObjectsAfter(lod - 4);
	} else
		for (int l = _loadedLOD + 1; l <= lod; l++)
			if (l > 3)
				_entries->addObject(createMeshForLOD(l, l == lod));
	_loadedLOD = (signed char)lod;
}

CMeshLODEntry *CAdaptiveFlatMesh::createMeshForLOD(int lod, unsigned char)
{
	LClass *cls = meshLODEntryClass();
	if (cls == 0)
		LF_RAISE(NilPtrException());
	LObject *obj = LAutorelease(cls->alloc());
	if (obj == 0 || !obj->getClass()->isSubclassOf(LF_CLASSOF(CMeshLODEntry)))
		LF_RAISE(NilPtrException());
	return ((CMeshLODEntry *)obj)->initWithParams(lod, this, _ads);
}

CAdaptiveFlatMesh *CAdaptiveFlatMesh::initWithDataSource(IAdaptiveMeshDataSource *ds, CRenderProfile *profile,
                                                         unsigned char retain)
{
	if (CBuffersOwner::initWithDataSource(0, profile, false) == 0)
		return 0;
	_ads = ds;
	if (retain) {
		_adsOwner = ds ? dynamic_cast<LObject *>(ds) : 0;
		LRetain(_adsOwner);
	}
	_entries = LArray::_alloc()->init();
	_maxLOD = (signed char)(_ads->logGridFullSize() * 2);
	changeCurrentLoadedLODTo(_minLOD);
	return this;
}
