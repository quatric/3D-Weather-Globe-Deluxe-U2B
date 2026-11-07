// LString
//
// LF::LString keeps its text in a CFMutableString whose UTF-16 buffer is
// external and owned by the string: _setLength() reallocates it with room
// for a terminating zero, which chars() writes. Most editing is done
// directly on that buffer; cString() and UTF8String() are cached until the
// next change.
//
// Formatting walks the format and takes each argument from the va_list by
// its conversion (the original rebuilt an i386 argument block for
// CFStringAppendFormatAndArguments): %@ is an LString or the object's
// description(), and a plain %s is a MacRoman C string.

#include "LString.h"
#include "LArray.h"
#include "LCoder.h"
#include "LData.h"
#include "LFileManager.h"
#include "LValue.h"
#include "LXMLTree.h"
#include "UTextUtils.h"
#include <wctype.h>

namespace LF {

LF_IMPLEMENT_CLASS(LString, LF_CLASSOF(LComparable))

static LConstantString sEmptyStr("");
static LConstantString sYesStr("YES");
static LConstantString sTrueStr("TRUE");
static LConstantString sOpenCloseSym("\"");
static LConstantString sGreaterThanSym(">");
static LConstantString sLessThanSym("<");
static LConstantString sAmpersandSym("&");
static LConstantString sGreaterThanSymXML("&gt;");
static LConstantString sLessThanSymXML("&lt;");
static LConstantString sAmpersandSymXML("&amp;");
static LConstantString sNullString("(null)");

static const CFStringEncoding kEncodings[] = { kCFStringEncodingMacRoman, kCFStringEncodingUTF8 };

// ---------------------------------------------------------------------------
// Storage

static void *_allocate(CFIndex size, CFOptionFlags, void *)
{
	return malloc(size);
}

static void *_reallocate(void *ptr, CFIndex size, CFOptionFlags, void *)
{
	return realloc(ptr, size);
}

static void _deallocate(void *ptr, void *)
{
	free(ptr);
}

UniChar *LString::_setLength(unsigned int length)
{
	static CFAllocatorRef sAllocator = 0;
	static pthread_once_t once = PTHREAD_ONCE_INIT;
	pthread_once(&once, [] {
		CFAllocatorContext ctx;
		memset(&ctx, 0, sizeof ctx);
		ctx.allocate = _allocate;
		ctx.reallocate = _reallocate;
		ctx.deallocate = _deallocate;
		sAllocator = CFAllocatorCreate(kCFAllocatorDefault, &ctx);
	});
	CFIndex bytes = length * 2 + 2;
	UniChar *buf;
	if (_cf == 0) {
		buf = (UniChar *)CFAllocatorAllocate(sAllocator, bytes, 0);
		_cf = CFStringCreateMutableWithExternalCharactersNoCopy(kCFAllocatorDefault, buf, length, length,
		                                                        sAllocator);
	} else {
		buf = (UniChar *)CFAllocatorReallocate(sAllocator, (void *)CFStringGetCharactersPtr(_cf), bytes, 0);
		CFStringSetExternalCharactersNoCopy(_cf, buf, length, length);
	}
	buf[length] = 0;
	return buf;
}

void LString::_resetCaches()
{
	_hashCached = false;
	if (_cStringCache) {
		delete[] _cStringCache;
		_cStringCache = 0;
	}
	if (_utf8Cache) {
		delete[] _utf8Cache;
		_utf8Cache = 0;
	}
}

void LString::dealloc()
{
	if (_cf)
		CFRelease(_cf);
	_cf = 0;
	_resetCaches();
	LObject::dealloc();
}

LConstantString::~LConstantString()
{
	if (_cf)
		CFRelease(_cf);
	_cf = 0;
	_resetCaches();
}

LString *LString::init()
{
	_setLength(0);
	return this;
}

const UniChar *LString::chars() const
{
	UniChar *p = (UniChar *)CFStringGetCharactersPtr(_cf);
	p[CFStringGetLength(_cf)] = 0;
	return p;
}

// After CF has edited the string itself, put it back in a buffer of ours
// with room for the terminator.
static void _rebuffer(LString *s)
{
	CFIndex n = CFStringGetLength(s->_cf);
	UniChar *tmp = (UniChar *)malloc(n * 2 + 2);
	CFStringGetCharacters(s->_cf, CFRangeMake(0, n), tmp);
	CFRelease(s->_cf);
	s->_cf = 0;
	memcpy(s->_setLength((unsigned int)n), tmp, n * 2);
	free(tmp);
	s->_resetCaches();
}

// ---------------------------------------------------------------------------
// Value

void LString::setValue(CFStringRef str)
{
	unsigned int n = str ? (unsigned int)CFStringGetLength(str) : 0;
	UniChar *buf = _setLength(n);
	if (n)
		CFStringGetCharacters(str, CFRangeMake(0, n), buf);
	_resetCaches();
}

bool LString::setValue(const char *str, unsigned int length, TStringEncoding encoding)
{
	if (length == 0)
		length = (unsigned int)strlen(str);
	if ((unsigned int)encoding > kLUTF8Encoding)
		return false;
	CFStringRef s = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)str, length, kEncodings[encoding],
	                                        false);
	if (s == 0)
		return false;
	setValue(s);
	CFRelease(s);
	return true;
}

