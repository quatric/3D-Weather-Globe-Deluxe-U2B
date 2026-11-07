// UPlanets: positions of the planets and the Moon. The planets come from the
// VSOP87 series in the stars database (heliocentric, ecliptic of the date),
// the Moon from the ELP-2000/82 series in the "elp.bin" resource; Kepler
// orbits are kept for the orbit lines.

#include "AppModel.h"

// Seconds from 1970 to 2000-01-01 12:00 UTC (J2000).
static const double kJ2000Since1970 = 946728000.0;
static const double kEarthPeriod = 31556927.8;
static const double kMoonPeriod = 2360591.56;

static const UPlanets::SPlanetOrbit sOrbits[UPlanets::kPlanetsCount] = {
	{ 0.38709893, 0.20563069, 7.00487, 48.33167, 77.45645, 0.0 },
	{ 0.72333199, 0.00677323, 3.39471, 76.68069, 131.53298, 0.0 },
	{ 1.00000011, 0.01671022, 5e-05, -11.26064, 102.94719, 0.010551 },
	{ 1.52366231, 0.09341233, 1.85061, 49.57854, 336.04084, 0.0 },
	{ 5.20336301, 0.04839266, 1.3053, 100.55615, 14.75385, 0.0 },
	{ 9.53707032, 0.0541506, 2.48446, 113.71504, 92.43194, 0.0 },
	{ 19.19126393, 0.04716771, 0.76986, 74.22988, 170.96424, 0.0 },
	{ 30.06896348, 0.00858587, 1.76917, 131.72169, 44.97135, 0.0 },
	{ 39.48168677, 0.24880766, 17.14175, 110.30347, 224.06676, 0.0 },
};

unsigned int UPlanets::_sPlanetsColors[16] = { 0xff33669a, 0xff82b6ce, 0xff7f2b2b, 0xff4c7297, 0xff6eaae1,
	                                           0xff86c6f6, 0xffc5ba9a, 0xffe2825e, 0xffffffff };
// The absolute magnitudes.
float UPlanets::_sPlanetsConstants[16] = { 1.954f, -2.79f, 0.0f, 0.202f, -7.7f, -7.31f, -5.79f, -5.66f };
StPointerBlock *UPlanets::_sELPData = 0;
CHashTable *UPlanets::_sVSOPArrays = new CHashTable;

double UPlanets::j2000Time(LDate *date)
{
	return date ? date->timeIntervalSince1970() - kJ2000Since1970 : 0.0;
}

const UPlanets::SPlanetOrbit *UPlanets::planetOrbit(TPlanet planet)
{
	return &sOrbits[planet];
}

unsigned int UPlanets::planetColor(TPlanet planet)
{
	return _sPlanetsColors[planet];
}

float UPlanets::planetMagnitude(Vector3 &planet, Vector3 &observer, TPlanet which)
{
	return ::planetMagnitude(planet, observer, _sPlanetsConstants[which]);
}

double UPlanets::moonPeriod(LDate *)
{
	return kMoonPeriod;
}

// Solves Kepler's equation E - e sin E = M: finds a quarter turn where the
// sign changes, then bisects.
double UPlanets::angleFromYearPhase(double phase, double e)
{
	double m = phase * M_PI + phase * M_PI;
	double end = m + e;
	double a = m - e, b, fa, fb;
	for (;;) {
		if (end <= a)
			return 0.0;
		double next = floor((a + M_PI_2) / M_PI_2) * M_PI_2;
		b = next <= end ? next : end;
		fa = (a - m) - sin(a) * e;
		fb = (b - m) - sin(b) * e;
		if (!((fb < 0 && fa < 0) || (0 < fb && 0 < fa)))
			break;
		a = next;
	}
	double mid = a;
	for (int i = 0; i < 50; i++) {
		mid = (b + a) * 0.5;
		double fm = (mid - m) - sin(mid) * e;
		if ((0 < fm && 0 < fb) || (fm < 0 && fb < 0)) {
			b = mid;
			fb = fm;
		} else {
			a = mid;
		}
	}
	double v = atan(sqrt((e + 1.0) / (1.0 - e)) * tan(mid * 0.5));
	return v + v;
}

double UPlanets::distanceFromAngle(double angle, double semiMajorAxis, double e)
{
	return ((1.0 - e * e) * semiMajorAxis) / (e * cos(angle) + 1.0);
}

// ---------------------------------------------------------------------------
// ELP-2000/82. t is in Julian centuries from J2000; the series give the
// longitude and latitude in arc seconds and the distance in kilometers,
// then Laskar's precession takes them to the ecliptic of J2000.

