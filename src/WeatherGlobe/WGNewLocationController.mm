#import "WGNewLocationController.h"
#import "WGCompassButton.h"
#import "WGTexturedView.h"
#import "WGTexturedButton.h"
#import "WGFilledView.h"
#import "WGCustomPopUpButtonCell.h"
#import "WGHelpBook.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "WGMainEntriesListController.h"
#import "NSMutableString+NSStringWGExtensions.h"
#import "NSButton+NSButtonWGExt.h"

NSString *_WGNewLocationCoordsPickerModeChangedNotification = @"WGNewLocationCoordsPickerModeChanged";

// The "New Location" palette: a name, latitude/longitude typed in decimal or
// degrees-minutes-seconds (or picked on the map with the compass center),
// and a time zone. The compass corners (zones 2-5: NW, NE, SW, SE) set the
// hemispheres.
@implementation WGNewLocationController

- (id)nibName
{
	return @"NewLocationDialog";
}

- (void)dealloc
{
	NSMenu *menu = [_timeZonesPopup menu];
	for (NSInteger i = 0; i < [menu numberOfItems]; i++) {
		LObject *zone = [[[menu itemAtIndex:i] representedObject] getLObject];
		if (zone)
			zone->release();
	}
	if (_coordsParser) {
		_coordsParser->release();
		_coordsParser = 0;
	}
	[_customLocationEditor release];
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	[super dealloc];
}

- (void)windowWillClose:(id)aClose
{
	[self setCoordsPickerEnabled:NO];
	[super windowWillClose:aClose];
}

- (BOOL)isCoordsPickerEnabled
{
	return [_compassButton isCenterZoneActive];
}

- (void)setCoordsPickerEnabled:(BOOL)aEnabled
{
	[_compassButton setCenterZoneActive:aEnabled];
}

- (void)setGeoCoords:(LPoint)aCoords
{
	_latitude = aCoords.y;
	_longitude = aCoords.x;
	[self refreshWorldPartButtons];
	[self refreshEditFields];
	[_addLocationButton setEnabled:[self validateCoordinates]];
}

- (void)compassButton:(id)aButton
{
	NSInteger zone = [aButton intValue];
	if (zone != 1)
		[self parseCoordIfTextBoxBeingEdited];
	switch (zone) {
	case 1:
		[[NSNotificationCenter defaultCenter] postNotificationName:_WGNewLocationCoordsPickerModeChangedNotification
			object:self userInfo:nil];
		return;
	case 2:	// NW
		if (_latitude < 0.0f)
			_latitude = -_latitude;
		if (_longitude > 0.0f)
			_longitude = -_longitude;
		break;
	case 3:	// NE
		if (_latitude < 0.0f)
			_latitude = -_latitude;
		if (_longitude < 0.0f)
			_longitude = -_longitude;
		break;
	case 4:	// SW
		if (_latitude > 0.0f)
			_latitude = -_latitude;
		if (_longitude > 0.0f)
			_longitude = -_longitude;
		break;
	case 5:	// SE
		if (_latitude > 0.0f)
			_latitude = -_latitude;
		if (_longitude < 0.0f)
			_longitude = -_longitude;
		break;
	}
	[self refreshEditFields];
}

- (void)degreesModeChanged:(id)aChanged
{
	[self parseCoordIfTextBoxBeingEdited];
	[self refreshEditFields];
}

- (void)coordEditorAction:(id)aAction
{
	if (aAction == _latitudeField || aAction == _longitudeField)
		[self parseCoordinate:[aAction stringValue] forTextField:aAction doUIRefresh:YES];
}

- (void)onHelpButton:(id)aButton
{
	[WGHelpBook gotoPage:@"topics/cust_loc.html" anchor:nil];
}

- (void)setupCellForPopupButton:(id)aButton
{
	WGCustomPopUpButtonCell *cell = [[WGCustomPopUpButtonCell alloc] initWithPopUpButtonCell:[aButton cell]];
	[cell setImage:[NSImage imageNamed:@"Popup_left_off.tiff"] forPosition:0];
	[cell setImage:[NSImage imageNamed:@"Popup_center_off.tiff"] forPosition:1];
	[cell setImage:[NSImage imageNamed:@"Popup_control_off.tiff"] forPosition:2];
	[aButton setCell:[cell autorelease]];
}

