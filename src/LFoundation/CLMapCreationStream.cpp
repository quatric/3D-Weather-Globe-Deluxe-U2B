// CLMapCreationStream
//
// LF::CLMapCreationStream: the old-style property list parser. Strings
// without quotes end at whitespace or punctuation; in quoted strings \n, \r,
// \t and \\ are unescaped and any other escape becomes a space. Text is read
// as MacRoman.

#include "CLMapCreationStream.h"
#include "LArray.h"
#include "LData.h"
#include "LMap.h"

namespace LF {

CLMapCreationStream::CLMapCreationStream(const LData *data)
    : CTextBlockStream((const char *)data->bytes(), data->length()), _lData(LRetain(const_cast<LData *>(data))),
      _lineStarts(0)
{
}

CLMapCreationStream::~CLMapCreationStream()
{
	_lData->release();
	if (_lineStarts) {
		delete _lineStarts;
		_lineStarts = 0;
	}
}

// Also stops at '/' and skips // comments (but, as in the original, not
// /* */ ones, nor quoted text).
unsigned int CLMapCreationStream::GetBlockOffsetStart(char open, char close)
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
			if (c == open || c == '"' || c == close || c == '/' || c == '\'')
				break;
		}
		if (length <= i)
			return (unsigned int)-1;
		if (c == open)
			return i;
		if (c == close)
			return (unsigned int)-1;
		if (c == '\'' || c == '"') {
			while (_data[i] != c) {
				i++;
				if (i == length)
					break;
			}
		} else if (c == '/') {
			if (i + 1 < length) {
				char next = _data[++i];
				if (next == '/') {
					while (i != length && _data[i] != '\n' && _data[i] != '\r')
						i++;
				} else if (next == '*') {
					while (i < length - 1 && _data[i] != '*' && _data[i + 1] != '/')
						i++;
				}
			} else
				i++;
		}
		from = i + 1;
	}
}

unsigned int CLMapCreationStream::GetBlockOffsetEnd(char open, char close)
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
			if (c == open || c == '"' || c == close || c == '\'')
				break;
		}
		if (length <= i)
			return (unsigned int)-1;
		if (c == '"' || c == '\'') {
			while (_data[i] != c) {
				i++;
				if (i == length)
					break;
			}
		} else if (c == open)
			depth++;
		else if (c == close && --depth < 0)
			return i;
		from = i + 1;
	}
}

// The offsets where lines start (the original tested for 'r' rather than
// '\r' when pairing line breaks).
TArray<unsigned int> *CLMapCreationStream::lineStartMap()
{
	if (_lineStarts == 0) {
		_lineStarts = new TArray<unsigned int>;
		unsigned int i = 0;
		do {
			_lineStarts->addObject(i);
			char c;
			for (;;) {
				if (_length <= i)
					goto done;
				c = _data[i];
				if (c == '\n' || c == '\r')
					break;
				i++;
			}
			unsigned int next = i + 1;
			if (_length <= next) {
				i = next;
				break;
			}
			char d = _data[next];
			if ((d == '\n' || d == 'r') && c != d)
				next = i + 2;
			i = next;
		} while (i < _length);
	done:
		_lineStarts->setKeepsSorted(true);
		_lineStarts->sort();
	}
	return _lineStarts;
}

int CLMapCreationStream::_currentLineNumber()
{
	return lineStartMap()->insertIndexForObject(&_marker, false);
}

void CLMapCreationStream::_throwOnSyntaxError(const char *message, const char *file, int line)
{
	TextStreamException(message, _currentLineNumber()).raise(file, line);
}