void LString::setValue(const UniChar *chars, unsigned int length)
{
	if (length == 0)
		while (chars[length] != 0)
			length++;
	memcpy(_setLength(length), chars, length * 2);
	_resetCaches();
}

void LString::setValue(const LString *str)
{
	setValue(str ? (CFStringRef)str->_cf : (CFStringRef)0);
}

void LString::setValue(int value)
{
	char buf[64];
	snprintf(buf, sizeof buf, "%d", value);
	setValue(buf, 0, kLMacRomanEncoding);
}

void LString::setValue(float value)
{
	char buf[64];
	snprintf(buf, sizeof buf, "%f", (double)value);
	setValue(buf, 0, kLMacRomanEncoding);
}

LString *LString::initWithCString(const char *str, unsigned int length, TStringEncoding encoding)
{
	if (!setValue(str, length, encoding)) {
		dealloc();
		return 0;
	}
	return this;
}

LString *LString::initFromTable(CFStringRef key, CFStringRef table, CFBundleRef bundle)
{
	if (bundle == 0)
		bundle = CFBundleGetMainBundle();
	CFStringRef s = CFBundleCopyLocalizedString(bundle, key, key, table);
	setValue(s);
	CFRelease(s);
	return this;
}

LString *LString::initWithIntPtr(intptr_t value)
{
	return initWithFormat("%qi", (long long)value);
}

LString *LString::initWithContentsOfFile(const LString *path, TStringEncoding encoding)
{
	LData *data = LData::dataWithContentsOfFile(path);
	if (data == 0) {
		dealloc();
		return 0;
	}
	return initWithCString((const char *)data->bytes(), data->length(), encoding);
}

LObject *LString::initWithDecoder(const LCoder *coder)
{
	LCoder *c = const_cast<LCoder *>(coder);
	StReadChunk chunk(c);
	if (c->nextBlockSize() != 4)
		LF_RAISE(WrongConditionException());
	uint32_t bytes;
	c->readBytes(&bytes);
	if (c->nextBlockSize() != bytes)
		LF_RAISE(WrongConditionException());
	c->readBytes(_setLength(bytes >> 1));
	return this;
}

void LString::encodeWithCoder(LCoder *coder) const
{
	StWriteChunk chunk(coder);
	CFIndex n = CFStringGetLength(_cf);
	const UniChar *p = chars();
	uint32_t bytes = (uint32_t)(n * 2);
	coder->storeBytes(&bytes, 4);
	coder->storeBytes(p, n * 2);
}

const char *LString::cString() const
{
	if (_cStringCache == 0) {
		CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(_cf) + 1, kCFStringEncodingMacRoman);
		_cStringCache = new char[size];
		CFStringGetCString(_cf, _cStringCache, size, kCFStringEncodingMacRoman);
	}
	return _cStringCache;
}

const char *LString::UTF8String() const
{
	if (_utf8Cache == 0) {
		CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(_cf) + 1, kCFStringEncodingUTF8);
		_utf8Cache = new char[size];
		CFStringGetCString(_cf, _utf8Cache, size, kCFStringEncodingUTF8);
	}
	return _utf8Cache;
}

// Leading white space is skipped; "YES" and "TRUE" (any case) are 1;
// "0x" starts a hex number.
int LString::intValue() const
{
	const UniChar *p = chars();
	unsigned int n = length();
	unsigned int i = 0;
	for (; i < n; i++, p++)
		if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
			break;
	if (i == n)
		return 0;
	unsigned int left = n - i;
	int sign = 1;
	UniChar c = *p;
	if (!iswdigit(c)) {
		if (c != '-') {
			if (CFStringCompare(_cf, sYesStr._cf, kCFCompareCaseInsensitive) == 0 ||
			    CFStringCompare(_cf, sTrueStr._cf, kCFCompareCaseInsensitive) == 0)
				return 1;
			return 0;
		}
		p++;
		if (--left == 0)
			return 0;
		sign = -1;
	}
	if (p[0] == '0' && left > 1 && p[1] == 'x')
		return _hexIntScan(p + 2, left - 2) * sign;
	int v = 0;
	for (unsigned int k = 0; k < left; k++) {
		if (!iswdigit(p[k]))
			break;
		v = (p[k] - '0') + v * 10;
	}
	return sign * v;
}

