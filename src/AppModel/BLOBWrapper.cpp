// BLOBWrapper and its subclasses: the binary objects of the atlas database
// (outlines, lookup grids, texts, property lists, images), loaded lazily
// from the BLOBs table. Multi-byte values in the BLOBs are big-endian.

#include "AppModel.h"
#include <libkern/OSByteOrder.h>

LF_IMPLEMENT_CLASS(BLOBWrapper, 0)
LF_IMPLEMENT_CLASS(ImageBLOB, LF_CLASSOF(BLOBWrapper))
LF_IMPLEMENT_CLASS(IndexBuffersBLOB, LF_CLASSOF(BLOBWrapper))
LF_IMPLEMENT_CLASS(PlistBLOB, LF_CLASSOF(BLOBWrapper))
LF_IMPLEMENT_CLASS(TextBLOB, LF_CLASSOF(BLOBWrapper))
LF_IMPLEMENT_CLASS(LookupGridBLOB, LF_CLASSOF(BLOBWrapper))
LF_IMPLEMENT_CLASS(VertexBuffersBLOB, LF_CLASSOF(BLOBWrapper))
LF_IMPLEMENT_CLASS(VertexBuffersBLOB::VBDataSource, 0)

LMap *BLOBWrapper::__kindToClassRemap = 0;
LMap *VertexBuffersBLOB::_loadedVBs = 0;

class _blobWrpCleaner : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		if (BLOBWrapper::__kindToClassRemap) {
			BLOBWrapper::__kindToClassRemap->release();
			BLOBWrapper::__kindToClassRemap = 0;
		}
		if (VertexBuffersBLOB::_loadedVBs) {
			VertexBuffersBLOB::_loadedVBs->release();
			VertexBuffersBLOB::_loadedVBs = 0;
		}
	}
};
static _blobWrpCleaner _sBlobWrpCleaner;

static inline uint32_t readBE32(const void *p)
{
	return OSReadBigInt32(p, 0);
}

// ---------------------------------------------------------------------------
// BLOBWrapper

BLOBWrapper::BLOBWrapper() : _data(0), _uid(0), _kind(0)
{
}

void BLOBWrapper::dealloc()
{
	if (_data) {
		_data->release();
		_data = 0;
	}
	if (_uid) {
		_uid->release();
		_uid = 0;
	}
	if (_kind) {
		_kind->release();
		_kind = 0;
	}
	LObject::dealloc();
}

BLOBWrapper *BLOBWrapper::initWithData(LData *data, LString *kind, LString *uid)
{
	if (!LObject::init())
		return 0;
	if (uid == 0)
		uid = AtlasDatabase::generateUID();
	_data = LRetain(data);
	_uid = LRetain(uid);
	_kind = LRetain(kind);
	return this;
}

// The data is read from the database the first time it is asked for.
LData *BLOBWrapper::blobData()
{
	if (_data == 0) {
		SQLiteResultsContainer *results =
		    AtlasDatabase::instance()->searchWithRequest(SQL_DB_Strings::selectBLOBDataWithUID(_uid), 0);
		if (results && results->count() != 0)
			_data = LRetain((LData *)results->objectAtIndex(0)->objectForKey(kColumnNameBLOBData));
	}
	return _data;
}

// The data, made writable for decoding in place.
void *BLOBWrapper::_mutableBlobBytes()
{
	LData *data = blobData();
	if (data == 0)
		return 0;
	data->setLength(data->length());
	return data->bytes();
}

