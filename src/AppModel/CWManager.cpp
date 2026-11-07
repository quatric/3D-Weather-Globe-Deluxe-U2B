// CWManager: the weather. Downloads the reports (CustomWeather XML: a report
// element with a header, observations, forecasts) through the download queue
// and parses them into report maps, refreshes the scheduled entries, and
// downloads the satellite cloud maps, compressed into textures in the
// background.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(CWManager, 0)
LF_SELECTOR(CWManager, _internetStateWasChanged);
LF_SELECTOR(CWManager, _cloudsUpdateTimerProc);
LF_SELECTOR(CWManager, _mapCompressionComplete);
LF_SELECTOR(CWManager, _entriesTimerProc);

// How often the scheduled entries and the clouds are checked.
static const unsigned int kEntriesCheckInterval = 15000;
static const unsigned int kCloudsCheckInterval = 120000;

void CWManager::invalidate()
{
	if (_cloudsManager) {
		_cloudsManager->release();
		_cloudsManager = 0;
	}
	if (_cloudsTimer)
		_cloudsTimer->invalidate();
	_cloudsTimer = 0;
	if (_entriesTimer)
		_entriesTimer->invalidate();
	_entriesTimer = 0;
	if (_compressionQueue)
		_compressionQueue->terminateQueue(1);
	_compressionQueue = 0;
	classObject()->killSingleton();
}

void CWManager::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	if (_cloudMapsInQueue) {
		_cloudMapsInQueue->release();
		_cloudMapsInQueue = 0;
	}
	if (_unused30) {
		_unused30->release();
		_unused30 = 0;
	}
	if (_unused34) {
		_unused34->release();
		_unused34 = 0;
	}
	if (_weatherImages) {
		_weatherImages->release();
		_weatherImages = 0;
	}
	if (_windImages) {
		_windImages->release();
		_windImages = 0;
	}
	if (_scheduledEntries) {
		_scheduledEntries->release();
		_scheduledEntries = 0;
	}
	if (_lastCloudsUpdate) {
		_lastCloudsUpdate->release();
		_lastCloudsUpdate = 0;
	}
	if (_compressionQueue)
		_compressionQueue->terminateQueue(1);
	_compressionQueue = 0;
	LObject::dealloc();
}

void CWManager::_startCloudsUpdate()
{
	if (!Preferences::usesInternet() || _cloudsTimer)
		return;
	_cloudsTimer = LTimer::timerWithParams(kCloudsCheckInterval, this, "_cloudsUpdateTimerProc", true, 0);
	invokeSelector("_cloudsUpdateTimerProc", _cloudsTimer);
}

static DownloadManager::TASK_ACTION _clearCloudsCB(LMap *params, void *)
{
	LObject *kind = params ? params->objectForKey(CWManager::kRequestKind) : 0;
	return kind && kind->isEqualTo(CWManager::kRequestKindMAP) ? DownloadManager::kTaskActionRemove
	                                                           : DownloadManager::kTaskActionKeep;
}

void CWManager::resetWeatherInternetState()
{
	if (_weatherInternetState == 0)
		return;
	_weatherInternetState = 0;
	LNotificationCenter::center()->postNotification(kWeatherInternetStateWasChangedNotification, this, 0);
}

int CWManager::cloudMapsCountInDownloadQueue()
{
	return _cloudMapsInQueue->count();
}

// CustomWeather writes "*" for no value.
void CWManager::_addStringIfExistsInDictionary(LMap *from, LString *key, LMap *to, LString *toKey)
{
	LString *value = (LString *)from->objectForKey(key);
	if (value && value->length() != 0 && !value->isEqualTo(&__asterixKey))
		to->setObjectForKey(value, toKey);
}

