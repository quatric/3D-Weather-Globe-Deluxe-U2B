#import "WeatherGlobePrefix.h"


@interface NSObject (SharedInstance)
+ (id *)sharedInstancePtr;
+ (id)sharedInstance;
+ (void)_terminateSharedInstance;
- (void)invalidate;
@end

@interface NSObject (WGExtensions)
+ (id)wgIdentifier;
- (id)wgIdentifier;
@end

