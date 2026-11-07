#import "WGPrintingEntryDetailsInfoPaneController.h"

@implementation WGPrintingEntryDetailsInfoPaneController

// The weather forecast's keys (2) when it's printed.
- (NSInteger)keySet
{
	return _printWeatherForecast ? 2 : 0;
}

- (void)setPrintWeatherForecast:(BOOL)aForecast
{
	_printWeatherForecast = aForecast;
	[self update];
}

- (BOOL)printWeatherForecast
{
	return _printWeatherForecast;
}

@end
