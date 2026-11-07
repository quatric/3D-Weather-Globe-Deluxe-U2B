// LFileSystemFileManager
//
// LF::LFileSystemFileManager: LFileManager on the file system, through stdio
// and the File Manager's FSRef calls (deprecated, but still present on
// arm64). Relative paths are taken from the root, with '\\' read as '/'.

#include "LFileManager.h"
#include "LArray.h"
#include "LData.h"
#include "LDate.h"
#include "LMap.h"
#include "LValue.h"
#include "UTextUtils.h"
#include <CoreServices/CoreServices.h>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace LF {

typedef LFileSystemFileManager::FileSystemStreamID StreamID;

static LStreamID _newStream(FILE *f)
{
	return f ? (LStreamID) new StreamID(f) : 0;
}

LFileSystemFileManager::LFileSystemFileManager(const LString *root) : LFileManager(root)
{
	if (sManager != 0)
		LF_RAISE(WrongConditionException());
	sManager = this;
	if (_root == 0 || !_root->isAbsolutePath()) {
		CFBundleRef bundle = CFBundleGetMainBundle();
		CFURLRef url = bundle ? CFBundleCopyBundleURL(bundle) : 0;
		if (url) {
			CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
			if (path) {
				LString *full = LString::_alloc();
				full->setValue(path);
				if (_root) {
					full->appendPathComponent(_root);
					_root->release();
				}
				_root = full;
				CFRelease(path);
			}
			CFRelease(url);
		}
	}
}

LFileSystemFileManager::~LFileSystemFileManager()
{
	sManager = 0;
}

LString *LFileSystemFileManager::systemPathFromInternalPath(const LString *path)
{
	if (path == 0 || path->isAbsolutePath())
		return const_cast<LString *>(path);
	LString *native = UTextUtils::convertPathToNativePath(path);
	if (_root == 0)
		return native;
	LString *full = LString::stringWithString(_root);
	full->appendPathComponent(native);
	return full;
}

FILE *LFileSystemFileManager::_openFileHandle(const LString *path, const char *mode)
{
	if (path == 0)
		return 0;
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path->cfString(), kCFURLPOSIXPathStyle, false);
	if (url == 0)
		return 0;
	char buf[1024];
	Boolean ok = CFURLGetFileSystemRepresentation(url, true, (UInt8 *)buf, sizeof buf);
	CFRelease(url);
	return ok ? fopen(buf, mode) : 0;
}

bool LFileSystemFileManager::_convertPathToFSRef(const LString *path, FSRef *ref)
{
	if (path == 0)
		return false;
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path->cfString(), kCFURLPOSIXPathStyle, false);
	if (url == 0)
		return false;
	bool ok = CFURLGetFSRef(url, ref);
	CFRelease(url);
	return ok;
}

// Reads the whole file; an empty file is logged and fails.
template <typename Sink> static bool _readFile(FILE *f, const LString *path, Sink sink)
{
	if (f == 0)
		return false;
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return false;
	}
	unsigned int size = (unsigned int)ftell(f);
	if (size == 0) {
		LLog("Warning: Can't read %@\n", path);
		fclose(f);	// the original left it open
		return false;
	}
	void *buffer = sink(size);
	rewind(f);
	size_t n = fread(buffer, size, 1, f);
	if (n != 1)
		ferror(f);
	fclose(f);
	return n == 1;
}

bool LFileSystemFileManager::loadFileIntoData(const LString *path, LData *data)
{
	if (data == 0)
		return false;
	FILE *f = _openFileHandle(systemPathFromInternalPath(path), "rb");
	return _readFile(f, path, [data](unsigned int size) {
		data->setLength(size);
		return data->bytes();
	});
}

bool LFileSystemFileManager::dataFromFile(const LString *path, StPointerBlock **block)
{
	if (block)
		*block = 0;
	FILE *f = _openFileHandle(systemPathFromInternalPath(path), "rb");
	bool ok = _readFile(f, path, [block](unsigned int size) {
		*block = new StPointerBlock(size, false);
		return (*block)->bytes();
	});
	if (!ok && block && *block) {
		delete *block;
		*block = 0;
	}
	return ok;
}

