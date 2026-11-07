// LDate: a point in time (CFAbsoluteTime) with its Gregorian date in a time
// zone. LTimeZone wraps a CFTimeZone.
#pragma once

#include "LString.h"

// CFGregorianDate is deprecated but still provided.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace LF {

class LDate;

class LTimeZone : public LObject {
	LF_DECLARE_CLASS(LTimeZone)
public:
	LTimeZone() : _cf(0) {}
	virtual ~LTimeZone() {}
	virtual void dealloc();
	virtual LString *description() const;
	virtual LTimeZone *getOwnerLObject() { return this; }

	static CSharableResource *locker();
	// The system time zone, made on first use if `create`.
	static LTimeZone *systemTimeZone(bool create);
	static LTimeZone *defaultTimeZone();
	static void setDefaultTimeZone(LTimeZone *zone);
	// Sorted.
	static LArray *knownTimeZoneNames();

	LTimeZone *initWithUNIXName(const LString *name);
	LTimeZone *initWithSecondsFromGMT(int seconds);
	static LTimeZone *timeZoneForSecondsFromGMT(int seconds)
	{
		LTimeZone *z = _alloc();
		if (z && (z = z->initWithSecondsFromGMT(seconds)) != 0)
			return LAutorelease(z);
		return 0;
	}
	int secondsFromGMTForDate(LDate *date);
	int secondsFromGMT();
	LString *name() const;

	CFTimeZoneRef _cf;

	static LTimeZone *sDefaultTimeZone;
	static LArray *sNames;
};

class LDate : public LObject {
	LF_DECLARE_CLASS(LDate)
public:
	LDate() : _timeZone(0), _time(0) { memset(&_date, 0, sizeof _date); }
	virtual ~LDate() {}
	virtual LDate *init();	// now
	// Now.
	static LDate *date()
	{
		LDate *d = _alloc();
		if (d && d->init())
			return LAutorelease(d);
		return 0;
	}
	virtual LDate *copy() const;
	virtual LString *description() const;
	virtual void dealloc();
	virtual LString *_plistRepresentation(unsigned int indent) const;
	virtual LDate *getOwnerLObject() { return this; }
	virtual LDate *initWithDate(const LDate *date);

	// Factories (autoreleased)
	static LDate *referenceDate();
	static LDate *dateWithTime(int year, int month, int day, int hour, int minute, int second, LTimeZone *zone = 0);
	static LDate *dateWithTimeIntervalSinceReferenceDate(double seconds);
	static LDate *dateWithTimeIntervalSince1970(double seconds);
	static double currentTimeIntervalSince1970();

	// Days past the end of a month roll over into the next months.
	LDate *initWithTime(int year, int month, int day, int hour, int minute, int second, LTimeZone *zone = 0);
	LDate *initWithTimeIntervalSinceReferenceDate(double seconds);
	LDate *initWithTimeIntervalSince1970(double seconds);

	int year() const { return _date.year; }
	int month() const { return _date.month; }
	int day() const { return _date.day; }
	int hour() const { return _date.hour; }
	int minute() const { return _date.minute; }
	double second() const { return _date.second; }
	// 0 is Sunday.
	int dayOfWeek() const;
	int dayOfYear() const;
	double timeIntervalSinceReferenceDate() const { return _time; }
	double timeIntervalSince1970() const { return _time + kCFAbsoluteTimeIntervalSince1970; }
	double timeIntervalSinceDate(const LDate *date) const { return _time - date->_time; }
	LTimeZone *timeZone() const { return _timeZone; }
	// 0 for the default zone.
	void setTimeZone(LTimeZone *zone);
	LDate *dateByAddingTime(int years, int months, int days, int hours, int minutes, int seconds) const;
	// strftime-style.
	LString *descriptionWithCalendarFormat(const LString *format) const;
	LString *localizedShortDateDescription() const;

	void _updateDate();

	LTimeZone *_timeZone;
	CFGregorianDate _date;
	CFAbsoluteTime _time;
};

} // namespace LF

#pragma clang diagnostic pop
