// WGTimeLineCell
#import "WGTimeLineControl.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

static LConstantString _kTimeLineMonthFormat("%b");

// _trackingObject
enum {
	kTrackingNone = 0,
	kTrackingTopKnob = 1,
	kTrackingTopSlider = 2,
	kTrackingBottomKnob = 3,
	kTrackingBottomSlider = 4
};

// _autoUpdateFlags
enum {
	kUpdateTopKnob = 1,
	kUpdateTopSlider = 2,
	kUpdateBottomKnob = 4,
	kUpdateBottomSlider = 8,
	kUpdateScrollToDate = 0x10
};

@implementation WGTimeLineCell

+ (BOOL)prefersTrackingUntilMouseUp
{
	return YES;
}

- (id)init
{
	self = [super init];
	if (self) {
		[self sendActionOn:NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged];
		_eclipseImage = [[NSImage imageNamed:@"Eclipse"] retain];
		_topKnobImage = [[NSImage imageNamed:@"Knoob_Top_Normal"] retain];
		_topKnobPressImage = [[NSImage imageNamed:@"Knoob_Top_Press"] retain];
		_bottomKnobImage = [[NSImage imageNamed:@"Knoob_Bottom_Normal"] retain];
		_bottomKnobPressImage = [[NSImage imageNamed:@"Knoob_Bottom_Press"] retain];
		_windowLeftImage = [[NSImage imageNamed:@"Window_Left"] retain];
		_windowCenterImage = [[NSImage imageNamed:@"Window_Center"] retain];
		_windowRightImage = [[NSImage imageNamed:@"Window_Right"] retain];
		_topBackgroundImage1 = [[NSImage imageNamed:@"TopLine_Darken"] retain];
		_topBackgroundImage2 = [[NSImage imageNamed:@"TopLine_Lighten"] retain];
		_bottomBackgroundImage = [[NSImage imageNamed:@"BottomLine"] retain];
		// One background per month.
		NSImage *d = _topBackgroundImage1, *l = _topBackgroundImage2;
		NSImage *pattern[12] = { d, d, l, l, l, d, d, d, l, l, l, d };
		for (int i = 0; i < 12; i++)
			_topBackgroundImages[i] = pattern[i];
	}
	return self;
}

- (void)dealloc
{
	[_animationTimer invalidate];
	[_monthTextCell release];
	[_hourTextCell release];
	[_eclipseImage release];
	[_topKnobImage release];
	[_topKnobPressImage release];
	[_bottomKnobImage release];
	[_bottomKnobPressImage release];
	[_windowLeftImage release];
	[_windowCenterImage release];
	[_windowRightImage release];
	[_topBackgroundImage1 release];
	[_topBackgroundImage2 release];
	[_bottomBackgroundImage release];
	if (_scrollToDate) {
		_scrollToDate->release();
		_scrollToDate = 0;
	}
	if (_trackingStartDate) {
		_trackingStartDate->release();
		_trackingStartDate = 0;
	}
	if (_dateVal) {
		_dateVal->release();
		_dateVal = 0;
	}
	[super dealloc];
}

- (int)intValue
{
	return (int)[self doubleValue];
}

- (void)setIntValue:(int)aValue
{
	[self setDoubleValue:aValue];
}

- (float)floatValue
{
	return (float)[self doubleValue];
}

- (void)setFloatValue:(float)aValue
{
	[self setDoubleValue:aValue];
}

- (double)doubleValue
{
	if (_dateVal)
		return _dateVal->timeIntervalSinceReferenceDate();
	return 0.0;
}

- (void)setDoubleValue:(double)aValue
{
	LDate *date = LDate::dateWithTimeIntervalSinceReferenceDate(aValue);
	LTimeZone *zone = [self dateValue]->timeZone();
	if (zone)
		date->setTimeZone(zone);
	[self setDateValue:date];
}

- (id)stringValue
{
	return [NSString stringReferenceFromLString:[self dateValue]->description()];
}

