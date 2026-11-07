#import "WeatherGlobePrefix.h"


@interface WBTableView : NSTableView
{
    BOOL _variableRowHeight;
    WBTableRowsLayout *_layout;
    NSString *_name;
}
- (void)accessibilityPerformAction:(id)aAction;
- (id)accessibilityActionDescription:(id)aDescription;
- (id)accessibilityActionNames;
- (BOOL)accessibilityIsAttributeSettable:(id)aSettable;
- (id)accessibilityAttributeValue:(id)aValue;
- (id)accessibilityAttributeNames;
- (void)setName:(id)aName;
- (id)name;
- (void)mouseDown:(id)aDown;
- (void)keyDown:(id)aDown;
- (void)drawGridInClipRect:(NSRect)aRect;
- (void)drawBackgroundInClipRect:(NSRect)aRect;
- (NSRect)frameOfCellAtColumn:(NSInteger)aColumn row:(NSInteger)aRow;
- (NSInteger)rowAtPoint:(NSPoint)aPoint;
- (NSRect)rectOfRow:(NSInteger)aRow;
- (NSRect)rectOfColumn:(NSInteger)aColumn;
- (void)setVariableRowHeight:(BOOL)aHeight;
- (BOOL)isVariableRowHeight;
- (void)scrollRowToVisible:(NSInteger)aVisible;
- (void)reloadData;
- (void)lockFocus;
- (void)viewDidMoveToWindow;
- (void)dealloc;
- (id)initWithCoder:(id)aCoder;
@end

