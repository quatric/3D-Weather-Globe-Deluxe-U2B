// InstallationController: copies the high resolution maps from the
// installation disc. Each file being copied has an empty "<name>.tmp"
// marker beside it, so a copy cut short is found and redone.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(InstallationController, 0)
LF_SELECTOR(InstallationController, _installHiResMaps);
LF_SELECTOR(InstallationController, _installHiResMapsCompleted);
LF_SELECTOR(InstallationController, _postNotification);

static const unsigned int kCopyBufferSize = 0x800000;

void InstallationController::dealloc()
{
	if (_filesToInstall) {
		_filesToInstall->release();
		_filesToInstall = 0;
	}
	if (_mapsList) {
		_mapsList->release();
		_mapsList = 0;
	}
	LObject::dealloc();
}

intptr_t InstallationController::_installHiResMapsCompleted(char *)
{
	if (_filesToInstall) {
		_filesToInstall->release();
		_filesToInstall = 0;
	}
	_totalSize = 0;
	_installing = false;
	if (_notify)
		LNotificationCenter::center()->postNotification(kHiResMapsInstallationDidEndNotification, this, 0);
	return 0;
}

intptr_t InstallationController::_postNotification(char *args)
{
	LNotificationCenter::center()->postNotification(LF_ARG(args, 0, LNotification *));
	return 0;
}

void InstallationController::_validateHiResMapsList()
{
	if (_mapsList)
		return;
	_mapsList = LArray::_alloc()->init();
	LArray *list = LArray::arrayWithContentsOfFile(&sHiResMapsListFile);
	int count = list ? list->count() : 0;
	for (int i = 0; i < count; i++) {
		LObject *object = list->objectAtIndex(i);
		if (object && object->getClass()->isSubclassOf(LF_CLASSOF(LString)))
			_mapsList->addObject(object);
	}
}

bool InstallationController::installHiResMapsInBackground(unsigned char notify)
{
	if (_busy || _installing)
		return false;
	bool wasBusy = _busy;
	_busy = true;
	bool result = false;
	LFileManager *fm = LFileManager::manager();
	LString *root;
	if (!fm->isWritableFileAtPath(&sEmptyString)) {
		IAlertCallback::showAlertWithCode(0x1030001);
	} else if (IModelCallbacks::_callbacks &&
	           (root = IModelCallbacks::_callbacks->_hiResMapsRootFolderSourcePath()) != 0) {
		_validateHiResMapsList();
		int count = _mapsList ? _mapsList->count() : 0;
		unsigned long long size = 0;
		LArray *files = 0;
		for (int i = 0; i < count; i++) {
			LString *name = (LString *)_mapsList->objectAtIndex(i);
			if (fm->objectExistsAtPath(name, 0)) {
				LString *marker = name->stringByAppendingPathExtension(&sTempFileExtension);
				if (!fm->objectExistsAtPath(marker, 0))
					continue;
				fm->removeFileAtPath(name);
				fm->removeFileAtPath(marker);
			}
			LString *source = root->stringByAppendingPathComponent(name);
			while (!fm->objectExistsAtPath(source, 0)) {
				if (IAlertCallback::showAlertWithCode(0x1030002) == 1)
					goto done;
			}
			if (!files)
				files = LArray::array();
			files->addObject(source);
			LValue *fileSize = (LValue *)fm->fileAttributesAtPath(source)->objectForKey(LFileManager::fileAttrSize);
			if (fileSize)
				size += (long long)fileSize->intPtrValue();
		}
		if (files) {
			// Some room to spare: a megabyte per file.
			_totalSize = size + (long long)(files->count() * 0x100000);
			do {
				unsigned long long freeBytes = 0;
				LFileSystemFileManager::getDiskSpaceInfo(fm->systemPathFromInternalPath(&sEmptyString), &freeBytes,
				                                         0, 0);
				if (_totalSize < freeBytes) {
					_installing = true;
					_notify = notify != 0;
					if (_filesToInstall)
						_filesToInstall->release();
					_filesToInstall = LRetain(files);
					LThread::performSingleAction(this, "_installHiResMaps", 0);
					result = true;
					break;
				}
			} while (IAlertCallback::showAlertWithCode(0x1030000) != 1);
		}
	}
done:
	_busy = wasBusy;
	return result;
}

bool InstallationController::isHiResMapsValid()
{
	if (_busy || _installing)
		return true;
	_validateHiResMapsList();
	LFileManager *fm = LFileManager::manager();
	int count = _mapsList ? _mapsList->count() : 0;
	for (int i = 0; i < count; i++) {
		LString *name = (LString *)_mapsList->objectAtIndex(i);
		if (!fm->objectExistsAtPath(name, 0))
			return false;
		if (fm->objectExistsAtPath(name->stringByAppendingPathExtension(&sTempFileExtension), 0))
			return false;
	}
	return true;
}

// Runs on its own thread.
intptr_t InstallationController::_installHiResMaps(char *)
{
	StPointerBlock buffer(kCopyBufferSize, false);
	LFileManager *fm = LFileManager::manager();
	int count = _filesToInstall ? _filesToInstall->count() : 0;
	for (int i = 0; i < count; i++) {
		LString *source = (LString *)_filesToInstall->objectAtIndex(i);
		LString *name = source->lastPathComponent();
		LString *marker = name->stringByAppendingPathExtension(&sTempFileExtension);
		sEmptyString.writeToFile(marker);
		LStreamID in = fm->openReadStreamForFile(source);
		LStreamID out = fm->openWriteStreamForFile(name);
		bool copied = false;
		if (in && out) {
			unsigned long long fileSize = (unsigned long long)fm->streamSize(in);
			unsigned long long offset = 0;
			unsigned int chunk = fileSize > kCopyBufferSize ? kCopyBufferSize : (unsigned int)fileSize;
			copied = true;
			while (chunk != 0) {
				if (_notify) {
					LMap *info = LMap::mapWithObjectsAndKeys(
					    name, kHiResMapsFileNameKey, LValue::valueWithInt(i), kHiResMapsFileIndexKey,
					    LValue::valueWithInt64(fileSize), kHiResMapsFileSizeKey, LValue::valueWithInt64(offset),
					    kHiResMapsFileCopiedSizeKey, (LObject *)0);
					LNotification *notification =
					    LNotification::notificationWithParams(this, kHiResMapsInstallationProgressNotification, info);
					invokeSelectorInMainThreadWithObjects("_postNotification", false, false, notification);
				}
				while (fm->loadDataFromStream(in, offset, buffer.bytes(), chunk) != 1) {
					if (IAlertCallback::showAlertWithCode(0x1030002) == 1) {
						copied = false;
						goto closeStreams;
					}
				}
				while (fm->appendDataToStream(out, buffer.bytes(), chunk) != 1) {
					if (IAlertCallback::showAlertWithCode(0x1030003) == 1) {
						copied = false;
						goto closeStreams;
					}
				}
				offset += chunk;
				unsigned long long left = fileSize - offset;
				chunk = left > kCopyBufferSize ? kCopyBufferSize : (unsigned int)left;
			}
		}
	closeStreams:
		if (in)
			fm->closeStreamWithID(in);
		if (out)
			fm->closeStreamWithID(out);
		if (!copied)
			fm->removeFileAtPath(name);
		fm->removeFileAtPath(marker);
	}
	invokeSelectorInMainThreadWithObjects("_installHiResMapsCompleted", false, false);
	return 0;
}
