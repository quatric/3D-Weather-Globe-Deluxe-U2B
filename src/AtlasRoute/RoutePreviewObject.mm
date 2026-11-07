// RoutePreviewObject: draws a route set's route over the tiles of the
// application's flat earth preview, zoomed to fit the route. The original
// was garbage collected; this is manual retain/release.
#import "AtlasRoute.h"

// The map tiles are 256 x 128 pixels; each level of detail doubles their
// number per side.
static const float kTileWidth = 256.0f;
static const float kTileHeight = 128.0f;
// The margin around the route when zooming to it.
static const float kFitMargin = 0.023f;

@implementation RoutePreviewObject

// 0 above two thirds of the map, 1 above one third, else 2.
- (char)_lod
{
	if (_scale <= 1.0f / 3.0f)
		return 2;
	return _scale <= 2.0f / 3.0f ? 1 : 0;
}

// Longitude and latitude to the unit square.
- (NSPoint)_renderPointFromGeoPoint:(NSPoint)aPoint
{
	return NSMakePoint((float)((aPoint.x + 180.0f) / 360.0f), (float)((aPoint.y + 90.0f) / 180.0f));
}

- (void)setScale:(float)aScale
{
	float scale = aScale < 0.1f ? 0.1f : MIN(1.0f, aScale);
	if (scale == _scale)
		return;
	_scale = scale;
	[self setPosition:_position];
	[_texturesToDraw release];
	_texturesToDraw = nil;
}

- (NSString *)_imagePathForTextureLevel:(char)aLevel i:(short)i j:(short)j
{
	return [[self _imagesRootPath] stringByAppendingPathComponent:
	    [NSString stringWithFormat:@"LOD%d/Earth_map_%d_%d.png", (int)aLevel, (int)j, (int)i]];
}

