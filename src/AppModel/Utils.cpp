// Utils: values formatted for display in the units the preferences ask for,
// with the strings of the "Utils" table; date arithmetic; DDS encoding.

#include "AppModel.h"
#include "FHTextures.h"
#include <math.h>

static inline LString *localized(const char *key) { return LString::stringFromTable(key, "Utils", getAppModelModule()); }

LString *Utils::temperatureDescription(LObject *celsius)
{
	if (!celsius)
		return kEmptyString;
	float value = loadFloatFromObject(celsius);
	LString *format;
	if (!Preferences::usesMetricSystem()) {
		format = localized("TemperatureFahrenheit");
		value = fahrenheitFromCelsius(value);
	}
	else {
		format = localized("TemperatureCelcius");
	}
	return LString::stringWithFormat(format, (int)floorf(value + 0.5f));
}

LString *Utils::temperatureValueShortDescription(float value)
{
	return LString::stringWithFormat(localized("ShortTemperatureFormat"), (int)floorf(value + 0.5f));
}

LString *Utils::temperatureShortDescription(LObject *celsius)
{
	if (!celsius)
		return kEmptyString;
	float value = loadFloatFromObject(celsius);
	if (!Preferences::usesMetricSystem())
		value = fahrenheitFromCelsius(value);
	return temperatureValueShortDescription(value);
}

LString *Utils::speedDescription(LObject *kmh)
{
	if (!kmh)
		return kEmptyString;
	float value = loadFloatFromObject(kmh);
	LString *format;
	if (!Preferences::usesMetricSystem()) {
		format = localized("SpeedMiFormat");
		value = milesFromKilometers(value);
	}
	else {
		format = localized("SpeedKmFormat");
	}
	return LString::stringWithFormat(format, (double)value);
}

LString *Utils::percentsDescription(LObject *value)
{
	if (!value)
		return kEmptyString;
	return LString::stringWithFormat("%d%%", loadIntFromObject(value));
}

LString *Utils::pressureDescription(LObject *millibars)
{
	if (!millibars)
		return kEmptyString;
	float value = loadFloatFromObject(millibars);
	LString *format;
	if (!Preferences::usesMetricSystem()) {
		format = localized("PressureInches");
		value = inchesFromMiliBars(value);
	}
	else {
		format = localized("PressureMillimeters");
		value = millimetersFromMiliBars(value);
	}
	return LString::stringWithFormat(format, (double)value);
}

// The number, from 1, of the wind arrow image for a direction.
int Utils::windImageIndex(int angle)
{
	return (int)((((float)((angle + 180) % 360) / 10.0f) * 31.0f) / 35.0f + 0.5f) + 1;
}

LString *Utils::shortSingleCoordDescription(float value, unsigned char isLatitude, TGeolocFormat format)
{
	TDegrees d = degreesFromFloat(value);
	const char *part;
	if (isLatitude)
		part = d.isNegative ? "AbbrSouth" : "AbbrNorth";
	else
		part = d.isNegative ? "AbbrWest" : "AbbrEast";
	LString *partName = localized(part);
	if (format == kGeolocFormatSimple)
		return LString::stringWithFormat(localized("kGeolocFormatSimple"), (int)d.degrees, partName);
	if (format == kGeolocFormatMedium)
		return LString::stringWithFormat(localized("kGeolocFormatMedium"), (int)d.degrees, (int)d.minutes, partName);
	int seconds = (int)floorf(d.seconds + 0.5f);
	return LString::stringWithFormat(localized("kGeolocFormatFull"), (int)d.degrees, (int)d.minutes, seconds,
	                                 partName);
}

LString *Utils::shortGeoCoordinatesDescription(LPoint geoPoint, TGeolocFormat format)
{
	LString *s = shortSingleCoordDescription(geoPoint.y, 1, format);
	s->appendString(&__separator);
	s->appendString(shortSingleCoordDescription(geoPoint.x, 0, format));
	return s;
}

// Heights under a mile are given in feet, under ten kilometers in meters.
LString *Utils::shortHeightDesription(float km)
{
	if (!Preferences::usesMetricSystem()) {
		float miles = milesFromKilometers(km);
		if (miles <= 1.0f)
			return LString::stringWithFormat(localized("shortHeightDesriptionFeetFormat"),
			                                 (int)(feetsFromKilometers(km) + 0.5f));
		return LString::stringWithFormat(localized("shortHeightDesriptionMiFormat"), (double)miles);
	}
	if (km <= 10.0f)
		return LString::stringWithFormat(localized("shortHeightDesriptionMetersFormat"), (int)(km * 1000.0f + 0.5f));
	return LString::stringWithFormat(localized("shortHeightDesriptionKmFormat"), (double)km);
}

