#import "WeatherGlobePrefix.h"


@interface CityInfoView : NSBox
{
    CityInfoController *_owner;
    NSImage *_image;
    NSView *_content;
    BOOL _isDragged;
    BOOL _needCreateImage;
    BOOL _needTearOffImage;
    NSWindow *_draggedWindow;
    NSTimer *_startSelfDraggingTimer;
    NSEvent *_mouseDownEvent;
    BOOL _continueDraggedEvent;
}
+ (void)setSelectedView:(id)aView;
+ (id)selectedView;
+ (id)_selectionView;
+ (BOOL)isDraggingInProgress;
- (void)mouseUp:(id)aUp;
- (void)mouseDragged:(id)aDragged;
- (void)mouseDown:(id)aDown;
- (void)_startSelfDragging:(id)aDragging;
- (void)_acceptFinishDragging:(id)aDragging;
- (void)_acceptPrepareForDragging:(id)aDragging;
- (void)draggedImage:(id)aImage endedAt:(NSPoint)aAt operation:(NSUInteger)aOperation;
- (void)draggedImage:(id)aImage movedTo:(NSPoint)aTo;
- (NSUInteger)draggingSourceOperationMaskForLocal:(BOOL)aLocal;
- (BOOL)isDraggingAborted;
- (void)abortDragging;
- (NSRect)opaqueBounds;
- (void)showDraggedImageIfCan;
- (void)tearOffImageIfNeeded;
- (void)createImageIfNeeded;
- (void)setFrameAnimated:(NSRect)aAnimated;
- (void)_animateDraggedWindowTo:(NSPoint)aTo;
- (void)_createDraggedImage;
- (void)_createImage;
- (void)_poofAnimationFinished;
- (void)setOwner:(id)aOwner;
- (id)owner;
- (void)setFrameSize:(NSSize)aSize;
- (void)setFrame:(NSRect)aFrame;
- (NSRect)frame;
- (void)drawRect:(NSRect)aRect;
- (BOOL)mouseDownCanMoveWindow;
- (void)dealloc;
- (id)initWithFrame:(NSRect)aFrame;
@end

@interface CityInfoSelectionView : NSView
{
}
- (void)drawRect:(NSRect)aRect;
@end

