// The weather values and codes, and the light of each weather.

#include "RenderLib.h"

// ---------------------------------------------------------------------------
// SWeatherInfo

SWeatherInfo &SWeatherInfo::operator+=(const SWeatherInfo &w)
{
	temperature += w.temperature;
	pressure += w.pressure;
	humidity += w.humidity;
	windSpeed += w.windSpeed;
	wind += w.wind;
	return *this;
}

SWeatherInfo &SWeatherInfo::operator-=(const SWeatherInfo &w)
{
	temperature -= w.temperature;
	pressure -= w.pressure;
	humidity -= w.humidity;
	windSpeed -= w.windSpeed;
	wind -= w.wind;
	return *this;
}

SWeatherInfo SWeatherInfo::operator+(const SWeatherInfo &w) const
{
	SWeatherInfo r = *this;
	return r += w;
}

SWeatherInfo SWeatherInfo::operator-(const SWeatherInfo &w) const
{
	SWeatherInfo r = *this;
	return r -= w;
}

SWeatherInfo SWeatherInfo::operator*(float f) const
{
	SWeatherInfo r = *this;
	r.temperature *= f;
	r.pressure *= f;
	r.humidity *= f;
	r.windSpeed *= f;
	return r;
}

// ---------------------------------------------------------------------------
// The codes

static TSkyCode sSkyCodeRemap[40] = {
	kSkyClean, kSkyClean, kSkyClean, kSkyAlmostClean, kSkyAlmostClean, kSkyClean, kSkyClean,
	kSkyBrokenCloudsLight, kSkyAlmostClean, kSkyAlmostClean, kSkyBrokenCloudsMedium, kSkyBrokenCloudsMedium,
	kSkyHighClouds, kSkyBrokenCloudsMedium, kSkyBrokenCloudsMedium, kSkyBrokenCloudsMedium,
	kSkyBrokenCloudsDense, kSkyDenseClouds, kSkyOvercastClouds, kSkyOvercastClouds, kSkyFoggyClouds,
	kSkyFoggyClouds, kSkyFoggyClouds, kSkyFoggyClouds, kSkyStormyClouds, kSkyStormyClouds,
	kSkyBrokenCloudsMedium, kSkyBrokenCloudsLight, kSkyAlmostClean, kSkyAlmostClean, kSkyClean,
	kSkyBrokenCloudsLight, kSkyAlmostClean, kSkyFoggyClouds, kSkyClean, kSkyClean, kSkyClean, kSkyClean,
	kSkyClean, kSkyClean,
};

#define P(code, intensity) SPrecipitationDescr(code, intensity)
static SPrecipitationDescr sPrecipCodeRemap[80] = {
	P(kPrecipNone, 0.0f), P(kPrecipRain, 0.2f), P(kPrecipRain, 0.2f), P(kPrecipRain, 0.3f),
	P(kPrecipRain, 0.3f), P(kPrecipRain, 0.3f), P(kPrecipRain, 0.3f), P(kPrecipRain, 0.3f),
	P(kPrecipRain, 0.4f), P(kPrecipRain, 0.5f), P(kPrecipRain, 0.6f), P(kPrecipRain, 0.7f),
	P(kPrecipRain, 0.8f), P(kPrecipRain, 0.9f), P(kPrecipRain, 1.0f), P(kPrecipRain, 1.0f),
	P(kPrecipTStorm, 0.3f), P(kPrecipTStorm, 0.4f), P(kPrecipTStorm, 0.5f), P(kPrecipTStorm, 0.6f),
	P(kPrecipTStorm, 0.7f), P(kPrecipTStorm, 0.9f), P(kPrecipTStorm, 1.0f), P(kPrecipHail, 1.0f),
	P(kPrecipTStorm, 0.8f), P(kPrecipTStorm, 1.0f), P(kPrecipTStorm, 1.0f), P(kPrecipRain, 0.4f),
	P(kPrecipRainWithSnow, 0.3f), P(kPrecipNone, 0.0f), P(kPrecipRainWithSnow, 0.5f), P(kPrecipRain, 0.5f),
	P(kPrecipRainWithSnow, 0.6f), P(kPrecipRainWithSnow, 0.9f), P(kPrecipRainWithSnow, 0.5f),
	P(kPrecipRainWithSnow, 0.5f), P(kPrecipRainWithSnow, 0.5f), P(kPrecipRainWithSnow, 0.5f),
	P(kPrecipRainWithSnow, 0.5f), P(kPrecipRainWithSnow, 0.5f),
	P(kPrecipSnow, 0.2f), P(kPrecipSnow, 0.2f), P(kPrecipSnow, 0.3f), P(kPrecipSnow, 0.4f),
	P(kPrecipSnow, 0.4f), P(kPrecipSnow, 0.5f), P(kPrecipSnow, 0.6f), P(kPrecipSnow, 0.8f),
	P(kPrecipSnow, 0.9f), P(kPrecipSnow, 1.0f), P(kPrecipRain, 0.2f), P(kPrecipRain, 0.3f),
	P(kPrecipRain, 0.4f), P(kPrecipRain, 0.5f), P(kPrecipRain, 0.8f), P(kPrecipTStorm, 0.5f),
	P(kPrecipSnow, 0.2f), P(kPrecipSnow, 0.3f), P(kPrecipSnow, 0.4f), P(kPrecipSnow, 0.5f),
	P(kPrecipSnow, 0.8f), P(kPrecipRainWithSnow, 0.4f), P(kPrecipRainWithSnow, 0.6f), P(kPrecipRain, 0.2f),
	P(kPrecipRain, 0.3f), P(kPrecipRain, 0.4f), P(kPrecipRain, 0.5f), P(kPrecipRain, 0.8f),
	P(kPrecipTStorm, 0.2f), P(kPrecipTStorm, 0.4f), P(kPrecipTStorm, 0.5f), P(kPrecipSnow, 0.2f),
	P(kPrecipSnow, 0.3f), P(kPrecipSnow, 0.4f), P(kPrecipSnow, 0.5f), P(kPrecipSnow, 0.8f),
	P(kPrecipRainWithSnow, 0.3f), P(kPrecipRainWithSnow, 0.6f), P(kPrecipNone, 0.0f), P(kPrecipNone, 0.0f),
};
#undef P

