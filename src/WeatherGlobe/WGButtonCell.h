#import "WeatherGlobePrefix.h"


@interface WGButtonCell : NSButtonCell
{
}
- (id)accessibilityAttributeValue:(id)aValue;
// Gives every NSButtonCell this accessibility title (was poseAsClass:).
+ (void)installAsButtonCellAccessibility;
@end

