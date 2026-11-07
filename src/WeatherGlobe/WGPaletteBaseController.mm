#import "WGPaletteBaseController.h"

NSString *_WGPaletteDidBecomeKey = @"WGPaletteDidBecomeKey";
NSString *_WGPaletteDidCloseNotification = @"WGPaletteDidCloseNotification";

// A nib-loaded floating palette; one with an autosaveName remembers its
// frame and whether to show it on startup.
@implementation WGPaletteBaseController

- (void)dealloc
{
	[_palette setDelegate:nil];
	[self hidePalette];
	[_palette release];
	[super dealloc];
}

- (id)_doShowOnStartupKey
{
	if (![self respondsToSelector:@selector(autosaveName)])
		return nil;
	return [NSString stringWithFormat:@"Do show %@ on startup", [(id)self autosaveName]];
}

- (void)_setShowOnStartup:(BOOL)aStartup
{
	NSString *key = [self _doShowOnStartupKey];
	if (key)
		[[NSUserDefaults standardUserDefaults] setBool:aStartup forKey:key];
}

- (void)windowDidBecomeKey:(id)aKey
{
	if ([aKey object] == _palette) {
		_paletteIsVisible = YES;
		[[NSNotificationCenter defaultCenter] postNotificationName:_WGPaletteDidBecomeKey object:self];
	}
}

- (void)windowWillClose:(id)aClose
{
	if ([aClose object] == _palette) {
		_paletteIsVisible = NO;
		[self _setShowOnStartup:NO];
		[[NSNotificationCenter defaultCenter] postNotificationName:_WGPaletteDidCloseNotification object:self];
	}
}

- (void)showPalette
{
	[self loadNibIfNeeded];
	if ([self respondsToSelector:@selector(autosaveName)])
		[_palette setFrameAutosaveName:[(id)self autosaveName]];
	[_palette makeKeyAndOrderFront:nil];
	[self _setShowOnStartup:YES];
	_paletteIsVisible = YES;
}

- (void)hidePalette
{
	_paletteIsVisible = NO;
	[_palette close];
}

- (void)togglePalette
{
	if (_paletteIsVisible)
		[self hidePalette];
	else
		[self showPalette];
}

- (BOOL)isPaletteVisible
{
	return _paletteIsVisible;
}

- (BOOL)doShowOnStartup
{
	NSString *key = [self _doShowOnStartupKey];
	return key ? [[NSUserDefaults standardUserDefaults] boolForKey:key] : NO;
}

@end
