#import "WeatherGlobePrefix.h"


@interface WGImageAndTextCell : NSTextFieldCell
{
    NSImage *_leftImage;
}
- (NSSize)cellSize;
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView;
- (id)image;
- (void)setImage:(id)aImage;
- (void)selectWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate start:(NSInteger)aStart length:(NSInteger)aLength;
- (void)editWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate event:(id)aEvent;
- (id)copyWithZone:(NSZone *)aZone;
- (void)dealloc;
@end

