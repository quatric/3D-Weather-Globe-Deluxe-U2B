#import "WGEntryInfoPopupController.h"
#import "WGEntryInfoPopupView.h"
#import "WGTexturedView.h"
#import "NSObject+SharedInstance.h"

// The info balloon shown next to the mouse for an entry: a borderless
// textured panel holding a WGEntryInfoPopupView; it fades out in a second.
@implementation WGEntryInfoPopupController

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	if (_entryInfoController) {
		_entryInfoController->release();
		_entryInfoController = 0;
	}
	[_entryInfoView release];
	[_palette release];
	[super dealloc];
}

- (void)invalidate
{
	[_fadeOutTimer invalidate];
	[super invalidate];
}

- (void)_createPaletteIfNeeded
{
	if (_palette)
		return;
	_entryInfoView = [[WGEntryInfoPopupView alloc] initWithFrame:NSZeroRect];
	[_entryInfoView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	WGTexturedView *background = [[[WGTexturedView alloc] initWithFrame:[_entryInfoView frame]] autorelease];
	[background setAutoresizesSubviews:YES];
	[background addSubview:_entryInfoView];
	static NSString *const textures[] = {
		@"bl-top-left-03", @"bl-top-center-03", @"bl-top-right-03",
		@"bl-midl-left-03", @"bl-center-03", @"bl-midl-right-03",
		@"bl-down-left-03", @"bl-down-center-03", @"bl-down-right-03"
	};
	for (int i = 0; i < 9; i++)
		[background setImage:[NSImage imageNamed:textures[i]] forTexture:i];
	_palette = [[NSPanel alloc] initWithContentRect:[background frame] styleMask:NSWindowStyleMaskUtilityWindow
		backing:NSBackingStoreBuffered defer:YES];
	[_palette setContentView:background];
	[_palette setBackgroundColor:[NSColor clearColor]];
	[_palette setLevel:CGWindowLevelForKey(kCGScreenSaverWindowLevelKey)];
	[_palette setMovableByWindowBackground:NO];
	[_palette setReleasedWhenClosed:NO];
	[_palette setHasShadow:YES];
	[_palette setOneShot:YES];
	[_palette setOpaque:NO];
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center addObserver:self selector:@selector(_hideImmediately) name:NSWindowDidBecomeKeyNotification object:nil];
	[center addObserver:self selector:@selector(_hideImmediately) name:NSWindowDidBecomeMainNotification object:nil];
}

// Title/value rows of the entry's info, except the description and the
// tree and image items.
- (id)_entryInfo
{
	NSMutableArray *info = [NSMutableArray array];
	LArray *keys = _entryInfoController->orderedKeys((TEntryKeySet)0, (TEntryKeyAccessMode)0);
	int count = keys ? keys->count() : 0;
	for (int i = 0; i < count; i++) {
		LString *key = (LString *)keys->objectAtIndex(i);
		if (key->isEqualTo(kEntryInfoDescriptionKey))
			continue;
		LString *title = EntryInfoController::parameterTitleFromKey(key);
		EntryInfoController::TEntryPropertyKind kind = EntryInfoController::rowKindFromKey(key);
		int items = _entryInfoController->itemsCountForKey(key, (TEntryKeySet)0, (TEntryKeyAccessMode)0);
		for (int j = 0; j < items; j++) {
			LObject *item = _entryInfoController->itemForKeyAtIndex(key, j, (TEntryKeySet)0, (TEntryKeyAccessMode)0);
			if (item->getClass()->isSubclassOf(LF_CLASSOF(DBTreeElement))
				|| item->getClass()->isSubclassOf(LF_CLASSOF(EntryInfoImageWrapper)))
				continue;
			LString *value = EntryInfoController::stringFromInfoObjectForKey(item, kind);
			[info addObject:[NSDictionary dictionaryWithObjectsAndKeys:
				[NSString stringReferenceFromLString:title], _kWGEntryInfoPopupTitleKey,
				[NSString stringReferenceFromLString:value], _kWGEntryInfoPopupValueKey, nil]];
		}
	}
	return info;
}

