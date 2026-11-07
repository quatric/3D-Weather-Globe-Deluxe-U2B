// CloudsManager: the frames of the cloud animation, each a pair of DXT1
// textures (west and east halves of the earth) for a period of
// Preferences::cloudsUpdateTime() hours, newest first. They are the
// downloaded maps of the last five days, or the default clouds shipped with
// the application (taken on 2008-12-09 09:00); with the default clouds asked
// for, the last downloaded ones are used when there are enough of them.

#include "AppModel.h"
#include "UTextUtils.h"

LF_IMPLEMENT_CLASS(CloudsManager, 0)
LF_IMPLEMENT_CLASS(_CloudFrame, 0)
LF_IMPLEMENT_CLASS(_CCloudTextureCompressionTask, LF_CLASSOF(LTask))
LF_SELECTOR(CloudsManager, _cloudsTextureSizeWasChanged);
LF_SELECTOR(CloudsManager, _cloudsEnabledStateWasChanged);

LMap *CloudsManager::sDownloadedCloudsFolders = 0;
LMap *_CloudFrame::sLoadedClouds = 0;

class _cloudsManagerCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		if (CloudsManager::sDownloadedCloudsFolders) {
			CloudsManager::sDownloadedCloudsFolders->release();
			CloudsManager::sDownloadedCloudsFolders = 0;
		}
		if (_CloudFrame::sLoadedClouds) {
			_CloudFrame::sLoadedClouds->release();
			_CloudFrame::sLoadedClouds = 0;
		}
	}
};

static _cloudsManagerCleanup _sCloudsManagerCleanup;

static LDate *_defaultCloudsDate()
{
	return LDate::dateWithTime(2008, 12, 9, 9, 0, 0, 0);
}

LDate *CloudsManager::defaultCloudsCurrentDate()
{
	return _defaultCloudsDate();
}

CloudsManager *CloudsManager::init()
{
	if (LObject::init() == 0)
		return 0;
	LNotificationCenter::center()->addObserver(this, "_cloudsEnabledStateWasChanged",
	                                           Preferences::kCloudsEnableStateWasChanged, 0);
	LNotificationCenter::center()->addObserver(this, "_cloudsTextureSizeWasChanged",
	                                           Preferences::kCloudsTextureSizeWasChanged, 0);
	return this;
}

void CloudsManager::_invalFrames()
{
	if (_frames) {
		_frames->release();
		_frames = 0;
	}
	if (_currentDate) {
		_currentDate->release();
		_currentDate = 0;
	}
}

void CloudsManager::cleanup()
{
	_invalFrames();
	if (_CloudFrame::sLoadedClouds) {
		_CloudFrame::sLoadedClouds->release();
		_CloudFrame::sLoadedClouds = 0;
	}
}

void CloudsManager::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	cleanup();
	LObject::dealloc();
}

// Five days' worth, as west and east files.
int CloudsManager::_maxDownloadedCloudsFilesCount()
{
	return (24 / Preferences::cloudsUpdateTime()) * 10;
}

void CloudsManager::setFrameTextureFormat(D3DFORMAT format)
{
	_frameFormat = format;
}

void CloudsManager::setUseDefaultClouds(unsigned char useDefault)
{
	if (_useDefaultClouds == (useDefault != 0))
		return;
	_useDefaultClouds = useDefault != 0;
	LNotificationCenter::center()->postNotification(Preferences::kCloudsTextureSizeWasChanged, this, 0);
}

intptr_t CloudsManager::_cloudsTextureSizeWasChanged(char *)
{
	cleanup();
	return 0;
}

intptr_t CloudsManager::_cloudsEnabledStateWasChanged(char *)
{
	if (!Preferences::cloudsEnabled())
		cleanup();
	return 0;
}

// "west_YY_MM_DD_HH.dds" or "east_...".
LString *CloudsManager::mapFileNameForDate(LDate *date, unsigned char east)
{
	return LString::stringWithFormat("%s_%02d_%02d_%02d_%02d.dds", east ? "east" : "west", date->year() - 2000,
	                                 date->month(), date->day(), date->hour());
}

// The default clouds are 1024 pixels wide.
LSize CloudsManager::frameTextureSize()
{
	float width;
	if (!Preferences::usesInternet() || (_useDefaultClouds && !_usesLastDownloaded))
		width = 1024.0f;
	else
		width = (float)Preferences::cloudsTextureWidth();
	return LMakeSize(width, width);
}

