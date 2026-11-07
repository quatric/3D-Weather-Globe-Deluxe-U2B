// AtlasRoute.qlgenerator: Quick Look previews and thumbnails of route set
// files, the route drawn over the route manager's earth preview images.
#import <Cocoa/Cocoa.h>

// A point of the drawn route, in the unit square of the flat map.
@interface RoutePointData : NSObject
{
	NSPoint _pt;
	BOOL _isHidden;	// no dot (the points added where the route crosses the date line)
	BOOL _isBreakHere;	// starts a new segment
}
- (BOOL)isBreakHere;
- (void)setIsBreakHere:(BOOL)aBreak;
- (BOOL)isHidden;
- (void)setIsHidden:(BOOL)aHidden;
- (NSPoint)point;
- (void)setPoint:(NSPoint)aPoint;
@end

// A map tile and where it goes: srcRect in its pixels, dstRect in the
// unit square of the drawn rectangle.
@interface TextureData : NSObject
{
	NSImage *_image;
	NSRect _srcRect;
	NSRect _dstRect;
}
@property (retain) NSImage *image;
@property NSRect srcRect;
@property NSRect dstRect;
@end

@interface RoutePreviewObject : NSObject
{
	NSURL *_url;
	float _scale;	// the shown part of the map, 0.1 .. 1
	NSPoint _position;	// its center
	NSMutableArray *_routePoints;
	NSMutableArray *_texturesToDraw;
	NSString *_imagesRootPathCache;
}
- (id)initWithURL:(NSURL *)aURL;
- (void)setScale:(float)aScale;
- (void)setPosition:(NSPoint)aPosition;
- (void)scaleToFit;
- (void)drawInRect:(NSRect)aRect;
@end

#ifdef __cplusplus
extern "C"
#endif
void drawRouteInContextWithSize(CFURLRef url, CGContextRef context, CGSize size);
