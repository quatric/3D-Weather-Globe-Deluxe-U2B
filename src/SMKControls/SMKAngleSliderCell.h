#import "SMKControlsPrefix.h"


@interface SMKAngleSliderCell : NSActionCell
{
    NSImage *_backgroundImage;
    NSImage *_indicatorImage;
    float _radiusMultiplier;
    BOOL _isRadiusDrawn;
    BOOL _isHighlighted;
    NSRect _cellFrame;
    BOOL _isZeroOnTop;
}
+ (void)setupImages;
+ (void)controlTintDidChangeNotification:(id)aNotification;
+ (BOOL)prefersTrackingUntilMouseUp;
+ (void)initialize;
- (id)accessibilityFocusedUIElement;
- (id)accessibilityHitTest:(NSPoint)aTest;
- (BOOL)accessibilityIsIgnored;
- (id)accessibilityAttributeValue:(id)aValue;
- (NSPoint)pointFromAngleValue:(CGFloat)aValue;
- (CGFloat)angleValueAtPoint:(NSPoint)aPoint increment:(NSInteger)aIncrement;
- (NSPoint)calcIndicatorPoint:(NSPoint)aPoint mouseFlags:(NSInteger)aFlags;
- (NSUInteger)mouseFlags;
- (CGFloat)radius;
- (id)_indicatorImage;
- (id)_backgroundImage;
- (void)setIntValue:(int)aValue;
- (int)intValue;
- (void)setFloatValue:(float)aValue;
- (float)floatValue;
- (id)objectValue;
- (void)moveInDirection:(NSInteger)aDirection withFlags:(NSInteger)aFlags;
- (void)setRadiusMultiplierToSmallestBackgroundSize:(CGFloat)aSize;
- (CGFloat)radiusMultiplierToSmallestBackgroundSize;
- (void)setZeroOnTop:(BOOL)aTop;
- (BOOL)isZeroOnTop;
- (void)setRadiusDrawingEnabled:(BOOL)aEnabled;
- (BOOL)isRadiusDrawingEnabled;
- (void)setIndicatorImage:(id)aImage;
- (void)setBackgroundImage:(id)aImage;
- (id)indicatorImage;
- (id)backgroundImage;
- (NSSize)contentSize;
- (BOOL)continueTracking:(NSPoint)aTracking at:(NSPoint)aAt inView:(id)aView;
- (BOOL)startTrackingAt:(NSPoint)aAt inView:(id)aView;
- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)update;
- (void)controlTintDidChangeNotification:(id)aNotification;
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (void)dealloc;
- (id)init;
@end

