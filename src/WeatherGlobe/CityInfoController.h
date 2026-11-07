#import "WeatherGlobePrefix.h"
#import "WGTexturedView.h"


@interface CityInfoController : NSObject
{
    PointEntry *_entry;
    NSSize _sizeForBestFit;
    CityInfoView *_mainView;
    NSView *_contentView;
    NSArray *_cityPropertyViews;
    NSArray *_lineViewsContainers;
}
+ (id)_propertyViewClassesForMenu;
+ (id)_allPropertyViewClasses;
+ (id)optionsMenuItems;
- (void)_allignViews;
- (void)_createAllViews;
- (id)_allPropertyViews;
- (void)manualUpdateWeather;
- (void)reconstructMainView;
- (void)setMenu:(id)aMenu;
- (PointEntry *)cityEntry;
- (NSSize)sizeForBestFit;
- (id)mainView;
- (void)dealloc;
- (id)initWithCityEntry:(PointEntry *)aEntry;
@end

@interface _WGCITexturedView : WGTexturedView
{
}
- (void)drawRect:(NSRect)aRect;
@end

