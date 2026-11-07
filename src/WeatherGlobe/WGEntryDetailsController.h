#import "WeatherGlobePrefix.h"
#import "WGMountableController.h"


@interface WGEntryDetailsController : WGMountableController <WGEntryOwner, WGMasterEditController, WGEditableController, WGRebuildLayoutProtocol, WGDragValidationDelegate>
{
    WGPlaceholderView *placeholder;
    NSMatrix *modeMatrix;
    NSView *paneContainer;
    WGTexturedView *backgroundView;
    NSScrollView *scrollView;
    NSMutableDictionary *_controllers;
    WGEntryDetailsPaneController *_currentController;
    EntryInfoController *_entryInfoController;
    BOOL _isLayoutingSubviews;
    BOOL _isComitting;
    NSMapTable *_dragTypeOwnerMap;
}
+ (id)wgIdentifier;
- (void)unmountPaneController:(id)aController;
- (void)mountPaneController:(id)aController;
- (id)controllerForClass:(Class)aClass;
- (id)currentController;
- (id)headerViewController;
- (void)setupCell:(id)aCell withController:(id)aController;
- (void)setupButtonCells;
- (void)buttonAction:(id)aAction;
- (void)entryDidChange:(id)aChange;
- (void)_appWillTerminate;
- (void)registerDraggedTypes:(id)aTypes forOwner:(id)aOwner;
- (void)validateCurrentPane;
- (void)setCurrentPaneController:(id)aController;
- (id)panesControllersClasses;
- (void)gotoCurrentLocation;
- (BOOL)isGotoLocationEnabled;
- (BOOL)view:(id)aView acceptDrop:(id)aDrop;
- (NSUInteger)view:(id)aView validateDrop:(id)aDrop;
- (void)rebuildLayout:(id)aLayout;
- (void)rebuildLayout;
- (BOOL)tryOpenFieldEditor;
- (void)validateEditing;
- (void)setIsEditing:(BOOL)aEditing;
- (BOOL)isEditing;
- (BOOL)canEdit;
- (EntryInfoController *)entryInfoController;
- (void)setCurrentEntry:(GenericEntry *)aEntry;
- (GenericEntry *)currentEntry;
- (void)didMounted:(BOOL)aMounted;
- (id)nibName;
- (void)dealloc;
- (void)awakeFromNib;
- (id)init;
@end

