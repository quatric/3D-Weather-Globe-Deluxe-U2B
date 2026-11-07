#import "WeatherGlobePrefix.h"
#import "CityPropertyAccessibilityObject.h"


@interface CPWeatherGenericView : CityPropertyGenericView
{
    LMap *_report;
    BOOL _scheduled;
    NSArray *_accessibilityObjects;
}
- (NSUInteger)_reportType;
- (NSUInteger)_reportMask;
- (BOOL)enabled;
- (LMap *)reportDictionary;
- (void)viewDidMoveToWindow;
- (void)_internetStateWasChanged;
- (void)_unitsOfMeasureWereChanged;
- (void)_reportsWereChanged:(id)aChanged;
- (void)dealloc;
- (id)initWithCityEntry:(PointEntry *)aEntry;
- (void)_unschedule;
- (void)_schedule;
@end