bool LString::boolValue() const
{
	return intValue() != 0;
}

float LString::floatValue() const
{
	return (float)CFStringGetDoubleValue(_cf);
}

double LString::doubleValue() const
{
	return CFStringGetDoubleValue(_cf);
}

LObject *LString::propertyList()
{
	return LXMLTree::objectFromXMLData(cString(), length(), 0);
}

int LString::_hexIntScan(const UniChar *chars, unsigned int length)
{
	int v = 0;
	for (unsigned int i = 0; i < length; i++) {
		UniChar c = chars[i];
		if (iswdigit(c))
			v = (c - '0') + v * 16;
		else if ((UniChar)(c - 'A') < 6)
			v = (c - 'A' + 10) + v * 16;
		else if ((UniChar)(c - 'a') < 6)
			v = (c - 'a' + 10) + v * 16;
		else
			break;
	}
	return v;
}

int LString::_decimalIntScan(const UniChar *chars, unsigned int length)
{
	int v = 0;
	for (unsigned int i = 0; i < length && iswdigit(chars[i]); i++)
		v = (chars[i] - '0') + v * 10;
	return v;
}

// Accepts any decimal digit, as the original did.
int LString::_octalIntScan(const UniChar *chars, unsigned int length)
{
	int v = 0;
	for (unsigned int i = 0; i < length && iswdigit(chars[i]); i++)
		v = (chars[i] - '0') + v * 8;
	return v;
}

// ---------------------------------------------------------------------------
// Comparison

bool LString::isAbsolutePath() const
{
	return CFStringHasPrefix(_cf, CFSTR("/")) || CFStringHasPrefix(_cf, CFSTR("\\"));
}

CFComparisonResult LString::compareIgnoringCase(const LString *other) const
{
	return CFStringCompare(_cf, other->_cf, kCFCompareCaseInsensitive);
}

bool LString::hasPrefix(const LString *prefix) const
{
	return CFStringHasPrefix(_cf, prefix->_cf);
}

bool LString::hasSuffix(const LString *suffix) const
{
	return CFStringHasSuffix(_cf, suffix->_cf);
}

unsigned int LString::_calcHashValue() const
{
	return (unsigned int)CFHash(_cf);
}

bool LString::_isEqualTo(const LComparable *other) const
{
	if (other && other->getClass()->isSubclassOf(classObject()))
		return CFEqual(_cf, ((const LString *)other)->_cf);
	return false;
}

// Strings order before nothing; against an LValue the value's ordering is
// reversed; other objects compare equal.
int LString::compareWith(const LComparable *other) const
{
	if (other == 0)
		return 1;
	if (other->getClass()->isSubclassOf(classObject()))
		return (int)CFStringCompare(_cf, ((const LString *)other)->_cf, 0);
	if (other->getClass()->isSubclassOf(LF_CLASSOF(LValue)))
		return -other->compareWith(this);
	return 0;
}

unsigned int LString::findString(const LString *str, unsigned int from, int direction) const
{
	CFRange range;
	CFStringCompareFlags flags;
	if (direction < 1) {
		range = CFRangeMake(0, from + 1);
		flags = kCFCompareBackwards;
	} else {
		range = CFRangeMake(from, CFStringGetLength(_cf) - from);
		flags = 0;
	}
	CFRange found;
	if (CFStringFindWithOptions(_cf, str->_cf, range, flags, &found))
		return (unsigned int)found.location;
	return (unsigned int)-1;
}

unsigned int LString::findFirstCharFromSet(const LCharSet *set, unsigned int from, int direction) const
{
	unsigned int n = length();
	if (from == (unsigned int)-1 || n == 0 || from >= n)
		return (unsigned int)-1;
	const UniChar *p = chars() + from;
	if (direction < 1) {
		for (; from != (unsigned int)-1; from--, p--)
			if (set->characterIsMember(*p))
				return from;
	} else {
		for (; from != n; from++, p++)
			if (set->characterIsMember(*p))
				return from;
	}
	return (unsigned int)-1;
}

