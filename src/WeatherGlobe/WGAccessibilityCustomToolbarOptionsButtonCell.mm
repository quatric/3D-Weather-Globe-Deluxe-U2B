#import "WGCustomToolbarButtonCell.h"

@implementation WGAccessibilityCustomToolbarOptionsButtonCell

// The target's popup indicator is the button.
- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityParentAttribute])
		return [self target];
	if ([aValue isEqualToString:NSAccessibilityTitleAttribute])
		return [self title];
	if ([aValue isEqualToString:NSAccessibilitySizeAttribute])
		return [NSValue valueWithSize:[self cellSize]];
	if ([aValue isEqualToString:NSAccessibilityPositionAttribute]) {
		id target = [self target];
		NSView *view = [target controlView];
		NSRect rect = [target popupIndicatorImageRectForBounds:[target bounds]];
		NSPoint point = [view convertPoint:rect.origin toView:nil];
		point.y -= rect.size.height;
		return [NSValue valueWithPoint:[[view window] convertBaseToScreen:point]];
	}
	return [super accessibilityAttributeValue:aValue];
}

- (void)accessibilityPerformAction:(id)aAction
{
	if ([aAction isEqualToString:NSAccessibilityPressAction])
		[[self target] performSelector:[self action]];
}

@end
