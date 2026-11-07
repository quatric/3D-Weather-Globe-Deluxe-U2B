// LHeaderParser
//
// LF::LHeaderParser and LF::LHeaderParserCacheCleanup.

#include "LHeaderParser.h"
#include "LCleanupTask.h"
#include "UTextUtils.h"

namespace LF {

LF_IMPLEMENT_CLASS(LHeaderParser, 0)

LObject *LHeaderParser::sResourceHeaderParserCache = 0;

static LConstantString ___defineString("#define");

static CSharableResource *_resourceHeaderParserCacheLock()
{
	static CSharableResource sLock;
	return &sLock;
}

class LHeaderParserCacheCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		StSharableLocker lock(_resourceHeaderParserCacheLock(), true);
		if (LHeaderParser::sResourceHeaderParserCache) {
			LHeaderParser::sResourceHeaderParserCache->release();
			LHeaderParser::sResourceHeaderParserCache = 0;
		}
	}
};
static LHeaderParserCacheCleanup sHeaderParserCacheCleanup;

void LHeaderParser::dealloc()
{
	if (_string) {
		_string->release();
		_string = 0;
	}
	if (_valuesByName) {
		_valuesByName->release();
		_valuesByName = 0;
	}
	if (_namesByValue) {
		_namesByValue->release();
		_namesByValue = 0;
	}
	LObject::dealloc();
}

LHeaderParser *LHeaderParser::initWithString(LString *string)
{
	if (LObject::init() == 0)
		return 0;
	_string = LRetain(string);
	return this;
}

void LHeaderParser::_parseIfNeeded()
{
	if (_parsed)
		return;
	_parsed = true;
	LCharSet *spaces = LCharSet::charSetWithType(LCharSet::kWhitespaceAndNewline);
	const char *eol;
	switch (UTextUtils::lineBreakStyleForBytes(_string->wString(), _string->length())) {
	case UTextUtils::kLFLineBreaks: eol = "\n"; break;
	case UTextUtils::kCRLineBreaks: eol = "\r"; break;
	case UTextUtils::kCRLFLineBreaks: eol = "\r\n"; break;
	default: return;
	}
	LArray *lines = _string->componentsSeparatedByString(LString::stringWithCString(eol), spaces);
	if (lines == 0)
		return;
	int n = lines->count();
	for (int i = 0; i < n; i++) {
		LString *line = (LString *)lines->objectAtIndex(i);
		if (!line->hasPrefix(&___defineString))
			continue;
		unsigned int nameStart = line->findFirstCharNotFromSet(spaces, ___defineString.length() + 1, 1);
		if (nameStart == (unsigned int)-1)
			continue;
		unsigned int nameEnd = line->findFirstCharFromSet(spaces, nameStart + 1, 1);
		if (nameEnd == (unsigned int)-1)
			continue;
		LString *name = line->substringWithRange(nameStart, nameEnd - nameStart);
		unsigned int valueStart = line->findFirstCharNotFromSet(spaces, nameEnd + 1, 1);
		if (valueStart == (unsigned int)-1)
			continue;
		LString *value = line->substringFromIndex(valueStart);
		if (_valuesByName == 0)
			_valuesByName = LMap::_alloc()->init();
		_valuesByName->setObjectForKey(value, name);
		if (_namesByValue == 0)
			_namesByValue = LMap::_alloc()->init();
		LArray *names = (LArray *)_namesByValue->objectForKey(value);
		if (names == 0) {
			names = LArray::array();
			_namesByValue->setObjectForKey(names, value);
		}
		names->addObject(name);
	}
}

int LHeaderParser::intValueForDefine(LString *name)
{
	_parseIfNeeded();
	if (_valuesByName) {
		LString *value = (LString *)_valuesByName->objectForKey(name);
		if (value)
			return value->intValue();
	}
	return 0;
}

LArray *LHeaderParser::definesForIntValue(int value)
{
	_parseIfNeeded();
	if (_namesByValue)
		return (LArray *)_namesByValue->objectForKey(LString::stringWithInt(value));
	return 0;
}

} // namespace LF
