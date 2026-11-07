// LValue
//
// LF::LValue: an int, intptr, long long, float, double or boolean, or a
// block of bytes (inline up to 16, else malloc'd). Conversions between the
// modes and to strings follow the original's choices, including its
// formatting of doubles through a float.

#include "LValue.h"
#include "LCoder.h"
#include <math.h>

namespace LF {

LF_IMPLEMENT_CLASS(LValue, LF_CLASSOF(LComparable))

// The original's read of a value "as an int", used by several methods.
static int _asInt(const LValue *v)
{
	switch (v->_mode) {
	case LValue::kInt:
	case LValue::kIntPtr:
	case LValue::kLongLong:
	case LValue::kBool:
		return v->_data.i;
	case LValue::kFloat:
		return (int)v->_data.f;
	case LValue::kDouble:
		return (int)v->_data.d;
	default:
		return 0;
	}
}

void LValue::_clear()
{
	if (_mode == kPointer)
		free(_data.pointer);
}

void LValue::_changed()
{
	_hashCached = false;
	if (_cfNumber) {
		CFRelease(_cfNumber);
		_cfNumber = 0;
	}
}

LValue *LValue::valueWithInt(int value)
{
	LValue *v = _alloc();
	v->setValue(value);
	return LAutorelease(v);
}

LValue *LValue::valueWithDouble(double value)
{
	LValue *v = _alloc();
	v->setValue(value);
	return LAutorelease(v);
}

LValue *LValue::valueWithIntPtr(intptr_t value)
{
	LValue *v = _alloc();
	v->setPtrValue(value);
	return LAutorelease(v);
}

LValue *LValue::valueWithBoolean(int value)
{
	LValue *v = _alloc();
	v->setValue((unsigned char)(value != 0));
	return LAutorelease(v);
}

LValue *LValue::valueWithFloat(float value)
{
	LValue *v = _alloc();
	v->setValue(value);
	return LAutorelease(v);
}

LValue *LValue::valueWithBytes(const void *bytes, unsigned int length)
{
	LValue *v = _alloc();
	v->setValue(bytes, length);
	return LAutorelease(v);
}

LValue *LValue::valueWithRange(LRange range)
{
	return valueWithBytes(&range, sizeof range);
}

LValue *LValue::valueWithRect(LRect rect)
{
	return valueWithBytes(&rect, sizeof rect);
}

LValue *LValue::valueWithSize(LSize size)
{
	return valueWithBytes(&size, sizeof size);
}

LValue *LValue::valueWithPoint(LPoint point)
{
	return valueWithBytes(&point, sizeof point);
}

void LValue::setValue(const void *bytes, unsigned int length)
{
	_clear();
	_size = length;
	if (length <= 16) {
		_mode = kBuffer;
		memcpy(_data.bytes, bytes, length);
	} else {
		_mode = kPointer;
		_data.pointer = malloc(length);
		memcpy(_data.pointer, bytes, length);
	}
	_changed();
}

void LValue::dealloc()
{
	_clear();
	memset(&_data, 0, sizeof _data);
	_mode = kInt;
	if (_cfNumber) {
		CFRelease(_cfNumber);
		_cfNumber = 0;
	}
	LObject::dealloc();
}

int LValue::bestGetMode() const
{
	switch (_mode) {
	case kInt:
	case kIntPtr:
	case kLongLong:
	case kBool:
		return 1;
	case kFloat:
		return 2;
	case kDouble:
		return 3;
	default:
		return 0;
	}
}

int LValue::intValue() const
{
	return _asInt(this);
}

intptr_t LValue::intPtrValue() const
{
	return _mode == kIntPtr ? _data.ptr : 0;
}

long long LValue::int64Value() const
{
	switch (_mode) {
	case kInt:
	case kBool:
		return _data.i;
	case kIntPtr:
		return _data.ptr;
	case kLongLong:
		return _data.ll;
	case kFloat:
		return llrintf(_data.f);
	case kDouble:
		return llrint(_data.d);
	default:
		return 0;
	}
}

float LValue::floatValue() const
{
	switch (_mode) {
	case kInt:
	case kBool:
		return (float)_data.i;
	case kIntPtr:
		return (float)_data.ptr;
	case kLongLong:
		return (float)_data.ll;
	case kFloat:
		return _data.f;
	case kDouble:
		return (float)_data.d;
	default:
		return 0;
	}
}

// Ints convert through a float, as the original did.
double LValue::doubleValue() const
{
	switch (_mode) {
	case kInt:
	case kBool:
		return (float)_data.i;
	case kIntPtr:
		return (float)_data.ptr;
	case kLongLong:
		return (double)(unsigned int)_data.ll + (double)(int)(_data.ll >> 32) * 4294967296.0;
	case kFloat:
		return _data.f;
	case kDouble:
		return _data.d;
	default:
		return 0;
	}
}

void LValue::getValue(void *out) const
{
	if (_mode == kBuffer)
		memcpy(out, _data.bytes, _size);
	else if (_mode == kPointer)
		memcpy(out, _data.pointer, _size);
}

CFNumberRef LValue::CFNumber()
{
	if (_cfNumber == 0) {
		CFNumberType type;
		switch (_mode) {
		case kInt:
		case kIntPtr:
		case kBool:
			type = kCFNumberIntType;
			break;
		case kLongLong:
			type = kCFNumberLongLongType;
			break;
		case kFloat:
			type = kCFNumberFloatType;
			break;
		case kDouble:
			type = kCFNumberDoubleType;
			break;
		default:
			return _cfNumber;
		}
		_cfNumber = CFNumberCreate(kCFAllocatorDefault, type, &_data);
	}
	return _cfNumber;
}

// The string hash over the value's bytes (at most 32 of a long block).
unsigned int LValue::_calcHashValue() const
{
	unsigned int n = 0;
	const unsigned char *p = _data.bytes;
	switch (_mode) {
	case kInt:
	case kIntPtr:
	case kFloat:
	case kBool:
		n = 4;
		break;
	case kLongLong:
	case kDouble:
		n = 8;
		break;
	case kBuffer:
		n = _size;
		break;
	case kPointer:
		n = _size;
		p = (const unsigned char *)_data.pointer;
		if (n > 32)
			n = 32;
		break;
	}
	unsigned int h = 0;
	for (unsigned int i = 0; i < n; i++) {
		h = h * 16 + p[i];
		unsigned int g = h & 0xf0000000;
		h ^= g >> 24 | g;
	}
	return h;
}

// Same mode and bytes. intptr values compare as nothing (equal), as in the
// original.
bool LValue::_isEqualTo(const LComparable *other) const
{
	if (other == 0 || !other->getClass()->isSubclassOf(classObject()))
		return false;
	const LValue *o = (const LValue *)other;
	if (_mode != o->_mode)
		return false;
	size_t n;
	switch (_mode) {
	case kInt:
	case kFloat:
	case kBool:
		n = 4;
		break;
	case kLongLong:
	case kDouble:
		n = 8;
		break;
	case kBuffer:
	case kPointer:
		n = _size;
		if (n != o->_size)
			return false;
		break;
	default:
		n = 0;
		break;
	}
	if (_mode == kPointer)
		return memcmp(o->_data.pointer, _data.pointer, n) == 0;
	return memcmp(o->_data.bytes, _data.bytes, n) == 0;
}

LPoint LValue::pointValue() const
{
	LPoint p;
	memcpy(&p, _data.bytes, sizeof p);
	return p;
}

LSize LValue::sizeValue() const
{
	LSize s;
	memcpy(&s, _data.bytes, sizeof s);
	return s;
}

LRect LValue::rectValue() const
{
	LRect r;
	memcpy(&r, _data.bytes, sizeof r);
	return r;
}

LRange LValue::rangeValue() const
{
	LRange r;
	memcpy(&r, _data.bytes, sizeof r);
	return r;
}

int LValue::compareWithString(const LString *str) const
{
	switch (_mode) {
	case kInt:
	case kLongLong:
	case kBool: {
		int a = _asInt(this), b = str->intValue();
		if (a == b)
			return 0;
		return b <= a ? 1 : -1;
	}
	case kFloat: {
		float a = _data.f, b = str->floatValue();
		if (a == b)
			return 0;
		return a > b ? 1 : -1;
	}
	case kDouble: {
		double a = _data.d, b = str->doubleValue();
		if (a == b)
			return 0;
		return a > b ? 1 : -1;
	}
	default:
		return -1;
	}
}

void LValue::setValue(int value)
{
	_clear();
	_mode = kInt;
	_data.ll = 0;
	_data.i = value;
	_changed();
}

void LValue::setValue(unsigned char value)
{
	setValue((int)value);
	_mode = kBool;
}

void LValue::setPtrValue(intptr_t value)
{
	_clear();
	_mode = kIntPtr;
	_data.ptr = value;
	_changed();
}

void LValue::setValue(long long value)
{
	_clear();
	_mode = kLongLong;
	_data.ll = value;
	_changed();
}

void LValue::setValue(float value)
{
	_clear();
	_mode = kFloat;
	_data.f = value;
	_changed();
}

void LValue::setValue(double value)
{
	_clear();
	_mode = kDouble;
	_data.d = value;
	_changed();
}

// Doubles are printed through a float, as the original did.
LString *LValue::fillTextRepresentation(unsigned int) const
{
	switch (_mode) {
	case kInt:
	case kBool:
		return LString::stringWithFormat("%d", _asInt(this));
	case kLongLong:
		return LString::stringWithFormat("%qi", _data.ll);
	case kFloat:
		return LString::stringWithFormat("%0.7g", (double)_data.f);
	case kDouble:
		return LString::stringWithFormat("%0.16g", (double)(float)_data.d);
	default:
		return 0;
	}
}

LString *LValue::description() const
{
	switch (_mode) {
	case kInt:
	case kBool:
	case kIntPtr:
		return LString::stringWithFormat("%d", _asInt(this));
	case kLongLong:
		return LString::stringWithFormat("%qi", _data.ll);
	case kFloat:
		return LString::stringWithFormat("%0.7g", (double)_data.f);
	case kDouble:
		return LString::stringWithFormat("%0.16g", _data.d);
	default:
		return 0;
	}
}

// Values order by number; against a string the string is read as a number.
int LValue::compareWith(const LComparable *other) const
{
	if (other == 0)
		return 1;
	if (!other->getClass()->isSubclassOf(classObject())) {
		if (!other->getClass()->isSubclassOf(LF_CLASSOF(LString)))
			return -1;
		return compareWithString((const LString *)other);
	}
	const LValue *o = (const LValue *)other;
	switch (_mode) {
	case kInt:
	case kBool: {
		int a = _asInt(this), b = _asInt(o);
		if (a == b)
			return 0;
		return a < b ? -1 : 1;
	}
	case kLongLong: {
		long long b;
		switch (o->_mode) {
		case kInt:
		case kIntPtr:
		case kBool:
			b = o->_data.i;
			break;
		case kLongLong:
			b = o->_data.ll;
			break;
		case kFloat:
			b = llrintf(o->_data.f);
			break;
		case kDouble:
			b = llrint(o->_data.d);
			break;
		default:
			b = 0;
			break;
		}
		if (_data.ll == b)
			return 0;
		return _data.ll > b ? 1 : -1;
	}
	case kFloat: {
		float a = _data.f, b = o->floatValue();
		if (o->_mode != kInt && o->_mode != kIntPtr && o->_mode != kBool && o->_mode != kLongLong &&
		    o->_mode != kFloat && o->_mode != kDouble)
			b = 0;
		if (a == b)
			return 0;
		return a > b ? 1 : -1;
	}
	case kDouble: {
		double a = _data.d, b;
		switch (o->_mode) {
		case kInt:
		case kIntPtr:
		case kBool:
			b = o->_data.i;
			break;
		case kLongLong:
			b = (double)(unsigned int)o->_data.ll + (double)(int)(o->_data.ll >> 32) * 4294967296.0;
			break;
		case kFloat:
			b = o->_data.f;
			break;
		case kDouble:
			b = o->_data.d;
			break;
		default:
			b = 0;
			break;
		}
		if (a == b)
			return 0;
		return a > b ? 1 : -1;
	}
	case kBuffer: {
		if (_size != o->_size)
			return 1;
		if (_size > 15)
			return -1;
		int c = memcmp(_data.bytes, o->_data.bytes, _size);
		return c < 0 ? -1 : (c > 0 ? 1 : 0);
	}
	case kPointer: {
		if (_size != o->_size)
			return 1;
		int c = memcmp(_data.pointer, o->_data.pointer, _size);
		return c < 0 ? -1 : (c > 0 ? 1 : 0);
	}
	default:
		return -1;
	}
}

// The original read the mode, then a block of any size into the data; the
// size of a byte block was not restored.
LObject *LValue::initWithDecoder(const LCoder *coder)
{
	LCoder *c = const_cast<LCoder *>(coder);
	StReadChunk chunk(c);
	if (c->nextBlockSize() != 4)
		LF_RAISE(WrongConditionException());
	int32_t mode;
	c->readBytes(&mode);
	_mode = (TMode)mode;
	unsigned int n = c->nextBlockSize();
	if (n <= 16) {
		c->readBytes(_data.bytes);
		if (_mode == kIntPtr)
			_data.ptr = *(int32_t *)_data.bytes;
	} else {
		_data.pointer = malloc(n);
		c->readBytes(_data.pointer);
	}
	return this;
}

// The file format stores intptr values in 4 bytes.
void LValue::encodeWithCoder(LCoder *coder) const
{
	StWriteChunk chunk(coder);
	int32_t mode = _mode;
	coder->storeBytes(&mode, 4);
	const void *p = _data.bytes;
	unsigned int n = 0;
	int32_t i32;
	switch (_mode) {
	case kInt:
	case kBool:
	case kFloat:
		n = 4;
		break;
	case kIntPtr:
		i32 = (int32_t)_data.ptr;
		p = &i32;
		n = 4;
		break;
	case kLongLong:
	case kDouble:
		n = 8;
		break;
	case kPointer:
		p = _data.pointer;
		n = _size;
		break;
	case kBuffer:
		n = _size;
		break;
	}
	coder->storeBytes(p, n);
}

LString *LValue::_plistRepresentation(unsigned int) const
{
	switch (_mode) {
	case kInt:
		return LString::stringWithFormat("<integer>%d</integer>", _data.i);
	case kLongLong:
		return LString::stringWithFormat("<integer>%qi</integer>", _data.ll);
	case kFloat:
		return LString::stringWithFormat("<real>%0.7g</real>", (double)_data.f);
	case kDouble:
		return LString::stringWithFormat("<real>%0.16g</real>", (double)(float)_data.d);
	case kBool:
		return LString::stringWithCString(_data.i ? "<true/>" : "<false/>", 0);
	default:
		return 0;
	}
}

LString *LValue::internalDescription() const
{
	char buf[312];
	snprintf(buf, sizeof buf, "LValue(%p) with ", this);
	LString *s = LString::stringWithCString(buf, 0);
	switch (_mode) {
	case kInt:
		snprintf(buf, sizeof buf, "INT mode; value is %d", _data.i);
		break;
	case kIntPtr:
		snprintf(buf, sizeof buf, "INT_PTR mode; value is %p", (void *)_data.ptr);
		break;
	case kLongLong:
		snprintf(buf, sizeof buf, "LONG_LONG mode; value is %lld", _data.ll);
		break;
	case kFloat:
		snprintf(buf, sizeof buf, "FLOAT mode; value is %0.7g", (double)_data.f);
		break;
	case kDouble:
		snprintf(buf, sizeof buf, "DOUBLE mode; value is %0.16g", (double)(float)_data.d);
		break;
	case kBool:
		snprintf(buf, sizeof buf, "BOOL mode; value is %d", _data.i);
		break;
	case kBuffer:
		snprintf(buf, sizeof buf, "BUFFER mode; length is %ld", (long)_size);
		break;
	case kPointer:
		snprintf(buf, sizeof buf, "POINTER mode; length is %ld", (long)_size);
		break;
	}
	s->appendString(LString::stringWithCString(buf, 0));
	return s;
}

// A long long gives "0": the original only formatted intptr values there,
// and a long long is never one.
LString *LValue::stringValue() const
{
	switch (_mode) {
	case kInt:
		return LString::stringWithFormat("%d", _asInt(this));
	case kLongLong:
		return LString::stringWithIntPtr(0);
	case kFloat:
		return LString::stringWithFormat("%0.7g", (double)_data.f);
	case kDouble:
		return LString::stringWithFormat("%0.16g", _data.d);
	case kBool:
		return LString::stringWithCString(_asInt(this) ? "1" : "0", 0);
	default:
		return 0;
	}
}

LValue *LValue::copy() const
{
	return _alloc()->initWithValue(this);
}

// The original copied a long block's pointer, so both values freed it;
// here the block is copied.
LValue *LValue::initWithValue(const LValue *other)
{
	if (init() == 0)
		return 0;
	_mode = other->_mode;
	_data = other->_data;
	_size = other->_size;
	if (_mode == kPointer) {
		_data.pointer = malloc(_size);
		memcpy(_data.pointer, other->_data.pointer, _size);
	}
	return this;
}

} // namespace LF