LString *CLMapCreationStream::loadString()
{
	ToFirstChar(_marker);
	const char *start = _data + _marker;
	bool quoted = *start == '"';
	unsigned int count = quoted;
	unsigned int length;
	bool escaped = false;
	unsigned int i = count + _marker;
	for (;; i++) {
		if (i >= _length || (quoted && _data[i] == '"')) {
			length = count;
			if (quoted) {
				start++;
				length = count - 1;
			}
			break;
		}
		char c = _data[i];
		if (!quoted && ((signed char)c < '!' || c == ';' || c == '=' || c == ',' || c == '(' || c == ')' || c == '{'
		                || c == '}' || c == '<' || c == '>')) {
			length = count;
			break;
		}
		if (c == '\\')
			escaped = true;
		count++;
	}
	unsigned int n = length;
	StPointerBlock buffer(100, true);
	if (escaped) {
		if (buffer.size() < length)
			buffer.setSize(length);
		char *out = (char *)buffer.bytes();
		const char *p = start;
		for (int k = 0; k < (int)length; k++) {
			char c = *p++;
			if (c == '\\') {
				if (k < (int)length - 1) {
					k++;
					n--;
					char e = *p++;
					c = e == 'r' ? '\r' : e == 'n' ? '\n' : e == 't' ? '\t' : e == '\\' ? '\\' : ' ';
				} else
					c = '\\';
			}
			*out++ = c;
		}
		start = (const char *)buffer.bytes();
	}
	if (n == 0)
		start = "";
	LString *str = LString::stringWithCString(start, n, kLMacRomanEncoding);
	if (quoted)
		count++;
	_marker += count;
	return str;
}

LObject *CLMapCreationStream::scanObject()
{
	ToFirstChar(_marker);
	unsigned int at = _marker;
	const char *p = _data + at;
	char c = *p;
	if (c == '@') {
		// The original tested p[1] rather than the character after the '/'.
		int n = 0;
		while (at + n < _length) {
			n++;
			if (p[n] == ';' || (p[n] == '/' && p[1] == '/'))
				break;
		}
		LString *str = LString::stringWithCString(p, n, kLMacRomanEncoding);
		_marker += n;
		return str;
	}
	if (c == '(') {
		SetMarker(at + 1);
		unsigned int end = GetBlockOffsetEnd('(', ')');
		LArray *a = LArray::array();
		scanArray(end, a);
		return a;
	}
	if (c == '{') {
		SetMarker(at + 1);
		unsigned int end = GetBlockOffsetEnd('{', '}');
		LMap *m = LMap::map();
		scanMap(end, m);
		return m;
	}
	if (c == '<') {
		SetMarker(at + 1);
		unsigned int end = GetBlockOffsetEnd('<', '>');
		LData *text = LData::dataWithBytes(_data + _marker - 1, end - _marker + 1);
		SetMarker(end + 1);
		return LAutorelease(LData::_alloc()->initWithTextRepresentation(text));
	}
	if (c == ')' || c == '}' || c == '>')
		_throwOnSyntaxError("unexcpected closing bracket found", __FILE__, __LINE__);
	return loadString();
}

void CLMapCreationStream::scanMap(unsigned int end, LMap *map)
{
	for (;;) {
		if (end < _marker)
			return;
		ToFirstChar(_marker);
		if (_data[_marker] == '}')
			break;
		LString *key = loadString();
		if (end <= _marker)
			return;
		ToFirstChar(_marker);
		if (_data[_marker] != '=') {
			_throwOnSyntaxError("'=' not found between key and value pair in a map", __FILE__, __LINE__);
			return;
		}
		_marker++;
		LObject *value = scanObject();
		if (value == 0) {
			_throwOnSyntaxError("object was not found in a map", __FILE__, __LINE__);
			return;
		}
		map->setObjectForKey(value, key);
		ToFirstChar(_marker);
		if (_data[_marker] != ';')
			_throwOnSyntaxError("';' was not found at the end of key-value pair", __FILE__, __LINE__);
		_marker++;
		ToFirstChar(_marker);
		if (_data[_marker] == '}')
			break;
	}
	_marker++;
}

void CLMapCreationStream::scanArray(unsigned int end, LArray *array)
{
	for (;;) {
		if (end < _marker)
			return;
		ToFirstChar(_marker);
		if (_data[_marker] == ')')
			break;
		LObject *obj = scanObject();
		if (obj == 0)
			return;
		array->addObject(obj);
		ToFirstChar(_marker);
		if (_data[_marker] == ')')
			break;
		if (_data[_marker] != ',')
			_throwOnSyntaxError("comma is missed between array elements", __FILE__, __LINE__);
		_marker++;
	}
	_marker++;
}

} // namespace LF
