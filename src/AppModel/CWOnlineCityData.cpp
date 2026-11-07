// CWOnlineCityData: the weather reports of one entry. Each report type is
// kept in a file in the application data folder; when it gets too old it is
// renamed with an "exp" extension and downloaded again. Reports are refreshed
// while they are scheduled (several views can schedule the same report).

#include "AppModel.h"

LF_IMPLEMENT_CLASS(CWOnlineCityData, 0)
LF_SELECTOR(CWOnlineCityData, _downloadComplete);

// The report type of each bit of the masks.
int kReportByIndex[CWOnlineCityData::kReportsCount] = { kReportTypeCurrent, kReportTypeForecast, kReportTypeExpanded,
	                                                     kReportTypeAstronomy, kReportTypeSevereWarnings };

float CWOnlineCityData::kReportsRefreshTime[kReportsCount] = { 0.25f, 6.0f, 6.0f, 24.0f, 0.25f };
LMap *CWOnlineCityData::sReportFolders = 0;

class _onlineCityDataCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		if (CWOnlineCityData::sReportFolders) {
			CWOnlineCityData::sReportFolders->release();
			CWOnlineCityData::sReportFolders = 0;
		}
	}
};

static _onlineCityDataCleanup _sOnlineCityDataCleanup;

void CWOnlineCityData::invalidate()
{
	release();
}

void CWOnlineCityData::dealloc()
{
	if (_cache) {
		_cache->release();
		_cache = 0;
	}
	if (_fileName) {
		_fileName->release();
		_fileName = 0;
	}
	LObject::dealloc();
}

LString *CWOnlineCityData::_keyForReport(TReportType type)
{
	return LString::stringWithFormat("%d", type);
}

void CWOnlineCityData::_incScheduledReportsCount(unsigned int mask)
{
	for (int i = 0; i < kReportsCount; i++) {
		if (mask & (1u << i))
			_scheduledCounts[i]++;
	}
}

bool CWOnlineCityData::downloadingReport(TReportType type)
{
	return _downloading[type];
}

void CWOnlineCityData::_postDownloadTaskForReport(TReportType type)
{
	if (_downloading[type])
		return;
	_downloading[type] = true;
	CWManager::instance()->postLoadTaskForEntry(_entry, type, this, "_downloadComplete");
}

void CWOnlineCityData::_cancelDownloadTaskForReport(TReportType type)
{
	if (!_downloading[type])
		return;
	_downloading[type] = false;
	CWManager::instance()->cancelLoadTaskForEntry(_entry, type);
}

// Returns the bits of the reports nobody schedules any more.
unsigned int CWOnlineCityData::_decScheduledReportsCount(unsigned int mask)
{
	for (int i = 0; i < kReportsCount; i++) {
		unsigned int bit = 1u << i;
		if ((mask & bit) == 0)
			continue;
		if (--_scheduledCounts[i] == 0)
			_cancelDownloadTaskForReport((TReportType)kReportByIndex[i]);
		else
			mask &= ~bit;
	}
	return mask;
}

void CWOnlineCityData::unscheduleReports(unsigned int mask)
{
	unsigned int done = _decScheduledReportsCount(mask);
	_scheduledReports &= ~done;
	// (The original made the manager here if there was none, which at exit
	// built one after the libraries were torn down. A new one has nothing
	// scheduled.)
	if (_scheduledReports == 0 && CWManager::classObject()->hasSingleton())
		CWManager::instance()->unscheduleEntry(_entry);
}

CWOnlineCityData *CWOnlineCityData::initWithPointEntry(PointEntry *entry)
{
	if (!LObject::init())
		return 0;
	_entry = entry;
	_cache = LMap::_alloc()->init();
	_fileName = LString::_alloc()->initWithFormat("%@(%@)", entry->ownName(0), entry->uid(0));
	_fileName->replaceOccurrencesOfString(LString::stringWithCString("/"), LString::stringWithCString("_"));
	_fileName->replaceOccurrencesOfString(LString::stringWithCString(":"), LString::stringWithCString("_"));
	return this;
}

