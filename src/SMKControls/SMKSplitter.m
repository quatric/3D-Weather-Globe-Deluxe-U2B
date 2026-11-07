#import "SMKSplitter.h"

static NSImage *_sDefaultSplitterImage;

// A grip that drags the divider of the split view it is in.
@implementation SMKSplitter

- (void)encodeWithCoder:(id)aCoder
{
	[super encodeWithCoder:aCoder];
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		[aCoder encodeObject:_splitterImage forKey:@"smk_splitterImage"];
		[aCoder encodeBool:[self isOpaque] forKey:@"smk_splitterOpaque"];
		return;
	}
	int version = 0;
	[aCoder encodeValueOfObjCType:"i" at:&version];
	[aCoder encodeObject:_splitterImage];
	char opaque = [self isOpaque];
	[aCoder encodeValueOfObjCType:"c" at:&opaque];
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self == nil) {
		return nil;
	}
	char opaque = YES;
	if ([aCoder respondsToSelector:@selector(allowsKeyedCoding)] && [aCoder allowsKeyedCoding]) {
		_splitterImage = [[aCoder decodeObjectForKey:@"smk_splitterImage"] retain];
		opaque = [aCoder decodeBoolForKey:@"smk_splitterOpaque"];
	}
	else {
		int version;
		[aCoder decodeValueOfObjCType:"i" at:&version];
		_splitterImage = [[aCoder decodeObject] retain];
		[aCoder decodeValueOfObjCType:"c" at:&opaque];
	}
	[self setOpaque:opaque];
	return self;
}

- (id)_trackableEnclosingSplitView
{
	id splitView = [self enclosingSplitView];
	return [splitView respondsToSelector:@selector(trackSMKSplitterView:withEvent:)] ? splitView : nil;
}

- (id)enclosingSplitView
{
	NSView *view = [self superview];
	NSSplitView *found = nil;
	while (view != nil && found == nil) {
		if ([view isKindOfClass:[NSSplitView class]]) {
			found = (NSSplitView *)view;
		}
		view = [view superview];
	}
	return found;
}

- (void)mouseDown:(id)aDown
{
	[[self _trackableEnclosingSplitView] trackSMKSplitterView:self withEvent:aDown];
}

- (void)sizeToFit
{
	NSImage *image = [self _splitterImage];
	NSSize size = image != nil ? [image size] : NSZeroSize;
	if (!NSEqualSizes([self frame].size, size)) {
		[self setFrameSize:size];
	}
}

- (void)drawRect:(NSRect)aRect
{
	NSImage *image = [self _splitterImage];
	if (image == nil) {
		return;
	}
	NSRect to = [self bounds];
	NSSize size = [image size];
	if (size.width <= to.size.width) {
		to.size.width = size.width;
	}
	if (size.height <= to.size.height) {
		to.size.height = size.height;
	}
	NSRect from = NSZeroRect;
	from.size = to.size;
	[image drawInRect:to fromRect:from operation:NSCompositingOperationSourceOver fraction:1.0];
}

- (void)setOpaque:(BOOL)aOpaque
{
	if ([self isOpaque] != aOpaque) {
		_smkcIsNotOpaque = !aOpaque;
		[self setNeedsDisplay:YES];
	}
}

- (BOOL)isOpaque
{
	return !_smkcIsNotOpaque;
}

// the set image, else the framework's, else an empty one
- (id)_splitterImage
{
	if (_splitterImage != nil) {
		return _splitterImage;
	}
	if (_sDefaultSplitterImage == nil) {
		NSString *path = [[NSBundle bundleForClass:[SMKSplitter class]] pathForResource:@"SMKSplitterImage" ofType:@"tiff"];
		if (path != nil) {
			_sDefaultSplitterImage = [[NSImage alloc] initWithContentsOfFile:path];
		}
		if (_sDefaultSplitterImage == nil) {
			_sDefaultSplitterImage = [[NSImage alloc] initWithSize:NSMakeSize(1.0, 1.0)];
		}
	}
	return _sDefaultSplitterImage;
}

- (void)setSplitterImage:(id)aImage
{
	if (_splitterImage != aImage) {
		[_splitterImage release];
		_splitterImage = [aImage retain];
		[self setNeedsDisplay:YES];
	}
}

- (id)splitterImage
{
	return _splitterImage;
}

@end
