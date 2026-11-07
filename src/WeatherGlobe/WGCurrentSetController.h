#import "WeatherGlobePrefix.h"
#import "WGMountableController.h"


@interface WGCurrentSetController : WGMountableController <WGEntryOwner>
{
    NSTextView *placeholderStringProvider;
    WGTexturedView *statusBar;
    NSMenu *popupMenu;
    NSButton *popupMenuButton;
    NSButton *flyMeThereButton;
    NSButton *saveButton;
    NSButton *clearRouteButton;
    NSButton *routeManagerButton;
    NSMenu *contextMenu;
    NSTextField *totalDistance;
    NSTextField *subdistanceTemplate;
    WGCurrentSetTableView *listTableView;
    WGCurrentSetTableView *balloonTableView;
    NSButton *columnSelectButton;
    NSView *_retainedView;
    int _mode;
    CurrentSetController *_currentSetController;
    NSMutableIndexSet *_draggedEntriesIndexes;
    NSString *_currentRoutePath;
    int _routeDocRevision;
    BOOL _hideExtensionInRouteFileName;
    BOOL _disableAutoscrollOnListSelectionChange;
    NSIndexSet *_savedSelectionIndexSet;
}
+ (id)wgIdentifier;
- (void)importRouteObject:(id)aObject;
- (void)wgPerformRedoInTableView:(id)aView;
- (void)wgPerformUndoInTableView:(id)aView;
- (BOOL)wgValidateRedo:(id)aRedo inTableView:(id)aView;
- (BOOL)wgValidateUndo:(id)aUndo inTableView:(id)aView;
- (GenericEntry *)currentEntry;
- (void)doRouteCommand:(NSInteger)aCommand;
- (void)flyStoped;
- (void)onGearButton:(id)aButton;
- (void)doubleAction:(id)aAction;
- (BOOL)validateRouteCommand:(id)aCommand;
- (void)_addSelectedItemsToDatabase;
- (BOOL)_canAddSelectedItemsToDatabase;
- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType;
- (BOOL)tableView:(id)aView writeRows:(id)aRows toPasteboard:(id)aPasteboard;
- (void)tableView:(id)aView didClickTableColumn:(id)aColumn;
- (id)tableView:(id)aView menuForEvent:(id)aEvent;
- (void)wgChangeItemColorInTableView:(id)aView;
- (BOOL)tableView:(id)aView acceptDrop:(id)aDrop row:(NSInteger)aRow dropOperation:(NSInteger)aOperation;
- (NSUInteger)tableView:(id)aView validateDrop:(id)aDrop proposedRow:(NSInteger)aRow proposedDropOperation:(NSInteger)aOperation;
- (id)orderedTableColumnIDsForTableView:(id)aView;
- (BOOL)tableView:(id)aView canHideTableColumn:(id)aColumn;
- (id)wgTableView:(id)aView distanceForSubset:(NSInteger)aSubset;
- (id)wgColumnIDForEditingInTableView:(id)aView;
- (BOOL)isWGTableViewPrinted:(id)aPrinted;
- (BOOL)wgTableView:(id)aView shouldDisplayBreakAtRow:(NSInteger)aRow;
- (BOOL)wgTableView:(id)aView shouldDisplayBreakAtColumn:(id)aColumn;
- (BOOL)tableView:(id)aView shouldEditTableColumn:(id)aColumn row:(NSInteger)aRow;
- (void)wgDeleteItemsInTableView:(id)aView;
- (BOOL)wgValidateDeleteItemsInTableView:(id)aView;
- (void)tableView:(id)aView setObjectValue:(id)aValue forTableColumn:(id)aColumn row:(NSInteger)aRow;
- (id)tableView:(id)aView objectValueForTableColumn:(id)aColumn row:(NSInteger)aRow;
- (NSInteger)numberOfRowsInTableView:(id)aView;
- (void)openColorPanelWithColor:(id)aColor;
- (void)applyColorToSelection;
- (id)entryExtendedInfo:(GenericEntry *)aInfo withLineBreaking:(NSInteger)aBreaking ownEntryName:(BOOL)aName;
- (id)distanceForSubset:(NSInteger)aSubset;
- (id)distanceStringForDistance:(CGFloat)aDistance;
- (void)addToCurrentSet:(RouteObject *)aSet;
- (void)addToCurrentSet:(RouteObject *)aSet withIndex:(NSInteger)aIndex;
- (void)replaceCurrentSet:(RouteObject *)aSet;
- (RouteObject *)currentRoute;
- (void)tableViewSelectionDidChange:(id)aChange;
- (void)measurementUnitsChangedNotification:(id)aNotification;
- (void)routeChangedNotification:(id)aNotification;
- (void)routeItemCountChangedNotification:(id)aNotification;
- (void)_validateRouteManagerButton;
- (void)_updateTotalDistance;
- (void)addEntriesFromDescrArray:(TLArray<LF::LMap*> *)aArray startingFromRow:(NSInteger)aRow;
- (void)addEntriesToRoute:(TLArray<GenericEntry*> *)aRoute;
- (void)addEntryToRoute:(RouteSetItem *)aRoute;
- (TLArray<GenericEntry*> *)selectedItems;
- (GenericEntry *)selectedItem;
- (void)_selectRowsWithRange:(NSRange)aRange;
- (NSInteger)_insertionRowIndex;
- (id)tableView;
- (NSInteger)mode;
- (void)setMode:(NSInteger)aMode;
- (id)nibName;
- (void)dealloc;
- (void)awakeFromNib;
- (id)init;
@end

