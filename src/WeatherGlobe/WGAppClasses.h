// The application's C++ classes: the model callbacks (AppController), the
// main window's interface to RenderLib, the entries list data source with a
// preview panel, and the helpers of the movie and image export.
#pragma once

#include "RenderLib.h"
#include "CZipReader.h"
#include "IAlertCallback.h"

#ifdef __OBJC__
@class MainWindowController;
@class WGMainEntriesListController;
@class WGMovieController;
@class NSBitmapImageRep;
#else
class MainWindowController;
class WGMainEntriesListController;
class WGMovieController;
class NSBitmapImageRep;
#endif

// The application's side of the libraries: the alert, image and model
// callbacks, and the database update's delegate (WGDBUpdateController shows
// the progress). The singleton starts AppModel; it owns the resource file
// manager (Contents/Resources/BASE) and the support and caches folders.
class AppController : public LObject, public IAlertCallback, public IModelCallbacks, public ILImageCallback,
                      public DownloadManager::IDBUpdateDelegate {
	LF_DECLARE_CLASS(AppController)
public:
	AppController() : _cachesPath(0), _sharedDataPath(0), _userDataPath(0), _fileManager(0) {}
	virtual ~AppController() {}
	virtual void initAsSingleton(void *param);
	virtual void dealloc();
	virtual AppController *getOwnerLObject() { return this; }
	virtual intptr_t _dbUpdatesAreAvailable(char *args);

	// DownloadManager::IDBUpdateDelegate
	virtual void updateWillStartWithFilesCount(int count);
	virtual void updateWasFinished(unsigned char success);
	virtual bool fileWillBeDownloadedWithName(LString *name, LConnection *connection);
	virtual int fileWasDownloadedWithName(LString *name, int status);
	virtual int errorWithCodeCatched(int status, LData *data);

	// IModelCallbacks
	virtual int _applicationBuildNumber();
	virtual void _applicationTerminate(int code);
	virtual void _applicationRestart();
	virtual void _openURL(LString *url);
	virtual LString *_applicationDataPath(unsigned char shared);
	virtual LString *_applicationCachesPath(unsigned char create);
	virtual bool _showSubscriptionDialog();
	virtual LMap *_registrationInfo();
	virtual void _showSelectCityDialog();
	virtual bool _showSerialNumberDialog();
	virtual LString *_serialNumber();
	virtual LString *_hiResMapsRootFolderSourcePath();

	// IAlertCallback
	virtual void _fillParamsFromTheCode(SAlertParams *params, int code);
	virtual int _showAlert(const SAlertParams *params);

	// ILImageCallback
	virtual CGImageRef _CGImageFromImageData(LData *data);
	virtual LData *_representationOfTypeForBitmapImage(TBitmapRepresentationType type, LBitmapImage *image);

	void resetFileManager();
	LString *_pathToFolderWithType(int folderType, unsigned char shared);
	bool _runAuthorizedTasks(LArray *tasks);
	bool _checkSupportFolderPermissions(LString *path, unsigned char shared);

	LString *_cachesPath;
	LString *_sharedDataPath;	// the local domain's
	LString *_userDataPath;
	CZipFileManager *_fileManager;
};

// The main window's view: RenderLib asks it about the current set, the
// popups, the cursor and the scale slider.
class AppWindowInterfaceWrapper : public BaseWindowInterface {
public:
	AppWindowInterfaceWrapper(MainWindowController *owner);
	virtual ~AppWindowInterfaceWrapper() {}
	virtual void toogleFullScreen();
	virtual bool entryIsMarked(GenericEntry *entry);
	virtual void appendEntryToSet(GenericEntry *entry);
	virtual void entryWasClicked(GenericEntry *entry, LPoint point, TBillboardKind kind);
	virtual void locationWasPicked(LPoint geoPoint);
	virtual void stopAnyFlying();
	virtual void setScaleValue(float scale);
	virtual void setMinimapWorldRect(LRect rect);
	virtual void compassValueChangedTo(TCompassValue value);
	virtual void setMouseCursorType(TMouseCursorType type);
	virtual void showPopupEntryInfo(GenericEntry *entry);
	virtual bool isPopupEntryInfoVisible();
	virtual void hidePopupEntryInfo();
	virtual bool pointEntriesOnMap();

