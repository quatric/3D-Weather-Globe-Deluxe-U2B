#import "AtlasGLView.h"
#import "MainWindowController.h"
#import "WGCurrentSetController.h"
#import "WGExportImageController.h"
#import "NSPasteboard+NSPasteboardWGExtensions.h"
#include "RLPanels.h"
#include "RLScenes.h"
#include "RLOptions.h"
#include "RLTextures.h"

// The main window's 3D view: it takes entries dragged from the lists (to
// the current panel, and on drop to the current set), and grabs the big
// pictures exported, showing a progress window while it renders.
@implementation AtlasGLView

- (void)awakeFromNib
{
	[self registerForDraggedTypes:[NSArray arrayWithObjects:_kEntriesListPasteboardType, _kPointsPasteboardType, nil]];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_entriesListDraggingWasEnded:)
		name:_kEntriesListDraggingEndedNotification object:nil];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_progressWnd release];
	[_cursor release];
	[super dealloc];
}

// No redraws while a picture is grabbed.
- (void)setNeedsDisplay:(BOOL)aDisplay
{
	if (!_disableRefresh)
		[super setNeedsDisplay:aDisplay];
}

- (AtlasPanelController *)_currentPanelController
{
	AppWindowInterfaceWrapper *window = [_controller windowInterface];
	return window ? window->currentPanel() : 0;
}

- (id)cursor
{
	return _cursor;
}

- (void)setCursor:(id)aCursor
{
	if (_cursor != aCursor) {
		[_cursor release];
		_cursor = [aCursor retain];
		[[self window] invalidateCursorRectsForView:self];
	}
}

- (void)resetCursorRects
{
	if (_cursor)
		[self addCursorRect:[self bounds] cursor:_cursor];
}

static NSArray *_draggedTypes()
{
	return [NSArray arrayWithObjects:_kEntriesListPasteboardType, _kPointsPasteboardType, nil];
}

// The dragged points go to the panel (stopping any flight).
- (NSUInteger)draggingEntered:(id)aEntered
{
	NSPasteboard *pasteboard = [aEntered draggingPasteboard];
	if (![pasteboard availableTypeFromArray:_draggedTypes()])
		return NSDragOperationNone;
	AppWindowInterfaceWrapper *window = [_controller windowInterface];
	if (!window)
		return NSDragOperationNone;
	AtlasPanelController *panel = window->currentPanel();
	if (!panel)
		return NSDragOperationNone;
	window->stopAnyFlying();
	NSData *data = [pasteboard dataForType:_kPointsPasteboardType];
	if (data) {
		LData *points = LData::dataWithBytesNoCopy((void *)[data bytes], (unsigned int)[data length], false);
		if (points)
			panel->dragBringsEntries(points);
	}
	return NSDragOperationCopy;
}

- (NSUInteger)draggingUpdated:(id)aUpdated
{
	return [[aUpdated draggingPasteboard] availableTypeFromArray:_draggedTypes()] ? NSDragOperationCopy : NSDragOperationNone;
}

- (void)draggingExited:(id)aExited
{
	if (![[aExited draggingPasteboard] availableTypeFromArray:_draggedTypes()])
		return;
	AtlasPanelController *panel = [self _currentPanelController];
	if (panel)
		panel->dragExitedWithAllEntries();
}

- (BOOL)prepareForDragOperation:(id)aOperation
{
	return YES;
}

- (BOOL)performDragOperation:(id)aOperation
{
	return YES;
}

// The dropped entries are added at the end of the current set.
- (void)concludeDragOperation:(id)aOperation
{
	NSPasteboard *pasteboard = [aOperation draggingPasteboard];
	if (![pasteboard availableTypeFromArray:[NSArray arrayWithObject:_kEntriesListPasteboardType]])
		return;
	AtlasPanelController *panel = [self _currentPanelController];
	if (panel)
		panel->dragExitedWithAllEntries();
	TLArray<LMap *> *list = lobj_cast<TLArray<LMap *> >([pasteboard wgPropertyListForType:_kEntriesListPasteboardType]);
	[[_controller currentSetController] addEntriesFromDescrArray:list startingFromRow:-1];
}

- (void)_entriesListDraggingWasEnded:(id)aEnded
{
	AtlasPanelController *panel = [self _currentPanelController];
	if (panel)
		panel->dragExitedWithAllEntries();
}

