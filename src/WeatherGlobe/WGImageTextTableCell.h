#import "WeatherGlobePrefix.h"
#import "WGAccessibilityTableImageCell.h"


@interface WGImageTextTableCell : WGAccessibilityTableTextCell
{
}
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)setObjectValue:(id)aValue;
- (id)init;
- (id)initImageCell:(id)aCell;
- (id)initTextCell:(id)aCell;
@end

