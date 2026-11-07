#import "SMKControlsPrefix.h"


@interface SMKSegmentedControl : NSControl <NSCoding>
{
    NSMutableArray *_cells;
    id _clickedCell;
    int _selectedSegment;
    int _keySegment;
    float _autosizedSegmentsMinSum;
    int _lastAutosizedIdx;
    int _autosizedSegmentsCount;
    NSSize _autoMinSize;
    struct _SMKSegControlFlags _smkFlags;
}
+ (Class)cellClass;
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (id)toolTipForCell:(id)aCell;
- (void)setToolTip:(id)aTip forCell:(id)aCell;
- (BOOL)selectCellWithTag:(NSInteger)aTag;
- (void)deselectAllCells;
- (void)deselectSelectedCell;
- (id)selectedCells;
- (id)selectedCell;
- (id)cells;
- (BOOL)allowsEmptySelection;
- (void)setAllowsEmptySelection:(BOOL)aSelection;
- (id)toolTipForSegment:(NSInteger)aSegment;
- (void)setToolTip:(id)aTip forSegment:(NSInteger)aSegment;
- (NSInteger)tagForSegment:(NSInteger)aSegment;
- (void)setTag:(NSInteger)aTag forSegment:(NSInteger)aSegment;
- (BOOL)isEnabledForSegment:(NSInteger)aSegment;
- (void)setEnabled:(BOOL)aEnabled forSegment:(NSInteger)aSegment;
- (BOOL)isSelectedForSegment:(NSInteger)aSegment;
- (void)setSelected:(BOOL)aSelected forSegment:(NSInteger)aSegment;
- (id)menuForSegment:(NSInteger)aSegment;
- (void)setMenu:(id)aMenu forSegment:(NSInteger)aSegment;
- (id)labelForSegment:(NSInteger)aSegment;
- (void)setLabel:(id)aLabel forSegment:(NSInteger)aSegment;
- (id)imageForSegment:(NSInteger)aSegment;
- (void)setImage:(id)aImage forSegment:(NSInteger)aSegment;
- (CGFloat)widthForSegment:(NSInteger)aSegment;
- (void)setWidth:(CGFloat)aWidth forSegment:(NSInteger)aSegment;
- (NSInteger)selectedSegment;
- (void)setSelectedSegment:(NSInteger)aSegment;
- (NSInteger)segmentCount;
- (void)setSegmentCount:(NSInteger)aCount;
- (id)nsSegmentedControl;
- (BOOL)autosizeIgnoresProportions;
- (void)setAutosizeIgnoresProportions:(BOOL)aProportions;
- (BOOL)autosizesCells;
- (void)setAutosizesCells:(BOOL)aCells;
- (NSControlSize)controlSize;
- (void)setControlSize:(NSControlSize)aSize;
- (NSInteger)trackingMode;
- (void)setTrackingMode:(NSInteger)aMode;
- (id)clickedCell;
- (id)cellWithTag:(NSInteger)aTag;
- (NSInteger)segmentWithTag:(NSInteger)aTag;
- (id)cellAtPoint:(NSPoint)aPoint;
- (NSInteger)segmentAtPoint:(NSPoint)aPoint;
- (NSRect)cellFrameForCell:(id)aCell;
- (NSRect)cellFrameForSegment:(NSInteger)aSegment;
- (id)cellForSegment:(NSInteger)aSegment;
- (NSInteger)segmentForCell:(id)aCell;
- (void)viewDidMoveToWindow;
- (void)viewWillMoveToWindow:(id)aWindow;
- (void)setFont:(id)aFont;
- (BOOL)sendAction:(SEL)aAction to:(id)aTo;
- (void)updateCell:(id)aCell;
- (void)setFrame:(NSRect)aFrame;
- (void)setFrameSize:(NSSize)aSize;
- (void)sizeToFit;
- (void)rightMouseDown:(id)aDown;
- (void)mouseDown:(id)aDown;
- (void)drawRect:(NSRect)aRect;
- (BOOL)isFlipped;
- (BOOL)isOpaque;
- (BOOL)resignFirstResponder;
- (BOOL)becomeFirstResponder;
- (void)performClick:(id)aClick;
- (void)moveLeft:(id)aLeft;
- (void)moveRight:(id)aRight;
- (void)_setUpdatesDelayed:(BOOL)aDelayed;
- (id)_addCell;
- (NSInteger)_firstSelectedSegmentExceptSegment:(NSInteger)aSegment;
- (BOOL)_getSegment:(int *)aSegment cell:(id *)aCell withTag:(NSInteger)aTag;
- (void)_numberOfCellFramesChanged;
- (BOOL)_getSegment:(int *)aSegment cell:(id *)aCell atPoint:(NSPoint)aPoint cellFrame:(NSRect *)aFrame;
- (void)_ensureFramesAreValid;
- (void)_autosizeCellsWithSize:(NSSize)aSize;
- (void)_sizeCellsToMinimum;
- (void)_calculateAutosizeInfo;
- (void)_calculateCellFrames;
- (void)dealloc;
- (id)initWithFrame:(NSRect)aFrame;
- (id)init;
@end

@interface SMKSegmentedControl (KeyboardUI)
- (id)keyCell;
- (void)setKeyCell:(id)aCell;
- (BOOL)tabKeyTraversesCells;
- (void)setTabKeyTraversesCells:(BOOL)aCells;
- (void)setKeySegment:(NSInteger)aSegment;
@end

@interface SMKSegmentedControl (SMKSegmentedControl_ToolTipsSupport)
- (id)accessibilityFocusedUIElement;
- (id)accessibilityHitTest:(NSPoint)aTest;
- (BOOL)accessibilityIsAttributeSettable:(id)aSettable;
- (BOOL)accessibilityIsIgnored;
- (id)accessibilityAttributeValue:(id)aValue;
- (void)_setToolTipSupport:(BOOL)aSupport;
- (id)view:(id)aView stringForToolTip:(NSInteger)aTip point:(NSPoint)aPoint userData:(void *)aData;
- (void)_resetToolTips;
- (void)_setupToolTips;
- (void)_cleanupToolTips;
@end

@interface SMKSegmentedControl (SMKSegmentedControl_Private)
- (void)_toolTipChangedForCell:(id)aCell;
- (BOOL)_allowsChangeStateForCell:(id)aCell;
- (BOOL)_compareCell:(id)aCell toCell:(id)aCell2 result:(int *)aResult;
- (void)_toggleAutosizeForCell:(id)aCell;
- (void)_cellContentWillChange:(id)aChange;
- (BOOL)_shouldSetNewState:(NSInteger)aState forCell:(id)aCell;
@end

