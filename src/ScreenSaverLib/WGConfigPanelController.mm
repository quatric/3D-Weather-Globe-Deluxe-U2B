// WGConfigPanelController: the saver's configure sheet (ConfigureSheet.nib):
// the point of view (outer space or satellite, each with its own settings
// view and speed slider) and the options, collapsed below a disclosure
// button.

#import <Cocoa/Cocoa.h>
#include "ScreenSaverLib.h"

@implementation WGConfigPanelController

- (id)init
{
	return [self initWithWindowNibName:@"ConfigureSheet"];
}

static void _setCellState(NSMatrix *matrix, NSInteger tag, bool on)
{
	[[matrix cellWithTag:tag] setState:on ? NSControlStateValueOn : NSControlStateValueOff];
}

static bool _cellIsOn(NSMatrix *matrix, NSInteger tag)
{
	return [[matrix cellWithTag:tag] state] == NSControlStateValueOn;
}

static bool _isOn(NSButton *button)
{
	return [button state] == NSControlStateValueOn;
}

// Tag 0 is outer space (mode 1), tag 1 the satellite.
- (void)_updateUIFromDefaults
{
	[_targetMatrix selectCellWithTag:Preferences::screenSaverPointOfViewMode() != 1];
	_setCellState(_satelliteOptionsMatrix, 0, Preferences::animatedWaterEnabled());
	_setCellState(_satelliteOptionsMatrix, 1, Preferences::drawWithReliefEnabled());
	_setCellState(_satelliteOptionsMatrix, 2, Preferences::screenSaverDisplayRegionNamesEnabled());
	_setCellState(_satelliteOptionsMatrix, 3, Preferences::screenSaverDisplayMiniMapEnabled());
	[_zoomSlider setFloatValue:Preferences::screenSaverCustomAltitudeValue()];
	[_globeTransparencySlider setFloatValue:Preferences::globeTransparencyLevel()];
	[_tiltTheEarthButt setState:Preferences::screenSaverTiltModeEnabled() ? NSControlStateValueOn : NSControlStateValueOff];
	[_flySpinSpeedSlider setFloatValue:Preferences::automoveSpeedSliderValue()];
	[_cloudsHistoryButt setState:Preferences::cloudsAnimationQuality() != kCloudsAnimationNone];
	[_planetNamesButt setState:Preferences::showPlanetsNamesEnabled() && Preferences::showConstellationsNamesEnabled()];
	[_useMetricUnitsButt setFloatValue:Preferences::usesMetricSystem()];
	_setCellState(_optionsMatrix, 0, Preferences::sunEnabled());
	_setCellState(_optionsMatrix, 2, Preferences::moonEnabled());
	_setCellState(_optionsMatrix, 4, Preferences::starsEnabled() && Preferences::planetsEnabled());
	_setCellState(_optionsMatrix, 5, Preferences::showCapitalPoints());
	_setCellState(_optionsMatrix, 6, Preferences::constellationsEnabled());
	_setCellState(_optionsMatrix, 7, Preferences::screenSaverBillboardsEnabled());
	_setCellState(_optionsMatrix, 8, Preferences::dayNightIsEnabled());
	_setCellState(_optionsMatrix, 9, Preferences::cloudsEnabled());
}

