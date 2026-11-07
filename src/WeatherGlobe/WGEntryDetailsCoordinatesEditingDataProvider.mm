#import "WGEntryDetailsCoordinatesEditingDataProvider.h"
#import "WGEntryDetailsInfoPaneController.h"
#import "WGPopUpButton.h"

// Decimal degrees, else degrees, minutes and seconds; shared by all the
// coordinates rows.
static int _sIsEntryDetailsCoordinatesDecimalDegrees = 0;

// The input controls (ddInputControls): ", ", latitude, longitude.
enum { kDDLatitude = 1, kDDLongitude = 2 };
// dmsInputControls: the six unit labels, then latitude degrees, minutes,
// seconds, longitude degrees, minutes, seconds, the N/S and E/W popups.
enum {
	kDMSLatDegrees = 3, kDMSLatMinutes = 4, kDMSLatSeconds = 5,
	kDMSLonDegrees = 9, kDMSLonMinutes = 10, kDMSLonSeconds = 11,
	kDMSLatSign = 12, kDMSLonSign = 13,
};

// The coordinates row of the entry details in editing: fields for the
// latitude and longitude, in decimal degrees or degrees, minutes and
// seconds as the title popup chooses. The data object is an LArray of
// two LValues, longitude and latitude.
@implementation WGEntryDetailsCoordinatesEditingDataProvider

- (void)dealloc
{
	[_ddInputControls release];
	[_dmsInputContols release];
	[_ddFormatter release];
	[_dmsFormatter release];
	[super dealloc];
}

- (BOOL)isDescriptionEditable
{
	return YES;
}

// The latitude field. (The original took index 3 of the decimal controls
// in the other mode, out of their range: the latitude degrees field is
// meant.)
- (BOOL)tryStartDescriptionEditing
{
	NSWindow *window = [_descriptionView window];
	id field = _sIsEntryDetailsCoordinatesDecimalDegrees
		? [[self ddInputControls] objectAtIndex:kDDLatitude]
		: [[self dmsInputControls] objectAtIndex:kDMSLatDegrees];
	return [window makeFirstResponder:field];
}

// The controls' extent.
- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset
{
	NSSize size = NSZeroSize;
	NSArray *subviews = [[self descriptionView] subviews];
	int count = (int)[subviews count];
	for (int i = 0; i < count; i++) {
		NSRect frame = [[subviews objectAtIndex:i] frame];
		if (size.height < frame.size.height)
			size.height = frame.size.height;
		if (size.width < frame.origin.x + frame.size.width)
			size.width = frame.origin.x + frame.size.width;
	}
	return size;
}

- (id)descriptionView
{
	if (!_descriptionView) {
		_descriptionView = [[NSView alloc] initWithFrame:NSMakeRect(NSZeroPoint.x, NSZeroPoint.y,
			WGEntryDetailsInfoDefaultFieldSize.width, 14.0)];
		[self addInputControlsToDescriptionView];
		[self validateControls];
	}
	return _descriptionView;
}

- (void)updateDescription
{
	[self validateControls];
}

- (void)rebuildLayout:(id)aLayout
{
	[self rebuildLayout];
}

// The pane relayouts when the controls' width changes.
- (void)rebuildLayout
{
	[self addInputControlsToDescriptionView];
	NSSize size = [self descriptionSizeForSuperviewBounds:NSZeroRect titleWidth:0.0 offset:0.0];
	if (size.width != [_descriptionView frame].size.width)
		[[self owner] rebuildLayout];
}

