#import "WGRouteManagerController.h"

// A route folder's name and children count, equal by name.
@implementation _FolderName

- (id)initWithName:(id)aName childrenCount:(NSInteger)aCount
{
	self = [super init];
	if (self) {
		_childrenCount = (int)aCount;
		_name = [aName retain];
	}
	return self;
}

- (id)copyWithZone:(NSZone *)aZone
{
	return [[_FolderName allocWithZone:aZone] initWithName:_name childrenCount:_childrenCount];
}

- (void)dealloc
{
	[_name release];
	[super dealloc];
}

- (BOOL)isEqual:(id)aEqual
{
	return [aEqual isKindOfClass:[_FolderName class]] && [_name isEqualToString:[aEqual name]];
}

- (NSInteger)childrenCount
{
	return _childrenCount;
}

- (id)name
{
	return _name;
}

@end
