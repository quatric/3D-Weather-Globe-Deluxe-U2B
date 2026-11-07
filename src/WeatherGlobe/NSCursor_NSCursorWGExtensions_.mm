#import "NSCursor+NSCursorWGExtensions.h"

@implementation NSCursor (NSCursorWGExtensions)

+ (id)_cursorWithImageName:(id)aName hotSpot:(NSPoint)aSpot
{
	NSImage *image = [NSImage imageNamed:aName];
	return image ? [[[NSCursor alloc] initWithImage:image hotSpot:aSpot] autorelease] : nil;
}

+ (id)pointingHandBigCursor
{
	return [self _cursorWithImageName:@"hand_big" hotSpot:NSMakePoint(10, 0)];
}

+ (id)coordPickerCursor
{
	return [self _cursorWithImageName:@"coord_picker" hotSpot:NSMakePoint(15, 15)];
}

+ (id)pushPinCursor
{
	return [self _cursorWithImageName:@"pushpin" hotSpot:NSMakePoint(3, 15)];
}

+ (id)zoomOutCursor
{
	return [self _cursorWithImageName:@"zoomout" hotSpot:NSMakePoint(6, 6)];
}

+ (id)zoomInCursor
{
	return [self _cursorWithImageName:@"zoomin" hotSpot:NSMakePoint(6, 6)];
}

@end
