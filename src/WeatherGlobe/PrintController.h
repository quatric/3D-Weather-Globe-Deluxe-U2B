#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface PrintController : WGNibOwnerController
{
    PrintingView *_pageView;
    NSTextField *_printJobTitleField;
    NSTextField *_printNameField;
    NSTextField *_pageCounterField;
    NSTextField *_printDateField;
    NSTextField *_copyrightField;
    NSView *_placeView;
    NSBox *_topSeparator;
    NSBox *_bottomSeparator;
    NSImageView *_logoLeft;
    NSImageView *_logoCenter;
    NSImageView *_logoRight;
    NSImageView *_logoFull;
    NSView *_currPrintView;
    NSView *_helperView;
    id<PrintDataSourceProviderProtocol> _dataSourceProvider;
    NSArray *_dataSources;
    NSRange *_dataSourcesPageRanges;
    id<PrintDataSourceProtocol> _currentDataSource;
    NSRect _originalPlaceViewFrame;
    int _currentPage;
    int _pagesNum;
    int _pervPageNum;
    NSMutableSet *_pagesSet;
}
+ (void)printWithDataSourceProvider:(id)aProvider;
- (id)printJobTitle;
- (NSRect)preparePage:(NSInteger)aPage;
- (NSInteger)pagesCount;
- (void)_prepeareLogoToScale:(BOOL)aScale;
- (void)printOperationDidRun:(id)aRun success:(BOOL)aSuccess contextInfo:(void *)aInfo;
- (void)_startPrintOperationModalForWindow:(id)aWindow;
- (void)_cleanUp;
- (NSInteger)_pagesCountForDataSource:(id)aSource;
- (id)_dataSourceForPage:(NSInteger)aPage;
- (NSInteger)_numPagesForScale:(CGFloat)aScale;
- (NSSize)_pageSize;
- (id)_printInfo;
- (id)nibName;
- (void)awakeFromNib;
- (void)dealloc;
- (void)_startPrint;
- (void)sheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo;
- (void)_printWithDataSourceProvider:(id)aProvider;
@end