unsigned int LString::findFirstCharNotFromSet(const LCharSet *set, unsigned int from, int direction) const
{
	unsigned int n = length();
	if (from == (unsigned int)-1 || n == 0 || from >= n)
		return (unsigned int)-1;
	const UniChar *p = chars() + from;
	if (direction < 1) {
		for (; from != (unsigned int)-1; from--, p--)
			if (!set->characterIsMember(*p))
				return from;
	} else {
		for (; from != n; from++, p++)
			if (!set->characterIsMember(*p))
				return from;
	}
	return (unsigned int)-1;
}

// ---------------------------------------------------------------------------
// Editing

void LString::appendString(const LString *str)
{
	unsigned int n = length(), m = str->length();
	UniChar *buf = _setLength(n + m);
	memcpy(buf + n, str->chars(), m * 2);
	_resetCaches();
}

void LString::eraseSubstringInRange(unsigned int location, unsigned int len)
{
	if (len == 0)
		return;
	UniChar *buf = (UniChar *)chars();
	unsigned int n = length();
	memmove(buf + location, buf + location + len, (n - location - len) * 2);
	_setLength(n - len);
	_resetCaches();
}

void LString::replaceCharactersInRange(unsigned int location, unsigned int len, const LString *str)
{
	int delta = (int)str->length() - (int)len;
	if (delta != 0) {
		unsigned int n = length();
		UniChar *buf = delta < 1 ? (UniChar *)chars() : _setLength(n + delta);
		memmove(buf + location + len + delta, buf + location + len, (n - location - len) * 2);
		if (delta < 0)
			_setLength(length() + delta);
	}
	memcpy((UniChar *)chars() + location, str->chars(), str->length() * 2);
	_resetCaches();
}

int LString::replaceOccurrencesOfString(const LString *target, const LString *replacement)
{
	int count = 0;
	unsigned int tlen = target->length(), rlen = replacement->length();
	unsigned int at = 0;
	while ((at = findString(target, at, 1)) != (unsigned int)-1) {
		count++;
		replaceCharactersInRange(at, tlen, replacement);
		at += rlen;
	}
	return count;
}

void LString::trimWithCharacters(const LCharSet *set)
{
	unsigned int first = findFirstCharNotFromSet(set, 0, 1);
	if (first - 1 < 0xfffffffe)
		eraseSubstringInRange(0, first);
	unsigned int last = findFirstCharNotFromSet(set, length() - 1, -1);
	if (last - 1 < 0xfffffffe)
		eraseSubstringInRange(last + 1, length() - last - 1);
}

static inline bool _isSeparator(UniChar c)
{
	return c == '\\' || c == '/';
}

// Removes from the last separator on (a trailing one is skipped first);
// with no separator the string becomes empty.
void LString::deleteLastPathComponent()
{
	const UniChar *buf = chars();
	unsigned int n = length();
	unsigned int i = n - 1;
	while (i != (unsigned int)-1 && !_isSeparator(buf[i]))
		i--;
	if (i == n - 1) {
		if (i != (unsigned int)-1) {
			for (i = n - 2; i != (unsigned int)-1; i--)
				if (_isSeparator(buf[i]))
					goto found;
		}
		i = 0;
	} else if (i == (unsigned int)-1) {
		i = 0;
	}
found:
	_setLength(i);
	_resetCaches();
}

void LString::appendPathComponent(const LString *component)
{
	unsigned int n = length();
	if (n != 0) {
		UniChar last = chars()[n - 1];
		if (!_isSeparator(last))
			_setLength(n + 1)[n] = '/';
	}
	appendString(component);
}

// Removes the extension when its dot is after the last separator (a
// trailing separator is ignored) and isn't the first character.
void LString::deletePathExtension()
{
	const UniChar *buf = chars();
	unsigned int n = length();
	unsigned int dot = n - 1;
	for (;; dot--) {
		if (dot == (unsigned int)-1)
			return;
		if (buf[dot] == '.')
			break;
	}
	unsigned int sep = _isSeparator(buf[n - 1]) ? n - 2 : n - 1;
	while (sep != (unsigned int)-1 && !_isSeparator(buf[sep]))
		sep--;
	if (dot != 0 && (sep == (unsigned int)-1 || sep < dot)) {
		_setLength(dot);
		_resetCaches();
	}
}

void LString::appendPathExtension(const LString *ext)
{
	unsigned int n = length();
	_setLength(n + 1)[n] = '.';
	appendString(ext);
}

