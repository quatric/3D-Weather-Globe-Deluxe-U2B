// LFileMapping: a file opened for mapping into memory. Each LFileMappingView
// maps one range of it read-only, and LMappedData is an LData over a view
// that keeps the view (and so the mapping) alive.
#pragma once

#include "LData.h"
#include "LContainers.h"

namespace LF {

class LMappedData;

class LFileMapping : public LObject {
	LF_DECLARE_CLASS(LFileMapping)
public:
	LFileMapping() : _path(0), _size(0), _fd(-1) {}
	virtual ~LFileMapping() {}
	virtual void dealloc();
	virtual LFileMapping *getOwnerLObject() { return this; }

	LFileMapping *initWithPathToFile(const LString *path);
	// An autoreleased LMappedData of the range, or 0.
	LMappedData *detachMappedData(LRange range);

	LString *_path;
	off_t _size;
	int _fd;
};

class LFileMappingView : public LObject {
	LF_DECLARE_CLASS(LFileMappingView)
public:
	LFileMappingView() : _mapping(0), _range(LZeroRange), _base(0), _offset(0) {}
	virtual ~LFileMappingView() {}
	virtual void dealloc();
	virtual LFileMappingView *getOwnerLObject() { return this; }

	LFileMappingView *initWithParams(LFileMapping *mapping, LRange range);
	LMappedData *mappedData();

	LFileMapping *_mapping;
	LRange _range;
	void *_base;	// the mapped pages
	unsigned int _offset;	// where the range starts in them
	size_t _mappedLength;
};

class LMappedData : public LData {
	LF_DECLARE_CLASS(LMappedData)
public:
	LMappedData() : _view(0) {}
	virtual ~LMappedData() {}
	virtual void dealloc();
	virtual void removeFromLeakerAnalyserTable();
	virtual LMappedData *getOwnerLObject() { return this; }

	LMappedData *initWithMappedView(LFileMappingView *view);

	LFileMappingView *_view;
};

} // namespace LF
