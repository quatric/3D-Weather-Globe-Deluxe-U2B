// WebCamsDB_C: the web camera collection, a Core Data style SQLite database
// shipped with the app (WebCamCollection.sql) and copied to Application
// Support, where downloaded updates ("wc*" files, compressed SQL dumps) are
// applied to it. The C functions are the API the app uses.

#ifndef WEBCAMSDB_H
#define WEBCAMSDB_H

#include <CoreFoundation/CoreFoundation.h>

typedef struct _opaqueWebCam *WebCamRef;
typedef struct _opaqueWebCamsContainer *WebCamsContainerRef;
typedef struct _opaqueWebCamsTree *WebCamsTreeRef;

extern "C" {
// The manager opens (or first copies) the database. These return whether it
// is open. The second form gives the cached and bundled database paths and the
// folder updates are downloaded to.
bool InitializeWebCamsManager(CFStringRef appKey);
bool InitializeWebCamsManager2(CFStringRef appKey, CFStringRef dbFilePathBundled, CFStringRef dbFilePathCached,
                               CFStringRef updatesFolderPath);
void FinalizeWebCamsManager();
// Applies new update files from the updates folder.
void WCManagerRescanForUpdates();
WebCamsContainerRef WCManagerGetAllWebCamsFlat();
WebCamsTreeRef WCManagerGetRootCategories();
// Cameras matching the search terms ("and", "or" and "not" are understood).
WebCamsContainerRef WCManagerGetFilteredWebCams(CFStringRef terms);
// Cameras for a World Book article.
WebCamsContainerRef WCManagerGetWebCamsForWBRIN(CFStringRef rin);

void WCContainerRetain(WebCamsContainerRef container);
void WCContainerRelease(WebCamsContainerRef container);
int WCContainerCamerasCount(WebCamsContainerRef container);
WebCamRef WCContainerCameraAtIndex(WebCamsContainerRef container, int index);

void WCTreeRetain(WebCamsTreeRef tree);
void WCTreeRelease(WebCamsTreeRef tree);
WebCamsContainerRef WCTreeGetWebCams(WebCamsTreeRef tree);
int WCTreeChildrenCount(WebCamsTreeRef tree);
WebCamsTreeRef WCTreeChildAtIndex(WebCamsTreeRef tree, int index);
CFStringRef WCTreeTitle(WebCamsTreeRef tree);

void WebCamRetain(WebCamRef cam);
void WebCamRelease(WebCamRef cam);
WebCamRef WebCamWithUID(CFStringRef uid);
WebCamRef WebCamWithRin(int rin);
CFStringRef WebCamUID(WebCamRef cam);
int WebCamRin(WebCamRef cam);
CFStringRef WebCamTitle(WebCamRef cam);
int WebCamUpdateFrequency(WebCamRef cam);
CFDataRef WebCamPreviewImageData(WebCamRef cam);
CFDataRef WebCamDescription(WebCamRef cam);
CFStringRef WebCamDescriptionString(WebCamRef cam);
CFStringRef WebCamGetWBRINs(WebCamRef cam);
CFStringRef WebCamPageTitle(WebCamRef cam);
CFStringRef WebCamPageURLString(WebCamRef cam);
CFStringRef WebCamImageURLString(WebCamRef cam);
CFStringRef WebCamCopyrightString(WebCamRef cam);
}

WebCamsTreeRef WCTreeSuperObject(WebCamsTreeRef tree);

#endif