- (id)ddInputControls
{
	if (!_ddInputControls) {
		float x = 0.0f;
		NSNumberFormatter *formatter = [self ddFormatter];
		NSNumberFormatter *latitudeFormatter = [[formatter copy] autorelease];
		// (Both limits are set as the maximum, so the minimum stays -180.)
		[latitudeFormatter setMaximum:[NSDecimalNumber numberWithInt:-90]];
		[latitudeFormatter setMaximum:[NSDecimalNumber numberWithInt:90]];
		id latitude = [self controlWithDescription:@"-000.0000" editable:YES frameXOrigin:&x formatter:latitudeFormatter];
		id comma = [self controlWithDescription:@"," editable:NO frameXOrigin:&x formatter:nil];
		id longitude = [self controlWithDescription:@"-000.0000" editable:YES frameXOrigin:&x formatter:formatter];
		_ddInputControls = [[NSArray alloc] initWithObjects:comma, latitude, longitude, nil];
	}
	return _ddInputControls;
}

static NSString *_localized(NSString *key)
{
	return [[NSBundle mainBundle] localizedStringForKey:key value:@"" table:nil];
}

// 00°00'00" N/S, 000°00'00" E/W: the labels a little apart, the popups
// closer.
- (id)dmsInputControls
{
	if (!_dmsInputContols) {
		NSNumberFormatter *formatter = [self dmsFormatter];
		float x = 0.0f;
		NSNumberFormatter *latitudeFormatter = [[formatter copy] autorelease];
		[latitudeFormatter setMaximum:[NSDecimalNumber numberWithInt:90]];
		id latDegrees = [self controlWithDescription:@"00" editable:YES frameXOrigin:&x formatter:latitudeFormatter];
		id degreesLabel = [self controlWithDescription:_localized(@"DegreesCharacter") editable:NO frameXOrigin:&x formatter:nil];
		id latMinutes = [self controlWithDescription:@"00" editable:YES frameXOrigin:&x formatter:formatter];
		x += 1.5f;
		id minutesLabel = [self controlWithDescription:_localized(@"MinutesCharacter") editable:NO frameXOrigin:&x formatter:nil];
		x += 1.5f;
		id latSeconds = [self controlWithDescription:@"00" editable:YES frameXOrigin:&x formatter:formatter];
		x += 1.5f;
		id secondsLabel = [self controlWithDescription:_localized(@"SecondsCharacter") editable:NO frameXOrigin:&x formatter:nil];
		x -= 3.0f;
		id latSign = [self popupWithItems:[NSArray arrayWithObjects:_localized(@"N"), _localized(@"S"), nil] frameXOrigin:&x];
		x += 8.0f;
		NSNumberFormatter *longitudeFormatter = [[formatter copy] autorelease];
		[longitudeFormatter setMaximum:[NSDecimalNumber numberWithInt:180]];
		id lonDegrees = [self controlWithDescription:@"000" editable:YES frameXOrigin:&x formatter:longitudeFormatter];
		id lonDegreesLabel = [self controlWithDescription:_localized(@"DegreesCharacter") editable:NO frameXOrigin:&x formatter:nil];
		id lonMinutes = [self controlWithDescription:@"00" editable:YES frameXOrigin:&x formatter:formatter];
		x += 1.5f;
		id lonMinutesLabel = [self controlWithDescription:_localized(@"MinutesCharacter") editable:NO frameXOrigin:&x formatter:nil];
		x += 1.5f;
		id lonSeconds = [self controlWithDescription:@"00" editable:YES frameXOrigin:&x formatter:formatter];
		x += 1.5f;
		id lonSecondsLabel = [self controlWithDescription:_localized(@"SecondsCharacter") editable:NO frameXOrigin:&x formatter:nil];
		x -= 3.0f;
		id lonSign = [self popupWithItems:[NSArray arrayWithObjects:_localized(@"E"), _localized(@"W"), nil] frameXOrigin:&x];
		_dmsInputContols = [[NSArray alloc] initWithObjects:degreesLabel, minutesLabel, secondsLabel,
			latDegrees, latMinutes, latSeconds, lonDegreesLabel, lonMinutesLabel, lonSecondsLabel,
			lonDegrees, lonMinutes, lonSeconds, latSign, lonSign, nil];
	}
	return _dmsInputContols;
}

