#import "WeatherGlobePrefix.h"


@interface WGCenteredTextFieldCell : NSTextFieldCell
{
}
- (void)editWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate event:(id)aEvent;
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView;
- (NSRect)titleRectForBounds:(NSRect)aBounds;
- (NSRect)drawingRectForBounds:(NSRect)aBounds;
@end

