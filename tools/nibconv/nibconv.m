// Converts an old typedstream nib (objects.nib, written with NSArchiver)
// into a keyed one (keyedobjects.nib) that 64-bit AppKit can load.
// usage: nibconv <in.nib> <out.nib>
#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

// Old nibs keep a control's custom class (LinkButton for an NSButton) in an
// NSClassSwapper, which picks the class while decoding and falls back to the
// standard one when the class doesn't exist. This tool doesn't have the app's
// classes, so empty stand-ins are registered under the names in classes.nib;
// the keyed archive then records the custom name and the app loads its own.
static void registerStandInClasses(NSString *aNib)
{
	NSString *path = [aNib stringByAppendingPathComponent:@"classes.nib"];
	NSString *text = [NSString stringWithContentsOfFile:path encoding:NSMacOSRomanStringEncoding error:NULL];
	NSArray *classes = nil;
	@try {
		classes = [[text propertyList] objectForKey:@"IBClasses"];
	} @catch (NSException *e) {
		return;
	}
	// a superclass may itself be one of the custom classes
	BOOL added = YES;
	while (added) {
		added = NO;
		for (NSDictionary *info in classes) {
			NSString *name = [info objectForKey:@"CLASS"];
			Class superclass = NSClassFromString([info objectForKey:@"SUPERCLASS"]);
			if (name == nil || superclass == Nil || NSClassFromString(name) != Nil)
				continue;
			Class standIn = objc_allocateClassPair(superclass, [name UTF8String], 0);
			if (standIn == Nil)
				continue;
			objc_registerClassPair(standIn);
			added = YES;
		}
	}
}

int main(int argc, const char *argv[])
{
	@autoreleasepool {
		if (argc != 3) {
			fprintf(stderr, "usage: nibconv <in.nib> <out.nib>\n");
			return 2;
		}
		[NSApplication sharedApplication];
		NSString *in = [NSString stringWithUTF8String:argv[1]];
		NSString *out = [NSString stringWithUTF8String:argv[2]];
		NSData *data = [NSData dataWithContentsOfFile:[in stringByAppendingPathComponent:@"objects.nib"]];
		id root = nil;
		registerStandInClasses(in);
		@try {
			root = [NSUnarchiver unarchiveObjectWithData:data];
		} @catch (NSException *e) {
			fprintf(stderr, "%s: %s\n", argv[1], [[e description] UTF8String]);
			return 1;
		}
		if (root == nil) {
			fprintf(stderr, "%s: nothing decoded\n", argv[1]);
			return 1;
		}
		fprintf(stderr, "decoded %s\n", [NSStringFromClass([root class]) UTF8String]);
		// nib loading looks for the object data under IB's key, not "root"
		NSKeyedArchiver *archiver = [[NSKeyedArchiver alloc] initRequiringSecureCoding:NO];
		[archiver encodeObject:root forKey:@"IB.objectdata"];
		[archiver finishEncoding];
		NSData *keyed = archiver.encodedData;
		[[NSFileManager defaultManager] createDirectoryAtPath:out withIntermediateDirectories:YES
			attributes:nil error:NULL];
		[keyed writeToFile:[out stringByAppendingPathComponent:@"keyedobjects.nib"] atomically:YES];
		return 0;
	}
}
