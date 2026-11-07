#import "SMKGridBaseCell.h"

// bits of the cache flags word
#define kCachedImageValid 1u
#define kUpdateNeeded 2u

NSString *SMKGridAttributeVoiceOverItemTypeKey = @"SMKGridAttributeVoiceOverItemType";

static NSImage *__sLoadingStatusImage;

// optional background settings of the matrix, implemented by subclasses
@interface NSObject (SMKGridBaseCellMatrixBackground)
- (BOOL)drawsCellBackground;
- (id)cellBackgroundColor;
@end

// what NSFont's removed -defaultLineHeightForFont gave
static float SMKDefaultLineHeight(NSFont *aFont)
{
	static NSLayoutManager *layoutManager;
	if (layoutManager == nil) {
		layoutManager = [[NSLayoutManager alloc] init];
	}
	return [layoutManager defaultLineHeightForFont:aFont];
}

// A grid item: a thumbnail with a title under it. The grid matrix it is
// drawn in (its control view) holds the layout settings shared by all cells.
// The item's content is fetched from the grid control and drawn possibly
// from another thread, so the state is kept under a lock and string
// rendering under another.
@implementation SMKGridBaseCell

- (id)init
{
	self = [super init];
	if (self == nil) {
		return nil;
	}
	[self setEnabled:NO];
	_stringRenderingLock = (NSLock *)[NSRecursiveLock new];
	_lock = (NSLock *)[NSRecursiveLock new];
	return self;
}

- (void)dealloc
{
	[_lock release];
	[_stringRenderingLock release];
	[_textValue release];
	[_cachedFittingTextValue release];
	[_itemType release];
	[_cachedImage release];
	[super dealloc];
}

- (void)setState:(NSInteger)aState
{
	NSInteger old = [self state];
	[super setState:aState];
	if (old != [self state]) {
		[[self controlView] stateChangedForCell:self];
	}
}

- (id)controlView
{
	return [super controlView];
}

- (BOOL)isImageSelectionOnTop
{
	return [[self controlView] selectionStyle] == 3;
}

- (id)lock
{
	return _lock;
}

- (id)stringRenderingLock
{
	return _stringRenderingLock;
}

// the matrix's, or the framework's
- (id)loadingStatusImage
{
	NSImage *image = [[self controlView] loadingStatusImage];
	if (image == nil) {
		if (__sLoadingStatusImage == nil) {
			NSString *path = [[NSBundle bundleForClass:[self class]] pathForResource:@"loadingStatus" ofType:@"tiff"];
			__sLoadingStatusImage = [[NSImage alloc] initWithContentsOfFile:path];
			[__sLoadingStatusImage setFlipped:YES];
		}
		image = __sLoadingStatusImage;
	}
	return image;
}

// the matrix's cell content size (or the frame less the border offsets when
// it autosizes its cells), centered in aFrame
- (NSRect)interiorRectForFrame:(NSRect)aFrame inView:(id)aView
{
	NSSize size = NSZeroSize;
	if (aView != nil) {
		if (![aView autosizesCells]) {
			size = [aView cellContentSize];
		}
		else {
			NSSize offset = [aView cellBorderOffset];
			size.height = (float)offset.height * -2.0f + (float)aFrame.size.height;
			size.width = (float)offset.width * -2.0f + (float)aFrame.size.width;
		}
	}
	NSRect r;
	r.size = size;
	r.origin.y = (float)trunc((double)(float)aFrame.origin.y + (double)((float)aFrame.size.height - (float)size.height) * 0.5);
	r.origin.x = (float)trunc((double)(float)aFrame.origin.x + (double)((float)aFrame.size.width - (float)size.width) * 0.5);
	return r;
}

