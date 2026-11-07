#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsInfoSimpleDataProvider.h"


@interface WGEntryDetailsKeyValueURLDataProvider : WGEntryDetailsInfoSimpleDataProvider
{
}
- (id)urlValue;
- (id)urlTitle;
- (id)descriptionView;
- (void)updateDescription;
@end