	MainWindowController *_owner;
};

// The main entries list's data source: the preview panel is the list
// controller's advanced (search results) tab.
class AdvancedEntriesListDataSource : public EntriesListDataSource {
	LF_DECLARE_CLASS(AdvancedEntriesListDataSource)
public:
	AdvancedEntriesListDataSource() : _entriesListController(0) {}
	virtual AdvancedEntriesListDataSource *getOwnerLObject() { return this; }
	virtual bool isPreviewPanelShown();
	virtual void showPreviewPanel();
	virtual void hidePreviewPanel();
	virtual void reloadGenericList();

	void setEntriesListController(WGMainEntriesListController *controller);

	WGMainEntriesListController *_entriesListController;
};

// Stops the movie recording when RenderLib says it must stop.
class _MovieStopper : public LObject {
	LF_DECLARE_CLASS(_MovieStopper)
public:
	_MovieStopper() : _controller(0) {}
	virtual void dealloc();
	virtual _MovieStopper *getOwnerLObject() { return this; }
	virtual intptr_t _onMovieRecordShouldStop(char *args);

	_MovieStopper *initWithController(WGMovieController *controller);

	WGMovieController *_controller;
};

// Passes MovieExporter's progress and errors to the movie controller.
class _MovieExporterCallbacksHandler : public LObject {
	LF_DECLARE_CLASS(_MovieExporterCallbacksHandler)
public:
	_MovieExporterCallbacksHandler() : _controller(0), _unused18(0), _unused1c(0) {}
	virtual void dealloc();
	virtual _MovieExporterCallbacksHandler *getOwnerLObject() { return this; }
	virtual intptr_t _onExportProgress(char *args);
	virtual intptr_t _onExportFinished(char *args);

	_MovieExporterCallbacksHandler *initWithController(WGMovieController *controller);

	WGMovieController *_controller;
	int _unused18;
	int _unused1c;
};

// Collects a grabbed back buffer into a 24-bit RGB NSBitmapImageRep.
class _bitmapRepGrabCallback : public StBBGrabbingAsync::IGrabCB {
public:
	_bitmapRepGrabCallback() : _rep(0), _width(0), _height(0) {}
	virtual void setOutSize(LSize size);
	virtual void dataIsReady(const void *bytes, unsigned int firstRow, unsigned int rowsCount, unsigned int rowBytes);

	NSBitmapImageRep *_rep;	// retained
	unsigned int _width;
	unsigned int _height;
};

