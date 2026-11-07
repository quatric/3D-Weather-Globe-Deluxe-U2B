// CityPointEntry: a city; capitals and large cities weigh more on the map.

#include "AppModel.h"
#include <math.h>

LF_IMPLEMENT_CLASS(CityPointEntry, LF_CLASSOF(PointEntry))

void CityPointEntry::interfaceRetain()
{
	retain();
}

void CityPointEntry::interfaceRelease()
{
	release();
}

void CityPointEntry::dealloc()
{
	PointEntry::dealloc();
}

LString *CityPointEntry::iconKey() const
{
	LString *type = altType(0);
	if (type) {
		if (type->isEqualTo(kAltTypeCapital) || type->isEqualTo(kAltTypeCountryStateCapital))
			return kEntryCountryCapitalIconKey;
		if (type->isEqualTo(kAltTypeStateCapital))
			return kEntryStateCapitalIconKey;
		if (type->isEqualTo(kAltTypeCountySeat))
			return kEntryCountySeatIconKey;
	}
	if (population(0) > 1000000)
		return kEntryLargeCityIconKey;
	return kEntryCityIconKey;
}

float CityPointEntry::visualWeight() const
{
	LString *type = altType(0);
	int population = this->population(0);
	int n = population > 9 ? population : 10;
	float weight = PointEntry::visualWeight();
	if (type) {
		if (type->isEqualTo(kAltTypeCapital))
			weight *= 12.0f;
		else if (type->isEqualTo(kAltTypeStateCapital))
			weight *= 8.0f;
		else if (type->isEqualTo(kAltTypeCountySeat))
			weight *= 3.0f;
	}
	return (logf((float)n) / 2.3f) * weight;
}