LString *CloudsManager::defaultCloudsFolder()
{
	return UTextUtils::convertPathToNativePath(LString::stringWithCString("TEXTURES\\CLOUDS"));
}

// <application data>/<CloudsFolderName>/<texture width>, made when missing.
LString *CloudsManager::downloadedCloudsFolder()
{
	if (sDownloadedCloudsFolders == 0)
		sDownloadedCloudsFolders = LMap::_alloc()->init();
	LString *width = LString::stringWithInt(Preferences::cloudsTextureWidth());
	LString *folder = (LString *)sDownloadedCloudsFolders->objectForKey(width);
	if (folder)
		return folder;
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	LString *root = callbacks ? callbacks->_applicationDataPath(1) : 0;
	if (root == 0)
		return 0;
	LFileManager *fm = LFileManager::manager();
	folder = root->stringByAppendingPathComponent(
	    LString::stringFromTable("CloudsFolderName", "PathComponents", getAppModelModule()));
	if (!fm->objectExistsAtPath(folder, 0)) {
		fm->createFolderAtPath(folder);
		fm->changeFileAttributesAtPath(
		    folder, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0777), LFileManager::fileAttrPosixPermissions, (LObject *)0));
	}
	folder->appendPathComponent(width);
	if (!fm->objectExistsAtPath(folder, 0)) {
		fm->createFolderAtPath(folder);
		fm->changeFileAttributesAtPath(
		    folder, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0777), LFileManager::fileAttrPosixPermissions, (LObject *)0));
	}
	sDownloadedCloudsFolders->setObjectForKey(folder, width);
	return folder;
}

// The paths of the newest complete pairs of downloaded maps: west, east,
// west, east... (the names sort newest first, west before east).
LArray *CloudsManager::_lastDownloadedCloudsFileNames()
{
	LFileManager *fm = LFileManager::manager();
	LString *folder = downloadedCloudsFolder();
	LArray *files = fm->directoryContentsAtPath(folder);
	if (files == 0)
		return 0;
	files->setComparator(CFileNameComparator::compare);
	files->sort();
	LArray *result = LArray::array();
	int max = _maxDownloadedCloudsFilesCount();
	int count = files->count();
	int i = 0;
	while (i < count && (int)result->count() < max) {
		LString *west = 0, *east = 0;
		while (i < count) {
			LString *name = (LString *)files->objectAtIndex(i++);
			if (!name->hasPrefix(&sCloudsWest))
				continue;
			west = name;
			if (i < count) {
				LString *next = (LString *)files->objectAtIndex(i);
				if (next->hasPrefix(&sCloudsEast)) {
					i++;
					if (next->substringFromIndex(4)->isEqualTo(name->substringFromIndex(4)))
						east = next;
				}
			}
			break;
		}
		if (west && east) {
			LString *path = LString::_alloc()->init();
			path->setValue(folder);
			path->appendPathComponent(west);
			result->addObject(path);
			path->release();
			path = LString::_alloc()->init();
			path->setValue(folder);
			path->appendPathComponent(east);
			result->addObject(path);
			path->release();
		}
	}
	return result;
}

// Deletes the downloaded maps older than five days (or from the future)
// that aren't among the last complete ones.
void CloudsManager::deleteObsoleteFiles()
{
	LAutoreleasePool::addNewPoolToTop();
	double now = LDate::date()->timeIntervalSinceReferenceDate();
	LFileManager *fm = LFileManager::manager();
	LString *folder = downloadedCloudsFolder();
	LArray *files = folder ? fm->directoryContentsAtPath(folder) : 0;
	LArray *keep = _lastDownloadedCloudsFileNames();
	int count = files ? files->count() : 0;
	for (int i = 0; i < count; i++) {
		LString *path = folder->stringByAppendingPathComponent((LString *)files->objectAtIndex(i));
		double age = now - fm->lastUpdateTimeForFile(path);
		// As in the original, `keep` is searched by identity, so it keeps nothing.
		if ((age < 0.0 || 432000.0 < age) && (keep == 0 || keep->indexOfObjectIdenticalTo(path) == -1) &&
		    fm->isDeletableFileAtPath(path))
			fm->removeFileAtPath(path);
	}
	LAutoreleasePool::removeTopPool();
}