- (void)setDateValue:(LDate *)aValue
{
	if (aValue)
		aValue->retain();
	if (_dateVal) {
		_dateVal->release();
		_dateVal = 0;
	}
	_dateVal = aValue;
	[[self controlView] setNeedsDisplay:YES];
}

- (LDate *)dateValue
{
	if (!_dateVal)
		_dateVal = LDate::_alloc()->initWithTimeIntervalSinceReferenceDate(0.0);
	return _dateVal;
}

#pragma mark Solar eclipses

- (NSUInteger)_indexOfNearestSolarEclipseForDate:(LDate *)aDate
{
	return AtlasDatabase::instance()->indexOfNearestSolarEclipseForDate(aDate);
}

- (SSolarEclipse *)_solarEclipseAtIndex:(NSUInteger)aIndex
{
	AtlasDatabase *db = AtlasDatabase::instance();
	unsigned int count = db->solarEclipsesCount();
	if (count && aIndex < count)
		return (SSolarEclipse *)db->solarEclipseAtIndex((unsigned int)aIndex);
	return 0;
}

- (LDate *)_dateFromSolarEclipse:(SSolarEclipse *)aEclipse
{
	return Utils::dateFromSolarEclipse(aEclipse, [self dateValue]->timeZone());
}

- (LDate *)_solarEclipseDateAtIndex:(NSUInteger)aIndex
{
	return [self _dateFromSolarEclipse:[self _solarEclipseAtIndex:aIndex]];
}

- (id)_solarEclipseTooltipAtIndex:(NSUInteger)aIndex
{
	SSolarEclipse *eclipse = [self _solarEclipseAtIndex:aIndex];
	return [NSString stringReferenceFromLString:Utils::solarEclipseTooltipDescription(eclipse, [self dateValue]->timeZone())];
}

- (NSSize)_solarEclipseSize
{
	return [_eclipseImage size];
}

#pragma mark Geometry

- (NSSize)_knobSize:(BOOL)aSize
{
	return [(aSize ? _topKnobImage : _bottomKnobImage) size];
}

// The knob's offset from the cell's center, kept inside the cell.
- (CGFloat)_normalizedKnobOffset:(BOOL)aOffset forCellFrame:(NSRect)aFrame
{
	CGFloat knobWidth = [self _knobSize:aOffset].width;
	CGFloat center = floor(aFrame.origin.x + aFrame.size.width * 0.5);
	CGFloat offset = aOffset ? _topKnobOffset : _bottomKnobOffset;
	CGFloat half = floor(knobWidth * 0.5);
	CGFloat minOffset = half + aFrame.origin.x - center;
	CGFloat maxOffset = (NSMaxX(aFrame) - (knobWidth - half)) - center;
	if (offset < minOffset)
		offset = minOffset;
	if (offset > maxOffset)
		offset = maxOffset;
	return offset;
}

- (CGFloat)_currentTimePos:(BOOL)aPos forCellFrame:(NSRect)aFrame
{
	return [self _normalizedKnobOffset:aPos forCellFrame:aFrame] + floor(aFrame.origin.x + aFrame.size.width * 0.5);
}

- (NSRect)_knobRect:(BOOL)aRect forCellFrame:(NSRect)aFrame
{
	NSRect rect;
	rect.origin = NSZeroPoint;
	rect.size = [self _knobSize:aRect];
	if (aRect)
		rect.origin.y = NSMaxY(aFrame) - rect.size.height;
	rect.origin.x = [self _currentTimePos:aRect forCellFrame:aFrame] - floor(rect.size.width * 0.5);
	return rect;
}

- (NSRect)_frameRectForCellFrame:(NSRect)aFrame
{
	NSRect rect = aFrame;
	rect.size.height = [_windowLeftImage size].height;
	rect.origin.y += floor((aFrame.size.height - rect.size.height) * 0.5);
	return rect;
}

