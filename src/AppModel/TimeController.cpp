// TimeController: the world time. It follows the current time (checked every
// second), or is a custom date (year, day of the year and time of the day in
// a time zone), which can be played forward or back at 21 speeds: up to 4
// hours per second in minutes, then up to 68 days per second in days.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(TimeController, 0)
LF_SELECTOR(TimeController, _updateCurrentDate);

// Minutes per second for the first 11 speeds, days per second for the rest.
static const int sPlayingSpeedFactor[21] = { 1, 3, 9, 15, 21, 30, 45, 60, 90, 150, 240,
	                                         5, 8, 12, 17, 23, 30, 38, 47, 57, 68 };
static const int sDaysInMonth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int TimeController::daysCountInMonthOfYear(int month, int year)
{
	if (month == 2 && (year & 3) == 0)
		return 29;
	return sDaysInMonth[month];
}

float TimeController::normalizedDayTimeFromDate(LDate *date)
{
	return ((float)date->hour() * 3600.0f + (float)date->minute() * 60.0f + (float)(int)date->second()) / 86400.0f;
}

TimeController::TimeController()
    : _timer(0), _date(0), _timeZone(0), _useCustomTime(false), _year(0), _dayTime(0), _dayOfYear(0), _playing(false),
      _speed(1), _playingSpeed(0), _direction(1), _playStartDate(0), _playStartTime(0), _sunValid(false),
      _moonValid(false), _sunPosition(0, 0, 0), _sunDirection(1, 0, 0), _moonPosition(0, 0, 0),
      _moonDirection(1, 0, 0)
{
	memset(_planetPositions, 0, sizeof(_planetPositions));
	memset(_planetValid, 0, sizeof(_planetValid));
}

void TimeController::dealloc()
{
	if (_playStartDate) {
		_playStartDate->release();
		_playStartDate = 0;
	}
	if (_timeZone) {
		_timeZone->release();
		_timeZone = 0;
	}
	if (_date) {
		_date->release();
		_date = 0;
	}
	LObject::dealloc();
}

void TimeController::invalidateTimer()
{
	if (_timer) {
		_timer->invalidate();
		_timer = 0;
	}
}

void TimeController::_worldTimeWasChanged()
{
	memset(_planetValid, 0, sizeof(_planetValid));
	_sunValid = false;
	_moonValid = false;
	LNotificationCenter::center()->postNotification(kWorldTimeWasChanged, this, 0);
	LNotificationCenter::center()->postNotification(kPlanetsParamsWereChanged, this, 0);
}

void TimeController::_calculatePlanetsParams(UPlanets::TPlanet planet)
{
	double x, y, z;
	UPlanets::planetPosition(planet, _date, &x, &y, &z);
	_planetValid[planet] = true;
	_planetPositions[planet] = Vector3((float)x, (float)y, (float)z);
}

const Vector3 *TimeController::planetPosition(UPlanets::TPlanet planet)
{
	if (!_planetValid[planet])
		_calculatePlanetsParams(planet);
	return &_planetPositions[planet];
}

// The speed from 1..10 or -1..-10 to a direction and 0..1.
void TimeController::_setPlayingSpeed()
{
	int sign = _speed >> 31;
	_direction = sign | 1;
	_playingSpeed = (float)(((_speed ^ sign) - sign) - 1) / 9.0f;
}

bool TimeController::canSpeedUp()
{
	return _speed < 10;
}

bool TimeController::canSpeedDown()
{
	return -10 < _speed;
}

bool TimeController::canResetSpeed()
{
	return _speed != 1;
}

void TimeController::_calculateSunDirection()
{
	_sunValid = true;
	_sunPosition = sunPosition_EarthSystem(_date);
	FHVec3Normalize(&_sunDirection, &_sunPosition);
}

const Vector3 *TimeController::sunPosition()
{
	if (!_sunValid)
		_calculateSunDirection();
	return &_sunPosition;
}

const Vector3 *TimeController::sunDirection()
{
	if (!_sunValid)
		_calculateSunDirection();
	return &_sunDirection;
}

void TimeController::_calculateMoonDirection()
{
	_moonValid = true;
	_moonPosition = moonPosition_EarthSystem(_date);
	FHVec3Normalize(&_moonDirection, &_moonPosition);
}

const Vector3 *TimeController::moonPosition()
{
	if (!_moonValid)
		_calculateMoonDirection();
	return &_moonPosition;
}

const Vector3 *TimeController::moonDirection()
{
	if (!_moonValid)
		_calculateMoonDirection();
	return &_moonDirection;
}

// Playing goes from the current date and time.
void TimeController::_evalPlayingStartDate()
{
	if (_playStartDate) {
		_playStartDate->release();
		_playStartDate = 0;
	}
	_playStartDate = LRetain(_date);
	_playStartTime = LTimer::currentTimeValue();
}

void TimeController::setPlayingSpeed(float speed)
{
	_playingSpeed = speed < 0 ? 0.0f : speed > 1.0f ? 1.0f : speed;
	if (_useCustomTime && _playing)
		_evalPlayingStartDate();
}

