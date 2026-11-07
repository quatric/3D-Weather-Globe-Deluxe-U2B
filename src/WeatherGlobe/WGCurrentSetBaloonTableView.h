#import "WeatherGlobePrefix.h"
#import "WGCurrentSetTableView.h"


@interface WGCurrentSetBaloonTableView : WGCurrentSetTableView
{
}
- (id)_highlightColorForCell:(id)aCell;
- (void)drawRow:(NSInteger)aRow clipRect:(NSRect)aRect;
@end

