#import "ImageViewAccessibilityIgnored.h"

// Invisible to accessibility: everything goes to the first unignored
// ancestor.
@implementation ImageViewAccessibilityIgnored

- (id)accessibilityAttributeNames
{
	return [super accessibilityAttributeNames];
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityChildrenAttribute])
		return nil;
	if ([aValue isEqualToString:NSAccessibilityParentAttribute])
		return NSAccessibilityUnignoredAncestor([self superview]);
	return [super accessibilityAttributeValue:aValue];
}

- (BOOL)accessibilityIsAttributeSettable:(id)aSettable
{
	return NO;
}

- (id)accessibilityHitTest:(NSPoint)aTest
{
	return NSAccessibilityUnignoredAncestor([self superview]);
}

- (BOOL)accessibilityIsIgnored
{
	return YES;
}

- (id)accessibilityFocusedUIElement
{
	return NSAccessibilityUnignoredAncestor([self superview]);
}

@end
