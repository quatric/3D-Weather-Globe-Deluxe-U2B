#import "WeatherGlobePrefix.h"


@interface WGCustomToolbarButtonCell : NSButtonCell
{
    NSRect _popupIndicatorImageRect;
    NSButtonCell *_buttonCell;
    NSButtonCell *_popupIndicatorButtonCell;
    NSRect _cellFrame;
    BOOL _showPopupIndicator;
    WGBalloonPaletteController *_balloonController;
    id _popupWindowContentViewDelegate;
}
- (BOOL)accessibilityIsIgnored;
- (id)accessibilityAttributeValue:(id)aValue;
- (id)accessibilityAttributeNames;
- (void)showPopupWindow:(id)aWindow;
- (NSRect)popupIndicatorImageRectForBounds:(NSRect)aBounds;
- (NSRect)bounds;
- (id)realCellAtPoint:(NSPoint)aPoint inRect:(NSRect)aRect ofView:(id)aView;
- (void)highlight:(BOOL)aHighlight withFrame:(NSRect)aFrame inView:(id)aView;
- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)setShowPopupIndicator:(BOOL)aIndicator;
- (BOOL)showPopupIndicator;
- (void)setPopupWindowContentViewDelegate:(id)aDelegate;
- (id)popupWindowContentViewDelegate;
- (void)dealloc;
- (id)initImageCell:(id)aCell;
- (id)initTextCell:(id)aCell;
- (id)initWithCoder:(id)aCoder;
- (id)init;
@end

@interface WGAccessibilityCustomToolbarButtonCell : NSButtonCell
{
}
- (NSInteger)state;
- (void)accessibilityPerformAction:(id)aAction;
- (id)accessibilityAttributeValue:(id)aValue;
@end

@interface WGAccessibilityCustomToolbarOptionsButtonCell : NSButtonCell
{
}
- (void)accessibilityPerformAction:(id)aAction;
- (id)accessibilityAttributeValue:(id)aValue;
@end

@interface NSObject (PopupWindowContentViewDelegate)
- (BOOL)isPopupWindowAutoHiddenForCell:(id)aCell;
- (id)popupWindowContentViewForCell:(id)aCell;
@end

