#import "NSPasteboard+NSPasteboardWGExtensions.h"

@implementation NSPasteboard (NSPasteboardWGExtensions)

// An LF object graph, archived by LCoder.
- (BOOL)wgSetPropertyList:(LObject *)aList forType:(id)aType
{
	LData *archive = LCoder::archiveRootObject(aList, false);
	NSData *data = archive ? [NSData dataWithBytes:archive->bytes() length:archive->length()] : nil;
	return [self setData:data forType:aType];
}

- (LObject *)wgPropertyListForType:(id)aType
{
	NSData *data = [self dataForType:aType];
	if (!data)
		return 0;
	LData *archive = LData::dataWithBytesNoCopy((void *)[data bytes], (unsigned int)[data length], false);
	return archive ? LCoder::unarchiveRootObject(archive) : 0;
}

@end
