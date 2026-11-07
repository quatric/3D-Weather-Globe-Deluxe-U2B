#import "WGRouteManagerController.h"

@implementation RouteNodeTextFormatter

+ (id)routeNodeTextFormatter
{
	return [[[self alloc] init] autorelease];
}

// Folders show their children count.
- (id)stringForObjectValue:(id)aValue
{
	if ([aValue isKindOfClass:[_FolderName class]])
		return [NSString stringWithFormat:@"%@ (%d)", [aValue name], (int)[aValue childrenCount]];
	return [aValue isKindOfClass:[NSString class]] ? aValue : nil;
}

- (BOOL)getObjectValue:(id *)aValue forString:(id)aString errorDescription:(id *)aDescription
{
	*aValue = aString;
	return YES;
}

- (id)attributedStringForObjectValue:(id)aValue withDefaultAttributes:(id)aAttributes
{
	return [aValue isKindOfClass:[NSAttributedString class]] ? aValue : nil;
}

- (id)editingStringForObjectValue:(id)aValue
{
	if ([aValue isKindOfClass:[_FolderName class]])
		return [aValue name];
	return [aValue isKindOfClass:[NSString class]] ? aValue : nil;
}

@end