// The values every kind of report has.
void CWManager::_processWeatherDictionary(LMap *from, LMap *to)
{
	_addStringIfExistsInDictionary(from, &__iconKey, to, kReportSummaryCode);
	_addStringIfExistsInDictionary(from, &__wind_speedKey, to, kReportWindSpeed);
	_addStringIfExistsInDictionary(from, &__wind_dirKey, to, kReportWindDir);
	_addStringIfExistsInDictionary(from, &__sky_descKey, to, kReportSkyStateCode);
	_addStringIfExistsInDictionary(from, &__visibilityKey, to, kReportVisibility);
	_addStringIfExistsInDictionary(from, &__precip_descKey, to, kReportPreciptationsCode);
	_addStringIfExistsInDictionary(from, &__comfortKey, to, kReportComfortLevel);
	_addStringIfExistsInDictionary(from, &__humidityKey, to, kReportHumidity);
	_addStringIfExistsInDictionary(from, &__descriptionKey, to, kReportSummaryDesc);
	_addStringIfExistsInDictionary(from, &__precipKey, to, kReportPreciptationsDesc);
	_addStringIfExistsInDictionary(from, &__skyKey, to, kReportSkyDescription);
	_addStringIfExistsInDictionary(from, &__tempKey, to, kReportTemperatureDesc);
	_addStringIfExistsInDictionary(from, &__dew_pointKey, to, kReportDewPoint);
}

void CWManager::_parseExtendedReport(LXMLTree *, LMap *)
{
}

intptr_t CWManager::_entriesTimerProc(char *)
{
	LSetIterator it(_scheduledEntries, true);
	while (PointEntry *entry = (PointEntry *)it.nextObject())
		entry->singleUpdateTaskForScheduledReports();
	return 0;
}

void CWManager::updateScheduledEntries()
{
	resetWeatherInternetState();
	if (_scheduledEntries == 0)
		return;
	LSetIterator it(_scheduledEntries, true);
	while (PointEntry *entry = (PointEntry *)it.nextObject())
		entry->manualUpdateOfScheduledReports();
}

void CWManager::unscheduleEntry(PointEntry *entry)
{
	if (entry == 0 || _scheduledEntries == 0)
		return;
	_scheduledEntries->removeObject(entry);
	if (_entriesTimer && _scheduledEntries->count() == 0) {
		_entriesTimer->invalidate();
		_entriesTimer = 0;
	}
}

struct SReportTask {
	PointEntry *entry;
	int type;
};

static DownloadManager::TASK_ACTION _clearReportTaskCB(LMap *params, void *context)
{
	SReportTask *task = (SReportTask *)context;
	if (params->objectForKey(&kRequestEntry) == task->entry &&
	    UMapParsingUtils::loadIntFromObject(params->objectForKey(&kRequestedReportID)) == task->type)
		return DownloadManager::kTaskActionRemove;
	return DownloadManager::kTaskActionKeep;
}

void CWManager::cancelLoadTaskForEntry(PointEntry *entry, TReportType type)
{
	SReportTask task = { entry, type };
	DownloadManager::instance()->enumerateTasksWithCallback(_clearReportTaskCB, &task);
}

void CWManager::scheduleCloudsUpdateTask()
{
	if (!DownloadManager::isInternetAvailable()) {
		_cloudsManager->setUseDefaultClouds(1);
		return;
	}
	_compressionFailed = false;
	_cloudsManager->setUseDefaultClouds(0);
	_startCloudsUpdate();
}

bool CWManager::isWeatherInternetStateOK()
{
	return _weatherInternetState == 0;
}

LXMLTree *CWManager::_subtreeWithDescriptor(LXMLTree *tree, LString *descriptor)
{
	int count = tree->childrenCount();
	for (int i = 0; i < count; i++) {
		LXMLTree *child = (LXMLTree *)tree->childAtIndex(i);
		if (child->node()->name()->compareIgnoringCase(descriptor) == 0)
			return child;
	}
	return 0;
}

