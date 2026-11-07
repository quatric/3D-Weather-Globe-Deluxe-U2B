#import "MainViewPrintDataSource.h"
#import "MapDrawView.h"
#import "AtlasController.h"
#import "MainWindowController.h"

// Prints the current globe or map on one page.
@implementation MainViewPrintDataSource

- (void)dealloc
{
	[_mapView release];
	[super dealloc];
}

- (void)setPrintMapTerrain:(BOOL)aTerrain
{
	_printMapTerrain = aTerrain;
}

- (void)setPrintCountryNames:(BOOL)aNames
{
	_printCountryNames = aNames;
}

// The map view is sized to the page and given a print drawing of the
// current panel.
- (NSInteger)pagesCountForSize:(NSSize)aSize
{
	if (!_mapView)
		_mapView = [[MapDrawView alloc] initWithFrame:NSZeroRect];
	NSRect frame = NSMakeRect(0, 0, aSize.width, aSize.height);
	if (!NSEqualRects([_mapView frame], frame))
		[_mapView setFrame:frame];
	unsigned int flags = BaseDrawing::defaultPrintingFlags();
	if (_printMapTerrain)
		flags |= kPrintFlagEarth;
	else
		flags &= ~kPrintFlagEarth;
	if (_printCountryNames)
		flags |= kPrintFlagCountryNames | kPrintFlagStateNames;
	else
		flags &= ~(kPrintFlagCountryNames | kPrintFlagStateNames);
	BaseDrawing *drawer = 0;
	AppWindowInterfaceWrapper *window = [[[AtlasController atlasController] mainWindowController] windowInterface];
	if (window) {
		NSRect bounds = [_mapView bounds];
		LRect rect = {{(float)bounds.origin.x, (float)bounds.origin.y}, {(float)bounds.size.width, (float)bounds.size.height}};
		drawer = (BaseDrawing *)window->currentPanel()->detachPrintObjectWithFrame(rect, flags);
	}
	[_mapView setDrawer:drawer];
	return 1;
}

- (BOOL)isPrintOnSinglePage
{
	return YES;
}

- (void)dataSourceWasChanged
{
}

- (id)pageViewAtIndex:(NSInteger)aIndex
{
	return _mapView;
}

- (id)printViewName
{
	switch ((int)Preferences::mapMode()) {
	case 0:
		return [[NSBundle mainBundle] localizedStringForKey:@"Globe View" value:@"" table:nil];
	case 1:
		return [[NSBundle mainBundle] localizedStringForKey:@"Map View" value:@"" table:nil];
	default:
		return nil;
	}
}

@end
