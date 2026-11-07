#import "WeatherGlobePrefix.h"


@interface ImageViewAccessibilityIgnored : NSImageView
{
}
- (id)accessibilityFocusedUIElement;
- (BOOL)accessibilityIsIgnored;
- (id)accessibilityHitTest:(NSPoint)aTest;
- (BOOL)accessibilityIsAttributeSettable:(id)aSettable;
- (id)accessibilityAttributeValue:(id)aValue;
- (id)accessibilityAttributeNames;
@end

