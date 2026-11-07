#import "WGEntryInfoPopupView.h"
#import "WGCGImageCell.h"

NSString *_kWGEntryInfoPopupTitleKey = @"title";
NSString *_kWGEntryInfoPopupValueKey = @"value";

// The entry's info balloon: icon, name, kind and region at the top, then
// title/value rows (dictionaries with the two keys above) and the user's
// description.
@implementation WGEntryInfoPopupView

static NSTextFieldCell *_WGPopupTextCell(NSTextAlignment alignment, NSFont *font, NSColor *color)
{
	NSTextFieldCell *cell = [NSTextFieldCell new];
	[cell setAlignment:alignment];
	[cell setFont:font];
	[cell setTextColor:color];
	return cell;
}

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self) {
		_cgImageCell = [WGCGImageCell new];
		_nameCell = _WGPopupTextCell(NSTextAlignmentLeft, [NSFont boldSystemFontOfSize:16.0], [NSColor whiteColor]);
		_headerCell = _WGPopupTextCell(NSTextAlignmentLeft, [NSFont systemFontOfSize:12.0], [NSColor whiteColor]);
		_titleCell = _WGPopupTextCell(NSTextAlignmentRight, [NSFont boldSystemFontOfSize:11.0],
			[NSColor colorWithDeviceWhite:1.0 alpha:0.5]);
		_valueCell = _WGPopupTextCell(NSTextAlignmentLeft, [NSFont systemFontOfSize:11.0], [NSColor whiteColor]);
	}
	return self;
}

- (void)dealloc
{
	[_kind release];
	[_region release];
	[_userDescr release];
	[_entryInfo release];
	[_nameCell release];
	[_headerCell release];
	[_cgImageCell release];
	[_titleCell release];
	[_valueCell release];
	[super dealloc];
}

- (void)setEntryIcon:(struct CGImage *)aIcon
{
	[_cgImageCell setObjectValue:(id)aIcon];
	[self setNeedsDisplay:YES];
}

- (void)setEntryName:(id)aName
{
	[_nameCell setStringValue:aName ? aName : @""];
	[self setNeedsDisplay:YES];
	_isSizeValid = NO;
}

- (void)setEntryKind:(id)aKind
{
	[aKind retain];
	[_kind release];
	_kind = aKind;
	[self setNeedsDisplay:YES];
	_isSizeValid = NO;
}

- (void)setEntryRegion:(id)aRegion
{
	[aRegion retain];
	[_region release];
	_region = aRegion;
	[self setNeedsDisplay:YES];
	_isSizeValid = NO;
}

- (void)setUserDescription:(id)aDescription
{
	[aDescription retain];
	[_userDescr release];
	_userDescr = aDescription;
	[self setNeedsDisplay:YES];
	_isSizeValid = NO;
}

- (void)setEntryInfo:(id)aInfo
{
	[aInfo retain];
	[_entryInfo release];
	_entryInfo = aInfo;
	[self setNeedsDisplay:YES];
	_isSizeValid = NO;
}

// The header is at least 62 points (the 54 point icon) tall.
static CGFloat _WGPopupHeaderHeight(CGFloat height)
{
	return height >= 54.0 ? height + 4.0 + 4.0 : 62.0;
}

- (NSSize)sizeForBestFit
{
	if (_isSizeValid)
		return _sizeForBestFit;
	_bestSizeForTitles = NSMakeSize(100.0, 0.0);
	_bestSizeForValues = NSZeroSize;
	_bestSizeForHeader = NSZeroSize;
	NSSize size;
	if (_region) {
		[_headerCell setStringValue:_region];
		size = [_headerCell cellSize];
		_bestSizeForHeader.height += size.height;
		_bestSizeForHeader.width = MAX(_bestSizeForHeader.width, size.width);
	}
	if (_kind) {
		[_headerCell setStringValue:_kind];
		size = [_headerCell cellSize];
		_bestSizeForHeader.height += size.height;
		_bestSizeForHeader.width = MAX(_bestSizeForHeader.width, size.width);
	}
	CGFloat nameWidth = MAX(_bestSizeForHeader.width, 246.0);
	size = [_nameCell cellSizeForBounds:NSMakeRect(0.0, 0.0, nameWidth, 0.0)];
	_bestSizeForHeader.height += size.height;
	_bestSizeForHeader.width = MAX(_bestSizeForHeader.width, size.width);

	NSUInteger count = [_entryInfo count];
	for (NSUInteger i = 0; i < count; i++) {
		NSDictionary *info = [_entryInfo objectAtIndex:i];
		NSString *title = [info objectForKey:_kWGEntryInfoPopupTitleKey];
		NSString *value = [info objectForKey:_kWGEntryInfoPopupValueKey];
		if (title) {
			[_titleCell setStringValue:title];
			_bestSizeForTitles.height += [_titleCell cellSizeForBounds:NSMakeRect(0.0, 0.0, 100.0, 0.0)].height;
		}
		if (value) {
			[_valueCell setStringValue:value];
			size = [_valueCell cellSizeForBounds:NSMakeRect(0.0, 0.0, 200.0, 0.0)];
			_bestSizeForValues.height += size.height;
			_bestSizeForValues.width = MAX(_bestSizeForValues.width, size.width);
		}
	}
	CGFloat width = MAX(_bestSizeForHeader.width + 58.0,
		_bestSizeForTitles.width + 4.0 + _bestSizeForValues.width);
	CGFloat rows = MAX(_bestSizeForTitles.height, _bestSizeForValues.height);
	CGFloat spacing = count ? count * 4.0 - 4.0 : 0.0;
	_sizeForBestFit.height = spacing + rows + _WGPopupHeaderHeight(_bestSizeForHeader.height) + 10.0;
	_sizeForBestFit.width = width + 10.0 + 10.0;
	if (_userDescr) {
		[_valueCell setStringValue:_userDescr];
		size = [_valueCell cellSizeForBounds:NSMakeRect(0.0, 0.0, width, 0.0)];
		_sizeForBestFit.height += size.height + 4.0 + 4.0;
		if (width < size.width)
			_sizeForBestFit.width += size.width - width;
	}
	_isSizeValid = YES;
	return _sizeForBestFit;
}