static LString *sPrecipNameRemap[kPrecipCodesCount] = {
	&sPrecipNameNone, &sPrecipNameRain, &sPrecipNameTStorm, &sPrecipNameRainWithSnow,
	&sPrecipNameSnow, &sPrecipNameHail, &sPrecipNameDustStorm,
};

// By day, then by night.
static LString *sPrecipSoundRemap[kPrecipCodesCount * 2] = {
	0, 0,
	&sPrecipSoundRainDay, &sPrecipSoundRainNight,
	&sPrecipSoundTStormDay, &sPrecipSoundTStormNight,
	&sPrecipSoundRainWithSnowDay, &sPrecipSoundRainWithSnowNight,
	0, 0,
	0, 0,
	&sPrecipSoundDustStormDay, &sPrecipSoundDustStormNight,
};

TSkyCode skyCodeFromReportCode(int code)
{
	return sSkyCodeRemap[code];
}

const SPrecipitationDescr *precipCodeFromReportCode(int code)
{
	return &sPrecipCodeRemap[code];
}

LString *precipNameFromCode(TPrecipitationCode code)
{
	return sPrecipNameRemap[code];
}

LString *precipSoundFromCode(TPrecipitationCode code, bool day)
{
	return sPrecipSoundRemap[code * 2 + (day ? 0 : 1)];
}

float sunFlaresVisibilityFromCode(TSkyCode code)
{
	switch (code) {
	case kSkyAlmostClean:
		return 0.9f;
	case kSkyBrokenCloudsLight:
		return 0.8f;
	case kSkyBrokenCloudsMedium:
		return 0.6f;
	case kSkyBrokenCloudsDense:
		return 0.1f;
	case kSkyDenseClouds:
	case kSkyOvercastClouds:
	case kSkyFoggyClouds:
	case kSkyStormyClouds:
		return 0.0f;
	default:
		return 1.0f;
	}
}

LString *skyTypeNameFromSkyCode(TSkyCode code)
{
	static const char *const names[kSkyCodesCount] = {
		"kSkyClean", "kSkyAlmostClean", "kSkyBrokenCloudsLight", "kSkyBrokenCloudsMedium",
		"kSkyBrokenCloudsDense", "kSkyHighClouds", "kSkyDenseClouds", "kSkyOvercastClouds",
		"kSkyFoggyClouds", "kSkyStormyClouds",
	};
	if ((unsigned)code >= kSkyCodesCount)
		return LString::emptyString();
	return LString::stringFromTable(names[code], "RenderLib", getRenderLibModule());
}

LString *precipTypeNameFromPrecipCode(TPrecipitationCode code)
{
	static const char *const names[kPrecipCodesCount] = {
		"kPrecipNone", "kPrecipRain", "kPrecipTStorm", "kPrecipRainWithSnow",
		"kPrecipSnow", "kPrecipHail", "kPrecipDustStorm",
	};
	if ((unsigned)code >= kPrecipCodesCount)
		return LString::emptyString();
	return LString::stringFromTable(names[code], "RenderLib", getRenderLibModule());
}