// Brings the content up to date (on the matrix's updating thread if it has
// one), then draws the selection and the content. Style 3 draws the image
// selection over the image.
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView
{
	if (![self isEnabled]) {
		return;
	}
	[self updateContentSize];
	NSLock *lock = [self lock];
	[lock lock];
	BOOL updateNeeded = _isUpdateNeeded;
	[lock unlock];
	if (updateNeeded) {
		if ([aView isThreadedCellUpdatingEnabled]) {
			[self updateTitleIfNeeded];
			[aView setNeedsUpdateCell:self];
		}
		else {
			[self updateContentIfNeeded];
		}
	}
	NSRect interior = [self interiorRectForFrame:aFrame inView:aView];
	if (NSIsEmptyRect(interior) || ![aView isDrawingPermitted]) {
		return;
	}
	[self drawTextSelectionInRect:interior cellFrame:aFrame];
	if (![aView isDrawingPermitted]) {
		return;
	}
	BOOL ready = [self image] != nil || [self _cachedImage] != nil;
	if (![self isImageSelectionOnTop]) {
		[self drawImageSelectionInRect:interior cellFrame:aFrame isGraphicsContentReady:ready];
	}
	[self drawInteriorWithFrame:interior inView:aView isGraphicsContentReady:ready];
	if ([self isImageSelectionOnTop]) {
		[self drawImageSelectionInRect:interior cellFrame:aFrame isGraphicsContentReady:ready];
	}
}

// A selected title gets a rounded rectangle when it takes two lines or more,
// a pill otherwise, cut to the cell's frame.
- (void)drawTextSelectionInRect:(NSRect)aRect cellFrame:(NSRect)aFrame
{
	SMKGridMatrix *matrix = [self controlView];
	NSRect text = [self textPlacementRectForBounds:aRect];
	if ([self state] == NSControlStateValueOff || ![matrix isDrawingPermitted]) {
		return;
	}
	float offset = (float)[matrix cellBorderOffset].width;
	if ([[self title] length] == 0) {
		return;
	}
	[(NSColor *)[matrix textFrameColor] set];
	float lineHeight = SMKDefaultLineHeight([matrix textFont]);
	NSBezierPath *path;
	if (lineHeight + lineHeight <= (float)text.size.height) {
		text = NSInsetRect(NSIntersectionRect(NSInsetRect(text, -offset, 0.0), aFrame), 1.0, 0.0);
		path = [NSBezierPath smkRoundedBorderPathWithRect:text cornerRadius:offset];
	}
	else {
		text = NSInsetRect(NSIntersectionRect(NSInsetRect(text, offset * -2.0f, 0.0), aFrame), 1.0, 0.0);
		path = [NSBezierPath smkRolloverStyleBorderPathWithRect:text];
	}
	[path fill];
}

// Styles 1 and 2 put a Finder style border around the thumbnail area
// (filled for 1, outlined for 2); style 3 frames the thumbnail itself.
- (void)drawImageSelectionInRect:(NSRect)aRect cellFrame:(NSRect)aFrame isGraphicsContentReady:(BOOL)aReady
{
	SMKGridMatrix *matrix = [self controlView];
	if ([self state] == NSControlStateValueOff || ![matrix isDrawingPermitted]) {
		return;
	}
	NSInteger style = [matrix selectionStyle];
	NSRect graphic = [self graphicContentPlacementRectForBounds:aRect];
	[(NSColor *)[matrix imageFrameColor] set];
	if (style == 1 || style == 2) {
		NSSize offset = [matrix cellBorderOffset];
		graphic = NSInsetRect(graphic, -(float)offset.width, -(float)offset.height);
		float inset = style == 1 ? -1.0f : 1.0f;
		graphic = NSInsetRect(graphic, inset, inset);
		NSBezierPath *path = [NSBezierPath smkFinderSelectionStyleBorderPathWithRect:graphic];
		if (style == 1) {
			[path fill];
		}
		else {
			[path stroke];
		}
		return;
	}
	if (style != 3) {
		return;
	}
	NSImage *loading = nil;
	NSRect r;
	if (!aReady && (loading = [self loadingStatusImage]) == nil) {
		r = NSZeroRect;
	}
	else {
		r = [self cachedImageRectForBounds:graphic];
	}
	if (loading != nil && !aReady) {
		r.size = [self cachedImageSizeForSize:[loading size] bounds:graphic.size];
		r.origin.x = (float)graphic.origin.x + (float)trunc((double)((float)graphic.size.width - (float)r.size.width) * 0.5);
		r.origin.y = (float)graphic.origin.y + (float)trunc((double)((float)graphic.size.height - (float)r.size.height) * 0.5);
	}
	NSFrameRectWithWidthUsingOperation(r, 2.0, NSCompositingOperationSourceOver);
	[(NSColor *)[[matrix imageFrameColor] colorWithAlphaComponent:0.5] set];
	NSFrameRectWithWidthUsingOperation(NSInsetRect(r, 2.0, 2.0), 2.0, NSCompositingOperationSourceOver);
}

