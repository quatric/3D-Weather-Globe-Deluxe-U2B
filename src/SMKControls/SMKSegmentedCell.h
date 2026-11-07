#import "SMKControlsPrefix.h"
#import "SMKBackgroundButtonCell.h"


@interface SMKSegmentedCell : SMKBackgroundButtonCell <NSCoding>
{
    id _segmentedControl;
    float _segmentWidth;
    float _minWidth;
    float _autosizeRatio;
    NSRect _frameInSegmentedControl;
    struct _SMKSegCellFlags _smkSegFlags;
    NSString *_toolTip;
    void *_reserved;
}
+ (BOOL)prefersTrackingUntilMouseUp;
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (NSInteger)compare:(id)aCompare;
- (void)setTracksPopUpMenuOnMouseDown:(BOOL)aDown;
- (BOOL)tracksPopUpMenuOnMouseDown;
- (void)setToolTip:(id)aTip;
- (id)toolTip;
- (void)setSegmentWidth:(CGFloat)aWidth;
- (CGFloat)segmentWidth;
- (BOOL)isRightCap;
- (BOOL)isLeftCap;
- (void)highlight:(BOOL)aHighlight withFrame:(NSRect)aFrame inView:(id)aView;
- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp;
- (NSSize)cellSizeForBounds:(NSRect)aBounds;
- (void)setImagePosition:(NSCellImagePosition)aPosition;
- (void)setImage:(id)aImage;
- (void)setStringValue:(id)aValue;
- (void)setFont:(id)aFont;
- (void)setAlignment:(NSTextAlignment)aAlignment;
- (void)setTitle:(id)aTitle;
- (SEL)action;
- (id)target;
- (void)setState:(NSInteger)aState;
- (void)dealloc;
- (id)initImageCell:(id)aCell;
- (id)initTextCell:(id)aCell;
@end

@interface SMKSegmentedCell (SMKSegmentedControlPrivateGeometry)
- (void)_segmentFrameChanged;
- (void)_segmentWidthAndFrameChanged;
- (void)_minSegmentWidthCouldChange;
@end

@interface SMKSegmentedCell (SMKSegmentedCell_Private)
+ (id)trackingPopUpButtonCell;
- (BOOL)showMenuWithEvent:(id)aEvent inRect:(NSRect)aRect ofView:(id)aView;
- (BOOL)trackMenuWithEvent:(id)aEvent inRect:(NSRect)aRect ofView:(id)aView;
- (void)_setAutosizeRatio:(CGFloat)aRatio;
- (BOOL)_getAutosizeRatio:(float *)aRatio;
- (void)_invalidateFrameInSegmentedControl;
- (void)_setFrameInSegmentedControl:(NSRect)aControl;
- (BOOL)_frameIsValid;
- (BOOL)_getFrameInSegmentedControl:(NSRect *)aControl;
- (void)_setSegmentedControl:(id)aControl;
- (id)_segmentedControl;
- (BOOL)_hasCustomSegmentWidth;
- (CGFloat)_segmentWidth;
- (CGFloat)_minSegmentWidth;
- (NSSize)_cellSizeAtMinimumWidth;
- (void)_resetSegmentWidth;
- (void)_setSegmentWidth:(CGFloat)aWidth;
- (BOOL)_allowsChangeState;
- (void)_setState:(NSInteger)aState;
- (void)_setRightCap:(BOOL)aCap;
- (void)_setLeftCap:(BOOL)aCap;
@end

@interface SMKSegmentedCell (SMKControlViewAccessors)
- (id)_smkSegmentedControl;
- (id)controlViewControl;
- (id)segmentedControlView:(id)aView;
- (id)segmentedControlView;
@end

