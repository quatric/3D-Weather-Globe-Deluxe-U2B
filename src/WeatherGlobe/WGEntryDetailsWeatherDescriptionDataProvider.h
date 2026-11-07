#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsInfoSimpleDataProvider.h"


@interface WGEntryDetailsWeatherDescriptionDataProvider : WGEntryDetailsInfoSimpleDataProvider
{
}
- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset;
- (id)descriptionView;
- (void)updateDescription;
- (void)_layoutSFViewsFromControllers:(id)aControllers;
@end