static void _setZeroSymbol(NSNumberFormatter *formatter)
{
	if ([formatter respondsToSelector:@selector(setZeroSymbol:)])
		[formatter performSelector:@selector(setZeroSymbol:) withObject:@"0"];
	else
		[formatter setAttributedStringForZero:[[[NSAttributedString alloc] initWithString:@"0"] autorelease]];
}

// -180..180 with four decimals.
- (id)ddFormatter
{
	if (!_ddFormatter) {
		_ddFormatter = [NSNumberFormatter new];
		[_ddFormatter setPositiveFormat:@"000.0000"];
		[_ddFormatter setNegativeFormat:@"-000.0000"];
		_setZeroSymbol(_ddFormatter);
		[_ddFormatter setMinimum:[NSDecimalNumber numberWithFloat:-180.0f]];
		[_ddFormatter setMaximum:[NSDecimalNumber numberWithFloat:180.0f]];
	}
	return _ddFormatter;
}

// Whole numbers 0..59.
- (id)dmsFormatter
{
	if (!_dmsFormatter) {
		_dmsFormatter = [NSNumberFormatter new];
		[_dmsFormatter setPositiveFormat:@"#"];
		[_dmsFormatter setAllowsFloats:YES];
		if ([_dmsFormatter respondsToSelector:@selector(setGeneratesDecimalNumbers:)])
			[_dmsFormatter setGeneratesDecimalNumbers:YES];
		else
			[_dmsFormatter setAllowsFloats:NO];
		_setZeroSymbol(_dmsFormatter);
		[_dmsFormatter setMinimum:[NSDecimalNumber numberWithInt:0]];
		[_dmsFormatter setMaximum:[NSDecimalNumber numberWithInt:59]];
	}
	return _dmsFormatter;
}

// A field sized to its description (the placeholder when editable),
// right-aligned, at x; x moves past it less 3 points.
- (id)controlWithDescription:(id)aDescription editable:(BOOL)aEditable frameXOrigin:(float *)aOrigin formatter:(id)aFormatter
{
	NSTextField *field = [super createDescriptionViewWithDescription:aDescription];
	[field sizeToFit];
	NSSize size = [[field cell] cellSizeForBounds:[field bounds]];
	[field setEditable:aEditable];
	[field setAlignment:NSTextAlignmentRight];
	[field setDelegate:(id)self];
	if (aFormatter)
		[field setFormatter:aFormatter];
	if (aEditable)
		[[field cell] setPlaceholderString:aDescription];
	NSRect frame = [field frame];
	[field setFrame:NSMakeRect(*aOrigin, 0.0, frame.size.width, frame.size.height)];
	*aOrigin = (size.width - 3.0f) + *aOrigin;
	return [field autorelease];
}

// A small borderless popup at x; x moves past it.
- (id)popupWithItems:(id)aItems frameXOrigin:(float *)aOrigin
{
	WGPopUpButton *popup = [[WGPopUpButton alloc] initWithFrame:NSMakeRect(*aOrigin, 0.0, 27.0, 14.0) pullsDown:NO];
	[popup setBordered:NO];
	[[popup cell] setControlSize:NSControlSizeSmall];
	// -3 before 10.4.
	[[popup cell] setExtraWidthForCellHeight:-5.0];
	[popup setTarget:self];
	[popup setAction:@selector(popupAction:)];
	[popup addItemsWithTitles:aItems];
	[[popup cell] setItemTitlesStringAttributes:[NSDictionary dictionaryWithObject:
		[NSFont systemFontOfSize:[NSFont smallSystemFontSize]] forKey:NSFontAttributeName]];
	*aOrigin = *aOrigin + [popup frame].size.width;
	return [popup autorelease];
}

- (void)addInputControlsToDescriptionView
{
	NSArray *controls = _sIsEntryDetailsCoordinatesDecimalDegrees ? [self ddInputControls] : [self dmsInputControls];
	[[_descriptionView subviews] makeObjectsPerformSelector:@selector(removeFromSuperviewWithoutNeedingDisplay)];
	int count = (int)[controls count];
	for (int i = 0; i < count; i++)
		[_descriptionView addSubview:[controls objectAtIndex:i]];
	[_descriptionView setNeedsDisplay:YES];
}