- (BOOL)isFlipped
{
	return YES;
}

- (void)drawRect:(NSRect)aRect
{
	[self sizeForBestFit];
	CGFloat valueX = _bestSizeForTitles.width + 10.0 + 4.0;
	CGFloat top = _WGPopupHeaderHeight(_bestSizeForHeader.height);
	NSRect bounds = [self bounds];
	CGFloat titleWidth = _bestSizeForTitles.width;
	CGFloat valueWidth = bounds.size.width - valueX - 10.0;
	CGFloat nameWidth = bounds.size.width - 68.0 - 10.0;
	[_cgImageCell drawWithFrame:NSMakeRect(10.0, 4.0, 54.0, 54.0) inView:self];

	NSRect rect = NSMakeRect(68.0, 4.0, nameWidth, [_nameCell cellSizeForBounds:
		NSMakeRect(68.0, 4.0, nameWidth, _bestSizeForHeader.height)].height);
	if (NSIntersectsRect(rect, aRect))
		[_nameCell drawWithFrame:rect inView:self];
	CGFloat y = 4.0, lastHeight = rect.size.height;
	if (_kind) {
		[_headerCell setStringValue:_kind];
		y = lastHeight + 4.0;
		rect = NSMakeRect(68.0, y, nameWidth, [_headerCell cellSize].height);
		if (NSIntersectsRect(rect, aRect))
			[_headerCell drawWithFrame:rect inView:self];
		lastHeight = rect.size.height;
	}
	if (_region) {
		[_headerCell setStringValue:_region];
		y += lastHeight;
		rect = NSMakeRect(68.0, y, nameWidth, [_headerCell cellSize].height);
		if (NSIntersectsRect(rect, aRect))
			[_headerCell drawWithFrame:rect inView:self];
		lastHeight = rect.size.height;
	}
	NSRect separator = NSMakeRect(8.0, lastHeight + y + 4.0, bounds.size.width - 16.0, 1.0);
	if (NSIntersectsRect(separator, aRect)) {
		[[_titleCell textColor] set];
		NSFrameRect(separator);
	}

	CGFloat rowY = top;
	NSUInteger count = [_entryInfo count];
	for (NSUInteger i = 0; i < count; i++) {
		NSDictionary *info = [_entryInfo objectAtIndex:i];
		NSString *title = [info objectForKey:_kWGEntryInfoPopupTitleKey];
		NSString *value = [info objectForKey:_kWGEntryInfoPopupValueKey];
		CGFloat titleHeight = 0.0, valueHeight = 0.0;
		if (title) {
			[_titleCell setStringValue:title];
			titleHeight = [_titleCell cellSizeForBounds:NSMakeRect(10.0, top, titleWidth,
				_bestSizeForTitles.height)].height;
			rect = NSMakeRect(10.0, rowY, titleWidth, titleHeight);
			if (NSIntersectsRect(rect, aRect))
				[_titleCell drawWithFrame:rect inView:self];
		}
		if (value) {
			[_valueCell setStringValue:value];
			valueHeight = [_valueCell cellSizeForBounds:NSMakeRect(valueX, top, valueWidth,
				_bestSizeForValues.height)].height;
			// Without a title the value moves half the title column left.
			rect = NSMakeRect(title ? valueX : valueX - titleWidth * 0.5, rowY, valueWidth, valueHeight);
			if (NSIntersectsRect(rect, aRect))
				[_valueCell drawWithFrame:rect inView:self];
		}
		rowY += MAX(titleHeight, valueHeight) + 4.0;
	}
	if (_userDescr) {
		separator.origin.y = rowY;
		if (NSIntersectsRect(separator, aRect)) {
			[[_titleCell textColor] set];
			NSFrameRect(separator);
		}
		[_valueCell setStringValue:_userDescr];
		CGFloat width = bounds.size.width - 10.0 - 10.0;
		rowY += 4.0;
		rect = NSMakeRect(10.0, rowY, width, [_valueCell cellSizeForBounds:NSMakeRect(0.0, 0.0, width, 0.0)].height);
		if (NSIntersectsRect(rect, aRect))
			[_valueCell drawWithFrame:rect inView:self];
	}
}

@end
