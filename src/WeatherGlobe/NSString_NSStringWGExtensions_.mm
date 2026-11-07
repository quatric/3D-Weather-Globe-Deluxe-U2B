#import "NSMutableString+NSStringWGExtensions.h"

@implementation NSString (NSStringWGExtensions)

+ (id)wgShortSingleCoordDescription:(CGFloat)aDescription coordTypeLatitude:(BOOL)aLatitude
{
	return [NSString stringReferenceFromLString:Utils::shortSingleCoordDescription(aDescription, aLatitude, Utils::kGeolocFormatFull)];
}

+ (id)wgShortGeoCoordinatesDescription:(LPoint)aDescription
{
	return [NSString stringReferenceFromLString:Utils::shortGeoCoordinatesDescription(aDescription, Utils::kGeolocFormatFull)];
}

- (id)wgStringByRemovingCharactersInSet:(id)aSet
{
	NSMutableString *string = [[self mutableCopy] autorelease];
	[string wgRemoveCharactersInSet:aSet];
	return [NSString stringWithString:string];
}

// Each "%@" in the format takes the next argument; a literal "\n" becomes a
// line break.
+ (id)wgStringWithFormat:(id)aFormat arguments:(id)aArguments
{
	NSMutableString *string = [NSMutableString string];
	NSEnumerator *arguments = [aArguments objectEnumerator];
	for (NSString *part in [aFormat componentsSeparatedByString:@"%@"]) {
		[string appendString:part];
		NSString *argument = [arguments nextObject];
		if (argument)
			[string appendString:argument];
	}
	[string replaceOccurrencesOfString:@"\\n" withString:@"\n" options:0 range:NSMakeRange(0, [string length])];
	return [[string copy] autorelease];
}

// "name.ext" if it's free, else "name 2.ext", "name 3.ext"…
+ (id)wgCreateUniqueFilename:(id)aFilename extension:(id)aExtension
{
	NSString *path = [aExtension length] ? [aFilename stringByAppendingPathExtension:aExtension] : aFilename;
	if (![[NSFileManager defaultManager] fileExistsAtPath:path])
		return path;
	NSString *name = [aFilename lastPathComponent];
	NSArray *words = [name componentsSeparatedByString:@" "];
	NSString *last = [words lastObject];
	int number = 0;
	if ([words count] > 1 && (number = [last intValue]) != 0)
		name = [name substringToIndex:[name length] - 1 - [last length]];
	name = [name stringByAppendingFormat:@" %i", number + 1];
	return [self wgCreateUniqueFilename:[[aFilename stringByDeletingLastPathComponent] stringByAppendingPathComponent:name]
	                          extension:aExtension];
}

@end
