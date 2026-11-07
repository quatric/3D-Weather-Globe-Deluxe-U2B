#import "WBHelpButton.h"

@implementation WBHelpButton

// Before Mac OS X 10.3 (no help bezel), a small round "?" button.
- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self) {
		static SInt32 currVersion = 0;
		if (currVersion == 0)
			Gestalt(gestaltSystemVersion, &currVersion);
		NSButtonCell *cell = [self cell];
		if (currVersion < 0x1030) {
			[self setButtonType:NSButtonTypeMomentaryPushIn];
			[self setBezelStyle:NSBezelStyleCircular];
			[cell setControlSize:NSControlSizeSmall];
			[self setImagePosition:NSNoImage];
			[self setAlignment:NSTextAlignmentCenter];
			[cell setFont:[NSFont boldSystemFontOfSize:14.0]];
			[self setTitle:@"?"];
			NSRect frame = [self frame];
			frame.size = NSMakeSize(25.0, 24.0);
			[self setFrame:frame];
		}
	}
	return self;
}

@end