void CloudsManager::_reloadFramesIfNeeded()
{
	if (_frames)
		return;
	LFileManager *fm = LFileManager::manager();
	int step = Preferences::cloudsUpdateTime();
	_frames = LArray::_alloc()->init();
	LDate *date;
	LString *folder;
	if (!Preferences::usesInternet()) {
		date = _defaultCloudsDate();
		folder = defaultCloudsFolder();
	} else if (!_useDefaultClouds) {
		date = LDate::date();
		folder = downloadedCloudsFolder();
	} else {
		LArray *names = _lastDownloadedCloudsFileNames();
		if (names && _maxDownloadedCloudsFilesCount() <= (int)names->count()) {
			_usesLastDownloaded = true;
			int count = names->count();
			for (int i = 0; i < count; i += 2) {
				_CloudFrame *frame = _CloudFrame::_alloc()->initWithPaths(
				    (LString *)names->objectAtIndex(i), (LString *)names->objectAtIndex(i + 1), this, (i / 2) * step);
				if (i == 0)
					_currentDate = LRetain(frame->date());
				_frames->addObject(frame);
				if (frame)
					frame->release();
			}
			return;
		}
		date = _defaultCloudsDate();
		folder = defaultCloudsFolder();
		_usesLastDownloaded = false;
	}
	// The start of the current period, then back an hour at a time over
	// five days, a frame per period whose two maps are there.
	int hour = date->hour();
	LDate *start = LDate::dateWithTime(date->year(), 1, date->dayOfYear(), hour, 0, 0, date->timeZone());
	int offset = -(hour % step);
	_currentDate = LRetain(start->dateByAddingTime(0, 0, 0, offset, 0, 0));
	int lastPeriod = -1;
	int lastDay = 0xff;
	for (int i = 0; i < 120; i++) {
		int period = date->hour() / step;
		if (period != lastPeriod) {
			LString *eastPath = folder->stringByAppendingPathComponent(mapFileNameForDate(date, 1));
			unsigned char isFolder;
			if (fm->objectExistsAtPath(eastPath, &isFolder)) {
				LString *westPath = folder->stringByAppendingPathComponent(mapFileNameForDate(date, 0));
				if (fm->objectExistsAtPath(westPath, &isFolder)) {
					_CloudFrame *frame = _CloudFrame::_alloc()->initWithPaths(westPath, eastPath, this, offset);
					_frames->addObject(frame);
					lastPeriod = period;
					if (frame)
						frame->release();
				}
			}
		}
		date = date->dateByAddingTime(0, 0, 0, -1, 0, 0);
		if (date->day() != lastDay)
			lastPeriod = -1;
		lastDay = date->day();
		offset++;
	}
}

LDate *CloudsManager::currentCloudsDate()
{
	_reloadFramesIfNeeded();
	return _currentDate;
}

// Frame 0 is the oldest.
void CloudsManager::updateTexture(C2DTexture *texture, int frame, unsigned char east)
{
	_reloadFramesIfNeeded();
	if (frame < 0 || frame >= (int)_frames->count() || texture->_context == 0)
		return;
	_CloudFrame *f = (_CloudFrame *)_frames->objectAtIndex(_frames->count() - 1 - frame);
	LData *data = f->textureData(east);
	if (data == 0)
		return;
	C2DTexture::StLocker locker(texture, 0, D3DLOCK_DISCARD);
	unsigned int size = locker.desc()->Height * locker.bytesPerRow();
	if (data->length() < size)
		size = data->length();
	memcpy(locker.bits(), data->bytes(), size);
}

int CloudsManager::framesCount()
{
	_reloadFramesIfNeeded();
	return _frames ? _frames->count() : 0;
}

int CloudsManager::frameIndexForBackInTimeOffset(int hours)
{
	int count = framesCount();
	if (count == 0)
		return -1;
	int step = Preferences::cloudsUpdateTime();
	for (int i = 0; i < count; i++) {
		int offset = ((_CloudFrame *)_frames->objectAtIndex(i))->_backInTimeOffset;
		if (hours <= offset && offset < step + hours)
			return count - i - 1;
	}
	return -1;
}

int CloudsManager::backInTimeOffsetForFrameAtIndex(int index)
{
	if (index < 0)
		return -1;
	int count = framesCount();
	if (count == 0)
		return -1;
	return ((_CloudFrame *)_frames->objectAtIndex(count - index % count - 1))->_backInTimeOffset;
}

// ---------------------------------------------------------------------------

