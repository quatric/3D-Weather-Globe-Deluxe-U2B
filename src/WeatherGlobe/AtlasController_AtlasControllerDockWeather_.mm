#import "AtlasController.h"
#import "MainWindowController.h"
#import "PreferencesController.h"

// The home city's current temperature on the Dock icon, kept up to date
// while the Dock weather preference and the Internet are on.
@implementation AtlasController (AtlasControllerDockWeather)

// No temperature (or an expired report) clears the overlay.
- (void)_refreshDockWeather
{
	NSAttributedString *text = nil;
	LMap *report = _dockEntry ? _dockEntry->dictionaryForReport(kReportTypeCurrent) : 0;
	if (report) {
		bool expired = UMapParsingUtils::loadIntFromObject(report->objectForKey(kReportCommonDataExpired));
		LObject *temperature = report->objectForKey(kReportCurrentTemperature);
		if (!expired && temperature) {
			bool metric = Preferences::usesMetricSystem();
			float value = UMapParsingUtils::loadFloatFromObject(temperature);
			if (!metric)
				value = fahrenheitFromCelsius(value);
			text = [NSAttributedString stringForTemperature:value inCelsius:metric];
		}
	}
	[self setDockIconOverlayText:text];
	[self _invalidateDockMenu];
}

- (void)_scheduleDockEntry
{
	if (_dockScheduled || !Preferences::usesInternet())
		return;
	LNotificationCenter::center()->addObserver(self, @selector(_dockReportsWereChanged:),
		PointEntry::kPointEntryWasUpdatedViaOnlineNotification, _dockEntry);
	LNotificationCenter::center()->addObserver(self, @selector(_dockReportsWereChanged:),
		PointEntry::kPointEntryWasExpiredViaOnlineNotification, _dockEntry);
	_dockEntry->scheduleReports(1);
	_dockScheduled = YES;
}

- (void)_unscheduleDockEntry
{
	if (!_dockScheduled)
		return;
	_dockEntry->unscheduleReports(1);
	LNotificationCenter::center()->removeObserver(self, PointEntry::kPointEntryWasUpdatedViaOnlineNotification, _dockEntry);
	LNotificationCenter::center()->removeObserver(self, PointEntry::kPointEntryWasExpiredViaOnlineNotification, _dockEntry);
	_dockScheduled = NO;
}

- (void)_showDockWeather
{
	if (_dockEntry)
		return;
	_dockEntry = Preferences::homeCity();
	if (!_dockEntry)
		return;
	_dockEntry->retain();
	LNotificationCenter::center()->addObserver(self, @selector(_unitsOfMeasureWereChanged:),
		Preferences::kUnitsOfMeasureWereChanged, 0);
	LNotificationCenter::center()->addObserver(self, @selector(_homeCityWasChanged:),
		Preferences::kHomeCityWasChanged, 0);
	[self _scheduleDockEntry];
	[self _refreshDockWeather];
}

- (void)_hideDockWeather
{
	if (!_dockEntry)
		return;
	[self _unscheduleDockEntry];
	LNotificationCenter::center()->removeObserver(self, Preferences::kUnitsOfMeasureWereChanged, (LObject *)0);
	LNotificationCenter::center()->removeObserver(self, Preferences::kHomeCityWasChanged, (LObject *)0);
	_dockEntry->release();
	_dockEntry = 0;
	[self _refreshDockWeather];
}

- (void)_initDockWeather
{
	if (_dockWeatherWasInitialize)
		return;
	if (Preferences::doShowWeatherInDock() && Preferences::usesInternet())
		[self _showDockWeather];
	LNotificationCenter::center()->addObserver(self, @selector(_showWeatherInDockStateWasChanged:),
		Preferences::kShowWeatherInDockStateWasChanged, 0);
	LNotificationCenter::center()->addObserver(self, @selector(_showWeatherInDockStateWasChanged:),
		Preferences::kInternetStateWasChanged, 0);
	_dockWeatherWasInitialize = YES;
}

- (void)_finilizeDockWeather
{
	if (!_dockWeatherWasInitialize)
		return;
	LNotificationCenter::center()->removeObserver(self, Preferences::kShowWeatherInDockStateWasChanged, (LObject *)0);
	LNotificationCenter::center()->removeObserver(self, Preferences::kInternetStateWasChanged, (LObject *)0);
	[self _hideDockWeather];
	_dockWeatherWasInitialize = NO;
}

- (void)_showWeatherInDockStateWasChanged:(id)aChanged
{
	if (Preferences::doShowWeatherInDock() && Preferences::usesInternet())
		[self _showDockWeather];
	else
		[self _hideDockWeather];
}

- (void)_unitsOfMeasureWereChanged:(id)aChanged
{
	[self _refreshDockWeather];
}

- (void)_dockEntryWasSelected:(id)aSelected
{
	[NSApp activateIgnoringOtherApps:YES];
	[_mainWindowController showDetailsForEntry:_dockEntry openPane:YES openInfoForBillboard:1];
}

- (void)_selectHomeCityFromDock:(id)aDock
{
	[_preferencesController showSelectCityDialog];
}

- (void)_updateDockWeather:(id)aWeather
{
	CWManager::instance()->resetWeatherInternetState();
	_dockEntry->manualUpdateOfScheduledReports();
}

// Only the current conditions report matters.
- (void)_dockReportsWereChanged:(id)aChanged
{
	LNotification *notification = lobj_cast<LNotification>([aChanged getLObject]);
	if (UMapParsingUtils::loadIntFromObject(notification->userInfo()->objectForKey(CWOnlineCityData::kReportIDKey)) == kReportTypeCurrent)
		[self _refreshDockWeather];
}

- (void)_homeCityWasChanged:(id)aChanged
{
	PointEntry *home = Preferences::homeCity();
	if (home == _dockEntry)
		return;
	[self _unscheduleDockEntry];
	if (_dockEntry) {
		_dockEntry->release();
		_dockEntry = 0;
	}
	if (home)
		home->retain();
	_dockEntry = home;
	[self _scheduleDockEntry];
	[self _refreshDockWeather];
}

@end
