#import "WGEFMatrix.h"
#import "WGEFMatrixCell.h"
#import "NSImage+NSImageWGExtensions.h"

// The first image of a CWManager image list.
static LImage *_firstImage(LArray *images)
{
	if (!images || images->count() == 0)
		return 0;
	return static_cast<LImage *>(images->objectAtIndex(0));
}

// The cells are tabs with rounded top corners of radius 5, inset 10 from
// the sides; these add the corner curves (the control points are the
// original's, at 1/6 and 1/2 of the corner).

// From (aRight, aTop) down to the right.
static void _topRightCorner(NSBezierPath *path, CGFloat aRight, CGFloat aTop)
{
	CGFloat x = aRight + 5.0, y = aTop - 5.0;
	[path curveToPoint:NSMakePoint(x, y) controlPoint1:NSMakePoint((x - aRight) * 0.5 + aRight, aTop - (aTop - y) / 6.0)
		controlPoint2:NSMakePoint(x - (x - aRight) / 6.0, (aTop - y) * 0.5 + y)];
}

// From 5 below and left of (aLeft, aTop) up to it.
static void _topLeftCorner(NSBezierPath *path, CGFloat aLeft, CGFloat aTop)
{
	CGFloat x = aLeft - 5.0, y = aTop - 5.0;
	[path curveToPoint:NSMakePoint(aLeft, aTop) controlPoint1:NSMakePoint((aLeft - x) / 6.0 + x, (aTop - y) * 0.5 + y)
		controlPoint2:NSMakePoint((aLeft - x) * 0.5 + x, aTop - (aTop - y) / 6.0)];
}

// The highlighted tab flares out at the bottom: from aFrom down to the
// right to aTo.
static void _bottomRightFlare(NSBezierPath *path, NSPoint aFrom, NSPoint aTo)
{
	CGFloat dx = aTo.x - aFrom.x, dy = aFrom.y - aTo.y;
	[path curveToPoint:aTo controlPoint1:NSMakePoint(dx / 6.0 + aFrom.x, aFrom.y - dy * 0.5)
		controlPoint2:NSMakePoint(aTo.x - dx * 0.5, dy / 6.0 + aTo.y)];
}

// From aFrom up to the right to aTo.
static void _bottomLeftFlare(NSBezierPath *path, NSPoint aFrom, NSPoint aTo)
{
	CGFloat dx = aTo.x - aFrom.x, dy = aTo.y - aFrom.y;
	[path curveToPoint:aTo controlPoint1:NSMakePoint(dx * 0.5 + aFrom.x, dy / 6.0 + aFrom.y)
		controlPoint2:NSMakePoint(aTo.x - dx / 6.0, aFrom.y + dy * 0.5)];
}

// The extended forecast strip: seven tab cells, one per day.
@implementation WGEFMatrix

- (NSRect)defaultCellBounds
{
	return NSMakeRect(0.0, 0.0, 51.0, 86.0);
}

- (id)leftCellBezierPath
{
	return [self cellBezierPath];
}

- (id)rightCellBezierPath
{
	return [self cellBezierPath];
}

- (id)defaultCellFrameColor
{
	return [NSColor colorWithDeviceRed:0.52 green:0.6 blue:0.69 alpha:1.0];
}

- (id)defaultCellFillColor
{
	return [NSColor colorWithDeviceRed:0.91 green:0.91 blue:0.92 alpha:1.0];
}

- (id)defaultCellHighlightColor
{
	return [NSColor colorWithDeviceRed:1.0 green:1.0 blue:1.0 alpha:1.0];
}

- (id)shortTemperatureFromOneDayReport:(LMap *)aReport high:(BOOL)aHigh
{
	LObject *temperature = 0;
	if (aReport)
		temperature = aReport->objectForKey(aHigh ? kReportFCTemperatureHigh : kReportFCTemperatureLow);
	return [NSString stringReferenceFromLString:Utils::temperatureShortDescription(temperature)];
}

