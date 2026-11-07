#import "WeatherGlobePrefix.h"
#import "WGPaletteBaseController.h"
#import "WGTexturedView.h"


@interface WGRouteManagerController : WGPaletteBaseController <EntriesDraggingHelperProtocol>
{
    NSView *_detailsViewPlaceholder;
    NSView *_leftView;
    WGEarthViewPlaceholder *_earthViewPlaceholder;
    WGRouteManagerTexturedView *_texturedBarView;
    WGRouteManagerActiveTexturedView *_texturedBottomView;
    NSScrollView *_mainScrollView;
    WGSplitView *_splitView;
    WGSplitter *_splitter;
    WGLeftPlaceholder *_leftPlaceholder;
    WGRightPlaceholder *_rightPlaceholder;
    NSButton *_editEntryButton;
    NSButton *_gearButton;
    NSMenu *_popupMenu;
    WGEarthView *_earthView;
    WGFilledView *_bottomFilledView;
    WGTexturedButton *_replaceCurrSetBtn;
    WGTexturedButton *_addToCurrSetBtn;
    NSMutableArray *_outlineViewsArray;
    NSOutlineView *_editableOutline;
    NSArray *_draggedNodes;
    RouteManagerController *_routeManagerController;
    WGRouteEntryDetailsController *_entryDetailsController;
    NSFont *_outlineVewsRootFont;
    WGCustomFieldEditor *_fieldEditor;
    BOOL _isEditingCanceled;
    unsigned int _tmpOutlineViewRowsCount;
    BOOL _shouldReactOnSelectionDidChange;
    TLArray<GenericEntry*> *_draggedEntries;
}
- (id)earthPreview;
- (id)entryDetailsController;
- (void)help:(id)aHelp;
- (void)addRouteToCurrentSet:(id)aSet;
- (void)replaceCurrentSet:(id)aSet;
- (void)editEntryAction:(id)aAction;
- (void)sharedSetButtonCellAction:(id)aAction;
- (void)popupMenuButtonAction:(id)aAction;
- (void)addRouteEntry:(UserRouteObjectEntry *)aEntry;
- (BOOL)_canEdit;
- (void)_validateButtons;
- (void)_validateEditModeButton;
- (void)redo:(id)aRedo;
- (void)undo:(id)aUndo;
- (void)sendRouteByEmail:(id)aEmail;
- (void)addSelectionToRouteObject:(MutableRouteObject *)aObject;
- (void)exportRoute:(id)aRoute;
- (void)importRouteObject:(id)aObject;
- (void)loadRoute:(id)aRoute;
- (void)renameRoute:(id)aRoute;
- (void)_deleteRoute:(id)aRoute;
- (void)deleteRoute:(id)aRoute;
- (void)addRoute:(id)aRoute;
- (BOOL)validateMenuItem:(id)aItem;
- (void)setDraggedEntries:(TLArray<GenericEntry*> *)aEntries;
- (TLArray<GenericEntry*> *)draggedEntries;
- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType;
- (BOOL)outlineView:(id)aView acceptDrop:(id)aDrop item:(id)aItem childIndex:(NSInteger)aIndex;
- (NSUInteger)outlineView:(id)aView validateDrop:(id)aDrop proposedItem:(id)aItem proposedChildIndex:(NSInteger)aIndex;
- (BOOL)outlineView:(id)aView writeItems:(id)aItems toPasteboard:(id)aPasteboard;
- (id)windowWillReturnFieldEditor:(id)aEditor toObject:(id)aObject;
- (void)windowWillClose:(id)aClose;
- (void)splitViewWillResizeSubviews:(id)aSubviews;
- (void)windowDidResize:(id)aResize;
- (void)currentRouteSetChanged;
- (void)_routesWereChanged:(id)aChanged;
- (CGFloat)splitView:(id)aView constrainMaxCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt;
- (CGFloat)splitView:(id)aView constrainMinCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt;
- (void)splitView:(id)aView resizeSubviewsWithOldSize:(NSSize)aSize;
- (void)outlineViewItemDidCollapse:(id)aCollapse;
- (void)outlineViewItemWillCollapse:(id)aCollapse;
- (void)outlineViewItemDidExpand:(id)aExpand;
- (void)outlineViewItemWillExpand:(id)aExpand;
- (void)outlineViewSelectionDidChange:(id)aChange;
- (void)_selectionDidChangeInOutlineView:(id)aView;
- (BOOL)outlineView:(id)aView shouldSelectItem:(id)aItem;
- (void)outlineView:(id)aView willDisplayCell:(id)aCell forTableColumn:(id)aColumn item:(id)aItem;
- (void)outlineView:(id)aView setObjectValue:(id)aValue forTableColumn:(id)aColumn byItem:(id)aItem;
- (id)outlineView:(id)aView objectValueForTableColumn:(id)aColumn byItem:(id)aItem;
- (NSInteger)outlineView:(id)aView numberOfChildrenOfItem:(id)aItem;
- (BOOL)outlineView:(id)aView isItemExpandable:(id)aExpandable;
- (id)outlineView:(id)aView child:(NSInteger)aChild ofItem:(id)aItem;
- (void)dealloc;
- (void)awakeFromNib;
- (id)_outlineVewForItem:(GenericRouteNode *)aItem withFrame:(NSRect)aFrame andID:(NSInteger)aID;
- (id)autosaveName;
- (id)nibName;
- (void)_reloadDataInOutlineView:(id)aView;
- (void)_outlineViewDidChange:(id)aChange;
- (void)_shiftSubviewsUnderOutlineView:(id)aView byDelta:(CGFloat)aDelta;
- (TLArray<GenericRouteNode*> *)_routeNodesFromRouteNodesWrappers:(id)aWrappers;
- (GenericRouteNode *)_getSelectedRouteNodeInOLV:(id)aOLV isFolder:(BOOL)aFolder;
- (GenericRouteNode *)_geteSelectedRouteNodeFolder:(BOOL)aFolder;
- (void)_cancelEditing;
- (void)_editItem:(id)aItem;
- (void)_setDraggedNodes:(id)aNodes;
- (id)draggedNodes;
@end

