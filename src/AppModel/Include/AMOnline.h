// The online part of the model: the download queue and the authorization with
// the MacKiev server, the plug-in that makes the weather service's URLs, the
// weather reports of the entries and the satellite clouds.

#ifndef AMONLINE_H
#define AMONLINE_H

#include "AMBase.h"
#include "LConnection.h"
#include "LXMLTree.h"
#include "FHTextures.h"

class CloudsManager;
class ImagesSequence;

// ---------------------------------------------------------------------------
// The URL creator plug-in. Its library is copied into a bundle in the
// application data folder (the server can replace it) and loaded from there.

class DynamicEnvironment : public LObject {
	LF_DECLARE_CLASS(DynamicEnvironment)
public:
	typedef LString *(*TCloudsURLFunc)(unsigned char west, LDate *date, int size);
	typedef LString *(*TReportURLFunc)(LPoint geoPoint, int report);
	typedef LString *(*TReportByIDURLFunc)(int customWeatherID, int report);
	typedef LData *(*TDataFunc)(LData *data);
	typedef int (*TVersionFunc)();
	typedef LString *(*TStringFunc)();

	DynamicEnvironment()
	    : _bundle(0), _getURLForClouds(0), _getURLForReport(0), _getURLForReportByID(0), _getXMLForReportData(0),
	      _getJPGForCloudsData(0), _getURLDLLVersion(0), _getReportUN(0), _getReportP(0), _getMapsUN(0),
	      _getMapsP(0)
	{
	}
	virtual void initAsSingleton(void *param);
	virtual void dealloc();
	virtual DynamicEnvironment *getOwnerLObject() { return this; }

	void replaceDYLIBWithNew(LData *body);
	LData *jpegDataForCloudsData(LData *data);
	LData *xmlDataForReport(TReportType type, LData *data);
	LString *mapsP();
	LString *mapsUN();
	LString *reportP();
	LString *reportUN();
	LString *URLForReportDownload(TReportType type, int customWeatherID);
	LString *URLForReportDownload(TReportType type, LPoint geoPoint);
	LString *URLForClouds(unsigned char west, LDate *date, int size);
	int currentVersion();

	static LString *_pathBundled();
	static LString *_dylibRootPath();
	static LString *_pathCached();
	void _unloadBundle();
	void _createPluginBundleWithDYLIBBody(LData *body, const LString *path);
	void _loadBundleIfNeeded();

	CFBundleRef _bundle;
	TCloudsURLFunc _getURLForClouds;
	TReportURLFunc _getURLForReport;
	TReportByIDURLFunc _getURLForReportByID;
	TDataFunc _getXMLForReportData;
	TDataFunc _getJPGForCloudsData;
	TVersionFunc _getURLDLLVersion;
	TStringFunc _getReportUN;
	TStringFunc _getReportP;
	TStringFunc _getMapsUN;
	TStringFunc _getMapsP;
};

// ---------------------------------------------------------------------------
// The queue of downloads. Tasks are maps of parameters (URL, method, user
// and password) run a few at a time once the application is authorized with
// the MacKiev server; their delegates get the data. It also checks for and
// downloads the database updates.
//
// Services that take more than one request (AccuWeather's reports, the live
// cloud maps) have URLs of their own schemes, made by the URL creator plug-in
// and fetched by OnlineServices; they need no authorization.

class DownloadManager : public LObject {
	LF_DECLARE_CLASS(DownloadManager)
public:
	enum TASK_ACTION { kTaskActionKeep = 0, kTaskActionRemove = 1 };
	enum { kUpdateActionStop = 0, kUpdateActionRetry = 1, kUpdateActionNext = 2 };

	class IDownloadDelegate {
	public:
		// `data` is 0 unless `status` is 200.
		virtual void downloadFinishedForTask(LMap *params, LData *data, int status) = 0;
		virtual ~IDownloadDelegate() {}
	};