void _CloudFrame::dealloc()
{
	if (_westPath) {
		_westPath->release();
		_westPath = 0;
	}
	if (_eastPath) {
		_eastPath->release();
		_eastPath = 0;
	}
	LObject::dealloc();
}

// The map's time from its name; the year is the file's.
LDate *_CloudFrame::date()
{
	LString *path = _westPath ? _westPath : _eastPath;
	if (path == 0)
		return 0;
	LDate *modified = LDate::dateWithTimeIntervalSinceReferenceDate(LFileManager::manager()->lastUpdateTimeForFile(path));
	LArray *parts = path->lastPathComponent()->stringByDeletingPathExtension()->componentsSeparatedByString(&sCloudsUnderscope);
	if (parts == 0 || parts->count() != 5)
		return 0;
	int hour = loadIntFromObject(parts->objectAtIndex(4));
	int day = loadIntFromObject(parts->objectAtIndex(3));
	int month = loadIntFromObject(parts->objectAtIndex(2));
	return LDate::dateWithTime(modified->year(), month, day, hour, 0, 0, 0);
}

_CloudFrame *_CloudFrame::initWithPaths(LString *westPath, LString *eastPath, CloudsManager *manager, int backInTimeOffset)
{
	if (LObject::init() == 0)
		return 0;
	_westPath = LRetain(westPath);
	_eastPath = LRetain(eastPath);
	_format = manager->_frameFormat;
	_size = manager->frameTextureSize();
	_backInTimeOffset = backInTimeOffset;
	return this;
}

// The decompressed texture, cached when it has the frame's size.
LData *_CloudFrame::textureData(unsigned char east)
{
	LString *path = east ? _eastPath : _westPath;
	if (sLoadedClouds) {
		LData *data = (LData *)sLoadedClouds->objectForKey(path);
		if (data)
			return data;
	}
	CTextureDecompressionTask *task = CTextureDecompressionTask::_alloc()->initWithParams(path, _format, 0, 0, 0);
	if (task == 0)
		return 0;
	task->l_autorelease();
	task->execute();
	LArray *results = task->_results;
	if (results == 0 || results->count() == 0)
		return 0;
	LData *data = lobj_cast<LData>(results->objectAtIndex(0));
	if (data == 0)
		return 0;
	if ((int)_size.width != (int)task->_width || (int)_size.width != (int)task->_height)
		return data;
	if (sLoadedClouds == 0)
		sLoadedClouds = LMap::_alloc()->init();
	sLoadedClouds->setObjectForKey(data, path);
	return data;
}

// ---------------------------------------------------------------------------

void _CCloudTextureCompressionTask::dealloc()
{
	if (_path) {
		_path->release();
		_path = 0;
	}
	if (_data) {
		_data->release();
		_data = 0;
	}
	LTask::dealloc();
}

// The JPEG as a DXT1 DDS file, readable by all.
void _CCloudTextureCompressionTask::execute()
{
	LF_CHECK_PTR(_data);
	ImageDecompressor *decompressor = ImageDecompressor::decompressorForData(_data);
	if (decompressor == 0)
		return;
	unsigned int width = decompressor->width();
	unsigned int height = decompressor->height();
	unsigned int size = FHGetSurfaceSize(width, height, D3DFMT_DXT1);
	LData *file = ImageDecompressor::ddsHeaderForFormatAndSize(D3DFMT_DXT1, width, height);
	unsigned int headerLength = file->length();
	file->setLength(size + headerLength);
	decompressor->decompressToBitmapWithFormat(D3DFMT_DXT1, (char *)file->bytes() + headerLength);
	bool written = file->writeToFile(_path);
	_failed = !written;
	if (!written)
		LFileManager::manager()->removeFileAtPath(_path);
	if (!_failed)
		LFileManager::manager()->changeFileAttributesAtPath(
		    _path, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0664), LFileManager::fileAttrPosixPermissions, (LObject *)0));
}

_CCloudTextureCompressionTask *_CCloudTextureCompressionTask::initWithParams(LString *path, LData *data, LObject *target,
                                                                             const char *sel)
{
	if (LTask::initWithTargetAndSelector(target, sel) == 0)
		return 0;
	_manager = target ? lobj_cast<CWManager>(target) : 0;
	_path = LString::_alloc()->init();
	_path->setValue(path);
	_data = LRetain(data);
	_failed = false;
	return this;
}
