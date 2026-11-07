#import "WeatherGlobePrefix.h"


@interface AboutController : NSObject
{
    NSPanel *_panel;
    NSTabView *_tabView;
    NSScrollView *_creditsView;
}
- (void)showPanel;
- (void)dealloc;
- (void)awakeFromNib;
@end

