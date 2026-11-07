#import "CompanyLinkButton.h"
#import "NSCursor+NSCursorWGExtensions.h"

// An underlined text button that opens http://<title> unless it has an
// action; blue, red while pressed.
@implementation LinkButton

- (void)_preInit
{
	if (!_titleAttr)
		_titleAttr = [[NSMutableDictionary alloc] initWithObjectsAndKeys:[NSColor blueColor],
			NSForegroundColorAttributeName, [NSNumber numberWithInt:NSUnderlineStyleSingle],
			NSUnderlineStyleAttributeName, nil];
	if (!_altTitleAttr)
		_altTitleAttr = [[NSMutableDictionary alloc] initWithObjectsAndKeys:[NSColor redColor],
			NSForegroundColorAttributeName, [NSNumber numberWithInt:NSUnderlineStyleSingle],
			NSUnderlineStyleAttributeName, nil];
	[self setURL:[NSURL URLWithString:[NSString stringWithFormat:@"http://%@", [self title]]]];
	[self setTitle:[self title]];
	[self setAlternateTitle:[self alternateTitle]];
}

- (id)init
{
	self = [super init];
	if (self)
		[self _preInit];
	return self;
}

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self)
		[self _preInit];
	return self;
}

- (void)awakeFromNib
{
	[self _preInit];
}

- (void)dealloc
{
	[_url release];
	[_titleAttr release];
	[_altTitleAttr release];
	[super dealloc];
}

// The original used the big hand from AppKit 682 on, which every
// supported system has.
- (void)resetCursorRects
{
	_cursorRect = [[self cell] drawingRectForBounds:[self bounds]];
	[self addCursorRect:_cursorRect cursor:[NSCursor pointingHandBigCursor]];
}

- (BOOL)sendAction:(SEL)aAction to:(id)aTo
{
	if (!aAction && _url) {
		[[NSWorkspace sharedWorkspace] openURL:_url];
		return YES;
	}
	return [super sendAction:aAction to:aTo];
}

- (void)setURL:(id)aURL
{
	[aURL retain];
	[_url release];
	_url = aURL;
}

- (id)getURL
{
	return _url;
}

- (void)setColor:(id)aColor
{
	[_titleAttr setObject:aColor forKey:NSForegroundColorAttributeName];
	[self setTitle:[self title]];
}

- (void)setAlternateColor:(id)aColor
{
	[_altTitleAttr setObject:aColor forKey:NSForegroundColorAttributeName];
	[self setAlternateTitle:[self alternateTitle]];
}

- (void)setLineBreakMode:(NSInteger)aMode
{
	NSMutableParagraphStyle *style = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
	[style setLineBreakMode:(NSLineBreakMode)aMode];
	[_titleAttr setObject:style forKey:NSParagraphStyleAttributeName];
	[_altTitleAttr setObject:style forKey:NSParagraphStyleAttributeName];
	[style release];
}

- (void)setTitle:(id)aTitle
{
	[super setTitle:aTitle];
	NSMutableAttributedString *title = [[[NSMutableAttributedString alloc]
		initWithAttributedString:[self attributedTitle]] autorelease];
	[title addAttributes:_titleAttr range:NSMakeRange(0, [title length])];
	[self setAttributedTitle:title];
}

- (void)setAlternateTitle:(id)aTitle
{
	[super setAlternateTitle:aTitle];
	NSMutableAttributedString *title = [[[NSMutableAttributedString alloc]
		initWithAttributedString:[self attributedAlternateTitle]] autorelease];
	[title addAttributes:_altTitleAttr range:NSMakeRange(0, [title length])];
	[self setAttributedAlternateTitle:title];
}

@end
