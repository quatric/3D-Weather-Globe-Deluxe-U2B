// LFileWrapper
//
// LF::LFileWrapper.

#include "LFileWrapper.h"
#include "LFileManager.h"
#include "CZipReader.h"

namespace LF {

LF_IMPLEMENT_CLASS(LFileWrapper, 0)

static CSharableResource *_subwrapMutex()
{
	static CSharableResource sSubwrapMutex;
	return &sSubwrapMutex;
}

void LFileWrapper::dealloc()
{
	if (_path) {
		_path->release();
		_path = 0;
	}
	if (_fileWrappers) {
		_fileWrappers->release();
		_fileWrappers = 0;
	}
	if (_filename) {
		_filename->release();
		_filename = 0;
	}
	if (_preferredFilename) {
		_preferredFilename->release();
		_preferredFilename = 0;
	}
	if (_contents) {
		_contents->release();
		_contents = 0;
	}
	if (_zip) {
		_zip->release();
		_zip = 0;
	}
	LObject::dealloc();
}

LString *LFileWrapper::description() const
{
	return LString::stringWithFormat("%s <0x%08X> file:'%@' preferred:'%@' isDir=%s", className(),
	    (unsigned int)(uintptr_t)this, _filename, _preferredFilename, _isDirectory ? "YES" : "NO");
}

void LFileWrapper::setFilename(LString *name)
{
	LRetain(name);
	if (_filename) {
		_filename->release();
		_filename = 0;
	}
	_filename = name;
}

void LFileWrapper::setPreferredFilename(LString *name)
{
	if (_preferredFilename) {
		_preferredFilename->release();
		_preferredFilename = 0;
	}
	if (name) {
		_preferredFilename = LString::_alloc();
		_preferredFilename->setValue(name);
	}
}

// The file name and preferred name both come from the path.
static void _setNames(LFileWrapper *wrapper, LString *path)
{
	wrapper->setPreferredFilename(path->lastPathComponent());
	wrapper->_filename = LString::_alloc();
	wrapper->_filename->setValue(wrapper->_preferredFilename);
}

LFileWrapper *LFileWrapper::initDirectoryWithFileWrappers(LMap *wrappers)
{
	if (LObject::init() == 0)
		return 0;
	_isDirectory = true;
	_fileWrappers = LRetain(wrappers);
	return this;
}

LFileWrapper *LFileWrapper::initRegularFileWithContents(LData *contents)
{
	if (LObject::init() == 0)
		return 0;
	_contents = LRetain(contents);
	_isRegularFile = true;
	return this;
}

LFileWrapper *LFileWrapper::initWithPath(LString *path)
{
	if (LObject::init() == 0)
		return 0;
	LFileManager *manager = LFileManager::manager();
	unsigned char isFolder = 0;
	if (manager && manager->objectExistsAtPath(path, &isFolder)) {
		_isDirectory = isFolder;
		_path = LString::_alloc();
		_path->setValue(path);
		_setNames(this, _path);
		if (!_isDirectory)
			_isRegularFile = true;
		return this;
	}
	dealloc();
	return 0;
}

LFileWrapper *LFileWrapper::initWithPathToZip(LString *path)
{
	if (LObject::init() == 0)
		return 0;
	_isDirectory = true;
	_zip = LZipSingleFileWrapper::_alloc()->initWithContentsOfFile(path);
	if (_zip) {
		_path = LRetain(LString::emptyString());
		_setNames(this, path);
		return this;
	}
	dealloc();
	return 0;
}

LFileWrapper *LFileWrapper::initWithZipSubath(LZipSingleFileWrapper *zip, LString *subpath)
{
	if (LObject::init() == 0)
		return 0;
	_zip = LRetain(zip);
	if (_zip == 0) {
		dealloc();
		return 0;
	}
	if (_zip->isDirectoryAtPath(subpath))
		_isDirectory = true;
	else
		_isRegularFile = true;
	_path = subpath->copy();
	_setNames(this, _path);
	return this;
}

LData *LFileWrapper::regularFileContents()
{
	if (!_isRegularFile)
		LF_RAISE(WrongConditionException());
	if (_contents)
		return _contents;
	if (_path == 0)
		LF_RAISE(NilPtrException());
	return _zip ? _zip->dataWithContentsOfFile(_path) : LData::dataWithContentsOfFile(_path);
}

LData *LFileWrapper::regularFileMappedContents()
{
	if (!_isRegularFile)
		LF_RAISE(WrongConditionException());
	if (_contents)
		return _contents;
	if (_path == 0)
		LF_RAISE(NilPtrException());
	return _zip ? _zip->mappedDataWithContentsOfFile(_path) : LData::dataWithContentsOfFile(_path);
}

LMap *LFileWrapper::fileWrappers()
{
	if (!_isDirectory)
		return 0;
	if (_fileWrappers == 0) {
		StSharableLocker lock(_subwrapMutex(), true);
		if (_fileWrappers == 0) {
			LArray *names = 0;
			LFileManager *manager = _zip ? 0 : LFileManager::manager();
			if (_zip || manager) {
				_fileWrappers = LMap::_alloc()->init();
				names = _zip ? _zip->directoryContentsAtSubpath(_path) : manager->directoryContentsAtPath(_path);
			}
			int n = names ? names->count() : 0;
			for (int i = 0; i < n; i++) {
				StAutoreleasePool pool;
				LString *path = _path->stringByAppendingPathComponent((LString *)names->objectAtIndex(i));
				LFileWrapper *wrapper = _zip ? _alloc()->initWithZipSubath(_zip, path) : _alloc()->initWithPath(path);
				if (wrapper) {
					_fileWrappers->setObjectForKey(wrapper, wrapper->_preferredFilename);
					wrapper->release();
				}
			}
		}
	}
	return _fileWrappers;
}

LString *LFileWrapper::addFileWrapper(LFileWrapper *wrapper)
{
	LMap *wrappers = fileWrappers();
	if (wrappers == 0)
		LF_RAISE(NilPtrException());
	LString *preferred = wrapper->_preferredFilename;
	LString *name = preferred;
	LString *base = 0;
	LString *extension = 0;
	// The original always tried "_1"; later numbers are tried here so a
	// second clash can't loop forever.
	for (int i = 1; wrappers->objectForKey(name) != 0; i++) {
		if (base == 0)
			base = preferred->stringByDeletingPathExtension();
		if (extension == 0)
			extension = preferred->pathExtension();
		name = LString::stringWithFormat("%@_%d.%@", base, i, extension);
	}
	wrappers->setObjectForKey(wrapper, name);
	return name;
}

void LFileWrapper::removeFileWrapper(LFileWrapper *wrapper)
{
	LMap *wrappers = fileWrappers();
	if (wrappers == 0)
		LF_RAISE(NilPtrException());
	LString *name = wrapper->_preferredFilename;
	if (wrappers->objectForKey(name) == wrapper) {
		wrappers->removeObjectForKey(name);
		return;
	}
	LMapIterator it(wrappers, true);
	while (it.next()) {
		if (it.object() == wrapper) {
			wrappers->removeObjectForKey(it.key());
			return;
		}
	}
}

} // namespace LF
