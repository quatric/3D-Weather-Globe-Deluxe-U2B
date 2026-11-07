#import "NSObject+SharedInstance.h"

@implementation NSObject (WGExtensions)

+ (id)wgIdentifier
{
	return NSStringFromClass(self);
}

- (id)wgIdentifier
{
	return [[self class] wgIdentifier];
}

@end
