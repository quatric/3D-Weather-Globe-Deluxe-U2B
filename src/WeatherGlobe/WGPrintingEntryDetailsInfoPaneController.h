#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsInfoPaneController.h"


@interface WGPrintingEntryDetailsInfoPaneController : WGEntryDetailsInfoPaneController
{
    BOOL _printWeatherForecast;
}
- (BOOL)printWeatherForecast;
- (void)setPrintWeatherForecast:(BOOL)aForecast;
- (NSInteger)keySet;
@end

