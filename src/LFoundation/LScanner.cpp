// LScanner
//
// LF::LScanner, LF::LMapScanner and LF::UMapParsingUtils.

#include "LScanner.h"
#include "LData.h"
#include "LMap.h"
#include "LValue.h"

namespace LF {

LF_IMPLEMENT_CLASS(LScanner, 0)

LScanner *LScanner::scannerWithString(const LString *str)
{
	LScanner *scanner = _alloc();
	if (scanner && scanner->initWithString(str))
		return LAutorelease(scanner);
	return scanner;
}

LScanner *LScanner::initWithString(const LString *str)
{
	_string = LRetain(const_cast<LString *>(str));
	return this;
}

void LScanner::dealloc()
{
	_string->release();
	LObject::dealloc();
}

void LScanner::movePositionBy(long delta)
{
	unsigned int pos = _position + delta;
	if (delta < 0 && _position < pos)
		_position = 0;
	else
		_position = pos;
	if (_position > _string->length())
		_position = _string->length();
}

bool LScanner::scanToCharSet(LCharSet *set, LString **out)
{
	unsigned int end = _string->findFirstCharFromSet(set, _position, 1);
	bool more = true;
	if (end == (unsigned int)-1) {
		end = _string->length();
		more = _position < end;
	}
	if (out)
		*out = _string->substringWithRange(_position, end - _position);
	_position = end;
	return more;
}

bool LScanner::scanCharSet(LCharSet *set, LString **out)
{
	unsigned int end = _string->findFirstCharNotFromSet(set, _position, 1);
	bool more = true;
	if (end == (unsigned int)-1) {
		end = _string->length();
		more = _position < end;
	}
	if (out)
		*out = _string->substringWithRange(_position, end - _position);
	_position = end;
	return more;
}

// Nested pairs are skipped: a close counts only if no further open comes
// before it.
bool LScanner::scanInsideBrackets(LString *open, LString *close, LString **out)
{
	unsigned int openLength = open->length();
	unsigned int closeLength = close->length();
	unsigned int at = _string->findString(open, _position, 1);
	if (at == (unsigned int)-1)
		return false;
	unsigned int start = at + openLength;
	unsigned int closeAt = start;
	unsigned int openAt = start;
	while ((closeAt = _string->findString(close, closeAt, 1)) != (unsigned int)-1) {
		openAt = _string->findString(open, openAt, 1);
		if (openAt == (unsigned int)-1 || closeAt < openAt) {
			if (out)
				*out = _string->substringWithRange(start, closeAt - start);
			_position = closeAt + closeLength;
			return true;
		}
		closeAt += closeLength;
		openAt += openLength;
	}
	return false;
}

void LMapScanner::scanMap(const LMap *map)
{
	LMapIterator it(map, false);
	while (it.next())
		scanKeyAndValue(it.key(), it.object(), map);
}

void UMapParsingUtils::loadStructFromObject(void *out, unsigned int size, const LObject *obj)
{
	if (obj == 0)
		return;
	if (obj->getClass()->isSubclassOf(LF_CLASSOF(LValue)))
		((const LValue *)obj)->getValue(out);
	else if (obj->getClass()->isSubclassOf(LF_CLASSOF(LData)))
		memcpy(out, ((const LData *)obj)->bytes(), size);
}

double UMapParsingUtils::loadDoubleFromObject(const LObject *obj)
{
	if (obj == 0)
		return 0;
	if (obj->getClass()->isSubclassOf(LF_CLASSOF(LString)))
		return ((const LString *)obj)->doubleValue();
	if (obj->getClass()->isSubclassOf(LF_CLASSOF(LValue)))
		return ((const LValue *)obj)->doubleValue();
	return 0;
}

int UMapParsingUtils::loadIntFromObject(const LObject *obj)
{
	if (obj == 0)
		return 0;
	if (obj->getClass()->isSubclassOf(LF_CLASSOF(LString)))
		return ((const LString *)obj)->intValue();
	if (obj->getClass()->isSubclassOf(LF_CLASSOF(LValue)))
		return ((const LValue *)obj)->intValue();
	return 0;
}

float UMapParsingUtils::loadFloatFromObject(const LObject *obj)
{
	if (obj == 0)
		return 0;
	if (obj->getClass()->isSubclassOf(LF_CLASSOF(LString)))
		return ((const LString *)obj)->floatValue();
	if (obj->getClass()->isSubclassOf(LF_CLASSOF(LValue)))
		return ((const LValue *)obj)->floatValue();
	return 0;
}

} // namespace LF
