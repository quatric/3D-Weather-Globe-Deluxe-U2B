#import "AtlasStartupDelegate.h"
#import "AtlasController.h"

@implementation AtlasStartupDelegate

- (void)applicationDidFinishLaunching:(id)aLaunching
{
	[AtlasController atlasController];
}

- (BOOL)application:(id)aApplication openFile:(id)aFile
{
	return [[AtlasController atlasController] application:aApplication openFile:aFile];
}

- (BOOL)application:(id)aApplication printFile:(id)aFile
{
	return [[AtlasController atlasController] application:aApplication printFile:aFile];
}

@end