- (void)_updateDefaultsFromUI
{
	Preferences::setScreenSaverPointOfViewMode((TScreenSaverPointOfViewMode)([_targetMatrix selectedTag] == 0));
	Preferences::setAnimatedWaterEnabled(_cellIsOn(_satelliteOptionsMatrix, 0));
	Preferences::setDrawWithReliefEnabled(_cellIsOn(_satelliteOptionsMatrix, 1));
	Preferences::setScreenSaverDisplayRegionNamesEnabled(_cellIsOn(_satelliteOptionsMatrix, 2));
	Preferences::setScreenSaverDisplayMiniMapEnabled(_cellIsOn(_satelliteOptionsMatrix, 3));
	Preferences::setScreenSaverCustomAltitudeValue([_zoomSlider floatValue]);
	Preferences::setGlobeTransparencyLevel([_globeTransparencySlider floatValue]);
	Preferences::setScreenSaverTiltModeEnabled(_isOn(_tiltTheEarthButt));
	Preferences::setAutomoveSpeedSliderValue([_flySpinSpeedSlider floatValue]);
	Preferences::setCloudsAnimationQuality(_isOn(_cloudsHistoryButt) ? kCloudsAnimationHigh : kCloudsAnimationNone);
	Preferences::setShowPlanetsNamesEnabled(_isOn(_planetNamesButt));
	Preferences::setShowConstellationsNamesEnabled(_isOn(_planetNamesButt));
	Preferences::useMetricSystem(_isOn(_useMetricUnitsButt));
	Preferences::setSunEnabled(_cellIsOn(_optionsMatrix, 0));
	Preferences::setMoonEnabled(_cellIsOn(_optionsMatrix, 2));
	Preferences::setStarsEnabled(_cellIsOn(_optionsMatrix, 4));
	Preferences::setPlanetsEnabled(_cellIsOn(_optionsMatrix, 4));
	Preferences::setShowCapitalPoints(_cellIsOn(_optionsMatrix, 5));
	Preferences::setConstellationsEnabled(_cellIsOn(_optionsMatrix, 6));
	Preferences::setScreenSaverBillboardsEnabled(_cellIsOn(_optionsMatrix, 7));
	Preferences::setDayNightEnabled(_cellIsOn(_optionsMatrix, 8));
	Preferences::setCloudsEnabled(_cellIsOn(_optionsMatrix, 9), 1);
	LUserDefaults::standardUserDefaults()->synchronize();
}

- (void)windowDidLoad
{
	_flySpinSpeedSlider = _flySpeedSlider;
	[_optionsContainerView addSubview:_optionsView];
	_placeHolderViewHeight = (float)[_optionsPlaceHolderView frame].size.height;
	[self _updateUIFromDefaults];
	[self _validateSettingsView];
	[_collapseButt setState:NSControlStateValueOff];
	[self toggleAdvancedOptions:_collapseButt];
}

// OK (tag 1) saves the settings, Cancel puts the saved ones back.
- (IBAction)configDone:(id)sender
{
	[NSApp endSheet:[self window]];
	if ([sender tag] == 1)
		[self _updateDefaultsFromUI];
	else
		[self _updateUIFromDefaults];
}

- (IBAction)selectTarget:(id)sender
{
	[self _validateSettingsView];
}

// Grows or shrinks the sheet by the options' height, keeping its top.
- (IBAction)toggleAdvancedOptions:(id)sender
{
	NSWindow *window = [self window];
	NSRect frame = [window frame];
	if ([sender state] == NSControlStateValueOn) {
		frame.size.height += _placeHolderViewHeight;
		frame.origin.y -= _placeHolderViewHeight;
	} else {
		frame.size.height -= _placeHolderViewHeight;
		frame.origin.y += _placeHolderViewHeight;
	}
	BOOL visible = [window isVisible];
	[window setFrame:frame display:visible animate:visible];
}

// Shows the settings view of the selected point of view, centred at the
// top of the container, and moves the speed over to its slider.
- (void)_validateSettingsView
{
	float speed = [_flySpinSpeedSlider floatValue];
	NSView *view = nil;
	NSInteger tag = [_targetMatrix selectedTag];
	if (tag == 0) {
		view = _outerSpaceView;
		_flySpinSpeedSlider = _spinningSpeedSlider;
	} else if (tag == 1) {
		view = _satelliteView;
		_flySpinSpeedSlider = _flySpeedSlider;
	}
	if (view == _currentSettingsView)
		return;
	[_flySpinSpeedSlider setFloatValue:speed];
	[_currentSettingsView removeFromSuperview];
	NSRect frame = [view frame];
	NSRect bounds = [_containerView bounds];
	NSPoint origin = NSMakePoint(ceil((bounds.size.width - frame.size.width) * 0.5) + bounds.origin.x,
	                             bounds.origin.y + bounds.size.height - frame.size.height);
	[view setFrameOrigin:origin];
	[_containerView addSubview:view];
	_currentSettingsView = view;
}

@end
