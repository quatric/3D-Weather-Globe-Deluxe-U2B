#import "WBAlert.h"

@interface NSObject (WBAlertDelegate)
- (void)wbAlertShowHelp:(WBAlert *)aAlert;
@end

// The nib's button widened to its title (at least 80 points); returns how
// much wider it got.
static int _WBAlertSizeButton(NSButton *button, NSString *title, NSRect *frame)
{
	*frame = [button frame];
	int oldWidth = (int)frame->size.width;
	[button setTitle:title];
	CGFloat width = [[button cell] cellSizeForBounds:NSMakeRect(frame->origin.x, frame->origin.y,
		300.0, frame->size.height)].width;
	frame->size.width = MAX(80.0, width);
	return (int)(frame->size.width - oldWidth);
}

// The nib's text field grown to its text in 300 points of height; returns
// how much taller it got.
static int _WBAlertSizeText(NSTextField *field, NSString *text, NSRect *frame)
{
	*frame = [field frame];
	int oldHeight = (int)frame->size.height;
	[field setStringValue:text];
	frame->size.height = [[field cell] cellSizeForBounds:NSMakeRect(frame->origin.x, frame->origin.y,
		frame->size.width, 300.0)].height;
	return (int)(frame->size.height - oldHeight);
}

static NSButton *_WBAlertButtonAtIndex(WBAlert *alert, NSInteger index, NSButton *right, NSButton *middle,
	NSButton *left)
{
	switch (index) {
	case 1: return right;
	case 2: return middle;
	case 3: return left;
	}
	return nil;
}

// A modal alert of up to three buttons (filled from the right), an optional
// help button and "don't show again" checkbox; the window grows to fit.
@implementation WBAlert

- (id)initWithTitle:(id)aTitle body:(id)aBody rightButt:(id)aButt middleButt:(id)aButt2 leftButt:(id)aButt3 checkboxTitle:(id)aTitle2 hasHelpButt:(BOOL)aButt4 defButtIndex:(NSInteger)aIndex cancelButtIndex:(NSInteger)aIndex2 hasCustomBgr:(BOOL)aBgr
{
	return [self initWithTitle:aTitle body:aBody rightButt:aButt middleButt:aButt2 leftButt:aButt3
		checkboxTitle:aTitle2 hasHelpButt:aButt4 defButtIndex:aIndex cancelButtIndex:aIndex2
		hasCustomBgr:aBgr withNibName:nil alertCaption:nil];
}

