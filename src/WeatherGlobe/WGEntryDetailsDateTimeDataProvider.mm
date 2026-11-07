#import "WGEntryDetailsDateTimeDataProvider.h"

@implementation WGEntryDetailsDateTimeDataProvider

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	[_currentDateTimeString release];
	[super dealloc];
}

// The date and time when last asked.
- (id)description
{
	if (!_currentDateTimeString)
		_currentDateTimeString = [[super description] retain];
	return _currentDateTimeString;
}

// Mounted, the time follows the clock format and the time adjustments.
- (void)didMountedToOwner:(id)aOwner
{
	LNotificationCenter::center()->removeObserver(self, Preferences::kClockFormatWasChanged, 0);
	[[NSNotificationCenter defaultCenter] removeObserver:self name:@"AdjustTimeNotification" object:nil];
	if (aOwner) {
		LNotificationCenter::center()->addObserver(self, @selector(_adjustTime), Preferences::kClockFormatWasChanged, 0);
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_adjustTime)
		                                             name:@"AdjustTimeNotification" object:nil];
	}
}

// A changed time is shown; the owner lays out again when its height
// changes.
- (void)_adjustTime
{
	NSString *description = [super description];
	if ([_currentDateTimeString isEqualToString:description])
		return;
	[_currentDateTimeString release];
	_currentDateTimeString = nil;
	NSRect bounds = [[self descriptionView] bounds];
	[[self descriptionView] setStringValue:[self description]];
	NSSize size = [[(id)_descriptionView cell] cellSizeForBounds:NSMakeRect(0, 0, bounds.size.width, WGEntryDetailsInfoDefaultFieldSize.height)];
	if (size.height == bounds.size.height) {
		[[self descriptionView] setNeedsDisplay:YES];
	} else {
		[[self descriptionView] setFrameSize:NSMakeSize(bounds.size.width, size.height)];
		[[self owner] rebuildLayout];
	}
}

@end
