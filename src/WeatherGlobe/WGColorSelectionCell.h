#import "WeatherGlobePrefix.h"


@interface WGColorSelectionCell : NSActionCell
{
}
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)setMaskImage:(id)aImage;
- (void)setColor:(id)aColor;
- (id)maskImage;
- (id)color;
@end