- (NSRect)_sliderRect:(BOOL)aRect forCellFrame:(NSRect)aFrame
{
	NSRect rect = NSInsetRect([self _frameRectForCellFrame:aFrame], 2.0, 3.0);
	if (aRect) {
		CGFloat height = [_topBackgroundImage1 size].height;
		rect.origin.y = NSMaxY(rect) - height;
		rect.size.height = height;
	} else
		rect.size.height = [_bottomBackgroundImage size].height;
	return rect;
}

#pragma mark Drawing

static void _drawImage(NSImage *image, NSRect rect)
{
	NSRect from = { NSZeroPoint, [image size] };
	[image drawInRect:rect fromRect:from operation:NSCompositingOperationSourceOver fraction:1.0];
}

- (void)_drawFrameWithFrame:(NSRect)aFrame
{
	NSRect frame = [self _frameRectForCellFrame:aFrame];
	CGFloat leftWidth = [_windowLeftImage size].width;
	_drawImage(_windowLeftImage, NSMakeRect(frame.origin.x, frame.origin.y, leftWidth, frame.size.height));
	CGFloat rightWidth = [_windowRightImage size].width;
	CGFloat rightX = NSMaxX(frame) - rightWidth;
	_drawImage(_windowRightImage, NSMakeRect(rightX, frame.origin.y, rightWidth, frame.size.height));
	CGFloat centerX = frame.origin.x + leftWidth;
	_drawImage(_windowCenterImage, NSMakeRect(centerX, frame.origin.y, rightX - centerX, frame.size.height));
}

- (void)_drawKnob:(BOOL)aKnob withFrame:(NSRect)aFrame
{
	NSRect rect = [self _knobRect:aKnob forCellFrame:aFrame];
	NSImage *image;
	if ([self isHighlighted] && _trackingObject == (aKnob ? kTrackingTopKnob : kTrackingBottomKnob))
		image = aKnob ? _topKnobPressImage : _bottomKnobPressImage;
	else
		image = aKnob ? _topKnobImage : _bottomKnobImage;
	_drawImage(image, rect);
}

- (void)_drawMonthString:(id)aString withFrame:(NSRect)aFrame inView:(id)aView
{
	if (!aString)
		return;
	if (!_monthTextCell) {
		_monthTextCell = [NSTextFieldCell new];
		[_monthTextCell setAlignment:NSTextAlignmentCenter];
		[_monthTextCell setFont:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSControlSizeSmall]]];
	}
	[_monthTextCell setStringValue:aString];
	CGFloat height = [_monthTextCell cellSize].height;
	CGFloat y = aFrame.origin.y + 3.0;
	[_monthTextCell setTextColor:[NSColor blackColor]];
	[_monthTextCell drawInteriorWithFrame:NSMakeRect(aFrame.origin.x, y + 1.0, aFrame.size.width, height) inView:aView];
	[_monthTextCell setTextColor:[NSColor whiteColor]];
	[_monthTextCell drawInteriorWithFrame:NSMakeRect(aFrame.origin.x, y, aFrame.size.width, height) inView:aView];
}

- (void)_drawHours:(NSInteger)aHours withFrame:(NSRect)aFrame inView:(id)aView
{
	if (!_hourTextCell) {
		_hourTextCell = [NSTextFieldCell new];
		[_hourTextCell setAlignment:NSTextAlignmentCenter];
		[_hourTextCell setFont:[NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:NSControlSizeMini]]];
	}
	NSString *string;
	if (Preferences::uses24HourClock())
		string = [NSString stringWithFormat:@"%02d", (int)aHours];
	else {
		int hours = (int)(aHours % 12);
		string = [NSString stringWithFormat:@"%d\n%@", hours ? hours : 12, aHours > 11 ? @"PM" : @"AM"];
	}
	[_hourTextCell setStringValue:string];
	CGFloat height = [_hourTextCell cellSize].height;
	CGFloat y = floor((aFrame.size.height - height) * 0.5) + aFrame.origin.y - 1.0;
	[_hourTextCell setTextColor:[NSColor whiteColor]];
	[_hourTextCell drawInteriorWithFrame:NSMakeRect(aFrame.origin.x, y, aFrame.size.width, height) inView:aView];
	[_hourTextCell setTextColor:[NSColor blackColor]];
	[_hourTextCell drawInteriorWithFrame:NSMakeRect(aFrame.origin.x, y + 1.0, aFrame.size.width, height) inView:aView];
}