void CWOnlineCityData::_postReportWasExpiredNotificationForReport(TReportType type)
{
	_entry->_wrapperDataWasExpired(LMap::mapWithObjectsAndKeys(LValue::valueWithInt(type), kReportIDKey, (LObject *)0));
}

void CWOnlineCityData::_postReportIsReadyNotificationForReport(TReportType type)
{
	_entry->_wrapperDataWasUpdated(LMap::mapWithObjectsAndKeys(LValue::valueWithInt(type), kReportIDKey, (LObject *)0));
}

// <data>/<reports folder>/<type - 1>/<file name>; the folders are made as needed.
LString *CWOnlineCityData::_filePathForReport(TReportType type, LString *fileName)
{
	if (sReportFolders == 0)
		sReportFolders = LMap::_alloc()->init();
	LString *key = LString::stringWithInt(type - 1);
	LString *folder = (LString *)sReportFolders->objectForKey(key);
	if (folder == 0) {
		IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
		LString *root = callbacks ? callbacks->_applicationDataPath(1) : 0;
		if (root == 0)
			return 0;
		LFileManager *fm = LFileManager::manager();
		LMap *attrs =
		    LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0777), LFileManager::fileAttrPosixPermissions, (LObject *)0);
		folder = root->stringByAppendingPathComponent(
		    LString::stringFromTable("ReportsFolderName", "PathComponents", getAppModelModule()));
		if (!fm->objectExistsAtPath(folder, 0)) {
			fm->createFolderAtPath(folder);
			fm->changeFileAttributesAtPath(folder, attrs);
		}
		folder->appendPathComponent(key);
		if (!fm->objectExistsAtPath(folder, 0)) {
			fm->createFolderAtPath(folder);
			fm->changeFileAttributesAtPath(folder, attrs);
		}
		sReportFolders->setObjectForKey(folder, key);
	}
	return folder->stringByAppendingPathComponent(fileName);
}

LString *CWOnlineCityData::filePathForReport(TReportType type)
{
	return _filePathForReport(type, _fileName);
}

static double _modificationTime(LString *path)
{
	LMap *attributes = LFileManager::manager()->fileAttributesAtPath(path);
	LDate *date = attributes ? lobj_cast<LDate>(attributes->objectForKey(LFileManager::fileAttrModificationDate)) : 0;
	return date ? date->timeIntervalSinceReferenceDate() : 0.0;
}

double CWOnlineCityData::_lastUpdateTimeForReport(TReportType type)
{
	return _modificationTime(_filePathForReport(type, _fileName));
}

LString *CWOnlineCityData::_filePathForExpiredReport(TReportType type, LString *fileName)
{
	LString *path = _filePathForReport(type, fileName);
	path->appendPathExtension(LString::stringWithCString("exp"));
	return path;
}

double CWOnlineCityData::_lastUpdateTimeForExpiredReport(TReportType type)
{
	return _modificationTime(_filePathForExpiredReport(type, _fileName));
}

// The report from the cache or its file, the expired one if there's no
// other, with whether it expired and when it was downloaded.
LMap *CWOnlineCityData::_dictionaryForReport(TReportType type)
{
	LString *key = LString::stringWithFormat("%d", type);
	LObject *cached = _cache->objectForKey(key);
	if (cached)
		return cached != LNull::null ? (LMap *)cached : 0;
	LMap *report = LMap::mapWithContentsOfFile(_filePathForReport(type, _fileName));
	if (report) {
		report->setObjectForKey(LValue::valueWithBoolean(0), kReportCommonDataExpired);
		report->setObjectForKey(LValue::valueWithDouble(_lastUpdateTimeForReport(type)), kReportCommonDownloadDate);
	} else {
		report = LMap::mapWithContentsOfFile(_filePathForExpiredReport(type, _fileName));
		if (report == 0) {
			_cache->setObjectForKey(LNull::null, key);
			return 0;
		}
		report->setObjectForKey(LValue::valueWithBoolean(1), kReportCommonDataExpired);
		report->setObjectForKey(LValue::valueWithDouble(_lastUpdateTimeForExpiredReport(type)),
		                        kReportCommonDownloadDate);
	}
	_cache->setObjectForKey(report, key);
	return report;
}

LMap *CWOnlineCityData::dictionaryForReport(TReportType type)
{
	return _dictionaryForReport(type);
}

