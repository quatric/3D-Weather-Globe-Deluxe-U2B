#import "WeatherGlobePrefix.h"


@interface HSScrollView : NSScrollView
{
}
- (NSSize)frameSizeForContentSize:(NSSize)aSize;
- (void)reflectScrolledClipView:(id)aView;
- (BOOL)mouseDownCanMoveWindow;
@end

