#import "WGHelpBook.h"
#include <Carbon/Carbon.h>

@implementation WGHelpBook

+ (BOOL)registerHelpBook
{
	FSRef ref;
	OSStatus status = FSPathMakeRef((const UInt8 *)[[[NSBundle mainBundle] bundlePath] fileSystemRepresentation], &ref, 0);
	if (status == noErr)
		status = AHRegisterHelpBook(&ref);
	return status == noErr;
}

+ (BOOL)gotoPage:(id)aPage anchor:(id)aAnchor
{
	NSString *book = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleHelpBookName"];
	return AHGotoPage((CFStringRef)book, (CFStringRef)aPage, (CFStringRef)aAnchor) == noErr;
}

@end
