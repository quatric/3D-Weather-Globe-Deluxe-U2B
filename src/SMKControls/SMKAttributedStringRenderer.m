#import "SMKAttributedStringRenderer.h"

static SMKAttributedStringRenderer *_sharedAttributedStringRenderer;
static SMKStringRenderer *_sharedStringRenderer;

// A text system with a container 10000 points tall, two points of line
// fragment padding, and one space of text so it is never empty.
static void _setUpTextSystem(NSTextStorage **aStorage, NSLayoutManager **aManager, NSTextContainer **aContainer, NSFont *aFont)
{
	*aStorage = [[NSTextStorage alloc] initWithString:@" "];
	[*aStorage addAttribute:NSFontAttributeName value:aFont range:NSMakeRange(0, 1)];
	*aManager = [NSLayoutManager new];
	*aContainer = [[NSTextContainer alloc] initWithContainerSize:NSMakeSize(10000.0, 10000.0)];
	[*aContainer setHeightTracksTextView:YES];
	[*aContainer setWidthTracksTextView:NO];
	[*aContainer setLineFragmentPadding:2.0];
	[*aManager addTextContainer:*aContainer];
	[*aContainer release];
	[*aStorage addLayoutManager:*aManager];
	[*aManager release];
}

static BOOL _rectFitsInSize(NSRect aRect, NSSize aSize)
{
	NSRect box = { NSZeroPoint, aSize };
	return NSEqualRects(NSUnionRect(box, aRect), box) && aRect.size.height != 0.0;
}

// Cuts the text storage's text to what fits in the size, ending in an
// ellipsis, and returns the character range kept. The text must already be
// laid out in the container.
static NSRange _fitTextStorage(NSTextStorage *aStorage, NSLayoutManager *aManager, NSTextContainer *aContainer, NSSize aSize, NSRect *aRect)
{
	NSUInteger length = [aStorage length];
	NSRect rect = [aManager boundingRectForGlyphRange:[aManager glyphRangeForCharacterRange:NSMakeRange(0, length) actualCharacterRange:NULL] inTextContainer:aContainer];
	if (_rectFitsInSize(rect, aSize)) {
		if (aRect != NULL) {
			*aRect = rect;
		}
		return NSMakeRange(0, length);
	}
	unichar ellipsisCharacter = 0x2026;
	NSString *ellipsis = [NSString stringWithCharacters:&ellipsisCharacter length:1];
	NSRect box = { NSZeroPoint, aSize };
	NSRange glyphs = [aManager glyphRangeForBoundingRect:box inTextContainer:aContainer];
	NSUInteger kept = [aManager characterRangeForGlyphRange:glyphs actualGlyphRange:NULL].length;
	if (kept > length) {
		kept = length;
	}
	// everything after what is visible becomes the ellipsis, then characters
	// before it go until the whole fits
	NSDictionary *attributes = length != 0 ? [aStorage attributesAtIndex:(kept != 0 ? kept - 1 : 0) effectiveRange:NULL] : nil;
	NSAttributedString *tail = [[NSAttributedString alloc] initWithString:ellipsis attributes:attributes];
	[aStorage replaceCharactersInRange:NSMakeRange(kept, length - kept) withAttributedString:tail];
	[tail release];
	for (;;) {
		length = [aStorage length];
		rect = [aManager boundingRectForGlyphRange:[aManager glyphRangeForCharacterRange:NSMakeRange(0, length) actualCharacterRange:NULL] inTextContainer:aContainer];
		if (_rectFitsInSize(rect, aSize) || length <= 1) {
			break;
		}
		[aStorage deleteCharactersInRange:NSMakeRange(length - 2, 1)];
	}
	if (aRect != NULL) {
		*aRect = rect;
	}
	return NSMakeRange(0, [aStorage length]);
}

// Lays out and measures attributed strings with one shared text system.
@implementation SMKAttributedStringRenderer

+ (id)sharedRenderer
{
	if (_sharedAttributedStringRenderer == nil) {
		_sharedAttributedStringRenderer = [self new];
	}
	return _sharedAttributedStringRenderer;
}