static void _calcAddition(const SELPHeader *h, int kind, int coordinate, double *sums, const double *t)
{
	int count = h->counts[kind][coordinate];
	const double *e = (const double *)((const char *)h + h->offsets[kind][coordinate]);
	double sum = sums[coordinate];
	if (kind == 0) {
		for (int i = 0; i < count; i++, e += 6)
			sum += sin(e[2] * t[1] + e[1] + e[3] * t[2] + e[4] * t[3] + e[5] * t[4]) * e[0];
	} else {
		double factor = kind == 11 ? t[2] : (kind == 2 || kind == 4 || kind == 6 || kind == 8) ? t[1] : 1.0;
		for (int i = 0; i < count; i++, e += 3)
			sum += e[0] * factor * sin(e[2] * t[1] + e[1]);
	}
	sums[coordinate] = sum;
}

static void compute_elp_xyz(StPointerBlock *data, double t, double *x, double *y, double *z)
{
	const SELPHeader *h = (const SELPHeader *)data->bytes();
	double powers[5] = { 0, t, t * t, t * t * t, t * t * t * t };
	double sums[3] = { 0, 0, 0 };
	for (int coordinate = 0; coordinate < 3; coordinate++)
		for (int kind = 0; kind < 12; kind++)
			_calcAddition(h, kind, coordinate, sums, powers);
	const double *w1 = h->polynomials[0];
	double longitude = sums[0] / 206264.80624709636 + w1[0] + w1[1] * powers[1] + w1[2] * powers[2] +
	                   w1[3] * powers[3] + w1[4] * powers[4];
	double latitude = sums[1] / 206264.80624709636;
	double r = (sums[2] * 384747.9806448954) / 384747.9806743165;
	double x0 = r * cos(longitude) * cos(latitude);
	double y0 = sin(longitude) * r * cos(latitude);
	double z0 = r * sin(latitude);
	double p = ((((t * 4.63486e-15 - 2.507948e-12) * t - 5.417367e-10) * t + 4.7020439e-07) * t + 1.0180391e-05) * t;
	double q = ((((t * -3.20334e-15 - 1.371808e-12) * t + 1.265417e-09) * t + 1.2372674e-07) * t - 0.000113469002) * t;
	double pp = p * -2.0 * p + 1.0;
	double qq = q * -2.0 * q + 1.0;
	double pq = (p + p) * q;
	double s = sqrt((1.0 - p * p) - q * q);
	s += s;
	double ps = p * s, qs = q * s;
	*x = x0 * pp + y0 * pq + z0 * ps;
	*y = (pq * x0 + y0 * qq) - z0 * qs;
	*z = (y0 * qs - ps * x0) + ((pp + qq) - 1.0) * z0;
}

// The data is little-endian, like the hosts.
static void _swapLittleSELPDataToHost(SELPHeader *)
{
}

void UPlanets::moonPosition(LDate *date, double *x, double *y, double *z)
{
	if (_sELPData == 0) {
		LData *data = LData::_alloc()->initWithResource(LString::stringWithCString("elp"),
		                                                LString::stringWithCString("bin"), getAppModelModule());
		if (data) {
			_swapLittleSELPDataToHost((SELPHeader *)data->bytes());
			_sELPData = new StPointerBlock(data->bytes(), data->length());
			data->release();
		}
		if (_sELPData == 0)
			return;
	}
	compute_elp_xyz(_sELPData, j2000Time(date) / 3155752224.0000005, x, y, z);
}

// ---------------------------------------------------------------------------

double UPlanets::period(const SPlanetOrbit *orbit)
{
	double a = orbit->semiMajorAxis / sOrbits[kEarth].semiMajorAxis;
	return sqrt(a * a * a) * kEarthPeriod;
}

double UPlanets::normalizedYearPhase(LDate *date, const SPlanetOrbit *orbit)
{
	double phase = date->timeIntervalSince1970() / period(orbit) - orbit->yearStart;
	return phase - floor(phase);
}

double UPlanets::period(TPlanet planet)
{
	return period(&sOrbits[planet]);
}

double UPlanets::normalizedYearPhase(LDate *date, TPlanet planet)
{
	return normalizedYearPhase(date, &sOrbits[planet]);
}

static int __vsop_cb(int, int *types, sqlite3_stmt *stmt, void *context)
{
	UPlanets::SVSOPEntry entry;
	entry.amplitude = SQLiteDatabase::checkAndGetDouble(0, types[0], stmt);
	if (1e-07 <= fabs(entry.amplitude)) {
		entry.phase = SQLiteDatabase::checkAndGetDouble(1, types[1], stmt);
		entry.frequency = SQLiteDatabase::checkAndGetDouble(2, types[2], stmt);
		((TArray<UPlanets::SVSOPEntry> *)context)->addObject(entry);
	}
	return 0;
}

