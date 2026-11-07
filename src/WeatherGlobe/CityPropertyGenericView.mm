#import "CityPropertyAccessibilityObject.h"

// One property line of a city in the city info palette; an accessibility
// group of the subclass's accessibilityObjects.
@implementation CityPropertyGenericView

- (id)initWithCityEntry:(PointEntry *)aEntry
{
	self = [super initWithFrame:NSMakeRect(0.0, 0.0, 1.0, 1.0)];
	if (self)
		_entry = aEntry ? (PointEntry *)aEntry->retain() : 0;
	return self;
}

- (void)dealloc
{
	if (_entry) {
		_entry->release();
		_entry = 0;
	}
	[super dealloc];
}

+ (id)_menuItemsDescriptors
{
	return nil;
}

// Menu items from the descriptors: TITLE ("-" for a separator), SUBMENU,
// SELECTOR and TAG; the class is the target.
+ (id)optionsMenuItems
{
	NSArray *descriptors = [self _menuItemsDescriptors];
	NSUInteger count = [descriptors count];
	NSMutableArray *items = [NSMutableArray array];
	for (NSUInteger i = 0; i < count; i++) {
		NSDictionary *descriptor = [descriptors objectAtIndex:i];
		NSString *title = [descriptor objectForKey:@"TITLE"];
		NSMenu *submenu = [descriptor objectForKey:@"SUBMENU"];
		SEL action = NSSelectorFromString([descriptor objectForKey:@"SELECTOR"]);
		int tag = [[descriptor objectForKey:@"TAG"] intValue];
		NSMenuItem *item;
		if ([title isEqual:@"-"]) {
			item = [NSMenuItem separatorItem];
		} else {
			item = [[NSMenuItem new] autorelease];
			[item setTitle:title];
			[item setTarget:self];
			[item setTag:tag];
			[item setAction:action];
			if (submenu)
				[item setSubmenu:submenu];
		}
		[items addObject:item];
	}
	return items;
}

- (BOOL)acceptsFirstMouse:(id)aMouse
{
	return YES;
}

- (BOOL)enabled
{
	return NO;
}

- (NSSize)sizeForBestFit
{
	return NSMakeSize(1.0, 1.0);
}

- (id)accessibilityObjects
{
	return nil;
}

- (id)accessibilityAttributeNames
{
	return [[super accessibilityAttributeNames] arrayByAddingObjectsFromArray:
		[NSArray arrayWithObjects:NSAccessibilityRoleAttribute, NSAccessibilityChildrenAttribute, nil]];
}

- (id)accessibilityAttributeValue:(id)aValue
{
	if ([aValue isEqualToString:NSAccessibilityChildrenAttribute])
		return [self accessibilityObjects];
	if ([aValue isEqualToString:NSAccessibilityRoleAttribute])
		return NSAccessibilityGroupRole;
	return [super accessibilityAttributeValue:aValue];
}

- (BOOL)accessibilityIsIgnored
{
	return NO;
}

- (BOOL)accessibilityIsAttributeSettable:(id)aSettable
{
	if ([aSettable isEqualToString:NSAccessibilityChildrenAttribute]
		|| [aSettable isEqualToString:NSAccessibilityRoleAttribute])
		return NO;
	return [super accessibilityIsAttributeSettable:aSettable];
}

@end
