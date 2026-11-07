#import "WeatherGlobePrefix.h"


@interface WeatherPalettePrintDataSourceProvider : NSObject <PrintDataSourceProviderProtocol>
{
    TLArray<PointEntry*> *_entries;
    BOOL _isShortForecast;
}
+ (id)weatherPalettePrintDataSourceProviderWithEntriesList:(TLArray<PointEntry*> *)aList isPrintShortForecast:(BOOL)aForecast;
- (void)preparePrintingDidEndSelector:(SEL)aSelector target:(id)aTarget;
- (id)printJobTitle;
- (id)customPrintDialog;
- (id)hostWindowForPrintDialog;
- (id)printDataSources;
- (id)initWithEntriesList:(TLArray<PointEntry*> *)aList isPrintShortForecast:(BOOL)aForecast;
- (void)dealloc;
@end

