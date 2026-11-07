#import "WGDragNDropView.h"

// Drops are validated and accepted by the delegate.
@implementation WGDragNDropView

- (id)initWithFrame:(NSRect)aFrame
{
	return [super initWithFrame:aFrame];
}

- (id)delegate
{
	return delegate;
}

- (void)setDelegate:(id)aDelegate
{
	delegate = aDelegate;
}

- (NSUInteger)draggingEntered:(id)aEntered
{
	return [delegate view:self validateDrop:aEntered];
}

- (NSUInteger)draggingUpdated:(id)aUpdated
{
	return [delegate view:self validateDrop:aUpdated];
}

- (BOOL)performDragOperation:(id)aOperation
{
	return [delegate view:self acceptDrop:aOperation];
}

@end