// At 0 the hemisphere is the one the field shows: a world-part letter right
// after the seconds mark.
- (void)refreshWorldPartButtons
{
	NSBundle *bundle = [NSBundle mainBundle];
	NSInteger latitudePart, longitudePart;
	if (_latitude > 0.0f)
		latitudePart = 0;
	else if (_latitude < 0.0f)
		latitudePart = 1;
	else {
		NSString *text = [[_latitudeField stringValue] uppercaseString];
		NSUInteger south = [text rangeOfString:[bundle localizedStringForKey:@"S" value:@"" table:nil]].location;
		NSUInteger seconds = [text rangeOfString:[bundle localizedStringForKey:@"SecondsCharacter" value:@""
			table:nil]].location;
		latitudePart = south == seconds + 1 ? 1 : 0;
	}
	if (_longitude > 0.0f)
		longitudePart = 2;
	else if (_longitude < 0.0f)
		longitudePart = 3;
	else {
		NSString *text = [[_longitudeField stringValue] uppercaseString];
		NSUInteger west = [text rangeOfString:[bundle localizedStringForKey:@"W" value:@"" table:nil]].location;
		NSUInteger seconds = [text rangeOfString:[bundle localizedStringForKey:@"SecondsCharacter" value:@""
			table:nil]].location;
		longitudePart = west == seconds + 1 ? 3 : 2;
	}
	[self selectOnCompassLatitude:latitudePart longitude:longitudePart];
}

- (void)parseCoordIfTextBoxBeingEdited
{
	id responder = [_palette firstResponder];
	if (![responder isKindOfClass:[NSView class]])
		return;
	NSTextField *field;
	if ([responder isDescendantOf:_latitudeField])
		field = _latitudeField;
	else if ([responder isDescendantOf:_longitudeField])
		field = _longitudeField;
	else
		return;
	[self parseCoordinate:[field stringValue] forTextField:field doUIRefresh:NO];
}

- (BOOL)validateCoordinates
{
	return !(_latitude > 90.0f || _latitude < -90.0f || _longitude > 180.0f || _longitude < -180.0f);
}

- (id)windowWillReturnFieldEditor:(id)aEditor toObject:(id)aObject
{
	return [aObject isKindOfClass:[NSTextField class]] ? _customLocationEditor : nil;
}

- (void)textFieldDidChange:(id)aChange
{
	id field = [aChange object];
	if (field == _locationNameField)
		_isLocationNameEntered = [[field stringValue] length] != 0;
}

// A pasted string with anything but digits and "-" is parsed whole.
- (BOOL)wgCustomLocationEditor:(id)aEditor pasteString:(id)aString
{
	id field = [aEditor delegate];
	if (![field isKindOfClass:[NSTextField class]] || (field != _latitudeField && field != _longitudeField))
		return NO;
	NSMutableCharacterSet *plain = [[[NSCharacterSet decimalDigitCharacterSet] mutableCopy] autorelease];
	[plain addCharactersInString:@"-"];
	[plain invert];
	if (![aString rangeOfCharacterFromSet:plain].length)
		return NO;
	[self parseCoordinate:aString forTextField:field doUIRefresh:YES];
	return YES;
}

- (BOOL)isCoordsModeDMS
{
	return [_coordsInputModePopup selectedTag] == 1;
}

- (void)parseNullCoordinate:(id)aCoordinate
{
	NSString *zero = @"0.0";
	if ([self isCoordsModeDMS])
		zero = [NSString stringWithFormat:@"0%@",
			[[NSBundle mainBundle] localizedStringForKey:@"DegreesCharacter" value:@"" table:nil]];
	[self parseCoordinate:zero forTextField:aCoordinate doUIRefresh:YES];
}

- (void)nullWasEntered
{
	id responder = [_palette firstResponder];
	if (![responder isKindOfClass:[WGCustomLocationEditor class]])
		return;
	id field = [responder delegate];
	if ([field isKindOfClass:[NSTextField class]] && (field == _latitudeField || field == _longitudeField))
		[self performSelector:@selector(parseNullCoordinate:) withObject:field afterDelay:0.01];
}

- (void)_adjustTime
{
	[self timeZoneChanged:_timeZonesPopup];
}

// aLatitude: 0 north, 1 south; aLongitude: 2 east, 3 west.
- (void)selectOnCompassLatitude:(NSInteger)aLatitude longitude:(NSInteger)aLongitude
{
	NSInteger zone = aLatitude == 0 ? 2 + (aLongitude == 2) : 4 + (aLongitude == 2);
	[_compassButton setCurrentZone:zone];
}

- (void)timeZoneChanged:(id)aChanged
{
	LTimeZone *zone = lobj_cast<LTimeZone>([[[aChanged selectedItem] representedObject] getLObject]);
	NSCalendarDate *date = [NSCalendarDate calendarDate];
	[date setTimeZone:zone ? (NSTimeZone *)zone->_cf : nil];
	NSString *format = Preferences::uses24HourClock() ? @"%A, %B %d, %H:%M" : @"%A, %B %d, %I:%M %p";
	[_timeZoneText setStringValue:[date descriptionWithCalendarFormat:format]];
}