void CWManager::_parseAstronomyReport(LXMLTree *tree, LMap *report)
{
	LXMLTree *forecast = _subtreeWithDescriptor(tree, &kXMLStringForecast);
	if (forecast == 0) {
		LLog("Unable to find forecast data inside the astronomy report\n");
		return;
	}
	LMap *attributes = forecast->node()->attributes();
	report->setObjectForKey(attributes->objectForKey(&__cityKey), kReportCommonCityID);
	report->setObjectForKey(attributes->objectForKey(&__city_nameKey), kReportCommonCityName);
	report->setObjectForKey(attributes->objectForKey(&__sunriseKey), kReportAstronomySunrise);
	report->setObjectForKey(attributes->objectForKey(&__sunsetKey), kReportAstronomySunset);
	report->setObjectForKey(attributes->objectForKey(&__moonriseKey), kReportAstronomyMoonrise);
	report->setObjectForKey(attributes->objectForKey(&__moonsetKey), kReportAstronomyMoonset);
	report->setObjectForKey(attributes->objectForKey(&__moon_phaseKey), kReportAstronomyMoonPhase);
}

void CWManager::_addFirstEntryWithKey(LString *key, TLArray<LMap *> *items, LMap *to)
{
	int count = items->count();
	for (int i = 0; i < count; i++) {
		LObject *value = items->objectAtIndex(i)->objectForKey(key);
		if (value) {
			to->setObjectForKey(value, key);
			return;
		}
	}
}

void CWManager::_addFirstNonZeroEntryWithKey(LString *key, TLArray<LMap *> *items, LMap *to)
{
	int count = items->count();
	for (int i = 0; i < count; i++) {
		LObject *value = items->objectAtIndex(i)->objectForKey(key);
		if (UMapParsingUtils::loadIntFromObject(value) != 0) {
			to->setObjectForKey(value, key);
			return;
		}
	}
}

void CWManager::_addAverageForEntryWithKey(LString *key, TLArray<LMap *> *items, LMap *to)
{
	int count = items->count();
	float sum = 0.0f, n = 0.0f;
	for (int i = 0; i < count; i++) {
		LObject *value = items->objectAtIndex(i)->objectForKey(key);
		if (value) {
			sum += UMapParsingUtils::loadFloatFromObject(value);
			n += 1.0f;
		}
	}
	if (n > 0.0f)
		to->setObjectForKey(LString::stringWithFormat("%f", (double)(sum / n)), key);
}

void CWManager::_clearCloudsLoadingQueue()
{
	DownloadManager::instance()->enumerateTasksWithCallback(_clearCloudsCB, 0);
	if (_cloudMapsInQueue)
		_cloudMapsInQueue->removeAllObjects();
	if (_lastCloudsUpdate) {
		_lastCloudsUpdate->release();
		_lastCloudsUpdate = 0;
	}
}

void CWManager::scheduleCloudsReloadTask()
{
	if (_compressionQueue)
		_compressionQueue->terminateQueue(1);
	_compressionQueue = 0;
	_clearCloudsLoadingQueue();
	if (_cloudsTimer)
		invokeSelector("_cloudsUpdateTimerProc", _cloudsTimer);
}

// args[0]: the compression task. A map that couldn't be written stops the
// clouds updates, once.
intptr_t CWManager::_mapCompressionComplete(char *args)
{
	_CCloudTextureCompressionTask *task = LF_ARG(args, 0, _CCloudTextureCompressionTask *);
	LString *fileName = task->_path->lastPathComponent();
	if (!task->_failed) {
		if (Preferences::usesInternet()) {
			_cloudMapsInQueue->removeObject(fileName);
			_cloudsManager->_invalFrames();
			LNotificationCenter::center()->postNotification(kCloudsWereUpdatedNotification, this, 0);
		}
	} else if (!_compressionFailed) {
		_compressionFailed = true;
		_clearCloudsLoadingQueue();
		if (_compressionQueue)
			_compressionQueue->terminateQueue(1);
		_compressionQueue = 0;
		if (_cloudsTimer)
			_cloudsTimer->invalidate();
		_cloudsTimer = 0;
		_cloudsManager->setUseDefaultClouds(1);
		IAlertCallback::showAlertWithCode(0x1020002);
	}
	return 0;
}

