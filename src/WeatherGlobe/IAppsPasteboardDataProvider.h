#import "WeatherGlobePrefix.h"


@interface IAppsPasteboardDataProvider : NSObject
{
    NSArray *_itemsArray;
}
+ (id)dataProviderWithItems:(id)aItems;
- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType;
- (void)dealloc;
- (id)initWithItems:(id)aItems;
@end