	class IDBUpdateDelegate {
	public:
		virtual void updateWillStartWithFilesCount(int count) = 0;
		virtual void updateWasFinished(unsigned char success) = 0;
		// Returns false to keep the file from being fetched now.
		virtual bool fileWillBeDownloadedWithName(LString *name, LConnection *connection) = 0;
		// Returns a kUpdateAction.
		virtual int fileWasDownloadedWithName(LString *name, int status) = 0;
		virtual int errorWithCodeCatched(int status, LData *data) = 0;
		virtual ~IDBUpdateDelegate() {}
	};

	DownloadManager()
	    : _authorized(false), _authorizing(false), _authorizationDone(false), _authorizationInvalid(false),
	      _offline(false), _showSubscription(false), _paused(false), _refreshAttempts(0), _updateFiles(0),
	      _updateDelegate(0), _unused24(0), _tasks(0), _maxConnections(4), _unused30(0), _connections(0),
	      _activationConnection(0), _refreshConnection(0), _authConnection(0), _getObjectConnection(0),
	      _sortInstalled(false)
	{
	}
	virtual void dealloc();
	virtual DownloadManager *getOwnerLObject() { return this; }
	virtual intptr_t _connectionClosed(char *args);
	virtual intptr_t _authRequestCompleted(char *args);
	virtual intptr_t _activateRequestCompleted(char *args);
	virtual intptr_t _dylibLoadCompleted(char *args);
	virtual intptr_t _refreshRequestCompleted(char *args);
	virtual intptr_t _getObjectRequestCompleted(char *args);

	static bool isInternetAvailable();
	static LString *_hostName();
	static LString *updatesDownloadFolder();
	static LString *_registrationCode();
	static bool _registrationIsValid();
	static LConnection *_prepareRegistrationRequest();

	void invalidate();
	void terminateUpdating();
	void pauseQueue();
	void resumeQueue();
	// `quietly` is set when the user was already told.
	bool authorizationFailed(unsigned char *quietly);
	bool isAuthorizing();
	bool updatesAreAvailable();
	bool updateWithDelegate(IDBUpdateDelegate *delegate, LComparableSet *skip);
	void refreshDatabase();
	void postActivationTask(LString *serial);
	void setTaskQueueSortCallback(CArrayComparator compare);
	void enumerateTasksWithCallback(TASK_ACTION (*callback)(LMap *params, void *context), void *context);
	void postDownloadTaskWithParameters(LMap *params, IDownloadDelegate *delegate, unsigned char urgent);
	void setMaxSimultaneousDownloads(int count);

	void _taskFinishedWithInfo(LMap *task, LData *data, int status);
	void _clearTasksQueue(int status);
	LConnection *_prepareGetObjectRequest(LString *name);
	void _downloadNextUpdateFile();
	LConnection *_prepareActivationRequest(LString *serial);
	void _processUpdatesFromData(LData *data);
	void _startAuthorization();
	void _openNextConnection();
	bool _processActions(LArray *actions);
	bool _runAlertWithRep(LMap *rep);

	static LString *kTaskParameterMethod;
	static LString *kTaskParameterPassword;
	static LString *kTaskParameterUsername;
	static LString *kTaskParameterRequestString;
	static LString *kActivationCompleteNotification;
	static LString *kNewUpdatesAreAvailableNotification;
	static LConstantString sTaskParameterMethod;
	static LConstantString sTaskParameterRequestString;
	static LConstantString sNewUpdatesAreAvailableNotification;

	bool _authorized;	// the server let the application in
	bool _authorizing;
	bool _authorizationDone;	// an answer came, good or bad
	bool _authorizationInvalid;	// the server said the registration is invalid
	bool _offline;	// the last authorization found no internet
	bool _showSubscription;
	bool _paused;
	int _refreshAttempts;
	LArray *_updateFiles;	// names of the update files still to download
	IDBUpdateDelegate *_updateDelegate;
	int _unused24;
	LArray *_tasks;	// queued tasks
	int _maxConnections;
	int _unused30;
	LArray *_connections;	// running tasks' connections
	LConnection *_activationConnection;
	LConnection *_refreshConnection;
	LConnection *_authConnection;
	LConnection *_getObjectConnection;
	bool _sortInstalled;
};

int _compareWithOtherTaskInfo(const void *a, const void *b, const void *context);

// ---------------------------------------------------------------------------
// The requests of the services that URLCreator makes URLs of its own
// schemes for (OnlineServices.mm).

