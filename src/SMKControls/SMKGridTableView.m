#import "SMKGridTableView.h"

// The grid control's list view. By default it keeps fitting its text while
// the window resizes, by never reporting a live resize.
@implementation SMKGridTableView

- (void)setAutofitTextDuringLiveResize:(BOOL)aResize
{
	_doAutofitTextDuringLiveResize = aResize;
}

// several dragged rows get the grid control's multiple-items image
- (id)dragImageForRows:(id)aRows event:(id)aEvent dragImageOffset:(NSPoint *)aOffset
{
	if ([aRows count] < 2) {
		return [super dragImageForRows:aRows event:aEvent dragImageOffset:aOffset];
	}
	NSImage *image = [super dragImageForRows:aRows event:aEvent dragImageOffset:aOffset];
	image = [[self owningGridControl] imageForMultipleDraggedItemsForImage:image];
	*aOffset = NSZeroPoint;
	return image;
}

- (BOOL)inLiveResize
{
	return _doAutofitTextDuringLiveResize ? NO : [super inLiveResize];
}

- (void)viewDidEndLiveResize
{
	if (!_doAutofitTextDuringLiveResize) {
		[super viewDidEndLiveResize];
	}
}

- (void)viewWillStartLiveResize
{
	if (!_doAutofitTextDuringLiveResize) {
		[super viewWillStartLiveResize];
	}
}

- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self != nil) {
		[self _initVars];
	}
	return self;
}

- (id)initWithFrame:(NSRect)aFrame
{
	self = [super initWithFrame:aFrame];
	if (self != nil) {
		[self _initVars];
	}
	return self;
}

// The grid control around its scroll view.
- (id)owningGridControl
{
	id grid = [[self superview] superview];
	return [grid isKindOfClass:[SMKGridControl class]] ? grid : nil;
}

- (void)_initVars
{
	_doAutofitTextDuringLiveResize = YES;
}

@end
