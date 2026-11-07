#import "WGEntryDetailsComplexDescriptionDataProvider.h"
#import "WGEntryDetailsInfoPaneController.h"

// A key with several items shown as a text view, one item per paragraph:
// "Title. text" with the title in bold.
@implementation WGEntryDetailsComplexDescriptionDataProvider

- (id)initWithOwner:(id)aOwner key:(LString *)aKey
{
	self = [super initWithOwner:aOwner key:aKey];
	if (self) {
		_textView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, WGEntryDetailsInfoDefaultFieldSize.width, 14.0)];
		[_textView setEditable:NO];
		[[NSNotificationCenter defaultCenter] addObserver:self
		                                         selector:@selector(viewFrameDidChange:)
		                                             name:NSViewFrameDidChangeNotification
		                                           object:_textView];
		[self validateDescriptionView];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_textView release];
	[super dealloc];
}

- (id)title
{
	return nil;
}

- (id)descriptionView
{
	return _textView;
}

static NSRect _usedRect(NSTextView *view)
{
	NSLayoutManager *layout = [view layoutManager];
	NSRange glyphs = [layout glyphRangeForCharacterRange:NSMakeRange(0, [[view textStorage] length]) actualCharacterRange:0];
	return [layout boundingRectForGlyphRange:glyphs inTextContainer:[view textContainer]];
}

// The layout is rebuilt when the text no longer fits the view's height.
- (void)viewFrameDidChange:(id)aChange
{
	if (_usedRect(_textView).size.height != [_textView frame].size.height)
		[[self owner] rebuildLayout];
}

- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset
{
	CGFloat width = aOffset * -2.0 + aBounds.size.width;
	[[_textView textContainer] setContainerSize:NSMakeSize(width, 10000.0)];
	return NSMakeSize(width, _usedRect(_textView).size.height);
}

- (void)validateDescriptionView
{
	EntryInfoController *info = [[self owner] entryInfoController];
	TEntryKeySet set = (TEntryKeySet)[[self owner] keySet];
	LString *key = [self key];
	NSTextStorage *storage = [_textView textStorage];
	[storage replaceCharactersInRange:NSMakeRange(0, [storage length]) withString:@" "];
	NSMutableParagraphStyle *style = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
	[style setParagraphSpacing:4.0];
	[storage addAttribute:NSParagraphStyleAttributeName value:style range:NSMakeRange(0, [storage length])];
	[storage addAttribute:NSFontAttributeName value:[NSFont systemFontOfSize:11.0] range:NSMakeRange(0, [storage length])];
	[storage addAttribute:NSForegroundColorAttributeName
	                value:[NSColor colorWithDeviceRed:0 green:0 blue:0 alpha:1]
	                range:NSMakeRange(0, [storage length])];
	int count = info ? info->itemsCountForKey(key, set, (TEntryKeyAccessMode)0) : 0;
	NSUInteger location = 0;
	for (int i = 0; i < count; i++) {
		LObject *item = info->itemForKeyAtIndex(key, i, set, (TEntryKeyAccessMode)0);
		LString *text = lobj_cast<LString>(item);
		LString *title = 0;
		if (!text) {
			if (DBTreeElement *element = lobj_cast<DBTreeElement>(item)) {
				text = lobj_cast<LString>(element->firstValueForKey(kEntryInfoItemStringKey));
				title = lobj_cast<LString>(element->firstValueForKey(kEntryInfoItemTitleKey));
			}
		}
		NSString *string = text ? (NSString *)text->cfString() : @"";
		NSString *titleString = title ? (NSString *)title->cfString() : nil;
		if (titleString)
			string = [titleString stringByAppendingFormat:@". %@", string];
		if (![string length])
			continue;
		if (i < count - 1)
			string = [string stringByAppendingFormat:@"%C", (unichar)'\n'];
		[storage replaceCharactersInRange:NSMakeRange(location, 0) withString:string];
		if (titleString)
			[storage addAttribute:NSFontAttributeName
			                value:[NSFont boldSystemFontOfSize:11.0]
			                range:NSMakeRange(location, [titleString length] + 2)];
		location += [string length];
	}
}

@end
