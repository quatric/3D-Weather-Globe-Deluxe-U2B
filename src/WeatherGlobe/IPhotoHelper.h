#import "WeatherGlobePrefix.h"


@interface IPhotoHelper : NSObject
{
}
+ (id)iAppsRecentDatabasesForKeys:(id)aKeys;
+ (id)applicationBundleForBundleID:(id)aID creator:(NSUInteger)aCreator;
+ (id)arrayByRemovingNonExistedPaths:(id)aPaths;
+ (id)resolveAliasesInURL:(id)aURL;
+ (BOOL)iPhotoApplicationVersionSupported;
+ (CGFloat)iPhotoApplicationVersion;
+ (id)iPhotoRecentDatabases;
@end

