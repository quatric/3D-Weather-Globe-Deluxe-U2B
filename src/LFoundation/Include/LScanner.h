// LScanner: reads an LString from a position. StScannerPosKeeper puts the
// position back at the end of its scope. LMapScanner calls scanKeyAndValue
// for each entry of a map; UMapParsingUtils reads numbers and structs from
// map values.
#pragma once

#include "LString.h"

namespace LF {

class LMap;

class LScanner : public LObject {
	LF_DECLARE_CLASS(LScanner)
public:
	LScanner() : _string(0), _position(0) {}
	virtual ~LScanner() {}
	virtual void dealloc();
	virtual LScanner *getOwnerLObject() { return this; }

	static LScanner *scannerWithString(const LString *str);
	LScanner *initWithString(const LString *str);

	unsigned int position() const { return _position; }
	void setPosition(unsigned int position) { _position = position; }
	bool isAtEnd() const { return _position >= _string->length(); }
	// Clamped to the string.
	void movePositionBy(long delta);
	// These return whether anything is left to scan; `out` (if given) gets
	// the text passed over.
	bool scanToCharSet(LCharSet *set, LString **out);
	bool scanCharSet(LCharSet *set, LString **out);
	// The text between the next `open` and its matching `close`; the
	// position moves past `close`.
	bool scanInsideBrackets(LString *open, LString *close, LString **out);

	LString *_string;
	unsigned int _position;
};

// Its destructor isn't in the original binary; it is taken to restore the
// position, as its name says.
class StScannerPosKeeper {
public:
	StScannerPosKeeper(LScanner *scanner) : _retained(LRetain(scanner)), _scanner(scanner), _position(scanner->_position)
	{
	}
	~StScannerPosKeeper()
	{
		_scanner->_position = _position;
		if (_retained)
			_retained->release();
	}

	LScanner *_retained;
	LScanner *_scanner;
	unsigned int _position;
};

class LMapScanner {
public:
	virtual void scanKeyAndValue(const LComparable *key, LObject *value, const LMap *map) = 0;
	void scanMap(const LMap *map);
};

namespace UMapParsingUtils {
// From an LValue's bytes or an LData's first `size` bytes.
void loadStructFromObject(void *out, unsigned int size, const LObject *obj);
// From an LString or LValue; 0 otherwise.
double loadDoubleFromObject(const LObject *obj);
int loadIntFromObject(const LObject *obj);
float loadFloatFromObject(const LObject *obj);
} // namespace UMapParsingUtils

} // namespace LF