// A DDS file of an A8R8G8B8 bitmap, converted to `format` unless that is
// what it is.
LData *Utils::ddsDataWithFormatFromBitmap(_D3DFORMAT format, const void *bits, LSize size, unsigned int bytesPerRow)
{
	unsigned int width = size.width > 0.0f ? (unsigned int)size.width : 0;
	unsigned int height = size.height > 0.0f ? (unsigned int)size.height : 0;
	LData *argb = ImageDecompressor::ddsHeaderForFormatAndSize(D3DFMT_A8R8G8B8, width, height);
	argb->appendBytes(bits, height * bytesPerRow);
	if (format == D3DFMT_A8R8G8B8)
		return argb;
	ImageDecompressor *decompressor = ImageDecompressor::decompressorForData(argb);
	if (!decompressor)
		return 0;
	unsigned int bitmapSize = FHGetSurfaceSize(width, height, format);
	LData *dds = ImageDecompressor::ddsHeaderForFormatAndSize(format, width, height);
	unsigned int headerSize = dds->length();
	dds->setLength(bitmapSize + headerSize);
	decompressor->decompressToBitmapWithFormat(format, (char *)dds->bytes() + headerSize);
	return dds;
}

LData *Utils::ddsDataWithFormatFromBitmapImage(_D3DFORMAT format, LBitmapImage *image)
{
	LSize size = image->size();
	LBitmapImage::StLocker locker(image, LBitmapImage::StLocker::kRead, (TPixelFormat)0);
	return ddsDataWithFormatFromBitmap(format, locker.bitmapData(), size, locker.bytesPerRow());
}

LString *Utils::geoCoordinatesDegreeDelimiter() { return localized("GeoCoordinatesDegreeDelimiter"); }

LString *Utils::geoCoordinatesMinuteDelimiter() { return localized("GeoCoordinatesMinuteDelimiter"); }

LString *Utils::geoCoordinatesSecondDelimiter() { return localized("GeoCoordinatesSecondDelimiter"); }

LString *Utils::solarEclipseTypeDescription(const SSolarEclipse *eclipse)
{
	if (!eclipse)
		return 0;
	switch (eclipse->type) {
	case 'H':
		return localized("SOLAR_ECLIPSE_TYPE_HYBRID");
	case 'A':
		return localized("SOLAR_ECLIPSE_TYPE_ANNULAR");
	case 'P':
		return localized("SOLAR_ECLIPSE_TYPE_PARTIAL");
	case 'T':
		return localized("SOLAR_ECLIPSE_TYPE_TOTAL");
	default:
		return 0;
	}
}

// Calendar days from one date to the other, negative when `to` is earlier.
int Utils::daysIntervalBetweenDates(LDate *from, LDate *to)
{
	double interval = to->timeIntervalSinceReferenceDate() - from->timeIntervalSinceReferenceDate();
	if (interval == 0.0)
		return 0;
	LDate *first = interval > 0.0 ? from : to;
	LDate *last = interval >= 0.0 ? to : from;
	int year = first->year();
	int lastYear = last->year();
	int firstDay = first->dayOfYear();
	int lastDay = last->dayOfYear();
	int days = year != lastYear ? lastDay : lastDay - firstDay;
	for (; year < lastYear; year++) {
		days += ((year & 3) == 0 ? 366 : 365) - firstDay;
		firstDay = 0;
	}
	return interval < 0.0 ? -days : days;
}

LDate *Utils::dateByAddingMinutesInterval(LDate *date, int minutes)
{
	int hour = date->hour() + (date->minute() + minutes) / 60;
	int minute = (date->minute() + minutes) % 60;
	if (minute < 0) {
		minute += 60;
		hour--;
	}
	int days = hour / 24;
	hour = hour % 24;
	if (hour < 0) {
		hour += 24;
		days--;
	}
	if (days != 0)
		date = date->dateByAddingTime(0, 0, days, 0, 0, 0);
	return LDate::dateWithTime(date->year(), 1, date->dayOfYear(), hour, minute, (int)date->second(), date->timeZone());
}

