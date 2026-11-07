#import "WeatherGlobePrefix.h"


@interface WGCustomToolbarMatrix : NSMatrix
{
    id _clickedCell;
}
- (id)clickedCell;
- (void)highlightCell:(BOOL)aCell atRow:(NSInteger)aRow column:(NSInteger)aColumn;
- (void)mouseDown:(id)aDown;
@end

