// LString: a mutable UTF-16 string kept in a CFMutableString with external
// characters, so chars() is the live buffer. LConstantString is one made
// from a C string at static-initialisation time that is never deallocated.
#pragma once

#include "LObject.h"
#include <CoreFoundation/CoreFoundation.h>
#include <stdarg.h>

namespace LF {

class LCharSet;

enum TStringEncoding {
	kLMacRomanEncoding = 0,	// what cString() returns
	kLUTF8Encoding = 1,
};

struct LPoint {
	float x, y;
};
struct LSize {
	float width, height;
};
struct LRect {
	LPoint origin;
	LSize size;
};
extern const LPoint LZeroPoint;
extern const LSize LZeroSize;
extern const LRect LZeroRect;
// 10000 tabs, for indenting text representations.
extern char _l_tabs[10000];
static inline LPoint LMakePoint(float x, float y) { LPoint p = { x, y }; return p; }
static inline LSize LMakeSize(float w, float h) { LSize s = { w, h }; return s; }
static inline LRect LMakeRect(float x, float y, float w, float h) { LRect r = { { x, y }, { w, h } }; return r; }
// The common part of two rects, or LZeroRect.
static inline LRect LIntersectionRect(LRect a, LRect b)
{
	float left = a.origin.x <= b.origin.x ? b.origin.x : a.origin.x;
	float right = b.origin.x + b.size.width <= a.origin.x + a.size.width ? b.origin.x + b.size.width : a.origin.x + a.size.width;
	float bottom = a.origin.y <= b.origin.y ? b.origin.y : a.origin.y;
	float top = b.origin.y + b.size.height <= a.origin.y + a.size.height ? b.origin.y + b.size.height : a.origin.y + a.size.height;
	if (right < left || top < bottom)
		return LZeroRect;
	return LMakeRect(left, bottom, right - left, top - bottom);
}
static inline bool LEqualRects(LRect a, LRect b)
{
	return a.origin.x == b.origin.x && a.origin.y == b.origin.y && a.size.width == b.size.width &&
	       a.size.height == b.size.height;
}
static inline bool LIntersectsRect(LRect a, LRect b)
{
	return !LEqualRects(LIntersectionRect(a, b), LZeroRect);
}
// The bounds of both; a rect of no size (both sides 0) doesn't count.
static inline LRect LUnionRect(LRect a, LRect b)
{
	bool aEmpty = a.size.width == 0 && a.size.height == 0;
	bool bEmpty = b.size.width == 0 && b.size.height == 0;
	if (aEmpty && bEmpty)
		return LZeroRect;
	if (aEmpty && b.size.width != 0 && b.size.height != 0)
		return b;
	if (bEmpty && a.size.width != 0 && a.size.height != 0)
		return a;
	float left = b.origin.x <= a.origin.x ? b.origin.x : a.origin.x;
	float bottom = b.origin.y <= a.origin.y ? b.origin.y : a.origin.y;
	float right = a.origin.x + a.size.width <= b.origin.x + b.size.width ? b.origin.x + b.size.width
	                                                                       : a.origin.x + a.size.width;
	float top = a.origin.y + a.size.height <= b.origin.y + b.size.height ? b.origin.y + b.size.height
	                                                                       : a.origin.y + a.size.height;
	return LMakeRect(left, bottom, right - left, top - bottom);
}

class LString : public LComparable {
	LF_DECLARE_CLASS(LString)
public:
	LString() : _cf(0), _cStringCache(0), _utf8Cache(0) {}
	virtual ~LString() {}
	virtual LString *init();
	virtual LString *copy() const;
	virtual LObject *initWithDecoder(const LCoder *coder);
	virtual void encodeWithCoder(LCoder *coder) const;
	virtual LString *description() const;
	virtual LString *internalDescription() const;
	virtual LString *fillTextRepresentation(unsigned int indent) const;
	virtual void dealloc();
	virtual LString *_plistRepresentation(unsigned int indent) const;
	virtual int compareWith(const LComparable *other) const;
	virtual unsigned int _calcHashValue() const;
	virtual bool _isEqualTo(const LComparable *other) const;
	virtual LString *getOwnerLObject() { return this; }

