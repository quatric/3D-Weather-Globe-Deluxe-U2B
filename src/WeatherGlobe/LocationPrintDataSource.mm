#import "LocationPrintDataSource.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "WGEntryDetailsController.h"
#import "WGPrintingEntryDetailsController.h"
#import "WGPrintingEntryDetailsInfoPaneController.h"
#import "WGEntryDetailsUserInfoImagesController.h"
#import "NSView+WGExtensions.h"

// Prints the details of the current location, sliced into pages between
// the rows of its images.
@implementation LocationPrintDataSource

- (void)dealloc
{
	if (_printedEntry) {
		_printedEntry->release();
		_printedEntry = 0;
	}
	[_printView release];
	[_entryDetailsController release];
	[super dealloc];
}

- (id)nibName
{
	return @"LocationInfoPrintLayout";
}

- (void)awakeFromNib
{
	[[[self _entryDetailsController] controllerForClass:[WGPrintingEntryDetailsInfoPaneController class]]
		setPrintWeatherForecast:NO];
}

- (id)_entryDetailsController
{
	if (!_entryDetailsController) {
		_entryDetailsController = [WGPrintingEntryDetailsController new];
		[_entryDetailsController setDelegate:self];
	}
	return _entryDetailsController;
}

// The main window's current entry, kept for the whole print.
- (GenericEntry *)printedEntry
{
	if (!_printedEntry) {
		_printedEntry = (GenericEntry *)[[[[AtlasController atlasController] mainWindowController]
			entryDetailsController] currentEntry];
		if (_printedEntry)
			_printedEntry->retain();
	}
	return _printedEntry;
}

- (BOOL)isPrintOnSinglePage
{
	return NO;
}

- (void)dataSourceWasChanged
{
	_tmpLastPrintViewOrigin = 0.0f;
}

- (id)printViewName
{
	return [[NSBundle mainBundle] localizedStringForKey:@"Location Information" value:@"" table:nil];
}

- (SMKGridMatrix *)_imagesMatrix
{
	WGEntryDetailsUserInfoImagesController *images = [[[self _entryDetailsController]
		controllerForClass:[WGPrintingEntryDetailsInfoPaneController class]] imagesContainer];
	return [(SMKGridControl *)[images descriptionView] gridMatrix];
}

// Installs the details pane in the print view, which grows to all of it.
- (NSInteger)pagesCountForSize:(NSSize)aSize
{
	[self loadNibIfNeeded];
	[_printView wgFixFlip];
	WGPrintingEntryDetailsController *controller = [self _entryDetailsController];
	NSView *pane = [controller pane];
	NSView *container = [controller paneContainer];
	[pane setFrame:[_detailsViewPlaceholder bounds]];
	[_detailsViewPlaceholder addSubview:pane];
	[controller didMounted:YES];
	WGEntryDetailsUserInfoImagesController *images = [[controller
		controllerForClass:[WGPrintingEntryDetailsInfoPaneController class]] imagesContainer];
	[(SMKGridControl *)[images descriptionView] setThumbnailSize:NSMakeSize(216.0, 160.0)];
	[images setScaleImageToCellSize:YES];
	[images adjustMatrixCentering];
	[controller setCurrentEntry:[self printedEntry]];
	NSScrollView *scrollView = [container enclosingScrollView];
	[scrollView setHasVerticalScroller:NO];
	[scrollView setHasHorizontalScroller:NO];
	_neededPrintViewSize = aSize;
	[_printView setFrameSize:aSize];
	[container setAutoresizingMask:NSViewNotSizable];
	[_printView setFrameSize:NSMakeSize([_printView frame].size.width, [container frame].size.height)];
	[images adjustMatrixCentering];
	return (int)([_printView frame].size.height / aSize.height) + 1;
}

// The next page: a window onto the print view whose top edge is moved up to
// the end of the image row it would cut.
- (id)pageViewAtIndex:(NSInteger)aIndex
{
	if (_tmpLastPrintViewOrigin < 0.0f)
		_tmpLastPrintViewOrigin = 0.0f;
	if (_tmpLastPrintViewOrigin == 0.0f) {
		NSRect frame = [_printView frame];
		_tmpLastPrintViewOrigin = frame.origin.y + frame.size.height;
	}
	NSView *container = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0,
		_neededPrintViewSize.width, _neededPrintViewSize.height)];
	[container addSubview:_printView];
	CGFloat height = _neededPrintViewSize.height;
	CGFloat origin = _tmpLastPrintViewOrigin - height;
	SMKGridMatrix *matrix = [self _imagesMatrix];
	_tmpLastPrintViewOrigin = origin;
	NSPoint point = [container convertPoint:NSMakePoint(0.0, origin) toView:matrix];
	NSInteger row = 0, column = 0;
	CGFloat boundsOrigin = origin;
	if ([matrix getRow:&row column:&column forPoint:point]) {
		NSRect cell = [container convertRect:[matrix cellFrameAtRow:row column:column] fromView:matrix];
		boundsOrigin = cell.origin.y + cell.size.height;
		height -= boundsOrigin - origin;
		_tmpLastPrintViewOrigin = boundsOrigin;
	}
	NSRect frame = [container frame];
	NSView *page = [[NSView alloc] initWithFrame:NSMakeRect(0.0, frame.size.height - height,
		frame.size.width, height)];
	[page addSubview:_printView];
	[page setBoundsOrigin:NSMakePoint(0.0, boundsOrigin)];
	[container release];
	return [page autorelease];
}

@end
