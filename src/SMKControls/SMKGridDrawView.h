#import "SMKControlsPrefix.h"


@interface SMKGridDrawView : NSView
{
}
+ (void)initialize;
- (void)mouseDown:(id)aDown;
- (void)mouseDragged:(id)aDragged;
- (NSUInteger)draggingSourceOperationMaskForLocal:(BOOL)aLocal;
- (void)drawRect:(NSRect)aRect;
- (void)resizeWithOldSuperviewSize:(NSSize)aSize;
- (NSRect)graphicsContentRectInBounds:(NSRect)aBounds cell:(id)aCell;
- (id)dataCells;
- (id)owningGridControl;
- (BOOL)isFlipped;
- (BOOL)acceptsFirstMouse:(id)aMouse;
- (void)dealloc;
- (id)initWithFrame:(NSRect)aFrame;
@end