// Minutes from one date to the other, counted by their calendar fields.
int Utils::minutesIntervalBetweenDates(LDate *from, LDate *to)
{
	double interval = to->timeIntervalSinceReferenceDate() - from->timeIntervalSinceReferenceDate();
	if (interval == 0.0)
		return 0;
	LDate *first = interval > 0.0 ? from : to;
	LDate *last = interval >= 0.0 ? to : from;
	int year = first->year();
	int lastYear = last->year();
	int firstMinutes = first->dayOfYear() * 1440 + first->hour() * 60 - 1440 + first->minute();
	int lastMinutes = last->dayOfYear() * 1440 + last->hour() * 60 - 1440 + last->minute();
	int minutes = year != lastYear ? lastMinutes : lastMinutes - firstMinutes;
	for (; year < lastYear; year++) {
		minutes += ((year & 3) == 0 ? 1440 : 0) - firstMinutes + 525600;
		firstMinutes = 0;
	}
	return interval < 0.0 ? -minutes : minutes;
}

// Eclipse times are UT.
LDate *Utils::dateFromSolarEclipse(const SSolarEclipse *eclipse, LTimeZone *timeZone)
{
	if (!eclipse)
		return 0;
	LDate *date = LDate::dateWithTime(eclipse->year, eclipse->month, eclipse->day, eclipse->hour, eclipse->minute, 0,
	                                  LTimeZone::timeZoneForSecondsFromGMT(0));
	date->setTimeZone(timeZone);
	return date;
}

LString *Utils::solarEclipseTooltipDescription(const SSolarEclipse *eclipse, LTimeZone *timeZone)
{
	if (!eclipse)
		return 0;
	LString *type = solarEclipseTypeDescription(eclipse);
	LDate *date = dateFromSolarEclipse(eclipse, timeZone);
	LString *format = Preferences::uses24HourClock() ? &kSolarEclipseDateFormat24 : &kSolarEclipseDateFormat12;
	return LString::stringWithFormat("%@ %@", type, date->descriptionWithCalendarFormat(format));
}

LString *Utils::weekdayName(int day, unsigned char abbr)
{
	if (day < 0)
		return kEmptyString;
	LDate *date = LDate::date();
	while (date->dayOfWeek() != day % 7)
		date = date->dateByAddingTime(0, 0, 1, 0, 0, 0);
	return date->descriptionWithCalendarFormat(abbr ? &kWeekdayAbbrFormat : &kWeekdayNameFormat);
}

LString *Utils::geoCoordinatesWorldPartCharacters(unsigned char negative)
{
	LString *key = LString::stringWithCString(negative ? "GeoCoordinatesWorldPartCharactersNegative"
	                                                   : "GeoCoordinatesWorldPartCharacters");
	return localized(key->cString());
}

// "1,234.56" with `fraction` decimals and `suffix` appended. The digits are
// written backwards from the end of a buffer.
LString *Utils::commaSeparatedNumericValue(double value, int fraction, const LString *suffix)
{
	static const UniChar digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	UniChar buffer[1000];
	int pos = 998;
	buffer[999] = 0;
	if (suffix) {
		int length = suffix->length();
		const UniChar *chars = suffix->wString();
		for (int i = length - 1; i >= 0; i--)
			buffer[pos--] = chars[i];
	}
	double unit = 10.0;
	double scale = 0.1;
	double power = 1.0;
	for (int i = 0; i < fraction; i++) {
		scale *= 10.0;
		unit *= 0.1;
		power *= 10.0;
	}
	double a = unit * 0.05 + fabs(value);
	int place = -fraction;
	for (;;) {
		if (power * a < 1.0 && place > 0) {
			if (value < 0.0)
				buffer[pos--] = '-';
			return LString::stringWithCharacters(buffer + pos + 1, 998 - pos);
		}
		if (fraction >= 1 && place == 0)
			buffer[pos--] = '.';
		else if (place > 0 && place % 3 == 0)
			buffer[pos--] = ',';
		double next = scale;
		int digit = (int)((a - floor(next * a) * unit) * power);
		unit *= 10.0;
		if (digit < 0)
			digit = 0;
		else if (digit > 9)
			digit = 9;
		place++;
		buffer[pos--] = digits[digit];
		scale = next * 0.1;
		power = next;
	}
}

LString *Utils::densityDescription(LObject *perSqKm)
{
	if (!perSqKm)
		return kEmptyString;
	double value = loadDoubleFromObject(perSqKm);
	LString *units;
	if (!Preferences::usesMetricSystem()) {
		units = localized("UnitsInResidentsPerSqMiles");
		value = value / 0.3861;
	}
	else {
		units = localized("UnitsInResidentsPerSqKm");
	}
	return commaSeparatedNumericValue(value, 2, units);
}

