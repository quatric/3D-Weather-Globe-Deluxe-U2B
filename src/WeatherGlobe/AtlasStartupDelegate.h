#import "WeatherGlobePrefix.h"


@interface AtlasStartupDelegate : NSObject
{
    NSMenu *_debugMenu;
}
- (BOOL)application:(id)aApplication printFile:(id)aFile;
- (BOOL)application:(id)aApplication openFile:(id)aFile;
- (void)applicationDidFinishLaunching:(id)aLaunching;
@end