void CWManager::_stopCloudsUpdate()
{
	if (_cloudsTimer)
		_cloudsTimer->invalidate();
	_cloudsTimer = 0;
	if (_cloudsManager)
		_cloudsManager->cleanup();
	_clearCloudsLoadingQueue();
}

void CWManager::unscheduleCloudsUpdateTask()
{
	_stopCloudsUpdate();
}

intptr_t CWManager::_internetStateWasChanged(char *)
{
	if (!Preferences::cloudsEnabled())
		return 0;
	if (!Preferences::usesInternet()) {
		_stopCloudsUpdate();
		LNotificationCenter::center()->postNotification(Preferences::kCloudsTextureSizeWasChanged, this, 0);
	} else {
		LNotificationCenter::center()->postNotification(Preferences::kCloudsTextureSizeWasChanged, this, 0);
		_startCloudsUpdate();
	}
	return 0;
}

void CWManager::initAsSingleton(void *)
{
	LNotificationCenter::center()->addObserver(this, "_internetStateWasChanged", Preferences::kInternetStateWasChanged,
	                                           0);
	_cloudMapsInQueue = LComparableSet::_alloc()->init();
	_cloudsManager = CloudsManager::_alloc()->init();
	if (Preferences::cloudsEnabled())
		scheduleCloudsUpdateTask();
}

void CWManager::scheduleEntry(PointEntry *entry)
{
	if (entry == 0)
		return;
	if (_scheduledEntries == 0)
		_scheduledEntries = LSet::_alloc()->init();
	_scheduledEntries->addObject(entry);
	if (_entriesTimer == 0)
		_entriesTimer = LTimer::timerWithParams(kEntriesCheckInterval, this, "_entriesTimerProc", true, 0);
}

LMap *CWManager::_headerForReport(LXMLNode *node)
{
	LMap *attributes = node->attributes();
	return LMap::mapWithObjectsAndKeys(attributes->objectForKey(&__unix_dateKey), kReportHeaderUnixDateKey,
	                                   attributes->objectForKey(&__country_nameKey), kReportHeaderCountryNameKey,
	                                   attributes->objectForKey(&__state_nameKey), kReportHeaderStateNameKey, 0);
}

// The attributes of the children with the name.
TLArray<LMap *> *CWManager::_allAttributesWithDescriptor(LString *descriptor, LXMLTree *tree)
{
	int count = tree->childrenCount();
	TLArray<LMap *> *items = (TLArray<LMap *> *)LArray::array();
	for (int i = 0; i < count; i++) {
		LXMLNode *node = ((LXMLTree *)tree->childAtIndex(i))->node();
		if (node->name()->compareIgnoringCase(descriptor) == 0)
			items->addObject(node->attributes());
	}
	return items;
}

// Every observation of a current conditions report, with its position.
LMap *CWManager::fullReportFromCurrentConditionsData(LData *data)
{
	LMap *report = LMap::map();
	LXMLTree *tree = LXMLTree::treeFromData(data);
	if (tree == 0)
		return report;
	tree = _subtreeWithDescriptor(tree, &kXMLStringReport);
	if (tree) {
		LMap *header = _headerForReport(tree->node());
		if (header)
			report->setObjectForKey(header, kReportHeaderDictionaryKey);
	}
	TLArray<LMap *> *observations = _allAttributesWithDescriptor(&kXMLStringObservation, tree);
	int count = observations ? observations->count() : 0;
	LArray *points = LArray::array();
	report->setObjectForKey(points, kReportObservationPoints);
	for (int i = 0; i < count; i++) {
		LMap *observation = observations->objectAtIndex(i);
		LMap *point = LMap::map();
		points->addObject(point);
		_processWeatherDictionary(observation, point);
		_addStringIfExistsInDictionary(observation, &__temperatureKey, point, kReportCurrentTemperature);
		_addStringIfExistsInDictionary(observation, &__baro_pressureKey, point, kReportBarometricPressure);
		_addStringIfExistsInDictionary(observation, &__latitudeKey, point, kReportObservationLat);
		_addStringIfExistsInDictionary(observation, &__longitudeKey, point, kReportObservationLong);
	}
	return report;
}

