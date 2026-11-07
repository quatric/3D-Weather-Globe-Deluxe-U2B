#import "WGTimeLineController.h"
#import "WGTimeLineControl.h"
#import "WGCustomSlider.h"
#import "WGTexturedButton.h"
#import "WGTextFieldCell.h"
#import "WGHelpBook.h"
#import "NSButton+NSButtonWGExt.h"
#import "NSView+WGExtensions.h"
#import "SMKTintedView.h"
#include "AMTime.h"

// The calendar formats of the date fields.
static LConstantString _shortMonthFormat("%b");
static LConstantString _amPMTimeFormat("%p");
static LConstantString _12TimeFormatHourRepresenation("%I");
static LConstantString _24TimeFormatHourRepresenation("%H");

// The controls' tags (controlsAction:).
enum {
	kEnableTag = 10,
	kStepperTag = 70,
	kCurrentTag = 80,
	kTimeLineTag = 110,
	kSpeedTag = 120,
	kBackwardTag = 201,
	kPlayPauseTag = 202,
	kForwardTag = 203,
};

// The time line palette: a custom world time (its date in fields set by a
// stepper, or on the time line) and its playing forward or backward.
@implementation WGTimeLineController

// The fields show the time controller's date; the stepper the focused
// field's value.
- (void)_setupCustomTimeControls
{
	LDate *date = TimeController::instance()->date();
	if (!date)
		return;
	bool uses24 = Preferences::uses24HourClock();
	[_timeLineControl setDateValue:date];
	[_dayField setStringValue:[NSString stringWithFormat:@"%d", date->day()]];
	[_yearField setStringValue:[NSString stringWithFormat:@"%d", date->year()]];
	[_minutesField setStringValue:[NSString stringWithFormat:@"%02d", date->minute()]];
	[_amPMField setStringValue:[NSString stringReferenceFromLString:date->descriptionWithCalendarFormat(&_amPMTimeFormat)]];
	[_monthField setStringValue:[NSString stringReferenceFromLString:date->descriptionWithCalendarFormat(&_shortMonthFormat)]];
	[_hoursField setStringValue:[NSString stringReferenceFromLString:date->descriptionWithCalendarFormat(
		uses24 ? &_24TimeFormatHourRepresenation : &_12TimeFormatHourRepresenation)]];
	if (!_currentField)
		return;
	int value;
	if (_currentField == _monthField)
		value = date->month();
	else if (_currentField == _amPMField)
		value = ![[_currentField stringValue] isEqualToString:@"AM"];
	else
		value = [_currentField intValue];
	[_dateStepper setIntValue:value];
}

// The AM/PM field is shown with the 12-hour clock only, the fields before
// it moving over its half width.
- (void)_validateTimeFormat
{
	bool uses24 = Preferences::uses24HourClock();
	if (_currentField == _amPMField)
		[[_currentField window] endEditingFor:_currentField];
	[self _setupCustomTimeControls];
	[_amPMField setSelectable:!uses24];
	[_amPMField setEditable:!uses24];
	BOOL shown = [[_dateFieldsView subviews] containsObject:_amPMField];
	if (shown && uses24) {
		[_amPMField retain];
		[_amPMField removeFromSuperview];
		[_dateFieldsView wgShiftSibviewsByXOnDelta:_fieldsDeltaX];
	} else if (!shown && !uses24) {
		[_dateFieldsView wgShiftSibviewsByXOnDelta:-_fieldsDeltaX];
		[_dateFieldsView addSubview:_amPMField];
		[_amPMField release];
	}
	[_dateFieldsView setNeedsDisplay:YES];
}

- (void)_setupCellForField:(id)aField
{
	[aField setCell:[[[WGTextFieldCell alloc] initWithCell:[aField cell]] autorelease]];
}

- (void)_setupControls
{
	[self _setupCellForField:_yearField];
	[self _setupCellForField:_monthField];
	[self _setupCellForField:_dayField];
	[self _setupCellForField:_hoursField];
	[self _setupCellForField:_minutesField];
	[self _setupCellForField:_amPMField];
	[_enableButton setTitleColor:[NSColor whiteColor]];
	[_currentButton setImagePosition:NSImageOverlaps];
	[_currentButton setImage:[NSImage imageNamed:@"BlackButton_Left"] forPosition:0];
	[_currentButton setImage:[NSImage imageNamed:@"BlackButton_Center"] forPosition:1];
	[_currentButton setImage:[NSImage imageNamed:@"BlackButton_Right"] forPosition:2];
	[_currentButton setTitleColor:[NSColor whiteColor]];
	[_dateFieldsView setBorderThickness:1.125];
	[_timeLineControl setTag:kTimeLineTag];
}

