#import "WeatherGlobePrefix.h"


@interface WGTexturedButton : NSButton
{
    float _lastWidth;
    BOOL _needRefreshIcon;
    BOOL _needRefreshAltIcon;
    NSImage * _images[3];
    NSImage * _altImages[3];
}
- (void)drawRect:(NSRect)aRect;
- (void)_refreshIcons;
- (id)_imageForType:(NSInteger)aType;
- (void)setAlternateImage:(id)aImage forPosition:(NSInteger)aPosition;
- (id)alternateImageForPosition:(NSInteger)aPosition;
- (void)setImage:(id)aImage forPosition:(NSInteger)aPosition;
- (id)imageForPosition:(NSInteger)aPosition;
- (void)dealloc;
@end

