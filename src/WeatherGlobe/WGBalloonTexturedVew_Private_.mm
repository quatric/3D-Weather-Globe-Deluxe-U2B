#import "WGBalloonTexturedVew.h"

@implementation WGBalloonTexturedVew (Private)

// The balloon image's place: at the frame's corner or next to it, inside
// the corner textures, for each of the eight positions.
- (NSRect)_balloonFrame
{
	NSPoint origin = NSZeroPoint;
	NSSize size = [_balloonImage size];
	NSRect bounds = [self bounds];
	NSSize texture;
	switch (_balloonPosition) {
	case 0:
		texture = [_textures[0] size];
		origin = NSMakePoint(NSMinX(bounds) + texture.width, NSMaxY(bounds) - size.height);
		break;
	case 1:
		texture = [_textures[2] size];
		origin = NSMakePoint(NSMaxX(bounds) - texture.width - size.width, NSMaxY(bounds) - size.height);
		break;
	case 2:
		texture = [_textures[2] size];
		origin = NSMakePoint(NSMaxX(bounds) - size.width, NSMaxY(bounds) - texture.height - size.height);
		break;
	case 3:
		texture = [_textures[8] size];
		origin = NSMakePoint(NSMaxX(bounds) - size.width, NSMinY(bounds) + texture.height);
		break;
	case 4:
		texture = [_textures[8] size];
		origin = NSMakePoint(NSMaxX(bounds) - texture.width - size.width, NSMinY(bounds) + 1.0f);
		break;
	case 5:
		texture = [_textures[6] size];
		origin = NSMakePoint(NSMinX(bounds) + texture.width, NSMinY(bounds) + 1.0f);
		break;
	case 6:
		texture = [_textures[6] size];
		origin = NSMakePoint(NSMinX(bounds), NSMinY(bounds) + texture.height);
		break;
	case 7:
		texture = [_textures[0] size];
		origin = NSMakePoint(NSMinX(bounds), NSMaxY(bounds) - texture.height - size.height);
		break;
	}
	return NSMakeRect(origin.x, origin.y, size.width, size.height);
}

@end
