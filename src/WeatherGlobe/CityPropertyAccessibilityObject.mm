#import "CityPropertyAccessibilityObject.h"

// A static text element for a rectangle of the parent view.
@implementation CityPropertyAccessibilityObject

- (id)initWithParentView:(id)aView frameRect:(NSRect)aRect description:(id)aDescription
{
	self = [super init];
	if (self) {
		_frameRect = aRect;
		_parentView = aView;
		_description = [aDescription retain];
	}
	return self;
}

- (void)dealloc
{
	[_description release];
	[super dealloc];
}

- (id)accessibilityAttributeNames
{
	return [NSArray arrayWithObjects:NSAccessibilityRoleAttribute, NSAccessibilityPositionAttribute,
	                                 NSAccessibilitySizeAttribute, NSAccessibilityParentAttribute,
	                                 NSAccessibilityValueAttribute, nil];
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityRoleAttribute])
		return NSAccessibilityStaticTextRole;
	if ([aValue isEqualToString:NSAccessibilityParentAttribute])
		return _parentView;
	if ([aValue isEqualToString:NSAccessibilityPositionAttribute]) {
		NSPoint point = [_parentView convertPoint:_frameRect.origin toView:nil];
		NSRect screen = [[_parentView window] convertRectToScreen:NSMakeRect(point.x, point.y, 0, 0)];
		return [NSValue valueWithPoint:screen.origin];
	}
	if ([aValue isEqualToString:NSAccessibilitySizeAttribute])
		return [NSValue valueWithSize:_frameRect.size];
	if ([aValue isEqualToString:NSAccessibilityValueAttribute])
		return _description;
	return nil;
}

- (BOOL)accessibilityIsAttributeSettable:(id)aSettable
{
	return NO;
}

- (id)accessibilityActionNames
{
	return nil;
}

- (BOOL)accessibilityIsIgnored
{
	return NO;
}

@end
