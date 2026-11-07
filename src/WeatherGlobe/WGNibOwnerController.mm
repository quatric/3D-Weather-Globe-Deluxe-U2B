#import "WGNibOwnerController.h"

@implementation WGNibOwnerController

- (id)nibName
{
	return @"";
}

// Loads the nib the first time; YES then.
- (BOOL)loadNibIfNeeded
{
	if (_nibWasLoaded)
		return NO;
	_nibWasLoaded = YES;
	[NSBundle loadNibNamed:[self nibName] owner:self];
	return YES;
}

- (id)delegate
{
	return delegate;
}

- (void)setDelegate:(id)aDelegate
{
	delegate = aDelegate;
}

@end