LString *cloudsTextureMaskPathFromCode(TSkyCode code)
{
	const char *path;
	if (code == kSkyBrokenCloudsLight)
		path = "TEXTURES\\SkyClouds\\BrokenClouds1Mask.png";
	else if (code == kSkyBrokenCloudsMedium)
		path = "TEXTURES\\SkyClouds\\BrokenClouds0Mask.png";
	else
		return 0;
	return LString::stringWithCString(path);
}

LString *cloudsTextureFromCode(TSkyCode code, bool normals)
{
	const char *path;
	if (!normals) {
		switch (code) {
		case kSkyAlmostClean:
			path = "TEXTURES\\SkyClouds\\AlmostClean.dds";
			break;
		case kSkyBrokenCloudsLight:
			path = "TEXTURES\\SkyClouds\\BrokenClouds1.dds";
			break;
		case kSkyBrokenCloudsMedium:
			path = "TEXTURES\\SkyClouds\\BrokenClouds0.dds";
			break;
		case kSkyBrokenCloudsDense:
			path = "TEXTURES\\SkyClouds\\BrokenClouds2.dds";
			break;
		case kSkyHighClouds:
			path = "TEXTURES\\SkyClouds\\HighClouds0.dds";
			break;
		case kSkyDenseClouds:
			path = "TEXTURES\\SkyClouds\\DenseClouds0.dds";
			break;
		case kSkyOvercastClouds:
			path = "TEXTURES\\SkyClouds\\OvercastClouds0.dds";
			break;
		case kSkyFoggyClouds:
			path = "TEXTURES\\SkyClouds\\FoggyClouds0.dds";
			break;
		case kSkyStormyClouds:
			path = "TEXTURES\\SkyClouds\\SStormClouds0.dds";
			break;
		default:
			return 0;
		}
	} else {
		switch (code) {
		case kSkyAlmostClean:
			path = "TEXTURES\\SkyClouds\\AlmostCleanNrm.dds";
			break;
		case kSkyBrokenCloudsLight:
			path = "TEXTURES\\SkyClouds\\BrokenCloudsNrm1.dds";
			break;
		case kSkyBrokenCloudsMedium:
			path = "TEXTURES\\SkyClouds\\BrokenCloudsNrm0.dds";
			break;
		case kSkyBrokenCloudsDense:
		case kSkyDenseClouds:
		case kSkyOvercastClouds:
		case kSkyFoggyClouds:
		case kSkyStormyClouds:
			path = "TEXTURES\\SkyClouds\\FlatNormals.dds";
			break;
		case kSkyHighClouds:
			path = "TEXTURES\\SkyClouds\\HighCloudsNrm0.dds";
			break;
		default:
			return 0;
		}
	}
	return LString::stringWithCString(path);
}

// ---------------------------------------------------------------------------
// The light of each weather

static TArray<SNatureParams *, _TPtrArrayAddRemoveFunc<SNatureParams> > sNPParamsArray;
// Indices in sNPParamsArray by precipitation, then sky.
static int sSkyPrecipToNPNameRemap[kPrecipCodesCount][kSkyCodesCount];

static void loadColor(Color *color, LMap *map, LString *key)
{
	LObject *obj = map->objectForKey(key);
	if (obj)
		UMapParsingUtilsFH::loadColorFromObject(color, obj);
}

