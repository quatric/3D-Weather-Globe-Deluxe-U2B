// LFileWrapper: a file or a folder, read lazily from the file manager or
// from inside a zip archive; a folder's wrappers are kept by name.
#pragma once

#include "LMap.h"
#include "LData.h"

namespace LF {

class LZipSingleFileWrapper;

class LFileWrapper : public LObject {
	LF_DECLARE_CLASS(LFileWrapper)
public:
	LFileWrapper()
	    : _isDirectory(false), _isRegularFile(false), _contents(0), _fileWrappers(0), _path(0), _filename(0),
	      _preferredFilename(0), _zip(0)
	{
	}
	virtual ~LFileWrapper() {}
	virtual LString *description() const;
	virtual void dealloc();
	virtual LFileWrapper *getOwnerLObject() { return this; }

	LFileWrapper *initWithPath(LString *path);
	LFileWrapper *initWithPathToZip(LString *path);
	LFileWrapper *initWithZipSubath(LZipSingleFileWrapper *zip, LString *subpath);
	LFileWrapper *initDirectoryWithFileWrappers(LMap *wrappers);
	LFileWrapper *initRegularFileWithContents(LData *contents);

	bool isDirectory() const { return _isDirectory; }
	bool isRegularFile() const { return _isRegularFile; }
	LString *filename() const { return _filename; }
	void setFilename(LString *name);
	LString *preferredFilename() const { return _preferredFilename; }
	void setPreferredFilename(LString *name);

	LData *regularFileContents();
	LData *regularFileMappedContents();
	// A folder's wrappers by name, read when first asked for.
	LMap *fileWrappers();
	// The name it was added with, made unique.
	LString *addFileWrapper(LFileWrapper *wrapper);
	void removeFileWrapper(LFileWrapper *wrapper);

	bool _isDirectory;
	bool _isRegularFile;
	LData *_contents;
	LMap *_fileWrappers;
	LString *_path;	// in the file manager or the archive
	LString *_filename;
	LString *_preferredFilename;
	LZipSingleFileWrapper *_zip;
};

} // namespace LF
