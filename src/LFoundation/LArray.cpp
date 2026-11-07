// LArray
//
// LF::LArray: an ordered list of retained LObjects in a CArray of pointers,
// and _LArrayEnumerator, which walks one. The file also holds the
// licence-key helpers (SHIFER_*) of the original.

#include "LArray.h"
#include "LCoder.h"
#include "LData.h"
#include "LXMLTree.h"
#include "CLMapCreationStream.h"
#include <IOKit/IOKitLib.h>
#include <IOKit/network/IOEthernetInterface.h>

namespace LF {

LF_IMPLEMENT_CLASS(LArray, 0)
LF_IMPLEMENT_CLASS(_LArrayEnumerator, LF_CLASSOF(LEnumerator))

static LConstantString sCloseBracket(")");
static LConstantString sNewLine("\n");
static LConstantString sComma(",");
static LConstantString sEmptyArrayString("()");

static LString *_concatStringsWithEOL(LString *str, LString *add)
{
	str->appendString(add);
	str->appendString(&sNewLine);
	return add;
}

// `length` tabs.
static LString *_tabs(unsigned int length)
{
	if (length == 0)
		return LString::string();
	return LString::stringWithCString(_l_tabs, length, kLMacRomanEncoding);
}

LArray *LArray::array()
{
	LArray *a = _alloc();
	if (a && a->init())
		a = LAutorelease(a);
	return a;
}

LArray *LArray::arrayWithObjects(LObject *first, ...)
{
	LArray *a = _alloc();
	if (!a)
		return 0;
	va_list ap;
	va_start(ap, first);
	a = a->initWithObjectsList(first, ap);
	va_end(ap);
	return a ? LAutorelease(a) : 0;
}

LArray *LArray::init()
{
	return LObject::init() ? this : 0;
}

LArray *LArray::initWithCapacity(unsigned int capacity)
{
	if (init() == 0)
		return 0;
	preserveCapacity(capacity);
	return this;
}

LArray *LArray::initWithArray(const LArray *other)
{
	_copyDataFromArray(other);
	return this;
}

LArray *LArray::initWithObjectsList(LObject *first, va_list rest)
{
	if (first) {
		unsigned int n = 1;
		va_list count;
		va_copy(count, rest);
		while (va_arg(count, LObject *) != 0)
			n++;
		va_end(count);
		preserveCapacity(n);
		LObject *obj = first;
		do {
			addObject(obj);
			obj = va_arg(rest, LObject *);
		} while (obj);
	}
	return this;
}

LArray *LArray::initWithObjects(LObject *first, ...)
{
	va_list ap;
	va_start(ap, first);
	LArray *a = initWithObjectsList(first, ap);
	va_end(ap);
	return a;
}

// An XML plist (when a '<' block comes first) or an old-style "( ... )" list.
LArray *LArray::initWithTextRepresentation(const LData *data)
{
	if (data && data->length() != 0) {
		CLMapCreationStream stream(data);
		unsigned int xml = stream.GetBlockOffsetStart('<', '>');
		unsigned int list = stream.GetBlockOffsetStart('(', ')');
		if (list != (unsigned int)-1 || xml != (unsigned int)-1) {
			if (list == (unsigned int)-1 || (xml != (unsigned int)-1 && xml <= list)) {
				if (LXMLTree::objectFromXMLData((const char *)data->bytes(), data->length(), this))
					return this;
				dealloc();
				return 0;
			}
			CLMapCreationStream scan(data);
			int start = scan.GetBlockOffsetStart('(', ')');
			if (start != -1) {
				scan.SetMarker(start + 1);
				unsigned int end = scan.GetBlockOffsetEnd('(', ')');
				if (end != (unsigned int)-1) {
					scan.scanArray(end, this);
					return this;
				}
			}
		}
	}
	dealloc();
	return 0;
}

LArray *LArray::initWithContentsOfFile(const LString *path)
{
	return initWithTextRepresentation(LData::dataWithContentsOfFile(path));
}

bool LArray::writeToFile(const LString *path)
{
	LString *plist = plistRepresentation(0);
	return plist ? plist->writeToFile(path) : false;
}

bool LArray::isEqualTo(const LObject *other) const
{
	if (other && other->getClass()->isSubclassOf(classObject())) {
		const LArray *a = (const LArray *)other;
		if (_count != a->_count)
			return false;
		for (unsigned int i = 0; i < _count; i++) {
			if (!objectAtIndex(i)->isEqualTo(a->objectAtIndex(i)))
				return false;
		}
		return true;
	}
	return LObject::isEqualTo(other);
}

int LArray::indexOfObjectIdenticalTo(const LObject *obj) const
{
	for (unsigned int i = 0; i < _count; i++) {
		if (objectAtIndex(i)->isEqualTo(obj))
			return i;
	}
	return -1;
}

void LArray::dealloc()
{
	_callRemoveForAllObjects();
	LObject::dealloc();
}

LObject *LArray::initWithDecoder(const LCoder *coder)
{
	LCoder *c = const_cast<LCoder *>(coder);
	StReadChunk chunk(c);
	if (c->nextBlockSize() != 4)
		LF_RAISE(WrongConditionException());
	uint32_t n;
	c->readBytes(&n);
	preserveCapacity(n);
	for (unsigned int i = 0; i < n; i++) {
		LObject *obj = c->decodeObject();
		if (obj == 0)
			LF_RAISE(NilPtrException());
		addObject(obj);
	}
	return this;
}

void LArray::encodeWithCoder(LCoder *coder) const
{
	StWriteChunk chunk(coder);
	uint32_t n = _count;
	coder->storeBytes(&n, 4);
	for (unsigned int i = 0; i < _count; i++)
		coder->encodeObject(objectAtIndex(i));
}

// The elements are copies, and the copy itself starts with a retain count
// of one as copy() requires.
LArray *LArray::copy() const
{
	LArray *a = _alloc();
	unsigned int n = _count;
	a->preserveCapacity(n);
	for (unsigned int i = 0; i < n; i++) {
		LObject *obj = objectAtIndex(i)->copy();
		a->addObject(obj);
		obj->release();
	}
	return a;
}

LString *LArray::componentsJoinedByString(LString *separator) const
{
	int n = _count;
	LString *result = LAutorelease(LString::_alloc()->init());
	unsigned int seplen = separator ? separator->length() : 0;
	unsigned int total = 0;
	for (int i = 0; i < n; i++) {
		LObject *obj = objectAtIndex(i);
		if (obj->getClass()->isSubclassOf(LF_CLASSOF(LString))) {
			total += ((LString *)obj)->length();
			if (i < n - 1)
				total += seplen;
		}
	}
	UniChar *p = result->_setLength(total);
	for (int i = 0; i < n; i++) {
		LObject *obj = objectAtIndex(i);
		if (obj->getClass()->isSubclassOf(LF_CLASSOF(LString))) {
			LString *s = (LString *)obj;
			memmove(p, s->chars(), s->length() * sizeof(UniChar));
			p += s->length();
			if (separator && i < n - 1) {
				memmove(p, separator->chars(), seplen * sizeof(UniChar));
				p += seplen;
			}
		}
	}
	return result;
}

LString *LArray::_plistRepresentation(unsigned int indent) const
{
	LString *str = LString::stringWithFormat("<array>%@", &sNewLine);
	int n = _count;
	for (int i = 0; i < n; i++) {
		str->appendString(_tabs(indent + 1));
		str->appendString(objectAtIndex(i)->_plistRepresentation(indent + 1));
		str->appendString(&sNewLine);
	}
	str->appendFormat("%@</array>", _tabs(indent));
	return str;
}

LString *LArray::fillTextRepresentation(unsigned int indent) const
{
	int n = _count;
	if (n == 0)
		return &sEmptyArrayString;
	LString *str = LString::stringWithCString("(\n", 0, kLMacRomanEncoding);
	for (int i = 0; i < n; i++) {
		str->appendString(_tabs(indent + 1));
		str->appendString(objectAtIndex(i)->fillTextRepresentation(indent + 1));
		if (i < n - 1)
			str->appendString(&sComma);
		str->appendString(&sNewLine);
	}
	str->appendString(_tabs(indent));
	str->appendString(&sCloseBracket);
	return str;
}

LString *LArray::internalDescription() const
{
	char buf[312];
	snprintf(buf, sizeof buf, "LArray(%p); count = %d\n( ", this, _count);
	LString *str = LString::stringWithCString(buf, 0, kLMacRomanEncoding);
	for (unsigned int i = 0; i < _count; i++)
		_concatStringsWithEOL(str, objectAtIndex(i)->internalDescription());
	str->appendString(&sCloseBracket);
	return str;
}

LString *LArray::description() const
{
	char buf[312];
	snprintf(buf, sizeof buf, "LArray(%p); count = %d\n( ", this, _count);
	LString *str = LString::stringWithCString(buf, 0, kLMacRomanEncoding);
	for (unsigned int i = 0; i < _count; i++)
		_concatStringsWithEOL(str, objectAtIndex(i)->description());
	str->appendString(&sCloseBracket);
	return str;
}

LEnumerator *LArray::reverseObjectEnumerator() const
{
	_LArrayEnumerator *e = _LArrayEnumerator::_alloc()->initWithContainer(this, true);
	return e ? LAutorelease(e) : 0;
}

LEnumerator *LArray::objectEnumerator() const
{
	_LArrayEnumerator *e = _LArrayEnumerator::_alloc()->initWithContainer(this, false);
	return e ? LAutorelease(e) : 0;
}

void LArray::_copyDataFromArray(const LArray *other)
{
	CArray::_copyDataFromArray(other);
	for (unsigned int i = 0; i < _count; i++)
		objectAtIndex(i)->retain();
}

void LArray::_addDataFromArray(const LArray *other)
{
	unsigned int i = _count;
	CArray::_addDataFromArray(other);
	for (; i < _count; i++)
		objectAtIndex(i)->retain();
}

void LArray::_callRemoveForAllObjects()
{
	for (unsigned int i = 0; i < _count; i++)
		objectAtIndex(i)->release();
}

void LArray::_callRemoveForObjectsInRange(LRange range)
{
	LRange r = LIntersectionRange(range, LMakeRange(0, _count));
	for (unsigned int i = r.location; i < r.location + r.length; i++)
		objectAtIndex(i)->release();
}

void LArray::addObject(const LObject *obj)
{
	const_cast<LObject *>(obj)->retain();
	CArray::addObject(&obj);
}

void LArray::insertObjectAtIndex(const LObject *obj, int index)
{
	const_cast<LObject *>(obj)->retain();
	CArray::insertObjectAtIndex(&obj, index);
}

// The new object is retained before the old one is released, so replacing
// an object with itself is safe.
void LArray::replaceObjectAtIndex(const LObject *obj, int index)
{
	const_cast<LObject *>(obj)->retain();
	objectAtIndex(index)->release();
	CArray::replaceObjectAtIndex(&obj, index);
}

void LArray::removeAllObjectsAfter(int index)
{
	for (unsigned int i = index + 1; i < _count; i++)
		objectAtIndex(i)->release();
	CArray::removeAllObjectsAfter(index);
}

void LArray::removeObjectAtIndex(int index)
{
	if (index >= 0 && index < (int)_count) {
		objectAtIndex(index)->release();
		CArray::removeObjectAtIndex(index);
	}
}

void LArray::removeAllObjects(unsigned int capacity)
{
	_callRemoveForAllObjects();
	CArray::removeAllObjects(capacity);
}

void LArray::removeObjectsInRange(LRange range)
{
	_callRemoveForObjectsInRange(range);
	CArray::removeObjectsInRange(range);
}

// The object is released before it is removed, as in the original.
void LArray::removeObject(const LObject *obj)
{
	int i = CArray::indexOfObject(&obj, false);
	if (i >= 0) {
		const_cast<LObject *>(obj)->release();
		CArray::removeObjectAtIndex(i);
	}
}

// _LArrayEnumerator

_LArrayEnumerator *_LArrayEnumerator::initWithContainer(const LArray *array, bool reverse)
{
	if (LEnumerator::initWithContainer(const_cast<LArray *>(array)) == 0)
		return 0;
	_reverse = reverse;
	reset();
	return this;
}

void _LArrayEnumerator::reset()
{
	_index = _reverse ? (int)((LArray *)_container)->count() - 1 : 0;
}

LObject *_LArrayEnumerator::next()
{
	LArray *a = (LArray *)_container;
	int i = _index;
	if (!_reverse) {
		if (i >= (int)a->count())
			return 0;
		_index = i + 1;
	} else {
		if (i < 0)
			return 0;
		_index = i - 1;
	}
	return a->objectAtIndex(i);
}

// Licence keys. A public key is pairs of characters, each of which carries
// alternate bits of a key byte (bits 0x55); the other bits are random.

int SHIFER_streamConvert(char c, unsigned int index, LString *publicKey, LString *ownKey)
{
	if (publicKey && ownKey && publicKey->length() > 1) {
		unsigned int pairs = publicKey->length() >> 1;
		unsigned int ownLength = ownKey->length();
		unsigned int i = index % pairs;
		const unsigned char *pub = (const unsigned char *)publicKey->cString();
		const unsigned char *own = (const unsigned char *)ownKey->cString();
		c = (char)(((pub[i * 2] & 0x55) | ((pub[i * 2 + 1] & 0x55) << 1)) ^ own[i % ownLength] ^ c);
	}
	return c;
}

// The MAC address of the first Ethernet interface that has one.
bool fillMacAddrBuffer(unsigned char *buffer)
{
	io_iterator_t iter = 0;
	kern_return_t kr = IOServiceGetMatchingServices(MACH_PORT_NULL, IOServiceMatching(kIOEthernetInterfaceClass), &iter);
	if (kr == KERN_SUCCESS) {
		kr = 5;
		memset(buffer, 0, 6);
		bool found = false;
		while (!found) {
			io_object_t service = IOIteratorNext(iter);
			if (service == 0)
				break;
			io_object_t parent;
			kr = IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent);
			if (kr == KERN_SUCCESS) {
				CFTypeRef mac = IORegistryEntryCreateCFProperty(parent, CFSTR("IOMACAddress"), kCFAllocatorDefault, 0);
				found = mac != 0;
				if (found) {
					CFDataGetBytes((CFDataRef)mac, CFRangeMake(0, 6), buffer);
					CFRelease(mac);
				}
				IOObjectRelease(parent);
			}
			IOObjectRelease(service);
		}
	}
	IOObjectRelease(iter);
	return kr == KERN_SUCCESS;
}