// Fills the bottom row from the day reports; days without one show the
// weekday from today and dashes.
- (void)refreshWithDayForecasts:(LArray *)aForecasts
{
	NSInteger row = [self numberOfRows] - 1;
	NSInteger columns = [self numberOfColumns];
	if (!aForecasts) {
		for (NSInteger column = 0; column < columns; column++) {
			WGEFMatrixCell *cell = [self cellAtRow:row column:column];
			[cell setWeekdayAbbreviation:[[NSString stringReferenceFromLString:Utils::weekdayName((int)column, YES)] uppercaseString]];
			[cell setSummaryImage:[self noReportWeatherImage]];
			[cell setTemperatureHigh:@"--"];
			[cell setTemperatureLow:@"--"];
			[self setToolTip:@"--" forCell:cell];
		}
		return;
	}
	int count = aForecasts->count();
	// The original keeps the last abbreviation for a report without a day.
	NSString *abbreviation = nil;
	for (NSInteger column = 0; column < columns && column < count; column++) {
		LMap *report = lobj_cast<LMap>(aForecasts->objectAtIndex((int)column));
		WGEFMatrixCell *cell = [self cellAtRow:row column:column];
		NSString *toolTip;
		if (!report) {
			abbreviation = [[NSString stringReferenceFromLString:Utils::weekdayName((int)column, YES)] uppercaseString];
			[cell setWeekdayAbbreviation:abbreviation];
			[cell setSummaryImage:[self noReportWeatherImage]];
			[cell setTemperatureHigh:@"--"];
			[cell setTemperatureLow:@"--"];
			toolTip = @"--";
		} else {
			LObject *day = report->objectForKey(kReportFCDayOfWeek);
			if (day && day->getClass()->isSubclassOf(LF_CLASSOF(LString)))
				abbreviation = [[NSString stringReferenceFromLString:
					Utils::weekdayName(UMapParsingUtils::loadIntFromObject(day) - 1, YES)] uppercaseString];
			[cell setWeekdayAbbreviation:abbreviation];
			[cell setSummaryImage:[self weatherImageFromObject:report->objectForKey(kReportSummaryCode)]];
			[cell setTemperatureHigh:[self shortTemperatureFromOneDayReport:report high:YES]];
			[cell setTemperatureLow:[self shortTemperatureFromOneDayReport:report high:NO]];
			toolTip = [NSString stringReferenceFromLString:Utils::weatherDescription(report)];
		}
		[self setToolTip:toolTip forCell:cell];
	}
}

// The selected tab of the rightmost day: straight down on the right, and
// flaring out to the left at the bottom.
- (id)rightCellHighlightBezierPath
{
	NSBezierPath *path = [NSBezierPath bezierPath];
	NSRect bounds = [self defaultCellBounds];
	CGFloat top = bounds.origin.y + bounds.size.height - 0.5;
	CGFloat left = bounds.origin.x + 10.0 + 0.5;
	CGFloat right = bounds.size.width - 10.0 - 0.5 + bounds.origin.x;
	CGFloat bottom = bounds.origin.y + 0.5;
	CGFloat edge = bounds.origin.x + 0.5;
	[path moveToPoint:NSMakePoint(left, top)];
	[path lineToPoint:NSMakePoint(right, top)];
	_topRightCorner(path, right, top);
	[path lineToPoint:NSMakePoint(bounds.origin.x + bounds.size.width - 5.0 - 0.5, bottom - 2.0)];
	[path lineToPoint:NSMakePoint(edge, bottom - 2.0)];
	[path lineToPoint:NSMakePoint(edge, bottom)];
	_bottomLeftFlare(path, NSMakePoint(edge, bottom), NSMakePoint(left - 5.0, bottom + 5.0));
	[path lineToPoint:NSMakePoint(left - 5.0, bounds.origin.y + bounds.size.height - 5.0 - 0.5)];
	_topLeftCorner(path, left, top);
	[path closePath];
	return path;
}

// The selected tab of the leftmost day: flaring out to the right at the
// bottom, straight down on the left.
- (id)leftCellHighlightBezierPath
{
	NSBezierPath *path = [NSBezierPath bezierPath];
	NSRect bounds = [self defaultCellBounds];
	CGFloat top = bounds.origin.y + bounds.size.height - 0.5;
	CGFloat left = bounds.origin.x + 10.0 + 0.5;
	CGFloat right = bounds.size.width - 10.0 - 0.5 + bounds.origin.x;
	CGFloat bottom = bounds.origin.y + 0.5;
	CGFloat edge = bounds.origin.x + bounds.size.width + 0.5;
	[path moveToPoint:NSMakePoint(left, top)];
	[path lineToPoint:NSMakePoint(right, top)];
	_topRightCorner(path, right, top);
	[path lineToPoint:NSMakePoint(right + 5.0, bounds.origin.y + 5.0 + 0.5)];
	_bottomRightFlare(path, NSMakePoint(right + 5.0, bounds.origin.y + 5.0 + 0.5), NSMakePoint(edge, bottom));
	[path lineToPoint:NSMakePoint(edge, bottom - 2.0)];
	[path lineToPoint:NSMakePoint(left - 5.0, bottom - 2.0)];
	[path lineToPoint:NSMakePoint(left - 5.0, bounds.origin.y + bounds.size.height - 5.0 - 0.5)];
	_topLeftCorner(path, left, top);
	[path closePath];
	return path;
}

