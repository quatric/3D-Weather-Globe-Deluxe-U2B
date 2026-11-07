#import "WeatherGlobePrefix.h"
#import "WGCurrentSetTableView.h"
#import "WGNibOwnerController.h"


@interface RouteSetPrintDataSource : WGNibOwnerController
{
    WGRouteSetPrintView *_printView;
    WGPrintTableViewPlaceholder *_tableViewPlaceholder;
    NSView *_headerView;
    WGFilledView *_headerViewBackground;
    NSView *_bottomView;
    WGFilledView *_bottomViewBackground;
    NSTextField *_totalDistanceTextField;
    NSTextField *_intervalBetweenTextField;
    WGPrintRouteSetTableView *_printedTableView;
    BOOL _printViewAlreadySetupped;
    NSSize _neededPrintViewSize;
    float _tmpLastPrintViewOrigin;
    int _pagesCount;
    int _currentPage;
}
- (id)printViewName;
- (id)pageViewAtIndex:(NSInteger)aIndex;
- (void)dataSourceWasChanged;
- (BOOL)isPrintOnSinglePage;
- (NSInteger)pagesCountForSize:(NSSize)aSize;
- (void)_setPrintTableViewCustomCell;
- (void)dealloc;
- (void)awakeFromNib;
- (id)nibName;
@end

@interface WGPrintTableViewCell : NSTextFieldCell
{
}
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView;
@end

@interface WGRouteSetPrintView : NSView
{
}
- (BOOL)isFlipped;
@end

@interface WGPrintTableViewPlaceholder : NSView
{
}
- (BOOL)isFlipped;
@end

@interface WGPrintRouteSetTableView : WGCurrentSetTableView
{
}
- (NSInteger)lineBraking;
@end

