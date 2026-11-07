#import "WeatherGlobePrefix.h"


@interface WGEarthViewPlaceholder : NSView
{
    NSView *_earthView;
    NSBox *_separator;
    NSView *_detailsView;
}
- (void)resizeSubviewsWithOldSize:(NSSize)aSize;
@end

