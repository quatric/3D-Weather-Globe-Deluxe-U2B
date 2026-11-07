#import "WeatherGlobePrefix.h"


@interface CIPaletteController : NSObject
{
    CIPaletteWindow *_citiesPalette;
    CIPaletteView *_citiesPaletteView;
    CIPaletteTitleView *_citiesPaletteTitleView;
    NSSize _minContentSize;
    NSSize _contentSize;
    NSSize _maxCityViewSize;
    NSTimer *_timeTick;
    NSTimer *_reconstructTimer;
    NSRect _titleViewComponentRect;
    NSMutableArray *_cityInfoControllers;
    NSImage *_verticalSeparatorImage;
    NSMutableArray *_separatorViews;
    NSMenu *_mainMenu;
    NSMenu *_paletteContextMenu;
    CityInfoController *_draggedCityInfo;
    BOOL _mouseOutside;
    BOOL _isPrintShortForecast;
}
- (void)runPageLayout:(id)aLayout;
- (void)doPrint:(id)aPrint;
- (void)dealloc;
- (void)awakeFromNib;
- (id)init;
- (void)windowDidBecomeKey:(id)aKey;
- (void)windowDidResignKey:(id)aKey;
- (void)windowDidResize:(id)aResize;
- (BOOL)windowShouldClose:(id)aClose;
- (void)moveUp:(id)aUp;
- (void)moveRight:(id)aRight;
- (void)moveLeft:(id)aLeft;
- (void)moveDown:(id)aDown;
- (void)deleteForward:(id)aForward;
- (void)removeAllCities;
- (void)manualUpdate;
- (void)invalidate;
- (BOOL)paletteIsVisible;
- (void)togglePalette;
- (void)hidePalette;
- (void)showPalette;
- (void)addCities:(TLArray<LF::LMap*> *)aCities;
- (void)resetCities:(TLArray<LF::LMap*> *)aCities;
- (void)addEntries:(TLArray<GenericEntry*> *)aEntries;
- (BOOL)canAddEntriesFromPropertyList:(TLArray<LF::LMap*> *)aList;
- (BOOL)canAddEntries:(TLArray<GenericEntry*> *)aEntries;
- (LComparableSet *)_entryUIDsInPalette;
- (void)_acceptDraggingRemoveCity:(id)aCity;
- (void)_acceptDraggingExited:(id)aExited;
- (void)_acceptDraggingUpdated:(id)aUpdated;
- (void)_acceptDraggingEntered:(id)aEntered;
- (void)_timeTick:(id)aTick;
- (void)_acceptMenuAction:(id)aAction;
- (void)_acceptVerticalAction:(id)aAction;
- (void)_acceptCloseAction:(id)aAction;
- (void)_createMenu;
- (void)_addSelfMenu;
- (void)_createSelfMenu;
- (id)_menuItemWithTitle:(id)aTitle selector:(SEL)aSelector tag:(NSInteger)aTag targer:(id)aTarger;
- (id)_fontSizeMenuItem;
- (id)_newFontSizeMenuItemWithTitle:(id)aTitle tag:(NSInteger)aTag;
- (void)_paletteActionsMenu:(id)aMenu;
- (void)_fontSizeMenu:(id)aMenu;
- (void)showCityDetails:(id)aDetails;
- (BOOL)validateMenuItem:(id)aItem;
- (void)_createTitleView;
- (id)_titleViewComponents;
- (id)_buttonImageWithName:(id)aName;
- (id)_buttonWithTitle:(id)aTitle action:(SEL)aAction image:(id)aImage tag:(NSInteger)aTag enabled:(BOOL)aEnabled sendActionOn:(NSInteger)aOn;
- (void)_setCitiesFromPreferences;
- (void)_addDefaultCityIfNeeded;
- (void)_addCityInfo:(id)aInfo;
- (void)_saveCitiesInPrefs;
- (void)_changeVerticalState;
- (void)_postponedReconstruct;
- (void)_immediateReconstruct:(id)aReconstruct;
- (BOOL)_refreshMaxSizeWithNeedReconstruct:(BOOL)aReconstruct;
- (void)_reconstructPalette;
- (void)_resizePalette;
- (void)_adjustPalettePosition;
- (id)_screen;
- (id)stringWithLocaleShortDateFormatForAbsoluteTime:(double)aTime;
- (void)_constrainKeyViewsChain;
- (void)_cacheNextImageIfNeeded;
- (void)_loadNibIfNeeded;
@end

@interface NSMenu (Extensions)
- (void)addItems:(id)aItems;
- (id)recursiveFindItemWithTag:(NSInteger)aTag;
@end

