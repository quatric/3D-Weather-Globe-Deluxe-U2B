#import "SMKControlsPrefix.h"


@interface SMKTintedView : NSView <NSCoding>
{
    NSColor *_borderColor;
    NSColor *_tintColor;
    NSImage *_backgroundImage;
    float _borderThickness;
    float _cornerRadius;
    struct _SMKInset _shapeInset;
    NSColor *_patternColor;
    NSImage *_offscreenCache;
    NSSize _boundsSizeForBackgroundCache;
    NSBezierPath *_shapeBezier;
    struct SMKControls_anon1 _smkFlags;
}
+ (id)boxStyleDefaultColor;
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (void)setOpaque:(BOOL)aOpaque;
- (BOOL)isOpaque;
- (void)setCornerRadius:(CGFloat)aRadius;
- (CGFloat)cornerRadius;
- (void)setBorderThickness:(CGFloat)aThickness;
- (CGFloat)borderThickness;
- (void)setBorderStyle:(NSInteger)aStyle;
- (void)_setBorderStyle:(NSInteger)aStyle force:(BOOL)aForce;
- (NSInteger)borderStyle;
- (void)drawRect:(NSRect)aRect;
- (void)_drawBoxBorderWithBounds:(NSRect)aBounds operation:(NSInteger)aOperation;
- (void)_drawBorderWithBounds:(NSRect)aBounds images:(id *)aImages operation:(NSInteger)aOperation;
- (void)_alignRects:(NSRect *)aRects forBounds:(NSRect)aBounds flippedView:(BOOL)aView;
- (id)_loadImageWithName:(id)aName;
- (void)_drawBackgroundImageInRect:(NSRect)aRect;
- (void)_drawBackgroundImageAsPatternInRect:(NSRect)aRect;
- (void)_invalidateShapeBezier;
- (BOOL)getBoxBorderInnerPathRect:(NSRect *)aRect;
- (id)_shapeBezier;
- (id)_patternColor;
- (void)setBackgroundImage:(id)aImage;
- (id)backgroundImage;
- (void)setTintColor:(id)aColor;
- (id)tintColor;
- (void)setBorderColor:(id)aColor;
- (id)borderColor;
- (void)setCacheBackground:(BOOL)aBackground;
- (BOOL)cacheBackground;
- (void)setDrawsFill:(BOOL)aFill;
- (BOOL)drawsFill;
- (void)setDrawsBorder:(BOOL)aBorder;
- (BOOL)drawsBorder;
- (void)dealloc;
- (id)initWithFrame:(NSRect)aFrame;
@end

