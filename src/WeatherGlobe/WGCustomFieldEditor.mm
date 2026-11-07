#import "WGCustomFieldEditor.h"
#import "WGEntryDetailsInfoSimpleDataProvider.h"

static NSString *_actionTitle(NSString *verb, LString *name)
{
	return [NSString stringWithFormat:@"%@ %@", [[NSBundle mainBundle] localizedStringForKey:verb value:@"" table:nil],
	                                  name ? (NSString *)name->cfString() : @""];
}

// A field editor with a focus frame whose undo goes to a model undo manager:
// undoing an edited field first commits it.
@implementation WGCustomFieldEditor

+ (id)sharedFieldEditor
{
	static WGCustomFieldEditor *sFieldEditor = nil;
	if (!sFieldEditor) {
		sFieldEditor = [[WGCustomFieldEditor alloc] initWithFrame:NSZeroRect];
		[sFieldEditor setFieldEditor:YES];
	}
	return sFieldEditor;
}

- (void)drawRect:(NSRect)aRect
{
	[super drawRect:aRect];
	[[NSColor alternateSelectedControlColor] set];
	NSFrameRectWithWidthUsingOperation([self bounds], 0.5, NSCompositingOperationSourceOver);
}

// The text differs from the control's value (its delegate's
// objectValueForControl:).
- (BOOL)isDataModified
{
	id control = [self delegate];
	id provider = [(id)control delegate];
	if (![provider respondsToSelector:@selector(objectValueForControl:)])
		return NO;
	id value = [provider objectValueForControl:control];
	if (![[self string] length] && !value)
		return NO;
	return ![[self string] isEqual:value];
}

- (void)undo:(id)aUndo
{
	if (![self isDataModified] && !(_undoManager && _undoManager->canUndo()))
		return;
	if (_undoManager) {
		[self doCommandBySelector:@selector(insertNewline:)];
		_undoManager->undo();
	} else
		[(id)[self delegate] abortEditing];
}

- (void)redo:(id)aRedo
{
	if (_undoManager && _undoManager->canRedo())
		_undoManager->redo();
}

- (void)setUndoManager:(LUndoManager *)aManager
{
	_undoManager = aManager;
}

- (BOOL)validateMenuItem:(id)aItem
{
	SEL action = [aItem action];
	if (action == @selector(undo:)) {
		if (![self isDataModified] && !(_undoManager && _undoManager->canUndo()))
			return NO;
		[aItem setTitle:_actionTitle(@"Undo", _undoManager ? _undoManager->undoActionName() : 0)];
		return YES;
	}
	if (action != @selector(redo:))
		return [super validateMenuItem:aItem];
	if (!_undoManager || !_undoManager->canRedo())
		return NO;
	[aItem setTitle:_actionTitle(@"Redo", _undoManager->redoActionName())];
	return YES;
}

@end