LClass *BLOBWrapper::_classWrapperForKind(LString *kind)
{
	if (__kindToClassRemap == 0)
		__kindToClassRemap = LRetain(LMap::mapWithObjectsAndKeys(
		    LString::stringWithCString("ImageBLOB"), kBlobKindGif, LString::stringWithCString("ImageBLOB"),
		    kBlobKindJpg, LString::stringWithCString("ImageBLOB"), kBlobKindJpeg,
		    LString::stringWithCString("ImageBLOB"), kBlobKindPng, LString::stringWithCString("ImageBLOB"),
		    kBlobKindTif, LString::stringWithCString("ImageBLOB"), kBlobKindTiff,
		    LString::stringWithCString("PlistBLOB"), kBlobKindPlistXml, LString::stringWithCString("TextBLOB"),
		    kBlobKindText, LString::stringWithCString("LookupGridBLOB"), kBlobKindLookupGrid,
		    LString::stringWithCString("VertexBuffersBLOB"), kBlobKindShapeVerts,
		    LString::stringWithCString("IndexBuffersBLOB"), kBlobKindPolyInds, 0));
	LString *className = (LString *)__kindToClassRemap->objectForKey(kind);
	return className ? LClass::classByName(className) : 0;
}

// A wrapper of the class for the BLOB's kind.
BLOBWrapper *BLOBWrapper::wrapperWithBLOBUid(LString *uid)
{
	LArray *rows = AtlasDatabase::instance()->simpleSearchWithRequest(SQL_DB_Strings::selectBLOBKindWithUID(uid));
	if (rows == 0 || rows->count() != 1)
		return 0;
	LString *kind = (LString *)((LMap *)rows->objectAtIndex(0))->objectForKey(kColumnNameKind);
	if (kind == 0)
		return 0;
	LClass *cls = _classWrapperForKind(kind->lowercaseString());
	if (cls == 0)
		return 0;
	BLOBWrapper *wrapper = ((BLOBWrapper *)cls->alloc())->initWithData(0, kind, uid);
	return wrapper ? (BLOBWrapper *)wrapper->l_autorelease() : 0;
}

// The object's BLOB of a representation type.
BLOBWrapper *BLOBWrapper::wrapperForObjectWithUID(LString *objectUID, LString *repType)
{
	LArray *rows = AtlasDatabase::instance()->simpleSearchWithRequest(
	    SQL_DB_Strings::selectBLOBUIDWithRepresentationOfType(repType, objectUID));
	if (rows == 0 || rows->count() != 1)
		return 0;
	return wrapperWithBLOBUid((LString *)((LMap *)rows->objectAtIndex(0))->objectForKey(kColumnNameBLOBUid));
}

// ---------------------------------------------------------------------------
// IndexBuffersBLOB

void IndexBuffersBLOB::dealloc()
{
	BLOBWrapper::dealloc();
}

// ---------------------------------------------------------------------------
// PlistBLOB: a compressed XML property list holding a name and an object.

PlistBLOB::PlistBLOB() : _dict(0)
{
}

void PlistBLOB::dealloc()
{
	if (_dict) {
		_dict->release();
		_dict = 0;
	}
	BLOBWrapper::dealloc();
}

PlistBLOB *PlistBLOB::initWithParams(LString *uid, LString *name, LObject *object)
{
	if (!initWithData(0, kBlobKindPlistXml, uid))
		return 0;
	_dict = LMap::_alloc()->init();
	if (name)
		_dict->setObjectForKey(name, kPlistBlobNameKey);
	if (object)
		_dict->setObjectForKey(object, kPlistBlobPlistKey);
	return this;
}

LMap *PlistBLOB::_dictionary()
{
	if (_dict == 0) {
		LData *data = blobData();
		LData *expanded = data ? data->detachDecomressedData() : 0;
		_dict = LRetain(lobj_cast<LMap>(expanded ? expanded->propertyList() : 0));
	}
	return _dict;
}

LObject *PlistBLOB::object()
{
	LMap *dict = _dictionary();
	return dict ? dict->objectForKey(kPlistBlobPlistKey) : 0;
}

LString *PlistBLOB::name()
{
	LMap *dict = _dictionary();
	return dict ? lobj_cast<LString>(dict->objectForKey(kPlistBlobNameKey)) : 0;
}

