#import "WeatherGlobePrefix.h"


@interface WGImageTextCell : SMKEngravedTextFieldCell
{
    NSImage *_image;
    int _imageScaling;
}
- (void)setImageScaling:(NSInteger)aScaling;
- (void)setImage:(id)aImage;
- (NSInteger)imageScaling;
- (id)image;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)dealloc;
@end

