// FeatureEntry: a natural or man-made feature (volcano, lake, park...).

#include "AppModel.h"

LF_IMPLEMENT_CLASS(FeatureEntry, LF_CLASSOF(PointEntry))

FeatureEntry::FeatureEntry() : _featureRect(LZeroRect)
{
}

void FeatureEntry::interfaceRetain()
{
	retain();
}

void FeatureEntry::interfaceRelease()
{
	release();
}

void FeatureEntry::dealloc()
{
	PointEntry::dealloc();
}

LString *FeatureEntry::iconKey() const
{
	static const struct {
		LString **type;
		LString **icon;
	} kIcons[] = {
		{ &kGeoTypeVolcano, &kEntryVolcanoIconKey },
		{ &kGeoTypeDestroyedLandmark, &kEntryLandmarkIconKey },
		{ &kGeoTypeEndangeredSite, &kEntryEndangeredSiteIconKey },
		{ &kGeoTypeMountain, &kEntryMountainsIconKey },
		{ &kGeoTypeWaterfall, &kEntryWaterfallIconKey },
		{ &kGeoTypeZoo, &kEntryZoosIconKey },
		{ &kGeoTypeDesert, &kEntryDesertsIconKey },
		{ &kGeoTypeHills, &kEntryHillsIconKey },
		{ &kGeoTypeMountains, &kEntryMountainsIconKey },
		{ &kGeoTypeStrait, &kEntryStraitIconKey },
		{ &kGeoTypeBay, &kEntryBayIconKey },
		{ &kGeoTypeLagoon, &kEntryLagoonIconKey },
		{ &kGeoTypeNationalPark, &kEntryParksIconKey },
		{ &kGeoTypeFjord, &kEntryFjordIconKey },
		{ &kGeoTypeGulf, &kEntryGulfIconKey },
		{ &kGeoTypeSea, &kEntryWaterIconKey },
		{ &kGeoTypeLake, &kEntryLakeIconKey },
		{ &kGeoTypeOcean, &kEntryOceanIconKey },
		{ &kGeoTypeRiver, &kEntryWaterIconKey },
		{ &kGeoTypeBuildingStructure, &kEntryBuildingIconKey },
	};
	for (unsigned int i = 0; i < sizeof(kIcons) / sizeof(kIcons[0]); i++)
		if (_ownType->isEqualTo(*kIcons[i].type))
			return *kIcons[i].icon;
	return 0;
}
