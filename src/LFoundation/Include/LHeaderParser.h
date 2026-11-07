// LHeaderParser: reads the "#define NAME value" lines of a C header, to look
// values up by name and names up by value.
#pragma once

#include "LMap.h"
#include "LArray.h"

namespace LF {

class LHeaderParser : public LObject {
	LF_DECLARE_CLASS(LHeaderParser)
public:
	LHeaderParser() : _string(0), _parsed(false), _valuesByName(0), _namesByValue(0) {}
	virtual ~LHeaderParser() {}
	virtual void dealloc();
	virtual LHeaderParser *getOwnerLObject() { return this; }

	LHeaderParser *initWithString(LString *string);
	int intValueForDefine(LString *name);
	// The names defined as the value, or 0.
	LArray *definesForIntValue(int value);

	void _parseIfNeeded();

	// Parsers of resource headers, kept until LFoundation is finalized.
	static LObject *sResourceHeaderParserCache;

	LString *_string;
	bool _parsed;
	LMap *_valuesByName;	// name -> value string
	LMap *_namesByValue;	// value string -> LArray of names
};

} // namespace LF
