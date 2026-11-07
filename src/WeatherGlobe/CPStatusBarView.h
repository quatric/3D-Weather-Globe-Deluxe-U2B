#import "WeatherGlobePrefix.h"
#import "CityPropertyAccessibilityObject.h"


@interface CPStatusBarView : CityPropertyGenericView
{
    NSProgressIndicator *_progressIndicator;
    NSButton *_updateButton;
    BOOL _isAnimating;
    NSString *_statusText;
    NSArray *_accessibilityObjects;
    NSRect _lastTextRect;
    int _state;
}
+ (id)_menuItemsDescriptors;
+ (void)initialize;
+ (void)_calculateSizeForBestFit;
+ (id)_updateButton;
+ (id)_progressIndicator;
- (void)_setState:(NSInteger)aState;
- (BOOL)_isWeatherExpired;
- (id)accessibilityObjects;
- (BOOL)enabled;
- (NSSize)sizeForBestFit;
- (void)drawRect:(NSRect)aRect;
- (void)resizeSubviewsWithOldSize:(NSSize)aSize;
- (void)setMenu:(id)aMenu;
- (void)_updateWeather;
- (void)_weatherInternetStateWasChanged:(id)aChanged;
- (void)_reportsWereUpdated:(id)aUpdated;
- (void)_reportsWereExpired:(id)aExpired;
- (void)_stopAnimations;
- (void)_startAnimations;
- (void)dealloc;
- (id)initWithCityEntry:(PointEntry *)aEntry;
@end

