// C3DViewWrapper, the NSOpenGLView of a C3DView; _C3DCachedImageView; and
// NSImage's proportional fill.

#include "C3DViewWrapper.h"
#include "LKeyRemap.h"
#include <OpenGL/gl.h>
#include <OpenGL/OpenGL.h>

static unsigned int _fhModifiersFromEvent(NSEvent *event)
{
	return LKeyRemap::modifiersFromNSEventParameters((unsigned int)[event modifierFlags], (unsigned int)[event type]);
}

static LPoint _lpoint(NSPoint p)
{
	LPoint r = {(float)p.x, (float)p.y};
	return r;
}

@implementation C3DViewWrapper

+ (NSOpenGLPixelFormat *)defaultPixelFormat
{
	NSOpenGLPixelFormatAttribute attributes[] = {NSOpenGLPFAAccelerated, NSOpenGLPFADoubleBuffer,
	                                             NSOpenGLPFANoRecovery,  NSOpenGLPFAColorSize,
	                                             24,                     NSOpenGLPFAAlphaSize,
	                                             8,                      NSOpenGLPFADepthSize,
	                                             24,                     0};
	NSOpenGLPixelFormat *format = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];
	if (format == nil) {
		NSLog(@"No OpenGL pixel format");
		return [super defaultPixelFormat];
	}
	return format;
}

- (void)_setupOpenGLContext
{
	NSOpenGLContext *context = [self openGLContext];
	if (CContextCaps::VSyncEnabled()) {
		GLint interval = 1;
		[context setValues:&interval forParameter:NSOpenGLCPSwapInterval];
	}
	if (CContextCaps::MPEngineEnabled()) {
		@try {
			CGLEnable([context CGLContextObj], kCGLCEMPEngine);
		} @catch (NSException *) {
		}
	}
}

- (id)initWithFrame:(NSRect)frame pixelFormat:(NSOpenGLPixelFormat *)format
{
	self = [super initWithFrame:frame pixelFormat:format];
	if (self)
		[self _setupOpenGLContext];
	return self;
}

- (id)initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame pixelFormat:[[self class] defaultPixelFormat]];
	if (self)
		[self _setupOpenGLContext];
	return self;
}

