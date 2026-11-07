#import "WGSettingsPaletteController.h"
#import "WGCustomSlider.h"

// The floating settings palette: transparency, spin speed, marker size and
// the display switches, kept in step with the preferences.
@implementation WGSettingsPaletteController

- (void)_setTextColor:(id)aColor forCell:(id)aCell
{
	NSMutableAttributedString *title = [[[NSMutableAttributedString alloc] initWithAttributedString:[aCell attributedTitle]] autorelease];
	[title addAttribute:NSForegroundColorAttributeName value:aColor range:NSMakeRange(0, [title length])];
	[aCell setAttributedTitle:title];
}

- (void)_setupCustomSlider:(id)aSlider
{
	[aSlider setKnobImage:[NSImage imageNamed:@"MWT_sliderknob"]];
	[aSlider setKnobDisabledImage:[NSImage imageNamed:@"MWT_sliderknob_disabl"]];
	[aSlider setKnobAlternateImage:[NSImage imageNamed:@"MWT_sliderknob_on"]];
	[aSlider setBarFirstEdgeImage:[NSImage imageNamed:@"MWT_sliderback_left"]];
	[aSlider setBarMiddleImage:[NSImage imageNamed:@"MWT_sliderback_center"]];
	[aSlider setBarLastEdgeImage:[NSImage imageNamed:@"MWT_sliderback_right"]];
	[aSlider setBarDisabledFirstEdgeImage:[NSImage imageNamed:@"MWT_sliderback_left_disabl"]];
	[aSlider setBarDisabledMiddleImage:[NSImage imageNamed:@"MWT_sliderback_center_disabl"]];
	[aSlider setBarDisabledLastEdgeImage:[NSImage imageNamed:@"MWT_sliderback_right_disabl"]];
}

- (void)_validateControls
{
	bool globe = Preferences::mapMode() == 0;
	bool markers = Preferences::showRouteMarkers();
	[_globeTransparency setFloatValue:Preferences::globeTransparencyLevel()];
	[_globeTransparency setEnabled:globe];
	[_spinSpeed setFloatValue:Preferences::automoveSpeedSliderValue()];
	[_markerSize setFloatValue:Preferences::markersScale()];
	[_markerSize setEnabled:markers];
	[_locationNames setState:Preferences::showLocationNamesEnabled() ? NSControlStateValueOn : NSControlStateValueOff];
	[_showRouteLines setState:Preferences::showRouteLines() ? NSControlStateValueOn : NSControlStateValueOff];
	[_labelOptions setEnabled:[_locationNames state] == NSControlStateValueOn];
	[_labelOptions selectCellWithTag:Preferences::showLocationNamesNearestOnly() ? 0 : 1];
	[_showPushpins setState:markers ? NSControlStateValueOn : NSControlStateValueOff];
	[_highlightBoundaries setState:Preferences::highlightBordersAnyway() ? NSControlStateValueOn : NSControlStateValueOff];
	[_showNearestPoint setState:Preferences::showNearestPoint() ? NSControlStateValueOn : NSControlStateValueOff];
}

- (id)nibName
{
	return @"SettingsPalette";
}

- (id)autosaveName
{
	return [self className];
}

- (void)awakeFromNib
{
	[super awakeFromNib];
	NSColor *color = [[NSColor whiteColor] colorWithAlphaComponent:0.95f];
	[self _setTextColor:color forCell:[_locationNames cell]];
	[self _setTextColor:color forCell:[_showRouteLines cell]];
	[self _setTextColor:color forCell:[_showPushpins cell]];
	[self _setTextColor:color forCell:[_labelOptions cellWithTag:0]];
	[self _setTextColor:color forCell:[_labelOptions cellWithTag:1]];
	[self _setTextColor:color forCell:[_highlightBoundaries cell]];
	[self _setTextColor:color forCell:[_showNearestPoint cell]];
	[self _setupCustomSlider:_globeTransparency];
	[self _setupCustomSlider:_markerSize];
	[self _setupCustomSlider:_spinSpeed];
	[self _validateControls];
	LNotificationCenter::center()->addObserver(self, @selector(_validateControls), Preferences::kDrawOptionsWereChanged, 0);
}

- (void)dealloc
{
	LNotificationCenter::center()->removeObserver(self);
	[super dealloc];
}

- (void)controlsAction:(id)aAction
{
	switch ([aAction tag]) {
	case 1:
		Preferences::setGlobeTransparencyLevel([aAction floatValue]);
		break;
	case 2:
		Preferences::setAutomoveSpeedSliderValue([aAction floatValue]);
		break;
	case 3:
		Preferences::setMarkersScale([aAction floatValue]);
		break;
	case 4:
		Preferences::setShowLocationNamesEnabled([aAction state] == NSControlStateValueOn);
		break;
	case 6:
		Preferences::setShowRouteLines([aAction state] == NSControlStateValueOn);
		break;
	case 7:
		Preferences::setShowLocationNamesNearestOnly([[aAction keyCell] tag] == 0);
		break;
	case 8:
		Preferences::setShowRouteMarkers([aAction state] == NSControlStateValueOn);
		break;
	case 9:
		Preferences::setHighlightBordersAnyway([aAction state] == NSControlStateValueOn);
		break;
	case 10:
		Preferences::setShowNearestPoint([aAction state] == NSControlStateValueOn);
		break;
	}
}

@end
