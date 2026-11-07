#import "WeatherGlobePrefix.h"
#import "CitiesListController.h"


@interface WGMainEntriesListController : EntriesListController <WGEntryOwner>
{
    WGFilledView *_entriesListView;
    NSTextField *_descriptionField;
    WGTexturedView *_statusBar;
    NSMenu *_popupMenu;
    NSButton *_plusButton;
    NSButton *_popupMenuButton;
    NSButton *_columnSelectButton;
    NSButton *_switchEntriesTableViewButton;
    NSView *_resourceView;
    NSTabView *_tabView;
    WGSearchedEntriesTableView *_advancedEntriesTableView;
    NSColor *_foundByNameBackgroundColor;
    BOOL _isEntriesTableViewReady;
    LString *_selectedPreviewEntryUID;
}
+ (id)wgIdentifier;
- (GenericEntry *)currentEntry;
- (void)reloadGenericList;
- (void)rebuildLayout;
- (void)runTransitionToTabViewItemID:(id)aID;
- (void)switchTabViewID:(id)aID;
- (void)switchEntriesTableViewAction:(id)aAction;
- (NSInteger)indexOfSerachedEntryWithUID:(LString *)aUID;
- (GenericEntry *)searchedEntryAtIndex:(NSInteger)aIndex createIfNeeded:(BOOL)aNeeded;
- (void)setLastSelectedEntryUID:(LString *)aUID;
- (LString *)lastSelectedEntryUID;
- (NSInteger)searchedEntriesCount;
- (id)entriesTableView;
- (LClass *)_entriesListDataSourceClass;
- (void)entriesListCommand:(NSInteger)aCommand;
- (BOOL)validateEntriesListCommand:(id)aCommand;
- (void)onGearButton:(id)aButton;
- (void)showPopupMenu:(id)aMenu;
- (void)entriesWereChanged;
- (void)tableViewSelectionDidChange:(id)aChange;
- (BOOL)tableView:(id)aView shouldSelectRow:(NSInteger)aRow;
- (id)tableView:(id)aView requestedCursorForColumnRowIndices:(id)aIndices;
- (void)tableView:(id)aView didClickColumnRowIndices:(id)aIndices;
- (void)tableView:(id)aView accessibilityActionOnColumnRowIndices:(id)aIndices;
- (id)tableView:(id)aView toolTipForCell:(id)aCell rect:(NSRect *)aRect tableColumn:(id)aColumn row:(NSInteger)aRow mouseLocation:(NSPoint)aLocation;
- (void)tableView:(id)aView didClickTableColumn:(id)aColumn;
- (void)tableView:(id)aView willHideTableColumn:(id)aColumn;
- (id)tableView:(id)aView menuForEvent:(id)aEvent;
- (id)tableView:(id)aView objectValueForTableColumn:(id)aColumn row:(NSInteger)aRow;
- (NSInteger)numberOfRowsInTableView:(id)aView;
- (unsigned long long)flagsForTag:(NSInteger)aTag;
- (BOOL)addNewEntry:(GenericEntry *)aEntry;
- (id)advancedEntriesTableView;
- (id)tabView;
- (BOOL)isActive;
- (id)pane;
- (BOOL)isDragSupported;
- (id)nibName;
- (void)dealloc;
- (void)awakeFromNib;
- (BOOL)validateUserInterfaceItem:(id)aItem;
- (void)_validateInterface;
- (void)_validatePlusButton;
@end

