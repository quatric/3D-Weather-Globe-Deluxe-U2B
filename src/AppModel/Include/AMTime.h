// The time of the globe and the solar system at that time: the planets
// (VSOP87 series from the stars database, or Kepler orbits), the Moon (the
// ELP-2000/82 series in the "elp.bin" resource) and the world time, which is
// the current time, a chosen time or a time played forward or back.

#ifndef AMTIME_H
#define AMTIME_H

#include "AMDatabase.h"

// The ELP-2000/82 data: the polynomial of the mean longitude, then per
// coordinate (longitude, latitude, distance) and kind of term, where the
// terms are and how many there are.
struct SELPHeader {
	double polynomials[3][5];	// [0] is W1: t^0..t^4, in radians
	char unknown[0x90];
	int offsets[12][3];	// of the terms, from the header
	int counts[12][3];
};

class UPlanets {
public:
	enum TPlanet { kMercury, kVenus, kEarth, kMars, kJupiter, kSaturn, kUranus, kNeptune, kPluto, kPlanetsCount };
	enum TVSOPParam { kVSOPLongitude, kVSOPLatitude, kVSOPRadius };

	// A Kepler orbit (AU and degrees) and the start of a year as a fraction
	// of the period since 1970.
	struct SPlanetOrbit {
		double semiMajorAxis;
		double eccentricity;
		double inclination;
		double ascendingNode;
		double perihelion;
		double yearStart;
	};

	struct SVSOPEntry {
		double amplitude;
		double phase;
		double frequency;
	};

	// Seconds since 2000-01-01 12:00 UTC.
	static double j2000Time(LDate *date);
	static const SPlanetOrbit *planetOrbit(TPlanet planet);
	static unsigned int planetColor(TPlanet planet);
	static float planetMagnitude(Vector3 &planet, Vector3 &observer, TPlanet which);
	static double moonPeriod(LDate *date);
	// The true anomaly at a phase (0..1) of the year, from Kepler's equation.
	static double angleFromYearPhase(double phase, double eccentricity);
	static double distanceFromAngle(double angle, double semiMajorAxis, double eccentricity);
	// Seconds.
	static double period(const SPlanetOrbit *orbit);
	static double period(TPlanet planet);
	static double normalizedYearPhase(LDate *date, const SPlanetOrbit *orbit);
	static double normalizedYearPhase(LDate *date, TPlanet planet);
	// Heliocentric ecliptic coordinates in AU.
	static void planetPosition(TPlanet planet, LDate *date, double *x, double *y, double *z);
	static void moonOrbit(LDate *date, SPlanetOrbit *orbit);
	// Geocentric, in kilometers.
	static void moonPosition(LDate *date, double *x, double *y, double *z);
	static void _positionFromOrbit(LDate *date, const SPlanetOrbit *orbit, double *x, double *y, double *z);
	static TArray<SVSOPEntry> *_vsopData(TPlanet planet, TVSOPParam param, int power);
	static double _planetVSOPParam(TPlanet planet, TVSOPParam param, LDate *date);

	static unsigned int _sPlanetsColors[16];
	static float _sPlanetsConstants[16];
	static StPointerBlock *_sELPData;
	static CHashTable *_sVSOPArrays;
};

class TimeController : public LObject {
	LF_DECLARE_CLASS(TimeController)
public:
	static LString *kWorldTimeWasChanged;
	static LString *kPlanetsParamsWereChanged;

	TimeController();
	virtual void initAsSingleton(void *param);
	virtual void dealloc();
	virtual TimeController *getOwnerLObject() { return this; }
	virtual intptr_t _updateCurrentDate(char *args);

	static int daysCountInMonthOfYear(int month, int year);
	static float normalizedDayTimeFromDate(LDate *date);

	LDate *date() const { return _date; }
	int year() const { return _year; }
	int dayOfYear() const { return _dayOfYear; }
	float normalizedDayTime() const { return _dayTime; }
	bool useCustomTime() const { return _useCustomTime; }
	bool isPlaying() const { return _playing; }
	int speed() const { return _speed; }
	float playingSpeed() const { return _playingSpeed; }
	int playingDirection() const { return _direction; }

	void invalidateTimer();
	const Vector3 *planetPosition(UPlanets::TPlanet planet);
	const Vector3 *sunPosition();
	const Vector3 *sunDirection();
	const Vector3 *moonPosition();
	const Vector3 *moonDirection();
	bool canSpeedUp();
	bool canSpeedDown();
	bool canResetSpeed();
	void speedUp();
	void speedDown();
	void resetSpeed();
	void setPlayingSpeed(float speed);
	void setPlayingDirection(int direction);
	void setDate(LDate *date);
	void setDayMonthYearFromDate(LDate *date);
	void setDayOfYear(int day);
	void setNormalizedDayTime(float time);
	void setYear(int year);
	void setTimeZone(LTimeZone *zone);
	void setUseCustomTime(unsigned char custom);
	void play();
	void stop();

	void _worldTimeWasChanged();
	void _calculatePlanetsParams(UPlanets::TPlanet planet);
	void _calculateSunDirection();
	void _calculateMoonDirection();
	void _setPlayingSpeed();
	void _evalPlayingStartDate();
	void _evalCurrentDate();
	void _evalCustomTime();
	void _setupTimer();

	LTimer *_timer;
	LDate *_date;
	LTimeZone *_timeZone;
	bool _useCustomTime;
	int _year;
	float _dayTime;	// 0..1
	int _dayOfYear;
	bool _playing;
	int _speed;	// -10..10, not 0
	float _playingSpeed;	// 0..1
	int _direction;	// 1 or -1
	LDate *_playStartDate;
	double _playStartTime;
	bool _sunValid;
	bool _moonValid;
	Vector3 _sunPosition;
	Vector3 _sunDirection;
	Vector3 _moonPosition;
	Vector3 _moonDirection;
	Vector3 _planetPositions[UPlanets::kPlanetsCount];
	bool _planetValid[UPlanets::kPlanetsCount];
};

#endif
