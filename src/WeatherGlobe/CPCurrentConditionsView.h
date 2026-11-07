#import "WeatherGlobePrefix.h"
#import "CPWeatherGenericView.h"


@interface CPCurrentConditionsView : CPWeatherGenericView
{
    BOOL _tooltipRectIsValid;
}
+ (void)_acceptAdjustFontSize;
+ (void)_enabledStateMenu:(id)aMenu;
+ (BOOL)validateMenuItem:(id)aItem;
+ (id)_menuItemsDescriptors;
+ (void)_applicationWillTerminate;
+ (void)initialize;
- (id)accessibilityObjects;
- (NSUInteger)_reportType;
- (NSUInteger)_reportMask;
- (BOOL)enabled;
- (NSSize)sizeForBestFit;
- (id)view:(id)aView stringForToolTip:(NSInteger)aTip point:(NSPoint)aPoint userData:(void *)aData;
- (void)drawRect:(NSRect)aRect;
- (void)_viewFrameWasChanged;
- (void)_checkToolTipRect;
- (id)_weatherDescriptionFromReport:(LMap *)aReport;
- (id)_temperatureFromReport:(LMap *)aReport;
- (id)_summaryIconFromReport:(LMap *)aReport;
- (void)_validateAllRects;
- (id)_metricAttribs;
- (id)_temperatureAttribs;
- (void)dealloc;
- (id)initWithCityEntry:(PointEntry *)aEntry;
@end