void LString::appendFormat(const char *format, ...)
{
	LConstantString fmt(format);
	LString *s = _alloc();
	va_list ap;
	va_start(ap, format);
	s->_initWithFormat(&fmt, ap);
	va_end(ap);
	appendString(s);
	s->release();
}

bool LString::writeToFile(const LString *path)
{
	const char *utf8 = UTF8String();
	size_t n = strlen(utf8);
	LData *data = LData::dataWithLength((unsigned int)n);
	memcpy(data->bytes(), utf8, n);
	return data->writeToFile(path);
}

// ---------------------------------------------------------------------------
// Derived strings

LString *LString::substringWithRange(unsigned int location, unsigned int len) const
{
	if (len == 0)
		return string();
	return stringWithCharacters(chars() + location, len);
}

LString *LString::substringFromIndex(unsigned int index) const
{
	return substringWithRange(index, length() - index);
}

LString *LString::capitalizedString() const
{
	LString *s = stringWithString(this);
	CFStringCapitalize(s->_cf, 0);
	_rebuffer(s);
	return s;
}

LString *LString::lowercaseString() const
{
	LString *s = stringWithString(this);
	CFStringLowercase(s->_cf, 0);
	_rebuffer(s);
	return s;
}

LString *LString::uppercaseString() const
{
	LString *s = stringWithString(this);
	CFStringUppercase(s->_cf, 0);
	_rebuffer(s);
	return s;
}

LString *LString::stringByDeletingPathExtension() const
{
	LString *s = stringWithString(this);
	if (s)
		s->deletePathExtension();
	return s;
}

LString *LString::stringByDeletingLastPathComponent() const
{
	LString *s = stringWithString(this);
	if (s)
		s->deleteLastPathComponent();
	return s;
}

LString *LString::stringByAppendingPathExtension(const LString *ext) const
{
	LString *s = stringWithString(this);
	if (s)
		s->appendPathExtension(ext);
	return s;
}

LString *LString::stringByAppendingPathComponent(const LString *component) const
{
	LString *s = stringWithString(this);
	if (s)
		s->appendPathComponent(component);
	return s;
}

LString *LString::stringByAppendingString(const LString *str) const
{
	LString *s = stringWithString(this);
	s->appendString(str);
	return s;
}

LString *LString::pathExtension() const
{
	const UniChar *buf = chars();
	unsigned int n = length();
	unsigned int dot = n - 1;
	for (;; dot--) {
		if (dot == (unsigned int)-1)
			return string();
		if (buf[dot] == '.')
			break;
	}
	bool trailing = _isSeparator(buf[n - 1]);
	unsigned int sep = trailing ? n - 2 : n - 1;
	while (sep != (unsigned int)-1 && !_isSeparator(buf[sep]))
		sep--;
	if (sep == (unsigned int)-1 || sep < dot)
		return substringWithRange(dot + 1, n - dot - (trailing ? 1 : 0) - 1);
	return string();
}

LString *LString::lastPathComponent() const
{
	const UniChar *buf = chars();
	int n = (int)length();
	int i = n - 1;
	while (i != -1 && !_isSeparator(buf[i]))
		i--;
	int trailing = 0;
	if (i == n - 1) {
		if (i == -1)
			return stringWithString(this);
		trailing = 1;
		for (i = n - 2; i != -1; i--)
			if (_isSeparator(buf[i]))
				break;
	}
	if (i != -1)
		return substringWithRange(i + 1, n - i - trailing - 1);
	if (trailing == 0)
		return stringWithString(this);
	return substringWithRange(0, n - trailing);
}

static LArray *_componentsSeparatedBy(const LString *self, const LString *separator, const LCharSet *trim)
{
	LArray *result = LArray::array();
	unsigned int start = 0;
	unsigned int seplen = separator->length();
	for (;;) {
		unsigned int end = self->findString(separator, start, 1);
		bool last = end == (unsigned int)-1;
		if (last)
			end = self->length();
		unsigned int cut = 0;
		if (trim) {
			unsigned int i = self->findFirstCharNotFromSet(trim, start, 1);
			if (i < end && i != (unsigned int)-1)
				start = i;
			i = self->findFirstCharNotFromSet(trim, end - 1, -1);
			if (!(i < start || i == (unsigned int)-1))
				cut = end - i - 1;
		}
		result->addObject(self->substringWithRange(start, end - start - cut));
		start = end + seplen;
		if (last)
			return result;
	}
}

LArray *LString::componentsSeparatedByString(const LString *separator, const LCharSet *trim) const
{
	return _componentsSeparatedBy(this, separator, trim);
}