// A new dictionary is encoded when the data is first asked for.
LData *PlistBLOB::blobData()
{
	if (_dict && _data == 0) {
		const char *xml = _dict->plistRepresentation(0)->UTF8String();
		_data = LRetain(LData::dataWithBytes(xml, (unsigned int)strlen(xml)));
		_data->compress();
	}
	return BLOBWrapper::blobData();
}

// ---------------------------------------------------------------------------
// TextBLOB: UTF-8 text.

TextBLOB::TextBLOB() : _text(0)
{
}

void TextBLOB::dealloc()
{
	if (_text) {
		_text->release();
		_text = 0;
	}
	BLOBWrapper::dealloc();
}

TextBLOB *TextBLOB::initWithParams(LString *uid, LString *text)
{
	if (!initWithData(0, kBlobKindText, uid))
		return 0;
	_text = LRetain(text);
	return this;
}

LData *TextBLOB::blobData()
{
	if (_text && _data == 0) {
		const char *utf8 = _text->UTF8String();
		_data = LRetain(LData::dataWithBytes(utf8, (unsigned int)strlen(utf8)));
	}
	return BLOBWrapper::blobData();
}

LString *TextBLOB::text()
{
	if (_text == 0) {
		LData *data = blobData();
		if (data)
			_text = LString::_alloc()->initWithCString((const char *)data->bytes(), data->length(), kLUTF8Encoding);
	}
	return _text;
}

// ---------------------------------------------------------------------------
// LookupGridBLOB: which objects cover each cell of a lat/long grid, and a
// coverage quad tree per object and hemisphere.
//
// Layout: cell width and height (floats), grid width and height (shorts),
// the record count, then per record x, y, a UID count and the 12-byte UID
// entries (10 characters used).

LookupGridBLOB::LookupGridBLOB() : _lookup(0), _eastTrees(0), _westTrees(0), _trees(0)
{
}

void LookupGridBLOB::dealloc()
{
	if (_lookup) {
		delete _lookup;
		_lookup = 0;
	}
	if (_eastTrees) {
		delete _eastTrees;
		_eastTrees = 0;
	}
	if (_westTrees) {
		delete _westTrees;
		_westTrees = 0;
	}
	if (_trees) {
		_trees->release();
		_trees = 0;
	}
	BLOBWrapper::dealloc();
}

// Maps a cell key, x | y << 16, to its UID count, which the UIDs follow.
CHashTable *LookupGridBLOB::_lookupDict()
{
	if (_lookup)
		return _lookup;
	uint32_t *p = (uint32_t *)_mutableBlobBytes();
	if (p == 0)
		return 0;
	LSingleProfilerPoint profile("lookup grid parsing");
	uint32_t w = readBE32(&p[0]), h = readBE32(&p[1]);
	memcpy(&_cellWidth, &w, 4);
	memcpy(&_cellHeight, &h, 4);
	_gridWidth = (short)OSReadBigInt16(p, 8);
	_gridHeight = (short)OSReadBigInt16(p, 10);
	int records = (int)readBE32(&p[3]);
	float cellW = 1.0f / _gridWidth;
	float cellH = 1.0f / _gridHeight;
	_lookup = new CHashTable(records);
	uint32_t *record = p + 4;
	for (int i = 0; i < records; i++) {
		uint32_t x = readBE32(&record[0]);
		uint32_t y = readBE32(&record[1]);
		record[2] = readBE32(&record[2]);
		int count = (int)record[2];
		_lookup->setValueForKey((intptr_t)&record[2], (intptr_t)((y & 0xffff) << 16 | x));
		uint32_t *next = record + count * 3 + 3;

		// Each hemisphere's trees cover it as the unit square.
		float fx = cellW * (int)x;
		_QuadContainerStrMap *trees;
		if (fx < 0.5f) {
			if (_westTrees == 0)
				_westTrees = new _QuadContainerStrMap;
			trees = _westTrees;
			fx = fx + fx;
		} else {
			if (_eastTrees == 0)
				_eastTrees = new _QuadContainerStrMap;
			trees = _eastTrees;
			fx = fx + fx - 1.0f;
		}
		const char *uid = (const char *)(record + 3);
		for (int j = 0; j < count; j++, uid += 12) {
			QuadTreeContainer *tree = trees->objectForKey(uid);
			if (tree == 0) {
				tree = QuadTreeContainer::_alloc()->initWithLogSize(7);
				trees->setObjectForKey(tree, uid);
				if (_trees == 0)
					_trees = LArray::_alloc()->init();
				_trees->addObject(tree);
				tree->release();
			}
			LRect rect;
			rect.origin.x = fx;
			rect.origin.y = (int)y * cellH;
			rect.size.width = cellW;
			rect.size.height = cellH;
			tree->setElementsInRect(rect, 1);
		}
		record = next;
	}
	return _lookup;
}