// The fitted title in the matrix's string renderer (white when selected
// with a selection style), then the thumbnail or, while it loads, the
// loading status image.
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView isGraphicsContentReady:(BOOL)aReady
{
	if ([[self title] length] != 0) {
		NSRect text = [self textPlacementRectForBounds:aFrame];
		if (![aView isDrawingPermitted]) {
			return;
		}
		[_lock lock];
		NSString *fitted = [[_cachedFittingTextValue copy] autorelease];
		[_lock unlock];
		NSLock *renderingLock = [self stringRenderingLock];
		[renderingLock lock];
		SMKStringRenderer *renderer = [[self controlView] stringRenderer];
		if (_cachedFittingTextValue == nil) {
			fitted = [renderer stringByFittingString:[self title] toSize:NSMakeSize(aFrame.size.width, text.size.height)
				boundingRect:NULL];
			_cachedFittingTextValue = [fitted retain];
		}
		[renderer setContainerWidth:aFrame.size.width];
		NSColor *color = nil;
		if ([self state] != NSControlStateValueOff && [aView selectionStyle] != 0) {
			color = [NSColor whiteColor];
		}
		[renderer drawString:fitted atPoint:NSMakePoint(aFrame.origin.x, text.origin.y) withColor:color];
		[renderingLock unlock];
	}
	NSRect graphic = [self graphicContentPlacementRectForBounds:aFrame];
	if (![aView isDrawingPermitted]) {
		return;
	}
	if (aReady) {
		[self drawGraphicContentInFrame:graphic];
		return;
	}
	NSImage *loading = [self loadingStatusImage];
	if (loading != nil) {
		NSRect r;
		r.size = [self cachedImageSizeForSize:[loading size] bounds:graphic.size];
		r.origin.y = (float)graphic.origin.y + (float)trunc((double)((float)graphic.size.height - (float)r.size.height) * 0.5);
		r.origin.x = (float)graphic.origin.x + (float)trunc((double)((float)graphic.size.width - (float)r.size.width) * 0.5);
		[loading drawInRect:r fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1.0];
	}
}

// the white shadow image of the matrix, around aRect
- (void)drawShadowAroundRect:(NSRect)aRect
{
	SMKGridMatrix *matrix = [self controlView];
	NSImage *shadow = [matrix shadowCacheForSize:aRect.size];
	float blur = (float)[(NSShadow *)[matrix shadow] shadowBlurRadius];
	SMKCompositeToPoint(shadow, NSMakePoint((float)aRect.origin.x - blur, (float)NSMaxY(aRect) + blur),
		NSCompositingOperationSourceOver, 1.0);
}

// Draws the cached thumbnail, making it first (or having the updating
// thread make it) when caching is on; without caching the image is drawn
// directly.
- (void)drawGraphicContentInFrame:(NSRect)aFrame
{
	if (![self hasGraphicComponent]) {
		return;
	}
	SMKGridMatrix *matrix = [self controlView];
	BOOL caching = [matrix isCacheAllowed] && [matrix isThumbnailCachingEnabled];
	NSLock *lock = [self lock];
	[lock lock];
	NSImage *cached = [[_cachedImage retain] autorelease];
	BOOL valid = _isCachedImageValid;
	[lock unlock];
	if ((cached == nil || !valid) && caching) {
		if (![matrix isThreadedCellUpdatingEnabled]) {
			[self cacheGraphicContentWithSize:aFrame.size];
			[lock lock];
			cached = [[_cachedImage retain] autorelease];
			[lock unlock];
		}
		else {
			[matrix setNeedsUpdateCell:self];
		}
	}
	if (cached == nil && caching) {
		return;
	}
	NSRect r = [self cachedImageRectForBounds:aFrame];
	if ([matrix isGraphicContentShadowEnabled]) {
		[self drawShadowAroundRect:r];
	}
	if (!caching || cached == nil) {
		[self drawCachingGraphicContentInRect:r intoCache:nil];
		return;
	}
	// an unflipped cache is drawn mirrored about the rectangle's middle
	NSGraphicsContext *context = nil;
	BOOL unflipped = ![cached isFlipped];
	if (unflipped) {
		context = [NSGraphicsContext currentContext];
		[context saveGraphicsState];
		NSAffineTransform *transform = [NSAffineTransform transform];
		NSAffineTransformStruct t = { 1.0, 0.0, 0.0, -1.0, 0.0, (float)r.origin.y + (float)r.origin.y + (float)r.size.height };
		[transform setTransformStruct:t];
		[transform concat];
	}
	[cached drawInRect:r fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1.0];
	if (unflipped) {
		[context restoreGraphicsState];
	}
}