LString *SHIFER_ownKey()
{
	unsigned char mac[6];
	if (!fillMacAddrBuffer(mac))
		return 0;
	return LString::stringWithFormat("%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Spreads the bits of a XOR b over pairs of printable characters, avoiding
// '"' and DEL. The original wrote into a 4008-byte stack buffer without a
// check; the buffer here is sized to the keys.
LString *SHIFER_genPublicKey(LString *a, LString *b)
{
	int alen = a->length();
	const unsigned char *as = (const unsigned char *)a->cString();
	int blen = b->length();
	const unsigned char *bs = (const unsigned char *)b->cString();
	int n = alen > blen ? alen : blen;
	unsigned char *buf = (unsigned char *)malloc(n * 2 + 1);
	for (int i = 0; i < n; i++) {
		unsigned char x = bs[i % blen] ^ as[i % alen];
		unsigned char even = x & 0x55;
		unsigned char odd = (x >> 1) & 0x55;
		unsigned char r = (unsigned char)(int)((float)rand() * 255.0f * 4.656613e-10f + 0.0f);
		unsigned char c = (r & 10) | even | 0x20;
		buf[i * 2] = (c == '"' || c == 0x7f) ? (even | 0x20) : c;
		c = odd | ((r >> 4) & 10) | 0x20;
		buf[i * 2 + 1] = (c == '"' || c == 0x7f) ? (odd | 0x20) : c;
	}
	buf[n * 2] = 0;
	LString *result = LString::stringWithCString((const char *)buf, 0, kLMacRomanEncoding);
	free(buf);
	return result;
}

} // namespace LF
