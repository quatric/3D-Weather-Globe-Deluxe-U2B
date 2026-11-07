#import "WeatherGlobePrefix.h"
#import "ImageViewAccessibilityIgnored.h"


@interface BackgroundImageView : ImageViewAccessibilityIgnored
{
}
- (void)mouseUp:(id)aUp;
- (void)mouseDragged:(id)aDragged;
- (void)mouseDown:(id)aDown;
- (BOOL)mouseDownCanMoveWindow;
@end