// a new thumbnail of the image scaled to fit aSize, nil for no image
- (id)cachedGraphicContentWithSize:(NSSize)aSize
{
	NSSize size = [self cachedImageSizeForSize:[self graphicContentSize] bounds:aSize];
	if (NSEqualSizes(size, NSZeroSize)) {
		return nil;
	}
	NSImage *image = [[[NSImage alloc] initWithSize:size] autorelease];
	[image setCacheMode:NSImageCacheNever];
	[self drawCachingGraphicContentInRect:NSMakeRect(0.0, 0.0, size.width, size.height) intoCache:image];
	return image;
}

- (void)cacheGraphicContentWithSize:(NSSize)aSize
{
	NSImage *image = [self cachedGraphicContentWithSize:aSize];
	NSLock *lock = [self lock];
	[lock lock];
	[_cachedImage release];
	_isCachedImageValid = 1;
	_cachedImage = [image retain];
	[lock unlock];
}

// the cached thumbnail when it is that size, else a new one
- (id)cachedImageWithSize:(NSSize)aSize
{
	if (aSize.width < 0.001 || aSize.height < 0.001) {
		return nil;
	}
	[self updateContentIfNeeded];
	if (![self hasGraphicComponent]) {
		return nil;
	}
	NSLock *lock = [self lock];
	[lock lock];
	NSImage *cached = nil;
	if (_isCachedImageValid) {
		cached = [[_cachedImage retain] autorelease];
	}
	[lock unlock];
	if (cached != nil && NSEqualSizes(aSize, [cached size])) {
		return cached;
	}
	return [self cachedGraphicContentWithSize:aSize];
}

- (id)cacheTextImageForBounds:(NSRect)aBounds
{
	if (NSEqualSizes(NSZeroSize, aBounds.size)) {
		return nil;
	}
	return [self cachedTextImageForBounds:aBounds];
}

// Draws the image into aRect, scaled to it: into aCache's focus, or into
// the current (flipped) context, flipping the image back upright.
- (void)drawCachingGraphicContentInRect:(NSRect)aRect intoCache:(id)aCache
{
	NSImage *image = [self image];
	if (image == nil) {
		return;
	}
	NSSize size = [self graphicContentSize];
	if (aCache != nil) {
		[aCache lockFocus];
	}
	else {
		[[NSGraphicsContext currentContext] saveGraphicsState];
	}
	NSAffineTransform *flip = [NSAffineTransform transform];
	NSAffineTransform *move = [NSAffineTransform transform];
	[move translateXBy:aRect.origin.x yBy:aRect.origin.y];
	[move concat];
	if (aCache == nil || [image isFlipped]) {
		[flip scaleXBy:1.0 yBy:-1.0];
		NSAffineTransform *up = [NSAffineTransform transform];
		[up translateXBy:0.0 yBy:aRect.size.height];
		[flip appendTransform:up];
		[flip concat];
	}
	if (!NSEqualSizes(aRect.size, size)) {
		NSAffineTransform *scale = [NSAffineTransform transform];
		[scale scaleXBy:(float)aRect.size.width / (float)size.width yBy:(float)aRect.size.height / (float)size.height];
		[scale concat];
	}
	[image drawAtPoint:NSZeroPoint fromRect:NSMakeRect(0.0, 0.0, size.width, size.height)
		operation:NSCompositingOperationSourceOver fraction:1.0];
	if (aCache != nil) {
		[aCache unlockFocus];
	}
	else {
		[[NSGraphicsContext currentContext] restoreGraphicsState];
	}
}

- (id)title
{
	[_lock lock];
	NSString *title = [_textValue copy];
	[_lock unlock];
	return [title autorelease];
}

// a new title drops the fitted one
- (void)setTitle:(id)aTitle
{
	NSLock *lock = [self lock];
	[lock lock];
	NSString *old = nil;
	NSString *oldFitted = nil;
	if (_textValue != aTitle) {
		old = _textValue;
		_textValue = [aTitle copy];
		oldFitted = _cachedFittingTextValue;
		_cachedFittingTextValue = nil;
	}
	[lock unlock];
	[old release];
	[oldFitted release];
}

