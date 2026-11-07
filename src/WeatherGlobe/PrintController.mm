#import "PrintController.h"
#import "PrintingView.h"
#import "CompanyLinkButton.h"

// What the print data sources answer.
@interface NSObject (PrintControllerDataSource)
- (BOOL)isPrintOnSinglePage;
- (NSInteger)pagesCountForSize:(NSSize)aSize;
- (void)dataSourceWasChanged;
- (id)printViewName;
- (id)pageViewAtIndex:(NSInteger)aIndex;
@end

// Prints the provider's data sources one after another on a page layout
// (title, name, page counter, date, logos), each source's views placed in
// the place view. A source printed on a single page is scaled up by the
// print scaling factor and split into n×n sheets. The controller keeps
// itself alive until the print operation ends.
@implementation PrintController

+ (void)printWithDataSourceProvider:(id)aProvider
{
	PrintController *controller = [PrintController new];
	[controller _printWithDataSourceProvider:aProvider];
	[controller release];
}

// A custom dialog runs first: as a sheet on the provider's window (the
// provider prepares, then prints), else modally.
- (void)_printWithDataSourceProvider:(id)aProvider
{
	[self retain];
	_pagesSet = [NSMutableSet new];
	_currentPage = 0;
	_pagesNum = 0;
	_nibWasLoaded = NO;
	_dataSourceProvider = [aProvider retain];
	[self loadNibIfNeeded];
	NSWindow *host = [_dataSourceProvider hostWindowForPrintDialog];
	NSWindow *dialog = [_dataSourceProvider customPrintDialog];
	if (!dialog) {
		[self _startPrintOperationModalForWindow:host];
		return;
	}
	if (host) {
		[host beginSheet:dialog completionHandler:^(NSModalResponse returnCode) {
			[self sheetDidEnd:dialog returnCode:returnCode contextInfo:0];
		}];
	} else if ([NSApp runModalForWindow:dialog]) {
		[self _startPrintOperationModalForWindow:nil];
	}
}

- (void)sheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo
{
	if (aCode)
		[_dataSourceProvider preparePrintingDidEndSelector:@selector(_startPrint) target:self];
}

- (void)_startPrint
{
	[self _startPrintOperationModalForWindow:[_dataSourceProvider hostWindowForPrintDialog]];
}

- (void)dealloc
{
	[self _cleanUp];
	[_helperView release];
	[_pageView release];
	[super dealloc];
}

