#import "WeatherGlobePrefix.h"
#import "DragSourceTableView.h"


@interface WGSearchedEntriesTableView : DragSourceTableView
{
}
- (void)accessibilityCellAction:(id)aAction;
- (id)accessibilityAttributeValue:(id)aValue;
- (void)resetCursorRects;
- (void)mouseDown:(id)aDown;
@end

@interface NSObject (EntriesCursorCategory)
- (id)tableView:(id)aView requestedCursorForColumnRowIndices:(id)aIndices;
@end

@interface NSObject (EntriesClickingCategory)
- (void)tableView:(id)aView didClickColumnRowIndices:(id)aIndices;
@end