LStreamID LFileSystemFileManager::openReadStreamForFile(const LString *path)
{
	return _newStream(_openFileHandle(systemPathFromInternalPath(path), "rb"));
}

LStreamID LFileSystemFileManager::openWriteStreamForFile(const LString *path)
{
	return _newStream(_openFileHandle(systemPathFromInternalPath(path), "wb"));
}

LStreamID LFileSystemFileManager::openAppendStreamForFile(const LString *path)
{
	return _newStream(_openFileHandle(systemPathFromInternalPath(path), "ab"));
}

LStreamID LFileSystemFileManager::openGlobalReadStreamForFile(const LString *path)
{
	if (path == 0)
		return 0;
	if (!path->isAbsolutePath())
		path = UTextUtils::convertPathToNativePath(path);
	return _newStream(_openFileHandle(path, "rb"));
}

LStreamID LFileSystemFileManager::openGlobalWriteStreamForFile(const LString *path)
{
	if (path == 0)
		return 0;
	if (!path->isAbsolutePath())
		path = UTextUtils::convertPathToNativePath(path);
	return _newStream(_openFileHandle(path, "wb"));
}

size_t LFileSystemFileManager::loadDataFromGlobalStream(LStreamID stream, long long offset, void *buffer, unsigned int length)
{
	StreamID *s = (StreamID *)stream;
	if (s == 0)
		return 0;
	StSharableLocker lock(s, true);
	if (s->file == 0)
		return 0;
	fpos_t pos = offset;
	fsetpos(s->file, &pos);
	return fread(buffer, length, 1, s->file);
}

size_t LFileSystemFileManager::loadDataFromStream(LStreamID stream, long long offset, void *buffer, unsigned int length)
{
	return loadDataFromGlobalStream(stream, offset, buffer, length);
}

size_t LFileSystemFileManager::appendDataToGlobalStream(LStreamID stream, const void *bytes, unsigned int length)
{
	StreamID *s = (StreamID *)stream;
	if (s == 0)
		return 0;
	StSharableLocker lock(s, true);
	return s->file ? fwrite(bytes, length, 1, s->file) : 0;
}

// Unlocked, unlike appendDataToGlobalStream.
size_t LFileSystemFileManager::appendDataToStream(LStreamID stream, const void *bytes, unsigned int length)
{
	StreamID *s = (StreamID *)stream;
	if (s == 0 || s->file == 0)
		return 0;
	return fwrite(bytes, length, 1, s->file);
}

bool LFileSystemFileManager::closeGlobalStreamWithID(LStreamID stream)
{
	StreamID *s = (StreamID *)stream;
	int err = 0;
	if (s->file)
		err = fclose(s->file);
	delete s;
	return err == 0;
}

bool LFileSystemFileManager::closeStreamWithID(LStreamID stream)
{
	return closeGlobalStreamWithID(stream);
}

long long LFileSystemFileManager::globalStreamSize(LStreamID stream)
{
	StreamID *s = (StreamID *)stream;
	StSharableLocker lock(s, true);
	off_t here = ftello(s->file);
	fseeko(s->file, 0, SEEK_END);
	off_t size = ftello(s->file);
	fseeko(s->file, here, SEEK_SET);
	return size;
}

long long LFileSystemFileManager::streamSize(LStreamID stream)
{
	return globalStreamSize(stream);
}

long long LFileSystemFileManager::offsetInStream(LStreamID stream)
{
	fpos_t pos = 0;
	fgetpos(((StreamID *)stream)->file, &pos);
	return pos;
}

long long LFileSystemFileManager::seekToGlobalStreamOffset(LStreamID stream, long long offset)
{
	fpos_t pos = offset;
	if (fsetpos(((StreamID *)stream)->file, &pos) != 0)
		return -1;
	return offset;
}

long long LFileSystemFileManager::seekToStreamOffset(LStreamID stream, long long offset)
{
	return seekToGlobalStreamOffset(stream, offset);
}

