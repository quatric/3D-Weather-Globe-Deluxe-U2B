#import "SMKControlsPrefix.h"


@interface SMKBackgroundAbstractRenderer : NSObject <SMKBackgroundRenderer, NSObject>
{
    float _leftCapWidth;
    float _rightCapWidth;
    float _segmentHeight;
    BOOL _sizesValid;
    NSImage * _images[20];
    unsigned int _imageInitAttempts;
}
+ (void)controlTintDidChange:(id)aChange;
+ (NSInteger)currentControlTint;
+ (NSInteger)_getCurrentControlTint;
- (void)drawBackgroundImageForObject:(id)aObject bounds:(NSRect)aBounds style:(NSUInteger)aStyle;
- (struct _SMKInset)contentInsetForObject:(id)aObject;
- (struct _SMKInset)defaultContentInset;
- (CGFloat)segmentHeightForObject:(id)aObject;
- (void)_drawFill:(NSUInteger)aFill client:(id)aClient withBounds:(NSRect)aBounds;
- (void)_drawRightCap:(NSUInteger)aCap client:(id)aClient withBounds:(NSRect *)aBounds;
- (void)_drawLeftCap:(NSUInteger)aCap client:(id)aClient withBounds:(NSRect *)aBounds;
- (CGFloat)rightCapWidthForObject:(id)aObject;
- (CGFloat)leftCapWidthForObject:(id)aObject;
- (void)_initSizes;
- (id)_imageForTag:(NSInteger)aTag;
- (id)loadImageForTag:(NSInteger)aTag;
- (BOOL)providesSeparatorImage;
- (BOOL)supportsSelectedBackground;
- (BOOL)supportsGraphiteAppearance;
- (BOOL)supportsInactiveBackground;
- (BOOL)supportsDisabledBackground;
- (NSInteger)tagByStrippingFlagsForAbsentResources:(NSInteger)aResources;
- (void)dealloc;
@end