void TimeController::speedUp()
{
	if (_speed >= 10)
		return;
	_speed = _speed + 1 != 0 ? _speed + 1 : 1;
	_setPlayingSpeed();
	if (_useCustomTime && _playing)
		_evalPlayingStartDate();
}

void TimeController::speedDown()
{
	if (_speed <= -10)
		return;
	_speed = _speed - 1 != 0 ? _speed - 1 : -1;
	_setPlayingSpeed();
	if (_useCustomTime && _playing)
		_evalPlayingStartDate();
}

void TimeController::resetSpeed()
{
	_speed = 1;
	_direction = 1;
	_playingSpeed = 0;
	if (_useCustomTime && _playing)
		_evalPlayingStartDate();
}

void TimeController::_evalCurrentDate()
{
	if (_date) {
		_date->release();
		_date = 0;
	}
	if (!_useCustomTime) {
		_date = LDate::_alloc()->init();
	} else {
		// The day of the year goes in as a day of January.
		int seconds = (int)floorf(_dayTime * 86400.0f + 0.5f);
		_date = LDate::_alloc()->initWithTime(_year, 1, _dayOfYear, seconds / 3600, seconds % 3600 / 60,
		                                      seconds % 3600 % 60, _timeZone);
	}
	_worldTimeWasChanged();
}

void TimeController::_evalCustomTime()
{
	if (!_useCustomTime)
		return;
	_evalCurrentDate();
	if (_playing)
		_evalPlayingStartDate();
}

void TimeController::setDate(LDate *date)
{
	_year = date->year();
	_dayOfYear = date->dayOfYear();
	_dayTime = normalizedDayTimeFromDate(date);
	_evalCustomTime();
}

void TimeController::setDayMonthYearFromDate(LDate *date)
{
	_year = date->year();
	_dayOfYear = date->dayOfYear();
	_evalCustomTime();
}

void TimeController::setDayOfYear(int day)
{
	_dayOfYear = day;
	_evalCustomTime();
}

void TimeController::setNormalizedDayTime(float time)
{
	_dayTime = time;
	if (1.0f <= time)
		_dayTime = 0.9999f;	// 0x3f7ff972
	_evalCustomTime();
}

void TimeController::setYear(int year)
{
	_year = year;
	_evalCustomTime();
}

void TimeController::setTimeZone(LTimeZone *zone)
{
	LRetain(zone);
	if (_timeZone) {
		_timeZone->release();
		_timeZone = 0;
	}
	_timeZone = zone;
	_evalCustomTime();
}

// The timer: the current time, or the played time from where playing started.
intptr_t TimeController::_updateCurrentDate(char *)
{
	if (!_useCustomTime) {
		_evalCurrentDate();
		return 0;
	}
	if (_playStartDate == 0)
		return 0;
	int index = (int)(_playingSpeed * 20.0f);
	double elapsed = (LTimer::currentTimeValue() - _playStartTime) * (double)(_direction * sPlayingSpeedFactor[index]);
	if (index <= 10)
		elapsed *= 60.0;
	// Start again when the time no longer fits an int.
	if (!(elapsed > -2147483648.0 && elapsed < 2147483648.0)) {
		_evalPlayingStartDate();
		return 0;
	}
	if (_date) {
		_date->release();
		_date = 0;
	}
	if (10 < index)
		_date = LRetain(_playStartDate->dateByAddingTime(0, 0, (int)elapsed, 0, 0, 0));
	else
		_date = LDate::_alloc()->initWithTimeIntervalSinceReferenceDate((double)(int)elapsed +
		                                                                _playStartDate->timeIntervalSinceReferenceDate());
	if (_timeZone)
		_date->setTimeZone(_timeZone);
	_year = _date->year();
	_dayOfYear = _date->dayOfYear();
	_dayTime = normalizedDayTimeFromDate(_date);
	_worldTimeWasChanged();
	return 0;
}

// Every second for the current time, 30 times a second while playing.
void TimeController::_setupTimer()
{
	invalidateTimer();
	unsigned int interval;
	if (!_useCustomTime) {
		interval = 1000;
	} else {
		if (!_playing)
			return;
		_evalPlayingStartDate();
		interval = 0x21;
	}
	_timer = LTimer::timerWithParams(interval, this, "_updateCurrentDate", true, 0);
}

void TimeController::stop()
{
	_playing = false;
	if (_useCustomTime)
		_setupTimer();
}

void TimeController::play()
{
	_playing = true;
	if (_useCustomTime)
		_setupTimer();
}

void TimeController::setUseCustomTime(unsigned char custom)
{
	_useCustomTime = custom != 0;
	_evalCurrentDate();
	_setupTimer();
}

void TimeController::initAsSingleton(void *param)
{
	LObject::initAsSingleton(param);
	LDate *now = LDate::date();
	_year = now->year();
	_dayOfYear = now->dayOfYear();
	_dayTime = normalizedDayTimeFromDate(now);
	_evalCurrentDate();
	_setupTimer();
}

void TimeController::setPlayingDirection(int direction)
{
	_direction = direction >> 31 | 1;
	if (!_playing) {
		_playing = true;
		if (_useCustomTime)
			_setupTimer();
	}
	if (_useCustomTime)
		_evalPlayingStartDate();
}
