// LCoder: binary archives of LObjects. Every value is a 4-byte length and
// its bytes; an object is a chunk ("gb", a 4-byte size, its class name and
// its encoded data, "ne") that StWriteChunk/StReadChunk open and close.
// Sizes stay 4 bytes on 64-bit, so archives are the same on every arch.
#pragma once

#include "LData.h"

namespace LF {

class LCoder : public LObject {
	LF_DECLARE_CLASS(LCoder)
public:
	LCoder() : _decodeData(0), _encodeData(0), _chunks(new TArray<unsigned int>), _offset(0), _blockSize(-1) {}
	virtual ~LCoder() {}
	virtual void dealloc();
	virtual LCoder *getOwnerLObject() { return this; }

	// The data of `root` (compressed with LData::compress if asked).
	static LData *archiveRootObject(const LObject *root, bool compress);
	// The object, autoreleased; compressed data is expanded first.
	static LObject *unarchiveRootObject(const LData *data);

	void initWithDataForEncode(LData *data);
	void initWithDataForDecode(const LData *data);

	void storeBytes(const void *bytes, unsigned int length);
	void storeCString(const char *str);
	void encodeObject(const LObject *obj);
	void beginChunk();
	void endChunk();

	// The size of the next value (read once and kept until readBytes).
	unsigned int nextBlockSize() const;
	// Copies the next value; nothing happens if it runs past the data.
	void readBytes(void *out) const;
	const char *readCString() const;
	LObject *decodeObject() const;
	void tryOpenChunk() const;
	void tryCloseChunk() const;

	const LData *_decodeData;
	LData *_encodeData;
	TArray<unsigned int> *_chunks;	// chunk ends (decode) or size offsets (encode)
	mutable unsigned int _offset;
	mutable int _blockSize;
};

class StWriteChunk {
public:
	StWriteChunk(LCoder *coder) : _coder(LRetain(coder)) { _coder->beginChunk(); }
	~StWriteChunk()
	{
		_coder->endChunk();
		_coder->release();
	}
	LCoder *_coder;
};

class StReadChunk {
public:
	StReadChunk(const LCoder *coder) : _coder(LRetain(const_cast<LCoder *>(coder))) { _coder->tryOpenChunk(); }
	~StReadChunk()
	{
		_coder->tryCloseChunk();
		_coder->release();
	}
	LCoder *_coder;
};

} // namespace LF
