// WeatherTracer: the weather where the camera looks.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(WeatherTracer, 0)
LF_SELECTOR(WeatherTracer, _onReportDownloadTimeout);

// A change of the sky or the precipitations takes this long, in seconds.
static const double kWeatherChangeTime = 1.2;
// At most this many stations of a report are averaged.
enum { kMaxStations = 20 };

WeatherTracer::WeatherTracer()
    : _useCustomWeather(false), _tracing(false), _realTracing(false), _hasReportData(false),
      _weatherVisible(false), _atmoFactor(0.0f), _taskFound(false), _retries(0), _reportRadius(0.0f),
      _loading(false), _timeoutTimer(0), _report(0), _hasStations(false), _downloadFailed(false),
      _retryTime(0.0), _lastTraceTime(-1.0), _skyChangeTime(-1.0), _skyCode(kSkyClean),
      _nextSkyCode(kSkyClean), _customSkyMix(0.0f), _precipChangeTime(-1.0), _customPrecipMix(0.0f)
{
	_point = _reportPoint = LZeroPoint;
}

WeatherTracer *WeatherTracer::init()
{
	if (LObject::init() == 0)
		return 0;
	return this;
}

void WeatherTracer::dealloc()
{
	LRelease(_report);
	LObject::dealloc();
}

void WeatherTracer::invalidate()
{
	if (_timeoutTimer) {
		_timeoutTimer->invalidate();
		_timeoutTimer = 0;
	}
	release();
}

void WeatherTracer::getSkyState(TSkyCode *sky, float *weight, TSkyCode *next, float *nextWeight) const
{
	*sky = _skyCode;
	*next = _nextSkyCode;
	if (_useCustomWeather) {
		*weight = 1.0f - _customSkyMix;
		*nextWeight = _customSkyMix;
	} else if (_skyChangeTime <= 0.0) {
		*weight = 1.0f;
		*nextWeight = 0.0f;
	} else {
		float f = (float)((LTimer::currentTimeValue() - _skyChangeTime) / (double)1.2f);
		if (f > 1.0f)
			f = 1.0f;
		*weight = 1.0f - f;
		*nextWeight = f;
	}
}

void WeatherTracer::getPrecipitations(SPrecipitationDescr *precip, float *weight, SPrecipitationDescr *next,
                                      float *nextWeight) const
{
	*precip = _precip;
	*next = _nextPrecip;
	if (_useCustomWeather) {
		*weight = 1.0f - _customPrecipMix;
		*nextWeight = _customPrecipMix;
	} else if (_precipChangeTime <= 0.0) {
		*weight = 1.0f;
		*nextWeight = 0.0f;
	} else {
		float f = (float)((LTimer::currentTimeValue() - _precipChangeTime) / (double)1.2f);
		if (f > 1.0f)
			f = 1.0f;
		*weight = 1.0f - f;
		*nextWeight = f;
	}
}

bool WeatherTracer::weatherInfoIsAvailable()
{
	if (_useCustomWeather && _weatherVisible)
		return true;
	return isTracingOnline();
}

void WeatherTracer::startTracing()
{
	_tracing = true;
	_realTracing = false;
}

void WeatherTracer::stopTracing()
{
	_tracing = false;
	if (_realTracing)
		_cancelRealTracing();
}

void WeatherTracer::tracePosition(LPoint geoPoint, float altitude)
{
	float factor = Atmosphere::transformFactorFromDistance(altitude + 1.0f);
	_weatherVisible = factor > 0.02f;
	_atmoFactor = factor;
	bool online = Preferences::simulatedWeatherEnabled();
	if (!online || !_weatherVisible || _useCustomWeather) {
		// Out of the atmosphere, offline or set by hand: no reports.
		if ((_atmoFactor < 0.01f || !online || _useCustomWeather) && _tracing) {
			if (!_realTracing)
				return;
			_cancelRealTracing();
		}
	} else if (_tracing && !_realTracing) {
		_startRealTracing(geoPoint);
	}
	if (_realTracing)
		_traceGeoPoint(geoPoint);
}

void WeatherTracer::_startRealTracing(LPoint)
{
	_lastTraceTime = -1.0;
	_realTracing = true;
}

void WeatherTracer::_cancelRealTracing()
{
	_lastTraceTime = -1.0;
	_realTracing = false;
}

void WeatherTracer::downloadFinishedForTask(LMap *params, LData *data, int status)
{
	_loading = false;
	if (_timeoutTimer) {
		_timeoutTimer->invalidate();
		_timeoutTimer = 0;
	}
	_retries = 0;
	if (data == 0 || status != 200) {
		unsigned char quietly;
		if (DownloadManager::instance()->authorizationFailed(&quietly) && !quietly)
			stopTracing();
		_downloadFailed = true;
		_retryTime = LTimer::currentTimeValue() + 5.0;
	} else {
		_downloadFailed = false;
		LMap *report = CWManager::fullReportFromCurrentConditionsData(data);
		LString *point = lobj_cast<LString>(params->objectForKey(&kTaskParamRequestPoint));
		_switchToNewReport(report, LString::pointFromString(point));
	}
}