namespace OnlineServices {
// Whether the URL is one of these services'.
bool handlesURL(LString *url);
// Fetches it and posts the connection's closed notification for the
// connection, as if it had fetched it, with the status and data.
void fetch(LString *url, LConnection *connection);
}

// ---------------------------------------------------------------------------
// The weather reports of one entry: kept in files in the application data
// folder, refreshed while they are scheduled.

class CWOnlineCityData : public LObject {
	LF_DECLARE_CLASS(CWOnlineCityData)
public:
	enum { kReportsCount = 30 };

	CWOnlineCityData() : _entry(0), _fileName(0), _scheduledReports(0), _cache(0)
	{
		memset(_scheduledCounts, 0, sizeof _scheduledCounts);
		memset(_downloading, 0, sizeof _downloading);
	}
	virtual void dealloc();
	virtual CWOnlineCityData *getOwnerLObject() { return this; }
	virtual intptr_t _downloadComplete(char *args);

	CWOnlineCityData *initWithPointEntry(PointEntry *entry);
	void invalidate();
	bool downloadingReport(TReportType type);
	void scheduleReports(unsigned int mask);
	void unscheduleReports(unsigned int mask);
	void singleUpdateTaskForScheduledReports();
	void manualUpdateOfScheduledReports();
	void singleUpdateTaskForReports(unsigned int mask);
	LString *filePathForReport(TReportType type);
	LMap *dictionaryForReport(TReportType type);

	static LString *_keyForReport(TReportType type);
	static LString *_filePathForReport(TReportType type, LString *fileName);
	static LString *_filePathForExpiredReport(TReportType type, LString *fileName);
	void _incScheduledReportsCount(unsigned int mask);
	unsigned int _decScheduledReportsCount(unsigned int mask);
	void _postDownloadTaskForReport(TReportType type);
	void _cancelDownloadTaskForReport(TReportType type);
	void _postReportWasExpiredNotificationForReport(TReportType type);
	void _postReportIsReadyNotificationForReport(TReportType type);
	double _lastUpdateTimeForReport(TReportType type);
	double _lastUpdateTimeForExpiredReport(TReportType type);
	LMap *_dictionaryForReport(TReportType type);
	void _singleUpdateTaskForReports(unsigned int mask, unsigned char force, unsigned char notify);

	static float kReportsRefreshTime[kReportsCount];	// hours
	static LMap *sReportFolders;	// report type - 1 -> folder
	static LString *kReportIDKey;
	static LString *kReportDictionaryKey;

	PointEntry *_entry;
	LString *_fileName;	// "name(uid)"
	unsigned int _scheduledReports;
	int _scheduledCounts[kReportsCount];
	bool _downloading[kReportsCount + 2];
	LMap *_cache;	// report key -> report, or LNull
};

extern int kReportByIndex[CWOnlineCityData::kReportsCount];

// ---------------------------------------------------------------------------
// The weather manager: downloads the reports and parses them, keeps the
// scheduled entries up to date and downloads the clouds.

class CWManager : public LObject, public DownloadManager::IDownloadDelegate {
	LF_DECLARE_CLASS(CWManager)
public:
	CWManager()
	    : _entriesTimer(0), _scheduledEntries(0), _lastCloudsUpdate(0), _cloudsTimer(0), _cloudMapsInQueue(0),
	      _cloudsManager(0), _unused30(0), _unused34(0), _weatherImages(0), _windImages(0), _compressionQueue(0),
	      _weatherInternetState(0), _compressionFailed(false)
	{
	}
	virtual void initAsSingleton(void *param);
	virtual void dealloc();
	virtual CWManager *getOwnerLObject() { return this; }
	virtual void downloadFinishedForTask(LMap *params, LData *data, int status);
	virtual intptr_t _internetStateWasChanged(char *args);
	virtual intptr_t _cloudsUpdateTimerProc(char *args);
	virtual intptr_t _mapCompressionComplete(char *args);
	virtual intptr_t _entriesTimerProc(char *args);