// Expires the reports of the mask that are too old (all of them when
// `force`) and downloads them again; `notify` tells about the fresh ones.
void CWOnlineCityData::_singleUpdateTaskForReports(unsigned int mask, unsigned char force, unsigned char notify)
{
	bool online = Preferences::usesInternet();
	double now = LDate::date()->timeIntervalSinceReferenceDate();
	for (int i = 0; i < kReportsCount; i++) {
		if ((mask & (1u << i)) == 0)
			continue;
		TReportType type = (TReportType)kReportByIndex[i];
		LMap *report = _dictionaryForReport(type);
		if (report) {
			LValue *expired = lobj_cast<LValue>(report->objectForKey(kReportCommonDataExpired));
			if (expired && !(char)expired->intValue()) {
				if (!force) {
					LValue *downloaded = lobj_cast<LValue>(report->objectForKey(kReportCommonDownloadDate));
					double time = downloaded ? downloaded->doubleValue() : _lastUpdateTimeForReport(type);
					if ((float)((now - time) / 3600.0) < kReportsRefreshTime[i]) {
						if (notify)
							_postReportIsReadyNotificationForReport(type);
						continue;
					}
				}
				LFileManager::manager()->movePathToPath(_filePathForReport(type, _fileName),
				                                        _filePathForExpiredReport(type, _fileName));
				report->setObjectForKey(LValue::valueWithBoolean(1), kReportCommonDataExpired);
				_postReportWasExpiredNotificationForReport(type);
			}
		}
		if (online)
			_postDownloadTaskForReport(type);
	}
}

void CWOnlineCityData::singleUpdateTaskForScheduledReports()
{
	_singleUpdateTaskForReports(_scheduledReports, 0, 0);
}

void CWOnlineCityData::manualUpdateOfScheduledReports()
{
	_singleUpdateTaskForReports(_scheduledReports, 1, 1);
}

void CWOnlineCityData::singleUpdateTaskForReports(unsigned int mask)
{
	_singleUpdateTaskForReports(mask, 0, 1);
}

void CWOnlineCityData::scheduleReports(unsigned int mask)
{
	_incScheduledReportsCount(mask);
	unsigned int old = _scheduledReports;
	_scheduledReports = old | mask;
	if (_scheduledReports)
		CWManager::instance()->scheduleEntry(_entry);
	if (mask != (mask & old))
		_singleUpdateTaskForReports(mask, 0, 1);
}

// args[0]: {kReportIDKey: type, kReportDictionaryKey: report}, without the
// report when the download failed.
intptr_t CWOnlineCityData::_downloadComplete(char *args)
{
	static bool doShowAlert = true;
	LMap *info = LF_ARG(args, 0, LMap *);
	if (info == 0)
		return 0;
	TReportType type = (TReportType)UMapParsingUtils::loadIntFromObject(info->objectForKey(kReportIDKey));
	LString *key = LString::stringWithFormat("%d", type);
	LMap *downloaded = lobj_cast<LMap>(info->objectForKey(kReportDictionaryKey));
	LMap *report = LMap::mapWithMap(downloaded);
	if (downloaded) {
		_cache->setObjectForKey(report, key);
		LString *path = _filePathForReport(type, _fileName);
		bool written = report->writeToFile(path);
		double time;
		if (written) {
			LFileManager::manager()->changeFileAttributesAtPath(
			    path, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0664),
			                                      LFileManager::fileAttrPosixPermissions, 0));
			doShowAlert = true;
			LFileManager::manager()->removeFileAtPath(_filePathForExpiredReport(type, _fileName));
			time = _lastUpdateTimeForReport(type);
		} else {
			time = LDate::date()->timeIntervalSinceReferenceDate();
		}
		report->setObjectForKey(LValue::valueWithBoolean(0), kReportCommonDataExpired);
		report->setObjectForKey(LValue::valueWithDouble(time), kReportCommonDownloadDate);
		_postReportIsReadyNotificationForReport(type);
		if (!written && doShowAlert) {
			doShowAlert = false;
			IAlertCallback::showAlertWithCode(0x1010000);
		}
	}
	_downloading[type] = false;
	return 0;
}