	// Factories (autoreleased)
	static LString *string();
	static LString *emptyString();
	static LString *stringWithString(const LString *str);
	static LString *stringWithCFString(CFStringRef str);
	static LString *stringWithCharacters(const UniChar *chars, unsigned int length);
	static LString *stringWithCString(const char *str, unsigned int length = 0,
	                                  TStringEncoding encoding = kLMacRomanEncoding);
	static LString *stringWithInt(int value);
	static LString *stringWithIntPtr(intptr_t value);
	static LString *stringWithFormat(const char *format, ...);
	static LString *stringWithFormat(const LString *format, ...);
	static LString *vStringWithFormat(const char *format, va_list args);
	static LString *stringFromSize(LSize size);
	static LString *stringFromPoint(LPoint point);
	static LSize sizeFromString(LString *str);
	static LPoint pointFromString(LString *str);
	static LString *stringAsUID();
	static LString *pathForResource(LString *name, LString *type, void *bundle = 0);
	static LString *stringFromTable(LString *key, const char *table, void *bundle = 0);
	static LString *stringFromTable(const char *key, const char *table, void *bundle = 0);
	static LString *alloc_with_new();

	// Initialisers
	LString *initWithCString(const char *str, unsigned int length = 0,
	                         TStringEncoding encoding = kLMacRomanEncoding);
	LString *initWithContentsOfFile(const LString *path, TStringEncoding encoding = kLMacRomanEncoding);
	static LString *stringWithContentsOfFile(const LString *path, TStringEncoding encoding = kLMacRomanEncoding)
	{
		LString *s = _alloc();
		if (s && s->initWithContentsOfFile(path, encoding))
			return LAutorelease(s);
		return 0;
	}
	LString *initWithFormat(const char *format, ...);
	LString *initWithFormat(const LString *format, ...);
	LString *_initWithFormat(const LString *format, va_list args);
	LString *initWithIntPtr(intptr_t value);	// "%qi"
	LString *initFromTable(CFStringRef key, CFStringRef table, CFBundleRef bundle);

	// Value
	void setValue(CFStringRef str);
	bool setValue(const char *str, unsigned int length, TStringEncoding encoding);
	void setValue(const UniChar *chars, unsigned int length);
	void setValue(const LString *str);
	void setValue(int value);
	void setValue(float value);

	unsigned int length() const { return (unsigned int)CFStringGetLength(_cf); }
	CFStringRef cfString() const { return _cf; }
	const UniChar *chars() const;
	const UniChar *wString() const { return chars(); }
	const char *cString() const;
	const char *UTF8String() const;

	int intValue() const;
	bool boolValue() const;
	float floatValue() const;
	double doubleValue() const;
	LObject *propertyList();

	// Comparison
	CFComparisonResult compareIgnoringCase(const LString *other) const;
	bool hasPrefix(const LString *prefix) const;
	bool hasSuffix(const LString *suffix) const;
	bool isAbsolutePath() const;
	// Index of `str`, searching forward from `from` (direction > 0) or
	// backward from it; -1 when not found.
	unsigned int findString(const LString *str, unsigned int from, int direction) const;
	unsigned int findFirstCharFromSet(const LCharSet *set, unsigned int from, int direction) const;
	unsigned int findFirstCharNotFromSet(const LCharSet *set, unsigned int from, int direction) const;

	// Editing
	void appendString(const LString *str);
	void appendFormat(const char *format, ...);
	void eraseSubstringInRange(unsigned int location, unsigned int length);
	void replaceCharactersInRange(unsigned int location, unsigned int length, const LString *str);
	int replaceOccurrencesOfString(const LString *target, const LString *replacement);
	void trimWithCharacters(const LCharSet *set);
	void deleteLastPathComponent();
	void appendPathComponent(const LString *component);
	void deletePathExtension();
	void appendPathExtension(const LString *ext);

	// Derived strings (autoreleased)
	LString *substringWithRange(unsigned int location, unsigned int length) const;
	LString *substringFromIndex(unsigned int index) const;
	LString *capitalizedString() const;
	LString *lowercaseString() const;
	LString *uppercaseString() const;
	LString *stringByDeletingPathExtension() const;
	LString *stringByDeletingLastPathComponent() const;
	LString *stringByAppendingPathExtension(const LString *ext) const;
	LString *stringByAppendingPathComponent(const LString *component) const;
	LString *stringByAppendingString(const LString *str) const;
	LString *pathExtension() const;
	LString *lastPathComponent() const;
	LArray *componentsSeparatedByString(const LString *separator, const LCharSet *trim = 0) const;
	LArray *componentsSeparatedByLineBreaks(const LCharSet *trim = 0) const;

