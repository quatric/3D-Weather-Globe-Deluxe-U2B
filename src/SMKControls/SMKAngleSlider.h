#import "SMKControlsPrefix.h"


@interface SMKAngleSlider : NSControl
{
    float _cachedValue;
}
+ (Class)cellClass;
- (void)keyDown:(id)aDown;
- (BOOL)sendAction:(SEL)aAction to:(id)aTo;
- (id)initWithFrame:(NSRect)aFrame;
- (BOOL)acceptsFirstMouse:(id)aMouse;
- (BOOL)isOpaque;
@end