// The control being edited commits its value.
- (void)validateDataObject
{
	NSArray *controls = _sIsEntryDetailsCoordinatesDecimalDegrees ? [self ddInputControls] : [self dmsInputControls];
	int count = (int)[controls count];
	for (int i = 0; i < count; i++) {
		id control = [controls objectAtIndex:i];
		if ([control respondsToSelector:@selector(currentEditor)] && [control currentEditor]) {
			[self validateDataObject:control];
			return;
		}
	}
}

- (id)popupTitles
{
	return [NSArray arrayWithObjects:_localized(@"coordinates (decimal degrees)"),
		_localized(@"coordinates (degrees-minutes-seconds)"), nil];
}

- (void)titlePopupAction:(id)aAction
{
	int decimal = [aAction indexOfSelectedItem] == 0;
	if (decimal == _sIsEntryDetailsCoordinatesDecimalDegrees)
		return;
	_sIsEntryDetailsCoordinatesDecimalDegrees = decimal;
	[self rebuildLayout];
	[self validateControls];
}

- (void)popupAction:(id)aAction
{
	[self validateDataObject:aAction];
}

- (void)controlTextDidEndEditing:(id)aEditing
{
	[self validateDataObject:[aEditing object]];
}

// The data object's longitude and latitude.
static void _coordinates(LObject *object, float *longitude, float *latitude)
{
	*longitude = *latitude = 0.0f;
	LArray *array = lobj_cast<LArray>(object);
	if (!array)
		return;
	LValue *lon = lobj_cast<LValue>(array->objectAtIndex(0));
	LValue *lat = lobj_cast<LValue>(array->objectAtIndex(1));
	*latitude = lat ? lat->floatValue() : 0.0f;
	*longitude = lon ? lon->floatValue() : 0.0f;
}

- (id)objectValueForControl:(id)aControl
{
	if ([aControl superview] != _descriptionView)
		return [super objectValueForControl:aControl];
	float longitude, latitude;
	_coordinates(_object, &longitude, &latitude);
	float value;
	NSNumberFormatter *formatter;
	if (_sIsEntryDetailsCoordinatesDecimalDegrees) {
		NSUInteger index = [[self ddInputControls] indexOfObject:aControl];
		if (index == kDDLatitude)
			value = latitude;
		else if (index == kDDLongitude)
			value = longitude;
		else
			return nil;
		formatter = [self ddFormatter];
	} else {
		NSUInteger index = [[self dmsInputControls] indexOfObject:aControl];
		TDegrees lon = degreesFromFloat(longitude);
		TDegrees lat = degreesFromFloat(latitude);
		switch (index) {
		case kDMSLatDegrees: value = lat.degrees; break;
		case kDMSLatMinutes: value = lat.minutes; break;
		case kDMSLatSeconds: value = lat.seconds; break;
		case kDMSLonDegrees: value = lon.degrees; break;
		case kDMSLonMinutes: value = lon.minutes; break;
		case kDMSLonSeconds: value = lon.seconds; break;
		default: return nil;
		}
		formatter = [self dmsFormatter];
	}
	return [formatter stringForObjectValue:[NSNumber numberWithFloat:value]];
}