- (void)_drawSolarEclipseWithFrame:(NSRect)aFrame doHighlight:(BOOL)aHighlight
{
	CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
	if (aHighlight)
		CGContextBeginTransparencyLayer(context, 0);
	_drawImage(_eclipseImage, aFrame);
	if (aHighlight) {
		[[[NSColor blackColor] colorWithAlphaComponent:0.5] set];
		NSRectFillUsingOperation(aFrame, NSCompositingOperationSourceAtop);
		CGContextEndTransparencyLayer(context);
	}
}

- (BOOL)_isScrollingToEclipseDate:(LDate *)aDate tracking:(int)aTracking
{
	return [self isHighlighted] && _trackingObject == aTracking && _scrollToDate
	    && _scrollToDate->timeIntervalSinceReferenceDate() == aDate->timeIntervalSinceReferenceDate();
}

// One pixel per day; each month's background with its name over it.
- (void)_drawTopSliderWithFrame:(NSRect)aFrame inView:(id)aView
{
	NSRect slider = [self _sliderRect:YES forCellFrame:aFrame];
	[NSGraphicsContext saveGraphicsState];
	CGContextClipToRect([[NSGraphicsContext currentContext] CGContext], NSRectToCGRect(slider));

	CGFloat pos = [self _currentTimePos:YES forCellFrame:aFrame];
	LDate *leftDate = [self dateValue]->dateByAddingTime(0, 0, (int)-(pos - slider.origin.x), 0, 0, 0);
	CGFloat x = slider.origin.x - (leftDate->day() - 1);
	LDate *month = LDate::dateWithTime(leftDate->year(), leftDate->month(), 1, 0, 0, 0, leftDate->timeZone());
	for (;;) {
		int monthIndex = month->month();
		int dayOfYear = month->dayOfYear();
		int year = month->year();
		LString *name = month->descriptionWithCalendarFormat(&_kTimeLineMonthFormat);
		month = month->dateByAddingTime(0, 1, 0, 0, 0, 0);
		CGFloat width = year == month->year() ? month->dayOfYear() - dayOfYear : 31.0;
		NSRect rect = NSMakeRect(x, slider.origin.y, width, slider.size.height);
		if (!NSIntersectsRect(rect, slider))
			break;
		_drawImage(_topBackgroundImages[monthIndex - 1], rect);
		[self _drawMonthString:[NSString stringReferenceFromLString:name] withFrame:rect inView:aView];
		x += width;
	}

	NSSize size = [self _solarEclipseSize];
	CGFloat y = NSMaxY(slider) - size.height - 3.0;
	NSUInteger index = [self _indexOfNearestSolarEclipseForDate:leftDate];
	CGFloat half = floor(size.width * 0.5);
	LDate *eclipseDate;
	while ((eclipseDate = [self _solarEclipseDateAtIndex:index])) {
		int days = Utils::daysIntervalBetweenDates([self dateValue], eclipseDate);
		NSRect rect = NSMakeRect(pos - half + days, y, size.width, size.height);
		if (rect.origin.x > NSMaxX(slider))
			break;
		if (NSIntersectsRect(rect, slider)) {
			[self _drawSolarEclipseWithFrame:rect
			                     doHighlight:[self _isScrollingToEclipseDate:eclipseDate tracking:kTrackingTopSlider]];
			[[NSView focusView] addToolTipRect:rect owner:self userData:(void *)index];
		}
		index++;
	}
	[NSGraphicsContext restoreGraphicsState];
}