// The page's views go into a helper view that resizes with it.
- (void)awakeFromNib
{
	[_pageView setPrintController:self];
	[_copyrightField setStringValue:[[NSBundle mainBundle] copyrightString]];
	_helperView = [[NSView alloc] initWithFrame:[_pageView frame]];
	[_helperView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	NSEnumerator *enumerator = [[[[_pageView subviews] copy] autorelease] objectEnumerator];
	NSView *view;
	while ((view = [enumerator nextObject]))
		[_helperView addSubview:view];
	[_pageView addSubview:_helperView];
	_originalPlaceViewFrame = [_placeView frame];
}

- (id)nibName
{
	return @"PrintLayout.nib";
}

// The shared print info with margins of at least 30 (the imageable area's
// when larger).
- (id)_printInfo
{
	NSPrintInfo *info = [NSPrintInfo sharedPrintInfo];
	NSRect imageable = [info imageablePageBounds];
	NSSize paper = [info paperSize];
	[info setLeftMargin:imageable.origin.x <= 30.0 ? 30.0 : imageable.origin.x];
	[info setBottomMargin:imageable.origin.y <= 30.0 ? 30.0 : imageable.origin.y];
	CGFloat right = paper.width - (imageable.origin.x + imageable.size.width);
	[info setRightMargin:right > 30.0 ? right : 30.0];
	CGFloat top = paper.height - (imageable.origin.y + imageable.size.height);
	[info setTopMargin:top > 30.0 ? top : 30.0];
	return info;
}

- (NSSize)_pageSize
{
	NSPrintInfo *info = [self _printInfo];
	NSSize paper = [info paperSize];
	return NSMakeSize(floorf(paper.width - ([info leftMargin] + [info rightMargin])),
		floorf(paper.height - ([info topMargin] + [info bottomMargin])));
}

// The sheets a single page is split into: n×n for a scale up to n.
- (NSInteger)_numPagesForScale:(CGFloat)aScale
{
	int n = (int)(aScale - 0.001) + 1;
	return n * n;
}

- (id)_dataSourceForPage:(NSInteger)aPage
{
	int count = (int)[_dataSources count];
	for (int i = 0; i < count; i++)
		if ((NSUInteger)(aPage - 1) - _dataSourcesPageRanges[i].location < _dataSourcesPageRanges[i].length)
			return [_dataSources objectAtIndex:i];
	return nil;
}

- (CGFloat)_scalingFactor
{
	return [[[[self _printInfo] dictionary] objectForKey:NSPrintScalingFactor] floatValue];
}

// Every page counts toward the page counter; a single-page source takes
// as many sheets as its scale needs.
- (NSInteger)_pagesCountForDataSource:(id)aSource
{
	BOOL single = [aSource isPrintOnSinglePage];
	CGFloat scale = [self _scalingFactor];
	NSSize size = [_placeView bounds].size;
	if (single) {
		size.width *= scale;
		size.height *= scale;
	}
	NSInteger pages = [aSource pagesCountForSize:size];
	_pagesNum += (int)pages;
	return single ? [self _numPagesForScale:scale] : pages;
}

- (void)_cleanUp
{
	if (_dataSourcesPageRanges) {
		free(_dataSourcesPageRanges);
		_dataSourcesPageRanges = 0;
	}
	[_currPrintView removeFromSuperview];
	_currPrintView = nil;
	[(id)_dataSourceProvider release];
	_dataSourceProvider = nil;
	[_pagesSet release];
	_pagesSet = nil;
	[_dataSources release];
	_dataSources = nil;
	_currentDataSource = nil;
}

- (void)_startPrintOperationModalForWindow:(id)aWindow
{
	_dataSources = [[_dataSourceProvider printDataSources] retain];
	[_printJobTitleField setStringValue:[_dataSourceProvider printJobTitle]];
	[_printDateField setStringValue:[[NSCalendarDate calendarDate] descriptionWithCalendarFormat:@"%A, %B %e, %Y"]];
	NSPrintOperation *operation = [NSPrintOperation printOperationWithView:_pageView];
	if (aWindow) {
		[operation runOperationModalForWindow:aWindow delegate:self
			didRunSelector:@selector(printOperationDidRun:success:contextInfo:) contextInfo:0];
		return;
	}
	[operation runOperation];
	[self release];
}

- (void)printOperationDidRun:(id)aRun success:(BOOL)aSuccess contextInfo:(void *)aInfo
{
	[self release];
}

// Scaled pages show the one-piece logo at the page width.
- (void)_prepeareLogoToScale:(BOOL)aScale
{
	[_logoLeft setHidden:aScale];
	[_logoCenter setHidden:aScale];
	[_logoRight setHidden:aScale];
	[_logoFull setHidden:!aScale];
	[_logoFull setFrameSize:NSMakeSize([_pageView bounds].size.width, [_logoFull frame].size.height)];
}

// Lays out the page (1-based) and returns the part of the page view to
// print. The data sources lay out their pages in order, so a page asked
// out of order lays out the ones before it first.
- (NSRect)preparePage:(NSInteger)aPage
{
	int page = (int)aPage;
	if (page <= _currentPage)
		_currentPage = 0;
	NSNumber *number = [NSNumber numberWithInt:page];
	if ([_pagesSet containsObject:number]) {
		_currentPage = 0;
		[_pagesSet removeAllObjects];
	} else {
		[_pagesSet addObject:number];
	}
	if (page != 1 && _currentPage == 0)
		for (int i = 1; i != page; i++)
			[self preparePage:i];

	id source = [self _dataSourceForPage:page];
	BOOL single = [source isPrintOnSinglePage];
	CGFloat scale = [self _scalingFactor];
	if (source != _currentDataSource || page == _pervPageNum) {
		[(id)_currentDataSource dataSourceWasChanged];
		_currentDataSource = source;
		if (page == _pervPageNum)
			_currentPage = 0;
		NSSize size = [self _pageSize];
		if (!single) {
			[self _prepeareLogoToScale:[_pageView bounds].size.width < 596.0];
		} else {
			if ([self _numPagesForScale:scale] > 1)
				[self _prepeareLogoToScale:NO];
			if (scale > 1.0) {
				float factor = ceilf(scale);
				size.width *= factor;
				size.height *= factor;
			}
		}
		[_pageView setFrameSize:size];
		[_helperView setFrameSize:[_pageView bounds].size];
		[_placeView setFrameOrigin:_originalPlaceViewFrame.origin];
	}
	_pervPageNum = page;
	if (!single && _currentPage)
		_currentPage++;
	else if (!_currentPage)
		_currentPage = page;

	NSRect rect = [_pageView bounds];
	[_pageCounterField setStringValue:[NSString stringWithFormat:
		[[NSBundle mainBundle] localizedStringForKey:@"CurrPage" value:@"" table:nil], _currentPage, _pagesNum]];
	[_printNameField setStringValue:[source printViewName]];
	NSView *view = [source pageViewAtIndex:page];
	if (view == _currPrintView) {
		if (!single)
			return rect;
	} else {
		[_currPrintView removeFromSuperview];
		_currPrintView = view;
		[_placeView addSubview:view];
		if (!single)
			return rect;
		// Centered between the separators.
		NSRect bounds = [_currPrintView bounds];
		NSRect top = [_topSeparator frame];
		NSRect bottom = [_bottomSeparator frame];
		[_placeView setFrameOrigin:NSMakePoint((rect.size.width - bounds.size.width) * 0.5,
			(top.origin.y + bottom.size.height + bottom.origin.y) * 0.5 + bounds.size.height * -0.5)];
	}
	int n = (int)sqrt((double)[self _numPagesForScale:scale]);
	rect.size.width /= n;
	rect.size.height /= n;
	rect.origin.x += ((page - 1) % n) * rect.size.width;
	rect.origin.y += (n - (page - 1) / n - 1) * rect.size.height;
	return rect;
}

- (id)printJobTitle
{
	return [_dataSourceProvider printJobTitle];
}

// Small pages are laid out at a fixed size (530 or 768 points on the short
// side) and scaled down; tiny ones keep the place view centered vertically.
- (NSInteger)pagesCount
{
	_pagesNum = 0;
	int count = (int)[_dataSources count];
	if (_dataSourcesPageRanges)
		free(_dataSourcesPageRanges);
	_dataSourcesPageRanges = (NSRange *)malloc(count * sizeof(NSRange));
	NSSize size = [self _pageSize];
	BOOL portrait = size.width <= size.height;
	float base = portrait ? 768.0f : 530.0f;
	float minWidth = portrait ? 530.0f : 768.0f;
	BOOL scaled = NO;
	if (size.width < minWidth || size.height < base) {
		scaled = YES;
		if (size.width < (portrait ? 90.0f : 130.0f) || size.height < (portrait ? 130.0f : 90.0f))
			[_placeView setAutoresizingMask:[_placeView autoresizingMask] | NSViewMinYMargin | NSViewMaxYMargin];
	}
	[_pageView setFrameSize:size];
	if (scaled) {
		NSSize bounds = size.height < size.width ? NSMakeSize(size.width / size.height * base, base)
			: NSMakeSize(base, 1.0f / (size.width / size.height) * base);
		[_pageView setBoundsSize:bounds];
		[_helperView setFrameSize:bounds];
	}
	[self _prepeareLogoToScale:[_pageView bounds].size.width < 596.0];
	NSUInteger location = 0;
	for (int i = 0; i < count; i++) {
		NSInteger pages = [self _pagesCountForDataSource:[_dataSources objectAtIndex:i]];
		_dataSourcesPageRanges[i] = NSMakeRange(location, pages);
		location += pages;
	}
	return location;
}

@end