bool LFileSystemFileManager::objectExistsAtPath(const LString *path, unsigned char *isFolder)
{
	FSRef ref;
	FSCatalogInfo info;
	if (!_convertPathToFSRef(systemPathFromInternalPath(path), &ref)
	    || FSGetCatalogInfo(&ref, kFSCatInfoNodeFlags, &info, 0, 0, 0) != noErr)
		return false;
	if (isFolder)
		*isFolder = (info.nodeFlags & kFSNodeIsDirectoryMask) != 0;
	return true;
}

bool LFileSystemFileManager::createFolderAtPath(const LString *path)
{
	FSRef parent;
	if (!_convertPathToFSRef(systemPathFromInternalPath(path->stringByDeletingLastPathComponent()), &parent))
		return false;
	CFStringRef name = path->lastPathComponent()->cfString();
	if (name == 0)
		return false;
	CFIndex n = CFStringGetLength(name);
	UniChar *chars = new UniChar[n];
	CFStringGetCharacters(name, CFRangeMake(0, n), chars);
	OSErr err = FSCreateDirectoryUnicode(&parent, n, chars, kFSCatInfoNone, 0, 0, 0, 0);
	delete[] chars;
	return err == noErr;
}

LMap *LFileSystemFileManager::fileAttributesAtPath(const LString *path)
{
	FSRef ref;
	FSCatalogInfo info;
	if (!_convertPathToFSRef(systemPathFromInternalPath(path), &ref)
	    || FSGetCatalogInfo(&ref, kFSCatInfoCreateDate | kFSCatInfoContentMod | kFSCatInfoAttrMod | kFSCatInfoAccessDate
	                                  | kFSCatInfoBackupDate | kFSCatInfoPermissions | kFSCatInfoDataSizes,
	                        &info, 0, 0, 0) != noErr)
		return 0;
	CFAbsoluteTime created, modified;
	UCConvertUTCDateTimeToCFAbsoluteTime(&info.createDate, &created);
	UCConvertUTCDateTimeToCFAbsoluteTime(&info.contentModDate, &modified);
	LValue *permissions = LValue::valueWithIntPtr(info.permissions.mode & 0777);
	LDate *createDate = LAutorelease(LDate::_alloc()->initWithTimeIntervalSinceReferenceDate(created));
	LDate *modDate = LAutorelease(LDate::_alloc()->initWithTimeIntervalSinceReferenceDate(modified));
	return LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr((int)info.dataLogicalSize), fileAttrSize,
	                                   permissions, fileAttrPosixPermissions, createDate, fileAttrCreationDate,
	                                   modDate, fileAttrModificationDate, (LObject *)0);
}

// Only the POSIX permissions can be changed.
bool LFileSystemFileManager::changeFileAttributesAtPath(const LString *path, LMap *attributes)
{
	if (attributes == 0)
		return false;
	FSRef ref;
	if (!_convertPathToFSRef(systemPathFromInternalPath(path), &ref))
		return false;
	FSCatalogInfo info;
	memset(&info, 0, sizeof info);
	LValue *value = (LValue *)attributes->objectForKey(fileAttrPosixPermissions);
	if (value == 0 || !value->getClass()->isSubclassOf(LF_CLASSOF(LValue)))
		return false;
	info.permissions.mode = (UInt16)value->intPtrValue() & 0777;
	return FSSetCatalogInfo(&ref, kFSCatInfoPermissions, &info) == noErr;
}

bool LFileSystemFileManager::removeFileAtPath(const LString *path)
{
	unsigned char isFolder;
	if (!objectExistsAtPath(path, &isFolder))
		return false;
	if (isFolder) {
		LArray *items = directoryContentsAtPath(path);
		unsigned int n = items ? items->count() : 0;
		bool ok = true;
		for (unsigned int i = 0; i < n; i++) {
			if (!removeFileAtPath(path->stringByAppendingPathComponent((LString *)items->objectAtIndex(i))))
				ok = false;
		}
		if (!ok)
			return false;
	}
	FSRef ref;
	if (!_convertPathToFSRef(systemPathFromInternalPath(path), &ref))
		return false;
	return FSDeleteObject(&ref) == noErr;
}

