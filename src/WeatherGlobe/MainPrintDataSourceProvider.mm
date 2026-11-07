#import "MainPrintDataSourceProvider.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "WGCurrentSetController.h"
#import "WGEntryDetailsController.h"
#import "MainViewPrintDataSource.h"
#import "RouteSetPrintDataSource.h"
#import "LocationPrintDataSource.h"
#import "WeatherPaletteExtPrintDataSource.h"
#import "WGHelpBook.h"
#include "AMShapes.h"

// The main window's print dialog: what to print (the main view, the
// current route, the selected location and its weather) and, in a settings
// palette, what the main view shows.
@implementation MainPrintDataSourceProvider

- (BOOL)_doPrintRoute
{
	RouteObject *route = [[[[AtlasController atlasController] mainWindowController] currentSetController]
		currentRoute];
	return route && route->locationsCount() != 0;
}

// Whether the main view would print anything besides an empty map.
- (BOOL)_isNeedToPrint
{
	return [_optionMapTerrain state] == NSControlStateValueOn || [_optionCountryNames state] == NSControlStateValueOn
		|| Preferences::showWeatherOnMapEnabled() || Preferences::showAllPoints() || Preferences::gridEnabled()
		|| (Preferences::mapMode() == 0 && Preferences::spaceEnabled())
		|| (Preferences::mapMode() == 1 && Preferences::cloudsEnabled())
		|| [self _doPrintBorders] || [self _doPrintRoute];
}

- (void)_validatePrintButton
{
	[_printButton setEnabled:[_printCurrentRouteBtn state] == NSControlStateValueOn
		|| [_printLocationsBtn state] == NSControlStateValueOn || [_printMainViewBtn state] == NSControlStateValueOn
		|| [_printLocationsWeatherBtn state] == NSControlStateValueOn];
}

- (void)dealloc
{
	[_previousTitleFieldContent release];
	[_locationWeatherPrintDS release];
	[_printDialog release];
	[_settingsPalette release];
	[super dealloc];
}

- (id)nibName
{
	return @"PrintDialog";
}

- (void)windowWillClose:(id)aClose
{
	if ([aClose object] == _settingsPalette)
		[NSApp stopModal];
}

- (id)printDataSources
{
	NSMutableArray *sources = [NSMutableArray array];
	if ([_printMainViewBtn state] == NSControlStateValueOn) {
		MainViewPrintDataSource *source = [[MainViewPrintDataSource new] autorelease];
		[source setPrintMapTerrain:[_optionMapTerrain state] == NSControlStateValueOn];
		[source setPrintCountryNames:[_optionCountryNames state] == NSControlStateValueOn];
		[sources addObject:source];
	}
	if ([_printCurrentRouteBtn state] == NSControlStateValueOn)
		[sources addObject:[[RouteSetPrintDataSource new] autorelease]];
	if ([_printLocationsBtn state] == NSControlStateValueOn)
		[sources addObject:[[LocationPrintDataSource new] autorelease]];
	if ([_printLocationsWeatherBtn state] == NSControlStateValueOn && _locationWeatherPrintDS)
		[sources addObject:_locationWeatherPrintDS];
	return sources;
}

- (id)hostWindowForPrintDialog
{
	return [[[AtlasController atlasController] mainWindowController] mainWindow];
}

- (id)printJobTitle
{
	return [_printTitle stringValue];
}

- (void)setMainViewBtnState:(NSInteger)aState
{
	[self loadNibIfNeeded];
	[_printMainViewBtn setState:aState];
}

- (void)setCurrentRouteBtnState:(NSInteger)aState
{
	[self loadNibIfNeeded];
	[_printCurrentRouteBtn setState:aState];
}

- (void)setLocationInfoBtnState:(NSInteger)aState
{
	[self loadNibIfNeeded];
	[_printLocationsBtn setState:aState];
}

- (void)setDoShowCustomDialog:(BOOL)aDialog
{
	_doNotShowCustomDialog = !aDialog;
}

- (void)setPrintTitle:(id)aTitle
{
	[self loadNibIfNeeded];
	[_printTitle setStringValue:aTitle];
}

// PrintController runs the dialog as a sheet on the main window, or modally.
- (void)endPrintDialogWithCode:(NSInteger)aCode
{
	// End the sheet before closing it: closing an attached sheet ends it
	// first, with a code that isn't aCode (the original closed first, which
	// the old -[NSApp endSheet:returnCode:] allowed).
	NSWindow *parent = [_printDialog sheetParent];
	if (parent)
		[parent endSheet:_printDialog returnCode:aCode];
	[_printDialog close];
	if (!parent)
		[NSApp stopModalWithCode:aCode];
}

- (void)_openSettingsPalette
{
	[NSApp runModalForWindow:_settingsPalette];
}

- (void)_closeSettingsPalette
{
	[_settingsPalette close];
	[NSApp stopModal];
}

- (void)cancelSettingsButton:(id)aButton
{
	[self _closeSettingsPalette];
}

- (void)okSettingsButton:(id)aButton
{
	[self _closeSettingsPalette];
}

- (void)customizeButton:(id)aButton
{
	[self _openSettingsPalette];
}

- (void)cancelButton:(id)aButton
{
	[self endPrintDialogWithCode:0];
}