- (void)validateControls
{
	float longitude, latitude;
	_coordinates(_object, &longitude, &latitude);
	if (_sIsEntryDetailsCoordinatesDecimalDegrees) {
		NSArray *controls = [self ddInputControls];
		[[controls objectAtIndex:kDDLatitude] setFloatValue:latitude];
		[[controls objectAtIndex:kDDLongitude] setFloatValue:longitude];
	} else {
		TDegrees lon = degreesFromFloat(longitude);
		TDegrees lat = degreesFromFloat(latitude);
		NSArray *controls = [self dmsInputControls];
		[[controls objectAtIndex:kDMSLatDegrees] setFloatValue:lat.degrees];
		[[controls objectAtIndex:kDMSLatMinutes] setFloatValue:lat.minutes];
		[[controls objectAtIndex:kDMSLatSeconds] setFloatValue:roundf(lat.seconds)];
		[[controls objectAtIndex:kDMSLonDegrees] setFloatValue:lon.degrees];
		[[controls objectAtIndex:kDMSLonMinutes] setFloatValue:lon.minutes];
		[[controls objectAtIndex:kDMSLonSeconds] setFloatValue:roundf(lon.seconds)];
		[[controls objectAtIndex:kDMSLatSign] selectItemAtIndex:lat.isNegative];
		[[controls objectAtIndex:kDMSLatSign] synchronizeTitleAndSelectedItem];
		[[controls objectAtIndex:kDMSLonSign] selectItemAtIndex:lon.isNegative];
		[[controls objectAtIndex:kDMSLonSign] synchronizeTitleAndSelectedItem];
	}
	[[self titleView] selectItemAtIndex:!_sIsEntryDetailsCoordinatesDecimalDegrees];
}

// A changed control makes a new data object (the longitude within 180,
// the latitude within 90), then the owner checks it.
- (void)validateDataObject:(id)aObject
{
	float longitude, latitude;
	_coordinates(_object, &longitude, &latitude);
	NSUInteger index = [[self ddInputControls] indexOfObject:aObject];
	if (index == NSNotFound) {
		index = [[self dmsInputControls] indexOfObject:aObject];
		int value = [aObject intValue];
		TDegrees lon = degreesFromFloat(longitude);
		TDegrees lat = degreesFromFloat(latitude);
		switch (index) {
		case kDMSLatDegrees:
			if (value == (int)lat.degrees)
				return;
			lat.degrees = value;
			break;
		case kDMSLatMinutes:
			if (value == (int)lat.minutes)
				return;
			lat.minutes = [aObject intValue];
			break;
		case kDMSLatSeconds:
			if (value == lroundf(lat.seconds))
				return;
			lat.seconds = [aObject intValue];
			break;
		case kDMSLonDegrees:
			if (value == (int)lon.degrees)
				return;
			lon.degrees = [aObject intValue];
			break;
		case kDMSLonMinutes:
			if (value == (int)lon.minutes)
				return;
			lon.minutes = [aObject intValue];
			break;
		case kDMSLonSeconds:
			if (value == lroundf(lon.seconds))
				return;
			lon.seconds = [aObject intValue];
			break;
		case kDMSLatSign:
			if (lat.isNegative == ([aObject indexOfSelectedItem] == 1))
				return;
			lat.isNegative = [aObject indexOfSelectedItem] == 1;
			break;
		case kDMSLonSign:
			if (lon.isNegative == ([aObject indexOfSelectedItem] == 1))
				return;
			lon.isNegative = [aObject indexOfSelectedItem] == 1;
			break;
		default:
			return;
		}
		longitude = floatFromDegrees(lon);
		latitude = floatFromDegrees(lat);
	} else {
		float value = [aObject floatValue];
		if (index == kDDLatitude) {
			if (fabsf(value - latitude) < 0.0001f)
				return;
			latitude = value;
		} else if (index == kDDLongitude) {
			if (fabsf(value - longitude) < 0.0001f)
				return;
			longitude = [aObject floatValue];
		} else {
			return;
		}
	}
	if (fabsf(longitude) - 180.0f > 0.0f)
		longitude = 180.0f;
	if (fabsf(latitude) - 90.0f > 0.0f)
		latitude = latitude <= 0.0f ? -90.0f : 90.0f;
	if (_object) {
		_object->release();
		_object = 0;
	}
	_object = LArray::arrayWithObjects(LValue::valueWithFloat(longitude), LValue::valueWithFloat(latitude), (LObject *)0);
	if (_object)
		_object->retain();
	[super validateDataObject];
}

@end
