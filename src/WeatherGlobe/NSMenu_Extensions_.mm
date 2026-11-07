#import "CIPaletteController.h"

@implementation NSMenu (Extensions)

- (id)recursiveFindItemWithTag:(NSInteger)aTag
{
	NSMenuItem *item = [self itemWithTag:aTag];
	if (item)
		return item;
	NSArray *items = [self itemArray];
	int count = (int)[items count];
	for (int i = 0; i < count; i++) {
		item = [[[items objectAtIndex:i] submenu] recursiveFindItemWithTag:aTag];
		if (item)
			return item;
	}
	return nil;
}

// Copies of the items (title, tag, target, action and submenus).
- (void)addItems:(id)aItems
{
	int count = (int)[aItems count];
	for (int i = 0; i < count; i++) {
		NSMenuItem *source = [aItems objectAtIndex:i];
		NSMenuItem *item;
		if ([source isSeparatorItem]) {
			item = [NSMenuItem separatorItem];
		} else {
			item = [[NSMenuItem new] autorelease];
			[item setTitle:[source title]];
			[item setTag:[source tag]];
			[item setTarget:[source target]];
			[item setAction:[source action]];
			NSMenu *submenu = [source submenu];
			if (submenu) {
				NSMenu *copy = [[NSMenu new] autorelease];
				[copy addItems:[submenu itemArray]];
				[item setSubmenu:copy];
			}
		}
		[self addItem:item];
	}
}

@end
