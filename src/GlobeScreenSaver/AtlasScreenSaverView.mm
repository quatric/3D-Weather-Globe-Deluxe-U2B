// AtlasScreenSaverView: the saver bundle only loads the application's
// ScreenSaverLib.framework and hosts its ScreenSaverView.
//
// The original symlinked the frameworks ScreenSaverLib needs into
// ~/Library/Frameworks (or /Library/Frameworks), where the screen saver host
// found them through their @executable_path-less install names, and removed
// the links once loaded. The frameworks now use @rpath install names and
// ScreenSaverLib carries an rpath to the application's Frameworks folder, so
// no links are made.
#import <Cocoa/Cocoa.h>
#import <ScreenSaver/ScreenSaver.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

static NSString *const kApplicationBundleIdentifier = @"com.mackiev.3dweatherglobev2";

@interface AtlasScreenSaverView : ScreenSaverView
{
	ScreenSaverView *_screenSaverView;
	NSString *_applicationFrameworksPath;
}
@end

@implementation AtlasScreenSaverView

- (NSString *)_applicationBundlePath
{
	return [[[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:kApplicationBundleIdentifier] path];
}

// Tells (in the preview only) when the application isn't installed.
- (NSString *)_applicationFrameworksPath
{
	if (!_applicationFrameworksPath) {
		NSBundle *application = [NSBundle bundleWithPath:[self _applicationBundlePath]];
		if (!application) {
			if ([self isPreview]) {
				NSBundle *bundle = [NSBundle bundleForClass:[self class]];
				NSRunAlertPanel([bundle localizedStringForKey:@"NoAppTitle" value:@"" table:@"Localizable"], @"%@",
				                [bundle localizedStringForKey:@"OK" value:@"" table:@"Localizable"], nil, nil,
				                [bundle localizedStringForKey:@"NoAppMessage" value:@"" table:@"Localizable"]);
			}
			return nil;
		}
		_applicationFrameworksPath = [[application privateFrameworksPath] retain];
	}
	return _applicationFrameworksPath;
}

- (NSString *)_screenSaverFrameworkPath
{
	return [[self _applicationFrameworksPath] stringByAppendingPathComponent:@"ScreenSaverLib.framework"];
}

- (void)_createScreenSaverView
{
	if (_screenSaverView)
		return;
	NSString *path = [self _screenSaverFrameworkPath];
	if (!path)
		return;
	NSBundle *framework = [NSBundle bundleWithPath:path];
	if (!framework) {
		NSLog(@"'%@' not exists", path);
		return;
	}
	if (![framework load])
		return;
	Class viewClass = [framework principalClass];
	if ([viewClass isSubclassOfClass:[ScreenSaverView class]])
		_screenSaverView = [[viewClass alloc] initWithFrame:[self bounds] isPreview:[self isPreview]];
	if (_screenSaverView)
		[self addSubview:_screenSaverView];
}

- (instancetype)initWithFrame:(NSRect)aFrame isPreview:(BOOL)aPreview
{
	self = [super initWithFrame:aFrame isPreview:aPreview];
	if (self) {
		[self setAnimationTimeInterval:1.0 / 30.0];
		[self _createScreenSaverView];
	}
	return self;
}

- (void)dealloc
{
	[_screenSaverView release];
	[_applicationFrameworksPath release];
	[super dealloc];
}

- (void)startAnimation
{
}

- (void)stopAnimation
{
}

- (void)drawRect:(NSRect)aRect
{
	[super drawRect:aRect];
}

- (void)animateOneFrame
{
	[self setNeedsDisplay:YES];
}

- (BOOL)hasConfigureSheet
{
	return [_screenSaverView hasConfigureSheet];
}

- (NSWindow *)configureSheet
{
	return [_screenSaverView configureSheet];
}

@end

#pragma clang diagnostic pop
