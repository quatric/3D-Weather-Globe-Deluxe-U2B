#import "WGEarthView.h"
#import "NSImage+NSImageWGExtensions.h"

// The route preview map: the RoutePreviewController's tiles and the route
// (points in 0..1 of the view) drawn over them; drag to move, scroll or the
// zoom buttons to zoom.
@implementation WGEarthView

- (void)dealloc
{
	LNotificationCenter::center()->removeObserver(self);
	if (_routePreviewController) {
		_routePreviewController->release();
		_routePreviewController = 0;
	}
	[super dealloc];
}

- (void)awakeFromNib
{
	[_zoomOut setEnabled:NO];
	LNotificationCenter::center()->addObserver(self, @selector(_needsDisplay:),
		RoutePreviewController::kRoutesPreviewNeedsDisplay, _routePreviewController);
}

// Snaps values within 1e-5 of a whole number to it.
- (CGFloat)_fixFloat:(CGFloat)aFloat
{
	float value = aFloat;
	float ceiling = ceilf(value);
	if (fabsf(ceiling - value) <= 1e-05f)
		value = ceiling;
	float floor = floorf(value);
	if (fabsf(floor - value) <= 1e-05f)
		value = floor;
	return value;
}

- (void)drawRect:(NSRect)aRect
{
	[self _drawImages];
	[self _drawRoute];
}

- (void)setSelectedRoute:(GenericRouteNode *)aRoute
{
	_routePreviewController->setSelectedRoute(aRoute);
}

- (BOOL)canZoomIn
{
	return !_routePreviewController->isMinScaled();
}

- (BOOL)canZoomOut
{
	return !_routePreviewController->isMaxScaled();
}

- (void)zoomIn:(id)aIn
{
	_routePreviewController->setScale(_routePreviewController->_scale - 0.01f);
}

- (void)scaleToFit:(id)aFit
{
	_routePreviewController->scaleToFit();
}

- (void)mouseDown:(id)aDown
{
}

- (void)_needsDisplay:(id)aDisplay
{
	[self setNeedsDisplay:YES];
	[_zoomIn setEnabled:[self canZoomIn]];
	[_zoomOut setEnabled:[self canZoomOut]];
}

- (void)scrollWheel:(id)aWheel
{
	_routePreviewController->scrollWheelWithDiff(((float)[aWheel deltaY] / 10.0f) * _routePreviewController->_scale);
}

// The drag in view fractions, scaled.
- (void)mouseDragged:(id)aDragged
{
	float scale = _routePreviewController->_scale;
	float dx = (float)[aDragged deltaX] / [self bounds].size.width * scale;
	float dy = (float)[aDragged deltaY] / [self bounds].size.height * scale;
	LPoint diff = {-dx, dy};
	_routePreviewController->mouseDraggedWithDiff(diff);
}

- (void)zoomOut:(id)aOut
{
	_routePreviewController->setScale(_routePreviewController->_scale + 0.01f);
}

// A dark wide line under a thin white one; visible points get white dots.
- (void)_drawRoute
{
	NSRect bounds = [self bounds];
	TArray<RPRoutePointData> *points = _routePreviewController->getRoutePointData();
	if (!points || !points->count())
		return;
	NSBezierPath *path = [NSBezierPath bezierPath];
	NSMutableArray *dots = [NSMutableArray array];
	int count = points->count();
	for (int i = 0; i < count; i++) {
		RPRoutePointData &point = (*points)[i];
		NSPoint location = NSMakePoint(bounds.size.width * point.point.x, bounds.size.height * point.point.y);
		// (The original read the point before the first one.)
		if (point.move || i == 0 || (*points)[i - 1].move)
			[path moveToPoint:location];
		else
			[path lineToPoint:location];
		if (!point.hidden) {
			NSBezierPath *dot = [NSBezierPath bezierPath];
			[dot appendBezierPathWithOvalInRect:NSMakeRect(location.x - 2.5, location.y - 2.5, 5.0, 5.0)];
			[dots addObject:dot];
		}
	}
	[path setLineWidth:3.7];
	[[NSColor colorWithCalibratedWhite:0.0 alpha:0.65] set];
	[path stroke];
	[[NSColor whiteColor] set];
	[path setLineWidth:1.5];
	[path stroke];
	for (NSUInteger i = 0; i < [dots count]; i++) {
		NSBezierPath *dot = [dots objectAtIndex:i];
		[[NSColor whiteColor] set];
		[dot fill];
		[[NSColor blackColor] set];
		[dot stroke];
	}
}

// The tiles fill the grid of column and row edges (fractions of the view),
// row by row, snapped to whole pixels.
- (void)_drawImages
{
	RPDrawingData *data = _routePreviewController->getDrawingData();
	NSRect bounds = [self bounds];
	int columns = data->_columns->count() - 1;
	int textures = data->_textures->count();
	for (int i = 0; i < textures; i++) {
		RPTextureData &texture = (*data->_textures)[i];
		float *columnEdges = data->_columns->items();
		float *rowEdges = data->_rows->items();
		int column = i % columns, row = i / columns;
		float left = bounds.size.width * columnEdges[column];
		float right = [self _fixFloat:(columnEdges[column + 1] - columnEdges[column]) * bounds.size.width + left];
		float bottom = bounds.size.height * rowEdges[row];
		float top = [self _fixFloat:bottom + (rowEdges[row + 1] - rowEdges[row]) * bounds.size.height];
		left = floorf(left);
		bottom = floorf(bottom);
		NSImage *image = [NSImage wgImageWithLImage:_routePreviewController->imageForTextureID(texture.textureID)];
		NSRect source = NSMakeRect(texture.srcRect.origin.x, texture.srcRect.origin.y,
			texture.srcRect.size.width, texture.srcRect.size.height);
		[image drawInRect:NSMakeRect(left, bottom, floorf(right) - left, floorf(top) - bottom) fromRect:source
			operation:NSCompositingOperationSourceOver fraction:1.0];
	}
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self)
		_routePreviewController = RoutePreviewController::_alloc()->init();
	return self;
}

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self)
		_routePreviewController = RoutePreviewController::_alloc()->init();
	return self;
}

@end
