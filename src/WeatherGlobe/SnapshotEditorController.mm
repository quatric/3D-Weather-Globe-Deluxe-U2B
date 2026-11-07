#import "SnapshotEditorController.h"
#import "AtlasController.h"
#import "MainWindowController.h"

// A palette for making screenshots: custom weather, mountain height and the
// view saved to or loaded from a plist.
@implementation SnapshotEditorController

- (id)nibName
{
	return @"SnapshotEditor";
}

- (id)autosaveName
{
	return [self className];
}

- (void)awakeFromNib
{
	[_cloudsTypePopupButton removeAllItems];
	for (int code = 0; code < kSkyCodesCount; code++)
		[_cloudsTypePopupButton addItemWithTitle:[NSString stringReferenceFromLString:
			skyTypeNameFromSkyCode((TSkyCode)code)]];
	[_precipTypePopupButton removeAllItems];
	for (int code = 0; code < kPrecipCodesCount; code++)
		[_precipTypePopupButton addItemWithTitle:[NSString stringReferenceFromLString:
			precipTypeNameFromPrecipCode((TPrecipitationCode)code)]];
}

- (AtlasPanelController *)_currentPanelController
{
	AppWindowInterfaceWrapper *window = [[[AtlasController atlasController] mainWindowController] windowInterface];
	return window ? window->currentPanel() : 0;
}

// The controls' weather replaces the traced one while the button is on.
- (void)_setupWeather
{
	WeatherTracer *tracer = [self _weatherTracer];
	if (!tracer)
		return;
	BOOL custom = [_overrideWeatherButton state] == NSControlStateValueOn;
	tracer->setUseCustomWeather(custom);
	if (!custom)
		return;
	TSkyCode sky = (TSkyCode)[_cloudsTypePopupButton indexOfSelectedItem];
	tracer->setCustomSkyState(sky, sky, 0.0f);
	SPrecipitationDescr precip((TPrecipitationCode)[_precipTypePopupButton indexOfSelectedItem],
		[_precipInensitySlider floatValue]);
	tracer->setCustomPrecipitations(&precip, &precip, 0.0f);
	SWeatherInfo info;
	info.temperature = 0.0f;
	info.pressure = 0.0f;
	info.humidity = 0.0f;
	info.windSpeed = [_windSpeedSlider floatValue];
	info.wind = windDirFromWindAngle([_windDirSlider floatValue]) * info.windSpeed;
	tracer->setCustomWeatherInfo(&info);
}

- (void)_saveFrame
{
	NSSavePanel *panel = [NSSavePanel savePanel];
	[panel setExtensionHidden:YES];
	[panel setCanSelectHiddenExtension:YES];
	[panel setAllowedFileTypes:[NSArray arrayWithObjects:@"plist", nil]];
	[panel setNameFieldStringValue:@"snapshot"];
	if ([panel runModal] != NSModalResponseOK)
		return;
	AtlasPanelController *controller = [self _currentPanelController];
	if (controller)
		controller->collectSnapshotInfo()->writeToFile([[[panel URL] path] LString]);
}

- (void)controlAction:(id)aAction
{
	switch ([aAction tag]) {
	case 1: case 2: case 3: case 4: case 5: case 6:
		[self _setupWeather];
		break;
	case 7:
		Preferences::setVerticalScaleFactor([aAction floatValue]);
		break;
	case 8:
		[self _loadFrame];
		break;
	case 9:
		[self _saveFrame];
		break;
	}
}

// Only the perspective panel traces weather.
- (WeatherTracer *)_weatherTracer
{
	AtlasPanelController *controller = [self _currentPanelController];
	if (controller && controller->getClass()->isSubclassOf(LF_CLASSOF(PerspPanelController)))
		return ((PerspPanelController *)controller)->weatherTracer();
	return 0;
}

- (void)_loadFrame
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseDirectories:NO];
	[panel setCanChooseFiles:YES];
	[panel setAllowedFileTypes:[NSArray arrayWithObjects:@"plist", nil]];
	if ([panel runModal] != NSModalResponseOK)
		return;
	AtlasPanelController *controller = [self _currentPanelController];
	if (!controller)
		return;
	LMap *info = LMap::mapWithContentsOfFile([[[panel URL] path] LString]);
	if (info) {
		controller->installSnapshotInfo(info);
		[self _updateControls];
		[[[AtlasController atlasController] timeLineController] showPalette];
	}
}

- (void)_updateControls
{
	[_mountainsHeightSlider setFloatValue:Preferences::verticalScaleFactor()];
	WeatherTracer *tracer = [self _weatherTracer];
	if (!tracer)
		return;
	BOOL custom = tracer->_useCustomWeather;
	[_overrideWeatherButton setState:custom ? NSControlStateValueOn : NSControlStateValueOff];
	if (!custom)
		return;
	TSkyCode sky, nextSky;
	float weight, nextWeight;
	tracer->getSkyState(&sky, &weight, &nextSky, &nextWeight);
	[_cloudsTypePopupButton selectItemAtIndex:sky];
	SPrecipitationDescr precip, nextPrecip;
	tracer->getPrecipitations(&precip, &weight, &nextPrecip, &nextWeight);
	[_precipTypePopupButton selectItemAtIndex:precip.code];
	[_precipInensitySlider setFloatValue:precip.intensity];
	const SWeatherInfo *info = tracer->currentWeatherInfo();
	if (info) {
		[_windSpeedSlider setFloatValue:info->windSpeed];
		[_windDirSlider setFloatValue:windAngleFromWindDir(info->wind / info->windSpeed)];
	}
}

@end
