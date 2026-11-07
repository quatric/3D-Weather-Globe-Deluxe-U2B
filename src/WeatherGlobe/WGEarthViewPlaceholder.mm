#import "WGEarthViewPlaceholder.h"

@implementation WGEarthViewPlaceholder

// The earth view keeps half its width as its height, at the top; the
// details view and the separator below take up the difference.
- (void)resizeSubviewsWithOldSize:(NSSize)aSize
{
	[super resizeSubviewsWithOldSize:aSize];
	NSRect frame = [_earthView frame];
	float height = floorf(frame.size.width * 0.5f);
	if (height == frame.size.height)
		return;
	[_earthView setFrameSize:NSMakeSize(frame.size.width, height)];
	float delta = frame.size.height - [_earthView frame].size.height;
	[_earthView setFrameOrigin:NSMakePoint([_earthView frame].origin.x, [_earthView frame].origin.y + delta)];
	[_detailsView setFrameSize:NSMakeSize([_detailsView frame].size.width, [_detailsView frame].size.height + delta)];
	[_separator setFrameOrigin:NSMakePoint([_separator frame].origin.x, [_separator frame].origin.y + delta)];
}

@end