// The original tested the userAccess bits these return.
bool LFileSystemFileManager::isReadableFileAtPath(const LString *path)
{
	FSRef ref;
	FSCatalogInfo info;
	if (!_convertPathToFSRef(systemPathFromInternalPath(path), &ref)
	    || FSGetCatalogInfo(&ref, kFSCatInfoUserAccess, &info, 0, 0, 0) != noErr)
		return false;
	return (info.permissions.userAccess >> 2) & 1;
}

bool LFileSystemFileManager::isWritableFileAtPath(const LString *path)
{
	FSRef ref;
	FSCatalogInfo info;
	if (!_convertPathToFSRef(systemPathFromInternalPath(path), &ref)
	    || FSGetCatalogInfo(&ref, kFSCatInfoUserAccess, &info, 0, 0, 0) != noErr)
		return false;
	return (info.permissions.userAccess >> 1) & 1;
}

// The parent folder must be writable, and so must every subfolder.
bool LFileSystemFileManager::isDeletableFileAtPath(const LString *path)
{
	unsigned char isFolder;
	if (!objectExistsAtPath(path, &isFolder))
		return false;
	bool ok = isWritableFileAtPath(path->stringByDeletingLastPathComponent());
	if (ok && isFolder) {
		LArray *items = directoryContentsAtPath(path);
		unsigned int n = items ? items->count() : 0;
		for (unsigned int i = 0; i < n; i++) {
			LString *item = path->stringByAppendingPathComponent((LString *)items->objectAtIndex(i));
			if (objectExistsAtPath(item, &isFolder) && isFolder && !isDeletableFileAtPath(item))
				return false;
		}
	}
	return ok;
}

bool LFileSystemFileManager::isLockedFileAtPath(const LString *path)
{
	FSRef ref;
	FSCatalogInfo info;
	if (!_convertPathToFSRef(systemPathFromInternalPath(path), &ref)
	    || FSGetCatalogInfo(&ref, kFSCatInfoNodeFlags, &info, 0, 0, 0) != noErr)
		return false;
	return info.nodeFlags & kFSNodeLockedMask;
}

// 0 for an empty folder.
LArray *LFileSystemFileManager::directoryContentsAtPath(const LString *path)
{
	FSRef ref;
	FSIterator iter;
	if (!_convertPathToFSRef(systemPathFromInternalPath(path), &ref) || FSOpenIterator(&ref, kFSIterateFlat, &iter) != noErr)
		return 0;
	LArray *items = 0;
	ItemCount count;
	HFSUniStr255 name;
	while (FSGetCatalogInfoBulk(iter, 1, &count, 0, kFSCatInfoNone, 0, 0, 0, &name) == noErr) {
		CFStringRef str = CFStringCreateWithCharacters(kCFAllocatorDefault, name.unicode, name.length);
		if (items == 0)
			items = LArray::array();
		items->addObject(LString::stringWithCFString(str));
		CFRelease(str);
	}
	FSCloseIterator(iter);
	return items;
}

bool LFileSystemFileManager::getDiskSpaceInfo(const LString *path, unsigned long long *freeBytes,
                                              unsigned long long *totalBytes, unsigned long long *available)
{
	FSRef ref;
	FSCatalogInfo info;
	FSVolumeInfo volume;
	if (!_convertPathToFSRef(path, &ref) || FSGetCatalogInfo(&ref, kFSCatInfoVolume, &info, 0, 0, 0) != noErr
	    || FSGetVolumeInfo(info.volume, 0, 0, kFSVolInfoSizes, &volume, 0, 0) != noErr) {
		if (freeBytes)
			*freeBytes = 0;
		if (available)
			*available = 0;
		if (totalBytes)
			*totalBytes = 0;
		return false;
	}
	if (freeBytes)
		*freeBytes = volume.freeBytes;
	if (available)
		*available = volume.freeBytes;
	if (totalBytes)
		*totalBytes = volume.totalBytes;
	return true;
}

} // namespace LF
