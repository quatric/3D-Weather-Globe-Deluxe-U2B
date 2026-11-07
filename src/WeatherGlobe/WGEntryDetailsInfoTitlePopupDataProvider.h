#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsInfoSimpleDataProvider.h"


@interface WGEntryDetailsInfoTitlePopupDataProvider : WGEntryDetailsInfoSimpleDataProvider
{
    NSTextField *_etalonTextField;
}
- (void)titlePopupAction:(id)aAction;
- (id)popupTitles;
- (NSSize)titleSizeForTextField:(id)aField titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset;
- (id)createTitleViewWithTitleString:(id)aString;
- (NSSize)titleSizeForTitleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset;
- (void)dealloc;
@end