#ifdef __OBJC__
// The main entries list's tab identifiers.
extern NSString *_kEntriesTableViewID;
extern NSString *_kAdvancedEntriesTableViewID;
// The dragged entries' pasteboard types: their dictionaries, and their
// geo points (LPoints).
extern NSString *_kEntriesListPasteboardType;
extern NSString *_kPointsPasteboardType;
extern NSString *_kEntriesListDraggingEndedNotification;
// The entries lists' column identifiers.
extern NSString *_kIconColumnName;
extern NSString *_kNameColumnIdentifier;
extern NSString *_kPopulationColumnIdentifier;
extern NSString *_kTypeColumnIdentifier;
extern NSString *_kRegionColumnIdentifier;
extern NSString *_kMarkColumnIdentifier;
extern NSString *HSScrollViewReflectScrolledClipViewNotification;
extern NSString *_kWGColorSelectionCellColorKey;
extern NSString *_kWGColorSelectionCellImageKey;
extern NSString *_kImageTextTableCellBackgroundColorKey;
extern NSString *_kImageTextTableCellImageKey;
extern NSString *_kImageTextTableCellTextAttributesKey;
extern NSString *_kImageTextTableCellTextKey;
// Sorts views by their orderWeight (WGTitledViewContainer).
NSInteger _compareOrderWeight(id first, id second, void *context);
extern NSString *_kWGCustomButtonCellScaleKey;
extern NSString *_kWGCustomButtonCellAlignKey;
// Desktop picture and mail helpers (WGDBUpdateController.mm).
void wgSetDesktopImageAtPath(NSString *path);
void wgCreateMailWithAttachment(NSString *path, NSString *text);
// The attributes of a WGTitledView content view (WGTitledView.mm).
extern NSString *_kWGViewOrderWeightKey;
extern NSString *_kWGViewTitleViewKey;
extern NSString *_kWGViewImageKey;
extern NSString *_kWGViewTitleKey;
extern NSString *_kWGViewMaxSizeKey;
extern NSString *_kWGViewMinSizeKey;
extern NSString *_kWGCustomButtonCellAltImageKey;
extern NSString *_kWGCustomButtonCellImageKey;
// The fields of the details panes are measured in this.
static const NSSize WGEntryDetailsInfoDefaultFieldSize = {100.0, 10000.0};
// Posted (with the control view) when a WGTextFieldCell starts editing.
extern NSString *_kEditCellDidStartEditingNotification;
// Posted by WGPaletteBaseController with itself as the object.
extern NSString *_WGPaletteDidBecomeKey;
extern NSString *_WGPaletteDidCloseNotification;
extern NSString *_WGNewLocationCoordsPickerModeChangedNotification;
// The keys of WGEntryInfoPopupView's info rows (defined in WGEntryInfoPopupView.mm).
extern NSString *_kWGEntryInfoPopupTitleKey;
extern NSString *_kWGEntryInfoPopupValueKey;
// The route files' extension, "smroute" (defined in WGRouteImportExport.mm).
extern NSString *_kRouteSetFileExt;
extern NSString *_WGCurrentSetEntryIndexesPasteboardType;
extern NSString *_WGCurrentSetRouteWasChangedNotification;
extern NSString *_WGCurrentSetRouteWasAddedNotification;
// Posted by CityInfoView when its size changes (defined in CityInfoController.mm).
extern NSString *_kCIViewSizeDidChangeNotification;
extern NSString *_WGEntryObjectKey;
extern NSString *_WGEntryDetailsCurrentEntryDidChangeNotification;
extern NSString *_WGEntryDetailsControllerWillCommitEntryNotification;
extern NSString *_WGEntryDetailsControllerAttributesDidChangeNotification;
extern NSString *_WGEntryDetailsControllerEditModeDidChangeNotification;
extern NSString *_WGEntryDetailsCurrentEntryWillChangeNotification;
// Keys of iPhoto's AlbumData.xml (defined in IPhotoPhoto.mm).
extern NSString *_ipSelectedAlbumIndexKey;
extern NSString *_ipMajorVersionKey;
extern NSString *_ipSongPathKey;
extern NSString *_ipPhotosListKey;
extern NSString *_ipAlbumsListKey;
extern NSString *_ipArchivePathKey;
extern NSString *_ipPhotoNameKey;
extern NSString *_ipThumbPathKey;
extern NSString *_ipPhotoPathKey;
extern NSString *_ipAlbumKeysListKey;
extern NSString *_ipAlbumPhotosKey;
extern NSString *_ipMasterKey;
extern NSString *_ipAlbumIDKey;
extern NSString *_ipAlbumTypeKey;
extern NSString *_ipAlbumNameKey;
// The iPhoto preference keys holding the recent libraries (defined in IPhotoHelper.mm).
extern NSString *_iPhotoRecentDatabasesKey;
extern NSString *_iPhotoRecentDatabases1Key;
#endif

#ifdef __OBJC__
// LFoundation's geometry is float, AppKit's CGFloat (double on 64-bit): the
// i386 binary passed one as the other.
static inline LPoint LPointFromNS(NSPoint p) { LPoint r = { (float)p.x, (float)p.y }; return r; }
static inline LSize LSizeFromNS(NSSize s) { LSize r = { (float)s.width, (float)s.height }; return r; }
static inline LRect LRectFromNS(NSRect r) { return LMakeRect(r.origin.x, r.origin.y, r.size.width, r.size.height); }
static inline NSPoint NSPointFromL(LPoint p) { return NSMakePoint(p.x, p.y); }
static inline NSSize NSSizeFromL(LSize s) { return NSMakeSize(s.width, s.height); }
static inline NSRect NSRectFromL(LRect r) { return NSMakeRect(r.origin.x, r.origin.y, r.size.width, r.size.height); }
#endif

// -1, 0 or 1.
int sign(int value);
