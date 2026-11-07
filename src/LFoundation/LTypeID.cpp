// LTypeID
//
// LF::LTypeID::convertValue: copies values between types for outlets and
// key-value coding. Numbers convert between the numeric types; one number
// becomes an LValue, several become an LString "{a, b, ...}"; an LString
// is split at commas into numbers; an LValue converts to a number, a
// struct of the same size, or a string.

#include "LValue.h"
#include "LArray.h"
#include <math.h>

namespace LF {

bool LTypeID::operator==(const LTypeID &o) const
{
	TType a = getType(), b = o.getType();
	if (a != b || _numElements != o._numElements)
		return false;
	if (a == kNone)
		return getClass() == o.getClass();
	if (a == kStruct)
		return *_typeInfo == *o._typeInfo;
	return true;
}

unsigned int LTypeID::getArgSize() const
{
	if (_structSize != 0)
		return _structSize * _numElements;
	switch (_typeOrClass) {
	case kStruct:
		return _structSize * _numElements;
	case kInt:
	case kFloat:
		return _numElements * 4;
	case kBool:
		return _numElements;
	case kDouble:
		return _numElements * 8;
	default:	// intptr, objects, none
		return _numElements * sizeof(intptr_t);
	}
}

namespace {

struct _IntValsCreator {
	static LValue *createLValue(int v) { return LValue::valueWithInt(v); }
	static LString *createLString(int v) { return LString::stringWithInt(v); }
};
struct _IntPtrValsCreator {
	static LValue *createLValue(intptr_t v) { return LValue::valueWithIntPtr(v); }
	static LString *createLString(intptr_t v) { return LString::stringWithIntPtr(v); }
};
struct _BoolValsCreator {
	static LValue *createLValue(unsigned char v) { return LValue::valueWithBoolean(v); }
	static LString *createLString(unsigned char v) { return LString::stringWithCString(v ? "YES" : "NO", 0); }
};
struct _FloatValsCreator {
	static LValue *createLValue(float v) { return LValue::valueWithFloat(v); }
	static LString *createLString(float v) { return LString::stringWithFormat("%G", (double)v); }
};
struct _DoubleValsCreator {
	static LValue *createLValue(double v) { return LValue::valueWithDouble(v); }
	static LString *createLString(double v) { return LString::stringWithFormat("%G", v); }
};

template <typename T, typename Creator>
bool _TConvert(const void *src, void *dst, unsigned int n, const LTypeID &dstType)
{
	const T *s = (const T *)src;
	switch (dstType.getType()) {
	case LTypeID::kInt:
		for (unsigned int i = 0; i < n; i++)
			((int *)dst)[i] = (int)s[i];
		return true;
	case LTypeID::kBool:
		for (unsigned int i = 0; i < n; i++)
			((unsigned char *)dst)[i] = s[i] != 0;
		return true;
	case LTypeID::kIntPtr:
		for (unsigned int i = 0; i < n; i++)
			((intptr_t *)dst)[i] = (intptr_t)s[i];
		return true;
	case LTypeID::kFloat:
		for (unsigned int i = 0; i < n; i++)
			((float *)dst)[i] = (float)s[i];
		return true;
	case LTypeID::kDouble:
		for (unsigned int i = 0; i < n; i++)
			((double *)dst)[i] = (double)s[i];
		return true;
	case LTypeID::kStruct:
		LLog("invalid conversion in 'LTypeID::convertValue'\n");
		return false;
	default:
		break;
	}
	LClass *cls = dstType.getClass();
	bool toValue = cls ? cls->isSubclassOf(LF_CLASSOF(LValue)) : n == 1;
	if (toValue) {
		if (n != 1)
			return false;
		*(LValue **)dst = Creator::createLValue(s[0]);
		return true;
	}
	if (cls && !cls->isSubclassOf(LF_CLASSOF(LString))) {
		LLog("cannot perform LClass conversion in 'LTypeID::convertValue'\n");
		return false;
	}
	LString *str = Creator::createLString(s[0]);
	for (unsigned int i = 1; i < n; i++)
		str->appendFormat(", %@", Creator::createLString(s[i]));
	if (n != 1)
		str = LString::stringWithFormat("{%@}", str);
	*(LString **)dst = str;
	return true;
}

} // namespace

// Number lists longer than the destination were written past its end by
// the original; here only the destination's elements are filled.
bool LTypeID::convertValue(const void *src, const LTypeID &dstType, void *dst) const
{
	LClass *dstClass = dstType.getClass();
	TType dstKind = dstType.getType();
	if (*this == dstType ||
	    (dstKind == kNone && dstClass == 0 && _structSize == 0 && (_typeOrClass > 6 || _typeOrClass == 0))) {
		memcpy(dst, src, getArgSize());
		return true;
	}
	if (_structSize != 0)
		return false;
	if (_typeOrClass < 7) {
		switch (_typeOrClass) {
		case kInt:
			return _TConvert<int, _IntValsCreator>(src, dst, _numElements, dstType);
		case kBool:
			return _TConvert<unsigned char, _BoolValsCreator>(src, dst, _numElements, dstType);
		case kIntPtr:
			return _TConvert<intptr_t, _IntPtrValsCreator>(src, dst, _numElements, dstType);
		case kFloat:
			return _TConvert<float, _FloatValsCreator>(src, dst, _numElements, dstType);
		case kDouble:
			return _TConvert<double, _DoubleValsCreator>(src, dst, _numElements, dstType);
		case kNone:
			break;
		default:
			return false;
		}
	}
	LClass *srcClass = getClass();
	if (dstClass && srcClass && srcClass->isSubclassOf(dstClass)) {
		memcpy(dst, src, getArgSize());
		return true;
	}
	LValue *value = 0;
	if (srcClass && srcClass->isSubclassOf(LF_CLASSOF(LString))) {
		LString *str = *(LString *const *)src;
		LArray *parts = 0;
		unsigned int count = dstType._numElements;
		if ((unsigned int)(dstKind - 2) <= 4 && str != 0) {
			parts = str->componentsSeparatedByString(LString::stringWithCString(",", 0),
			                                         LCharSet::charSetWithType(LCharSet::kSeparators));
			if (parts)
				count = parts->count();
		}
		if (count < dstType._numElements)
			return false;
		if (count > dstType._numElements)
			count = dstType._numElements;
#define PART(i) ((LString *)parts->objectAtIndex(i))
		switch (dstKind) {
		case kNone: {
			double d = str ? str->doubleValue() : 0;
			value = d - floor(d) != 0 ? LValue::valueWithDouble(d) : LValue::valueWithIntPtr((intptr_t)d);
			if (dstClass && dstClass->isSubclassOf(LF_CLASSOF(LValue))) {
				*(LValue **)dst = value;
				return true;
			}
			break;
		}
		case kInt:
			for (unsigned int i = 0; i < count; i++)
				((int *)dst)[i] = parts ? PART(i)->intValue() : 0;
			return true;
		case kBool:
			for (unsigned int i = 0; i < count; i++)
				((unsigned char *)dst)[i] = parts ? PART(i)->boolValue() : 0;
			return true;
		case kIntPtr:
			for (unsigned int i = 0; i < count; i++)
				((intptr_t *)dst)[i] = parts ? PART(i)->intValue() : 0;
			return true;
		case kFloat:
			for (unsigned int i = 0; i < count; i++)
				((float *)dst)[i] = parts ? PART(i)->floatValue() : 0;
			return true;
		case kDouble:
			for (unsigned int i = 0; i < count; i++)
				((double *)dst)[i] = parts ? PART(i)->doubleValue() : 0;
			return true;
		default:
			LLog("invalid conversion in 'LTypeID::convertValue'\n");
			return false;
		}
#undef PART
	} else if (srcClass && srcClass->isSubclassOf(LF_CLASSOF(LValue))) {
		value = *(LValue *const *)src;
		if (dstType._numElements != 1)
			return false;
		switch (dstKind) {
		case kNone:
			break;
		case kStruct:
			if (dstType._structSize == value->_size) {
				memcpy(dst, value->bytes(), dstType._structSize);
				return true;
			}
			LLog("invalid conversion in 'LTypeID::convertValue'\n");
			return false;
		case kInt:
			*(int *)dst = value->intValue();
			return true;
		case kBool:
			*(bool *)dst = value->intValue() != 0;
			return true;
		case kIntPtr:
			*(intptr_t *)dst = value->intValue();
			return true;
		case kFloat:
			*(float *)dst = value->floatValue();
			return true;
		case kDouble:
			*(double *)dst = value->doubleValue();
			return true;
		}
	} else {
		return false;
	}
	if (dstClass && dstClass->isSubclassOf(LF_CLASSOF(LString))) {
		*(LString **)dst = value->stringValue();
		return true;
	}
	LLog("cannot perform LClass conversion in 'LTypeID::convertValue'\n");
	return false;
}

} // namespace LF