// A slider with tick marks has the round knob.
- (void)_setupCustomSlider:(id)aSlider
{
	if (![aSlider numberOfTickMarks]) {
		[aSlider setKnobImage:[NSImage imageNamed:@"MWT_sliderknob"]];
		[aSlider setKnobDisabledImage:[NSImage imageNamed:@"MWT_sliderknob_disabl"]];
		[aSlider setKnobAlternateImage:[NSImage imageNamed:@"MWT_sliderknob_on"]];
	} else {
		[aSlider setKnobImage:[NSImage imageNamed:@"Knob_Normal"]];
		[aSlider setKnobDisabledImage:[NSImage imageNamed:@"Knob_Disable"]];
		[aSlider setKnobAlternateImage:[NSImage imageNamed:@"Knob_press"]];
	}
	[aSlider setBarFirstEdgeImage:[NSImage imageNamed:@"MWT_sliderback_left"]];
	[aSlider setBarMiddleImage:[NSImage imageNamed:@"MWT_sliderback_center"]];
	[aSlider setBarLastEdgeImage:[NSImage imageNamed:@"MWT_sliderback_right"]];
	[aSlider setBarDisabledFirstEdgeImage:[NSImage imageNamed:@"MWT_sliderback_left_disabl"]];
	[aSlider setBarDisabledMiddleImage:[NSImage imageNamed:@"MWT_sliderback_center_disabl"]];
	[aSlider setBarDisabledLastEdgeImage:[NSImage imageNamed:@"MWT_sliderback_right_disabl"]];
}

- (id)nibName
{
	return @"TimeLine";
}

- (void)awakeFromNib
{
	[super awakeFromNib];
	[self _setupCustomSlider:_playingSpeedSlider];
	_12timeFormatCaptions = [[NSArray alloc] initWithObjects:@"12:00 AM", @"12:00 PM", @"11:59 PM", nil];
	_24timeFormatCaptions = [[NSArray alloc] initWithObjects:@"00:00", @"12:00", @"23:59", nil];
	_fieldsDeltaX = [_amPMField frame].size.width * 0.5f;
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_textFieldDidStartEditing:)
		name:_kEditCellDidStartEditingNotification object:nil];
	[self _setupControls];
	LNotificationCenter::center()->addObserver(self, @selector(_validateTimeFormat), Preferences::kClockFormatWasChanged, 0);
	LNotificationCenter::center()->addObserver(self, @selector(_setupCustomTimeControls), TimeController::kWorldTimeWasChanged, 0);
	[self _setupCustomTimeControls];
	[self _validateControls];
	[self _validateTimeFormat];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	[_12timeFormatCaptions release];
	[_24timeFormatCaptions release];
	// The AM/PM field is retained while out of its superview.
	if (![[_dateFieldsView subviews] containsObject:_amPMField])
		[_amPMField release];
	[_timeLineControl setDateValue:0];
	[super dealloc];
}

- (void)showPalette
{
	[super showPalette];
	[self _validateControls];
}

- (id)autosaveName
{
	return [self className];
}

- (void)helpButton:(id)aButton
{
	[WGHelpBook gotoPage:@"topics/timeline.html" anchor:nil];
}

- (void)controlsAction:(id)aAction
{
	TimeController *controller = TimeController::instance();
	switch ([aAction tag]) {
	case kTimeLineTag:
		controller->setDate([_timeLineControl dateValue]);
		break;
	case kSpeedTag:
		controller->setPlayingSpeed([aAction floatValue]);
		break;
	case kPlayPauseTag:
		if (controller->isPlaying()) {
			controller->stop();
		} else {
			[[_currentField window] endEditingFor:_currentField];
			controller->play();
		}
		[self _validateControls];
		break;
	case kBackwardTag:
	case kForwardTag:
		controller->setPlayingDirection((int)[aAction tag] - kPlayPauseTag);
		[self _validateControls];
		break;
	case kEnableTag:
		controller->setUseCustomTime([aAction state] == NSControlStateValueOn);
		[self _validateControls];
		break;
	case kCurrentTag:
		controller->setDate(LDate::date());
		break;
	case kStepperTag:
		if (_currentField == _monthField) {
			LDate *month = LDate::dateWithTime(2000, [aAction intValue], 0, 0, 0, 1);
			[_monthField setStringValue:[NSString stringReferenceFromLString:month->descriptionWithCalendarFormat(&_shortMonthFormat)]];
		} else if (_currentField == _amPMField) {
			[_currentField setStringValue:[aAction intValue] ? @"PM" : @"AM"];
		} else {
			[_currentField setStringValue:[NSString stringWithFormat:@"%d", [aAction intValue]]];
		}
		controller->setDate([self _dateFromFields]);
		break;
	}
}

// Closing the palette goes back to the current time.
- (void)windowWillClose:(id)aClose
{
	[super windowWillClose:aClose];
	TimeController::instance()->setUseCustomTime(NO);
	[self _validateControls];
}

// Playing pauses while the time line is dragged.
- (void)timeDidEndEditing:(id)aEditing
{
	if (_isPlaying)
		TimeController::instance()->play();
}

- (void)timeDidBeginEditing:(id)aEditing
{
	TimeController *controller = TimeController::instance();
	_isPlaying = controller->isPlaying();
	controller->stop();
}

