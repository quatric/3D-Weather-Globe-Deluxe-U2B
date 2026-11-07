#import "WGNewLocationController.h"

@implementation NSMenuItem (WGTimeZoneNamesComparsion)

// Orders time zone items west to east; items of the same side of GMT keep
// the titles' order east of it, the farther first west of it.
- (NSInteger)timeZoneComparisonFunc:(id)aFunc
{
	if (![aFunc representedObject] || ![self representedObject])
		return NSOrderedSame;
	LTimeZone *zone = lobj_cast<LTimeZone>([[self representedObject] getLObject]);
	LTimeZone *other = lobj_cast<LTimeZone>([[aFunc representedObject] getLObject]);
	if (zone && other) {
		int offset = zone->secondsFromGMT();
		int otherOffset = other->secondsFromGMT();
		if (sign(offset) != sign(otherOffset))
			return offset < otherOffset ? NSOrderedAscending : NSOrderedDescending;
		if (offset < 0) {
			int otherMagnitude = otherOffset < 0 ? -otherOffset : otherOffset;
			if (-otherMagnitude != offset && otherMagnitude <= -offset)
				return NSOrderedAscending;
			return NSOrderedDescending;
		}
	}
	return [[self title] compare:[aFunc title]];
}

@end
