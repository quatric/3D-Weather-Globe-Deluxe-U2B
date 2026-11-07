// WGScreenSaverView: the ScreenSaverView the .saver puts up. The first one
// starts the ScreenSaverController (kept until exit). Each saver view has
// its own 3D view, except the preview's, which share one that moves into
// the view last shown (the preferences pane makes a new preview view each
// time).

#import <Cocoa/Cocoa.h>
#include "ScreenSaverLib.h"

static ScreenSaverController *sScreenSaverController = 0;
static bool sUsesSharedGLView = false;
static ScreenSaverGLView *sGLView = nil;

static void _screenSaverExitFunc()
{
	delete sScreenSaverController;
	sScreenSaverController = 0;
}

@implementation WGScreenSaverView

- (id)initWithFrame:(NSRect)frame isPreview:(BOOL)isPreview
{
	self = [super initWithFrame:frame isPreview:isPreview];
	if (self == nil)
		return nil;
	if (sScreenSaverController == 0) {
		atexit(_screenSaverExitFunc);
		sScreenSaverController = new ScreenSaverController();
	}
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	if (callbacks == 0 || callbacks->_applicationDataPath(1) == 0 || callbacks->_applicationDataPath(0) == 0)
		NSLog(@"Cannot access application support folder");
	else if (callbacks->_applicationCachesPath(1) == 0 || callbacks->_applicationCachesPath(0) == 0)
		NSLog(@"Cannot access caches folder");
	else {
		if (isPublicKeyActivationNeeded())
			activatePublicKey();
		if (!isPublicKeyActivationNeeded()) {
			if (isPreview)
				sUsesSharedGLView = true;
			return self;
		}
		if (isPreview) {
			NSBundle *bundle = [NSBundle bundleForClass:[self class]];
			NSAlert *alert = [[[NSAlert alloc] init] autorelease];
			[alert setMessageText:[bundle localizedStringForKey:@"AppNotActivatedTitle" value:@"" table:@"Localizable"]];
			[alert setInformativeText:[bundle localizedStringForKey:@"AppNotActivatedMessage" value:@"" table:@"Localizable"]];
			[alert addButtonWithTitle:[bundle localizedStringForKey:@"OK" value:@"" table:@"Localizable"]];
			[alert runModal];
		}
	}
	[self release];
	return nil;
}

- (void)dealloc
{
	if (_glView)
		sScreenSaverController->destroyAcceleratedView(_glView);
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_configWindowController release];
	[_glView release];
	[super dealloc];
}

- (void)_addScreenSaverViewIfNeeded
{
	ScreenSaverGLView *view;
	if (!sUsesSharedGLView) {
		if (_glView)
			return;
		view = _glView = [[ScreenSaverGLView alloc] initWithFrame:[self bounds]];
		[self addSubview:_glView];
	} else {
		if ([sGLView superview] == self)
			return;
		if (sGLView) {
			if ([sGLView superview])
				[sGLView removeFromSuperviewWithoutNeedingDisplay];
			[sGLView setFrame:[self bounds]];
			[self addSubview:sGLView];
			return;
		}
		view = sGLView = [[ScreenSaverGLView alloc] initWithFrame:[self bounds]];
		[self addSubview:sGLView];
	}
	sScreenSaverController->createAcceleratedView(view, 0);
}

// The view is added once the window is up (and again when it comes back).
- (void)viewDidMoveToWindow
{
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	if ([self window] == nil) {
		[center removeObserver:self];
		return;
	}
	[self _addScreenSaverViewIfNeeded];
	[center addObserver:self
	           selector:@selector(_addScreenSaverViewIfNeeded)
	               name:NSWindowDidBecomeKeyNotification
	             object:[self window]];
	[center addObserver:self
	           selector:@selector(_addScreenSaverViewIfNeeded)
	               name:NSWindowDidBecomeMainNotification
	             object:[self window]];
}

// The 3D view animates itself.
- (void)startAnimation {}
- (void)stopAnimation {}
- (void)drawRect:(NSRect)rect {}
- (void)animateOneFrame {}

- (BOOL)hasConfigureSheet
{
	return YES;
}

- (NSWindow *)configureSheet
{
	if (_configWindowController == nil)
		_configWindowController = [WGConfigPanelController new];
	return [_configWindowController window];
}

@end