- (void)printButton:(id)aButton
{
	[self endPrintDialogWithCode:1];
}

- (void)helpButton:(id)aButton
{
	[WGHelpBook gotoPage:@"topics/printing.html" anchor:nil];
}

- (void)settingsHelpButton:(id)aButton
{
	[WGHelpBook gotoPage:@"topics/printing.html" anchor:nil];
}

// At most 100 characters, on one line.
- (BOOL)_isTitleValid
{
	NSString *title = [_printTitle stringValue];
	if ([title length] > 100)
		return NO;
	return [title rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"\n\r\x03"]].location
		== NSNotFound;
}

- (void)_applyTitleTextUndo:(id)aUndo
{
	[_printTitle setStringValue:aUndo];
	[self controlTextDidChange:nil];
}

// Each edit of the title is undoable; an invalid one is taken back.
- (void)controlTextDidChange:(id)aChange
{
	NSString *title = [_printTitle stringValue];
	if ([title isEqualToString:_previousTitleFieldContent])
		return;
	NSString *previous = _previousTitleFieldContent ? [NSString stringWithString:_previousTitleFieldContent] : @"";
	if (![self _isTitleValid]) {
		[_printTitle setStringValue:previous];
		NSBeep();
		return;
	}
	NSUndoManager *undoManager = [_printDialog undoManager];
	[undoManager registerUndoWithTarget:self selector:@selector(_applyTitleTextUndo:) object:previous];
	[undoManager setActionName:@""];
	[_previousTitleFieldContent release];
	_previousTitleFieldContent = [[NSString alloc] initWithString:title];
}

// The "main view" checkbox (tag 1) switches its options.
- (void)printOptionsCheckbox:(id)aCheckbox
{
	if ([aCheckbox tag] == 1) {
		BOOL on = [aCheckbox state] == NSControlStateValueOn;
		if (on && ![self _isNeedToPrint])
			[_optionMapTerrain setState:NSControlStateValueOn];
		[_optionMapTerrain setEnabled:on];
		[_optionCountryNames setEnabled:on && [self _doPrintBorders]];
	}
	[self _validatePrintButton];
}

- (void)printCustomOptionsCheckbox:(id)aCheckbox
{
	if ([aCheckbox state] == NSControlStateValueOn)
		[_printMainViewBtn setState:NSControlStateValueOn];
	else if (![self _isNeedToPrint]) {
		[_optionMapTerrain setEnabled:NO];
		[_optionCountryNames setEnabled:NO];
		[_printMainViewBtn setState:NSControlStateValueOff];
	}
	[self _validatePrintButton];
}

- (void)locationWeatherDataSourceDidEndWeatherPrepearing
{
	[_prepearingEndTarget performSelector:_prepearingEndSel];
}

- (id)customPrintDialog
{
	if (_doNotShowCustomDialog)
		return nil;
	[self loadNibIfNeeded];
	MainWindowController *window = [[AtlasController atlasController] mainWindowController];
	RouteObject *route = [[window currentSetController] currentRoute];
	BOOL noRoute = !route || route->locationsCount() < 1;
	GenericEntry *entry = [[window entryDetailsController] currentEntry];
	[_printCurrentRouteBtn setEnabled:!noRoute];
	[_printLocationsBtn setEnabled:entry != 0];
	[_printLocationsWeatherBtn setEnabled:Preferences::usesInternet() && lobj_cast<PointEntry>(entry)];
	[_printLocationsWeatherBtn setState:NSControlStateValueOff];
	[_optionCountryNames setEnabled:[self _doPrintBorders]];
	[_optionCountryNames setState:NSControlStateValueOff];
	[_optionMapTerrain setState:NSControlStateValueOn];
	[self _validatePrintButton];
	return _printDialog;
}

// The selected location's weather is downloaded first when it is printed.
- (void)preparePrintingDidEndSelector:(SEL)aSelector target:(id)aTarget
{
	_prepearingEndSel = aSelector;
	_prepearingEndTarget = aTarget;
	if ([_printLocationsWeatherBtn state] == NSControlStateValueOn) {
		PointEntry *entry = lobj_cast<PointEntry>([[[[AtlasController atlasController] mainWindowController]
			entryDetailsController] currentEntry]);
		if (entry) {
			LArray *entries = LArray::arrayWithCapacity(1);
			entries->addObject(entry);
			[_locationWeatherPrintDS release];
			_locationWeatherPrintDS = [[WeatherPaletteExtPrintDataSource alloc]
				initWithEntriesList:(TLArray<PointEntry *> *)entries];
			[_locationWeatherPrintDS prepareWeatherDidEndSelector:@selector(locationWeatherDataSourceDidEndWeatherPrepearing)
				target:self];
			return;
		}
	}
	[self locationWeatherDataSourceDidEndWeatherPrepearing];
}

// Country names print only when country or state borders are shown.
- (BOOL)_doPrintBorders
{
	LComparableSet *shapes = Preferences::shapesIDsToDraw();
	return Preferences::showAllBorders() && (shapes->containsObject(CShapeManager::kShapeIDCountries)
		|| shapes->containsObject(CShapeManager::kShapeIDStates));
}

@end
