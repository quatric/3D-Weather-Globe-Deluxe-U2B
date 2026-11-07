#import "IAppsPasteboardDataProvider.h"

@implementation IAppsPasteboardDataProvider

+ (id)dataProviderWithItems:(id)aItems
{
	return [[[[self class] alloc] initWithItems:aItems] autorelease];
}

- (id)initWithItems:(id)aItems
{
	self = [super init];
	if (self)
		_itemsArray = [aItems retain];
	return self;
}

- (void)dealloc
{
	[_itemsArray release];
	[super dealloc];
}

// The items' file paths, for the filenames type.
- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType
{
	if ([_itemsArray count] == 0 || ![aType isEqualToString:NSFilenamesPboardType])
		return;
	NSMutableArray *paths = nil;
	NSEnumerator *items = [_itemsArray objectEnumerator];
	id item;
	while ((item = [items nextObject])) {
		if (![item respondsToSelector:@selector(URL)])
			continue;
		NSString *path = [[item URL] path];
		if (path) {
			if (!paths)
				paths = [NSMutableArray arrayWithCapacity:0];
			[paths addObject:path];
		}
	}
	if (paths)
		[aPasteboard setPropertyList:paths forType:aType];
}

@end
