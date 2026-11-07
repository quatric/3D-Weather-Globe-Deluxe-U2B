#import "WeatherGlobePrefix.h"


@interface WGTextFieldCell : NSTextFieldCell
{
}
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)selectWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate start:(NSInteger)aStart length:(NSInteger)aLength;
- (id)initWithCell:(id)aCell;
@end