// Marks the tracer's tasks still queued, and removes them.
DownloadManager::TASK_ACTION WeatherTracer::_enumDownloadTasks(LMap *params, void *tracer)
{
	LString *point = lobj_cast<LString>(params->objectForKey(&kTaskParamRequestPoint));
	if (point)
		((WeatherTracer *)tracer)->_taskFound = true;
	return point ? DownloadManager::kTaskActionRemove : DownloadManager::kTaskActionKeep;
}

intptr_t WeatherTracer::_onReportDownloadTimeout(char *)
{
	_timeoutTimer = 0;
	if (_retries < 1) {
		_taskFound = false;
		DownloadManager::instance()->enumerateTasksWithCallback(_enumDownloadTasks, this);
		// Still waiting in the queue: ask once more.
		if (_taskFound) {
			_retries++;
			_loadReportForGeoPoint(_point);
		}
	}
	return 0;
}

void WeatherTracer::_loadReportForGeoPoint(LPoint geoPoint)
{
	_loading = true;
	LString *url = DynamicEnvironment::instance()->URLForReportDownload(kReportTypeCurrent, geoPoint);
	LMap *params = LMap::mapWithObjectsAndKeys(url, DownloadManager::kTaskParameterRequestString,
	                                           LString::stringWithCString("GET"), DownloadManager::kTaskParameterMethod,
	                                           LString::stringFromPoint(geoPoint), &kTaskParamRequestPoint, 0);
	DownloadManager::instance()->postDownloadTaskWithParameters(params, this, true);
	if (_timeoutTimer)
		_timeoutTimer->invalidate();
	_timeoutTimer = LTimer::timerWithParams(10000, this, "_onReportDownloadTimeout", false, 3);
}

void WeatherTracer::_switchToNewReport(LMap *report, LPoint geoPoint)
{
	LAssign(_report, report);
	_stations.removeAllObjects();
	_reportRadius = 5.0f;
	_reportPoint = geoPoint;
	LArray *points = (LArray *)_report->objectForKey(kReportObservationPoints);
	_hasStations = false;
	int count = points ? points->count() : 0;
	for (int i = 0; i < count; i++) {
		LMap *point = (LMap *)points->objectAtIndex(i);
		float lat = UMapParsingUtils::loadFloatFromObject(point->objectForKey(kReportObservationLat));
		float lon = UMapParsingUtils::loadFloatFromObject(point->objectForKey(kReportObservationLong));
		float dx = geoPoint.x - lon, dy = geoPoint.y - lat;
		float distance = sqrtf(dx * dx + dy * dy);
		if (distance <= 5.0f) {
			_stations.addObject(Vector2(lon, lat));
			_hasStations = true;
			// The next report is loaded beyond the nearest station.
			if (distance < _reportRadius && distance > 0.1f)
				_reportRadius = distance;
		} else {
			points->removeObjectAtIndex(i);
			count--;
			i--;
		}
	}
	if (_reportRadius >= 5.0f)
		_reportRadius = 0.1f;
}

