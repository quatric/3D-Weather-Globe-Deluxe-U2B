#import "WeatherGlobePrefix.h"
#import "CPWeatherGenericView.h"


@interface CPExpandedForecastView : CPWeatherGenericView
{
    BOOL _tooltipsRectsAreValid;
}
+ (void)_acceptAdjustFontSize;
+ (void)_enabledStateMenu:(id)aMenu;
+ (BOOL)validateMenuItem:(id)aItem;
+ (id)_menuItemsDescriptors;
+ (id)_submenu;
+ (id)_menuItemWithTitle:(id)aTitle selector:(SEL)aSelector tag:(NSInteger)aTag;
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
- (void)_checkToolTipRects;
- (id)_weatherDescriptionFromOneDayReport:(LMap *)aReport;
- (id)_temperatureFromOneDayReport:(LMap *)aReport high:(BOOL)aHigh;
- (id)_summaryIconFromOneDayReport:(LMap *)aReport;
- (void)_validateAllRects;
- (id)_dateAttribs;
- (id)_defaultAttribs;
- (id)_lowTemperatureAttribs;
- (id)_highTemperatureAttribs;
- (void)dealloc;
- (id)initWithCityEntry:(PointEntry *)aEntry;
@end

@interface NSCalendarDate (DescriptionsExtension)
+ (NSInteger)monthWithMaxAbbrDescriptionWidth;
+ (NSInteger)weekdayWithMaxAbbrDescriptionWidth;
@end