SNatureParams *natureParamsForSkyPrecip(TSkyCode sky, TPrecipitationCode precip)
{
	static bool isValid = false;
	if (!isValid) {
		isValid = true;
		// The entries by name, each loaded once; the indices are one more.
		LStrMap indices;
		LMap *env = LMap::mapWithContentsOfFile(LString::stringWithCString("MAPS\\NatureEnv.map"));
		for (int p = 0; p != kPrecipCodesCount; p++) {
			LString *name = 0;
			switch (p) {
			case kPrecipNone:
				name = &sNatureParamsClean;
				break;
			case kPrecipRain:
			case kPrecipHail:
				name = &sNatureParamsRain;
				break;
			case kPrecipTStorm:
				name = &sNatureParamsTStorm;
				break;
			case kPrecipRainWithSnow:
				name = &sNatureParamsRainSnow;
				break;
			case kPrecipSnow:
				name = &sNatureParamsSnow;
				break;
			case kPrecipDustStorm:
				name = &sNatureParamsSandStorm;
				break;
			}
			for (int s = 0; s != kSkyCodesCount; s++) {
				// Without precipitations the clouds decide.
				if (p == kPrecipNone) {
					switch (s) {
					case kSkyAlmostClean:
					case kSkyBrokenCloudsLight:
					case kSkyHighClouds:
						name = &sNatureParamsLightClouds;
						break;
					case kSkyBrokenCloudsMedium:
						name = &sNatureParamsMidCLouds;
						break;
					case kSkyBrokenCloudsDense:
					case kSkyDenseClouds:
					case kSkyOvercastClouds:
					case kSkyStormyClouds:
						name = &sNatureParamsDenseClouds;
						break;
					case kSkyFoggyClouds:
						name = &sNatureParamsFoggyClouds;
						break;
					}
				}
				intptr_t index = indices.objectForKey(name);
				if (index != 0) {
					index--;
				} else {
					index = sNPParamsArray.count();
					indices.setObjectForKey(index + 1, name);
					LMap *entry = lobj_cast<LMap>(env ? env->objectForKey(name) : 0);
					if (entry) {
						SNatureParams *params = new SNatureParams;
						sNPParamsArray.addObject(params);
						loadColor(&params->sunDiffuse, entry, &sSunDiffuseColorKey);
						loadColor(&params->sunAmbient, entry, &sSunAmbientColorKey);
						loadColor(&params->waterLight, entry, &sWaterLightColorKey);
						loadColor(&params->moonDiffuse, entry, &sMoonDiffuseColorKey);
						loadColor(&params->moonAmbient, entry, &sMoonAmbientColorKey);
						loadColor(&params->fogDay, entry, &sFogDayColorKey);
						loadColor(&params->fogNight, entry, &sFogNightColorKey);
						loadColor(&params->envDay, entry, &sEnvDayColorKey);
						loadColor(&params->envNight, entry, &sEnvNightColorKey);
						LObject *obj = entry->objectForKey(&sLightingProbabilityKey);
						if (obj)
							params->lightingProbability = UMapParsingUtils::loadFloatFromObject(obj);
					}
				}
				sSkyPrecipToNPNameRemap[p][s] = (int)index;
			}
		}
	}
	int index = sSkyPrecipToNPNameRemap[precip][sky];
	// An entry missing from the map has no light.
	return index < (int)sNPParamsArray.count() ? sNPParamsArray[index] : 0;
}

void fillNatureParams(SNatureParams *out, TSkyCode sky, const SPrecipitationDescr *precip)
{
	SNatureParams *target = natureParamsForSkyPrecip(sky, precip->code);
	SNatureParams *clean = sNPParamsArray.count() ? sNPParamsArray[0] : 0;
	if (clean == 0 || target == 0) {
		memset(out, 0, sizeof(SNatureParams));
		return;
	}
	float intensity = precip->code == kPrecipNone ? 1.0f : precip->intensity;
	float w = 1.0f - (float)pow((double)(1.0f - intensity), 3.0);
	out->sunDiffuse = target->sunDiffuse * w + clean->sunDiffuse * (1.0f - w);
	out->sunAmbient = target->sunAmbient * w + clean->sunAmbient * (1.0f - w);
	out->waterLight = target->waterLight * w + clean->waterLight * (1.0f - w);
	out->moonDiffuse = target->moonDiffuse * w + clean->moonDiffuse * (1.0f - w);
	out->moonAmbient = target->moonAmbient * w + clean->moonAmbient * (1.0f - w);
	out->lightingProbability = (1.0f - w) * clean->lightingProbability + w * target->lightingProbability;
	out->fogDay = target->fogDay * w + clean->fogDay * (1.0f - w);
	out->fogNight = target->fogNight * w + clean->fogNight * (1.0f - w);
	out->envDay = target->envDay * w + clean->envDay * (1.0f - w);
	out->envNight = target->envNight * w + clean->envNight * (1.0f - w);
}

void SNatureParams::fillParams(const SNatureParams *a, float wa, const SNatureParams *b, float wb, float dayFactor,
                               SParams *out)
{
	out->sunDiffuse = b->sunDiffuse * wb + a->sunDiffuse * wa;
	out->sunAmbient = b->sunAmbient * wb + a->sunAmbient * wa;
	out->waterLight = b->waterLight * wb + a->waterLight * wa;
	out->moonDiffuse = b->moonDiffuse * wb + a->moonDiffuse * wa;
	out->moonAmbient = b->moonAmbient * wb + a->moonAmbient * wa;
	out->lightingProbability = wa * a->lightingProbability + wb * b->lightingProbability;
	float f = (float)pow((double)dayFactor, (double)0.6f);
	out->fog = b->fogNight * (wb * (1.0f - f)) + b->fogDay * (wb * f) + a->fogNight * ((1.0f - f) * wa) +
	           a->fogDay * (f * wa);
	out->env = b->envNight * (wb * (1.0f - dayFactor)) + a->envNight * ((1.0f - dayFactor) * wa) +
	           a->envDay * (dayFactor * wa) + b->envDay * (wb * dayFactor);
}

void SNatureParams::fillDefaultParams(float dayFactor, SParams *out)
{
	SNatureParams params;
	SPrecipitationDescr none;
	fillNatureParams(&params, kSkyClean, &none);
	fillParams(&params, 1.0f, &params, 0.0f, dayFactor, out);
}
