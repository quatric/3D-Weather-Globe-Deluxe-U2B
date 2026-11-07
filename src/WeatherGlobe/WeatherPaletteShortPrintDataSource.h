#import "WeatherGlobePrefix.h"
#import "BaseWeatherPalettePrintDataSource.h"


@interface WeatherPaletteShortPrintDataSource : BaseWeatherPalettePrintDataSource
{
    WGFilledView *_topView;
    WGFilledView *_bottomView;
    NSTextField *_7dayForecastTitle;
    NSView *_placeholderView;
    BOOL _printViewAlreadySetupped;
    NSSize _neededPrintViewSize;
    float _tmpLastPrintViewOrigin;
    int _pagesCount;
}
- (id)printViewName;
- (id)pageViewAtIndex:(NSInteger)aIndex;
- (void)dataSourceWasChanged;
- (NSInteger)pagesCountForSize:(NSSize)aSize;
- (void)_layoutSFViewsFromControllers:(id)aControllers;
- (id)_forecastViewForPoint:(NSPoint)aPoint;
- (void)awakeFromNib;
- (id)nibName;
@end

@interface WGSFWeatherPrintView : NSView
{
}
- (BOOL)isFlipped;
@end

