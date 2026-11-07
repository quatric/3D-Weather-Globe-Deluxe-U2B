#import "WeatherGlobePrefix.h"


@interface MainViewPrintDataSource : NSObject
{
    MapDrawView *_mapView;
    NSRect _mapFrame;
    BOOL _mapFrameChanged;
    BOOL _printMapTerrain;
    BOOL _printCountryNames;
}
- (id)printViewName;
- (id)pageViewAtIndex:(NSInteger)aIndex;
- (void)dataSourceWasChanged;
- (BOOL)isPrintOnSinglePage;
- (NSInteger)pagesCountForSize:(NSSize)aSize;
- (void)setPrintCountryNames:(BOOL)aNames;
- (void)setPrintMapTerrain:(BOOL)aTerrain;
- (void)dealloc;
@end

