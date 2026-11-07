#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface SFOneDayController : WGNibOwnerController
{
    NSView *_dataView;
    NSTextField *_weekday;
    NSImageView *_summary;
    NSTextFieldCell *_temperatureHigh;
    NSTextFieldCell *_temperatureLow;
    NSBox *_topBorder;
    NSBox *_bottomBorder;
    NSBox *_rightBorder;
}
- (void)setDrawsRightBorder:(BOOL)aBorder;
- (void)setDrawsTopAndBottomBorders:(BOOL)aBorders;
- (void)setValues:(LMap *)aValues forDate:(id)aDate;
- (id)dataView;
- (void)awakeFromNib;
- (id)nibName;
- (void)dealloc;
@end

