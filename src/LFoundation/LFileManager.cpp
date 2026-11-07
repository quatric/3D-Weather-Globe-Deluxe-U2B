// LFileManager
//
// LF::LFileManager: the shared manager and the operations built on the
// abstract stream interface.

#include "LFileManager.h"
#include "LData.h"
#include "LDate.h"
#include "LMap.h"
#include <CoreServices/CoreServices.h>

namespace LF {

static LConstantString sFileAttrPosixPermissions("filePosixPermissions");
static LConstantString sFileAttrCreationDate("fileCreationDate");
static LConstantString sFileAttrModificationDate("fileModificationDate");
static LConstantString sFileAttrSize("fileSize");

LString *LFileManager::fileAttrPosixPermissions = &sFileAttrPosixPermissions;
LString *LFileManager::fileAttrCreationDate = &sFileAttrCreationDate;
LString *LFileManager::fileAttrModificationDate = &sFileAttrModificationDate;
LString *LFileManager::fileAttrSize = &sFileAttrSize;

LFileManager *LFileManager::sManager = 0;

LFileManager::LFileManager(const LString *root) : _root(0)
{
	if (root) {
		_root = LString::_alloc();
		_root->setValue(root);
	}
}

LFileManager::~LFileManager()
{
	if (_root) {
		_root->release();
		_root = 0;
	}
}

LFileManager *LFileManager::manager()
{
	return sManager;
}

double LFileManager::lastUpdateTimeForFile(const LString *path)
{
	LMap *attrs = fileAttributesAtPath(path);
	if (attrs) {
		LDate *date = (LDate *)attrs->objectForKey(fileAttrModificationDate);
		if (date && date->getClass()->isSubclassOf(LF_CLASSOF(LDate)))
			return date->timeIntervalSinceReferenceDate();
	}
	return 0;
}

LData *LFileManager::readDataOfLength(LStreamID stream, long long offset, unsigned int length)
{
	LData *data = LData::dataWithLength(length);
	if (loadDataFromStream(stream, offset, data->bytes(), length) != 1)
		return 0;
	return data;
}

LData *LFileManager::dataWithContentsOfFile(const LString *path)
{
	LStreamID stream = openReadStreamForFile(path);
	LData *data = 0;
	if (stream) {
		unsigned int size = (unsigned int)streamSize(stream);
		data = LData::dataWithLength(size);
		loadDataFromStream(stream, 0, data->bytes(), size);
		closeStreamWithID(stream);
	}
	return data;
}

bool LFileManager::movePathToPath(const LString *from, const LString *to)
{
	if (from == 0 || to == 0)
		return false;
	LAutoreleasePool::addNewPoolToTop();
	LData *data = dataWithContentsOfFile(from);
	bool ok = data && data->writeToFile(to) && removeFileAtPath(from);
	LAutoreleasePool::removeTopPool();
	return ok;
}

LString *LFileManager::pathToFolder(int folderType, short domain, bool create)
{
	FSRef ref;
	if (FSFindFolder(domain, folderType, create, &ref) != noErr)
		return 0;
	CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, &ref);
	if (url == 0)
		return 0;
	LString *result = 0;
	CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	if (path) {
		result = LString::stringWithCFString(path);
		CFRelease(path);
	}
	CFRelease(url);
	return result;
}

} // namespace LF