	void invalidate();
	void resetWeatherInternetState();
	bool isWeatherInternetStateOK();
	int cloudMapsCountInDownloadQueue();
	void updateScheduledEntries();
	void scheduleEntry(PointEntry *entry);
	void unscheduleEntry(PointEntry *entry);
	// The target's selector gets {kReportIDKey: type, kReportDictionaryKey: report}.
	void postLoadTaskForEntry(PointEntry *entry, TReportType type, LObject *target, const char *sel);
	void cancelLoadTaskForEntry(PointEntry *entry, TReportType type);
	void scheduleCloudsUpdateTask();
	void unscheduleCloudsUpdateTask();
	void scheduleCloudsReloadTask();
	CloudsManager *cloudsManager() const { return _cloudsManager; }
	LArray *weatherImagesForIconAtIndex(int index);
	LArray *windImagesForIconAtIndex(int index);

	static LMap *fullReportFromCurrentConditionsData(LData *data);
	static LMap *parseXMLData(LData *data, TReportType type, PointEntry *entry);
	static void _addStringIfExistsInDictionary(LMap *from, LString *key, LMap *to, LString *toKey);
	static void _processWeatherDictionary(LMap *from, LMap *to);
	static void _parseExtendedReport(LXMLTree *tree, LMap *report);
	static void _parseAstronomyReport(LXMLTree *tree, LMap *report);
	static void _parseCurrentReport(LXMLTree *tree, LMap *report, PointEntry *entry);
	static void _parseExpandedReport(LXMLTree *tree, LMap *report);
	static LXMLTree *_subtreeWithDescriptor(LXMLTree *tree, LString *descriptor);
	static LMap *_headerForReport(LXMLNode *node);
	static TLArray<LMap *> *_allAttributesWithDescriptor(LString *descriptor, LXMLTree *tree);
	static void _addFirstEntryWithKey(LString *key, TLArray<LMap *> *items, LMap *to);
	static void _addFirstNonZeroEntryWithKey(LString *key, TLArray<LMap *> *items, LMap *to);
	static void _addAverageForEntryWithKey(LString *key, TLArray<LMap *> *items, LMap *to);

	void _startCloudsUpdate();
	void _stopCloudsUpdate();
	void _clearCloudsLoadingQueue();
	void _postCloudsRequest(LDate *date, unsigned char east, LString *fileName);
	void _postTextureCompressionTask(LData *data, LString *path, unsigned char synchronous);
	void _mapLoadComplete(LMap *params, LData *data);
	void _mapDownloadFinishedForTask(LMap *params, LData *data, int status);
	void _reportDownloadFinishedForTask(LMap *params, LData *data, int status);

	static LString *kRequestKind;
	static LString *kRequestKindMAP;
	static LString *kRequestKindReport;
	static LString *kCloudsWereUpdatedNotification;
	static LString *kWeatherInternetStateWasChangedNotification;

	LTimer *_entriesTimer;
	LSet *_scheduledEntries;
	LDate *_lastCloudsUpdate;
	LTimer *_cloudsTimer;
	LComparableSet *_cloudMapsInQueue;	// file names
	CloudsManager *_cloudsManager;
	LObject *_unused30;
	LObject *_unused34;
	ImagesSequence *_weatherImages;
	ImagesSequence *_windImages;
	LTaskQueue *_compressionQueue;
	int _weatherInternetState;	// 0 is OK
	bool _compressionFailed;
};

// ---------------------------------------------------------------------------
// The clouds: frames of two DXT1 textures (the west and east halves of the
// earth) a few hours apart, downloaded or the default ones.

class _CloudFrame : public LObject {
	LF_DECLARE_CLASS(_CloudFrame)
public:
	_CloudFrame() : _westPath(0), _eastPath(0), _format(D3DFMT_UNKNOWN), _backInTimeOffset(-1) {}
	virtual void dealloc();
	virtual _CloudFrame *getOwnerLObject() { return this; }

	_CloudFrame *initWithPaths(LString *westPath, LString *eastPath, CloudsManager *manager, int backInTimeOffset);
	LDate *date();
	LData *textureData(unsigned char east);

	static LMap *sLoadedClouds;	// path -> decompressed texture

	LString *_westPath;
	LString *_eastPath;
	D3DFORMAT _format;
	LSize _size;
	int _backInTimeOffset;	// hours
};

