#import "WGMountableController.h"

// A nib-owned pane that a container mounts.
@implementation WGMountableController

- (id)pane
{
	if ([self loadNibIfNeeded])
		[pane setPostsFrameChangedNotifications:YES];
	return pane;
}

// The window's first responder is inside the pane.
- (BOOL)isActive
{
	id responder = [[pane window] firstResponder];
	return [responder isKindOfClass:[NSView class]] && [responder isDescendantOf:pane];
}

- (void)dealloc
{
	[pane release];
	[super dealloc];
}

- (void)didMounted:(BOOL)aMounted
{
	_isMounted = aMounted;
}

- (BOOL)isMounted
{
	return _isMounted;
}

- (id)container
{
	return _container;
}

- (void)setContainer:(id)aContainer
{
	_container = aContainer;
}

@end
