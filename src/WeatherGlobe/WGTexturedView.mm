#import "WGTexturedView.h"

enum {
	kTextureTopLeft, kTextureTop, kTextureTopRight,
	kTextureLeft, kTextureCenter, kTextureRight,
	kTextureBottomLeft, kTextureBottom, kTextureBottomRight,
	kTexturesCount
};

// Draws nine images stretched over the texture rect: fixed corners, edges
// stretched along, the center filling the rest.
@implementation WGTexturedView

- (void)dealloc
{
	for (int i = 0; i < kTexturesCount; i++)
		[_textures[i] release];
	[super dealloc];
}

- (CGFloat)fraction
{
	return _fraction;
}

- (void)setFraction:(CGFloat)aFraction
{
	if (aFraction != _fraction) {
		_fraction = aFraction;
		_isFractionValid = YES;
		[self setNeedsDisplay:YES];
	}
}

- (NSInteger)compositeOperation
{
	return _operation;
}

- (void)setCompositeOperation:(NSInteger)aOperation
{
	if (_operation != aOperation) {
		_operation = (int)aOperation;
		_isOperationValid = YES;
		[self setNeedsDisplay:YES];
	}
}

- (id)imageForTexture:(NSInteger)aTexture
{
	return (NSUInteger)aTexture < kTexturesCount ? _textures[aTexture] : nil;
}

- (void)setImage:(id)aImage forTexture:(NSInteger)aTexture
{
	if ((NSUInteger)aTexture < kTexturesCount && _textures[aTexture] != aImage) {
		[_textures[aTexture] release];
		_textures[aTexture] = [aImage retain];
		[self setNeedsDisplay:YES];
	}
}

- (NSRect)textureRect
{
	return [self bounds];
}

// Subclasses may move the edges.
- (NSRect)correctedDestinationRectForTexture:(NSInteger)aTexture destinationRect:(NSRect)aRect
{
	return aRect;
}

- (void)drawRect:(NSRect)aRect
{
	NSRect src[kTexturesCount], dst[kTexturesCount];
	memset(src, 0, sizeof(src));
	memset(dst, 0, sizeof(dst));
	NSRect rect = [self textureRect];
	CGFloat x = rect.origin.x, y = rect.origin.y, w = rect.size.width, h = rect.size.height;
	NSSize size;

	if (_textures[kTextureTopLeft]) {
		size = [_textures[kTextureTopLeft] size];
		src[kTextureTopLeft].size = size;
		dst[kTextureTopLeft] = NSMakeRect(x, y + h - size.height, size.width, size.height);
	}
	if (_textures[kTextureTopRight]) {
		size = [_textures[kTextureTopRight] size];
		src[kTextureTopRight].size = size;
		dst[kTextureTopRight] = NSMakeRect(x + w - size.width, y + h - size.height, size.width, size.height);
	}
	if (_textures[kTextureBottomLeft]) {
		size = [_textures[kTextureBottomLeft] size];
		src[kTextureBottomLeft].size = size;
		dst[kTextureBottomLeft] = NSMakeRect(x, y, size.width, size.height);
	}
	if (_textures[kTextureBottomRight]) {
		size = [_textures[kTextureBottomRight] size];
		src[kTextureBottomRight].size = size;
		dst[kTextureBottomRight] = NSMakeRect(x + w - size.width, y, size.width, size.height);
	}
	if (_textures[kTextureTop]) {
		size = [_textures[kTextureTop] size];
		src[kTextureTop].size = size;
		NSRect r = NSMakeRect(NSMaxX(dst[kTextureTopLeft]), y + h - size.height,
			w - dst[kTextureTopLeft].size.width - dst[kTextureTopRight].size.width, size.height);
		dst[kTextureTop] = [self correctedDestinationRectForTexture:kTextureTop destinationRect:r];
	}
	if (_textures[kTextureBottom]) {
		size = [_textures[kTextureBottom] size];
		src[kTextureBottom].size = size;
		NSRect r = NSMakeRect(NSMaxX(dst[kTextureBottomLeft]), y,
			w - dst[kTextureBottomLeft].size.width - dst[kTextureBottomRight].size.width, size.height);
		dst[kTextureBottom] = [self correctedDestinationRectForTexture:kTextureBottom destinationRect:r];
	}
	if (_textures[kTextureLeft]) {
		size = [_textures[kTextureLeft] size];
		src[kTextureLeft].size = size;
		NSRect r = NSMakeRect(x, NSMaxY(dst[kTextureBottomLeft]), size.width,
			h - dst[kTextureTopLeft].size.height - dst[kTextureBottomLeft].size.height);
		dst[kTextureLeft] = [self correctedDestinationRectForTexture:kTextureLeft destinationRect:r];
	}
	if (_textures[kTextureRight]) {
		size = [_textures[kTextureRight] size];
		src[kTextureRight].size = size;
		NSRect r = NSMakeRect(x + w - size.width, NSMaxY(dst[kTextureBottomRight]), size.width,
			h - dst[kTextureTopRight].size.height - dst[kTextureBottomRight].size.height);
		dst[kTextureRight] = [self correctedDestinationRectForTexture:kTextureRight destinationRect:r];
	}
	if (_textures[kTextureCenter]) {
		size = [_textures[kTextureCenter] size];
		src[kTextureCenter].size = size;
		dst[kTextureCenter] = NSMakeRect(NSMaxX(dst[kTextureLeft]), NSMaxY(dst[kTextureBottom]),
			w - dst[kTextureLeft].size.width - dst[kTextureRight].size.width,
			h - dst[kTextureTop].size.height - dst[kTextureBottom].size.height);
	}

	if (!_isOperationValid) {
		_operation = NSCompositingOperationSourceOver;
		_isOperationValid = YES;
	}
	if (!_isFractionValid) {
		_fraction = 1.0f;
		_isFractionValid = YES;
	}
	for (int i = 0; i < kTexturesCount; i++)
		[_textures[i] drawInRect:dst[i] fromRect:src[i] operation:(NSCompositingOperation)_operation
			fraction:_fraction];
}

@end
