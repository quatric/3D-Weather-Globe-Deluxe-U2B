#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface BaseWeatherPalettePrintDataSource : WGNibOwnerController
{
    TLArray<PointEntry*> *_entries;
    NSView *_printView;
}
- (BOOL)isPrintOnSinglePage;
- (id)initWithEntriesList:(TLArray<PointEntry*> *)aList;
- (void)dealloc;
@end