// Four minutes per pixel: a tick every half hour, labels every six hours.
- (void)_drawBottomSliderWithFrame:(NSRect)aFrame inView:(id)aView
{
	NSRect slider = [self _sliderRect:NO forCellFrame:aFrame];
	_drawImage(_bottomBackgroundImage, slider);

	CGFloat pos = [self _currentTimePos:NO forCellFrame:aFrame];
	LDate *leftDate = Utils::dateByAddingMinutesInterval([self dateValue], (int)-((pos - slider.origin.x) * 4.0));
	LDate *hourDate = LDate::dateWithTime(leftDate->year(), leftDate->month(), leftDate->day(), leftDate->hour(), 0, 0,
	                                      leftDate->timeZone());
	CGFloat hourX = slider.origin.x - leftDate->minute() / 4;
	CGFloat x = hourX;
	int minutes = leftDate->hour() * 60;
	for (;;) {
		minutes %= 1440;
		if (minutes % 360 == 0)
			[self _drawHours:minutes / 60 withFrame:NSMakeRect(x - 15.0, slider.origin.y, 30.0, slider.size.height)
			          inView:aView];
		else {
			CGFloat tickHeight = minutes % 60 == 0 ? 8.0 : 4.0;
			NSRect tick = NSMakeRect(floor(x), floor((slider.size.height - tickHeight) * 0.5) + slider.origin.y, 1.0,
			                         tickHeight);
			if (tick.origin.x > NSMaxX(slider))
				break;
			if (NSIntersectsRect(tick, slider)) {
				[[NSColor blackColor] set];
				NSRectFill(tick);
				[[NSColor whiteColor] set];
				NSRectFill(NSMakeRect(tick.origin.x, tick.origin.y - 1.0, 1.0, 1.0));
			}
		}
		minutes += 30;
		x += 7.5;
	}

	NSUInteger index = [self _indexOfNearestSolarEclipseForDate:[self dateValue]];
	LDate *eclipseDate = [self _solarEclipseDateAtIndex:index];
	if (!eclipseDate)
		return;
	NSSize size = [self _solarEclipseSize];
	CGFloat half = floor(size.width * 0.5);
	CGFloat y = floor((slider.size.height - size.height) * 0.5) + slider.origin.y;
	int offset = Utils::minutesIntervalBetweenDates(hourDate, eclipseDate) / 4;
	NSRect rect = NSMakeRect(offset + (hourX - half), y, size.width, size.height);
	if (!NSIntersectsRect(rect, slider))
		return;
	[self _drawSolarEclipseWithFrame:rect
	                     doHighlight:[self _isScrollingToEclipseDate:eclipseDate tracking:kTrackingBottomSlider]];
	[[NSView focusView] addToolTipRect:rect owner:self userData:(void *)index];
}

- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView
{
	[self drawInteriorWithFrame:aFrame inView:aView];
}

- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	BOOL enabled = [self isEnabled];
	[[NSView focusView] removeAllToolTips];
	CGContextRef context = 0;
	if (!enabled) {
		[NSGraphicsContext saveGraphicsState];
		context = [[NSGraphicsContext currentContext] CGContext];
		CGContextSetAlpha(context, 0.5);
		CGContextBeginTransparencyLayer(context, 0);
	}
	[self _drawTopSliderWithFrame:aFrame inView:aView];
	[self _drawBottomSliderWithFrame:aFrame inView:aView];
	[self _drawFrameWithFrame:aFrame];
	[self _drawKnob:YES withFrame:aFrame];
	[self _drawKnob:NO withFrame:aFrame];
	if (!enabled) {
		CGContextEndTransparencyLayer(context);
		[NSGraphicsContext restoreGraphicsState];
	}
}

- (id)view:(id)aView stringForToolTip:(NSInteger)aTip point:(NSPoint)aPoint userData:(void *)aData
{
	return [self _solarEclipseTooltipAtIndex:(NSUInteger)aData];
}

#pragma mark Tracking

