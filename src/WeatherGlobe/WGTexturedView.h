#import "WeatherGlobePrefix.h"


@interface WGTexturedView : NSView
{
    int _operation;
    BOOL _isOperationValid;
    float _fraction;
    BOOL _isFractionValid;
    NSImage * _textures[9];
}
- (NSRect)correctedDestinationRectForTexture:(NSInteger)aTexture destinationRect:(NSRect)aRect;
- (NSRect)textureRect;
- (void)setImage:(id)aImage forTexture:(NSInteger)aTexture;
- (id)imageForTexture:(NSInteger)aTexture;
- (void)setCompositeOperation:(NSInteger)aOperation;
- (NSInteger)compositeOperation;
- (void)setFraction:(CGFloat)aFraction;
- (CGFloat)fraction;
- (void)drawRect:(NSRect)aRect;
- (void)dealloc;
@end