- (BOOL)hasGraphicComponent
{
	return [self image] != nil;
}

- (id)image
{
	NSLock *lock = [self lock];
	[lock lock];
	NSImage *image = [[super image] retain];
	[lock unlock];
	return [image autorelease];
}

// a new image drops the thumbnail
- (void)setImage:(id)aImage
{
	NSImage *old = nil;
	NSLock *lock = [self lock];
	[lock lock];
	if ([super image] != aImage) {
		old = _cachedImage;
		_cachedImage = nil;
		[super setImage:aImage];
	}
	[lock unlock];
	[old release];
}

- (void)setNeedsUpdateAsItem:(NSInteger)aItem discardCacheImmediately:(BOOL)aImmediately
{
	NSLock *lock = [self lock];
	[lock lock];
	_isUpdateNeeded = 1;
	_dataNo = (int)aItem;
	NSImage *old = nil;
	if (aImmediately) {
		old = _cachedImage;
		_cachedImage = nil;
	}
	[lock unlock];
	[old release];
}

- (void)setNeedsUpdateAsItem:(NSInteger)aItem
{
	NSLock *lock = [self lock];
	[lock lock];
	_isUpdateNeeded = 1;
	_dataNo = (int)aItem;
	[lock unlock];
}

// fetches the item's image, title and VoiceOver type from the grid control
- (void)updateContentIfNeeded
{
	NSLock *lock = [self lock];
	[lock lock];
	BOOL needed = _isUpdateNeeded;
	_isUpdateNeeded = 0;
	[lock unlock];
	if (!needed) {
		return;
	}
	SMKGridControl *grid = [[self controlView] owningGridControl];
	NSImage *image = [grid imageForItemAt:_dataNo];
	NSString *title = [grid titleForItemAt:_dataNo];
	id itemType = [[grid additionalInfoForItemAt:_dataNo] objectForKey:SMKGridAttributeVoiceOverItemTypeKey];
	[self setImage:image];
	[self setTitle:title];
	[lock lock];
	if (itemType == nil || [itemType isKindOfClass:[NSString class]]) {
		[_itemType autorelease];
		_itemType = [itemType retain];
	}
	[lock unlock];
}

- (void)updateTitleIfNeeded
{
	NSLock *lock = [self lock];
	[lock lock];
	BOOL needed = _isUpdateNeeded;
	[lock unlock];
	if (needed) {
		[self setTitle:[[[self controlView] owningGridControl] titleForItemAt:_dataNo]];
	}
}

- (void)emptyTitleCache
{
	NSLock *lock = [self lock];
	[lock lock];
	NSString *old = _cachedFittingTextValue;
	_cachedFittingTextValue = nil;
	[lock unlock];
	[old release];
}

// a new content size invalidates the thumbnail and the fitted title
- (void)updateContentSize
{
	SMKGridMatrix *matrix = [self controlView];
	NSSize size = matrix != nil ? [matrix cellContentSize] : NSZeroSize;
	NSLock *lock = [self lock];
	[lock lock];
	NSString *old = nil;
	if (!NSEqualSizes(_contentSize, size) || NSEqualSizes(NSZeroSize, size)) {
		_isCachedImageValid = 0;
		_contentSize = size;
		old = _cachedFittingTextValue;
		_cachedFittingTextValue = nil;
	}
	[lock unlock];
	[old release];
}

// the bounds above the title and the border offsets around it
- (NSRect)graphicContentPlacementRectForBounds:(NSRect)aBounds
{
	SMKGridMatrix *matrix = [self controlView];
	float titleHeight;
	if (![matrix isCacheAllowed]) {
		titleHeight = (float)[self textPlacementRectForBounds:aBounds].size.height;
	}
	else {
		titleHeight = (float)[matrix cellTitleAdditionalHeight];
	}
	if (titleHeight > 0.0f) {
		float offset = (float)[[self controlView] cellBorderOffset].height;
		aBounds.size.height = (float)aBounds.size.height - (offset + offset + titleHeight);
	}
	return aBounds;
}

