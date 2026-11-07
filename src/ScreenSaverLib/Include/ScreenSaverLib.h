// ScreenSaverLib: the screen saver side of the application, loaded from the
// app's Frameworks by the .saver: the ScreenSaverView (WGScreenSaverView),
// its configure sheet, and the model callbacks and window interface that
// RenderLib's saver panels run with.

#ifndef SCREENSAVERLIB_H
#define SCREENSAVERLIB_H

#include "RenderLib.h"
#include "CZipReader.h"

// The saver's window: no entries, no popups, no picking.
class ScreenSaverInterface : public BaseWindowInterface {
public:
	ScreenSaverInterface(void *nativeView, CDeviceFormat *format) : BaseWindowInterface(nativeView, format) {}
	virtual void toogleFullScreen() {}
	virtual bool entryIsMarked(GenericEntry *) { return false; }
	virtual void appendEntryToSet(GenericEntry *) {}
	virtual void locationWasPicked(LPoint) {}
	virtual void stopAnyFlying() {}
	virtual void setScaleValue(float) {}
	virtual void setMinimapWorldRect(LRect) {}
	virtual void compassValueChangedTo(TCompassValue) {}
	virtual void setMouseCursorType(TMouseCursorType) {}
	virtual void showPopupEntryInfo(GenericEntry *) {}
	virtual bool isPopupEntryInfoVisible() { return false; }
	virtual void hidePopupEntryInfo() {}
	virtual GenericEntry *currentEntry() { return 0; }
	virtual LArray *currentEntries() { return 0; }
	virtual bool pointEntriesOnMap() { return false; }
	virtual bool isGeoPosMode() { return false; }
};

// Starts the libraries for the saver and keeps one ScreenSaverInterface per
// system view.
class ScreenSaverController : public IModelCallbacks {
public:
	ScreenSaverController();
	virtual ~ScreenSaverController();

	virtual int _applicationBuildNumber();
	virtual void _applicationTerminate(int) {}
	virtual void _applicationRestart() {}
	virtual void _openURL(LString *) {}
	// The saver's support folder: the local domain's with `shared`, else the
	// user's.
	virtual LString *_applicationDataPath(unsigned char shared);
	virtual LString *_applicationCachesPath(unsigned char create);
	virtual bool _showSubscriptionDialog() { return false; }
	virtual LMap *_registrationInfo() { return Preferences::registrationInfo(); }
	virtual void _showSelectCityDialog() {}
	virtual bool _showSerialNumberDialog() { return false; }
	virtual LString *_serialNumber() { return 0; }
	virtual LString *_hiResMapsRootFolderSourcePath() { return 0; }

	virtual bool createAcceleratedView(void *systemView, CDeviceFormat *format);
	virtual bool destroyAcceleratedView(void *systemView);

	void openPanel(int mode, void *systemView);

	static LString *_screenSaverLibBundleID();
	static LString *_resourcePath();
	static void _validateFrameworks();

	CZipFileManager *_fileManager;
	CHashTable _interfaces;	// system view -> ScreenSaverInterface
	LString *_cachesPath;
	LString *_sharedDataPath;
	LString *_userDataPath;
};

#ifdef __OBJC__
#import <ScreenSaver/ScreenSaver.h>
#include "C3DViewWrapper.h"

// The 3D view, without the mouse.
@interface ScreenSaverGLView : C3DViewWrapper
@end

@interface WGConfigPanelController : NSWindowController {
	IBOutlet NSMatrix *_targetMatrix;
	IBOutlet NSView *_containerView;
	IBOutlet NSView *_satelliteView;
	IBOutlet NSView *_outerSpaceView;
	IBOutlet NSView *_optionsView;
	IBOutlet NSView *_optionsContainerView;
	IBOutlet NSView *_optionsPlaceHolderView;
	IBOutlet NSButton *_collapseButt;
	IBOutlet NSMatrix *_optionsMatrix;
	IBOutlet NSButton *_planetNamesButt;
	IBOutlet NSButton *_cloudsHistoryButt;
	IBOutlet NSButton *_useMetricUnitsButt;
	IBOutlet NSMatrix *_satelliteOptionsMatrix;
	IBOutlet NSSlider *_flySpeedSlider;
	IBOutlet NSSlider *_zoomSlider;
	IBOutlet NSSlider *_globeTransparencySlider;
	IBOutlet NSButton *_tiltTheEarthButt;
	IBOutlet NSSlider *_spinningSpeedSlider;
	NSView *_currentSettingsView;
	NSSlider *_flySpinSpeedSlider;	// of the current settings view
	float _placeHolderViewHeight;
}
- (IBAction)configDone:(id)sender;
- (IBAction)selectTarget:(id)sender;
- (IBAction)toggleAdvancedOptions:(id)sender;
- (void)_updateUIFromDefaults;
- (void)_updateDefaultsFromUI;
- (void)_validateSettingsView;
@end

@interface WGScreenSaverView : ScreenSaverView {
	NSWindowController *_configWindowController;
	ScreenSaverGLView *_glView;
}
- (void)_addScreenSaverViewIfNeeded;
@end
#endif

#endif
