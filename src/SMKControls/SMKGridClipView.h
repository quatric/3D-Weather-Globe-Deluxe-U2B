#import "SMKControlsPrefix.h"


@interface SMKGridClipView : NSClipView
{
}
+ (id)contentViewWithFrame:(NSRect)aFrame;
- (void)drawRect:(NSRect)aRect;
- (NSPoint)constrainScrollPoint:(NSPoint)aPoint;
- (BOOL)copiesOnScroll;
- (id)owningGridControl;
@end

