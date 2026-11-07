#import "WeatherGlobePrefix.h"


@interface WGCompassButton : NSButton
{
}
+ (Class)cellClass;
+ (void)initialize;
- (void)viewDidMoveToWindow;
- (void)viewWillMoveToWindow:(id)aWindow;
- (void)setCenterZoneActive:(BOOL)aActive;
- (BOOL)isCenterZoneActive;
- (void)setCurrentZone:(NSInteger)aZone;
- (NSInteger)currentZone;
- (void)resetToolTipRects;
- (id)initWithCoder:(id)aCoder;
@end

@interface WGCompassCell : NSButtonCell
{
    int _currentZone;
    int _clickedZone;
    BOOL _isCenterZoneActive;
    NSArray *_accessibilityObjects;
    BOOL _isContinious;
    BOOL _isTrackingOverCenter;
    int _zoneWithPressedState;
}
- (id)accessibilityFocusedUIElement;
- (id)accessibilityHitTest:(NSPoint)aTest;
- (BOOL)accessibilityIsAttributeSettable:(id)aSettable;
- (BOOL)accessibilityIsIgnored;
- (id)accessibilityAttributeValue:(id)aValue;
- (id)accessibilityAttributeNames;
- (void)performClick:(id)aClick;
- (void)setCenterZoneActive:(BOOL)aActive;
- (BOOL)isCenterZoneActive;
- (void)setCurrentZone:(NSInteger)aZone;
- (NSInteger)currentZone;
- (void)addToolTipRectsToView:(id)aView;
- (int)intValue;
- (void)stopTracking:(NSPoint)aTracking at:(NSPoint)aAt inView:(id)aView mouseIsUp:(BOOL)aUp;
- (BOOL)continueTracking:(NSPoint)aTracking at:(NSPoint)aAt inView:(id)aView;
- (BOOL)startTrackingAt:(NSPoint)aAt inView:(id)aView;
- (NSInteger)_hitZoneForPoint:(NSPoint)aPoint;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (BOOL)isOpaque;
- (id)accessibilityObjects;
- (id)_helpStringForZone:(NSInteger)aZone;
- (NSRect)_tooltipRectForZone:(NSInteger)aZone;
- (id)_segmentPressedImageForZone:(NSInteger)aZone;
- (id)_segmentImageForZone:(NSInteger)aZone;
- (id)_compassImage;
- (NSPoint)_segmentOriginForZone:(NSInteger)aZone;
- (void)dealloc;
- (id)init;
@end

@interface CompassAccessibilityObject : NSObject
{
    int _hotZone;
    NSRect _frameRect;
    WGCompassCell *_cell;
}
- (id)accessibilityFocusedUIElement;
- (void)accessibilityPerformAction:(id)aAction;
- (id)accessibilityActionDescription:(id)aDescription;
- (id)accessibilityActionNames;
- (BOOL)accessibilityIsIgnored;
- (id)accessibilityHitTest:(NSPoint)aTest;
- (BOOL)accessibilityIsAttributeSettable:(id)aSettable;
- (id)accessibilityAttributeValue:(id)aValue;
- (id)accessibilityAttributeNames;
- (int)intValue;
- (id)initWithCell:(id)aCell frameRect:(NSRect)aRect hotZone:(NSInteger)aZone;
@end

