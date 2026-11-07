#import "WeatherGlobePrefix.h"


@interface CIPaletteView : NSView
{
    CIPaletteController *_controller;
    NSPoint _mouseLocation;
}
- (void)concludeDragOperation:(id)aOperation;
- (BOOL)performDragOperation:(id)aOperation;
- (BOOL)prepareForDragOperation:(id)aOperation;
- (void)draggingExited:(id)aExited;
- (NSUInteger)draggingUpdated:(id)aUpdated;
- (NSUInteger)draggingEntered:(id)aEntered;
- (void)moveUp:(id)aUp;
- (void)moveRight:(id)aRight;
- (void)moveLeft:(id)aLeft;
- (void)moveDown:(id)aDown;
- (void)deleteForward:(id)aForward;
- (void)keyDown:(id)aDown;
- (BOOL)acceptsFirstResponder;
- (BOOL)isFlipped;
- (id)initWithController:(id)aController;
- (id)initWithFrame:(NSRect)aFrame;
@end