// The first observation, unless it has no weather: then the codes of the
// first one that has, and the averages of the measures.
void CWManager::_parseCurrentReport(LXMLTree *tree, LMap *report, PointEntry *)
{
	TLArray<LMap *> *observations = _allAttributesWithDescriptor(&kXMLStringObservation, tree);
	int count = observations ? observations->count() : 0;
	if (count == 0)
		return;
	TLArray<LMap *> *items = (TLArray<LMap *> *)LArray::array();
	int i;
	for (i = 0; i < count; i++) {
		LMap *observation = observations->objectAtIndex(i);
		LMap *item = LMap::map();
		items->addObject(item);
		_processWeatherDictionary(observation, item);
		_addStringIfExistsInDictionary(observation, &__temperatureKey, item, kReportCurrentTemperature);
		_addStringIfExistsInDictionary(observation, &__baro_pressureKey, item, kReportBarometricPressure);
		if (UMapParsingUtils::loadIntFromObject(observation->objectForKey(&__sky_descKey)) != 0)
			break;
		if (UMapParsingUtils::loadIntFromObject(observation->objectForKey(&__iconKey)) != 0)
			break;
	}
	if (i == 0 || i == count) {
		report->addEntriesFromMap(items->objectAtIndex(0));
		return;
	}
	_addFirstNonZeroEntryWithKey(kReportSummaryCode, items, report);
	_addFirstNonZeroEntryWithKey(kReportSkyStateCode, items, report);
	_addFirstNonZeroEntryWithKey(kReportPreciptationsCode, items, report);
	_addFirstEntryWithKey(kReportSummaryDesc, items, report);
	_addFirstEntryWithKey(kReportSkyDescription, items, report);
	_addFirstEntryWithKey(kReportPreciptationsDesc, items, report);
	_addFirstEntryWithKey(kReportTemperatureDesc, items, report);
	_addFirstEntryWithKey(kReportCurrentTemperature, items, report);
	_addAverageForEntryWithKey(kReportWindSpeed, items, report);
	_addAverageForEntryWithKey(kReportWindDir, items, report);
	_addAverageForEntryWithKey(kReportVisibility, items, report);
	_addAverageForEntryWithKey(kReportComfortLevel, items, report);
	_addAverageForEntryWithKey(kReportHumidity, items, report);
	_addAverageForEntryWithKey(kReportDewPoint, items, report);
	_addAverageForEntryWithKey(kReportBarometricPressure, items, report);
}

// The seven days forecast.
void CWManager::_parseExpandedReport(LXMLTree *tree, LMap *report)
{
	LXMLTree *location = _subtreeWithDescriptor(tree, &kXMLStringLocation);
	if (location == 0)
		return;
	TLArray<LMap *> *forecasts = _allAttributesWithDescriptor(&kXMLStringForecast, location);
	LArray *days = LArray::array();
	report->setObjectForKey(days, kReportSevenDaysForecasts);
	int count = forecasts->count();
	for (int i = 0; i < count; i++) {
		LMap *day = LMap::map();
		days->addObject(day);
		LMap *forecast = forecasts->objectAtIndex(i);
		_processWeatherDictionary(forecast, day);
		_addStringIfExistsInDictionary(forecast, &__day_of_week, day, kReportFCDayOfWeek);
		_addStringIfExistsInDictionary(forecast, &__low_temp, day, kReportFCTemperatureLow);
		_addStringIfExistsInDictionary(forecast, &__high_temp, day, kReportFCTemperatureHigh);
		_addStringIfExistsInDictionary(forecast, &__precip_prob, day, kReportFCPreciptProbability);
		_addStringIfExistsInDictionary(forecast, &__rainfall, day, kReportFCRainfall);
		_addStringIfExistsInDictionary(forecast, &__snowfall, day, kReportFCSnowfall);
		_addStringIfExistsInDictionary(forecast, &__uv_index, day, kReportFCUVIndex);
		_addStringIfExistsInDictionary(forecast, &__uv, day, kReportFCUVDescription);
	}
}

