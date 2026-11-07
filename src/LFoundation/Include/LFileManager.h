// LFileManager: the abstract file access layer, with one shared manager
// (manager()). LFileSystemFileManager is the one on the real file system;
// relative paths are taken from its root (the app bundle's folder by
// default). Streams are opaque IDs; the original kept them in an int.
#pragma once

#include "LString.h"

namespace LF {

class LMap;
class LArray;
class LData;
class StPointerBlock;

typedef intptr_t LStreamID;

class LFileManager {
public:
	LFileManager(const LString *root);
	virtual ~LFileManager();

	virtual LString *systemPathFromInternalPath(const LString *path) = 0;
	// *block gets a new StPointerBlock with the contents.
	virtual bool dataFromFile(const LString *path, StPointerBlock **block) = 0;
	virtual bool loadFileIntoData(const LString *path, LData *data) = 0;
	virtual LStreamID openReadStreamForFile(const LString *path) = 0;
	virtual LStreamID openWriteStreamForFile(const LString *path) = 0;
	virtual LStreamID openAppendStreamForFile(const LString *path) = 0;
	// These return the fread/fwrite item count: 1 on success.
	virtual size_t loadDataFromStream(LStreamID stream, long long offset, void *buffer, unsigned int length) = 0;
	virtual size_t appendDataToStream(LStreamID stream, const void *bytes, unsigned int length) = 0;
	virtual bool closeStreamWithID(LStreamID stream) = 0;
	virtual long long streamSize(LStreamID stream) = 0;
	virtual long long offsetInStream(LStreamID stream) = 0;
	// The new offset, or -1.
	virtual long long seekToStreamOffset(LStreamID stream, long long offset) = 0;
	virtual bool objectExistsAtPath(const LString *path, unsigned char *isFolder) = 0;
	virtual bool createFolderAtPath(const LString *path) = 0;
	virtual LMap *fileAttributesAtPath(const LString *path) = 0;
	virtual bool changeFileAttributesAtPath(const LString *path, LMap *attributes) = 0;
	virtual bool removeFileAtPath(const LString *path) = 0;
	// A copy followed by a removal.
	virtual bool movePathToPath(const LString *from, const LString *to);
	virtual bool isReadableFileAtPath(const LString *path) = 0;
	virtual bool isWritableFileAtPath(const LString *path) = 0;
	virtual bool isDeletableFileAtPath(const LString *path) = 0;
	virtual bool isLockedFileAtPath(const LString *path) = 0;
	virtual LArray *directoryContentsAtPath(const LString *path) = 0;

	static LFileManager *manager();
	// The path of a Folder Manager folder (FSFindFolder), or 0.
	static LString *pathToFolder(int folderType, short domain, bool create);

	double lastUpdateTimeForFile(const LString *path);
	LData *readDataOfLength(LStreamID stream, long long offset, unsigned int length);
	LData *dataWithContentsOfFile(const LString *path);

	// Keys of fileAttributesAtPath's map.
	static LString *fileAttrPosixPermissions;	// an LValue
	static LString *fileAttrCreationDate;	// an LDate
	static LString *fileAttrModificationDate;	// an LDate
	static LString *fileAttrSize;	// an LValue

	static LFileManager *sManager;

	LString *_root;
};

class LFileSystemFileManager : public LFileManager {
public:
	// Becomes the shared manager; only one may exist at a time.
	LFileSystemFileManager(const LString *root);
	virtual ~LFileSystemFileManager();

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
	virtual bool createFolderAtPath(const LString *path);
	virtual LMap *fileAttributesAtPath(const LString *path);
	virtual bool changeFileAttributesAtPath(const LString *path, LMap *attributes);
	virtual bool removeFileAtPath(const LString *path);
	virtual bool isReadableFileAtPath(const LString *path);
	virtual bool isWritableFileAtPath(const LString *path);
	virtual bool isDeletableFileAtPath(const LString *path);
	virtual bool isLockedFileAtPath(const LString *path);
	virtual LArray *directoryContentsAtPath(const LString *path);

	// Streams that don't go through a manager: relative paths use '/' for
	// '\\' but no root. Access to them is locked.
	static LStreamID openGlobalReadStreamForFile(const LString *path);
	static LStreamID openGlobalWriteStreamForFile(const LString *path);
	static size_t loadDataFromGlobalStream(LStreamID stream, long long offset, void *buffer, unsigned int length);
	static size_t appendDataToGlobalStream(LStreamID stream, const void *bytes, unsigned int length);
	static bool closeGlobalStreamWithID(LStreamID stream);
	static long long globalStreamSize(LStreamID stream);
	static long long seekToGlobalStreamOffset(LStreamID stream, long long offset);
	// Free and total bytes of the path's volume; `available` gets the free
	// bytes too.
	static bool getDiskSpaceInfo(const LString *path, unsigned long long *freeBytes,
	                             unsigned long long *totalBytes, unsigned long long *available);

	static FILE *_openFileHandle(const LString *path, const char *mode);
	static bool _convertPathToFSRef(const LString *path, struct FSRef *ref);

	struct FileSystemStreamID : public CSharableResource {
		FileSystemStreamID(FILE *f) : file(f) {}
		FILE *file;
	};
};

} // namespace LF
