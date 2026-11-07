// LValue: a boxed number, boolean or small block of bytes.
#pragma once

#include "LString.h"

namespace LF {

class LValue : public LComparable {
	LF_DECLARE_CLASS(LValue)
public:
	enum TMode {
		kInt = 0,
		kIntPtr = 1,
		kLongLong = 2,
		kFloat = 3,
		kDouble = 4,
		kBool = 5,
		kBuffer = 6,	// up to 16 bytes, kept inline
		kPointer = 7,	// longer, kept in a malloc'd block
	};

	LValue() : _mode(kInt), _size(0), _cfNumber(0) { memset(&_data, 0, sizeof _data); }
	virtual ~LValue() {}
	virtual LValue *copy() const;
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
	virtual LValue *getOwnerLObject() { return this; }

	// Factories (autoreleased)
	static LValue *valueWithInt(int value);
	static LValue *valueWithInt64(long long value)
	{
		LValue *v = _alloc();
		v->setValue(value);
		return LAutorelease(v);
	}
	static LValue *valueWithIntPtr(intptr_t value);
	static LValue *valueWithBoolean(int value);
	static LValue *valueWithFloat(float value);
	static LValue *valueWithDouble(double value);
	static LValue *valueWithBytes(const void *bytes, unsigned int length);
	static LValue *valueWithRange(LRange range);
	static LValue *valueWithRect(LRect rect);
	static LValue *valueWithSize(LSize size);
	static LValue *valueWithPoint(LPoint point);

	LValue *initWithValue(const LValue *other);

	void setValue(int value);
	void setValue(unsigned char value);	// a boolean
	void setPtrValue(intptr_t value);
	void setValue(long long value);
	void setValue(float value);
	void setValue(double value);
	void setValue(const void *bytes, unsigned int length);

	TMode mode() const { return _mode; }
	// 1 for integer modes, 2 for float, 3 for double, otherwise 0.
	int bestGetMode() const;
	int intValue() const;
	intptr_t intPtrValue() const;	// 0 unless an intptr
	long long int64Value() const;
	float floatValue() const;
	double doubleValue() const;
	void getValue(void *out) const;	// buffer modes only
	unsigned int length() const { return _size; }
	const void *bytes() const { return _mode == kPointer ? _data.pointer : (const void *)&_data; }
	CFNumberRef CFNumber();
	LPoint pointValue() const;
	LSize sizeValue() const;
	LRect rectValue() const;
	LRange rangeValue() const;
	LString *stringValue() const;
	int compareWithString(const LString *str) const;

	void _clear();
	void _changed();

	union {
		int i;
		intptr_t ptr;
		long long ll;
		float f;
		double d;
		unsigned char bytes[16];
		void *pointer;
	} _data;
	TMode _mode;
	unsigned int _size;
	CFNumberRef _cfNumber;
};

} // namespace LF