// A zero coordinate in DMS gets its world-part letter from the compass zone.
- (void)refreshEditFields
{
	if ([_coordsInputModePopup selectedTag] == 1) {
		[_latitudeField setStringValue:[NSString wgShortSingleCoordDescription:_latitude coordTypeLatitude:YES]];
		[_longitudeField setStringValue:[NSString wgShortSingleCoordDescription:_longitude coordTypeLatitude:NO]];
		NSBundle *bundle = [NSBundle mainBundle];
		if (_latitude == 0.0f) {
			NSMutableString *text = [NSMutableString stringWithString:[_latitudeField stringValue]];
			NSUInteger length = [text length];
			NSInteger zone = [_compassButton currentZone];
			NSString *part = zone == 3 || zone == 2 ? [bundle localizedStringForKey:@"N" value:@"" table:nil]
				: [bundle localizedStringForKey:@"S" value:@"" table:nil];
			[text replaceCharactersInRange:NSMakeRange(length - 1, 1) withString:part];
			[_latitudeField setStringValue:text];
		}
		if (_longitude == 0.0f) {
			NSMutableString *text = [NSMutableString stringWithString:[_longitudeField stringValue]];
			NSUInteger length = [text length];
			NSInteger zone = [_compassButton currentZone];
			NSString *part = zone == 3 || zone == 5 ? [bundle localizedStringForKey:@"E" value:@"" table:nil]
				: [bundle localizedStringForKey:@"W" value:@"" table:nil];
			[text replaceCharactersInRange:NSMakeRange(length - 1, 1) withString:part];
			[_longitudeField setStringValue:text];
		}
	} else {
		[_latitudeField setStringValue:[NSString stringWithFormat:@"%.4f", (double)_latitude]];
		[_longitudeField setStringValue:[NSString stringWithFormat:@"%.4f", (double)_longitude]];
	}
	if (!_isLocationNameEntered) {
		NSString *name = @"";
		if (_latitude != 0.0f || _longitude != 0.0f) {
			if ([_coordsInputModePopup selectedTag] == 1)
				name = [NSString wgShortGeoCoordinatesDescription:LMakePoint(_longitude, _latitude)];
			else
				name = [NSString stringWithFormat:@"%.4f / %.4f", (double)_latitude, (double)_longitude];
		}
		[_locationNameField setStringValue:name];
	}
}

- (void)awakeFromNib
{
	[super awakeFromNib];
	_coordsParser = (CoordinateParser *)CoordinateParser::_alloc()->init();
	LNotificationCenter::center()->addObserver(self, @selector(_adjustTime), Preferences::kClockFormatWasChanged, 0);
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(textFieldDidChange:)
		name:NSControlTextDidChangeNotification object:_locationNameField];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_adjustTime)
		name:@"AdjustTimeNotification" object:nil];
	[_locationNameField setCell:[[[WGLocationNameTextFieldCell alloc] initWithCell:[_locationNameField cell]]
		autorelease]];
	_customLocationEditor = [WGCustomLocationEditor new];
	[_customLocationEditor setFieldEditor:YES];
	[_customLocationEditor setPasteDelegate:self];
	WGCustomLocationFormatter *formatter = [[WGCustomLocationFormatter alloc] init];
	[formatter setDelegate:self];
	[_compassButton setCurrentZone:3];
	[_latitudeField setFormatter:[[formatter copy] autorelease]];
	[_longitudeField setFormatter:[[formatter copy] autorelease]];
	// The original leaked the template formatter.
	[formatter release];
	[_boxTexturedView setImage:[NSImage imageNamed:@"MBG_TextBlock_center"] forTexture:4];
	[_boxTexturedView setImage:[NSImage imageNamed:@"NewLoc_TextBlock_Left"] forTexture:3];
	[_boxTexturedView setImage:[NSImage imageNamed:@"NewLoc_TextBlock_Right"] forTexture:5];
	[_addLocationButton setImagePosition:NSImageOverlaps];
	[_addLocationButton setImage:[NSImage imageNamed:@"BlackButton_Left"] forPosition:0];
	[_addLocationButton setImage:[NSImage imageNamed:@"BlackButton_Center"] forPosition:1];
	[_addLocationButton setImage:[NSImage imageNamed:@"BlackButton_Right"] forPosition:2];
	[_addLocationButton setTitleColor:[NSColor whiteColor]];
	[_separatorView setFillColor:[NSColor whiteColor]];
	[_locationNameView setBorderThickness:1.125];
	[self setupCellForPopupButton:_timeZonesPopup];
	[self setupCellForPopupButton:_coordsInputModePopup];
	[_timeZonesPopup setMenu:[self timeZonePopupMenu]];
	// 10.5+ (AppKit > 824) has no plain "GMT" zone name.
	[_timeZonesPopup selectItemWithTitle:@"(GMT 00:00) Africa/Abidjan"];
	[self timeZoneChanged:_timeZonesPopup];
	[self degreesModeChanged:_coordsInputModePopup];
}