- (id)initWithTitle:(id)aTitle body:(id)aBody rightButt:(id)aButt middleButt:(id)aButt2 leftButt:(id)aButt3 checkboxTitle:(id)aTitle2 hasHelpButt:(BOOL)aButt4 defButtIndex:(NSInteger)aIndex cancelButtIndex:(NSInteger)aIndex2 hasCustomBgr:(BOOL)aBgr withNibName:(id)aName alertCaption:(id)aCaption
{
	self = [super init];
	if (!self)
		return nil;
	_defButt = (int)aIndex;
	_cancelButt = (int)aIndex2;
	[NSBundle loadNibNamed:aName ? aName : @"WBAlert" owner:self];

	// The titles move right over the missing ones.
	NSString *right = aButt, *middle = aButt2, *left = aButt3;
	if (!right) {
		if (middle) {
			right = middle;
			middle = left;
		} else {
			right = left;
		}
		left = nil;
	} else if (!middle) {
		middle = left;
		left = nil;
	}

	NSSize contentSize = [[_window contentView] frame].size;
	NSRect frame;
	int grow = 0;
	// The nib's _leftButt is the rightmost button: those right of the left
	// one grow leftwards.
	if (right) {
		grow = _WBAlertSizeButton(_leftButt, right, &frame);
		frame.origin.x -= grow;
		[_leftButt setFrame:frame];
	}
	if (!middle) {
		[_midButt removeFromSuperview];
		_midButt = nil;
	} else {
		grow += _WBAlertSizeButton(_midButt, middle, &frame);
		frame.origin.x -= grow;
		[_midButt setFrame:frame];
	}
	if (!left) {
		[_rightButt removeFromSuperview];
		_rightButt = nil;
	} else {
		grow += _WBAlertSizeButton(_rightButt, left, &frame);
		[_rightButt setFrame:frame];
	}
	if (!aButt4) {
		[_helpButt removeFromSuperview];
		_helpButt = nil;
	}
	if (grow > 0) {
		contentSize.width += grow;
		[_window setContentSize:contentSize];
	}

	int titleGrow = _WBAlertSizeText(_title, aTitle, &frame);
	frame.origin.y -= titleGrow;
	[_title setFrame:frame];
	grow = titleGrow + _WBAlertSizeText(_body, aBody, &frame);
	frame.origin.y -= grow;
	[_body setFrame:frame];
	if (!aTitle2) {
		grow = (int)(grow - [_dontShowButt frame].size.height);
		[_dontShowButt removeFromSuperview];
		_dontShowButt = nil;
	} else {
		[_dontShowButt setTitle:aTitle2];
	}
	if (grow > 0) {
		contentSize.height += grow;
		[_window setContentSize:contentSize];
	}
	if (aCaption)
		[_window setTitle:aCaption];

	if (aIndex > 0)
		[_WBAlertButtonAtIndex(self, aIndex, _leftButt, _midButt, _rightButt) setKeyEquivalent:@"\r"];
	if (aIndex != aIndex2 && aIndex2 > 0)
		[_WBAlertButtonAtIndex(self, aIndex2, _leftButt, _midButt, _rightButt) setKeyEquivalent:@"\x1b"];
	[_window setContentSize:contentSize];
	[_window center];

	// The nib's controls move into a borderless window over the custom
	// background (14 points wider; the controls shift right by 10).
	if (aBgr) {
		NSRect contentFrame = [[_window contentView] frame];
		NSArray *subviews = [NSArray arrayWithArray:[[_window contentView] subviews]];
		[_window release];
		contentFrame.size.width += 14.0;
		_window = [[WBAlertWindow alloc] initWithContentRect:contentFrame styleMask:NSWindowStyleMaskBorderless
			backing:NSBackingStoreBuffered defer:NO];
		[_window setMovableByWindowBackground:YES];
		[_window setOpaque:NO];
		[_window setAlphaValue:0.999];
		[_window setBackgroundColor:[NSColor clearColor]];
		[_window setHasShadow:YES];
		[_window center];
		NSUInteger count = [subviews count];
		NSRect bgrFrame;
		bgrFrame.origin = NSZeroPoint;
		bgrFrame.size = [[_window contentView] frame].size;
		[_customBgrView setFrame:bgrFrame];
		[[_window contentView] addSubview:_customBgrView];
		for (NSUInteger i = 0; i < count; i++) {
			NSView *view = [subviews objectAtIndex:i];
			NSPoint origin = [view frame].origin;
			if (![view isKindOfClass:[NSImageView class]])
				origin.x += 10.0;
			[view setFrameOrigin:origin];
			[_customBgrView addSubview:view];
		}
	}
	return self;
}

- (void)dealloc
{
	[_window release];
	[_customBgrView release];
	[super dealloc];
}

- (void)setDelegate:(id)aDelegate
{
	_delegate = aDelegate;
}

- (id)delegate
{
	return _delegate;
}

- (void)setTag:(NSInteger)aTag
{
	_alertTag = (int)aTag;
}

- (NSInteger)tag
{
	return _alertTag;
}

// Button tags are the modal codes; 4 is the help button.
- (void)buttAction:(id)aAction
{
	NSInteger tag = [aAction tag];
	if (tag == 0)
		return;
	if (tag != 4) {
		[NSApp stopModalWithCode:tag];
		return;
	}
	if ([_delegate respondsToSelector:@selector(wbAlertShowHelp:)])
		[_delegate wbAlertShowHelp:self];
}

- (NSInteger)runModal
{
	[_window makeKeyAndOrderFront:self];
	NSInteger code = [NSApp runModalForWindow:_window];
	[_window orderOut:self];
	return code;
}

- (BOOL)dontShowAgain
{
	return [_dontShowButt intValue];
}

- (void)setDontShowAgain:(BOOL)aAgain
{
	[_dontShowButt setState:aAgain ? NSControlStateValueOn : NSControlStateValueOff];
}

@end