@interface RouteNodeTextFormatter : NSFormatter
{
}
+ (id)routeNodeTextFormatter;
- (id)editingStringForObjectValue:(id)aValue;
- (id)attributedStringForObjectValue:(id)aValue withDefaultAttributes:(id)aAttributes;
- (BOOL)getObjectValue:(id *)aValue forString:(id)aString errorDescription:(id *)aDescription;
- (id)stringForObjectValue:(id)aValue;
@end

@interface _FolderName : NSObject
{
    NSString *_name;
    int _childrenCount;
}
- (id)name;
- (NSInteger)childrenCount;
- (BOOL)isEqual:(id)aEqual;
- (void)dealloc;
- (id)copyWithZone:(NSZone *)aZone;
- (id)initWithName:(id)aName childrenCount:(NSInteger)aCount;
@end

@interface WGRouteManagerOutlineView : NSOutlineView
{
    BOOL _editingFlag;
    NSString *_accessibilityRoleDesc;
}
- (id)accessibilityAttributeValue:(id)aValue;
- (void)deleteForward:(id)aForward;
- (void)keyDown:(id)aDown;
- (void)setEditingFlag:(BOOL)aFlag;
- (void)mouseDown:(id)aDown;
- (void)editColumn:(NSInteger)aColumn row:(NSInteger)aRow withEvent:(id)aEvent select:(BOOL)aSelect;
- (void)setAccessibilityRoleDesc:(id)aDesc;
- (void)dealloc;
@end

@interface WGRightPlaceholder : NSView
{
}
- (NSSize)wgMinSize;
@end

@interface WGLeftPlaceholder : NSView
{
}
- (NSSize)wgMinSize;
@end

@interface WGRouteManagerPanel : NSPanel
{
}
- (void)doCommandBySelector:(SEL)aSelector;
- (void)redo:(id)aRedo;
- (void)undo:(id)aUndo;
- (BOOL)validateMenuItem:(id)aItem;
@end

@interface WGRouteManagerActiveTexturedView : WGTexturedView
{
}
- (BOOL)mouseDownCanMoveWindow;
@end

@interface WGRouteManagerTexturedView : WGTexturedView
{
}
- (BOOL)mouseDownCanMoveWindow;
@end

@interface WGRouteManagerFlippedDocumentView : NSView
{
}
- (BOOL)isFlipped;
@end

