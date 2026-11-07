#import "CPExpandedForecastView.h"

@implementation NSCalendarDate (DescriptionsExtension)

// The day of the week whose abbreviation is the widest.
+ (NSInteger)weekdayWithMaxAbbrDescriptionWidth
{
	float maxWidth = 0.0f;
	NSInteger result = 0;
	for (int day = 0; day != 7; day++) {
		NSCalendarDate *date = [NSCalendarDate dateWithYear:0 month:0 day:day hour:0 minute:0 second:0 timeZone:nil];
		float width = [[date descriptionWithCalendarFormat:@"%a"] sizeWithAttributes:nil].width;
		if (maxWidth < width) {
			result = [date dayOfWeek];
			maxWidth = width;
		}
	}
	return result;
}

// The month whose abbreviation is the widest.
+ (NSInteger)monthWithMaxAbbrDescriptionWidth
{
	float maxWidth = 0.0f;
	NSInteger result = 0;
	for (int month = 0; month != 12; month++) {
		NSCalendarDate *date = [NSCalendarDate dateWithYear:0 month:month day:0 hour:0 minute:0 second:0 timeZone:nil];
		float width = [[date descriptionWithCalendarFormat:@"%b"] sizeWithAttributes:nil].width;
		if (maxWidth < width) {
			result = [date monthOfYear];
			maxWidth = width;
		}
	}
	return result;
}

@end
