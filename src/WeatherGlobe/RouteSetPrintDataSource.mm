#import "RouteSetPrintDataSource.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "WGCurrentSetController.h"
#import "WGFilledView.h"
#import "NSView+WGExtensions.h"

static NSString *const _kLocationColumnDistance = @"interval";

// Prints the current route's table, sliced into pages between rows.
@implementation RouteSetPrintDataSource

- (id)nibName
{
	return @"RouteSetPrintLayout";
}

- (void)awakeFromNib
{
	NSColor *color = [NSColor colorWithCalibratedWhite:0.8196f alpha:1.0];
	[_headerViewBackground setFillColor:color];
	[_bottomViewBackground setFillColor:color];
}

- (void)dealloc
{
	[_printView release];
	[super dealloc];
}

// Wrapping 13 pt cells; the distance column is right aligned.
- (void)_setPrintTableViewCustomCell
{
	NSFont *font = [NSFont systemFontOfSize:13.0];
	for (NSTableColumn *column in [_printedTableView tableColumns]) {
		WGPrintTableViewCell *cell = [WGPrintTableViewCell new];
		[cell setFont:font];
		[cell setWraps:YES];
		[cell setLineBreakMode:NSLineBreakByTruncatingMiddle];
		if ([[column identifier] isEqualToString:_kLocationColumnDistance]) {
			[cell setAlignment:NSTextAlignmentRight];
			[cell setPlaceholderString:@"---"];
		}
		[column setDataCell:[cell autorelease]];
	}
}

- (BOOL)isPrintOnSinglePage
{
	return NO;
}

- (void)dataSourceWasChanged
{
	_tmpLastPrintViewOrigin = 0.0f;
	_currentPage = 0;
}

- (id)printViewName
{
	return [[NSBundle mainBundle] localizedStringForKey:@"Route Set" value:@"" table:nil];
}

// The next page: a window onto the print view whose top edge is moved up to
// the end of the last row that fits.
- (id)pageViewAtIndex:(NSInteger)aIndex
{
	if (_pagesCount < ++_currentPage) {
		_tmpLastPrintViewOrigin = 0.0f;
		_currentPage = 0;
	}
	if (_tmpLastPrintViewOrigin == 0.0f) {
		NSRect frame = [_printView frame];
		_tmpLastPrintViewOrigin = frame.origin.y + frame.size.height;
	}
	NSView *container = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0,
		_neededPrintViewSize.width, _neededPrintViewSize.height)];
	[container addSubview:_printView];
	CGFloat height = _neededPrintViewSize.height;
	CGFloat origin = _tmpLastPrintViewOrigin - height;
	_tmpLastPrintViewOrigin = origin;
	NSPoint point = [container convertPoint:NSMakePoint(0.0, origin) toView:_printedTableView];
	NSInteger row = [_printedTableView rowAtPoint:point];
	if (row != -1) {
		NSRect cell = [container convertRect:[_printedTableView frameOfCellAtColumn:0 row:row]
			fromView:_printedTableView];
		CGFloat cellEnd = cell.origin.y + cell.size.height;
		height = height - (cellEnd - origin) + 1.0;
		origin = cellEnd + 1.0;
		_tmpLastPrintViewOrigin = cellEnd;
	}
	if (_pagesCount == _currentPage) {
		height += 1.0;
		origin += 1.0;
	}
	NSRect frame = [container frame];
	NSView *page = [[NSView alloc] initWithFrame:NSMakeRect(0.0, frame.size.height - height,
		frame.size.width, height)];
	[page addSubview:_printView];
	[page setBoundsOrigin:NSMakePoint(0.0, origin)];
	[container release];
	return [page autorelease];
}

// Lays the print view out once: the table grows to all its rows and the
// bottom view moves below it.
- (NSInteger)pagesCountForSize:(NSSize)aSize
{
	[self loadNibIfNeeded];
	_currentPage = 0;
	_tmpLastPrintViewOrigin = 0.0f;
	if (_printViewAlreadySetupped)
		return _pagesCount;

	NSBundle *bundle = [NSBundle mainBundle];
	NSString *unit = [bundle localizedStringForKey:Preferences::usesMetricSystem() ? @"Kilometer" : @"Mile"
		value:@"" table:nil];
	NSString *format = [bundle localizedStringForKey:@"Interval" value:@"" table:nil];
	[_intervalBetweenTextField setStringValue:[NSString stringWithFormat:format, unit]];
	[_printView wgFixFlip];
	[_tableViewPlaceholder wgFixFlip];
	_neededPrintViewSize = aSize;
	[self _setPrintTableViewCustomCell];
	[_printView setFrameSize:aSize];
	[_printedTableView setFrameSize:NSMakeSize([_printedTableView frame].size.width, 0.0)];
	[_headerView setAutoresizingMask:NSViewNotSizable];
	[_bottomView setAutoresizingMask:NSViewNotSizable];
	[_tableViewPlaceholder setAutoresizingMask:NSViewNotSizable];

	WGCurrentSetController *controller = [[[AtlasController atlasController] mainWindowController]
		currentSetController];
	[_printedTableView setDataSource:(id)controller];
	[_printedTableView setDelegate:(id)controller];
	[_printedTableView reloadData];
	CGFloat tableHeight = [_printedTableView numberOfRows]
		* ([_printedTableView intercellSpacing].height + [_printedTableView rowHeight]);
	NSScrollView *scrollView = [_printedTableView enclosingScrollView];
	[_tableViewPlaceholder setFrameSize:NSMakeSize([_tableViewPlaceholder frame].size.width, tableHeight)];
	[scrollView setFrameSize:NSMakeSize([scrollView frame].size.width, tableHeight)];
	[_printedTableView setFrameSize:NSMakeSize([_printedTableView frame].size.width, tableHeight)];

	CGFloat bottomY = [_bottomView frame].origin.y;
	NSRect placeholder = [_tableViewPlaceholder frame];
	[_bottomView setFrameOrigin:NSMakePoint([_bottomView frame].origin.x,
		placeholder.origin.y + placeholder.size.height)];
	CGFloat moved = bottomY - [_bottomView frame].origin.y;
	NSRect printFrame = [_printView frame];
	[_printView setFrameSize:NSMakeSize(printFrame.size.width, printFrame.size.height - moved)];

	LString *distance = Utils::distanceDescription(LF::LValue::valueWithFloat([controller currentRoute]->totalDistance()));
	[_totalDistanceTextField setStringValue:(NSString *)distance->cfString()];
	_printViewAlreadySetupped = YES;
	_pagesCount = (int)([_printView frame].size.height / aSize.height) + 1;
	return _pagesCount;
}

@end
