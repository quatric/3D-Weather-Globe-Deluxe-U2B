// CStream
//
// LF::CStream and its buffer streams, and CTextBlockStream's scanning.
// Characters are compared as signed, as on the original i386 build, so bytes
// above 0x7f count as whitespace where the code tests for < '!'.

#include "CStream.h"
#include <string>

namespace LF {

void CStream::SetMarker(long offset, EStreamFrom from)
{
	unsigned int m;
	if (from == kStreamFromEnd)
		m = GetLength() - offset;
	else if (from == kStreamFromMarker)
		m = _marker + offset;
	else
		m = from == kStreamFromStart ? (unsigned int)offset : _marker;
	if (GetLength() < m)
		m = GetLength();
	_marker = m;
}

void CStream::SetMarker(unsigned int offset)
{
	unsigned int m = 0;
	if (offset != (unsigned int)-1) {
		m = offset;
		if (GetLength() < offset)
			m = GetLength();
	}
	_marker = m;
}

void CStream::SetLength(unsigned int length)
{
	unsigned int old = GetLength();
	_length = length;
	if (length < old)
		SetMarker(GetMarker());
}

int CStream::PutBytes(const void *, unsigned int)
{
	return kStreamParamErr;
}

int CStream::GetBytes(void *, unsigned int &length)
{
	length = 0;
	return kStreamParamErr;
}

unsigned int CStream::ReadData(void *bytes, unsigned int length)
{
	GetBytes(bytes, length);
	return length;
}

void CStream::ReadBlock(void *bytes, unsigned int length)
{
	if (GetBytes(bytes, length) < 0)
		LF_RAISE(WrongConditionException());
}

unsigned int CStream::PeekData(void *bytes, unsigned int length)
{
	unsigned int m = GetMarker();
	GetBytes(bytes, length);
	SetMarker(m);
	return length;
}

void CStream::ReadCString(char *str)
{
	int32_t n;
	ReadBlock(&n, 4);
	ReadBlock(str, n);
	str[n] = 0;
}

void CStream::WriteBlock(const void *bytes, unsigned int length)
{
	if (PutBytes(bytes, length) < 0)
		LF_RAISE(WrongConditionException());
}

void CStream::WriteCString(const char *str)
{
	uint32_t n = (uint32_t)strlen(str);
	WriteBlock(&n, 4);
	WriteBlock(str, n);
}

// CDataStream

void CDataStream::SetBuffer(void *buffer, unsigned int length)
{
	_buffer = (char *)buffer;
	SetLength(length);
}

int CDataStream::PutBytes(const void *bytes, unsigned int length)
{
	int err = 0;
	if (GetLength() < GetMarker() + length) {
		err = kStreamParamErr;
		length = GetLength() - GetMarker();
	}
	memcpy(_buffer + GetMarker(), bytes, length);
	CStream::SetMarker(length, kStreamFromMarker);
	return err;
}

int CDataStream::GetBytes(void *bytes, unsigned int &length)
{
	int err = 0;
	if (GetLength() < GetMarker() + length) {
		length = GetLength() - GetMarker();
		err = kStreamParamErr;
	}
	memcpy(bytes, _buffer + GetMarker(), length);
	CStream::SetMarker(length, kStreamFromMarker);
	return err;
}

// CMutableDataStream

CMutableDataStream::CMutableDataStream(unsigned int capacity) : _capacity(0)
{
	ExpandBufferBy(capacity);
}

CMutableDataStream::~CMutableDataStream()
{
	delete[] _buffer;
}

void CMutableDataStream::ExpandBufferBy(unsigned int length)
{
	if (length == 0)
		return;
	unsigned int old = GetLength();
	unsigned int blocks = (length + old) >> 10;
	if ((old >> 10) < blocks) {
		char *b = new char[blocks * 0x400 + 0x400];
		if (old != 0 && _buffer != 0)
			memcpy(b, _buffer, old);
		delete[] _buffer;
		_buffer = b;
		SetLength(old + length);
	}
}

int CMutableDataStream::PutBytes(const void *bytes, unsigned int length)
{
	ExpandBufferBy(length);
	return CDataStream::PutBytes(bytes, length);
}

void CMutableDataStream::quickAppend(const void *bytes, unsigned int length)
{
	if (_capacity < _length + length) {
		unsigned int capacity = _capacity + 0x400;
		char *b = new char[capacity];
		if (_capacity != 0 && _buffer != 0)
			memcpy(b, _buffer, _length);
		delete[] _buffer;
		_buffer = b;
		_capacity = capacity;
	}
	memcpy(_buffer + _length, bytes, length);
	_length += length;
}

// CTextBlockStream

CTextBlockStream::~CTextBlockStream()
{
	if (_block) {
		delete _block;
		_block = 0;
	}
}

static inline bool _isSpace(char c)
{
	return (signed char)c < '!';
}

void CTextBlockStream::ToFirstChar(unsigned int &offset)
{
	unsigned int i = offset;
	for (;;) {
		while (i < _length && _isSpace(_data[i]))
			offset = ++i;
		if (_length < 3 || i > _length - 3)
			return;
		if (_data[i] == '/' && _data[i + 1] == '/') {
			while (i < _length && _data[i] != '\r' && _data[i] != '\n')
				offset = ++i;
		} else if (_data[i] == '/' && _data[i + 1] == '*') {
			while (i < _length && !(_data[i] == '*' && _data[i + 1] == '/'))
				offset = ++i;
			i += 2;
			offset = i;
			if (i >= _length)
				i = _length;
			offset = i;
		} else
			return;
	}
}

// As in the original, a quote does not skip the quoted text: the scan for
// the closing quote starts on the opening one.
static unsigned int _skipQuote(const char *data, unsigned int i, unsigned int length, char quote)
{
	while (data[i] != quote) {
		i++;
		if (i == length)
			break;
	}
	return i;
}

unsigned int CTextBlockStream::GetBlockOffsetStart()
{
	unsigned int from = _marker;
	unsigned int length = _length;
	for (;;) {
		if (length <= from)
			return (unsigned int)-1;
		unsigned int i = from;
		char c = 0;
		for (; i < length; i++) {
			c = _data[i];
			if (c == '{' || c == '"' || c == '}' || c == '\'')
				break;
		}
		if (length <= i)
			return (unsigned int)-1;
		if (c == '"' || c == '\'')
			i = _skipQuote(_data, i, length, c);
		else if (c == '{')
			return i;
		else if (c == '}')
			return (unsigned int)-1;
		from = i + 1;
	}
}

unsigned int CTextBlockStream::GetBlockOffsetEnd()
{
	unsigned int from = _marker;
	unsigned int length = _length;
	int depth = 0;
	for (;;) {
		if (length <= from)
			return (unsigned int)-1;
		unsigned int i = from;
		char c = 0;
		for (; i < length; i++) {
			c = _data[i];
			if (c == '{' || c == '"' || c == '}' || c == '\'')
				break;
		}
		if (length <= i)
			return (unsigned int)-1;
		if (c == '"' || c == '\'')
			i = _skipQuote(_data, i, length, c);
		else if (c == '{')
			depth++;
		else if (c == '}' && --depth < 0)
			return i;
		from = i + 1;
	}
}

// The length of the token at the marker.
static unsigned int _tokenLength(const char *data, unsigned int from, unsigned int length)
{
	unsigned int n = 0;
	while (from < length && !_isSpace(data[from++]))
		n++;
	return n;
}

// atof reads past the token, as in the original.
double CTextBlockStream::readDouble()
{
	ToFirstChar(_marker);
	unsigned int n = _tokenLength(_data, _marker, _length);
	double d = atof(_data + _marker);
	_marker += n;
	return d;
}

// Backslash-escaped quotes stay escaped; the first character is never
// taken as the closing quote.
void CTextBlockStream::readString(char *str)
{
	ToFirstChar(_marker);
	unsigned int start = _marker;
	if (_data[start] != '"')
		LF_RAISE(WrongConditionException());
	unsigned int n = 1;
	while (start + n < _length) {
		n++;
		const char *p = _data + start + n;
		if (*p == '"' && p[-1] != '\\')
			break;
	}
	memcpy(str, _data + start + 1, n - 1);
	str[n - 1] = 0;
	_marker += n + 1;
}

int CTextBlockStream::readSInt32()
{
	ToFirstChar(_marker);
	unsigned int start = _marker;
	unsigned int n = _tokenLength(_data, start, _length);
	const signed char *p = (const signed char *)_data + start;
	int value = 0;
	if (*p == '\'') {
		if (p[n - 1] == '\'') {
			value = p[4] + p[3] * 0x100 + p[2] * 0x10000 + p[1] * 0x1000000;
			_marker = start + n;
			return value;
		}
	} else if (*p == '0' && p[1] == 'x') {
		std::string token(_data + start, n);
		unsigned int v = 0;
		sscanf(token.c_str(), "%X", &v);
		_marker = start + n;
		return (int)v;
	}
	std::string token(_data + start, n);
	value = (int)atol(token.c_str());
	_marker = start + n;
	return value;
}

float CTextBlockStream::readFloat()
{
	ToFirstChar(_marker);
	unsigned int n = _tokenLength(_data, _marker, _length);
	std::string token(_data + _marker, n);
	double d = atof(token.c_str());
	_marker += n;
	return (float)d;
}

} // namespace LF