class CloudsManager : public LObject {
	LF_DECLARE_CLASS(CloudsManager)
public:
	CloudsManager() : _frames(0), _currentDate(0), _useDefaultClouds(true), _usesLastDownloaded(false),
	                  _frameFormat(D3DFMT_DXT1)
	{
	}
	virtual CloudsManager *init();
	virtual void dealloc();
	virtual CloudsManager *getOwnerLObject() { return this; }
	virtual intptr_t _cloudsTextureSizeWasChanged(char *args);
	virtual intptr_t _cloudsEnabledStateWasChanged(char *args);

	static LDate *defaultCloudsCurrentDate();
	static int _maxDownloadedCloudsFilesCount();
	static LString *mapFileNameForDate(LDate *date, unsigned char east);
	static LString *defaultCloudsFolder();
	static LString *downloadedCloudsFolder();
	static LArray *_lastDownloadedCloudsFileNames();
	static void deleteObsoleteFiles();

	void _invalFrames();
	void cleanup();
	void setFrameTextureFormat(D3DFORMAT format);
	void setUseDefaultClouds(unsigned char useDefault);
	LSize frameTextureSize();
	void _reloadFramesIfNeeded();
	LDate *currentCloudsDate();
	void updateTexture(C2DTexture *texture, int frame, unsigned char east);
	int framesCount();
	int frameIndexForBackInTimeOffset(int hours);
	int backInTimeOffsetForFrameAtIndex(int index);

	static LMap *sDownloadedCloudsFolders;	// texture width -> folder

	LArray *_frames;	// of _CloudFrame, the newest first
	LDate *_currentDate;
	bool _useDefaultClouds;
	bool _usesLastDownloaded;	// default clouds were asked for, but enough were downloaded
	D3DFORMAT _frameFormat;
};

// Compresses a downloaded JPEG into a DXT1 texture file.
class _CCloudTextureCompressionTask : public LTask {
	LF_DECLARE_CLASS(_CCloudTextureCompressionTask)
public:
	_CCloudTextureCompressionTask() : _manager(0), _failed(false), _data(0), _path(0) {}
	virtual void dealloc();
	virtual void execute();
	virtual _CCloudTextureCompressionTask *getOwnerLObject() { return this; }

	_CCloudTextureCompressionTask *initWithParams(LString *path, LData *data, LObject *target, const char *sel);

	CWManager *_manager;
	bool _failed;
	LData *_data;
	LString *_path;
};

// Reads a texture file into bitmaps of a format.
class CTextureDecompressionTask : public LTask {
	LF_DECLARE_CLASS(CTextureDecompressionTask)
public:
	CTextureDecompressionTask() : _context(0), _width(0), _height(0), _format(D3DFMT_UNKNOWN), _path(0), _results(0) {}
	virtual void dealloc();
	virtual void execute();
	virtual CTextureDecompressionTask *getOwnerLObject() { return this; }

	CTextureDecompressionTask *initWithParams(LString *path, D3DFORMAT format, LObject *target, const char *sel,
	                                          void *context);
	void cleanup();

	void *_context;
	unsigned int _width;
	unsigned int _height;
	D3DFORMAT _format;	// 0 for the file's own
	LString *_path;
	LArray *_results;	// of LData
};

// A strip of square icons with a few frames each, cut into images.
class ImagesSequence : public LObject {
	LF_DECLARE_CLASS(ImagesSequence)
public:
	ImagesSequence() : _images(0), _cache(0), _iconsPerRow(0), _framesCount(0), _iconsCount(0), _iconSize(1) {}
	virtual void dealloc();
	virtual ImagesSequence *getOwnerLObject() { return this; }

	ImagesSequence *initWithFiles(TLArray<LString *> *files, int iconSize);
	// The frames of the icon (1-based), as LBitmapImages.
	LArray *imagesForIconAtIndex(int index);
	LBitmapImage *_createImageForIconAtIndex(int index, int frame);

	LArray *_images;	// of LBitmapImage
	LMap *_cache;	// index -> LArray of frames
	int _iconsPerRow;
	int _framesCount;
	int _iconsCount;
	int _iconSize;
};

#endif