QuadTreeContainer *LookupGridBLOB::treeForObjectWithUID(LString *uid, unsigned char east)
{
	_lookupDict();
	_QuadContainerStrMap *trees = east ? _eastTrees : _westTrees;
	return trees ? trees->objectForKey(uid->cString()) : 0;
}

// The UIDs of the objects covering the cell of the point.
LArray *LookupGridBLOB::objectUIDsBelowGeoPoint(LPoint geoPoint)
{
	CHashTable *lookup = _lookupDict();
	if (lookup == 0)
		return 0;
	intptr_t value = 0;
	intptr_t key = (int)((geoPoint.x + 180.0f) / _cellWidth) | (int)((geoPoint.y + 90.0f) / _cellHeight) << 16;
	if (!lookup->getValueForKey(&value, key) || value == 0)
		return 0;
	const uint32_t *entry = (const uint32_t *)value;
	int count = (int)entry[0];
	const char *uid = (const char *)(entry + 1);
	LSet *uids = LSet::set();
	for (int i = 0; i < count; i++, uid += 12)
		uids->addObject(LString::stringWithCString(uid, 10, kLMacRomanEncoding));
	return uids->allObjects();
}

// ---------------------------------------------------------------------------
// VertexBuffersBLOB: geo points of an outline, in up to 100 chunks of 6
// levels of detail each, made into device vertex buffers on demand.
//
// Layout: the level count, then per level its index and chunk count, and
// per chunk its index, point count and the points (two floats each).

VertexBuffersBLOB::VertexBuffersBLOB() : _parsed(false), _deviceBuffers(0), _lastLod(-1), _projection(0)
{
	memset(_points, 0, sizeof(_points));
}

void VertexBuffersBLOB::_releaseDeviceBuffers()
{
	if (_deviceBuffers == 0)
		return;
	CHashTableIterator it(_deviceBuffers);
	while (it.next()) {
		LObject *vb = (LObject *)it.value();
		if (vb)
			vb->release();
	}
	delete _deviceBuffers;
	_deviceBuffers = 0;
}

void VertexBuffersBLOB::dealloc()
{
	_releaseDeviceBuffers();
	BLOBWrapper::dealloc();
}

void VertexBuffersBLOB::invalidateCaches()
{
	if (_loadedVBs) {
		_loadedVBs->release();
		_loadedVBs = 0;
	}
}

// The points of one level of a chunk, after the data is byte-swapped in
// place the first time.
LPoint *VertexBuffersBLOB::pointsForBufferAtIndex(int chunk, int lod)
{
	if (!_parsed) {
		uint32_t *p = (uint32_t *)_mutableBlobBytes();
		if (p == 0)
			return 0;
		int lods = (int)readBE32(p++);
		for (int i = 0; i < lods; i++) {
			uint32_t lod = readBE32(&p[0]);
			int chunks = (int)readBE32(&p[1]);
			p += 2;
			for (int c = 0; c < chunks; c++) {
				if ((int)readBE32(&p[0]) != c)
					return 0;
				_points[c][lod] = p + 1;
				p[1] = readBE32(&p[1]);
				int count = (int)p[1];
				uint32_t *pt = p + 2;
				for (int k = 0; k < count; k++, pt += 2) {
					pt[0] = readBE32(&pt[0]);
					pt[1] = readBE32(&pt[1]);
				}
				p = pt;
			}
		}
		_parsed = true;
	}
	return (LPoint *)(_points[chunk][lod] + 1);
}