- (LDate *)_dateFromPoint:(NSPoint)aPoint forCellFrame:(NSRect)aFrame isTopWheel:(BOOL *)aWheel
{
	if (NSPointInRect(aPoint, [self _sliderRect:YES forCellFrame:aFrame])) {
		CGFloat pos = [self _currentTimePos:YES forCellFrame:aFrame];
		if (aWheel)
			*aWheel = YES;
		return [self dateValue]->dateByAddingTime(0, 0, (int)(aPoint.x - pos), 0, 0, 0);
	}
	if (NSPointInRect(aPoint, [self _sliderRect:NO forCellFrame:aFrame])) {
		CGFloat pos = [self _currentTimePos:NO forCellFrame:aFrame];
		if (aWheel)
			*aWheel = NO;
		return Utils::dateByAddingMinutesInterval([self dateValue], (int)((aPoint.x - pos) * 4.0));
	}
	return 0;
}

// The eclipse under the point, if any.
- (LDate *)_solarEclipseDateFromPoint:(NSPoint)aPoint forCellFrame:(NSRect)aFrame
{
	BOOL topWheel = NO;
	LDate *date = [self _dateFromPoint:aPoint forCellFrame:aFrame isTopWheel:&topWheel];
	if (!date)
		return 0;
	NSUInteger index = [self _indexOfNearestSolarEclipseForDate:date];
	LDate *eclipseDate = [self _solarEclipseDateAtIndex:index];
	if (!eclipseDate)
		return 0;
	double distance = fabs(date->timeIntervalSinceReferenceDate() - eclipseDate->timeIntervalSinceReferenceDate()) / 60.0;
	if (topWheel)
		distance /= 1440.0;
	else
		distance *= 0.25;
	if (distance <= floor([self _solarEclipseSize].width * 0.5 + 0.5))
		return eclipseDate;
	return 0;
}

- (void)_sendDateAction
{
	id view = [self controlView];
	if ([view isKindOfClass:[NSControl class]])
		[(NSControl *)view sendAction:[self action] to:[self target]];
}

- (void)_timeDidBeginEditing
{
	id view = [self controlView];
	if ([view isKindOfClass:[WGTimeLineControl class]] && view) {
		if ([[self target] respondsToSelector:@selector(timeDidBeginEditing:)])
			[[self target] performSelector:@selector(timeDidBeginEditing:) withObject:view];
	}
}

- (void)_timeDidEndEditing
{
	id view = [self controlView];
	if ([view isKindOfClass:[WGTimeLineControl class]] && view) {
		if ([[self target] respondsToSelector:@selector(timeDidEndEditing:)])
			[[self target] performSelector:@selector(timeDidEndEditing:) withObject:view];
	}
}

- (void)_setupTimer
{
	if (_animationTimer) {
		if (_autoUpdateFlags)
			return;
		[_animationTimer invalidate];
		_animationTimer = nil;
		[self _timeDidEndEditing];
	}
	if (_autoUpdateFlags && !_animationTimer) {
		_animationTimer = [NSTimer timerWithTimeInterval:1.0 / 30.0
		                                          target:self
		                                        selector:@selector(_onTimer:)
		                                        userInfo:nil
		                                         repeats:YES];
		[[NSRunLoop currentRunLoop] addTimer:_animationTimer forMode:NSDefaultRunLoopMode];
		[[NSRunLoop currentRunLoop] addTimer:_animationTimer forMode:NSEventTrackingRunLoopMode];
	}
}

// Scrolls a wheel by the drag speed: days on the top one, minutes on the bottom one.
- (void)_moveSlider:(BOOL)aSlider
{
	double now = [NSDate timeIntervalSinceReferenceDate];
	int factor = aSlider ? 3 : 12;
	double prev = _slidingPrevTime;
	_slidingPrevTime = now;
	float speed = factor * _slidingSpeed;
	float limit = factor * 100;
	if (speed < -limit)
		speed = -limit;
	else if (speed > limit)
		speed = limit;
	double offset = _slidingOffset + (now - prev) * speed;
	int steps = (int)offset;
	_slidingOffset = (float)offset - steps;
	LDate *date;
	if (aSlider)
		date = [self dateValue]->dateByAddingTime(0, 0, steps, 0, 0, 0);
	else
		date = Utils::dateByAddingMinutesInterval([self dateValue], steps);
	[self setDateValue:date];
	[self _sendDateAction];
}