// The selected tab: flaring out at the bottom on both sides.
- (id)cellHighlightBezierPath
{
	NSBezierPath *path = [NSBezierPath bezierPath];
	NSRect bounds = [self defaultCellBounds];
	CGFloat top = bounds.origin.y + bounds.size.height - 0.5;
	CGFloat left = bounds.origin.x + 10.0 + 0.5;
	CGFloat right = bounds.size.width - 10.0 - 0.5 + bounds.origin.x;
	CGFloat bottom = bounds.origin.y + 0.5;
	CGFloat rightEdge = bounds.origin.x + bounds.size.width + 0.5;
	CGFloat leftEdge = bounds.origin.x + 0.5;
	[path moveToPoint:NSMakePoint(left, top)];
	[path lineToPoint:NSMakePoint(right, top)];
	_topRightCorner(path, right, top);
	[path lineToPoint:NSMakePoint(right + 5.0, bounds.origin.y + 5.0 + 0.5)];
	_bottomRightFlare(path, NSMakePoint(right + 5.0, bounds.origin.y + 5.0 + 0.5), NSMakePoint(rightEdge, bottom));
	[path lineToPoint:NSMakePoint(rightEdge, bottom - 2.0)];
	[path lineToPoint:NSMakePoint(leftEdge, bottom - 2.0)];
	[path lineToPoint:NSMakePoint(leftEdge, bottom)];
	_bottomLeftFlare(path, NSMakePoint(leftEdge, bottom), NSMakePoint(left - 5.0, bottom + 5.0));
	[path lineToPoint:NSMakePoint(left - 5.0, bounds.origin.y + bounds.size.height - 5.0 - 0.5)];
	_topLeftCorner(path, left, top);
	[path closePath];
	return path;
}

// An unselected tab, square at the bottom.
- (id)cellBezierPath
{
	NSBezierPath *path = [NSBezierPath bezierPath];
	NSRect bounds = [self defaultCellBounds];
	CGFloat top = bounds.origin.y + bounds.size.height - 0.5;
	CGFloat left = bounds.origin.x + 10.0 + 0.5;
	CGFloat right = bounds.size.width - 10.0 - 0.5 + bounds.origin.x;
	[path moveToPoint:NSMakePoint(left, top)];
	[path lineToPoint:NSMakePoint(right, top)];
	_topRightCorner(path, right, top);
	[path lineToPoint:NSMakePoint(right + 5.0, bounds.origin.y + 0.5)];
	[path lineToPoint:NSMakePoint(left - 5.0, bounds.origin.y + 0.5)];
	[path lineToPoint:NSMakePoint(left - 5.0, bounds.origin.y + bounds.size.height - 5.0 - 0.5)];
	_topLeftCorner(path, left, top);
	[path closePath];
	return path;
}

// One row of seven overlapping tabs; the end ones get their own paths.
- (void)setupCells
{
	[self setCellClass:[WGEFMatrixCell class]];
	[self setMode:NSRadioModeMatrix];
	[self setDrawsBackground:NO];
	[self setDrawsCellBackground:NO];
	[self setCellSize:NSMakeSize(51.0, 86.0)];
	[self setIntercellSpacing:NSMakeSize(-11.0, 0.0)];
	[self renewRows:1 columns:7];
	[self sizeToCells];
	NSInteger rows = [self numberOfRows];
	NSInteger columns = [self numberOfColumns];
	NSBezierPath *path = [self cellBezierPath];
	NSBezierPath *highlightPath = [self cellHighlightBezierPath];
	NSRect bounds = [self defaultCellBounds];
	NSColor *frameColor = [self defaultCellFrameColor];
	NSColor *fillColor = [self defaultCellFillColor];
	NSColor *highlightColor = [self defaultCellHighlightColor];
	for (NSInteger column = 0; column < columns; column++) {
		WGEFMatrixCell *cell = [self cellAtRow:rows - 1 column:column];
		NSBezierPath *cellPath = path;
		NSBezierPath *cellHighlightPath = highlightPath;
		if (column == columns - 1) {
			cellPath = [self rightCellBezierPath];
			cellHighlightPath = [self rightCellHighlightBezierPath];
		} else if (column == 0) {
			cellPath = [self leftCellBezierPath];
			cellHighlightPath = [self leftCellHighlightBezierPath];
		}
		[cell setBezierPath:cellPath];
		[cell setHighlightBezierPath:cellHighlightPath];
		[cell setDefaultCellBounds:bounds];
		[cell setFrameColor:frameColor];
		[cell setFillColor:fillColor];
		[cell setHighlightColor:highlightColor];
	}
}

- (id)weatherImageFromObject:(LObject *)aObject
{
	int icon = aObject ? UMapParsingUtils::loadIntFromObject(aObject) : 0;
	if (!icon)
		return nil;
	return [NSImage wgImageWithLImage:_firstImage(CWManager::instance()->weatherImagesForIconAtIndex(icon))];
}

// Icon 35 is the "no report" image.
- (id)noReportWeatherImage
{
	return [NSImage wgImageWithLImage:_firstImage(CWManager::instance()->weatherImagesForIconAtIndex(35))];
}

@end