	bool writeToFile(const LString *path);

	static int _hexIntScan(const UniChar *chars, unsigned int length);
	static int _decimalIntScan(const UniChar *chars, unsigned int length);
	static int _octalIntScan(const UniChar *chars, unsigned int length);

	UniChar *_setLength(unsigned int length);
	void _resetCaches();

	CFMutableStringRef _cf;
	mutable char *_cStringCache;
	mutable char *_utf8Cache;
};

class LConstantString : public LString {
public:
	LConstantString() {}
	LConstantString(const char *str) { initWithCString(str, 0, kLMacRomanEncoding); }
	virtual ~LConstantString();
	virtual void dealloc() {}
};

// A set of UTF-16 code units, as a bitmap.
class LCharSet : public LObject {
	LF_DECLARE_CLASS(LCharSet)
public:
	// 0-11 are bitmaps of the Core Foundation sets of the same names,
	// stored in LFoundation's LCharSet.bitmaps resource.
	enum TPredefinedCharSetType {
		kAlphaNumeric = 0,
		kControl,
		kDecimalDigit,
		kDecomposable,
		kIllegal,
		kLetter,
		kLowercaseLetter,
		kNonBase,
		kPunctuation,
		kUppercaseLetter,
		kWhitespaceAndNewline,
		kWhitespace,
		kOperators,	// "<>=!^&|"
		kSeparators,	// " \r\n\t{}"
	};

	LCharSet() { memset(_bits, 0, sizeof _bits); }
	virtual ~LCharSet() {}
	virtual LCharSet *copy() const;
	virtual void dealloc();
	virtual LCharSet *getOwnerLObject() { return this; }

	static LCharSet *charSetWithType(int type);
	static LCharSet *characterSetWithCharactersInString(const LString *str);
	static LCharSet *characterSetWithCharactersInCString(const char *str);

	LCharSet *initWithCharSet(const LCharSet *other);
	LCharSet *initWithCString(const char *str);
	LCharSet *initWithString(const LString *str);
	LCharSet *initWithData(LData *data);
	void formUnionWithCharacterSet(const LCharSet *other);
	LCharSet *invertedSet() const;
	void addCharactersInString(LString *str);
	void addCharactersInCString(const char *str);

	bool characterIsMember(UniChar c) const { return (_bits[c >> 3] >> (c & 7)) & 1; }
	void addCharacter(UniChar c) { _bits[c >> 3] |= (unsigned char)(1 << (c & 7)); }

	unsigned char _bits[0x2000];
};

// Per-thread stacks of objects to release.
class LAutoreleasePool {
public:
	static LAutoreleasePool *currentPool();
	static void addNewPoolToTop();
	static void removeTopPool();
	void addObject(LObject *obj);
	int releaseAllObjects();

	LAutoreleasePool *_above;
	LAutoreleasePool *_below;
	TArray<LObject *> *_objects;
};

// Creates a pool for its scope.
class StAutoreleasePool {
public:
	StAutoreleasePool() { LAutoreleasePool::addNewPoolToTop(); }
	~StAutoreleasePool() { LAutoreleasePool::removeTopPool(); }
};

// While it exists, every NSAutoreleasePool (and, in this port, every pass of
// the main run loop) has an LAutoreleasePool that is removed with it. In
// LAutoreleasePoolInstaller.mm.
class StAutoreleasePoolInstaller {
public:
	StAutoreleasePoolInstaller();
	~StAutoreleasePoolInstaller();
};

// An NSAutoreleasePool with an LAutoreleasePool (unless the installer already
// gives it one). In LAutoreleasePoolInstaller.mm.
class StNSAutoreleasePool {
public:
	StNSAutoreleasePool();
	~StNSAutoreleasePool();

	void *_nsPool;
	StAutoreleasePool *_lPool;
};

void LLog(const char *format, ...);

} // namespace LF
