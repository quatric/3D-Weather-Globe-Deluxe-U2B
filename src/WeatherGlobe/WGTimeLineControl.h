#import "WeatherGlobePrefix.h"


@interface WGTimeLineControl : NSControl
{
}
+ (Class)cellClass;
- (BOOL)acceptsFirstMouse:(id)aMouse;
- (void)setDateValue:(LDate *)aValue;
- (LDate *)dateValue;
@end

@interface WGTimeLineCell : NSActionCell
{
    LDate *_dateVal;
    float _topKnobOffset;
    double _topKnobStartTime;
    float _bottomKnobOffset;
    double _bottomKnobStartTime;
    float _slidingSpeed;
    float _slidingOffset;
    double _slidingPrevTime;
    NSTimer *_animationTimer;
    unsigned int _autoUpdateFlags;
    int _trackingObject;
    LDate *_trackingStartDate;
    LDate *_scrollToDate;
    double _scrollToDateStartTime;
    NSTextFieldCell *_hourTextCell;
    NSTextFieldCell *_monthTextCell;
    NSImage *_eclipseImage;
    NSImage *_topKnobImage;
    NSImage *_topKnobPressImage;
    NSImage *_bottomKnobImage;
    NSImage *_bottomKnobPressImage;
    NSImage *_windowLeftImage;
    NSImage *_windowCenterImage;
    NSImage *_windowRightImage;
    NSImage *_topBackgroundImage1;
    NSImage *_topBackgroundImage2;
    NSImage *_bottomBackgroundImage;
    NSImage * _topBackgroundImages[12];
}
+ (BOOL)prefersTrackingUntilMouseUp;
- (void)stopTracking:(NSPoint)aTracking at:(NSPoint)aAt inView:(id)aView mouseIsUp:(BOOL)aUp;
- (BOOL)continueTracking:(NSPoint)aTracking at:(NSPoint)aAt inView:(id)aView;
- (BOOL)startTrackingAt:(NSPoint)aAt inView:(id)aView;
- (void)_onTimer:(id)aTimer;
- (void)_moveSlider:(BOOL)aSlider;
- (void)_setupTimer;
- (void)_timeDidEndEditing;
- (void)_timeDidBeginEditing;
- (void)_sendDateAction;
- (LDate *)_solarEclipseDateFromPoint:(NSPoint)aPoint forCellFrame:(NSRect)aFrame;
- (LDate *)_dateFromPoint:(NSPoint)aPoint forCellFrame:(NSRect)aFrame isTopWheel:(BOOL *)aWheel;
- (id)view:(id)aView stringForToolTip:(NSInteger)aTip point:(NSPoint)aPoint userData:(void *)aData;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)_drawBottomSliderWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)_drawTopSliderWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)_drawSolarEclipseWithFrame:(NSRect)aFrame doHighlight:(BOOL)aHighlight;
- (void)_drawHours:(NSInteger)aHours withFrame:(NSRect)aFrame inView:(id)aView;
- (void)_drawMonthString:(id)aString withFrame:(NSRect)aFrame inView:(id)aView;
- (void)_drawKnob:(BOOL)aKnob withFrame:(NSRect)aFrame;
- (void)_drawFrameWithFrame:(NSRect)aFrame;
- (NSRect)_sliderRect:(BOOL)aRect forCellFrame:(NSRect)aFrame;
- (NSRect)_frameRectForCellFrame:(NSRect)aFrame;
- (NSRect)_knobRect:(BOOL)aRect forCellFrame:(NSRect)aFrame;
- (CGFloat)_currentTimePos:(BOOL)aPos forCellFrame:(NSRect)aFrame;
- (CGFloat)_normalizedKnobOffset:(BOOL)aOffset forCellFrame:(NSRect)aFrame;
- (NSSize)_solarEclipseSize;
- (NSSize)_knobSize:(BOOL)aSize;
- (id)_solarEclipseTooltipAtIndex:(NSUInteger)aIndex;
- (LDate *)_solarEclipseDateAtIndex:(NSUInteger)aIndex;
- (LDate *)_dateFromSolarEclipse:(SSolarEclipse *)aEclipse;
- (SSolarEclipse *)_solarEclipseAtIndex:(NSUInteger)aIndex;
- (NSUInteger)_indexOfNearestSolarEclipseForDate:(LDate *)aDate;
- (void)setDateValue:(LDate *)aValue;
- (LDate *)dateValue;
- (id)stringValue;
- (void)setDoubleValue:(double)aValue;
- (double)doubleValue;
- (void)setFloatValue:(float)aValue;
- (float)floatValue;
- (void)setIntValue:(int)aValue;
- (int)intValue;
- (void)dealloc;
- (id)init;
@end

