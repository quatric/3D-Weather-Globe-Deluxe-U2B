#import "WeatherGlobePrefix.h"
#import "WGTexturedView.h"


@interface WGBalloonTexturedVew : WGTexturedView
{
    NSImage *_balloonImage;
    int _balloonPosition;
    BOOL _isBalloonVisible;
}
- (NSPoint)balloonPositionForProposedPosition:(NSPoint)aPosition;
- (NSRect)correctedDestinationRectForTexture:(NSInteger)aTexture destinationRect:(NSRect)aRect;
- (NSRect)textureRect;
- (void)drawRect:(NSRect)aRect;
- (void)setIsBalloonVisible:(BOOL)aVisible;
- (BOOL)isBalloonVisible;
- (void)setBalloonPosition:(NSInteger)aPosition;
- (NSInteger)balloonPosition;
- (void)setBalloonImage:(id)aImage;
- (id)balloonImage;
- (void)dealloc;
- (id)initWithFrame:(NSRect)aFrame;
@end

@interface WGBalloonTexturedVew (Private)
- (NSRect)_balloonFrame;
@end

