// RoutePointData
#import "AtlasRoute.h"

@implementation RoutePointData

- (BOOL)isBreakHere
{
	return _isBreakHere;
}

- (void)setIsBreakHere:(BOOL)aBreak
{
	_isBreakHere = aBreak;
}

- (BOOL)isHidden
{
	return _isHidden;
}

- (void)setIsHidden:(BOOL)aHidden
{
	_isHidden = aHidden;
}

- (NSPoint)point
{
	return _pt;
}

- (void)setPoint:(NSPoint)aPoint
{
	_pt = aPoint;
}

@end
