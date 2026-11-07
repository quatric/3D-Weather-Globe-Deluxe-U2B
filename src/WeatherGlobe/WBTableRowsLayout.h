#import "WeatherGlobePrefix.h"


@interface WBTableRowsLayout : NSObject
{
    NSTableView *_tableView;
    NSMutableData *_layoutInfo;
    BOOL _layoutInProgress;
    BOOL _invalidateLayout;
    float _tableHeight;
}
- (void)dealloc;
- (id)initWithTableView:(id)aView;
- (void)drawGridInClipRect:(NSRect)aRect;
- (void)drawBackgroundInClipRect:(NSRect)aRect;
- (NSRect)correctFrame:(NSRect)aFrame ofCellAtColumn:(NSInteger)aColumn row:(NSInteger)aRow;
- (NSInteger)correctRow:(NSInteger)aRow atPoint:(NSPoint)aPoint;
- (NSRect)correctRect:(NSRect)aRect ofRow:(NSInteger)aRow;
- (NSRect)correctRect:(NSRect)aRect ofColumn:(NSInteger)aColumn;
- (void)displayRow:(NSInteger)aRow;
- (BOOL)displayVisibleRows;
- (void)resetLayout;
- (void)_setColumnsWrap:(BOOL)aWrap;
- (void)_becomeObserver;
- (void)_outlineViewItemDidCollapse:(id)aCollapse;
- (void)_outlineViewItemDidExpand:(id)aExpand;
- (void)_reflectScrolledView:(id)aView;
- (void)_columnDidResize:(id)aResize;
- (BOOL)_displayRowsInRect:(NSRect)aRect;
- (void)_postDisplayRows:(id)aRows;
- (NSInteger)_rowAtPoint:(NSPoint)aPoint;
- (BOOL)_getOrigin:(float *)aOrigin height:(float *)aHeight ofRow:(NSInteger)aRow;
- (void)_calcHeightsInRange:(NSRange)aRange timeout:(double)aTimeout display:(BOOL)aDisplay;
- (NSInteger)_storeLayoutInfo:(struct _layout_info_t *)aInfo;
- (NSInteger)_indexAtOrigin:(NSInteger)aOrigin;
- (NSInteger)_indexAtRow:(NSInteger)aRow;
- (void)_displayRowsAtIndex:(NSInteger)aIndex;
- (void)_invalidateLayout;
- (id)outlineView;
@end

