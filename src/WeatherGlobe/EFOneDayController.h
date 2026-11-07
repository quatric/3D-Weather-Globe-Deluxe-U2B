#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface EFOneDayController : WGNibOwnerController
{
    NSView *_dataView;
    NSTextField *_weekday;
    NSImageView *_summary;
    NSImageView *_windDirection;
    NSTextFieldCell *_temperatureHigh;
    NSTextFieldCell *_temperatureLow;
    NSTextFieldCell *_feelsLike;
    NSTextFieldCell *_windSpeed;
    NSTextFieldCell *_humidity;
    NSTextFieldCell *_preciption;
    NSTextFieldCell *_dewPoint;
    NSTextFieldCell *_uvIndex;
}
- (void)setValues:(LMap *)aValues forDate:(id)aDate;
- (id)dataView;
- (id)nibName;
- (void)dealloc;
@end

