#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface LocationPrintDataSource : WGNibOwnerController
{
    WGLocationInfoPrintView *_printView;
    NSView *_detailsViewPlaceholder;
    WGPrintingEntryDetailsController *_entryDetailsController;
    GenericEntry *_printedEntry;
    NSSize _neededPrintViewSize;
    float _tmpLastPrintViewOrigin;
}
- (id)printViewName;
- (id)pageViewAtIndex:(NSInteger)aIndex;
- (void)dataSourceWasChanged;
- (BOOL)isPrintOnSinglePage;
- (NSInteger)pagesCountForSize:(NSSize)aSize;
- (GenericEntry *)printedEntry;
- (id)_entryDetailsController;
- (void)awakeFromNib;
- (id)nibName;
- (void)dealloc;
@end

@interface WGLocationInfoPrintView : NSView
{
}
- (BOOL)isFlipped;
@end

