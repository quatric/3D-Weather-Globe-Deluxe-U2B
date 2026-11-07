#import "WGCompassButton.h"

// The navigation compass: a button whose WGCompassCell has zones.
@implementation WGCompassButton

+ (void)initialize
{
	if (self == [WGCompassButton class])
		[self setCellClass:[WGCompassCell class]];
}

+ (Class)cellClass
{
	return [WGCompassCell class];
}

// The nib's cell is replaced, keeping its target and action.
- (id)initWithCoder:(id)aCoder
{
	self = [super initWithCoder:aCoder];
	if (self) {
		NSCell *oldCell = [[self cell] retain];
		WGCompassCell *cell = [[WGCompassCell allocWithZone:[self zone]] init];
		[self setCell:cell];
		[cell setTarget:[oldCell target]];
		[cell setAction:[oldCell action]];
		[oldCell release];
		[cell release];
	}
	return self;
}

- (void)resetToolTipRects
{
	if (![self window])
		return;
	id cell = [self cell];
	if ([cell respondsToSelector:@selector(addToolTipRectsToView:)]) {
		[self removeAllToolTips];
		[cell addToolTipRectsToView:self];
	}
}

- (NSInteger)currentZone
{
	return [[self cell] currentZone];
}

- (void)setCurrentZone:(NSInteger)aZone
{
	[[self cell] setCurrentZone:aZone];
}

- (BOOL)isCenterZoneActive
{
	return [[self cell] isCenterZoneActive];
}

- (void)setCenterZoneActive:(BOOL)aActive
{
	[[self cell] setCenterZoneActive:aActive];
}

- (void)viewWillMoveToWindow:(id)aWindow
{
	if ([self window])
		[self removeAllToolTips];
	[super viewWillMoveToWindow:aWindow];
}

- (void)viewDidMoveToWindow
{
	[super viewDidMoveToWindow];
	[self resetToolTipRects];
}

@end
