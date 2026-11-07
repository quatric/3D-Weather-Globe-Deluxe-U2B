// nibconv old.nib/objects.nib old.nib/keyedobjects.nib
//
// Re-archives a typedstream (NSArchiver) nib as a keyed one. The NSIBObjectData
// root decodes with NSUnarchiver and encodes itself keyed; NSNib looks for it
// under "IB.objectdata".
#import <Cocoa/Cocoa.h>
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
int main(int argc, char **argv)
{
	@autoreleasepool {
		[NSApplication sharedApplication];
		NSData *data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:argv[1]]];
		id root = nil;
		@try {
			root = [NSUnarchiver unarchiveObjectWithData:data];
		} @catch (NSException *e) {
			fprintf(stderr, "unarchive: %s\n", [[e description] UTF8String]);
			return 1;
		}
		fprintf(stderr, "root: %s\n", [NSStringFromClass([root class]) UTF8String]);
		NSMutableData *keyed = [NSMutableData data];
		NSKeyedArchiver *archiver = [[NSKeyedArchiver alloc] initForWritingWithMutableData:keyed];
		[archiver setOutputFormat:NSPropertyListBinaryFormat_v1_0];
		[archiver encodeObject:root forKey:@"IB.objectdata"];
		[archiver encodeInt:1 forKey:@"IB.systemFontUpdateVersion"];
		[archiver finishEncoding];
		if (![keyed writeToFile:[NSString stringWithUTF8String:argv[2]] atomically:YES])
			return 1;
		fprintf(stderr, "wrote %lu bytes\n", (unsigned long)[keyed length]);
	}
	return 0;
}
