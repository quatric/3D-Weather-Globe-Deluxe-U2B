#import "WGCustomSlider.h"

@interface NSObject (WGCustomSliderTarget)
- (void)sliderWillStartTracking:(id)aSlider;
- (void)sliderDidEndTracking:(id)aSlider;
@end

// A slider drawn by WGCustomSliderCell from images; its target hears when
// tracking starts and ends.
@implementation WGCustomSlider

+ (void)initialize
{
	[self setCellClass:[WGCustomSliderCell class]];
}

// The nib's NSSliderCell is decoded as a WGCustomSliderCell.
- (id)initWithCoder:(id)aCoder
{
	NSKeyedUnarchiver *unarchiver = [aCoder isKindOfClass:[NSKeyedUnarchiver class]] ? aCoder : nil;
	[unarchiver setClass:[WGCustomSliderCell class] forClassName:NSStringFromClass([NSSliderCell class])];
	self = [super initWithCoder:aCoder];
	[unarchiver setClass:[NSSliderCell class] forClassName:NSStringFromClass([NSSliderCell class])];
	return self;
}

- (WGCustomSliderCell *)_customCell
{
	id cell = [self cell];
	return [cell isKindOfClass:[WGCustomSliderCell class]] ? cell : nil;
}

- (void)setKnobImage:(id)aImage
{
	[[self _customCell] setKnobImage:aImage];
}

- (void)setKnobDisabledImage:(id)aImage
{
	[[self _customCell] setKnobDisabledImage:aImage];
}

- (void)setKnobAlternateImage:(id)aImage
{
	[[self _customCell] setKnobAlternateImage:aImage];
}

- (void)setBarFirstEdgeImage:(id)aImage
{
	[[self _customCell] setBarFirstEdgeImage:aImage];
}

- (void)setBarLastEdgeImage:(id)aImage
{
	[[self _customCell] setBarLastEdgeImage:aImage];
}

- (void)setBarMiddleImage:(id)aImage
{
	[[self _customCell] setBarMiddleImage:aImage];
}

- (void)setBarDisabledFirstEdgeImage:(id)aImage
{
	[[self _customCell] setBarDisabledFirstEdgeImage:aImage];
}

- (void)setBarDisabledLastEdgeImage:(id)aImage
{
	[[self _customCell] setBarDisabledLastEdgeImage:aImage];
}

- (void)setBarDisabledMiddleImage:(id)aImage
{
	[[self _customCell] setBarDisabledMiddleImage:aImage];
}

- (void)setDrawsTickMarks:(BOOL)aMarks
{
	[[self _customCell] setDrawsTickMarks:aMarks];
}

- (void)setTickMarksColor:(id)aColor
{
	[[self _customCell] setTickMarksColor:aColor];
}

// NSSlider tracks inside mouseDown:.
- (void)mouseDown:(id)aDown
{
	if ([[self target] respondsToSelector:@selector(sliderWillStartTracking:)])
		[[self target] sliderWillStartTracking:self];
	[super mouseDown:aDown];
	if ([[self target] respondsToSelector:@selector(sliderDidEndTracking:)])
		[[self target] sliderDidEndTracking:self];
}

@end
