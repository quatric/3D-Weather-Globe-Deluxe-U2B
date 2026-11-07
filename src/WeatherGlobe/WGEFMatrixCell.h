#import "WeatherGlobePrefix.h"
#import "WGBezierPathCell.h"


@interface WGEFMatrixCell : WGBezierPathCell
{
    NSBezierPath *_highlightBezierPath;
    NSAttributedString *_weekdayAbbreviation;
    NSImage *_summaryImage;
    NSAttributedString *_temperatureHigh;
    NSAttributedString *_temperatureLow;
    NSDictionary *_weekdayAbbreviationAttributes;
    NSDictionary *_temperatureHighAttributes;
    NSDictionary *_temperatureLowAttributes;
}
- (id)temperatureLowAttributes;
- (id)temperatureHighAttributes;
- (id)weekdayAbbreviationAttributes;
- (BOOL)containsPoint:(NSPoint)aPoint inRect:(NSRect)aRect ofView:(id)aView;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)setTemperatureLow:(id)aLow;
- (id)temperatureLow;
- (void)setTemperatureHigh:(id)aHigh;
- (id)temperatureHigh;
- (void)setSummaryImage:(id)aImage;
- (id)summaryImage;
- (void)setWeekdayAbbreviation:(id)aAbbreviation;
- (id)weekdayAbbreviation;
- (void)setHighlightBezierPath:(id)aPath;
- (void)dealloc;
@end