// A device buffer of the projected points; all are dropped when the level
// of detail or the projection changes.
CVertexBuffer *VertexBuffersBLOB::deviceBufferAtIndex(int chunk, int lod, CProjection *projection,
                                                      CRenderProfile *profile)
{
	if (_lastLod != lod || projection != _projection) {
		_projection = projection;
		_lastLod = lod;
		_releaseDeviceBuffers();
	}
	intptr_t vb = 0;
	intptr_t key = (lod & 0xffff) | chunk << 16;
	if (_deviceBuffers == 0 || !_deviceBuffers->getValueForKey(&vb, key)) {
		CHolder *holder = profile->holder();
		if (holder)
			holder->beginAccumulationBlock();
		VBDataSource *source = VBDataSource::dataSourceWithBLOB(this, chunk, lod, profile);
		vb = (intptr_t)CVertexBuffer::_alloc()->initWithDataSource(source, holder, 1, 0);
		if (vb) {
			if (_deviceBuffers == 0)
				_deviceBuffers = new CHashTable;
			_deviceBuffers->setValueForKey(vb, key);
		}
		if (holder)
			holder->endAccumulationBlock();
	}
	return (CVertexBuffer *)vb;
}

// The loaded vertex BLOBs are kept until invalidateCaches.
VertexBuffersBLOB *VertexBuffersBLOB::verticesWithUID(LString *uid)
{
	if (_loadedVBs == 0)
		_loadedVBs = LMap::_alloc()->init();
	VertexBuffersBLOB *blob = (VertexBuffersBLOB *)_loadedVBs->objectForKey(uid);
	if (blob == 0) {
		blob = lobj_cast<VertexBuffersBLOB>(BLOBWrapper::wrapperWithBLOBUid(uid));
		if (blob)
			_loadedVBs->setObjectForKey(blob, uid);
		else
			LLog("Unable to create VB BLOB with blob_uid %@\n", uid);
	}
	return blob;
}

VertexBuffersBLOB::VBDataSource::VBDataSource() : _blob(0), _profile(0), _chunk(0), _lod(0)
{
}

VertexBuffersBLOB::VBDataSource *VertexBuffersBLOB::VBDataSource::dataSourceWithBLOB(VertexBuffersBLOB *blob,
                                                                                     int chunk, int lod,
                                                                                     CRenderProfile *profile)
{
	VBDataSource *source = new VBDataSource;
	source->_blob = blob;
	source->_chunk = chunk;
	source->_lod = lod;
	source->_profile = profile;
	source->l_autorelease();
	return source;
}

unsigned int VertexBuffersBLOB::VBDataSource::vertCountForBuffer(CVertexBuffer *)
{
	_blob->pointsForBufferAtIndex(0, 0);
	return *_blob->_points[_chunk][_lod];
}

CShader *VertexBuffersBLOB::VBDataSource::renderShaderForBuffer(CVertexBuffer *)
{
	return _profile->shader();
}

unsigned int VertexBuffersBLOB::VBDataSource::streamNumberForBuffer(CVertexBuffer *)
{
	return 0;
}

void VertexBuffersBLOB::VBDataSource::fillBuffer(CVertexBuffer *vb)
{
	std::unique_ptr<StVertexBufferLocker> locker(vb->detachLocker(0));
	Vector3 *dst = (Vector3 *)locker->data();
	unsigned int count = vertCountForBuffer(vb);
	const LPoint *src = _blob->pointsForBufferAtIndex(_chunk, _lod);
	CProjection *projection = _blob->_projection;
	for (unsigned int i = 0; i < count; i++)
		projection->scenePointFromGeoPoint(dst++, src++);
}
