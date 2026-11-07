#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsInfoSimpleDataProvider.h"


@interface WGEntryDetailsDateTimeDataProvider : WGEntryDetailsInfoSimpleDataProvider
{
    NSString *_currentDateTimeString;
}
- (void)_adjustTime;
- (void)didMountedToOwner:(id)aOwner;
- (id)description;
- (void)dealloc;
@end

