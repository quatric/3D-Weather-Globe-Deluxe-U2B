#import "NSColor+WGExtensions.h"

@implementation NSColor (WGExtensions)

- (Color)wgColor
{
	NSColor *color = [self colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
	return Color([color redComponent], [color greenComponent], [color blueComponent], [color alphaComponent]);
}

+ (id)colorWithWGColor:(Color)aColor
{
	return [NSColor colorWithCalibratedRed:aColor.r green:aColor.g blue:aColor.b alpha:aColor.a];
}

// [r, g, b, a] as LValues.
- (LArray *)wgColorComponents
{
	NSColor *color = [self colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
	LValue *alpha = LValue::valueWithFloat([color alphaComponent]);
	LValue *blue = LValue::valueWithFloat([color blueComponent]);
	LValue *green = LValue::valueWithFloat([color greenComponent]);
	LValue *red = LValue::valueWithFloat([color redComponent]);
	return LArray::arrayWithObjects(red, green, blue, alpha, (LObject *)0);
}

@end