// One item per known zone, "(GMT +hh:mm) Name", sorted by the category
// below; each item's represented object wraps a retained LTimeZone.
- (id)timeZonePopupMenu
{
	LArray *names = LTimeZone::knownTimeZoneNames();
	NSMutableArray *items = [NSMutableArray arrayWithCapacity:names->count()];
	for (unsigned int i = 0; i < names->count(); i++) {
		LString *name = lobj_cast<LString>(names->objectAtIndex(i));
		LTimeZone *zone = LTimeZone::_alloc()->initWithUNIXName(name);
		if (!zone)
			continue;
		int seconds = zone->secondsFromGMT();
		NSString *title = [NSString stringWithFormat:@"(GMT %@%.2d:%.2d) %@", seconds > 0 ? @"+" : @"",
			seconds / 3600, abs((seconds / 60) % 60), (NSString *)name->cfString()];
		NSMenuItem *item = [[[NSMenuItem alloc] initWithTitle:title action:0 keyEquivalent:@""] autorelease];
		[item setRepresentedObject:(id)zone->detachNSWrapper()];
		[items addObject:item];
	}
	[items sortUsingSelector:@selector(timeZoneComparisonFunc:)];
	NSMenu *menu = [NSMenu new];
	for (NSUInteger i = 0; i < [items count]; i++)
		[menu addItem:[items objectAtIndex:i]];
	return [menu autorelease];
}

- (void)addLocationButton:(id)aButton
{
	NSString *latitude = [_latitudeField stringValue];
	NSString *longitude = [_longitudeField stringValue];
	[self parseCoordinate:latitude forTextField:_latitudeField doUIRefresh:YES];
	[self parseCoordinate:longitude forTextField:_longitudeField doUIRefresh:YES];
	if (![self validateCoordinates])
		return;
	LString *name = LString::stringWithCFString((CFStringRef)[_locationNameField stringValue]);
	LTimeZone *zone = lobj_cast<LTimeZone>([[[_timeZonesPopup selectedItem] representedObject] getLObject]);
	LString *zoneName = zone ? zone->name() : LString::emptyString();
	UserCustomEntry *entry = UserCustomEntry::entryWithGeoPointAndName(LMakePoint(_longitude, _latitude), name);
	entry->setTimeZoneName(zoneName);
	MainWindowController *window = [[AtlasController atlasController] mainWindowController];
	if ([(id)[window entriesListController] addNewEntry:entry])
		[window showDetailsForEntry:entry openPane:YES openInfoForBillboard:0];
}

// Two coordinates are latitude then longitude; one goes to the field it was
// typed in. A typed format other than the popup's switches the popup.
- (void)parseCoordinate:(id)aCoordinate forTextField:(id)aField doUIRefresh:(BOOL)aRefresh
{
	CoordinateParsingResult *result = _coordsParser->Parse(LString::stringWithCFString((CFStringRef)aCoordinate));
	TDegrees first = result->getNextCoordinate();
	bool hasSecond = result->hasNextCoordinate();
	TDegrees second = result->getNextCoordinate();
	CoordinateStringFormat format = result->getCoordinateFormat();
	if (hasSecond) {
		_latitude = floatFromDegrees(clampDegreesWithPeriod(first, 90.0f));
		_longitude = floatFromDegrees(clampDegreesWithPeriod(second, 180.0f));
	} else if (aField == _latitudeField)
		_latitude = floatFromDegrees(clampDegreesWithPeriod(first, 90.0f));
	else
		_longitude = floatFromDegrees(clampDegreesWithPeriod(first, 180.0f));
	if (aRefresh) {
		NSInteger tag = [_coordsInputModePopup selectedTag];
		if ((format == kCoordinateFormatDMS && tag == 2) || (format == kCoordinateFormatDecimal && tag == 1)) {
			[_coordsInputModePopup selectItemAtIndex:[_coordsInputModePopup indexOfItemWithTag:
				format == kCoordinateFormatDMS ? 1 : 2]];
			[self degreesModeChanged:_coordsInputModePopup];
		}
		[self refreshWorldPartButtons];
		[self refreshEditFields];
	}
	[_addLocationButton setEnabled:[self validateCoordinates]];
}

@end
