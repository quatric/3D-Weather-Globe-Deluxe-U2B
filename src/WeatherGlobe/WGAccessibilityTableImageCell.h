#import "WeatherGlobePrefix.h"


@interface WGAccessibilityTableImageCell : NSImageCell
{
}
- (void)accessibilityPerformAction:(id)aAction;
- (id)accessibilityActionDescription:(id)aDescription;
- (id)accessibilityActionNames;
@end

@interface WGAccessibilityTableTextCell : NSTextFieldCell
{
}
- (void)accessibilityPerformAction:(id)aAction;
- (id)accessibilityActionDescription:(id)aDescription;
- (id)accessibilityActionNames;
@end

@interface NSCell (WGAccessibilityTableCellExtensions)
- (void)wgTableCellAccessibilityPerformAction:(id)aAction;
- (id)wgTableCellAccessibilityActionDescription:(id)aDescription;
- (id)wgTableCellAccessibilityActionNames;
@end

