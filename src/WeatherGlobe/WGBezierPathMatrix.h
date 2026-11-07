#import "WeatherGlobePrefix.h"


@interface WGBezierPathMatrix : NSMatrix
{
}
- (struct _WGMatrixPosition)cellPositionByPoint:(NSPoint)aPoint;
- (void)mouseDown:(id)aDown;
- (void)drawRect:(NSRect)aRect;
@end

