#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface EntriesListController : WGNibOwnerController <EntriesDraggingHelperProtocol>
{
    DragSourceTableView *_entriesTableView;
    NSSearchField *_filterTextField;
    TLArray<GenericEntry*> *_draggedEntries;
    EntriesListDataSource *_entriesDataSource;
    NSMutableDictionary *_searchesCountMap;
    LString *_selectedEntryUID;
    BOOL _isReloading;
}
- (void)searchFieldAction:(id)aAction;
- (void)controlTextDidChange:(id)aChange;
- (void)controlTextDidEndEditing:(id)aEditing;
- (void)controlTextDidBeginEditing:(id)aEditing;
- (void)setDraggedEntries:(TLArray<GenericEntry*> *)aEntries;
- (TLArray<GenericEntry*> *)draggedEntries;
- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType;
- (id)defaultInvisibleTableColumnIDsForForTableView:(id)aView;
- (id)orderedTableColumnIDsForTableView:(id)aView;
- (BOOL)tableView:(id)aView canHideTableColumn:(id)aColumn;
- (void)tableViewSelectionDidChange:(id)aChange;
- (void)tableView:(id)aView didClickTableColumn:(id)aColumn;
- (void)tableView:(id)aView willHideTableColumn:(id)aColumn;
- (BOOL)tableView:(id)aView writeRows:(id)aRows toPasteboard:(id)aPasteboard;
- (id)tableView:(id)aView objectValueForTableColumn:(id)aColumn row:(NSInteger)aRow;
- (NSInteger)numberOfRowsInTableView:(id)aView;
- (id)_indicatorImageForSortingMode:(NSInteger)aMode;
- (void)entriesWereChanged;
- (LString *)initialSearchTerm;
- (unsigned long long)initialGeotypeFilter;
- (NSInteger)indexOfSerachedEntryWithUID:(LString *)aUID;
- (void)selectEntryWithUID:(LString *)aUID;
- (void)setLastSelectedEntryUID:(LString *)aUID;
- (LString *)lastSelectedEntryUID;
- (TLArray<GenericEntry*> *)selectedItemsWithLimit:(NSUInteger)aLimit createIfNeeded:(BOOL)aNeeded;
- (GenericEntry *)searchedEntryAtIndex:(NSInteger)aIndex createIfNeeded:(BOOL)aNeeded;
- (BOOL)singleItemIsSelected;
- (TLArray<GenericEntry*> *)selectedItems;
- (GenericEntry *)selectedItem;
- (void)setSearchTerm:(LString *)aTerm andRefreshResults:(BOOL)aResults;
- (void)setGeotypeFilter:(unsigned long long)aFilter;
- (unsigned long long)geotypeFilter;
- (void)reloadTableData;
- (NSInteger)searchedEntriesCount;
- (id)entriesTableView;
- (BOOL)isDragSupported;
- (LClass *)_entriesListDataSourceClass;
- (void)dealloc;
- (id)init;
- (void)awakeFromNib;
@end

@interface CitiesListController : EntriesListController
{
}
- (void)setGeotypeFilter:(unsigned long long)aFilter;
- (unsigned long long)initialGeotypeFilter;
@end