LMap *CWManager::parseXMLData(LData *data, TReportType type, PointEntry *entry)
{
	LMap *report = LMap::map();
	LXMLTree *tree = LXMLTree::treeFromData(data);
	if (tree == 0)
		return report;
	tree = _subtreeWithDescriptor(tree, &kXMLStringReport);
	if (tree == 0)
		return report;
	LMap *header = _headerForReport(tree->node());
	if (header)
		report->setObjectForKey(header, kReportHeaderDictionaryKey);
	if (type == kReportTypeExpanded)
		_parseExpandedReport(tree, report);
	else if (type == kReportTypeAstronomy)
		_parseAstronomyReport(tree, report);
	else if (type == kReportTypeCurrent)
		_parseCurrentReport(tree, report, entry);
	return report;
}

// Tells the requester; a failed download marks the weather internet state.
void CWManager::_reportDownloadFinishedForTask(LMap *params, LData *data, int status)
{
	LObject *target = params->objectForKey(&kNotificationTarget);
	const char *sel = LSelectorFromString((LString *)params->objectForKey(&kNotificationSelectorString));
	LValue *reportID = (LValue *)params->objectForKey(&kRequestedReportID);
	TReportType type = (TReportType)reportID->intValue();
	LMap *report = 0;
	if (data && status == 200) {
		LData *xml = DynamicEnvironment::instance()->xmlDataForReport(type, data);
		report = parseXMLData(xml, type, (PointEntry *)params->objectForKey(&kRequestEntry));
	}
	if (reportID && report) {
		target->invokeSelector(sel, LMap::mapWithObjectsAndKeys(report, CWOnlineCityData::kReportDictionaryKey, reportID,
		                                                        CWOnlineCityData::kReportIDKey, 0));
		resetWeatherInternetState();
		return;
	}
	if (type == kReportTypeNone)
		LLog("Unable to get downloaded dictionary or report ID\n");
	else
		target->invokeSelector(sel, LMap::mapWithObjectsAndKeys(reportID, CWOnlineCityData::kReportIDKey, (LObject *)0));
	if (_weatherInternetState != 0)
		return;
	_weatherInternetState = 1;
	LNotificationCenter::center()->postNotification(kWeatherInternetStateWasChangedNotification, this, 0);
}

LArray *CWManager::windImagesForIconAtIndex(int index)
{
	if (_windImages == 0) {
		TLArray<LString *> *files =
		    (TLArray<LString *> *)LArray::arrayWithObjects(LString::stringWithCString("INTERFACE\\wind_icons0.png"), (LObject *)0);
		_windImages = ImagesSequence::_alloc()->initWithFiles(files, 32);
		if (_windImages == 0)
			return 0;
	}
	return _windImages->imagesForIconAtIndex(index);
}

LArray *CWManager::weatherImagesForIconAtIndex(int index)
{
	if (_weatherImages == 0) {
		TLArray<LString *> *files =
		    (TLArray<LString *> *)LArray::arrayWithObjects(LString::stringWithCString("INTERFACE\\weather_icons0.png"),
		                                         LString::stringWithCString("INTERFACE\\weather_icons1.png"), 0);
		_weatherImages = ImagesSequence::_alloc()->initWithFiles(files, 36);
		if (_weatherImages == 0)
			return 0;
	}
	return _weatherImages->imagesForIconAtIndex(index);
}

