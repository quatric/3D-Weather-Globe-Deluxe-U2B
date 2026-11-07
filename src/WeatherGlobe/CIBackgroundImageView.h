#import "WeatherGlobePrefix.h"
#import "BackgroundImageView.h"


@interface CIBackgroundImageView : BackgroundImageView
{
    int _style;
}
- (BOOL)acceptsFirstResponder;
- (void)setImageViewStyle:(NSInteger)aStyle;
- (void)drawRect:(NSRect)aRect;
@end

