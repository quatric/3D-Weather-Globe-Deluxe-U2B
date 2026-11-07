// LCoder
//
// LF::LCoder: writes and reads the chunked binary archive format.

#include "LCoder.h"

namespace LF {

LF_IMPLEMENT_CLASS(LCoder, 0)

static const char kChunkBegin[2] = { 'g', 'b' };
static const char kChunkEnd[2] = { 'n', 'e' };

static uint32_t _read32(const void *p)
{
	uint32_t v;
	memcpy(&v, p, 4);
	return v;
}

static void _write32(void *p, uint32_t v)
{
	memcpy(p, &v, 4);
}

void LCoder::initWithDataForEncode(LData *data)
{
	_encodeData = LRetain(data);
	_offset = 0;
	_blockSize = -1;
}

void LCoder::initWithDataForDecode(const LData *data)
{
	_decodeData = LRetain(const_cast<LData *>(data));
	_offset = 0;
	_blockSize = -1;
}

void LCoder::dealloc()
{
	if (_encodeData) {
		_encodeData->release();
		_encodeData = 0;
	}
	if (_decodeData)
		const_cast<LData *>(_decodeData)->release();
	if (_chunks) {
		delete _chunks;
		_chunks = 0;
	}
	LObject::dealloc();
}

const char *LCoder::readCString() const
{
	const char *p = (const char *)_decodeData->bytes() + _offset;
	_offset += _read32(p) + 5;
	return p + 4;
}

void LCoder::storeBytes(const void *bytes, unsigned int length)
{
	unsigned int at = _encodeData->length();
	_encodeData->setLength(at + 4 + length);
	char *p = (char *)_encodeData->bytes() + at;
	_write32(p, length);
	memcpy(p + 4, bytes, length);
}

void LCoder::storeCString(const char *str)
{
	size_t n = strlen(str);
	unsigned int at = _encodeData->length();
	_encodeData->setLength(at + (unsigned int)n + 5);
	char *p = (char *)_encodeData->bytes() + at;
	_write32(p, (uint32_t)n);
	memcpy(p + 4, str, n + 1);
}

// "gb" and room for the size, which endChunk fills in.
void LCoder::beginChunk()
{
	unsigned int at = _encodeData->length();
	unsigned int sizeAt = at + 2;
	_encodeData->setLength(at + 6);
	memcpy((char *)_encodeData->bytes() + at, kChunkBegin, 2);
	_chunks->addObject(sizeAt);
}

void LCoder::endChunk()
{
	unsigned int sizeAt = _chunks->lastObject();
	_chunks->removeObjectAtIndex(_chunks->count() - 1);
	unsigned int at = _encodeData->length();
	_encodeData->setLength(at + 2);
	char *p = (char *)_encodeData->bytes();
	_write32(p + sizeAt, at - sizeAt - 4);
	memcpy(p + at, kChunkEnd, 2);
}

unsigned int LCoder::nextBlockSize() const
{
	if (_blockSize == -1) {
		_blockSize = (int)_read32((const char *)_decodeData->bytes() + _offset);
		_offset += 4;
	}
	return _blockSize;
}

void LCoder::readBytes(void *out) const
{
	unsigned int n = nextBlockSize();
	if (n + _offset <= _decodeData->length()) {
		_blockSize = -1;
		memcpy(out, (const char *)_decodeData->bytes() + _offset, n);
		_offset += n;
	}
}

void LCoder::tryOpenChunk() const
{
	const char *p = (const char *)_decodeData->bytes();
	if (memcmp(p + _offset, kChunkBegin, 2) != 0)
		LF_RAISE(WrongConditionException());
	_offset += 2;
	unsigned int end = _offset + _read32(p + _offset) + 4;
	if (memcmp(p + end, kChunkEnd, 2) != 0)
		LF_RAISE(WrongConditionException());
	_chunks->addObject(end);
	_offset += 4;
}

void LCoder::tryCloseChunk() const
{
	unsigned int end = _chunks->lastObject();
	_chunks->removeObjectAtIndex(_chunks->count() - 1);
	_offset = end + 2;
}

LObject *LCoder::decodeObject() const
{
	LObject *obj;
	{
		StReadChunk chunk(this);
		LClass *cls = LClass::classByName(readCString());
		if (cls == 0)
			LF_RAISE(NilPtrException());
		obj = cls->alloc();
		if (obj == 0)
			LF_RAISE(NilPtrException());
		obj->initWithDecoder(this);
	}
	return obj->l_autorelease();
}

void LCoder::encodeObject(const LObject *obj)
{
	StWriteChunk chunk(this);
	storeCString(obj->className());
	obj->encodeWithCoder(this);
}

LObject *LCoder::unarchiveRootObject(const LData *data)
{
	const char *p = data ? (const char *)data->bytes() : 0;
	unsigned int n = data->length();
	if (n >= 4 && memcmp(p + n - 4, "zdat", 4) == 0)
		data = data->detachDecomressedData();
	LCoder *coder = _alloc();
	coder->initWithDataForDecode(data);
	return LAutorelease(coder)->decodeObject();
}

LData *LCoder::archiveRootObject(const LObject *root, bool compress)
{
	LData *data = LData::dataWithLength(0);
	LAutoreleasePool::addNewPoolToTop();
	LCoder *coder = _alloc();
	coder->initWithDataForEncode(data);
	LAutorelease(coder)->encodeObject(root);
	LAutoreleasePool::removeTopPool();
	if (compress)
		data->compress();
	return data;
}

} // namespace LF
