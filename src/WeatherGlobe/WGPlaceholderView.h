#import "WeatherGlobePrefix.h"


@interface WGPlaceholderView : NSView
{
    NSAttributedString *_string;
    NSColor *_backgroundColor;
}
- (void)setWGBackgroundColor:(id)aColor;
- (id)wgBackgroundColor;
- (void)setWGPlaceholderString:(id)aString;
- (id)wgPlaceholderString;
- (void)drawRect:(NSRect)aRect;
- (void)awakeFromNib;
- (void)dealloc;
- (id)initWithFrame:(NSRect)aFrame;
- (BOOL)isFlipped;
@end

