#import "WGEntryDetailsInfoTitlePopupDataProvider.h"
#import "WGPopUpButton.h"

// The title is a small borderless popup of popupTitles, styled like the
// plain title field (kept as the etalon for measuring).
@implementation WGEntryDetailsInfoTitlePopupDataProvider

- (void)dealloc
{
	[_etalonTextField release];
	[super dealloc];
}

- (NSSize)titleSizeForTitleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset
{
	return [self titleSizeForTextField:_etalonTextField titleWidth:aWidth offset:aOffset];
}

// The original checked the system version with Gestalt and, before 10.4,
// left-aligned the etalon and set the popup's title width; only the 10.4+
// path is kept.
- (id)createTitleViewWithTitleString:(id)aString
{
	NSString *title = [[self popupTitles] objectAtIndex:0];
	if (!_etalonTextField)
		_etalonTextField = [super createTitleViewWithTitleString:title];
	else
		[_etalonTextField setStringValue:title];
	[[_etalonTextField cell] setWraps:NO];
	NSSize size = [[_etalonTextField cell] cellSizeForBounds:[_etalonTextField bounds]];
	[[_etalonTextField cell] setWraps:YES];

	WGPopUpButton *popup = [[WGPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, size.width, size.height)];
	[popup setBordered:NO];
	[[popup cell] setControlSize:NSControlSizeSmall];
	[(WGPopUpButtonCell *)[popup cell] setExtraWidthForCellHeight:5.0];
	[popup setTarget:self];
	[popup setAction:@selector(titlePopupAction:)];
	[popup addItemsWithTitles:[self popupTitles]];
	[(WGPopUpButtonCell *)[popup cell]
	    setItemTitlesStringAttributes:[[_etalonTextField attributedStringValue] attributesAtIndex:0 effectiveRange:0]];
	[[_etalonTextField cell] setWraps:NO];
	[popup selectItemAtIndex:0];
	[popup synchronizeTitleAndSelectedItem];
	return popup;
}

- (NSSize)titleSizeForTextField:(id)aField titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset
{
	NSSize size = [[aField cell] cellSizeForBounds:NSMakeRect(aOffset, 0, aWidth, WGEntryDetailsInfoDefaultFieldSize.height)];
	return NSMakeSize(aWidth, size.height);
}

- (id)popupTitles
{
	return [NSArray array];
}

- (void)titlePopupAction:(id)aAction
{
}

@end