// Splits on the line break style the text uses: CRLF, CR or LF.
LArray *LString::componentsSeparatedByLineBreaks(const LCharSet *trim) const
{
	int style = UTextUtils::lineBreakStyleForBytes(chars(), length());
	const char *sep = style == 0 ? "\r\n" : (style == 2 ? "\r" : "\n");
	return _componentsSeparatedBy(this, stringWithCString(sep, 0, kLMacRomanEncoding), trim);
}

// ---------------------------------------------------------------------------
// Descriptions

LString *LString::copy() const
{
	LString *s = _alloc();
	s->setValue(this);
	return s;
}

LString *LString::description() const
{
	return stringWithString(this);
}

// Quoted unless every character is a letter (from '@' to DEL), digit or '_'.
LString *LString::fillTextRepresentation(unsigned int) const
{
	unsigned int n = length();
	const UniChar *p = chars();
	for (unsigned int i = 0; i < n; i++) {
		UniChar c = p[i];
		if ((UniChar)(c - 0x40) > 0x3f && !iswdigit(c) && c != '_') {
			LString *s = stringWithCString("\"", 0, kLMacRomanEncoding);
			s->appendString(this);
			s->appendString(&sOpenCloseSym);
			return s;
		}
	}
	return stringWithString(this);
}

LString *LString::internalDescription() const
{
	char buf[312];
	snprintf(buf, sizeof buf, "LString(%p)\"", this);
	LString *s = stringWithCString(buf, 0, kLMacRomanEncoding);
	s->appendString(this);
	s->appendString(&sOpenCloseSym);
	return s;
}

LString *LString::_plistRepresentation(unsigned int) const
{
	LString *s = stringWithString(this);
	s->replaceOccurrencesOfString(&sAmpersandSym, &sAmpersandSymXML);
	s->replaceOccurrencesOfString(&sGreaterThanSym, &sGreaterThanSymXML);
	s->replaceOccurrencesOfString(&sLessThanSym, &sLessThanSymXML);
	return stringWithFormat("<string>%@</string>", s);
}

// ---------------------------------------------------------------------------
// Factories

LString *LString::string()
{
	LString *s = _alloc();
	if (s && s->init())
		return LAutorelease(s);
	return 0;
}

LString *LString::emptyString()
{
	return &sEmptyStr;
}

LString *LString::alloc_with_new()
{
	return new LString;
}

LString *LString::stringWithString(const LString *str)
{
	LString *s = _alloc();
	s->setValue(str);
	return LAutorelease(s);
}

LString *LString::stringWithCFString(CFStringRef str)
{
	LString *s = _alloc();
	s->setValue(str);
	return LAutorelease(s);
}

LString *LString::stringWithCharacters(const UniChar *chars, unsigned int length)
{
	LString *s = _alloc();
	s->setValue(chars, length);
	return LAutorelease(s);
}

LString *LString::stringWithInt(int value)
{
	LString *s = _alloc();
	s->setValue(value);
	return LAutorelease(s);
}

LString *LString::stringWithCString(const char *str, unsigned int length, TStringEncoding encoding)
{
	LString *s = _alloc()->initWithCString(str, length, encoding);
	return s ? LAutorelease(s) : 0;
}

LString *LString::stringWithIntPtr(intptr_t value)
{
	LString *s = _alloc()->initWithIntPtr(value);
	return s ? LAutorelease(s) : 0;
}

LString *LString::stringFromSize(LSize size)
{
	return stringWithFormat("{%G, %G}", (double)size.width, (double)size.height);
}

LString *LString::stringFromPoint(LPoint point)
{
	return stringWithFormat("{%G, %G}", (double)point.x, (double)point.y);
}

LSize LString::sizeFromString(LString *str)
{
	LSize size = LZeroSize;
	if (str)
		sscanf(str->cString(), "{%G, %G}", &size.width, &size.height);
	return size;
}

LPoint LString::pointFromString(LString *str)
{
	LPoint point = LZeroPoint;
	sscanf(str->cString(), "{%G, %G}", &point.x, &point.y);
	return point;
}