// One power of a series, read once (0 when there is none).
TArray<UPlanets::SVSOPEntry> *UPlanets::_vsopData(TPlanet planet, TVSOPParam param, int power)
{
	intptr_t key = param << 4 | planet | power << 6;
	intptr_t cached = 0;
	if (_sVSOPArrays->getValueForKey(&cached, key))
		return (TArray<SVSOPEntry> *)cached;
	LAutoreleasePool::addNewPoolToTop();
	TArray<SVSOPEntry> *entries = 0;
	LString *request = SQL_DB_Strings::stars_selectVSOPBlock(planet, param, power);
	if (request) {
		entries = new TArray<SVSOPEntry>;
		AtlasDatabase::instance()->sqlDB()->processRequest(request, __vsop_cb, entries);
		if (entries->count() == 0) {
			delete entries;
			entries = 0;
		}
	}
	_sVSOPArrays->setValueForKey((intptr_t)entries, key);
	LAutoreleasePool::removeTopPool();
	return entries;
}

// The sum of the series' powers of t (Julian millennia from J2000).
double UPlanets::_planetVSOPParam(TPlanet planet, TVSOPParam param, LDate *date)
{
	double t = j2000Time(date) / 31557600000.0;
	double value = 0.0, tn = 1.0;
	for (int power = 0; power < 4; power++) {
		TArray<SVSOPEntry> *entries = _vsopData(planet, param, power);
		if (entries == 0)
			continue;
		double sum = 0.0;
		for (unsigned int i = 0; i < entries->count(); i++) {
			const SVSOPEntry &e = (*entries)[i];
			sum += cos(t * e.frequency + e.phase) * e.amplitude;
		}
		value = sum * tn + value;
		tn *= t;
	}
	return value;
}

void UPlanets::planetPosition(TPlanet planet, LDate *date, double *x, double *y, double *z)
{
	double l = _planetVSOPParam(planet, kVSOPLongitude, date);
	double b = _planetVSOPParam(planet, kVSOPLatitude, date);
	double r = _planetVSOPParam(planet, kVSOPRadius, date);
	if (x)
		*x = cos(l) * cos(b) * r;
	if (y)
		*y = sin(l) * cos(b) * r;
	if (z)
		*z = r * sin(b);
}

// The Moon's mean orbit at a date (Chapront), in the orbit's units.
void UPlanets::moonOrbit(LDate *date, SPlanetOrbit *orbit)
{
	if (orbit == 0)
		return;
	double t = j2000Time(date) / 3155760000.0;
	double t2 = t * t, t3 = t * t2, t4 = t2 * t2;
	orbit->eccentricity = 0.05554553;
	orbit->semiMajorAxis = ((t * 0.0038 + 383397.7916) * 0.0025645) / 384000.0;
	orbit->inclination = (t * -8e-05 + t2 * 0.02966 + t3 * -4.2e-05 + t4 * -1.3e-07) / 3600.0 + 5.15668983;
	orbit->perihelion = (t * 14643420.2632 + t2 * -38.2776 + t3 * -0.045047 + t4 * 0.00021301) / 3600.0 + 83.35324299;
	orbit->ascendingNode = (t * -6967919.3622 + t2 * 6.3622 + t3 * 0.007625 + t4 * -3.586e-05) / 3600.0 + 125.04455504;
	orbit->yearStart =
	    (((t2 * 5.8883 + t3 * 0.006604 + t4 * -3.169e-05) / -3600.0 + 218.316654) * -kMoonPeriod) / 360.0;
}

// A position on a Kepler orbit, with the Moon's period.
void UPlanets::_positionFromOrbit(LDate *date, const SPlanetOrbit *orbit, double *x, double *y, double *z)
{
	double t = (j2000Time(date) - orbit->yearStart) / kMoonPeriod;
	double v = angleFromYearPhase(t - floor(t), orbit->eccentricity);
	double r = distanceFromAngle(v, orbit->semiMajorAxis, orbit->eccentricity);
	double node = (orbit->ascendingNode * M_PI) / 180.0;
	double inclination = (orbit->inclination * M_PI) / 180.0;
	double u = (orbit->perihelion * M_PI) / 180.0 + v;
	double cosNode = cos(node), sinNode = sin(node), cosU = cos(u), sinU = sin(u);
	double cosI = cos(inclination), sinI = sin(inclination);
	if (x)
		*x = (cosNode * cosU - sinNode * sinU * cosI) * r;
	if (y)
		*y = (cosU * sinNode + cosI * cosNode * sinU) * r;
	if (z)
		*z = sinI * sinU * r;
}
