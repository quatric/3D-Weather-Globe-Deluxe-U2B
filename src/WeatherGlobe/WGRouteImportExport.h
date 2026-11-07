#import "WeatherGlobePrefix.h"


@interface WGRouteImportExport : NSObject
{
    id _target;
    SEL _endCallback;
}
+ (void)_appWillTerminate;
+ (id)sharedRouteImportExport;
- (BOOL)panel:(id)aPanel shouldShowFilename:(id)aFilename;
- (void)importRouteWithTarget:(id)aTarget didEndSelector:(SEL)aSelector hostWindow:(id)aWindow;
- (void)exportRoute:(RouteObject *)aRoute withName:(id)aName hostWindow:(id)aWindow;
- (void)_openPanelDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo;
- (void)_savePanelDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo;
- (RouteObject *)loadRouteObjectAtPath:(id)aPath;
- (void)saveRouteObject:(RouteObject *)aObject atPath:(id)aPath hideExt:(BOOL)aExt;
- (id)_path;
- (id)defaultRoutesFolder;
- (void)dealloc;
@end

