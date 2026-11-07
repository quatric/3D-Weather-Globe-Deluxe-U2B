#import "NSBezierPath+SMKControlBorders.h"

@implementation NSBezierPath (SMKControlBorders)

+ (id)smkRoundedBorderPathWithRect:(NSRect)aRect cornerRadius:(CGFloat)aRadius
{
	NSRect inner = NSInsetRect(aRect, aRadius, aRadius);
	NSBezierPath *path = [NSBezierPath bezierPath];
	[path appendBezierPathWithArcWithCenter:NSMakePoint(NSMinX(inner), NSMinY(inner)) radius:aRadius
		startAngle:180.0 endAngle:270.0 clockwise:NO];
	[path appendBezierPathWithArcWithCenter:NSMakePoint(NSMaxX(inner), NSMinY(inner)) radius:aRadius
		startAngle:270.0 endAngle:360.0 clockwise:NO];
	[path appendBezierPathWithArcWithCenter:NSMakePoint(NSMaxX(inner), NSMaxY(inner)) radius:aRadius
		startAngle:0.0 endAngle:90.0 clockwise:NO];
	[path appendBezierPathWithArcWithCenter:NSMakePoint(NSMinX(inner), NSMaxY(inner)) radius:aRadius
		startAngle:90.0 endAngle:180.0 clockwise:NO];
	[path closePath];
	return path;
}

// A pill around aRect, extended by half its height on the capped sides when
// aRect2 is set; aBounds gets the path's rect.
+ (id)smkRolloverStyleBorderPathWithRect:(NSRect)aRect aroundRect:(BOOL)aRect2 rightCap:(BOOL)aCap leftCap:(BOOL)aCap2 bounds:(NSRect *)aBounds
{
	NSRect rect = aRect;
	float radius = floorf((float)aRect.size.height * 0.5f);
	if (aRect2) {
		if (aCap && aCap2) {
			rect = NSInsetRect(aRect, -radius, 0.0);
		}
		else if (aCap) {
			rect.size.width += radius;
		}
		else if (aCap2) {
			rect.size.width += radius;
			rect.origin.x -= radius;
		}
	}
	if (aBounds != NULL) {
		*aBounds = rect;
	}
	return [self smkRoundedBorderPathWithRect:rect cornerRadius:radius minXminY:aCap2 minXmaxY:aCap2
		maxXminY:aCap maxXmaxY:aCap];
}

+ (id)smkRolloverStyleBorderPathAroundRect:(NSRect)aRect bounds:(NSRect *)aBounds
{
	float radius = floorf((float)aRect.size.height * 0.5f);
	NSRect rect = NSInsetRect(aRect, -radius, 0.0);
	if (aBounds != NULL) {
		*aBounds = rect;
	}
	return [self smkRoundedBorderPathWithRect:rect cornerRadius:radius];
}

+ (id)smkRolloverStyleBorderPathWithRect:(NSRect)aRect
{
	return [self smkRoundedBorderPathWithRect:aRect cornerRadius:floorf((float)aRect.size.height * 0.5f)];
}

+ (id)smkFinderSelectionStyleBorderPathWithRect:(NSRect)aRect
{
	NSBezierPath *path = [self smkRoundedBorderPathWithRect:aRect cornerRadius:4.0];
	[path setLineWidth:2.0];
	return path;
}

// a rect with only the chosen corners rounded
+ (id)smkRoundedBorderPathWithRect:(NSRect)aRect cornerRadius:(CGFloat)aRadius minXminY:(BOOL)aY minXmaxY:(BOOL)aY2 maxXminY:(BOOL)aY3 maxXmaxY:(BOOL)aY4
{
	NSRect inner = NSInsetRect(aRect, aRadius, aRadius);
	NSBezierPath *path = [NSBezierPath bezierPath];
	if (aY) {
		[path appendBezierPathWithArcWithCenter:NSMakePoint(NSMinX(inner), NSMinY(inner)) radius:aRadius
			startAngle:180.0 endAngle:270.0 clockwise:NO];
	}
	else {
		[path moveToPoint:NSMakePoint(NSMinX(aRect), NSMinY(aRect))];
		[path lineToPoint:NSMakePoint(NSMaxX(inner), NSMinY(aRect))];
	}
	if (aY3) {
		[path appendBezierPathWithArcWithCenter:NSMakePoint(NSMaxX(inner), NSMinY(inner)) radius:aRadius
			startAngle:270.0 endAngle:360.0 clockwise:NO];
	}
	else {
		[path lineToPoint:NSMakePoint(NSMaxX(aRect), NSMinY(aRect))];
	}
	if (aY4) {
		[path appendBezierPathWithArcWithCenter:NSMakePoint(NSMaxX(inner), NSMaxY(inner)) radius:aRadius
			startAngle:0.0 endAngle:90.0 clockwise:NO];
	}
	else {
		[path lineToPoint:NSMakePoint(NSMaxX(aRect), NSMaxY(aRect))];
	}
	if (aY2) {
		[path appendBezierPathWithArcWithCenter:NSMakePoint(NSMinX(inner), NSMaxY(inner)) radius:aRadius
			startAngle:90.0 endAngle:180.0 clockwise:NO];
	}
	else {
		[path lineToPoint:NSMakePoint(NSMinX(aRect), NSMaxY(aRect))];
	}
	[path closePath];
	return path;
}

@end
