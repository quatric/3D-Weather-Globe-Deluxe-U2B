#import "WeatherGlobePrefix.h"
#import "WGBezierPathMatrix.h"


@interface WGEFMatrix : WGBezierPathMatrix
{
}
- (id)shortTemperatureFromOneDayReport:(LMap *)aReport high:(BOOL)aHigh;
- (id)noReportWeatherImage;
- (id)weatherImageFromObject:(LObject *)aObject;
- (id)defaultCellHighlightColor;
- (id)defaultCellFillColor;
- (id)defaultCellFrameColor;
- (id)rightCellHighlightBezierPath;
- (id)rightCellBezierPath;
- (id)leftCellHighlightBezierPath;
- (id)leftCellBezierPath;
- (id)cellHighlightBezierPath;
- (id)cellBezierPath;
- (NSRect)defaultCellBounds;
- (void)refreshWithDayForecasts:(LArray *)aForecasts;
- (void)setupCells;
@end