- (void)_hideImmediately
{
	[_fadeOutTimer invalidate];
	_fadeOutTimer = nil;
	[_palette close];
}

- (void)_onFadeOutTimer:(id)aTimer
{
	NSTimeInterval elapsed = [NSDate timeIntervalSinceReferenceDate] - _fadeOutStartTime;
	if (elapsed < 1.0)
		[_palette setAlphaValue:1.0 - elapsed];
	else
		[self _hideImmediately];
}

- (void)_hidePopupPalette
{
	if (!_fadeOutTimer) {
		_fadeOutTimer = [NSTimer scheduledTimerWithTimeInterval:1.0 / 30.0 target:self
			selector:@selector(_onFadeOutTimer:) userInfo:nil repeats:YES];
		_fadeOutStartTime = [NSDate timeIntervalSinceReferenceDate];
	}
}

- (unsigned char)_isVisible
{
	return [_palette isVisible];
}

+ (id *)sharedInstancePtr
{
	static id sSharedInstance = nil;
	return &sSharedInstance;
}

+ (void)showPopupPaletteForEntry:(GenericEntry *)aEntry
{
	[[self sharedInstance] _showPopupPaletteForEntry:aEntry];
}

+ (void)hidePopupPalette
{
	[[self sharedInstance] _hidePopupPalette];
}

+ (unsigned char)isVisible
{
	return [[self sharedInstance] _isVisible];
}

- (id)_userDescription
{
	LString *key = kEntryInfoDescriptionKey;
	if (!_entryInfoController->itemsCountForKey(key, (TEntryKeySet)0, (TEntryKeyAccessMode)0))
		return nil;
	LObject *item = _entryInfoController->itemForKeyAtIndex(key, 0, (TEntryKeySet)0, (TEntryKeyAccessMode)0);
	if (item->getClass()->isSubclassOf(LF_CLASSOF(DBTreeElement)))
		return nil;
	return [NSString stringReferenceFromLString:
		EntryInfoController::stringFromInfoObjectForKey(item, EntryInfoController::rowKindFromKey(key))];
}

// Fills the balloon for the entry and opens it at the mouse, kept on the
// mouse's screen.
- (void)_showPopupPaletteForEntry:(GenericEntry *)aEntry
{
	if (!aEntry) {
		[self _hidePopupPalette];
		return;
	}
	[_fadeOutTimer invalidate];
	_fadeOutTimer = nil;
	[self _createPaletteIfNeeded];
	if (!_entryInfoController)
		_entryInfoController = EntryInfoController::_alloc()->init();
	_entryInfoController->setEntry(aEntry);
	[_entryInfoView setEntryInfo:[self _entryInfo]];
	LImage *icon = aEntry->icon(0);
	[_entryInfoView setEntryIcon:icon ? icon->CGImage() : 0];
	[_entryInfoView setUserDescription:[self _userDescription]];
	[_entryInfoView setEntryName:[NSString stringReferenceFromLString:aEntry->ownName(0)]];
	[_entryInfoView setEntryKind:[NSString stringReferenceFromLString:aEntry->kind(0)]];
	[_entryInfoView setEntryRegion:[NSString stringReferenceFromLString:aEntry->fullRegionName(0)]];
	[_palette setContentSize:[_entryInfoView sizeForBestFit]];
	NSSize size = [_palette frame].size;
	NSRect visible = [[[[NSApp currentEvent] window] screen] visibleFrame];
	NSPoint origin = [NSEvent mouseLocation];
	if (visible.origin.x + visible.size.width < size.width + origin.x)
		origin.x -= size.width;
	if (visible.origin.y + visible.size.height < size.height + origin.y)
		origin.y -= size.height;
	[_palette setFrameOrigin:origin];
	[_palette setAlphaValue:1.0];
	[_palette makeKeyAndOrderFront:nil];
}

@end