// Moves a released knob back toward the center, faster as time goes on.
static BOOL _returnKnob(float *offset, double startTime, double now)
{
	float step = floorf((float)((now - startTime) * 30.0 + 0.5));
	if (step < fabsf(*offset)) {
		*offset += *offset > 0.0f ? -step : step;
		return YES;
	}
	*offset = 0.0f;
	return NO;
}

- (void)_onTimer:(id)aTimer
{
	double now = [NSDate timeIntervalSinceReferenceDate];
	if (_autoUpdateFlags & kUpdateTopKnob) {
		if (_trackingObject == kTrackingTopKnob || _trackingObject == kTrackingTopSlider)
			_topKnobStartTime = now;
		else {
			if (!_returnKnob(&_topKnobOffset, _topKnobStartTime, now)) {
				_autoUpdateFlags &= ~kUpdateTopKnob;
				[self _setupTimer];
			}
			[[self controlView] setNeedsDisplay:YES];
		}
	}
	if (_autoUpdateFlags & kUpdateBottomKnob) {
		if (_trackingObject == kTrackingBottomKnob || _trackingObject == kTrackingBottomSlider)
			_bottomKnobStartTime = now;
		else {
			if (!_returnKnob(&_bottomKnobOffset, _bottomKnobStartTime, now)) {
				_autoUpdateFlags &= ~kUpdateBottomKnob;
				[self _setupTimer];
			}
			[[self controlView] setNeedsDisplay:YES];
		}
	}
	if (_autoUpdateFlags & kUpdateTopSlider)
		[self _moveSlider:YES];
	if (_autoUpdateFlags & kUpdateBottomSlider)
		[self _moveSlider:NO];
	if (!(_autoUpdateFlags & kUpdateScrollToDate))
		return;

	if (_trackingObject != kTrackingNone || ![self isEnabled]) {
		if (_scrollToDate) {
			_scrollToDate->release();
			_scrollToDate = 0;
		}
	}
	if (_scrollToDate) {
		// Scroll to the clicked eclipse: by days while it is a day or more away, then by minutes.
		float step = (float)((now - _scrollToDateStartTime) * 30.0);
		LDate *date = [self dateValue];
		double minutes = (_scrollToDate->timeIntervalSinceReferenceDate() - date->timeIntervalSinceReferenceDate()) / 60.0;
		LDate *newDate;
		if (fabs(minutes) >= 1440.0) {
			double days = minutes / 1440.0;
			if (fabs(days) < step) {
				_scrollToDateStartTime = now;
				step = fabsf((float)days);
			}
			if (days < 0.0)
				step = -step;
			newDate = date->dateByAddingTime(0, 0, (int)step, 0, 0, 0);
		} else {
			step *= 4.0f;
			if (step < fabs(minutes)) {
				if (minutes < 0.0)
					step = -step;
				newDate = Utils::dateByAddingMinutesInterval(date, (int)step);
			} else {
				newDate = LAutorelease(_scrollToDate);
				_scrollToDate = 0;
			}
		}
		[self setDateValue:newDate];
		[self _sendDateAction];
		if (_scrollToDate)
			return;
	}
	_autoUpdateFlags &= ~kUpdateScrollToDate;
	[self _setupTimer];
}

- (void)_setTrackingStartDate:(LDate *)aDate
{
	LRetain(aDate);
	LRelease(_trackingStartDate);
	_trackingStartDate = aDate;
}