// in the thumbnail (with the border offsets) or the title, within aFrame
- (BOOL)hitTestPoint:(NSPoint)aPoint inFrame:(NSRect)aFrame view:(id)aView
{
	NSRect interior = [self interiorRectForFrame:aFrame inView:aView];
	NSRect graphic = [self graphicContentPlacementRectForBounds:interior];
	NSRect image = [self cachedImageRectForBounds:graphic];
	NSSize offset = [[self controlView] cellBorderOffset];
	if (!NSIsEmptyRect(image)) {
		image = NSInsetRect(image, -(float)offset.width, -(float)offset.height);
	}
	image = NSIntersectionRect(image, aFrame);
	if (!NSIsEmptyRect(image) && NSPointInRect(aPoint, image)) {
		return YES;
	}
	if ([[self title] length] != 0) {
		NSRect text = NSIntersectionRect([self textPlacementRectForBounds:interior], aFrame);
		if (!NSIsEmptyRect(text) && NSPointInRect(aPoint, text)) {
			return YES;
		}
	}
	return NO;
}

- (id)_cachedImage
{
	NSLock *lock = [self lock];
	[lock lock];
	NSImage *image = [_cachedImage retain];
	[lock unlock];
	return [image autorelease];
}

// NSCell copies the instance bitwise; the copy gets its own objects and locks
- (id)copyWithZone:(NSZone *)aZone
{
	NSLock *lock = [self lock];
	[lock lock];
	SMKGridBaseCell *copy = [super copyWithZone:aZone];
	copy->_contentSize = _contentSize;
	copy->_cachedImage = [_cachedImage retain];
	copy->_isCachedImageValid = _isCachedImageValid;
	copy->_textValue = [_textValue copy];
	copy->_cachedFittingTextValue = [_cachedFittingTextValue copy];
	copy->_cachedFittingTextBoundsSize = _cachedFittingTextBoundsSize;
	copy->_isUpdateNeeded = _isUpdateNeeded;
	copy->_itemType = [_itemType retain];
	copy->_lock = (NSLock *)[NSRecursiveLock new];
	copy->_stringRenderingLock = (NSLock *)[NSRecursiveLock new];
	[lock unlock];
	return copy;
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityRoleAttribute]) {
		return NSAccessibilityUnknownRole;
	}
	if ([aValue isEqualToString:NSAccessibilityRoleDescriptionAttribute]) {
		NSLock *lock = [self lock];
		[lock lock];
		NSString *itemType = _itemType;
		[lock unlock];
		return itemType;
	}
	if ([aValue isEqualToString:NSAccessibilityTitleAttribute] || [aValue isEqualToString:NSAccessibilityHelpAttribute]) {
		return [self title];
	}
	return [super accessibilityAttributeValue:aValue];
}

- (BOOL)accessibilityIsIgnored
{
	return NO;
}

- (id)accessibilityHitTest:(NSPoint)aTest
{
	return self;
}

- (id)accessibilityFocusedUIElement
{
	return self;
}

// The fitted title drawn into a flipped image of its size, over the
// matrix's (or its scroll view's) background.
- (id)cachedTextImageForBounds:(NSRect)aBounds
{
	if (NSIsEmptyRect(aBounds)) {
		return nil;
	}
	[self textPlacementRectForBounds:aBounds];
	SMKGridMatrix *matrix = [self controlView];
	NSLock *lock = [self lock];
	[lock lock];
	NSString *fitted = [[_cachedFittingTextValue copy] autorelease];
	NSSize size = _cachedFittingTextBoundsSize;
	[lock unlock];
	NSImage *image = [[NSImage alloc] initWithSize:size];
	[image setCacheMode:NSImageCacheNever];
	[image setFlipped:YES];
	[image lockFocus];
	NSColor *background = nil;
	if ([matrix respondsToSelector:@selector(drawsCellBackground)] && [matrix drawsCellBackground]) {
		background = [matrix cellBackgroundColor];
	}
	else if ([matrix drawsBackground]) {
		background = [matrix backgroundColor];
	}
	else if ([[matrix enclosingScrollView] drawsBackground]) {
		background = [[matrix enclosingScrollView] backgroundColor];
	}
	if (background != nil) {
		[background set];
		NSRectFill(NSMakeRect(0.0, 0.0, size.width, size.height));
	}
	SMKStringRenderer *renderer = [matrix stringRenderer];
	NSLock *renderingLock = [self stringRenderingLock];
	[renderingLock lock];
	[renderer setContainerWidth:size.width];
	[renderer drawString:fitted atPoint:NSZeroPoint withColor:nil];
	[renderingLock unlock];
	[image unlockFocus];
	return image;
}

