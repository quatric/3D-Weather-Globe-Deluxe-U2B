#import "WGEntryDetailsInfoDataProvider.h"

// One row of the entry details info pane: a bold, right-aligned title for
// the key; subclasses add the description view.
@implementation WGEntryDetailsInfoDataProvider

- (id)initWithOwner:(id)aOwner key:(LString *)aKey
{
	self = [super init];
	if (self) {
		_owner = aOwner;
		if (aKey)
			aKey->retain();
		_key = aKey;
		_titleString = [[NSString stringWithLString:EntryInfoController::parameterTitleFromKey(aKey)] retain];
	}
	return self;
}

- (void)dealloc
{
	if (_key) {
		_key->release();
		_key = 0;
	}
	[_titleString release];
	[_titleView release];
	[super dealloc];
}

- (id)owner
{
	return _owner;
}

- (LString *)key
{
	return _key;
}

- (id)title
{
	return _titleString;
}

- (id)titleView
{
	if (!_titleView)
		_titleView = [self createTitleViewWithTitleString:[self title]];
	return _titleView;
}

- (id)descriptionView
{
	return nil;
}

- (BOOL)isDescriptionEditable
{
	return NO;
}

- (BOOL)tryStartDescriptionEditing
{
	return NO;
}

- (void)viewSizesForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset outTitleSize:(NSSize *)aSize outDescriptionSize:(NSSize *)aSize2
{
	if (aSize)
		*aSize = [self titleSizeForTitleWidth:aWidth offset:aOffset];
	if (aSize2)
		*aSize2 = [self descriptionSizeForSuperviewBounds:aBounds titleWidth:aWidth offset:aOffset];
}

- (void)didMountedToOwner:(id)aOwner
{
}

- (void)prepareCommit
{
}

// The title's height wrapped to the width.
- (NSSize)titleSizeForTitleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset
{
	NSSize size = [[[self titleView] cell] cellSizeForBounds:NSMakeRect(aOffset, 0, aWidth, 10000.0)];
	return NSMakeSize(aWidth, size.height);
}

- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset
{
	if (![self descriptionView])
		return NSZeroSize;
	return [[self descriptionView] frame].size;
}

- (id)createTitleViewWithTitleString:(id)aString
{
	if (!aString)
		return nil;
	NSTextField *field = [[NSTextField alloc] initWithFrame:NSMakeRect(NSZeroPoint.x, NSZeroPoint.y, 100.0, 10000.0)];
	[field setBordered:NO];
	[field setFont:[NSFont boldSystemFontOfSize:11.0]];
	[field setTextColor:[NSColor colorWithDeviceRed:0.0 green:0.0 blue:0.0 alpha:0.4]];
	[field setEditable:NO];
	[field setSelectable:NO];
	[field setAlignment:NSTextAlignmentRight];
	[[field cell] setWraps:YES];
	[[field cell] setScrollable:NO];
	[field setStringValue:aString];
	return field;
}

- (id)createDescriptionViewWithDescription:(id)aDescription
{
	return nil;
}

@end
