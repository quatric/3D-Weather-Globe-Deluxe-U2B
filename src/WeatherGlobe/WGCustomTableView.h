#import "WeatherGlobePrefix.h"


@interface WGCustomTableView : NSTableView
{
    NSMenu *_popupMenu;
    NSMutableArray *_invisibleColumns;
}
- (void)menuAction:(id)aAction;
- (BOOL)validateMenuItem:(id)aItem;
- (void)onCornerViewButton:(id)aButton;
- (id)cornerViewPopupMenu;
- (void)createCornerMenuForButton:(id)aButton;
- (id)_createMenu;
- (BOOL)isHiddenColumnWithIdentifier:(id)aIdentifier;
- (void)hideColumnWithIdentifier:(id)aIdentifier;
- (void)showColumnWithIdentifier:(id)aIdentifier;
- (id)tableColumnWithIdentifier:(id)aIdentifier;
- (NSInteger)positionOfInvisibleColumn:(id)aColumn;
- (id)invisibleColumnWithIdentifier:(id)aIdentifier;
- (id)invisibleColumnIDs;
- (id)invisibleColumns;
- (void)dealloc;
@end

@interface NSTableColumn (WGCustomTableViewColumn)
- (NSInteger)comparisonFunc:(id)aFunc;
@end

