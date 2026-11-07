// LTimeZone
//
// LF::LTimeZone: a CFTimeZone, and the shared system and default zones.

#include "LCleanupTask.h"
#include "LDate.h"
#include "LArray.h"

namespace LF {

LF_IMPLEMENT_CLASS(LTimeZone, 0)

LTimeZone *LTimeZone::sDefaultTimeZone = 0;
LArray *LTimeZone::sNames = 0;

CSharableResource *LTimeZone::locker()
{
	static CSharableResource *sLocker = new CSharableResource;
	return sLocker;
}

LTimeZone *LTimeZone::initWithUNIXName(const LString *name)
{
	if (LObject::init() == 0)
		return 0;
	_cf = CFTimeZoneCreateWithName(kCFAllocatorDefault, name->cfString(), true);
	if (_cf)
		return this;
	dealloc();
	return 0;
}

LTimeZone *LTimeZone::initWithSecondsFromGMT(int seconds)
{
	if (LObject::init() == 0)
		return 0;
	_cf = CFTimeZoneCreateWithTimeIntervalFromGMT(kCFAllocatorDefault, seconds);
	return this;
}

void LTimeZone::dealloc()
{
	if (_cf)
		CFRelease(_cf);
	LObject::dealloc();
}

int LTimeZone::secondsFromGMTForDate(LDate *date)
{
	return (int)CFTimeZoneGetSecondsFromGMT(_cf, date->_time);
}

int LTimeZone::secondsFromGMT()
{
	return (int)CFTimeZoneGetSecondsFromGMT(_cf, CFAbsoluteTimeGetCurrent());
}

void LTimeZone::setDefaultTimeZone(LTimeZone *zone)
{
	StSharableLocker lock(locker(), true);
	LRetain(zone);
	if (sDefaultTimeZone)
		sDefaultTimeZone->release();
	sDefaultTimeZone = zone;
}

LString *LTimeZone::name() const
{
	return LString::stringWithCFString(CFTimeZoneGetName(_cf));
}

LString *LTimeZone::description() const
{
	double offset = CFTimeZoneGetSecondsFromGMT(_cf, CFAbsoluteTimeGetCurrent());
	return LString::stringWithFormat("%s<%p>; name = %@; GMT offset = %g", className(), this, name(), offset);
}

LArray *LTimeZone::knownTimeZoneNames()
{
	if (sNames == 0) {
		StSharableLocker lock(locker(), true);
		if (sNames == 0) {
			CFArrayRef names = CFTimeZoneCopyKnownNames();
			CFIndex n = names ? CFArrayGetCount(names) : 0;
			LArray *a = LArray::_alloc()->init();
			for (CFIndex i = 0; i < n; i++)
				a->addObject(LString::stringWithCFString((CFStringRef)CFArrayGetValueAtIndex(names, i)));
			if (names)
				CFRelease(names);
			a->setComparator(_LComparableObjectCompareCallback);
			a->sort();
			sNames = a;
		}
	}
	return sNames;
}

LTimeZone *LTimeZone::systemTimeZone(bool create)
{
	static LTimeZone *sSystemTimeZone = 0;
	StSharableLocker lock(locker(), true);
	if (create && sSystemTimeZone == 0) {
		LTimeZone *zone = _alloc();
		zone->_cf = CFTimeZoneCopySystem();
		sSystemTimeZone = zone;
	}
	return sSystemTimeZone;
}

LTimeZone *LTimeZone::defaultTimeZone()
{
	StSharableLocker lock(locker(), true);
	if (sDefaultTimeZone == 0)
		setDefaultTimeZone(systemTimeZone(true));
	return sDefaultTimeZone;
}

// Releases the system and default zones and the zone names. (As in the
// original, the system zone pointer itself is left as it was.)
class _tzCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		StSharableLocker lock(LTimeZone::locker(), true);
		if (LTimeZone *zone = LTimeZone::systemTimeZone(false))
			zone->release();
		if (LTimeZone::sDefaultTimeZone) {
			LTimeZone::sDefaultTimeZone->release();
			LTimeZone::sDefaultTimeZone = 0;
		}
		if (LTimeZone::sNames) {
			LTimeZone::sNames->release();
			LTimeZone::sNames = 0;
		}
	}
};
static _tzCleanup sTZCleanup;

} // namespace LF