// 21 characters from a new UUID's 128 bits, six bits each.
LString *LString::stringAsUID()
{
	static const char kDigits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_#";
	CFUUIDRef uuid = CFUUIDCreate(0);
	CFUUIDBytes b = CFUUIDGetUUIDBytes(uuid);
	CFRelease(uuid);
	const UInt8 *u = &b.byte0;
	uint32_t w0 = u[0] | u[1] << 8 | u[2] << 16 | (uint32_t)u[3] << 24;
	uint32_t w1 = u[4] | u[5] << 8 | u[6] << 16 | (uint32_t)u[7] << 24;
	uint32_t w2 = u[8] | u[9] << 8 | u[10] << 16 | (uint32_t)u[11] << 24;
	uint32_t w3 = u[12] | u[13] << 8 | u[14] << 16 | (uint32_t)u[15] << 24;
	uint32_t hi0 = __builtin_bswap32(w0);
	uint32_t lo0 = (w1 >> 16) << 24 | (w1 >> 24) << 16 | (w1 & 0xff) << 8 | (w1 >> 8 & 0xff);
	uint64_t first = (uint64_t)hi0 << 32 | lo0;
	uint32_t a = (w2 >> 16) << 24 | (w2 >> 24) << 16 | (w3 & 0xff) << 8 | (w3 >> 8 & 0xff);
	uint32_t c = (w3 >> 16) << 24 | (w3 >> 24) << 16 | (w2 & 0xff) << 8 | (w2 >> 8 & 0xff);
	uint64_t second = (uint64_t)c << 32 | a;
	char out[22];
	int k = 0;
	for (int shift = 0x3a; shift != -2; shift -= 6)
		out[k++] = kDigits[(second >> shift) & 0x3f];
	out[k++] = kDigits[(w3 >> 8 & 0xf) << 2 | hi0 >> 30];
	for (int shift = 0x38; shift != -4; shift -= 6)
		out[k++] = kDigits[(first >> shift) & 0x3f];
	out[21] = 0;
	return stringWithCString(out, 21, kLMacRomanEncoding);
}

LString *LString::pathForResource(LString *name, LString *type, void *bundle)
{
	if (name == 0)
		return 0;
	if (bundle == 0)
		bundle = CFBundleGetMainBundle();
	CFURLRef url = CFBundleCopyResourceURL((CFBundleRef)bundle, name->_cf, type ? type->_cf : 0, 0);
	LString *path = 0;
	if (url) {
		CFStringRef s = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
		if (s) {
			path = stringWithCFString(s);
			CFRelease(s);
		}
		CFRelease(url);
	}
	LFileManager *fm = LFileManager::manager();
	if (fm && path && fm->isReadableFileAtPath(path))
		return path;
	return 0;
}

LString *LString::stringFromTable(LString *key, const char *table, void *bundle)
{
	CFStringRef t = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)table, strlen(table),
	                                        kCFStringEncodingMacRoman, false);
	if (bundle == 0)
		bundle = CFBundleGetMainBundle();
	CFStringRef s = CFBundleCopyLocalizedString((CFBundleRef)bundle, key->_cf, 0, t);
	CFRelease(t);
	if (s) {
		key = stringWithCFString(s);
		CFRelease(s);
	}
	return key;
}

LString *LString::stringFromTable(const char *key, const char *table, void *bundle)
{
	LConstantString k(key);
	return stringFromTable(&k, table, bundle);
}

// ---------------------------------------------------------------------------
// Formatting

namespace {

void _appendCF(CFMutableStringRef out, CFStringRef s)
{
	if (s)
		CFStringAppend(out, s);
}

void _appendASCII(CFMutableStringRef out, const char *s)
{
	CFStringAppendCString(out, s, kCFStringEncodingMacRoman);
}

} // namespace

