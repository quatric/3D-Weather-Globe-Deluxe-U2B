#import "WeatherGlobePrefix.h"


@interface AtlasController : NSObject
{
    MainWindowController *_mainWindowController;
    NSStatusItem *_barItems;
    NSMenu *_defaultStatusMenu;
    NSMenuItem *_onlineStatusMenuItem;
    CIPaletteController *_citiesPaletteController;
    WGRouteManagerController *_routeManagerPaletteController;
    PreferencesController *_preferencesController;
    AboutController *_aboutController;
    WGTimeLineController *_timeLineController;
    WGMovieController *_movieController;
    IPhotoImportController *_iPhotoImportController;
    WGSettingsPaletteController *_settingsPaletteController;
    WGNewLocationController *_newLocationController;
    SnapshotEditorController *_snapshotEditorController;
    NSMenu *_dockMenu;
    PointEntry *_dockEntry;
    BOOL _dockScheduled;
    BOOL _dockWeatherWasInitialize;
    NSWindow *_copyingWindow;
    NSTextField *_copyingStatus;
    NSProgressIndicator *_copyingProgress;
    int _copyingCurrentFileIdx;
    unsigned long long _copyingCurrentFileSize;
    unsigned long long _copyingCurrentFileCopiedSize;
    NSWindow *_insertDiscDialog;
}
+ (id)atlasController;
+ (void)_checkFreeDiskSpace;
+ (void)_assertUnsupportedColors;
+ (void)_assertUnsupportedResolution;
+ (id)generalScreen;
+ (void)_appWillTerminate:(id)aTerminate;
+ (void)hideSplashScreen;
+ (void)showSplashScreen;
- (LString *)hiResMapsRootFolderSourcePath;
- (id)_hiResMapsRootFolderSourcePath;
- (id)_hiResMapsRootFolderPathForMediaWithName:(id)aName;
- (void)insertDiscDialogAction:(id)aAction;
- (BOOL)application:(id)aApplication printFile:(id)aFile;
- (BOOL)application:(id)aApplication openFile:(id)aFile;
- (id)newLocationController;
- (id)settingsPaletteController;
- (id)iPhotoImportController;
- (id)movieController;
- (id)routeManagerPaletteController;
- (id)timeLineController;
- (id)citiesPaletteController;
- (id)preferencesController;
- (id)mainWindowController;
- (void)hideMainWindow;
- (void)showMainWindow;
- (void)gotoGetMoreRoutes:(id)aRoutes;
- (void)gotoTechSupport:(id)aSupport;
- (void)openUserGuide:(id)aGuide;
- (void)showSettingsHelp:(id)aHelp;
- (void)installHiResMaps:(id)aMaps;
- (void)registerAtlas:(id)aAtlas;
- (void)provideFeedback:(id)aFeedback;
- (id)_fullURLForPageURLString:(id)aString;
- (void)showAboutPanel:(id)aPanel;
- (void)showPreferences:(id)aPreferences;
- (void)toggleSnapshotEditorPalette:(id)aPalette;
- (void)toggleIPhotoMediaPalette:(id)aPalette;
- (void)toggleRouteManagerPalette:(id)aPalette;
- (void)hideRouteManagerPalette:(id)aPalette;
- (void)showRouteManagerPalette:(id)aPalette;
- (void)toggleCitiesInfoPalette:(id)aPalette;
- (void)hideCitiesInfoPalette:(id)aPalette;
- (void)showCitiesInfoPalette:(id)aPalette;
- (void)hideStatusMenu:(id)aMenu;
- (void)changeOnlineStatus:(id)aStatus;
- (void)browseTheEarth:(id)aEarth;
- (void)activateAtlas:(id)aAtlas;
- (void)_hiResMapsInstallationDidEnd:(id)aEnd;
- (void)_hiResMapsInstallationProgress:(id)aProgress;
- (void)_updateCopyingProgress;
- (void)_processSerialNumber;
- (void)_invalidateDockMenu;
- (id)applicationDockMenu:(id)aMenu;
- (void)setDockIconOverlayText:(id)aText;
- (BOOL)statusBarItemsWereInstalled;
- (void)hideStatusBarItems;
- (void)installStatusBarItems;
- (id)_menuForStatusBar;
- (BOOL)validateMenuItem:(id)aItem;
- (void)servicesLookup:(id)aLookup userData:(id)aData error:(id *)aError;
- (void)_openSearchForTerm:(id)aTerm;
- (void)dealloc;
- (void)awakeFromNib;
- (id)init;
@end

@interface AtlasController (AtlasControllerDockWeather)
- (void)_updateDockWeather:(id)aWeather;
- (void)_selectHomeCityFromDock:(id)aDock;
- (void)_dockEntryWasSelected:(id)aSelected;
- (void)_homeCityWasChanged:(id)aChanged;
- (void)_dockReportsWereChanged:(id)aChanged;
- (void)_unitsOfMeasureWereChanged:(id)aChanged;
- (void)_showWeatherInDockStateWasChanged:(id)aChanged;
- (void)_finilizeDockWeather;
- (void)_initDockWeather;
- (void)_hideDockWeather;
- (void)_showDockWeather;
- (void)_unscheduleDockEntry;
- (void)_scheduleDockEntry;
- (void)_refreshDockWeather;
@end

@interface NSAttributedString (NSAttributedStringDockIconText)
+ (id)stringForTemperature:(CGFloat)aTemperature inCelsius:(BOOL)aCelsius;
@end

