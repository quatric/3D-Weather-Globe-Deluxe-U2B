// CStream: a byte stream with a marker and a length. CDataStream reads and
// writes a fixed buffer, CMutableDataStream grows its own; CBlockStream is a
// read-only text block, which CTextBlockStream parses (skipping whitespace
// and // or /* */ comments).
#pragma once

#include "LFBase.h"
#include "LContainers.h"

namespace LF {

enum EStreamFrom {
	kStreamFromStart = 1,
	kStreamFromEnd = 2,
	kStreamFromMarker = 3,
};

enum { kStreamParamErr = -50 };

class CStream {
public:
	CStream() : _marker(0), _length(0) {}
	virtual ~CStream() {}
	virtual unsigned int GetMarker() const { return _marker; }
	virtual void SetLength(unsigned int length);
	virtual unsigned int GetLength() const { return _length; }
	// 0, or kStreamParamErr when not everything could be written/read.
	virtual int PutBytes(const void *bytes, unsigned int length);
	virtual int GetBytes(void *bytes, unsigned int &length);
	virtual unsigned int ReadData(void *bytes, unsigned int length);
	// A 4-byte length and the characters.
	virtual void WriteCString(const char *str);
	virtual void ReadCString(char *str);

	void SetMarker(long offset, EStreamFrom from);
	// Clamped to the length; -1 means 0.
	void SetMarker(unsigned int offset);
	// These raise WrongConditionException on failure.
	void ReadBlock(void *bytes, unsigned int length);
	void WriteBlock(const void *bytes, unsigned int length);
	unsigned int PeekData(void *bytes, unsigned int length);

	unsigned int _marker;
	unsigned int _length;
};

class CDataStream : public CStream {
public:
	CDataStream() : _buffer(0) {}
	CDataStream(void *buffer, unsigned int length) : _buffer((char *)buffer) { CStream::SetLength(length); }
	virtual ~CDataStream() {}
	virtual int PutBytes(const void *bytes, unsigned int length);
	virtual int GetBytes(void *bytes, unsigned int &length);
	virtual void SetBuffer(void *buffer, unsigned int length);
	virtual void *GetBuffer() { return _buffer; }

	char *_buffer;
};

class CMutableDataStream : public CDataStream {
public:
	CMutableDataStream(unsigned int capacity);
	virtual ~CMutableDataStream();
	virtual int PutBytes(const void *bytes, unsigned int length);
	// Grows the buffer (in 1K steps) and the length, but only when the
	// length crosses into a new 1K block, as in the original.
	void ExpandBufferBy(unsigned int length);
	// Appends at the end, growing the capacity 1K at a time.
	void quickAppend(const void *bytes, unsigned int length);

	unsigned int _capacity;
};

class CBlockStream : public CStream {
public:
	CBlockStream(const char *data, unsigned int length) : _data(data) { CStream::SetLength(length); }
	virtual ~CBlockStream() {}
	virtual unsigned int GetBlockOffsetStart() = 0;
	virtual unsigned int GetBlockOffsetEnd() = 0;
	virtual int readSInt32() = 0;
	virtual float readFloat() = 0;
	virtual double readDouble() = 0;
	virtual void readString(char *str) = 0;

	const char *_data;
};

class CTextBlockStream : public CBlockStream {
public:
	CTextBlockStream(const char *data, unsigned int length) : CBlockStream(data, length), _block(0) {}
	// Reads the block's bytes; the stream deletes the block if `own`.
	CTextBlockStream(StPointerBlock *block, bool own)
	    : CBlockStream((const char *)block->bytes(), block->size()), _block(own ? block : 0)
	{
	}
	virtual ~CTextBlockStream();
	virtual void ReadCString(char *str) { readString(str); }
	// The offset of the next '{' (0xffffffff if a '}' or the end comes first).
	virtual unsigned int GetBlockOffsetStart();
	// The offset of the '}' closing the current block.
	virtual unsigned int GetBlockOffsetEnd();
	// A decimal, 0x hex or 'abcd' four-character number.
	virtual int readSInt32();
	virtual float readFloat();
	virtual double readDouble();
	// A double-quoted string.
	virtual void readString(char *str);

	// Moves *offset past whitespace and comments.
	void ToFirstChar(unsigned int &offset);
	char _at(unsigned int i) const { return _data[i]; }

	StPointerBlock *_block;
};

} // namespace LF
