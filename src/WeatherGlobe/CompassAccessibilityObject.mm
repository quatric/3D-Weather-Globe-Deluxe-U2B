#import "WGCompassButton.h"

// One zone of the compass, exposed to accessibility as a button.
@implementation CompassAccessibilityObject

- (id)initWithCell:(id)aCell frameRect:(NSRect)aRect hotZone:(NSInteger)aZone
{
	self = [super init];
	if (self) {
		_frameRect = aRect;
		_cell = aCell;
		_hotZone = (int)aZone;
	}
	return self;
}

- (int)intValue
{
	return _hotZone;
}

- (id)accessibilityAttributeNames
{
	return [NSArray arrayWithObjects:NSAccessibilityRoleAttribute, NSAccessibilityHelpAttribute,
		NSAccessibilityTitleAttribute, NSAccessibilityPositionAttribute, NSAccessibilitySizeAttribute,
		NSAccessibilityParentAttribute, NSAccessibilityRoleDescriptionAttribute, nil];
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityRoleAttribute])
		return NSAccessibilityButtonRole;
	if ([aValue isEqualToString:NSAccessibilityRoleDescriptionAttribute])
		return [[NSBundle mainBundle] localizedStringForKey:@"Button" value:@"" table:nil];
	if ([aValue isEqualToString:NSAccessibilityParentAttribute])
		return _cell;
	if ([aValue isEqualToString:NSAccessibilityTitleAttribute]
		|| [aValue isEqualToString:NSAccessibilityHelpAttribute])
		return [_cell _helpStringForZone:_hotZone];
	if ([aValue isEqualToString:NSAccessibilityPositionAttribute]) {
		NSView *view = [_cell controlView];
		NSPoint point = [view convertPoint:_frameRect.origin toView:nil];
		NSRect screen = [[view window] convertRectToScreen:NSMakeRect(point.x, point.y, 0.0, 0.0)];
		return [NSValue valueWithPoint:screen.origin];
	}
	if ([aValue isEqualToString:NSAccessibilitySizeAttribute])
		return [NSValue valueWithSize:_frameRect.size];
	return nil;
}

- (BOOL)accessibilityIsAttributeSettable:(id)aSettable
{
	return NO;
}

- (id)accessibilityHitTest:(NSPoint)aTest
{
	return self;
}

- (BOOL)accessibilityIsIgnored
{
	return NO;
}

- (id)accessibilityActionNames
{
	return [NSArray arrayWithObjects:NSAccessibilityPressAction, nil];
}

- (id)accessibilityActionDescription:(id)aDescription
{
	if ([aDescription isEqualToString:NSAccessibilityPressAction])
		return [[NSBundle mainBundle] localizedStringForKey:@"Press" value:@"" table:nil];
	return nil;
}

- (void)accessibilityPerformAction:(id)aAction
{
	if ([aAction isEqualToString:NSAccessibilityPressAction])
		[_cell performClick:aAction];
}

- (id)accessibilityFocusedUIElement
{
	return NSAccessibilityUnignoredAncestor(self);
}

@end
