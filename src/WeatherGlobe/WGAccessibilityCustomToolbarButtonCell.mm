#import "WGCustomToolbarButtonCell.h"

@implementation WGAccessibilityCustomToolbarButtonCell

// Stands for the target button: its parent is the button, which also
// answers about its size and position.
- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityParentAttribute])
		return [self target];
	if ([aValue isEqualToString:NSAccessibilitySizeAttribute] || [aValue isEqualToString:NSAccessibilityPositionAttribute])
		return [[self target] accessibilityAttributeValue:aValue];
	return [super accessibilityAttributeValue:aValue];
}

- (void)accessibilityPerformAction:(id)aAction
{
	if ([aAction isEqualToString:NSAccessibilityPressAction])
		[[self target] performClick:nil];
}

- (NSInteger)state
{
	return [[self target] state];
}

@end
