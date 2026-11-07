#import "SMKControlsPrefix.h"


@interface SMKAttributedStringRenderer : NSObject
{
    NSTextStorage *_textStorage;
    NSLayoutManager *_layoutManager;
    NSTextContainer *_tc;
    NSAttributedString *_attrString;
    float _width;
}
+ (id)sharedRenderer;
- (id)attributedStringByFittingAttributedString:(id)aString toSize:(NSSize)aSize boundingRect:(NSRect *)aRect;
- (NSRect)boundingRectOfAttributedString:(id)aString;
- (void)drawAttributedString:(id)aString atPoint:(NSPoint)aPoint;
- (void)_prepareLayoutManagerForAttributedString:(id)aString outWholeGlyphRange:(NSRange *)aRange;
- (void)setHyphenationFactor:(CGFloat)aFactor;
- (void)setContainerWidth:(CGFloat)aWidth;
- (void)dealloc;
- (id)init;
@end

@interface SMKStringRenderer : NSObject
{
    NSTextStorage *_textStorage;
    NSLayoutManager *_layoutManager;
    NSTextContainer *_tc;
    NSString *_string;
    NSFont *_font;
    NSColor *_color;
    int _alignment;
    float _width;
}
+ (id)sharedRenderer;
- (id)stringByFittingString:(id)aString toSize:(NSSize)aSize boundingRect:(NSRect *)aRect;
- (NSRect)boundingRectOfString:(id)aString;
- (void)drawString:(id)aString atPoint:(NSPoint)aPoint withColor:(id)aColor;
- (void)_prepareLayoutManagerForString:(id)aString outWholeGlyphRange:(NSRange *)aRange;
- (void)setHyphenationFactor:(CGFloat)aFactor;
- (void)setFont:(id)aFont;
- (void)setContainerWidth:(CGFloat)aWidth;
- (void)setAlignment:(NSInteger)aAlignment;
- (void)dealloc;
- (id)init;
@end