// The application's images when the generator is inside it, else the
// bundle's own.
- (NSString *)_imagesRootPath
{
	if (!_imagesRootPathCache) {
		NSBundle *bundle = [NSBundle bundleForClass:[self class]];
		NSString *bundlePath = [bundle bundlePath];
		NSString *path;
		if ([[bundlePath pathExtension] isEqualToString:@"qlgenerator"])
			path = [[[[bundlePath stringByDeletingLastPathComponent] stringByDeletingLastPathComponent]
			    stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"Resources/BASE/INTERFACE/RouteManagerPalette/Earth preview"];
		else
			path = [[bundle resourcePath] stringByAppendingPathComponent:@"Earth preview"];
		_imagesRootPathCache = [path retain];
	}
	return _imagesRootPathCache;
}

// The tiles per side.
- (int)_numImages
{
	switch ([self _lod]) {
	case 0:
		return 2;
	case 1:
		return 4;
	}
	return 8;
}

- (id)initWithURL:(NSURL *)aURL
{
	self = [super init];
	if (self) {
		_url = [aURL retain];
		[self scaleToFit];
	}
	return self;
}

- (void)dealloc
{
	[_url release];
	[_routePoints release];
	[_texturesToDraw release];
	[_imagesRootPathCache release];
	[super dealloc];
}

// The shown part stays inside the map.
- (void)setPosition:(NSPoint)aPosition
{
	float half = _scale * 0.5f;
	float x = MIN(1.0f - half, MAX(half, (float)aPosition.x));
	float y = MIN(1.0f - half, MAX(half, (float)aPosition.y));
	if (NSEqualPoints(NSMakePoint(x, y), _position))
		return;
	_position = NSMakePoint(x, y);
	[_texturesToDraw release];
	_texturesToDraw = nil;
}

// The route's points; where a leg crosses the date line, it ends at one
// edge of the map and goes on from the other (two hidden points).
- (void)_validateRoutePoints
{
	if (_routePoints)
		return;
	_routePoints = [[NSMutableArray alloc] init];
	NSDictionary *routeSet = [NSDictionary dictionaryWithContentsOfURL:_url];
	NSArray *entries = [routeSet objectForKey:@"entries"];
	if (![entries isKindOfClass:[NSArray class]])
		return;
	NSUInteger count = [entries count];
	if (count == 0)
		return;
	NSMutableData *breaksData = [NSMutableData dataWithLength:count];
	char *breaks = (char *)[breaksData mutableBytes];
	breaks[0] = 1;
	NSArray *breakIndexes = [routeSet objectForKey:@"breaks"];
	NSUInteger breaksCount = [breakIndexes isKindOfClass:[NSArray class]] ? [breakIndexes count] : 0;
	for (NSUInteger i = 0; i < breaksCount; i++) {
		id index = [breakIndexes objectAtIndex:i];
		if ([index respondsToSelector:@selector(intValue)]) {
			int value = [index intValue];
			if (value >= 0 && (NSUInteger)value < count)
				breaks[value] = 1;
		}
	}
	NSPoint previous = NSMakePoint(0.5, 0.5);
	for (NSUInteger i = 0; i < count; i++) {
		NSDictionary *entry = [entries objectAtIndex:i];
		if (![entry isKindOfClass:[NSDictionary class]])
			break;
		id longitude = [entry objectForKey:@"Longitude"];
		if (![longitude respondsToSelector:@selector(floatValue)])
			break;
		id latitude = [entry objectForKey:@"Lattitude"];
		if (![latitude respondsToSelector:@selector(floatValue)])
			break;
		RoutePointData *point = [[RoutePointData new] autorelease];
		float lat = [latitude floatValue];
		NSPoint current = [self _renderPointFromGeoPoint:NSMakePoint([longitude floatValue], lat)];
		[point setPoint:current];
		if (breaks[i]) {
			[point setIsBreakHere:YES];
			previous = NSMakePoint(0.5, 0.5);
		}
		if (fabsf((float)(previous.x - current.x)) > 0.5f) {
			float low = MIN(previous.x, current.x);
			float high = MAX(previous.x, current.x);
			float crossingY = (1.0f - high) / (low + 1.0f - high) * fabsf((float)(previous.y - current.y))
			    + (float)MIN(previous.y, current.y);
			BOOL eastward = previous.x < current.x;
			RoutePointData *edge = [[RoutePointData new] autorelease];
			[edge setPoint:NSMakePoint(eastward ? 0.0 : 1.0, crossingY)];
			[edge setIsHidden:YES];
			[_routePoints addObject:edge];
			edge = [[RoutePointData new] autorelease];
			[edge setPoint:NSMakePoint(eastward ? 1.0 : 0.0, crossingY)];
			[edge setIsBreakHere:YES];
			[edge setIsHidden:YES];
			[_routePoints addObject:edge];
		}
		[_routePoints addObject:point];
		previous = current;
	}
}

// The shown part of the map.
- (NSRect)_renderRect
{
	return NSMakeRect(_position.x - _scale * 0.5f, _position.y - _scale * 0.5f, _scale, _scale);
}

// The tiles of the current level over the shown part.
- (void)_validateTextures
{
	if (_texturesToDraw)
		return;
	int numImages = [self _numImages];
	NSRect renderRect = [self _renderRect];
	float n = (float)numImages;
	int firstI = (int)floorf(numImages * renderRect.origin.x);
	int lastI = (int)ceilf((renderRect.origin.x + renderRect.size.width) * numImages);
	int firstJ = (int)floorf(numImages * renderRect.origin.y);
	int lastJ = (int)ceilf((renderRect.origin.y + renderRect.size.height) * numImages);
	char lod = [self _lod];
	for (int i = firstI; i < lastI; i++) {
		for (int j = firstJ; j < lastJ; j++) {
			NSImage *image = [[[NSImage alloc] initWithContentsOfFile:
			    [self _imagePathForTextureLevel:lod i:(short)i j:(short)j]] autorelease];
			if (!image)
				continue;
			NSRect tile = NSMakeRect(i / n, j / n, 1.0f / n, 1.0f / n);
			NSRect part = NSIntersectionRect(renderRect, tile);
			if (NSIsEmptyRect(part))
				continue;
			TextureData *texture = [[TextureData new] autorelease];
			[texture setImage:image];
			[texture setSrcRect:NSMakeRect(floorf(n * kTileWidth * part.origin.x - kTileWidth * i),
			                               floorf(n * kTileHeight * part.origin.y - kTileHeight * j),
			                               ceilf(n * kTileWidth * part.size.width),
			                               ceilf(n * kTileHeight * part.size.height))];
			[texture setDstRect:NSMakeRect((part.origin.x - renderRect.origin.x) / renderRect.size.width,
			                               (part.origin.y - renderRect.origin.y) / renderRect.size.height,
			                               part.size.width / renderRect.size.width,
			                               part.size.height / renderRect.size.height)];
			if (!_texturesToDraw)
				_texturesToDraw = [[NSMutableArray alloc] init];
			[_texturesToDraw addObject:texture];
		}
	}
}

// Zooms to the route's bounds (with a margin), or shows the whole map.
- (void)scaleToFit
{
	[self _validateRoutePoints];
	float minX = 1.0f, minY = 1.0f;
	float maxX = NSZeroPoint.x, maxY = NSZeroPoint.y;
	for (RoutePointData *point in _routePoints) {
		NSPoint p = [point point];
		minX = MIN(minX, (float)p.x);
		minY = MIN(minY, (float)p.y);
		maxX = MAX(maxX, (float)p.x);
		maxY = MAX(maxY, (float)p.y);
	}
	float width = maxX - minX;
	float height = maxY - minY;
	float scale;
	NSPoint center;
	if (!NSIsEmptyRect(NSMakeRect(minX, minY, width, height))) {
		scale = MAX(width, height) + kFitMargin;
		center = NSMakePoint(minX + width * 0.5f, minY + height * 0.5f);
	} else {
		scale = 1.0f;
		center = NSMakePoint(0.5, 0.5);
	}
	[self setScale:scale];
	[self setPosition:center];
}

// The map keeps its 2:1 proportion, centered in aRect.
- (void)drawInRect:(NSRect)aRect
{
	NSRect rect = aRect;
	float ratio = (float)(aRect.size.width / aRect.size.height);
	if (ratio > 2.0f) {
		rect.origin.x += floorf((aRect.size.width - aRect.size.height * 2.0) * 0.5);
		rect.size.width = ceilf(aRect.size.height * 2.0);
	} else if (ratio < 2.0f) {
		rect.origin.y += floorf((aRect.size.height - aRect.size.width * 0.5) * 0.5);
		rect.size.height = ceilf(aRect.size.width * 0.5);
	}

	[self _validateTextures];
	for (TextureData *texture in _texturesToDraw) {
		NSRect dst = [texture dstRect];
		NSRect drawRect = NSMakeRect(floorf(rect.size.width * dst.origin.x + rect.origin.x),
		                             floorf(rect.size.height * dst.origin.y + rect.origin.y),
		                             ceilf(rect.size.width * dst.size.width), ceilf(rect.size.height * dst.size.height));
		[[texture image] drawInRect:drawRect
		                   fromRect:[texture srcRect]
		                  operation:NSCompositingOperationSourceOver
		                   fraction:1.0];
	}

	[self _validateRoutePoints];
	NSRect renderRect = [self _renderRect];
	NSBezierPath *route = [NSBezierPath bezierPath];
	NSMutableArray *dots = [NSMutableArray array];
	for (RoutePointData *point in _routePoints) {
		NSPoint p = [point point];
		NSPoint viewPoint = NSMakePoint((p.x - renderRect.origin.x) / _scale * rect.size.width + rect.origin.x,
		                                (p.y - renderRect.origin.y) / _scale * rect.size.height + rect.origin.y);
		if ([point isBreakHere])
			[route moveToPoint:viewPoint];
		else
			[route lineToPoint:viewPoint];
		if (![point isHidden]) {
			NSBezierPath *dot = [NSBezierPath bezierPath];
			[dot appendBezierPathWithOvalInRect:NSMakeRect(viewPoint.x - 2.5, viewPoint.y - 2.5, 5.0, 5.0)];
			[dots addObject:dot];
		}
	}
	[[NSColor colorWithCalibratedWhite:0.0 alpha:0.65] set];
	[route setLineWidth:3.7];
	[route stroke];
	[[NSColor whiteColor] set];
	[route setLineWidth:1.5];
	[route stroke];
	for (NSBezierPath *dot in dots) {
		[[NSColor whiteColor] set];
		[dot fill];
		[[NSColor blackColor] set];
		[dot stroke];
	}
}

@end

void drawRouteInContextWithSize(CFURLRef url, CGContextRef context, CGSize size)
{
	if (!url || !context)
		return;
	NSGraphicsContext *graphicsContext = [NSGraphicsContext graphicsContextWithCGContext:context flipped:YES];
	if (!graphicsContext)
		return;
	NSGraphicsContext *savedContext = [NSGraphicsContext currentContext];
	[NSGraphicsContext setCurrentContext:graphicsContext];
	RoutePreviewObject *preview = [[RoutePreviewObject alloc] initWithURL:(NSURL *)url];
	[preview drawInRect:NSMakeRect(0, 0, size.width, size.height)];
	[preview release];
	[NSGraphicsContext setCurrentContext:savedContext];
}
