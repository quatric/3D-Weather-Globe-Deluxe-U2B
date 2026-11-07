#import "WGAutoAdjustableTextField.h"

// A text field that asks its owner to rebuild the layout when its text
// needs another size, never shorter than its initial height.
@implementation WGAutoAdjustableTextField

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self)
		_initialHeight = 18.0f;
	return self;
}

- (void)awakeFromNib
{
	_wasInitialHeightInit = YES;
	_initialHeight = [self bounds].size.height;
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(adjustSize)
		name:NSViewFrameDidChangeNotification object:self];
	[self setPostsFrameChangedNotifications:YES];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}

- (void)setOwner:(id)aOwner
{
	_owner = aOwner;
}

// The editor is remembered while the owner measures the new text.
- (void)textDidChange:(id)aChange
{
	_currentEditor = [aChange object];
	[self adjustSize];
	_currentEditor = nil;
	[super textDidChange:aChange];
}

- (id)currentEditor
{
	return _currentEditor ? _currentEditor : [super currentEditor];
}

- (void)adjustSize
{
	NSSize size = [self bounds].size;
	if (!NSEqualSizes(size, [self sizeForProposedSize:size]))
		[_owner rebuildLayout:self];
}

- (void)_calcInitialHeight
{
	_initialHeight = [[self cell] cellSizeForBounds:NSMakeRect(0.0, 0.0, 10000.0, 10000.0)].height;
}

// The width stays; the height fits the text (the edited text when editing).
- (NSSize)sizeForProposedSize:(NSSize)aSize
{
	NSSize size = aSize;
	NSText *editor = [self currentEditor];
	if (!editor) {
		NSSize cellSize = [[self cell] cellSizeForBounds:NSMakeRect(0.0, 0.0, aSize.width, aSize.height)];
		size.width = cellSize.width;
		if (!_wasInitialHeightInit && [[self stringValue] length]) {
			[self _calcInitialHeight];
			_wasInitialHeightInit = YES;
		}
		size.height = MAX(_initialHeight, cellSize.height);
	} else {
		NSTextView *textView = (NSTextView *)editor;
		NSLayoutManager *layoutManager = [textView layoutManager];
		NSRange glyphs = [layoutManager glyphRangeForCharacterRange:NSMakeRange(0, [[textView textStorage] length])
			actualCharacterRange:NULL];
		NSRect rect = NSIntegralRect([layoutManager boundingRectForGlyphRange:glyphs
			inTextContainer:[textView textContainer]]);
		if (!_wasInitialHeightInit && [[self stringValue] length]) {
			[self _calcInitialHeight];
			_wasInitialHeightInit = YES;
		}
		if (aSize.height - 4.0 != rect.size.height) {
			size.height = _initialHeight;
			if (_initialHeight < rect.size.height)
				size.height = rect.size.height + 4.0;
		}
	}
	return size;
}

@end
