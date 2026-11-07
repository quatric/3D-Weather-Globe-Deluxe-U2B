#import "WeatherGlobePrefix.h"
#import "CPWeatherGenericView.h"


@interface CPWeatherDescriptionView : CPWeatherGenericView
{
}
+ (void)_acceptAdjustFontSize;
+ (void)_applicationWillTerminate;
+ (void)initialize;
- (id)accessibilityObjects;
- (NSUInteger)_reportType;
- (NSUInteger)_reportMask;
- (BOOL)enabled;
- (NSSize)sizeForBestFit;
- (void)drawRect:(NSRect)aRect;
- (id)_weatherDescription;
- (id)_attributes;
- (id)initWithCityEntry:(PointEntry *)aEntry;
@end

