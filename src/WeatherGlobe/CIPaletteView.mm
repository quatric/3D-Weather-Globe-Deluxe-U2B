#import "CIPaletteView.h"
#import "CIPaletteController.h"
#import "CityInfoView.h"
#import "NSPasteboard+NSPasteboardWGExtensions.h"

// The city info palette's content: passes the delete and arrow keys to the
// controller, takes dropped entry lists, and reports city info drags
// (CityInfoPboardType) moving over it as notifications.
@implementation CIPaletteView

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self)
		[self registerForDraggedTypes:[NSArray arrayWithObjects:@"CityInfoPboardType", _kEntriesListPasteboardType, nil]];
	return self;
}

- (id)initWithController:(id)aController
{
	self = [self initWithFrame:NSMakeRect(0, 0, 1.0, 1.0)];
	if (self)
		_controller = aController;
	return self;
}

- (BOOL)isFlipped
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (void)keyDown:(id)aDown
{
	NSString *characters = [aDown charactersIgnoringModifiers];
	SEL action = 0;
	if ([characters length] == 1) {
		switch ([characters characterAtIndex:0]) {
		case NSDeleteFunctionKey: action = @selector(deleteForward:); break;
		case NSUpArrowFunctionKey: action = @selector(moveUp:); break;
		case NSDownArrowFunctionKey: action = @selector(moveDown:); break;
		case NSLeftArrowFunctionKey: action = @selector(moveLeft:); break;
		case NSRightArrowFunctionKey: action = @selector(moveRight:); break;
		}
	}
	if (action)
		[self performSelector:action withObject:self];
	else
		[super keyDown:aDown];
}

// Only the view's own key presses reach the controller.
- (void)deleteForward:(id)aForward
{
	if (aForward == self)
		[_controller deleteForward:aForward];
}

- (void)moveDown:(id)aDown
{
	if (aDown == self)
		[_controller moveDown:aDown];
}

- (void)moveLeft:(id)aLeft
{
	if (aLeft == self)
		[_controller moveLeft:aLeft];
}

- (void)moveRight:(id)aRight
{
	if (aRight == self)
		[_controller moveRight:aRight];
}

- (void)moveUp:(id)aUp
{
	if (aUp == self)
		[_controller moveUp:aUp];
}

// A key press during the drag (Escape) aborts it.
- (void)draggingExited:(id)aExited
{
	NSEvent *event = [NSApp currentEvent];
	if ([event type] == NSEventTypeKeyDown && [[event charactersIgnoringModifiers] characterAtIndex:0]) {
		if ([[aExited draggingSource] respondsToSelector:@selector(abortDragging)])
			[[aExited draggingSource] abortDragging];
		return;
	}
	if (![[aExited draggingPasteboard] availableTypeFromArray:[NSArray arrayWithObject:_kEntriesListPasteboardType]])
		[[NSNotificationCenter defaultCenter] postNotificationName:@"DraggingExitedNotification"
			object:[[aExited draggingSource] owner] userInfo:nil];
}

- (BOOL)prepareForDragOperation:(id)aOperation
{
	if ([[aOperation draggingPasteboard] availableTypeFromArray:[NSArray arrayWithObject:@"CityInfoPboardType"]])
		[[aOperation draggingSource] showDraggedImageIfCan];
	return YES;
}

- (BOOL)performDragOperation:(id)aOperation
{
	return YES;
}

// Entry lists are copied when the controller can add them; anything else
// is reported.
- (NSUInteger)draggingEntered:(id)aEntered
{
	NSPasteboard *pasteboard = [aEntered draggingPasteboard];
	if (![pasteboard availableTypeFromArray:[NSArray arrayWithObject:_kEntriesListPasteboardType]]) {
		_mouseLocation = [aEntered draggingLocation];
		[[NSNotificationCenter defaultCenter] postNotificationName:@"DraggingEnteredNotification" object:aEntered userInfo:nil];
		return NSDragOperationEvery;
	}
	TLArray<LMap *> *list = lobj_cast<TLArray<LMap *> >([pasteboard wgPropertyListForType:_kEntriesListPasteboardType]);
	return [_controller canAddEntriesFromPropertyList:list] ? NSDragOperationCopy : NSDragOperationNone;
}

// Reported only when the mouse moved.
- (NSUInteger)draggingUpdated:(id)aUpdated
{
	NSPasteboard *pasteboard = [aUpdated draggingPasteboard];
	if (![pasteboard availableTypeFromArray:[NSArray arrayWithObject:_kEntriesListPasteboardType]]) {
		NSPoint location = [aUpdated draggingLocation];
		if (!NSEqualPoints(location, _mouseLocation)) {
			_mouseLocation = location;
			[[NSNotificationCenter defaultCenter] postNotificationName:@"DraggingUpdatedNotification" object:aUpdated userInfo:nil];
		}
		return NSDragOperationEvery;
	}
	TLArray<LMap *> *list = lobj_cast<TLArray<LMap *> >([pasteboard wgPropertyListForType:_kEntriesListPasteboardType]);
	return [_controller canAddEntriesFromPropertyList:list] ? NSDragOperationCopy : NSDragOperationNone;
}

- (void)concludeDragOperation:(id)aOperation
{
	NSPasteboard *pasteboard = [aOperation draggingPasteboard];
	if (![pasteboard availableTypeFromArray:[NSArray arrayWithObject:_kEntriesListPasteboardType]])
		return;
	TLArray<LMap *> *list = lobj_cast<TLArray<LMap *> >([pasteboard wgPropertyListForType:_kEntriesListPasteboardType]);
	if (list)
		[_controller addCities:list];
}

@end