// The target's selector is called with the report when it comes. After a
// failed download the requests fail at once while the authorization is
// known to fail. Places without a CustomWeather ID go by their coordinates,
// and so do the others when the URL creator has no URL for the ID (the
// AccuWeather one goes by coordinates only).
void CWManager::postLoadTaskForEntry(PointEntry *entry, TReportType type, LObject *target, const char *sel)
{
	if (_weatherInternetState != 0 && DownloadManager::instance()->authorizationFailed(0)) {
		target->invokeSelector(sel, LMap::mapWithObjectsAndKeys(LValue::valueWithInt(type),
		                                                        CWOnlineCityData::kReportIDKey, 0));
		return;
	}
	DynamicEnvironment *environment = DynamicEnvironment::instance();
	int customWeatherID = entry->customWeatherID(0);
	LString *url = customWeatherID > 0 ? environment->URLForReportDownload(type, customWeatherID) : 0;
	if (url == 0)
		url = environment->URLForReportDownload(type, entry->geoPoint(0));
	if (url == 0)
		return;
	LMap *params = LMap::mapWithObjectsAndKeys(
	    url, DownloadManager::kTaskParameterRequestString, LString::stringWithCString("GET"),
	    DownloadManager::kTaskParameterMethod, kRequestKindReport, kRequestKind, LValue::valueWithInt(type),
	    &kRequestedReportID, entry, &kRequestEntry, target, &kNotificationTarget, LStringFromSelector(sel),
	    &kNotificationSelectorString, 0);
	DownloadManager::instance()->postDownloadTaskWithParameters(params, this, 0);
}

void CWManager::_postCloudsRequest(LDate *date, unsigned char east, LString *fileName)
{
	int size = (int)_cloudsManager->frameTextureSize().width;
	LString *url = DynamicEnvironment::instance()->URLForClouds(!east, date, size);
	if (url == 0)
		return;
	_cloudMapsInQueue->addObject(fileName);
	LMap *params = LMap::mapWithObjectsAndKeys(url, DownloadManager::kTaskParameterRequestString,
	                                           LString::stringWithCString("GET"), DownloadManager::kTaskParameterMethod,
	                                           kRequestKindMAP, kRequestKind, fileName, &kRequestMapFileName, 0);
	DownloadManager::instance()->postDownloadTaskWithParameters(params, this, 0);
}

// Hourly: requests the maps of the last five days that aren't there, a pair
// (west and east) for each period of the clouds update time. The newest are
// downloaded again when they are an hour old.
intptr_t CWManager::_cloudsUpdateTimerProc(char *)
{
	LDate *now = LDate::_alloc()->init();
	double nowTime = now->timeIntervalSinceReferenceDate();
	if (_lastCloudsUpdate) {
		if (nowTime - _lastCloudsUpdate->timeIntervalSinceReferenceDate() < 3620.0) {
			now->release();
			return 0;
		}
		_lastCloudsUpdate->release();
		_lastCloudsUpdate = 0;
	}
	_lastCloudsUpdate = LRetain(now);
	LFileManager *fm = LFileManager::manager();
	LString *folder = CloudsManager::downloadedCloudsFolder();
	int step = Preferences::cloudsUpdateTime();
	LDate *frameStart = now;	// retained
	if (Preferences::usesInternet()) {
		int frames = (24 / step) * 5;
		for (int frame = 0; frame < frames; frame++) {
			int startHour = frameStart->hour();
			LAutoreleasePool::addNewPoolToTop();
			bool needWest = true, needEast = true;
			LString *westName = 0, *eastName = 0;
			LDate *date = LRetain(frameStart);
			for (int h = 0; h < step; h++) {
				westName = CloudsManager::mapFileNameForDate(date, 0);
				if (needWest && !_cloudMapsInQueue->containsObject(westName)) {
					LString *path = folder->stringByAppendingPathComponent(westName);
					if (fm->objectExistsAtPath(path, 0) &&
					    (frame != 0 || (nowTime - fm->lastUpdateTimeForFile(path)) / 3600.0 < 1.0))
						needWest = false;
				} else {
					needWest = false;
				}
				eastName = CloudsManager::mapFileNameForDate(date, 1);
				if (needEast && !_cloudMapsInQueue->containsObject(eastName)) {
					LString *path = folder->stringByAppendingPathComponent(eastName);
					if (fm->objectExistsAtPath(path, 0) &&
					    (frame != 0 || (nowTime - fm->lastUpdateTimeForFile(path)) / 3600.0 < 1.0))
						needEast = false;
				} else {
					needEast = false;
				}
				LDate *previous = LRetain(date->dateByAddingTime(0, 0, 0, -1, 0, 0));
				date->release();
				date = previous;
				if (startHour / step != date->hour() / step)
					break;
			}
			if (needWest)
				_postCloudsRequest(frameStart, 0, westName);
			if (needEast)
				_postCloudsRequest(frameStart, 1, eastName);
			LAutoreleasePool::removeTopPool();
			frameStart->release();
			frameStart = date;
		}
	}
	frameStart->release();
	CloudsManager::deleteObsoleteFiles();
	return 0;
}

