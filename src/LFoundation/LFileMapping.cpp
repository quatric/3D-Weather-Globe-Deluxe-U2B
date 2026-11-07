// LFileMapping
//
// LF::LFileMapping, LF::LFileMappingView and LF::LMappedData.

#include "LFileMapping.h"
#include <CoreFoundation/CoreFoundation.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace LF {

LF_IMPLEMENT_CLASS(LFileMapping, 0)
LF_IMPLEMENT_CLASS(LFileMappingView, 0)
LF_IMPLEMENT_CLASS(LMappedData, LF_CLASSOF(LData))

LFileMapping *LFileMapping::initWithPathToFile(const LString *path)
{
	if (LObject::init() == 0)
		return 0;
	_path = path->copy();
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, _path->cfString(), kCFURLPOSIXPathStyle, false);
	if (url) {
		char fsPath[1024];
		Boolean ok = CFURLGetFileSystemRepresentation(url, true, (UInt8 *)fsPath, sizeof fsPath);
		CFRelease(url);
		if (ok) {
			struct stat st;
			if (stat(fsPath, &st) != -1) {
				_size = st.st_size;
				_fd = open(fsPath, O_RDONLY);
			}
			if (_fd != -1)
				return this;
		}
	}
	LLog("Cannot create file mapping\n");
	dealloc();
	return 0;
}

void LFileMapping::dealloc()
{
	if (_fd != -1) {
		close(_fd);
		_fd = -1;
	}
	if (_path) {
		_path->release();
		_path = 0;
	}
	LObject::dealloc();
}

LMappedData *LFileMapping::detachMappedData(LRange range)
{
	LFileMappingView *view = LFileMappingView::_alloc()->initWithParams(this, range);
	if (view == 0)
		return 0;
	LMappedData *data = view->mappedData();
	view->release();
	return data;
}

LFileMappingView *LFileMappingView::initWithParams(LFileMapping *mapping, LRange range)
{
	if (LObject::init() == 0)
		return 0;
	_mapping = LRetain(mapping);
	_range = range;
	_base = 0;
	if (_mapping) {
		// mmap wants a page-aligned offset; the range starts _offset bytes in.
		off_t pageStart = range.location - range.location % getpagesize();
		_offset = (unsigned int)(range.location - pageStart);
		_mappedLength = range.length + _offset;
		void *p = mmap(0, _mappedLength, PROT_READ, MAP_FILE | MAP_SHARED, _mapping->_fd, pageStart);
		if (p != MAP_FAILED)
			_base = p;
	}
	if (_base)
		return this;
	dealloc();
	return 0;
}

void LFileMappingView::dealloc()
{
	if (_base) {
		munmap(_base, _mappedLength);
		_base = 0;
	}
	if (_mapping) {
		_mapping->release();
		_mapping = 0;
	}
	LObject::dealloc();
}

LMappedData *LFileMappingView::mappedData()
{
	LMappedData *data = LMappedData::_alloc()->initWithMappedView(this);
	return data ? LAutorelease(data) : 0;
}

LMappedData *LMappedData::initWithMappedView(LFileMappingView *view)
{
	if (initWithBytesNoCopy((char *)view->_base + view->_offset, view->_range.length, false) == 0)
		return 0;
	_view = LRetain(view);
	return this;
}

void LMappedData::dealloc()
{
	if (_view) {
		_view->release();
		_view = 0;
	}
	LData::dealloc();
}

void LMappedData::removeFromLeakerAnalyserTable()
{
	LObject::removeFromLeakerAnalyserTable();
	_view->removeFromLeakerAnalyserTable();
	_view->_mapping->removeFromLeakerAnalyserTable();
}

} // namespace LF