void WeatherTracer::_traceGeoPoint(LPoint geoPoint)
{
	_hasReportData = false;
	_point = geoPoint;
	float dx = _reportPoint.x - geoPoint.x, dy = _reportPoint.y - geoPoint.y;
	if (_reportRadius * _reportRadius < dx * dx + dy * dy && !_loading &&
	    (!_downloadFailed || _retryTime < LTimer::currentTimeValue()))
		_loadReportForGeoPoint(geoPoint);

	TSkyCode sky = kSkyClean;
	SPrecipitationDescr precip;
	bool hasSky = false, hasPrecip = false;
	LArray *points;
	if (_report && _stations.count() && (points = (LArray *)_report->objectForKey(kReportObservationPoints)) != 0) {
		// The stations weigh the inverse of their distances.
		float weights[kMaxStations];
		int count = _stations.count() < kMaxStations ? _stations.count() : kMaxStations;
		for (int i = 0; i < count; i++) {
			const Vector2 &station = _stations[i];
			float sx = station.x - geoPoint.x, sy = station.y - geoPoint.y;
			float distance = sqrtf(sx * sx + sy * sy);
			weights[i] = 1.0f / (distance < 0.001f ? 0.001f : distance);
		}
		SWeatherInfo defaults;
		float sums[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		_target.temperature = _target.pressure = _target.humidity = _target.windSpeed = 0.0f;
		_target.wind = Vector2(0.0f, 0.0f);
		for (int i = 0; i < count; i++) {
			LMap *point = (LMap *)points->objectAtIndex(i);
			float w = weights[i];
			LObject *obj;
			if ((obj = point->objectForKey(kReportCurrentTemperature)) != 0) {
				_target.temperature += UMapParsingUtils::loadFloatFromObject(obj) * w;
				sums[0] += w;
			}
			if ((obj = point->objectForKey(kReportBarometricPressure)) != 0) {
				_target.pressure += UMapParsingUtils::loadFloatFromObject(obj) * w;
				sums[1] += w;
			}
			if ((obj = point->objectForKey(kReportHumidity)) != 0) {
				_target.humidity += UMapParsingUtils::loadFloatFromObject(obj) * w;
				sums[2] += w;
			}
			if ((obj = point->objectForKey(kReportWindDir)) != 0) {
				float angle = UMapParsingUtils::loadFloatFromObject(obj);
				if ((obj = point->objectForKey(kReportWindSpeed)) != 0) {
					float speed = UMapParsingUtils::loadFloatFromObject(obj);
					_target.wind += windDirFromWindAngle(angle) * (speed * w);
					sums[3] += w;
				}
			}
			// The first station with a sky decides the sky and the precipitations.
			if (!hasSky) {
				int skyReport = 0;
				if ((obj = point->objectForKey(kReportSkyStateCode)) != 0) {
					int code = UMapParsingUtils::loadIntFromObject(obj);
					if (code != 0) {
						sky = skyCodeFromReportCode(code);
						hasSky = true;
						skyReport = code;
					}
				}
				if ((obj = point->objectForKey(kReportPreciptationsCode)) != 0) {
					int code = UMapParsingUtils::loadIntFromObject(obj);
					if (code != 0) {
						precip = *precipCodeFromReportCode(code);
						hasPrecip = true;
					}
				}
				// Dust storms are only told by the sky.
				if (hasSky && !hasPrecip && skyReport - 24U < 2) {
					precip = SPrecipitationDescr(kPrecipDustStorm, 1.0f);
					hasPrecip = true;
				}
			}
			_hasReportData = true;
		}
		if (sums[0] > 0.0f)
			_target.temperature /= sums[0];
		else
			_target.temperature = defaults.temperature;
		if (sums[1] > 0.0f)
			_target.pressure /= sums[1];
		else
			_target.pressure = defaults.pressure;
		if (sums[2] > 0.0f)
			_target.humidity /= sums[2];
		else
			_target.humidity = defaults.humidity;
		if (sums[3] > 0.0f) {
			_target.wind /= sums[3];
			_target.windSpeed = sqrtf(_target.wind.x * _target.wind.x + _target.wind.y * _target.wind.y);
		} else {
			_target.wind = defaults.wind;
			_target.windSpeed = defaults.windSpeed;
		}
		_delta = _target - _current;
		// Precipitations come from clouds.
		if (hasPrecip && sky < kSkyBrokenCloudsDense && precip.code != kPrecipNone)
			sky = kSkyBrokenCloudsDense;
	}

	// The values move towards the target in a second.
	double now = LTimer::currentTimeValue();
	if (_lastTraceTime <= 0.0 || !_hasReportData) {
		_delta.temperature = _delta.pressure = _delta.humidity = _delta.windSpeed = 0.0f;
		_delta.wind = Vector2(0.0f, 0.0f);
		_current = _target;
	} else {
		float dt = (float)(now - _lastTraceTime);
		if (dt > 0.0f)
			_current += _delta * (dt <= 1.0f ? dt : 1.0f);
	}
	_lastTraceTime = now;

	// The sky changes in kWeatherChangeTime.
	if ((float)(now - _skyChangeTime) > kWeatherChangeTime && _skyChangeTime > 0.0) {
		_skyChangeTime = -1.0;
		_skyCode = _nextSkyCode;
		_nextSkyCode = kSkyClean;
	}
	if (hasSky && _skyChangeTime <= 0.0 && sky != _skyCode) {
		_nextSkyCode = sky;
		_skyChangeTime = now;
	}

	// So do the precipitations; when they stop they fade out.
	if (!hasPrecip && _precipChangeTime <= 0.0) {
		if (_nextPrecip.code != kPrecipNone) {
			_precip = _nextPrecip;
			_nextPrecip = SPrecipitationDescr();
		}
		if (_precip.code != kPrecipNone)
			_precipChangeTime = now;
	}
	if ((float)(now - _precipChangeTime) > kWeatherChangeTime && _precipChangeTime > 0.0) {
		_precipChangeTime = -1.0;
		_precip = _nextPrecip;
		_nextPrecip = SPrecipitationDescr();
	}
	if (hasPrecip && _precipChangeTime <= 0.0 && memcmp(&_precip, &precip, sizeof(precip)) != 0) {
		_precipChangeTime = now;
		_nextPrecip = precip;
	}
}
