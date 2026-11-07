#import "WeatherGlobePrefix.h"
#import "CityPropertyAccessibilityObject.h"


@interface CPMainPropertyView : CityPropertyGenericView
{
    NSString *_title;
    NSString *_formatedTime;
    NSSize _sizeForBestFit;
    BOOL _sizeIsValid;
    NSArray *_accessibilityObjects;
    NSRect _lastTitleRect;
    NSRect _lastDateRect;
}
+ (void)_detailViewMenu:(id)aMenu;
+ (BOOL)validateMenuItem:(id)aItem;
+ (id)_menuItemsDescriptors;
+ (id)_submenu;
+ (id)_menuItemWithTitle:(id)aTitle selector:(SEL)aSelector tag:(NSInteger)aTag;
- (id)accessibilityObjects;
- (BOOL)enabled;
- (NSSize)sizeForBestFit;
- (void)drawRect:(NSRect)aRect;
- (void)_acceptAdjustTime;
- (void)_acceptDetailsChanges;
- (void)_acceptAdjustFontSize;
- (void)_clocksMeasureWereChanged;
- (void)_calculateSizeIfNeeded;
- (void)_validateAllData;
- (id)_calendarDate;
- (id)_calendarFormat;
- (void)dealloc;
- (id)initWithCityEntry:(PointEntry *)aEntry;
@end