- (id)initWithCoder:(NSCoder *)coder
{
	self = [super initWithCoder:coder];
	if (self) {
		[self setPixelFormat:[[self class] defaultPixelFormat]];
		[self _setupOpenGLContext];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	if (_view)
		_view->adjustToSystemView(0, 0);
	[_cachedImage release];
	[super dealloc];
}

- (C3DView *)view
{
	return _view;
}

// Called by the C3DView taking or leaving this view.
- (void)_adjustToView:(C3DView *)view
{
	if (_view == view)
		return;
	if (_view) {
		_view->_context->_device->ValidateContext();
		LRelease(_glBugFixes);
		_glBugFixes = 0;
	}
	_view = view;
	[self setNeedsDisplay:YES];
}

- (NSImage *)_snapshotImageFromRect:(NSRect)rect
{
	NSImage *image = [[[NSImage alloc] initWithSize:rect.size] autorelease];
	[image addRepresentation:[self _snapshotBitmapFromRect:rect]];
	[image setFlipped:YES];
	[image lockFocus];
	[image unlockFocus];
	return image;
}

- (NSImage *)_cachedImage
{
	return _cachedImage;
}

// Takes a picture of the view and stops the context drawing to the
// screen (surface opacity off) while it is shown.
- (void)createCachedImage
{
	[self discardCachedImage];
	[self displayIfNeeded];
	_cachedImage = [[self _snapshotImageFromRect:[self bounds]] retain];
	GLint value = -1;
	[[self openGLContext] setValues:&value forParameter:NSOpenGLCPSurfaceOrder];
	[self display];
}

- (void)discardCachedImage
{
	if (_cachedImage == nil)
		return;
	GLint value = 1;
	[[self openGLContext] setValues:&value forParameter:NSOpenGLCPSurfaceOrder];
	[_cachedImage release];
	_cachedImage = nil;
	[self setNeedsDisplay:YES];
}

- (void)_windowDidResignKey
{
	_prevModifierFlags = 0;
	if (_view)
		_view->resignFocus();
}

- (BOOL)isOpaque
{
	return YES;
}

- (void)viewDidMoveToWindow
{
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	if ([self window] == nil)
		[center removeObserver:self name:NSWindowDidResignKeyNotification object:nil];
	else
		[center addObserver:self
		           selector:@selector(_windowDidResignKey)
		               name:NSWindowDidResignKeyNotification
		             object:[self window]];
}

- (void)viewWillStartLiveResize
{
	[super viewWillStartLiveResize];
	if (![[[self window] contentView] inLiveResize])
		[self createCachedImage];
}

- (void)viewDidEndLiveResize
{
	[self discardCachedImage];
	[super viewDidEndLiveResize];
}

- (void)removeFromSuperviewWithoutNeedingDisplay
{
	bool cache = _cachedImage == nil;
	if (cache)
		[self createCachedImage];
	[super removeFromSuperviewWithoutNeedingDisplay];
	if (cache)
		[self discardCachedImage];
}

- (float)_userSpaceScaleFactor
{
	NSWindow *window = [self window];
	if ([window respondsToSelector:@selector(userSpaceScaleFactor)])
		return (float)[window userSpaceScaleFactor];
	return 1.0f;
}

- (NSPoint)_scaleViewPointToUserSpace:(NSPoint)point
{
	float scale = [self _userSpaceScaleFactor];
	return NSMakePoint(scale * point.x, scale * point.y);
}

// Top-down view coordinates.
- (NSPoint)_viewPointFromEvent:(NSEvent *)event
{
	NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
	p = [self _scaleViewPointToUserSpace:p];
	if (![self isFlipped])
		p.y = [self bounds].size.height - p.y;
	return p;
}

- (void)mouseDown:(NSEvent *)event
{
	if (_view == 0)
		return;
	NSInteger clicks = [event clickCount];
	if (clicks == 1) {
		_mouseDownPoint = _lpoint([self _viewPointFromEvent:event]);
		_view->mouseDown(_fhModifiersFromEvent(event), _mouseDownPoint);
	} else if (clicks == 2)
		_view->mouseDblClick((unsigned int)[event buttonNumber], _lpoint([self _viewPointFromEvent:event]));
}

- (void)mouseDragged:(NSEvent *)event
{
	if (_view == 0)
		return;
	if (!_mouseDragOperation) {
		_view->dragStarted(_fhModifiersFromEvent(event), _mouseDownPoint);
		_mouseDragOperation = 1;
	}
	_view->mouseDragged(_fhModifiersFromEvent(event), _lpoint([self _viewPointFromEvent:event]));
}

- (void)mouseUp:(NSEvent *)event
{
	if (_view == 0)
		return;
	if (_mouseDragOperation) {
		_view->dragFinished(_fhModifiersFromEvent(event), _lpoint([self _viewPointFromEvent:event]));
		_mouseDragOperation = 0;
	}
	if ([event clickCount] < 3)
		_view->mouseUp(_fhModifiersFromEvent(event), _lpoint([self _viewPointFromEvent:event]));
}

- (void)rightMouseDown:(NSEvent *)event
{
	[self mouseDown:event];
}

- (void)rightMouseDragged:(NSEvent *)event
{
	[self mouseDragged:event];
}

- (void)rightMouseUp:(NSEvent *)event
{
	[self mouseUp:event];
}

- (void)otherMouseDown:(NSEvent *)event
{
	[self mouseDown:event];
}

- (void)otherMouseDragged:(NSEvent *)event
{
	[self mouseDragged:event];
}

- (void)otherMouseUp:(NSEvent *)event
{
	[self mouseUp:event];
}

// Tells the view when the mouse leaves the bounds; moves inside them pass.
- (void)mouseMoved:(NSEvent *)event
{
	if (_view == 0)
		return;
	NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
	p = [self _scaleViewPointToUserSpace:p];
	if (!_trackingMouseLeave) {
		if ([self mouse:p inRect:[self bounds]])
			_trackingMouseLeave = 1;
	} else if (![self mouse:p inRect:[self bounds]]) {
		_trackingMouseLeave = 0;
		_view->onMouseLeave(_fhModifiersFromEvent(event), _lpoint([self _viewPointFromEvent:event]));
	}
	if (_trackingMouseLeave)
		_view->mouseMove(_fhModifiersFromEvent(event), _lpoint([self _viewPointFromEvent:event]));
}

- (void)keyDown:(NSEvent *)event
{
	if (_view)
		_view->onKeyDown([event keyCode], (unsigned int)[event modifierFlags]);
}

- (void)keyUp:(NSEvent *)event
{
	if (_view)
		_view->onKeyUp([event keyCode], (unsigned int)[event modifierFlags]);
}

// Modifier keys go down and up as keys 0x1000 (shift) to 0x1003 (command).
- (void)flagsChanged:(NSEvent *)event
{
	NSUInteger flags = [event modifierFlags];
	NSUInteger changed = flags ^ _prevModifierFlags;
	if (changed == 0)
		return;
	if (_view) {
		static const struct {
			NSUInteger mask;
			unsigned int key;
		} keys[] = {{NSEventModifierFlagCommand, 0x1003},
		            {NSEventModifierFlagOption, 0x1002},
		            {NSEventModifierFlagControl, 0x1001},
		            {NSEventModifierFlagShift, 0x1000}};
		for (unsigned int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
			if (!(changed & keys[i].mask))
				continue;
			if (flags & keys[i].mask)
				_view->onKeyDown(keys[i].key, (unsigned int)flags);
			else
				_view->onKeyUp(keys[i].key, (unsigned int)flags);
		}
	}
	_prevModifierFlags = flags;
}

- (BOOL)acceptsFirstResponder
{
	return _view ? _view->canBecameInFocus() : NO;
}

- (BOOL)becomeFirstResponder
{
	return _view ? _view->becameInFocus() : YES;
}

- (BOOL)resignFirstResponder
{
	return _view ? _view->resignFocus() : YES;
}

- (void)scrollWheel:(NSEvent *)event
{
	if (_view == 0)
		return;
	LPoint p = _lpoint([self _viewPointFromEvent:event]);
	unsigned int modifiers = _fhModifiersFromEvent(event);
	_view->mouseWheel((short)(int)((float)[event deltaY] * 10.0f), modifiers, p);
}

- (NSBitmapImageRep *)_snapshotBitmapFromRect:(NSRect)rect
{
	int width = (int)rect.size.width, height = (int)rect.size.height;
	NSBitmapImageRep *rep = [[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
	                                                                 pixelsWide:width
	                                                                 pixelsHigh:height
	                                                              bitsPerSample:8
	                                                            samplesPerPixel:3
	                                                                   hasAlpha:NO
	                                                                   isPlanar:NO
	                                                             colorSpaceName:NSCalibratedRGBColorSpace
	                                                                bytesPerRow:width * 4
	                                                               bitsPerPixel:32] autorelease];
	if ([rep bitmapData]) {
		[[self openGLContext] makeCurrentContext];
		glReadPixels((int)rect.origin.x, (int)rect.origin.y, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
		             [rep bitmapData]);
	}
	return rep;
}

// The frame being drawn shows what asks for a redraw while it draws (a
// played back frame restores the scene, which refreshes it): with a layer,
// that request would display the view again at once, over and over, and
// the run loop's timers would never fire.
- (void)setNeedsDisplay:(BOOL)flag
{
	if (!_isDrawing)
		[super setNeedsDisplay:flag];
}

- (void)setNeedsDisplayInRect:(NSRect)rect
{
	if (!_isDrawing)
		[super setNeedsDisplayInRect:rect];
}

- (void)drawRect:(NSRect)rect
{
	if (_cachedImage == nil) {
		if (_view) {
			_view->_context->_device->ValidateContext();
			glFogi(GL_FOG_COORDINATE_SOURCE, GL_FOG_COORDINATE);
			[self _drawGLBugFixes];
			_isDrawing = 1;
			_view->drawAccelerated(true, true, true);
			_isDrawing = 0;
		}
		return;
	}
	NSRect bounds = [self bounds];
	bounds.size = [self convertSize:bounds.size fromView:nil];
	[_cachedImage fillProportinallyInRect:bounds operation:NSCompositingOperationCopy fraction:1.0f];
}

- (void)_drawGLBugFixes
{
	if (_glBugFixes == 0) {
		CHolder *holder = _view->_holder;
		if (holder)
			holder->beginAccumulationBlock();
		_glBugFixes = GLBugFixes::_alloc();
		if (_glBugFixes)
			_glBugFixes->initWithHolder(_view->_holder);
		if (holder)
			holder->endAccumulationBlock();
		if (_glBugFixes == 0)
			return;
	}
	_glBugFixes->drawVertexProgramTexcoordFix();
}

@end

@implementation _C3DCachedImageView

- (id)initWithFrame:(NSRect)frame image:(NSImage *)image
{
	self = [super initWithFrame:frame];
	if (self)
		_image = [image retain];
	return self;
}

- (void)dealloc
{
	[_image release];
	[super dealloc];
}

- (void)drawRect:(NSRect)rect
{
	[_image fillProportinallyInRect:[self bounds] operation:NSCompositingOperationCopy fraction:1.0f];
}

@end

@implementation NSImage (NSImageFHExtension)

- (void)fillProportinallyInRect:(NSRect)rect operation:(NSCompositingOperation)operation fraction:(float)fraction
{
	NSSize size = [self size];
	NSRect from = {NSZeroPoint, size};
	float ratio = rect.size.width / rect.size.height;
	if (size.width / size.height < ratio) {
		from.size.height = size.width / ratio;
		from.origin.y += (size.height - from.size.height) * 0.5f;
	} else {
		from.size.width = ratio * size.height;
		from.origin.x += (size.width - from.size.width) * 0.5f;
	}
	[self drawInRect:rect fromRect:from operation:operation fraction:fraction];
}

@end
