// LCharSet
//
// LF::LCharSet: a bitmap of UTF-16 code units. The predefined sets are made
// once and kept; most come from LFoundation's LCharSet.bitmaps resource.

#include "LCleanupTask.h"
#include "LString.h"
#include "LData.h"

namespace LF {

LF_IMPLEMENT_CLASS(LCharSet, 0)

static LCharSet *sAllCharSets[14];
static LData *sAllBitmapReps;

LCharSet *LCharSet::initWithCharSet(const LCharSet *other)
{
	if (other)
		memcpy(_bits, other->_bits, sizeof _bits);
	return this;
}

// Characters above 0x7f are sign-extended, as the original did, so they
// land in 0xff80-0xffff.
LCharSet *LCharSet::initWithCString(const char *str)
{
	size_t n = strlen(str);
	for (size_t i = 0; i < n; i++) {
		UniChar c = (UniChar)(short)str[i];
		_bits[c >> 3] |= (unsigned char)(1 << (str[i] & 7));
	}
	return this;
}

LCharSet *LCharSet::initWithData(LData *data)
{
	if (data->length() != sizeof _bits) {
		dealloc();
		return 0;
	}
	memcpy(_bits, data->bytes(), sizeof _bits);
	return this;
}

void LCharSet::dealloc()
{
	LObject::dealloc();
}

void LCharSet::formUnionWithCharacterSet(const LCharSet *other)
{
	if (other)
		for (size_t i = 0; i < sizeof _bits; i++)
			_bits[i] |= other->_bits[i];
}

LCharSet *LCharSet::initWithString(const LString *str)
{
	const UniChar *p = str->chars();
	unsigned int n = str->length();
	for (unsigned int i = 0; i < n; i++)
		addCharacter(p[i]);
	return this;
}

LCharSet *LCharSet::invertedSet() const
{
	LCharSet *set = _alloc();
	for (size_t i = 0; i < sizeof _bits; i++)
		set->_bits[i] = (unsigned char)~_bits[i];
	return LAutorelease(set);
}

void LCharSet::addCharactersInString(LString *str)
{
	formUnionWithCharacterSet(characterSetWithCharactersInString(str));
}

void LCharSet::addCharactersInCString(const char *str)
{
	formUnionWithCharacterSet(characterSetWithCharactersInCString(str));
}

LCharSet *LCharSet::copy() const
{
	LCharSet *set = _alloc();
	memcpy(set->_bits, _bits, sizeof _bits);
	return set;
}

LCharSet *LCharSet::charSetWithType(int type)
{
	if ((unsigned int)type > kSeparators)
		return 0;
	if (sAllCharSets[type])
		return sAllCharSets[type];
	static CSharableResource mutex;
	StSharableLocker lock(&mutex);
	if (sAllCharSets[type])
		return sAllCharSets[type];
	LCharSet *set = 0;
	if (type < kOperators) {
		if (sAllBitmapReps == 0) {
			sAllBitmapReps = LData::_alloc()->initWithResource(LString::stringWithCString("LCharSet"),
			                                                   LString::stringWithCString("bitmaps"),
			                                                   getLFoundationModule());
			if (sAllBitmapReps == 0)
				return 0;
			sAllBitmapReps->expand();
			if (sAllBitmapReps->length() >> 13 != 12)
				return 0;
		}
		LData *data = LData::dataWithBytesNoCopy((char *)sAllBitmapReps->bytes() + type * 0x2000, 0x2000, false);
		set = _alloc()->initWithData(data);
	} else {
		set = _alloc()->initWithCString(type == kOperators ? "<>=!^&|" : " \r\n\t{}");
	}
	sAllCharSets[type] = set;
	return set;
}

LCharSet *LCharSet::characterSetWithCharactersInString(const LString *str)
{
	return str ? LAutorelease(_alloc()->initWithString(str)) : 0;
}

LCharSet *LCharSet::characterSetWithCharactersInCString(const char *str)
{
	return str ? LAutorelease(_alloc()->initWithCString(str)) : 0;
}

// Releases the shared character sets and their bitmaps.
class _charSetsCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		for (int i = 0; i < 14; i++) {
			if (sAllCharSets[i]) {
				sAllCharSets[i]->release();
				sAllCharSets[i] = 0;
			}
		}
		if (sAllBitmapReps) {
			sAllBitmapReps->release();
			sAllBitmapReps = 0;
		}
	}
};
static _charSetsCleanup sCharSetsCleanup;

} // namespace LF