LString *LString::_initWithFormat(const LString *format, va_list args)
{
	_setLength(0);
	va_list ap;
	va_copy(ap, args);
	CFMutableStringRef out = CFStringCreateMutable(kCFAllocatorDefault, 0);
	const UniChar *f = format->chars();
	unsigned int n = format->length();
	unsigned int i = 0, start = 0;
	while (i < n) {
		if (f[i] != '%') {
			i++;
			continue;
		}
		if (i > start)
			CFStringAppendCharacters(out, f + start, i - start);
		unsigned int specStart = i++;
		if (i < n && f[i] == '%') {
			CFStringAppendCharacters(out, f + i, 1);
			start = ++i;
			continue;
		}
		// Flags, width, precision; then length modifiers and the conversion.
		char spec[32];
		unsigned int k = 0;
		spec[k++] = '%';
		while (i < n && f[i] < '@' && k < 24) {
			if (f[i] == '*') {
				k += snprintf(spec + k, sizeof spec - k, "%d", va_arg(ap, int));
				i++;
			} else {
				spec[k++] = (char)f[i++];
			}
		}
		int longs = 0;
		bool shortMod = false, sizeMod = false;
		while (i < n) {
			UniChar c = f[i];
			if (c == 'l')
				longs++;
			else if (c == 'q')
				longs = 2;
			else if (c == 'h')
				shortMod = true;
			else if (c == 'z' || c == 't' || c == 'j')
				sizeMod = true;
			else
				break;
			i++;
		}
		if (i >= n) {
			start = specStart;
			break;
		}
		UniChar conv = f[i++];
		start = i;
		char buf[512];
		switch (conv) {
		case '@': {
			LObject *o = va_arg(ap, LObject *);
			const LString *s = &sNullString;
			if (o)
				s = o->getClass() && o->getClass()->isSubclassOf(classObject()) ? (const LString *)o
				                                                                    : o->description();
			if (s)
				_appendCF(out, s->_cf);
			break;
		}
		case 's': {
			const char *s = va_arg(ap, const char *);
			if (k == 1) {
				LString *str = s ? stringWithCString(s, 0, kLMacRomanEncoding) : 0;
				_appendCF(out, (str ? str : &sNullString)->_cf);
			} else {
				spec[k++] = 's';
				spec[k] = 0;
				snprintf(buf, sizeof buf, spec, s);
				_appendASCII(out, buf);
			}
			break;
		}
		case 'S': {
			const UniChar *s = va_arg(ap, const UniChar *);
			if (s) {
				unsigned int len = 0;
				while (s[len])
					len++;
				CFStringAppendCharacters(out, s, len);
			}
			break;
		}
		case 'C': {
			UniChar c = (UniChar)va_arg(ap, int);
			CFStringAppendCharacters(out, &c, 1);
			break;
		}
		case 'e': case 'E': case 'f': case 'F': case 'g': case 'G': case 'a': case 'A': {
			spec[k++] = (char)conv;
			spec[k] = 0;
			snprintf(buf, sizeof buf, spec, va_arg(ap, double));
			_appendASCII(out, buf);
			break;
		}
		case 'p': {
			spec[k++] = 'p';
			spec[k] = 0;
			snprintf(buf, sizeof buf, spec, va_arg(ap, void *));
			_appendASCII(out, buf);
			break;
		}
		case 'd': case 'i': case 'u': case 'x': case 'X': case 'o': case 'c': {
			if (longs >= 2) {
				spec[k++] = 'l';
				spec[k++] = 'l';
				spec[k++] = (char)conv;
				spec[k] = 0;
				snprintf(buf, sizeof buf, spec, va_arg(ap, long long));
			} else if (longs == 1 || sizeMod) {
				spec[k++] = 'l';
				spec[k++] = (char)conv;
				spec[k] = 0;
				snprintf(buf, sizeof buf, spec, va_arg(ap, long));
			} else {
				if (shortMod)
					spec[k++] = 'h';
				spec[k++] = (char)conv;
				spec[k] = 0;
				snprintf(buf, sizeof buf, spec, va_arg(ap, int));
			}
			_appendASCII(out, buf);
			break;
		}
		default:
			// Not a conversion we know: copy it as text.
			CFStringAppendCharacters(out, f + specStart, i - specStart);
			break;
		}
	}
	if (n > start)
		CFStringAppendCharacters(out, f + start, n - start);
	va_end(ap);
	setValue((CFStringRef)out);
	CFRelease(out);
	return this;
}

LString *LString::initWithFormat(const LString *format, ...)
{
	va_list ap;
	va_start(ap, format);
	LString *s = _initWithFormat(format, ap);
	va_end(ap);
	return s;
}

LString *LString::initWithFormat(const char *format, ...)
{
	LConstantString fmt(format);
	va_list ap;
	va_start(ap, format);
	LString *s = _initWithFormat(&fmt, ap);
	va_end(ap);
	return s;
}

LString *LString::stringWithFormat(const LString *format, ...)
{
	LString *s = _alloc();
	va_list ap;
	va_start(ap, format);
	LString *r = s->_initWithFormat(format, ap);
	va_end(ap);
	return r ? LAutorelease(r) : 0;
}

LString *LString::stringWithFormat(const char *format, ...)
{
	LConstantString fmt(format);
	LString *s = _alloc();
	va_list ap;
	va_start(ap, format);
	LString *r = s->_initWithFormat(&fmt, ap);
	va_end(ap);
	return r ? LAutorelease(r) : 0;
}

LString *LString::vStringWithFormat(const char *format, va_list args)
{
	LConstantString fmt(format);
	LString *r = _alloc()->_initWithFormat(&fmt, args);
	return r ? LAutorelease(r) : 0;
}

} // namespace LF