LString *Utils::areaDescription(LObject *sqKm)
{
	if (!sqKm)
		return kEmptyString;
	double value = loadDoubleFromObject(sqKm);
	LString *units;
	if (!Preferences::usesMetricSystem()) {
		units = localized("UnitsInSquareMiles");
		value = value * 0.3861;
	}
	else {
		units = localized("UnitsInSquareKilometers");
	}
	return commaSeparatedNumericValue(value, 2, units);
}

LString *Utils::smallDistanceDescription(LObject *meters)
{
	if (!meters)
		return kEmptyString;
	float value = loadFloatFromObject(meters);
	LString *units;
	if (!Preferences::usesMetricSystem()) {
		units = localized("UnitsInFeet");
		value = feetsFromKilometers(value * 0.001f);
	}
	else {
		units = localized("UnitsInMeters");
	}
	return commaSeparatedNumericValue((double)value, 2, units);
}

LString *Utils::distanceShortDescription(float km)
{
	if (!Preferences::usesMetricSystem())
		km = milesFromKilometers(km);
	return commaSeparatedNumericValue((double)km, 2, (const LString *)0);
}

LString *Utils::distanceDescription(float km)
{
	LString *units;
	if (!Preferences::usesMetricSystem()) {
		units = localized("UnitsInMiles");
		km = milesFromKilometers(km);
	}
	else {
		units = localized("UnitsInKilometers");
	}
	return commaSeparatedNumericValue((double)km, 2, units);
}

LString *Utils::distanceDescription(LObject *km)
{
	if (!km)
		return kEmptyString;
	return distanceDescription(loadFloatFromObject(km));
}

LString *Utils::commaSeparatedNumericValue(LObject *value, int fraction, const LString *suffix)
{
	return commaSeparatedNumericValue(loadDoubleFromObject(value), fraction, suffix);
}

// "Rain. Cloudy. Warm." from the report's descriptions, localized.
LString *Utils::weatherDescription(LMap *report)
{
	if (!report)
		return 0;
	LString *s = LString::string();
	LString *keys[] = {kReportPreciptationsDesc, kReportSkyDescription, kReportTemperatureDesc};
	for (LString *key : keys) {
		LString *part = (LString *)report->objectForKey(key);
		if (!part)
			continue;
		if (s->length() != 0)
			s->appendString(LString::stringWithCString(". "));
		s->appendString(LString::stringFromTable(part, "WeatherDescription", getAppModelModule()));
	}
	if (s->length() != 0)
		s->appendString(LString::stringWithCString("."));
	return s;
}

// "NE" and the like for a direction in degrees.
static LString *_windDirectionString(int angle)
{
	int a = angle % 360;
	LString *s = LString::string();
	if ((unsigned int)(a - 111) < 139)
		s->appendString(localized("AbbrSouth"));
	else if ((unsigned int)(a - 70) >= 221)
		s->appendString(localized("AbbrNorth"));
	if ((unsigned int)(a - 21) < 139)
		s->appendString(localized("AbbrEast"));
	else if ((unsigned int)(a - 201) < 139)
		s->appendString(localized("AbbrWest"));
	return s;
}

LString *Utils::windShortDescription(int angle)
{
	return LString::stringWithFormat(localized("WindDescr_ShortFormat"), _windDirectionString(angle));
}

LString *Utils::windDescription(int angle)
{
	return LString::stringWithFormat(localized("WindDescr_Format"), _windDirectionString(angle));
}

// Cloud map names are "west_YY_MM_DD_HH.dds" and "east_…": newer first,
// and for the same time west before east.
int CFileNameComparator::compare(const void *a, const void *b, const void *)
{
	StAutoreleasePool pool;
	LString *nameA = (*(LString *const *)a)->stringByDeletingPathExtension();
	LString *nameB = (*(LString *const *)b)->stringByDeletingPathExtension();
	LArray *partsA = nameA->componentsSeparatedByString(&sCloudsUnderscope);
	LArray *partsB = nameB->componentsSeparatedByString(&sCloudsUnderscope);
	if (partsA->count() != 5)
		return partsB->count() != 5 ? 0 : 1;
	if (partsB->count() != 5)
		return -1;
	for (int i = 1; i < 5; i++) {
		int va = loadIntFromObject(partsA->objectAtIndex(i));
		int vb = loadIntFromObject(partsB->objectAtIndex(i));
		if (va < vb)
			return 1;
		if (vb < va)
			return -1;
	}
	bool westA = lobj_cast<LString>(partsA->objectAtIndex(0))->isEqualTo(&sCloudsWest);
	bool westB = lobj_cast<LString>(partsB->objectAtIndex(0))->isEqualTo(&sCloudsWest);
	if (westA < westB)
		return 1;
	if (westA > westB)
		return -1;
	return 0;
}
