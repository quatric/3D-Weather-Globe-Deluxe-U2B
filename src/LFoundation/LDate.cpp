// LDate
//
// LF::LDate: a CFAbsoluteTime with its CFGregorianDate in a time zone (the
// default zone unless set). The Gregorian-date calls are deprecated but
// still in CoreFoundation.

#include "LDate.h"
#include <wchar.h>
#include <time.h>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace LF {

LF_IMPLEMENT_CLASS(LDate, 0)

static LConstantString kStdDateDescr("%Y-%b-%d, %H:%M:%S");

static const int kDaysPerMonth365[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const int kDaysPerMonth366[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

void LDate::_updateDate()
{
	_date = CFAbsoluteTimeGetGregorianDate(_time, _timeZone->_cf);
}

void LDate::dealloc()
{
	if (_timeZone) {
		_timeZone->release();
		_timeZone = 0;
	}
	LObject::dealloc();
}

int LDate::dayOfWeek() const
{
	int day = (int)CFAbsoluteTimeGetDayOfWeek(_time, _timeZone->_cf);
	return day == 7 ? 0 : day;
}

int LDate::dayOfYear() const
{
	return (int)CFAbsoluteTimeGetDayOfYear(_time, _timeZone->_cf);
}

double LDate::currentTimeIntervalSince1970()
{
	return CFAbsoluteTimeGetCurrent() + kCFAbsoluteTimeIntervalSince1970;
}

void LDate::setTimeZone(LTimeZone *zone)
{
	if (zone == 0) {
		zone = LTimeZone::defaultTimeZone();
		if (zone == 0)
			LF_RAISE(NilPtrException());
	}
	LRetain(zone);
	if (_timeZone)
		_timeZone->release();
	_timeZone = zone;
	_updateDate();
}

// The seconds are written as an int.
LString *LDate::_plistRepresentation(unsigned int) const
{
	return LString::stringWithFormat("<date>%04d-%02d-%02dT%02d:%02d:%02dZ</date>", (int)_date.year, (int)_date.month,
	                                 (int)_date.day, (int)_date.hour, (int)_date.minute, (int)_date.second);
}

LDate *LDate::init()
{
	_timeZone = LRetain(LTimeZone::defaultTimeZone());
	_time = CFAbsoluteTimeGetCurrent();
	_updateDate();
	return this;
}

LDate *LDate::initWithDate(const LDate *date)
{
	if (LObject::init() == 0)
		return 0;
	_timeZone = LRetain(date->_timeZone);
	_time = date->_time;
	_date = date->_date;
	return this;
}

LDate *LDate::initWithTime(int year, int month, int day, int hour, int minute, int second, LTimeZone *zone)
{
	if (day < 1)
		day = 1;
	if (month < 1)
		month = 1;
	else if (month > 12)
		month = 12;
	if (zone == 0)
		zone = LTimeZone::defaultTimeZone();
	_timeZone = LRetain(zone);
	// Whole four-year cycles, then years, then months.
	day--;
	if (day / 1461 != 0) {
		year += (day / 1461) * 4;
		day %= 1461;
	}
	for (day++; day > 366; year++)
		day -= (year & 3) == 0 ? 366 : 365;
	for (;;) {
		const int *days = (year & 3) == 0 ? kDaysPerMonth366 : kDaysPerMonth365;
		int m = month - 1;
		if (day <= days[m])
			break;
		month++;
		day -= days[m];
		if (month > 12) {
			year++;
			month = 1;
		}
	}
	_date.second = second;
	_date.minute = (SInt8)minute;
	_date.hour = (SInt8)hour;
	_date.day = (SInt8)(day > 0 ? day : 1);
	_date.month = (SInt8)(month > 0 ? month : 1);
	_date.year = year;
	_time = CFGregorianDateGetAbsoluteTime(_date, _timeZone->_cf);
	return this;
}

LDate *LDate::initWithTimeIntervalSinceReferenceDate(double seconds)
{
	_timeZone = LRetain(LTimeZone::defaultTimeZone());
	_time = seconds;
	_updateDate();
	return this;
}

LDate *LDate::initWithTimeIntervalSince1970(double seconds)
{
	return initWithTimeIntervalSinceReferenceDate(seconds - kCFAbsoluteTimeIntervalSince1970);
}

LString *LDate::localizedShortDateDescription() const
{
	CFDateRef date = CFDateCreate(kCFAllocatorDefault, _time);
	CFLocaleRef locale = CFLocaleCopyCurrent();
	CFDateFormatterRef formatter = CFDateFormatterCreate(kCFAllocatorDefault, locale, kCFDateFormatterShortStyle,
	                                                     kCFDateFormatterNoStyle);
	CFStringRef str = CFDateFormatterCreateStringWithDate(kCFAllocatorDefault, formatter, date);
	LString *result = LString::stringWithCFString(str);
	CFRelease(str);
	CFRelease(formatter);
	CFRelease(locale);
	CFRelease(date);
	return result;
}

LDate *LDate::dateByAddingTime(int years, int months, int days, int hours, int minutes, int seconds) const
{
	LDate *date = _alloc();
	date->_timeZone = LRetain(_timeZone);
	CFGregorianUnits units = { years, months, days, hours, minutes, (double)seconds };
	date->_time = CFAbsoluteTimeAddGregorianUnits(_time, _timeZone->_cf, units);
	date->_updateDate();
	return LAutorelease(date);
}

LDate *LDate::referenceDate()
{
	LDate *date = _alloc();
	date->setTimeZone(LTimeZone::systemTimeZone(true));
	return LAutorelease(date);
}

LDate *LDate::dateWithTime(int year, int month, int day, int hour, int minute, int second, LTimeZone *zone)
{
	return LAutorelease(_alloc()->initWithTime(year, month, day, hour, minute, second, zone));
}

LDate *LDate::dateWithTimeIntervalSinceReferenceDate(double seconds)
{
	return LAutorelease(_alloc()->initWithTimeIntervalSinceReferenceDate(seconds));
}

LDate *LDate::dateWithTimeIntervalSince1970(double seconds)
{
	return LAutorelease(_alloc()->initWithTimeIntervalSinceReferenceDate(seconds - kCFAbsoluteTimeIntervalSince1970));
}

LDate *LDate::copy() const
{
	return _alloc()->initWithDate(this);
}

// Through wcsftime, so the format is widened to wchar_t and back.
LString *LDate::descriptionWithCalendarFormat(const LString *format) const
{
	unsigned int n = format->length() + 200;
	StPointerBlock out(n * sizeof(wchar_t), false);
	struct tm t;
	memset(&t, 0, sizeof t);
	t.tm_year = _date.year - 1900;
	t.tm_mon = _date.month - 1;
	t.tm_mday = _date.day;
	t.tm_hour = _date.hour;
	t.tm_min = _date.minute;
	t.tm_sec = (int)_date.second;
	t.tm_wday = dayOfWeek();
	StPointerBlock wide(n * sizeof(wchar_t), false);
	wchar_t *w = (wchar_t *)wide.bytes();
	const UniChar *src = format->chars();
	unsigned int len = format->length();
	for (unsigned int i = 0; i < len; i++)
		w[i] = src[i];
	w[len] = 0;
	size_t written = wcsftime((wchar_t *)out.bytes(), n, w, &t);
	if (written == 0)
		return LString::stringWithCString("", 0, kLMacRomanEncoding);
	UniChar *chars = (UniChar *)wide.bytes();
	const wchar_t *res = (const wchar_t *)out.bytes();
	for (size_t i = 0; i <= written; i++)
		chars[i] = (UniChar)res[i];
	return LString::stringWithCharacters(chars, (unsigned int)written);
}

LString *LDate::description() const
{
	return descriptionWithCalendarFormat(&kStdDateDescr);
}

} // namespace LF
