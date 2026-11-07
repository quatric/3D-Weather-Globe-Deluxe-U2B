#import "EFOneDayController.h"
#import "NSImage+NSImageWGExtensions.h"

// The report value's description, or "".
static NSString *_description(LString *description)
{
	NSString *string = [NSString stringReferenceFromLString:description];
	return string ? string : @"";
}

// The first image of a CWManager image list.
static LImage *_firstImage(LArray *images)
{
	if (!images || images->count() == 0)
		return 0;
	return static_cast<LImage *>(images->objectAtIndex(0));
}

@implementation EFOneDayController

- (void)dealloc
{
	[_dataView release];
	[super dealloc];
}

- (id)nibName
{
	return @"EFOneDayView";
}

- (id)dataView
{
	return _dataView;
}

// A day of the extended forecast (a weather report map).
- (void)setValues:(LMap *)aValues forDate:(id)aDate
{
	[self loadNibIfNeeded];
	[_weekday setStringValue:aDate ? [aDate descriptionWithCalendarFormat:@"%a %d"] : @""];
	[_temperatureHigh setStringValue:aValues ? _description(Utils::temperatureDescription(aValues->objectForKey(kReportFCTemperatureHigh))) : @""];
	[_temperatureLow setStringValue:aValues ? _description(Utils::temperatureDescription(aValues->objectForKey(kReportFCTemperatureLow))) : @""];
	[_feelsLike setStringValue:aValues ? _description(Utils::temperatureDescription(aValues->objectForKey(kReportComfortLevel))) : @""];
	[_windSpeed setStringValue:aValues ? _description(Utils::speedDescription(aValues->objectForKey(kReportWindSpeed))) : @""];
	[_humidity setStringValue:aValues ? _description(Utils::percentsDescription(aValues->objectForKey(kReportHumidity))) : @""];
	[_preciption setStringValue:aValues ? _description(Utils::percentsDescription(aValues->objectForKey(kReportFCPreciptProbability))) : @""];
	[_dewPoint setStringValue:aValues ? _description(Utils::temperatureDescription(aValues->objectForKey(kReportDewPoint))) : @""];
	[_uvIndex setStringValue:aValues ? _description(lobj_cast<LString>(aValues->objectForKey(kReportFCUVDescription))) : @""];
	LImage *summary = 0;
	if (aValues) {
		int icon = loadIntFromObject(aValues->objectForKey(kReportSummaryCode));
		if (icon)
			summary = _firstImage(CWManager::instance()->weatherImagesForIconAtIndex(icon));
	}
	[_summary setImage:[NSImage wgImageWithLImage:summary]];
	LImage *wind = 0;
	if (aValues) {
		int index = Utils::windImageIndex(loadIntFromObject(aValues->objectForKey(kReportWindDir)));
		if (index)
			wind = _firstImage(CWManager::instance()->windImagesForIconAtIndex(index));
	}
	[_windDirection setImage:[NSImage wgImageWithLImage:wind]];
}

@end