- (BOOL)startTrackingAt:(NSPoint)aAt inView:(id)aView
{
	NSRect bounds = [aView bounds];
	if (NSPointInRect(aAt, [self _knobRect:YES forCellFrame:bounds])) {
		CGFloat offset = [self _normalizedKnobOffset:YES forCellFrame:bounds];
		[self _setTrackingStartDate:[self dateValue]->dateByAddingTime(0, 0, (int)-offset, 0, 0, 0)];
		_trackingObject = kTrackingTopKnob;
	} else if (NSPointInRect(aAt, [self _knobRect:NO forCellFrame:bounds])) {
		CGFloat offset = [self _normalizedKnobOffset:NO forCellFrame:bounds];
		[self _setTrackingStartDate:Utils::dateByAddingMinutesInterval([self dateValue], (int)-(offset * 4.0))];
		_trackingObject = kTrackingBottomKnob;
	} else {
		if (NSPointInRect(aAt, [self _sliderRect:YES forCellFrame:bounds])) {
			_autoUpdateFlags |= kUpdateTopSlider;
			_trackingObject = kTrackingTopSlider;
		} else if (NSPointInRect(aAt, [self _sliderRect:NO forCellFrame:bounds])) {
			_autoUpdateFlags |= kUpdateBottomSlider;
			_trackingObject = kTrackingBottomSlider;
		}
		if (_trackingObject != kTrackingTopSlider && _trackingObject != kTrackingBottomSlider)
			return NO;
		_autoUpdateFlags &= ~kUpdateScrollToDate;
		if (_scrollToDate) {
			_scrollToDate->release();
			_scrollToDate = 0;
		}
		// A click on an eclipse scrolls to it once the mouse is up.
		_scrollToDate = LRetain([self _solarEclipseDateFromPoint:aAt forCellFrame:bounds]);
		_slidingPrevTime = [NSDate timeIntervalSinceReferenceDate];
		_slidingOffset = 0.0f;
		_slidingSpeed = 0.0f;
		[self _setupTimer];
	}
	[self _timeDidBeginEditing];
	return YES;
}

- (BOOL)continueTracking:(NSPoint)aTracking at:(NSPoint)aAt inView:(id)aView
{
	float delta = aAt.x - aTracking.x;
	switch (_trackingObject) {
	case kTrackingTopSlider:
	case kTrackingBottomSlider:
		if (_scrollToDate) {
			_scrollToDate->release();
			_scrollToDate = 0;
		}
		_slidingSpeed += delta;
		break;
	case kTrackingTopKnob: {
		_topKnobOffset += delta;
		CGFloat offset = [self _normalizedKnobOffset:YES forCellFrame:[aView bounds]];
		[self setDateValue:_trackingStartDate->dateByAddingTime(0, 0, (int)offset, 0, 0, 0)];
		break;
	}
	case kTrackingBottomKnob: {
		_bottomKnobOffset += delta;
		CGFloat offset = [self _normalizedKnobOffset:NO forCellFrame:[aView bounds]];
		[self setDateValue:Utils::dateByAddingMinutesInterval(_trackingStartDate, (int)((float)offset * 4.0f))];
		break;
	}
	}
	return [super continueTracking:aTracking at:aAt inView:aView];
}

- (void)stopTracking:(NSPoint)aTracking at:(NSPoint)aAt inView:(id)aView mouseIsUp:(BOOL)aUp
{
	switch (_trackingObject) {
	case kTrackingTopSlider:
		_autoUpdateFlags &= ~kUpdateTopSlider;
		break;
	case kTrackingTopKnob:
		_topKnobOffset = [self _normalizedKnobOffset:YES forCellFrame:[aView bounds]];
		_autoUpdateFlags |= kUpdateTopKnob;
		_topKnobStartTime = [NSDate timeIntervalSinceReferenceDate];
		break;
	case kTrackingBottomKnob:
		_bottomKnobOffset = [self _normalizedKnobOffset:NO forCellFrame:[aView bounds]];
		_autoUpdateFlags |= kUpdateBottomKnob;
		_bottomKnobStartTime = [NSDate timeIntervalSinceReferenceDate];
		break;
	case kTrackingBottomSlider:
		_autoUpdateFlags &= ~kUpdateBottomSlider;
		break;
	}
	if (_scrollToDate) {
		_autoUpdateFlags |= kUpdateScrollToDate;
		_scrollToDateStartTime = [NSDate timeIntervalSinceReferenceDate];
	}
	_trackingObject = kTrackingNone;
	[self _setupTimer];
}

@end

#pragma clang diagnostic pop
