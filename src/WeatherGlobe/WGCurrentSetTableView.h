#import "WeatherGlobePrefix.h"
#import "WGCustomTableView.h"


@interface WGCurrentSetTableView : WGCustomTableView
{
    NSAttributedString *_placeholderString;
    NSView *_savedHeaderView;
    BOOL _editingFlag;
    BOOL _isBecomingFirstResponder;
}
- (id)accessibilityAttributeValue:(id)aValue;
- (void)keyDown:(id)aDown;
- (void)redo:(id)aRedo;
- (void)undo:(id)aUndo;
- (void)changeColor:(id)aColor;
- (void)delete:(id)aDelete;
- (BOOL)validateMenuItem:(id)aItem;
- (void)drawRect:(NSRect)aRect;
- (void)drawRow:(NSInteger)aRow clipRect:(NSRect)aRect;
- (void)drawPrintSeparatorInRow:(NSInteger)aRow;
- (void)drawSeparatorInRect:(NSRect)aRect;
- (BOOL)becomeFirstResponder;
- (NSUInteger)draggingSourceOperationMaskForLocal:(BOOL)aLocal;
- (void)reloadData;
- (void)toggleHeaderView;
- (id)wgBackgroundColor;
- (void)setWGPlaceholderString:(id)aString;
- (id)wgPlaceholderString;
- (void)dealloc;
@end

