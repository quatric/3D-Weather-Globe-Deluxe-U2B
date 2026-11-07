// C3DViewWrapper: the NSOpenGLView a C3DView draws in, passing it the
// mouse and keyboard events.

#ifndef C3DVIEWWRAPPER_H
#define C3DVIEWWRAPPER_H

#import <Cocoa/Cocoa.h>
#include "FHContext.h"

class GLBugFixes;

@interface C3DViewWrapper : NSOpenGLView {
	C3DView *_view;
	LPoint _mouseDownPoint;
	char _mouseDragOperation;
	char _trackingMouseLeave;
	char _isInLiveResize;
	char _isDrawing;
	NSSize _oldSize;
	GLBugFixes *_glBugFixes;
	NSImage *_cachedImage;
	NSUInteger _prevModifierFlags;
}
- (C3DView *)view;
- (void)_adjustToView:(C3DView *)view;
- (NSImage *)_cachedImage;
// A picture of the view shown while it cannot draw (live resizing, capture).
- (void)createCachedImage;
- (void)discardCachedImage;
- (NSImage *)_snapshotImageFromRect:(NSRect)rect;
- (NSBitmapImageRep *)_snapshotBitmapFromRect:(NSRect)rect;
- (float)_userSpaceScaleFactor;
- (NSPoint)_scaleViewPointToUserSpace:(NSPoint)point;
- (NSPoint)_viewPointFromEvent:(NSEvent *)event;
- (void)_setupOpenGLContext;
- (void)_drawGLBugFixes;
@end

// Shows a view's cached image in its place.
@interface _C3DCachedImageView : NSView {
	NSImage *_image;
}
- (id)initWithFrame:(NSRect)frame image:(NSImage *)image;
@end

@interface NSImage (NSImageFHExtension)
// Draws the middle of the image with the proportions of `rect` into it.
- (void)fillProportinallyInRect:(NSRect)rect operation:(NSCompositingOperation)operation fraction:(float)fraction;
@end

#endif
