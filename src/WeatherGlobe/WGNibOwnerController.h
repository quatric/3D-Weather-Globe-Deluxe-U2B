#import "WeatherGlobePrefix.h"


@interface WGNibOwnerController : NSObject
{
    BOOL _nibWasLoaded;
    id delegate;
}
- (void)setDelegate:(id)aDelegate;
- (id)delegate;
- (BOOL)loadNibIfNeeded;
- (id)nibName;
@end

