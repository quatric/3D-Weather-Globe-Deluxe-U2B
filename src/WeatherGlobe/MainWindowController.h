#import "WeatherGlobePrefix.h"


@interface MainWindowController : NSObject
{
    BOOL _nibWasLoaded;
    WGLeftAreaController *_leftAreaController;
    NSWindow *_mainWindow;
    WGSplitView *_mainSplitView;
    WGSplitView *_leftSplitView;
    NSView *_rightAreaView;
    AtlasGLView *_glView;
    WGTexturedView *_bottomArea;
    WGFilledView *_titleSeparator;
    WGCustomSlider *_scaleSlider;
    NSMatrix *_mapModeButtons;
    NSMatrix *_mouseBehaviourButtons;
    NSButton *_popupMenuButton;
    NSMenu *_popupMenu;
    NSButton *_weatherButton;
    NSButton *_currentSetButton;
    NSButton *_entryDetailsButton;
    NSButton *_editEntryButton;
    NSButton *_leftAreaButton;
    WGCustomFieldEditor *_fieldEditor;
    AppWindowInterfaceWrapper *_wndInterface;
    WGCustomToolbarController *_toolbarController;
    WGExportImageController *_exportImageController;
    NSWindow *_fullScreenWindow;
    NSRect _rightAreaRect;
    NSRect _glViewRect;
    BOOL _isAddingEntryToCurrentSet;
    id _currentEntryOwner;
}
- (void)addEntryToCurrentSet:(GenericEntry *)aSet;
- (void)_newLocationPaletteDidClose;
- (void)_newLocationCoordsPickerModeChanged;
- (void)_setMouseBehaviourWithControlTag:(NSInteger)aTag;
- (void)_glViewFrameDidChange;
- (void)_currentSetRouteWasAdded:(id)aAdded;
- (void)_appDidBecomeActive;
- (void)_validateInterface;
- (void)_validateLeftAreaControls;
- (void)_validateEditModeButton;
- (BOOL)_canEdit;
- (void)currentEntryDidChangeIn:(id)aIn;
- (void)contentDidChangeInController:(id)aController;
- (id)windowWillReturnFieldEditor:(id)aEditor toObject:(id)aObject;
- (NSRect)splitView:(id)aView effectiveRect:(NSRect)aRect forDrawnRect:(NSRect)aRect2 ofDividerAtIndex:(NSInteger)aIndex;
- (CGFloat)splitView:(id)aView constrainMaxCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt;
- (CGFloat)splitView:(id)aView constrainMinCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt;
- (void)splitView:(id)aView resizeSubviewsWithOldSize:(NSSize)aSize;
- (void)startDemo:(id)aDemo;
- (void)runPageLayout:(id)aLayout;
- (void)doPrint:(id)aPrint;
- (void)printGLViewOnly;
- (void)printLocationInfoOnly;
- (void)printCurrentSetOnly;
- (void)exportAsImage:(id)aImage;
- (void)onAddNewEntryButton:(id)aButton;
- (void)showDetails:(id)aDetails;
- (void)leftArea:(id)aArea;
- (void)entriesListMenu:(id)aMenu;
- (void)routeMenu:(id)aMenu;
- (void)editEntryAction:(id)aAction;
- (void)routeToggleFlying:(id)aFlying;
- (void)mainWindowMenu:(id)aMenu;
- (void)toggleWeatherPalette:(id)aPalette;
- (void)popupMenuButtonAction:(id)aAction;
- (void)mouseBehaviourChanged:(id)aChanged;
- (void)viewModeChanged:(id)aChanged;
- (void)scaleWasChanged:(id)aChanged;
- (void)scaleButtonAction:(id)aAction;
- (void)toggleFullWindowMode:(id)aMode;
- (void)toggleLeftArea:(id)aArea;
- (void)resetGLView:(id)aView;
- (void)_paletteActionsMenu:(id)aMenu;
- (void)viewControlMenu:(id)aMenu;
- (void)cloudsMenu:(id)aMenu;
- (void)locationsMenu:(id)aMenu;
- (void)regionHighlightMenu:(id)aMenu;
- (void)gridMenu:(id)aMenu;
- (void)spaceMenu:(id)aMenu;
- (void)drawElementsMenu:(id)aMenu;
- (BOOL)validateMenuItem:(id)aItem;
- (void)gotoLocation:(GenericEntry *)aLocation forced:(BOOL)aForced;
- (void)showDetailsForEntry:(GenericEntry *)aEntry openPane:(BOOL)aPane openInfoForBillboard:(NSInteger)aBillboard;
- (void)openSearchForTerm:(id)aTerm;
- (void)setScaleValue:(CGFloat)aValue;
- (void)_showInNormalWindowModeForced;
- (BOOL)isInFullWindowMode;
- (BOOL)isLeftAreaVisible;
- (BOOL)isWindowVisible;
- (void)hideWindow;
- (void)showWindow;
- (id)exportImageController;
- (id)toolbarController;
- (id)entryDetailsController;
- (id)currentSetController;
- (id)entriesListController;
- (AppWindowInterfaceWrapper *)windowInterface;
- (id)mainWindow;
- (id)glView;
- (void)dealloc;
- (void)awakeFromNib;
- (void)_setLeftWidth:(CGFloat)aWidth forSplitView:(id)aView;
- (void)_setupSeparators;
- (void)_setupScaleSlider;
- (void)_setupBottomArea;
- (void)_setupCustomTitle;
- (void)_setupToolbar;
- (void)windowDidResignKey:(id)aKey;
- (void)windowDidBecomeKey:(id)aKey;
- (void)windowWillMiniaturize:(id)aMiniaturize;
- (void)windowWillClose:(id)aClose;
- (void)_cleanupOnWindowClose;
- (BOOL)_loadNibIfNeeded;
@end

@interface WGMainWindow : NSWindow
{
}
- (void)redo:(id)aRedo;
- (void)undo:(id)aUndo;
- (BOOL)validateMenuItem:(id)aItem;
- (void)sendEvent:(id)aEvent;
- (NSRect)constrainFrameRect:(NSRect)aRect toScreen:(id)aScreen;
@end

@interface _WGFullScreenWindow : NSWindow
{
}
- (BOOL)canBecomeKeyWindow;
@end