void CWManager::_postTextureCompressionTask(LData *data, LString *path, unsigned char synchronous)
{
	if (!synchronous && _compressionQueue == 0) {
		_compressionQueue = LTaskQueue::detachNewQueue();
		_compressionQueue->runQueue();
	}
	_CCloudTextureCompressionTask *task =
	    LAutorelease(_CCloudTextureCompressionTask::_alloc()->initWithParams(path, data, this, "_mapCompressionComplete"));
	if (!synchronous)
		_compressionQueue->postTaskToQueue(task);
	else
		task->executeAndInvokeSelector(0);
}

// Only JPEGs make clouds.
void CWManager::_mapLoadComplete(LMap *params, LData *data)
{
	if (data == 0)
		return;
	LData *jpeg = DynamicEnvironment::instance()->jpegDataForCloudsData(data);
	const unsigned char *bytes = (const unsigned char *)jpeg->bytes();
	if (jpeg->length() < 3 || bytes[0] != 0xff || bytes[1] != 0xd8 || bytes[2] != 0xff)
		return;
	LString *path = CloudsManager::downloadedCloudsFolder()->stringByAppendingPathComponent(
	    (LString *)params->objectForKey(&kRequestMapFileName));
	_postTextureCompressionTask(jpeg, path, 0);
}

// A failed map stops the clouds updates and falls back to the default
// clouds, telling why unless the user knows already.
void CWManager::_mapDownloadFinishedForTask(LMap *params, LData *data, int status)
{
	if (data && status == 200) {
		_compressionFailed = false;
		_cloudsManager->setUseDefaultClouds(0);
		_mapLoadComplete(params, data);
		return;
	}
	if (_cloudMapsInQueue->isEmpty())
		return;
	_clearCloudsLoadingQueue();
	if (_cloudsTimer)
		_cloudsTimer->invalidate();
	_cloudsTimer = 0;
	_cloudsManager->setUseDefaultClouds(1);
	unsigned char quietly;
	int code;
	if (!DownloadManager::instance()->authorizationFailed(&quietly))
		code = 0x1020001;
	else {
		if (quietly)
			return;
		code = 0x1020000;
	}
	IAlertCallback::showAlertWithCode(code);
}

void CWManager::downloadFinishedForTask(LMap *params, LData *data, int status)
{
	LObject *kind = params->objectForKey(kRequestKind);
	if (kind == 0 || !kind->getClass()->isSubclassOf(LF_CLASSOF(LString)))
		return;
	if (kind->isEqualTo(kRequestKindMAP))
		_mapDownloadFinishedForTask(params, data, status);
	else if (kind->isEqualTo(kRequestKindReport))
		_reportDownloadFinishedForTask(params, data, status);
}
