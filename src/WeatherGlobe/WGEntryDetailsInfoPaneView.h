#import "WeatherGlobePrefix.h"


@interface WGEntryDetailsInfoPaneView : NSView
{
    id delegate;
}
- (void)awakeFromNib;
- (void)dealloc;
- (BOOL)isFlipped;
@end

