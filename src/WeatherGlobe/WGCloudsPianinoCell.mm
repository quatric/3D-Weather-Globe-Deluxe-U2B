#import "WGCloudsPianino.h"

@implementation WGCloudsPianinoCell

- (void)dealloc
{
	[_cell release];
	[super dealloc];
}

// A small white centered title, if it fits.
- (void)_drawTitle:(id)aTitle withFrame:(NSRect)aFrame inView:(id)aView
{
	if (!aTitle)
		return;
	if (!_cell) {
		_cell = [NSTextFieldCell new];
		[_cell setTextColor:[NSColor whiteColor]];
		[_cell setAlignment:NSTextAlignmentCenter];
		[_cell setFont:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSControlSizeMini]]];
	}
	[_cell setStringValue:aTitle];
	if ([_cell cellSize].width <= aFrame.size.width)
		[_cell drawInteriorWithFrame:aFrame inView:aView];
}

// The pianino's keys in an 8 pixel strip at the top (the enabled ones
// lit), with the dates under them: a title for each run of keys of the
// same day, separated by ticks.
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView
{
	WGCloudsPianino *pianino = [aView isKindOfClass:[WGCloudsPianino class]] ? aView : nil;
	[[NSColor darkGrayColor] set];
	float stripY = NSMaxY(aFrame) - 8.0f;
	NSRect strip = NSMakeRect(aFrame.origin.x, stripY, aFrame.size.width, 8.0);
	NSFrameRect(strip);
	int count = (int)[pianino keysCount];
	NSRect keys = NSInsetRect(strip, 2.0, 2.0);
	float keyWidth = (keys.size.width - (float)(count - 1)) / (float)count;
	float keyX = keys.origin.x;
	float titleX = aFrame.origin.x;
	float titleWidth = 0.0f;
	float titleHeight = aFrame.size.height - 8.0f;
	NSString *title = nil;
	for (int i = 0; i < count; i++) {
		if ([pianino isEnabledKeyAtIndex:i]) {
			[[NSColor lightGrayColor] set];
			NSRectFill(NSMakeRect(keyX, keys.origin.y, keyWidth, keys.size.height));
		}
		NSString *keyTitle = [[pianino dateForKeyAtIndex:i] descriptionWithCalendarFormat:@"%b %e"];
		if (![title isEqualToString:keyTitle]) {
			if (i != 0) {
				[[NSColor grayColor] set];
				NSRectFill(NSMakeRect(keyX - 1.0f, stripY - 4.0f, 1.0, 4.0));
			}
			if (title) {
				[self _drawTitle:title withFrame:NSMakeRect(titleX, aFrame.origin.y, titleWidth, titleHeight) inView:aView];
				titleX += titleWidth;
				titleWidth = 0.0f;
			}
			title = keyTitle;
		}
		titleWidth += keyWidth + 1.0f;
		keyX += keyWidth + 1.0f;
	}
	[self _drawTitle:title withFrame:NSMakeRect(titleX, aFrame.origin.y, titleWidth, titleHeight) inView:aView];
}

@end