// Renders the picture segment by segment while the progress window runs
// modally (its button stops it), then converts it to the data format on a
// thread. Captions and the on-screen controls are hidden meanwhile.
- (id)grabScreenshotOfApproxSize:(NSSize)aSize virtualSize:(NSSize)aSize2 dataFormat:(NSInteger)aFormat
{
	unsigned char savedDisableRefresh = _disableRefresh;
	_disableRefresh = YES;
	LargeBBEmulator *emulator = (LargeBBEmulator *)LargeBBEmulator::_alloc()->initWithHolder(_view->holder())->l_autorelease();
	_destImgData = nil;
	_destImgDataFormat = (int)aFormat;
	_bitmapRepGrabCallback callback;
	bool savedGrabbing = sLargeBBGrabbingIsActive;
	sLargeBBGrabbingIsActive = true;
	{
		LSize size = {(float)aSize.width, (float)aSize.height};
		LSize virtualSize = {(float)aSize2.width, (float)aSize2.height};
		StBBGrabbingAsync grabbing(emulator, size, virtualSize, &callback);

		NSString *filename = [[_controller exportImageController] exportedImageFilename];
		[_progressText setStringValue:[NSString stringWithFormat:
			[[NSBundle mainBundle] localizedStringForKey:@"Exporting to file..." value:@"" table:nil], filename]];
		[_progressIndicator setDoubleValue:0.0];
		[_progressIndicator setUsesThreadedAnimation:YES];
		[_progressIndicator setIndeterminate:YES];
		[_progressIndicator startAnimation:self];
		[_progressButton setEnabled:YES];
		[_progressWnd center];
		[_progressWnd makeKeyAndOrderFront:self];

		BaseScene *scene = [self _currentPanelController]->scene();
		bool controlsVisible = scene->_controlsVisible;
		scene->setControlsVisibility(NO);
		scene->renderOptions()->setShowCaptions(NO);

		NSModalSession session = [NSApp beginModalSessionForWindow:_progressWnd];
		bool rendered = false;
		bool converting = false;
		NSModalResponse response;
		do {
			response = [NSApp runModalSession:session];
			if (!rendered) {
				rendered = !grabbing.tryRender();
				float progress = grabbing.renderProgress();
				[_progressIndicator setIndeterminate:NO];
				[_progressIndicator setDoubleValue:progress < 0.0f ? 0.0 : progress <= 1.0f ? progress * 100.0 : 100.0];
			}
			if (rendered && !converting) {
				[_progressIndicator setUsesThreadedAnimation:YES];
				[_progressIndicator setIndeterminate:YES];
				[_progressIndicator startAnimation:self];
				[_progressButton setEnabled:NO];
				[NSThread detachNewThreadSelector:@selector(_onImageConversion:) toTarget:self withObject:callback._rep];
				converting = true;
			}
		} while (response == NSModalResponseContinue);
		[NSApp endModalSession:session];

		scene->setControlsVisibility(controlsVisible);
		scene->renderOptions()->setShowCaptions(YES);
		[_progressWnd orderOut:self];
		[_progressIndicator setDoubleValue:0.0];
		[_progressIndicator setUsesThreadedAnimation:NO];
		[_progressIndicator stopAnimation:self];
	}
	id result = [_destImgData autorelease];
	sLargeBBGrabbingIsActive = savedGrabbing;
	[callback._rep release];
	_disableRefresh = savedDisableRefresh;
	return result;
}

// On the conversion thread.
- (NSInteger)_onImageConversion:(id)aConversion
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	NSData *data = [[aConversion representationUsingType:(NSBitmapImageFileType)_destImgDataFormat
		properties:[NSDictionary dictionary]] retain];
	[self performSelectorOnMainThread:@selector(_onImageConversionDone:) withObject:data waitUntilDone:NO];
	[pool release];
	return 0;
}

- (void)_onImageConversionDone:(id)aDone
{
	_destImgData = aDone;
	[NSApp stopModalWithCode:100];
}

- (void)onProgressWindowButton:(id)aButton
{
	[NSApp stopModalWithCode:101];
}

- (BOOL)accessibilityIsIgnored
{
	return NO;
}

// "View", named "Globe" or "Map" as the map mode is.
- (id)accessibilityAttributeValue:(id)aValue
{
	NSString *key = nil;
	if ([aValue isEqualToString:NSAccessibilityRoleDescriptionAttribute])
		key = @"View";
	else if ([aValue isEqualToString:NSAccessibilityTitleAttribute] || [aValue isEqualToString:NSAccessibilityHelpAttribute]) {
		if (Preferences::mapMode() == 0)
			key = @"Globe";
		else if (Preferences::mapMode() == 1)
			key = @"Map";
	}
	if (key)
		return [[NSBundle mainBundle] localizedStringForKey:key value:@"" table:nil];
	return [super accessibilityAttributeValue:aValue];
}

@end
