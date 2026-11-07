#import "WGEntryDetailsInfoSimpleDataProvider.h"
#import "WGEntryDetailsInfoPaneController.h"
#include "AMControllers.h"

// An info row showing its data object as text, in a wrapping text field
// (editable when the row is).
@implementation WGEntryDetailsInfoSimpleDataProvider

- (Class)descriptionViewClass
{
	return [NSTextField class];
}

- (id)initWithOwner:(id)aOwner key:(LString *)aKey dataObject:(LObject *)aObject
{
	self = [super initWithOwner:aOwner key:aKey];
	if (self) {
		if (aObject)
			aObject->retain();
		_object = aObject;
	}
	return self;
}

- (void)dealloc
{
	if (_object) {
		_object->release();
		_object = 0;
	}
	[_descriptionView release];
	[super dealloc];
}

- (void)didMountedToOwner:(id)aOwner
{
}

- (LObject *)dataObject
{
	return _object;
}

- (void)validateDataObject
{
	[(WGEntryDetailsInfoPaneController *)[self owner] validateObjectFromDataProvider:self];
}

- (id)description
{
	return [NSString stringReferenceFromLString:
		EntryInfoController::stringFromInfoObjectForKey(_object, EntryInfoController::rowKindFromKey([self key]))];
}

- (void)updateDescription
{
	[(NSTextField *)_descriptionView setStringValue:[self description]];
}

- (id)descriptionPlaceholder
{
	return [[NSBundle mainBundle] localizedStringForKey:@"none" value:@"" table:nil];
}

- (void)descriptionDidEndEditing:(id)aEditing
{
	[[self owner] rebuildLayout];
}

- (BOOL)tryStartDescriptionEditing
{
	if (![self isDescriptionEditable])
		return NO;
	return [[_descriptionView window] makeFirstResponder:_descriptionView];
}

- (id)descriptionView
{
	if (!_descriptionView)
		_descriptionView = [self createDescriptionViewWithDescription:[self description]];
	return _descriptionView;
}

// The width left of the title (and the offsets on both sides), the height
// the text needs at that width.
- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset
{
	CGFloat width = [self title] ? aBounds.size.width - aWidth - aOffset * 2.0 : aBounds.size.width - aOffset * 2.0;
	return [[(NSTextField *)_descriptionView cell] cellSizeForBounds:
		NSMakeRect(0.0, 0.0, width, WGEntryDetailsInfoDefaultFieldSize.height)];
}

- (id)objectValueForControl:(id)aControl
{
	if (aControl == [self descriptionView])
		return [self description];
	if (aControl == [self titleView])
		return [self title];
	return nil;
}

- (id)createTitleViewWithTitleString:(id)aString
{
	id view = [super createTitleViewWithTitleString:aString];
	if ([self isDescriptionEditable])
		[view setTextColor:[NSColor blackColor]];
	return view;
}

- (id)createDescriptionViewWithDescription:(id)aDescription
{
	if (!aDescription)
		return nil;
	NSFont *font = [NSFont systemFontOfSize:11.0];
	NSTextField *field = [[[self descriptionViewClass] alloc] initWithFrame:
		NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, WGEntryDetailsInfoDefaultFieldSize.width, WGEntryDetailsInfoDefaultFieldSize.height)];
	[field setBordered:NO];
	[field setFont:font];
	[field setTextColor:[NSColor colorWithDeviceRed:0.0 green:0.0 blue:0.0 alpha:1.0]];
	[[field cell] setPlaceholderString:[self descriptionPlaceholder]];
	BOOL editable = [self isDescriptionEditable];
	if (editable)
		[field setFocusRingType:NSFocusRingTypeNone];
	[field setEditable:editable];
	[field setSelectable:YES];
	[field setAlignment:NSTextAlignmentLeft];
	[[field cell] setWraps:YES];
	[[field cell] setScrollable:NO];
	[field setStringValue:aDescription];
	[field setBackgroundColor:[NSColor whiteColor]];
	[field setDrawsBackground:YES];
	return field;
}

- (void)controlTextDidEndEditing:(id)aEditing
{
}

- (void)controlTextDidBeginEditing:(id)aEditing
{
}

- (void)setDataObject:(LObject *)aObject
{
	if (aObject)
		aObject->retain();
	if (_object) {
		_object->release();
		_object = 0;
	}
	_object = aObject;
	[self updateDescription];
}

@end
