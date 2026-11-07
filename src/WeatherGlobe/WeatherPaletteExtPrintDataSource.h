#import "WeatherGlobePrefix.h"
#import "BaseWeatherPalettePrintDataSource.h"


@interface WeatherPaletteExtPrintDataSource : BaseWeatherPalettePrintDataSource
{
    NSImageView *_locationIcon;
    NSTextField *_locationName;
    NSTextField *_locationKind;
    NSTextField *_region;
    NSTextField *_dateAndTime;
    NSTextField *_timeOffset;
    NSTextField *_coordinates;
    NSTextField *_temperature;
    NSTextField *_feelsLike;
    NSTextField *_windSpeed;
    NSTextField *_humidity;
    NSTextField *_visibility;
    NSTextField *_devPoint;
    NSTextField *_pressure;
    NSImageView *_conditinIcon;
    NSImageView *_windDirectionIcon;
    NSView *_efPlaceholder;
    NSWindow *_downloadWeatherDialog;
    NSProgressIndicator *_downloadProgress;
    int _weatherDownloadDone;
    BOOL _isPrintForecast;
    BOOL _printViewAlreadySetupped;
    SEL _endWeatherPrepareCallback;
    id _endWeatherPrepareTarget;
    int _currentPageNum;
}
- (id)printViewName;
- (id)pageViewAtIndex:(NSInteger)aIndex;
- (void)dataSourceWasChanged;
- (NSInteger)pagesCountForSize:(NSSize)aSize;
- (void)_layoutEFViewsFromControllers:(id)aControllers;
- (void)_setCurrentConditions:(LMap *)aConditions;
- (void)_reportsWereChanged:(id)aChanged;
- (void)printWithoutWeather:(id)aWeather;
- (void)_downloadWeatherDialogDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo;
- (void)prepareWeatherDidEndSelector:(SEL)aSelector target:(id)aTarget;
- (BOOL)_isWeatherExpiredForEntry:(PointEntry *)aEntry;
- (BOOL)_isExtendedForecastExpiredForEntry:(PointEntry *)aEntry;
- (BOOL)_isCurrentConditionExpiredForEntry:(PointEntry *)aEntry;
- (void)awakeFromNib;
- (id)nibName;
- (void)dealloc;
@end

@interface WGExtWeatherForecastPrintView : NSView
{
}
- (BOOL)isFlipped;
@end