// the string, cut with an ellipsis if it doesn't fit the size
- (id)attributedStringByFittingAttributedString:(id)aString toSize:(NSSize)aSize boundingRect:(NSRect *)aRect
{
	[self setContainerWidth:aSize.width];
	NSRange glyphs;
	[self _prepareLayoutManagerForAttributedString:aString outWholeGlyphRange:&glyphs];
	NSUInteger length = [_textStorage length];
	NSRange kept = _fitTextStorage(_textStorage, _layoutManager, _tc, aSize, aRect);
	if (kept.length == length && [_textStorage isEqualToAttributedString:aString]) {
		return [[aString retain] autorelease];
	}
	NSAttributedString *result = [[_textStorage attributedSubstringFromRange:kept] copy];
	// the storage no longer holds the string
	[_attrString release];
	_attrString = nil;
	return [result autorelease];
}

- (NSRect)boundingRectOfAttributedString:(id)aString
{
	NSRange glyphs;
	[self _prepareLayoutManagerForAttributedString:aString outWholeGlyphRange:&glyphs];
	return NSIntegralRect([_layoutManager boundingRectForGlyphRange:glyphs inTextContainer:_tc]);
}

- (void)drawAttributedString:(id)aString atPoint:(NSPoint)aPoint
{
	NSRange glyphs;
	[self _prepareLayoutManagerForAttributedString:aString outWholeGlyphRange:&glyphs];
	[_layoutManager drawGlyphsForGlyphRange:glyphs atPoint:aPoint];
}

- (void)_prepareLayoutManagerForAttributedString:(id)aString outWholeGlyphRange:(NSRange *)aRange
{
	NSUInteger length = [_textStorage length];
	if (_attrString != aString) {
		// the original released the new string here instead of the old one
		[_attrString release];
		_attrString = [aString retain];
		if (aString != nil && [aString length] != 0) {
			[_textStorage replaceCharactersInRange:NSMakeRange(0, length) withAttributedString:aString];
		}
		else {
			[_textStorage replaceCharactersInRange:NSMakeRange(0, length) withString:@" "];
		}
	}
	*aRange = [_layoutManager glyphRangeForCharacterRange:NSMakeRange(0, [_textStorage length]) actualCharacterRange:NULL];
}

// Where the layout manager hyphenates.
- (void)setHyphenationFactor:(CGFloat)aFactor
{
	if ([_layoutManager respondsToSelector:@selector(setHyphenationFactor:)] && [_layoutManager hyphenationFactor] != (float)aFactor)
		[_layoutManager setHyphenationFactor:(float)aFactor];
}

- (void)setContainerWidth:(CGFloat)aWidth
{
	if (_width != (float)aWidth) {
		[_tc setContainerSize:NSMakeSize(aWidth, 10000.0)];
		_width = (float)aWidth;
	}
}

- (void)dealloc
{
	if (self == _sharedAttributedStringRenderer) {
		_sharedAttributedStringRenderer = nil;
	}
	[_attrString release];
	[_textStorage release];
	[super dealloc];
}

- (id)init
{
	self = [super init];
	if (self != nil) {
		_setUpTextSystem(&_textStorage, &_layoutManager, &_tc, [NSFont systemFontOfSize:[NSFont smallSystemFontSize]]);
		_attrString = nil;
		_width = 0.0f;
	}
	return self;
}

@end

// Lays out and measures plain strings in one font, color and alignment with
// one shared text system.
@implementation SMKStringRenderer

+ (id)sharedRenderer
{
	if (_sharedStringRenderer == nil) {
		_sharedStringRenderer = [self new];
	}
	return _sharedStringRenderer;
}

// the string, cut with an ellipsis if it doesn't fit the size
- (id)stringByFittingString:(id)aString toSize:(NSSize)aSize boundingRect:(NSRect *)aRect
{
	[self setContainerWidth:aSize.width];
	if (aString == nil) {
		aString = @"";
	}
	NSRange glyphs;
	[self _prepareLayoutManagerForString:aString outWholeGlyphRange:&glyphs];
	NSUInteger length = [_textStorage length];
	NSRange kept = _fitTextStorage(_textStorage, _layoutManager, _tc, aSize, aRect);
	if (kept.length == length && [[_textStorage string] isEqualToString:aString]) {
		return [[aString retain] autorelease];
	}
	NSString *result = [[NSString alloc] initWithString:[[_textStorage attributedSubstringFromRange:kept] string]];
	[_string release];
	_string = nil;
	return [result autorelease];
}

