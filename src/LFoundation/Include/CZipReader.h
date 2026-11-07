// Zip archives as read-only folders. CZipReader indexes one archive's
// central directory (names lowercased, optionally deciphered with SHIFER);
// SZipStream reads one stored or deflated member. CZipFileManager is the
// file manager that falls back to every .zip in its root, newest first;
// LZipSingleFileWrapper opens one archive.
#pragma once

#include "LFileManager.h"
#include "LSet.h"
#include "LArray.h"
#include "LData.h"
#include "LFileMapping.h"

namespace LF {

// Where a member is, from its central directory entry.
struct SZipFileInfo {
	uint32_t localHeaderOffset;
	uint32_t dataOffset;	// found from the local header when first needed
	uint16_t method;	// 0 stored, 8 deflated
	uint32_t compressedSize;
	uint32_t uncompressedSize;
};

// A member's name, with its info.
class ZipString : public LString {
	LF_DECLARE_CLASS(ZipString)
public:
	ZipString() { memset(&_info, 0, sizeof _info); }
	virtual ~ZipString() {}
	virtual ZipString *getOwnerLObject() { return this; }

	SZipFileInfo _info;
};

struct SZipStream {
	SZipStream(SZipFileInfo *info) : _info(info), _position(0) {}
	// Finds the member's data in the archive (a stored member only).
	bool getFileMappingOffset(LStreamID archive, unsigned int *offset);
	// Reads the whole member: length must be its size.
	bool loadData(LStreamID archive, void *buffer, unsigned int length);
	// Clamped to the member's size.
	long long seekToStreamOffset(long long offset);

	bool _findDataOffset(LStreamID archive);

	SZipFileInfo *_info;
	long long _position;
};

class CZipReader {
public:
	CZipReader() : _files(0), _stream(0), _path(0), _mapping(0) {}
	~CZipReader();
	// key, if given, deciphers the member names.
	bool initWithFile(const LString *path, LString *key);

	bool isDirectoryAtPath(LString *path);
	bool isFileExist(const LString *path);
	SZipStream *openStream(const LString *path);
	bool closeStream(SZipStream *stream);
	// Adds the names in the folder (all of them at the top level for "") to
	// *contents, creating it if needed.
	void directoryContentsAtPath(LArray **contents, const LString *path);
	bool loadFileIntoData(const LString *path, LData *data);
	// A new block with the contents, or 0.
	StPointerBlock *loadFile(const LString *path);
	// A stored member, mapped from the archive; 0 for others.
	LData *mappedDataWithContentsOfFile(const LString *path);

	bool _scanLocalHeader(const LString *path, LString *key);
	bool _decompressFileIntoData(SZipFileInfo *info, LData *data);
	StPointerBlock *_decompressFile(SZipFileInfo *info);
	ZipString *_fileNamed(const LString *path);

	LSet *_files;	// LComparableSet of ZipString
	LStreamID _stream;
	LString *_path;
	LFileMapping *_mapping;
	CSharableResource _mappingLock;
};

class CZipFileManager : public LFileSystemFileManager {
public:
	CZipFileManager(const LString *root);
	virtual ~CZipFileManager();

	virtual LString *systemPathFromInternalPath(const LString *path);
	virtual bool dataFromFile(const LString *path, StPointerBlock **block);
	virtual bool loadFileIntoData(const LString *path, LData *data);
	virtual LStreamID openReadStreamForFile(const LString *path);
	virtual LStreamID openWriteStreamForFile(const LString *path);
	virtual LStreamID openAppendStreamForFile(const LString *path);
	virtual size_t loadDataFromStream(LStreamID stream, long long offset, void *buffer, unsigned int length);
	virtual size_t appendDataToStream(LStreamID stream, const void *bytes, unsigned int length);
	virtual bool closeStreamWithID(LStreamID stream);
	virtual long long streamSize(LStreamID stream);
	virtual long long offsetInStream(LStreamID stream);
	virtual long long seekToStreamOffset(LStreamID stream, long long offset);
	virtual bool objectExistsAtPath(const LString *path, unsigned char *isFolder);
	virtual bool isReadableFileAtPath(const LString *path);
	virtual LArray *directoryContentsAtPath(const LString *path);

	// Reopens the archive named name with the key that deciphers its names.
	void oneArgFunc(LString *name, LString *key);

	// A stream of this manager: a file's or an archive member's.
	struct StreamID {
		bool inZip;
		LStreamID fileStream;	// when !inZip
		SZipStream *zipStream;	// when inZip
		int reader;
	};

	TArray<CZipReader *> *_readers;
};

class LZipSingleFileWrapper : public LObject {
	LF_DECLARE_CLASS(LZipSingleFileWrapper)
public:
	LZipSingleFileWrapper() : _reader(0) {}
	virtual ~LZipSingleFileWrapper() {}
	virtual void dealloc();
	virtual LZipSingleFileWrapper *getOwnerLObject() { return this; }

	LZipSingleFileWrapper *initWithContentsOfFile(LString *path);
	bool isDirectoryAtPath(LString *path);
	LArray *directoryContentsAtSubpath(const LString *path);
	LData *dataWithContentsOfFile(LString *path);
	LData *mappedDataWithContentsOfFile(LString *path);

	CZipReader *_reader;
};

} // namespace LF
