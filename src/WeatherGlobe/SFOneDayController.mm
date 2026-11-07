#import "SFOneDayController.h"
#import "NSImage+NSImageWGExtensions.h"

static NSString *_temperature(LMap *values, LString *key)
{
	NSString *string = values ? [NSString stringReferenceFromLString:Utils::temperatureDescription(values->objectForKey(key))] : nil;
	return string ? string : @"";
}

// One day of the short forecast: weekday, high and low, summary icon.
@implementation SFOneDayController

- (void)dealloc
{
	[_dataView release];
	[super dealloc];
}

- (id)nibName
{
	return @"SFOneDayView";
}

- (void)awakeFromNib
{
	[_topBorder setHidden:YES];
	[_bottomBorder setHidden:YES];
	[_rightBorder setHidden:YES];
}

- (id)dataView
{
	[self loadNibIfNeeded];
	return _dataView;
}

- (void)setValues:(LMap *)aValues forDate:(id)aDate
{
	[self loadNibIfNeeded];
	[_weekday setStringValue:aDate ? [aDate descriptionWithCalendarFormat:@"%a %d"] : @""];
	[_temperatureHigh setStringValue:_temperature(aValues, kReportFCTemperatureHigh)];
	[_temperatureLow setStringValue:_temperature(aValues, kReportFCTemperatureLow)];
	LImage *summary = 0;
	if (aValues) {
		int icon = loadIntFromObject(aValues->objectForKey(kReportSummaryCode));
		if (icon) {
			LArray *images = CWManager::instance()->weatherImagesForIconAtIndex(icon);
			if (images && images->count())
				summary = static_cast<LImage *>(images->objectAtIndex(0));
		}
	}
	[_summary setImage:[NSImage wgImageWithLImage:summary]];
}

- (void)setDrawsTopAndBottomBorders:(BOOL)aBorders
{
	[self loadNibIfNeeded];
	[_topBorder setHidden:!aBorders];
	[_bottomBorder setHidden:!aBorders];
}

- (void)setDrawsRightBorder:(BOOL)aBorder
{
	[self loadNibIfNeeded];
	[_rightBorder setHidden:!aBorder];
}

@end