- (NSRect)boundingRectOfString:(id)aString
{
	NSRange glyphs;
	[self _prepareLayoutManagerForString:aString outWholeGlyphRange:&glyphs];
	return NSIntegralRect([_layoutManager boundingRectForGlyphRange:glyphs inTextContainer:_tc]);
}

- (void)drawString:(id)aString atPoint:(NSPoint)aPoint withColor:(id)aColor
{
	if (aString == nil) {
		aString = @"";
	}
	NSRange glyphs;
	[self _prepareLayoutManagerForString:aString outWholeGlyphRange:&glyphs];
	NSUInteger length = [_textStorage length];
	if (_color != aColor) {
		if (aColor == nil) {
			[_textStorage removeAttribute:NSForegroundColorAttributeName range:NSMakeRange(0, length)];
		}
		else {
			[_textStorage addAttribute:NSForegroundColorAttributeName value:aColor range:NSMakeRange(0, length)];
		}
		[_color release];
		_color = [aColor retain];
	}
	[_layoutManager drawGlyphsForGlyphRange:glyphs atPoint:aPoint];
}

- (void)_prepareLayoutManagerForString:(id)aString outWholeGlyphRange:(NSRange *)aRange
{
	NSUInteger length = [_textStorage length];
	if (_string != aString) {
		[_string release];
		_string = [aString retain];
		[_textStorage replaceCharactersInRange:NSMakeRange(0, length)
			withString:(aString != nil && [aString length] != 0) ? aString : @" "];
		// new text takes the attributes of the old first character, which a
		// fitted string's ellipsis may not have had; keep them uniform
		NSRange all = NSMakeRange(0, [_textStorage length]);
		if (_font != nil) {
			[_textStorage addAttribute:NSFontAttributeName value:_font range:all];
		}
		if (_color != nil) {
			[_textStorage addAttribute:NSForegroundColorAttributeName value:_color range:all];
		}
	}
	*aRange = [_layoutManager glyphRangeForCharacterRange:NSMakeRange(0, [_textStorage length]) actualCharacterRange:NULL];
}

- (void)setFont:(id)aFont
{
	if (_font != aFont) {
		NSUInteger length = [_textStorage length];
		if (aFont == nil) {
			[_textStorage removeAttribute:NSFontAttributeName range:NSMakeRange(0, length)];
		}
		else {
			[_textStorage addAttribute:NSFontAttributeName value:aFont range:NSMakeRange(0, length)];
		}
		[_font release];
		_font = [aFont retain];
	}
}

// Where the layout manager hyphenates.
- (void)setHyphenationFactor:(CGFloat)aFactor
{
	if ([_layoutManager respondsToSelector:@selector(setHyphenationFactor:)] && [_layoutManager hyphenationFactor] != (float)aFactor)
		[_layoutManager setHyphenationFactor:(float)aFactor];
}

- (void)setContainerWidth:(CGFloat)aWidth
{
	if (_width != (float)aWidth) {
		[_tc setContainerSize:NSMakeSize(aWidth, 10000.0)];
		_width = (float)aWidth;
	}
}

- (void)setAlignment:(NSInteger)aAlignment
{
	if (_alignment != aAlignment) {
		[_textStorage setAlignment:(NSTextAlignment)aAlignment range:NSMakeRange(0, [_textStorage length])];
		_alignment = (int)aAlignment;
	}
}

- (void)dealloc
{
	if (self == _sharedStringRenderer) {
		_sharedStringRenderer = nil;
	}
	[_string release];
	[_font release];
	[_color release];
	[_textStorage release];
	[super dealloc];
}

- (id)init
{
	self = [super init];
	if (self != nil) {
		_setUpTextSystem(&_textStorage, &_layoutManager, &_tc, [NSFont systemFontOfSize:[NSFont systemFontSize]]);
		[_textStorage setAlignment:NSTextAlignmentLeft range:NSMakeRange(0, 1)];
		_string = nil;
		_font = nil;
		_color = nil;
		_alignment = 0;
		_width = 0.0f;
	}
	return self;
}

@end