- (void)controlTextDidEndEditing:(id)aEditing
{
	TimeController::instance()->setDate([self _dateFromFields]);
	[_dateStepper setEnabled:NO];
	_currentField = nil;
}

- (void)controlTextDidChange:(id)aChange
{
	TimeController::instance()->stop();
	[_playPauseButton setState:NSControlStateValueOff];
}

// The stepper takes the range and value of the field being edited.
- (void)_textFieldDidStartEditing:(id)aEditing
{
	_currentField = [aEditing object];
	[_dateStepper setEnabled:YES];
	if (_currentField == _yearField) {
		[_dateStepper setMinValue:1900.0];
		[_dateStepper setMaxValue:2100.0];
		[_dateStepper setIntValue:[_yearField intValue]];
	} else if (_currentField == _monthField) {
		[_dateStepper setMinValue:1.0];
		[_dateStepper setMaxValue:12.0];
		[_dateStepper setIntValue:TimeController::instance()->date()->month()];
	} else if (_currentField == _dayField) {
		LDate *date = TimeController::instance()->date();
		int days = TimeController::daysCountInMonthOfYear(date->month(), date->year());
		[_dateStepper setMinValue:1.0];
		[_dateStepper setMaxValue:days];
		[_dateStepper setIntValue:[_dayField intValue]];
	} else if (_currentField == _hoursField) {
		[_dateStepper setMinValue:0.0];
		[_dateStepper setMaxValue:23.0];
		[_dateStepper setIntValue:[_hoursField intValue]];
	} else if (_currentField == _minutesField) {
		[_dateStepper setMinValue:0.0];
		[_dateStepper setMaxValue:59.0];
		[_dateStepper setIntValue:[_minutesField intValue]];
	} else if (_currentField == _amPMField) {
		[_dateStepper setMinValue:0.0];
		[_dateStepper setMaxValue:1.0];
		[_dateStepper setIntValue:![[_amPMField stringValue] isEqualToString:@"AM"]];
	}
}

// The fields' date, or the current one when they make none (a day past
// the month's end is its last day).
- (LDate *)_dateFromFields
{
	int year = [_yearField intValue];
	if ((unsigned)(year - 1900) <= 200) {
		NSString *monthString = [_monthField stringValue];
		NSCalendarDate *monthDate = [NSCalendarDate dateWithString:monthString calendarFormat:@"%B"];
		if (!monthDate)
			monthDate = [NSCalendarDate dateWithString:monthString calendarFormat:@"%b"];
		if (monthDate) {
			int month = (int)[monthDate monthOfYear];
			int day = [_dayField intValue];
			int days = TimeController::daysCountInMonthOfYear(month, year);
			if (day > days && day < 32)
				day = days;
			if (day > 0 && day <= days) {
				int hour = -1;
				if (Preferences::uses24HourClock()) {
					hour = [_hoursField intValue];
				} else {
					NSCalendarDate *hourDate = [NSCalendarDate dateWithString:[NSString stringWithFormat:@"%@ %@",
						[_hoursField stringValue], [_amPMField stringValue]] calendarFormat:@"%I %p"];
					if (hourDate)
						hour = (int)[hourDate hourOfDay];
				}
				int minute = [_minutesField intValue];
				if ((unsigned)hour < 24 && (unsigned)minute < 60)
					return LDate::dateWithTime(year, month, day, hour, minute, 0);
			}
		}
	}
	return TimeController::instance()->date();
}

// The controls work with the custom time only.
- (void)_validateControls
{
	TimeController *controller = TimeController::instance();
	BOOL custom = controller->useCustomTime();
	if (!custom && _currentField)
		[[_currentField window] endEditingFor:_currentField];
	[_enableButton setState:custom];
	[_dateStepper setEnabled:_currentField != nil];
	[_currentButton setEnabled:custom];
	[_timeLineControl setEnabled:custom];
	[_monthField setEnabled:custom];
	[_dayField setEnabled:custom];
	[_yearField setEnabled:custom];
	[_hoursField setEnabled:custom];
	[_minutesField setEnabled:custom];
	[_amPMField setEnabled:custom];
	[_forwardButton setEnabled:custom];
	[_backwardButton setEnabled:custom];
	[_playPauseButton setEnabled:custom];
	[_playingSpeedSlider setEnabled:custom];
	[_playingSpeedSlider setFloatValue:controller->playingSpeed()];
	BOOL playing = controller->isPlaying();
	[_playPauseButton setState:playing];
	[_forwardButton setState:controller->playingDirection() >= 0];
	[_backwardButton setState:controller->playingDirection() < 0];
	[_playPauseButton setImage:[NSImage imageNamed:playing ? @"Button_Pause_Press" : @"Button_Play_Normal"]];
	[_playPauseButton setAlternateImage:[NSImage imageNamed:playing ? @"Button_Pause_Normal" : @"Button_Play_Press"]];
}

@end