// aSize scaled down (never up) to fit aBounds, whole points
- (NSSize)cachedImageSizeForSize:(NSSize)aSize bounds:(NSSize)aBounds
{
	if ((float)aSize.width <= 1.0f || (float)aSize.height <= 1.0f) {
		return NSZeroSize;
	}
	float scale = (float)aBounds.height / (float)aSize.height;
	if ((float)aBounds.width / (float)aSize.width <= scale) {
		scale = (float)aBounds.width / (float)aSize.width;
	}
	if (scale > 1.0f) {
		scale = 1.0f;
	}
	return NSMakeSize(truncf(scale * (float)aSize.width), truncf((float)aSize.height * scale));
}

- (NSSize)graphicContentSize
{
	NSImage *image = [self image];
	if (image == nil) {
		return NSZeroSize;
	}
	NSSize size = [image size];
	return NSMakeSize(truncf((float)size.width), truncf((float)size.height));
}

// the thumbnail's rectangle centered in aBounds: the cached one's size when
// caching is on and it is valid, else the image scaled to fit
- (NSRect)cachedImageRectForBounds:(NSRect)aBounds
{
	SMKGridMatrix *matrix = [self controlView];
	BOOL caching = [matrix isCacheAllowed] && [matrix isThumbnailCachingEnabled];
	NSSize size = NSZeroSize;
	NSLock *lock = [self lock];
	[lock lock];
	BOOL useCache = caching && _cachedImage != nil && _isCachedImageValid;
	if (useCache) {
		size = [_cachedImage size];
	}
	[lock unlock];
	if (!useCache) {
		size = [self cachedImageSizeForSize:[self graphicContentSize] bounds:aBounds.size];
	}
	NSRect r;
	r.origin.y = (float)aBounds.origin.y + (float)trunc((double)((float)aBounds.size.height - (float)size.height) * 0.5);
	r.origin.x = (float)aBounds.origin.x + (float)trunc((double)((float)aBounds.size.width - (float)size.width) * 0.5);
	r.size = size;
	return r;
}

// The title's rectangle at the bottom of the bounds, centered: its fitted
// size, worked out once (no taller than the matrix's title height so far)
// and kept with the fitted string. The matrix's title height follows the
// last fitted title.
- (NSRect)textPlacementRectForBounds:(NSRect)aBounds
{
	SMKGridMatrix *matrix = [self controlView];
	float additional = (float)[matrix cellTitleAdditionalHeight];
	NSLock *lock = [self lock];
	[lock lock];
	NSString *fitted = _cachedFittingTextValue;
	NSSize size = _cachedFittingTextBoundsSize;
	[lock unlock];
	if (fitted == nil) {
		NSString *title = [self title];
		NSRect bounding = NSZeroRect;
		NSString *newFitted = nil;
		if ([title length] != 0) {
			// (the original left an empty title the whole bounds here)
			bounding = aBounds;
			SMKStringRenderer *renderer = [matrix stringRenderer];
			NSLock *renderingLock = [self stringRenderingLock];
			if (additional > 0.0f && additional < (float)bounding.size.height) {
				bounding.size.height = additional;
			}
			[renderingLock lock];
			newFitted = [renderer stringByFittingString:title toSize:bounding.size boundingRect:&bounding];
			[renderingLock unlock];
			if (!NSIsEmptyRect(bounding)) {
				[matrix setCellTitleAdditionalHeight:bounding.size.height];
			}
		}
		[lock lock];
		if ([title isEqual:_textValue] && _cachedFittingTextValue == nil) {
			_cachedFittingTextBoundsSize = bounding.size;
			_cachedFittingTextValue = [newFitted retain];
		}
		[lock unlock];
		size = bounding.size;
	}
	NSRect r;
	r.origin.y = ((float)aBounds.origin.y + (float)aBounds.size.height) - (float)[matrix cellTitleAdditionalHeight];
	r.origin.x = ceilf(((float)aBounds.size.width - (float)size.width) * 0.5f + (float)aBounds.origin.x);
	r.size = size;
	return r;
}

@end
