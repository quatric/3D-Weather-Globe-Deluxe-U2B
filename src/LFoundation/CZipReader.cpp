// CZipReader
//
// LF::CZipReader, LF::SZipStream, LF::ZipString, LF::CZipFileManager and
// LF::LZipSingleFileWrapper.

#include "CZipReader.h"
#include "LDate.h"
#include "LMap.h"
#include "LProfiler.h"
#include "UTextUtils.h"
#include <ctype.h>
#include <zlib.h>

namespace LF {

LF_IMPLEMENT_CLASS(ZipString, LF_CLASSOF(LString))
LF_IMPLEMENT_CLASS(LZipSingleFileWrapper, 0)

static LConstantString kSlash("/");

static inline uint16_t _le16(const unsigned char *p) { return p[0] | (p[1] << 8); }
static inline uint32_t _le32(const unsigned char *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

// SZipStream

bool SZipStream::_findDataOffset(LStreamID archive)
{
	if (_info->dataOffset != 0)
		return true;
	unsigned char header[30];
	if (LFileSystemFileManager::loadDataFromGlobalStream(archive, _info->localHeaderOffset, header, sizeof header) != 1)
		return false;
	_info->dataOffset = _info->localHeaderOffset + 30 + _le16(header + 26) + _le16(header + 28);
	return true;
}

bool SZipStream::getFileMappingOffset(LStreamID archive, unsigned int *offset)
{
	_findDataOffset(archive);
	if (offset == 0 || _info->method != 0)
		return false;
	*offset = _info->dataOffset;
	return true;
}

long long SZipStream::seekToStreamOffset(long long offset)
{
	long long size = _info ? _info->uncompressedSize : 0;
	if (offset > size)
		offset = size;
	_position = offset;
	return offset;
}

bool SZipStream::loadData(LStreamID archive, void *buffer, unsigned int length)
{
	_findDataOffset(archive);
	if (_info->method == 0)
		return LFileSystemFileManager::loadDataFromGlobalStream(archive, _info->dataOffset, buffer, length) == 1;
	if (_info->method != 8 || _info->uncompressedSize != length)
		return false;

	LData *compressed = LData::_alloc();
	compressed->setLength(_info->compressedSize);
	bool ok = false;
	if (LFileSystemFileManager::loadDataFromGlobalStream(archive, _info->dataOffset, compressed->bytes(),
	        _info->compressedSize) == 1) {
		z_stream z;
		memset(&z, 0, sizeof z);
		z.next_in = (Bytef *)compressed->bytes();
		z.avail_in = compressed->length();
		z.next_out = (Bytef *)buffer;
		z.avail_out = length;
		if (inflateInit2(&z, -MAX_WBITS) == Z_OK) {
			ok = inflate(&z, Z_FINISH) == Z_STREAM_END;
			inflateEnd(&z);
		}
	}
	compressed->release();
	return ok;
}

// CZipReader

CZipReader::~CZipReader()
{
	if (_mapping) {
		_mapping->release();
		_mapping = 0;
	}
	if (_files) {
		_files->release();
		_files = 0;
	}
	if (_stream)
		LFileSystemFileManager::closeGlobalStreamWithID(_stream);
	_stream = 0;
	if (_path) {
		_path->release();
		_path = 0;
	}
}

bool CZipReader::initWithFile(const LString *path, LString *key)
{
	if (!_scanLocalHeader(path, key))
		return false;
	_path = LString::_alloc();
	_path->setValue(path);
	return _stream != 0;
}

ZipString *CZipReader::_fileNamed(const LString *path)
{
	if (path == 0 || _files == 0)
		return 0;
	LObject *own = _files->ownObjectForObject(path->lowercaseString());
	if (own && own->getClass()->isSubclassOf(ZipString::classObject()))
		return (ZipString *)own;
	return 0;
}

bool CZipReader::isDirectoryAtPath(LString *path)
{
	LString *dir = path->hasSuffix(&kSlash) ? LAutorelease(path->copy()) : path->stringByAppendingString(&kSlash);
	return _files && _files->containsObject(dir);
}

bool CZipReader::isFileExist(const LString *path)
{
	return _files && _files->containsObject(path->lowercaseString());
}

SZipStream *CZipReader::openStream(const LString *path)
{
	ZipString *file = _fileNamed(path);
	return file ? new SZipStream(&file->_info) : 0;
}

bool CZipReader::closeStream(SZipStream *stream)
{
	delete stream;
	return true;
}

void CZipReader::directoryContentsAtPath(LArray **contents, const LString *path)
{
	unsigned int length = path->length();
	LString *dir = LString::_alloc();
	dir->setValue(path);
	if (length != 0 && dir->hasSuffix(&kSlash))
		dir->eraseSubstringInRange(dir->length() - 1, 1);
	LString *parent = LString::_alloc();
	if (_files) {
		LSetIterator it(_files, true);
		while (LObject *obj = it.nextObject()) {
			LString *name = (LString *)obj;
			parent->setValue(name);
			parent->deleteLastPathComponent();
			LString *entry;
			if (length == 0) {
				if (parent->length() != 0)
					continue;
				entry = name;
			} else {
				if (parent->length() == 0 || parent->compareIgnoringCase(dir) != kCFCompareEqualTo)
					continue;
				entry = name->lastPathComponent();
			}
			if (entry && entry->length()) {
				if (*contents == 0)
					*contents = LArray::array();
				(*contents)->addObject(entry);
			}
		}
	}
	parent->release();
	dir->release();
}

bool CZipReader::_decompressFileIntoData(SZipFileInfo *info, LData *data)
{
	SZipStream stream(info);
	data->setLength(info->uncompressedSize);
	return stream.loadData(_stream, data->bytes(), info->uncompressedSize);
}

bool CZipReader::loadFileIntoData(const LString *path, LData *data)
{
	ZipString *file = _fileNamed(path);
	return file && _decompressFileIntoData(&file->_info, data);
}

StPointerBlock *CZipReader::_decompressFile(SZipFileInfo *info)
{
	SZipStream stream(info);
	StPointerBlock *block = new StPointerBlock(info->uncompressedSize, false);
	stream.loadData(_stream, block->bytes(), info->uncompressedSize);
	return block;
}

StPointerBlock *CZipReader::loadFile(const LString *path)
{
	ZipString *file = _fileNamed(path);
	return file ? _decompressFile(&file->_info) : 0;
}

LData *CZipReader::mappedDataWithContentsOfFile(const LString *path)
{
	ZipString *file = _fileNamed(path);
	if (file == 0)
		return 0;
	SZipStream stream(&file->_info);
	unsigned int offset = 0;
	if (!stream.getFileMappingOffset(_stream, &offset))
		return 0;
	if (_mapping == 0) {
		StSharableLocker lock(&_mappingLock, true);
		if (_mapping == 0)
			_mapping = LFileMapping::_alloc()->initWithPathToFile(_path);
	}
	if (_mapping == 0)
		return 0;
	return _mapping->detachMappedData(LMakeRange(offset, file->_info.uncompressedSize));
}

bool CZipReader::_scanLocalHeader(const LString *path, LString *key)
{
	StAutoreleasePool pool;
	_stream = LFileSystemFileManager::openGlobalReadStreamForFile(path);
	if (_stream == 0)
		return false;
	LString *ownKey = key ? SHIFER_ownKey() : 0;
	unsigned int size = (unsigned int)LFileSystemFileManager::globalStreamSize(_stream);

	// The end of central directory record, which must end the file (no
	// archive comment).
	LData *tail = LData::dataWithLength(size < 0x800 ? size : 0x800);
	LFileSystemFileManager::loadDataFromGlobalStream(_stream, size - tail->length(), tail->bytes(), tail->length());
	const unsigned char *bytes = (const unsigned char *)tail->bytes();
	const unsigned char *end = bytes + tail->length();
	for (const unsigned char *p = end - 22; p >= bytes && p < end; p++) {
		if (_le32(p) != 0x06054b50)
			continue;
		unsigned int entries = _le16(p + 10);
		if (entries != _le16(p + 8))
			return false;
		unsigned int cdOffset = _le32(p + 16);
		LData *cd = LData::dataWithLength(size - cdOffset);
		LFileSystemFileManager::loadDataFromGlobalStream(_stream, cdOffset, cd->bytes(), cd->length());
		const unsigned char *cdBytes = (const unsigned char *)cd->bytes();
		unsigned int cdLength = cd->length();
		unsigned int pos = 0;
		unsigned int charIndex = 0;
		char name[4097];
		while (pos + 46 <= cdLength) {
			const unsigned char *e = cdBytes + pos;
			if (_le32(e) != 0x02014b50)
				break;
			ZipString *file = ZipString::_alloc();
			file->init();
			file->_info.method = _le16(e + 10);
			file->_info.compressedSize = _le32(e + 20);
			file->_info.uncompressedSize = _le32(e + 24);
			unsigned int nameLength = _le16(e + 28);
			unsigned int copied = nameLength < sizeof name - 1 ? nameLength : sizeof name - 1;
			memcpy(name, e + 46, copied);
			name[copied] = 0;
			for (unsigned int i = 0; i < copied; i++)
				name[i] = (char)tolower(SHIFER_streamConvert(name[i], charIndex++, key, ownKey));
			file->setValue(name, copied, kLMacRomanEncoding);
			pos += 46 + nameLength + _le16(e + 30) + _le16(e + 32);
			file->_info.localHeaderOffset = _le32(e + 42);
			file->_info.dataOffset = 0;
			if (_files == 0) {
				_files = LComparableSet::_alloc()->init();
				_files->preserveCapacity(entries);
			}
			_files->addObject(file);
			file->release();
		}
		return true;
	}
	return false;
}

// CZipFileManager

CZipFileManager::CZipFileManager(const LString *root) : LFileSystemFileManager(root), _readers(0)
{
	StAutoreleasePool pool;
	sManager = this;
	_readers = new TArray<CZipReader *>;

	LString *zipExtension = LString::stringWithCString(".zip");
	LArray *files = LFileSystemFileManager::directoryContentsAtPath(LString::stringWithCString(""));
	for (int i = files ? (int)files->count() - 1 : -1; i >= 0; i--)
		if (!((LString *)files->objectAtIndex(i))->hasSuffix(zipExtension))
			files->removeObjectAtIndex(i);

	LSingleProfilerPoint profile("ZIP FileManager initialization");
	int n = files ? files->count() : 0;
	LArray *dates = LArray::array();	// of the readers, newest first
	for (int i = 0; i < n; i++) {
		LString *file = (LString *)files->objectAtIndex(i);
		LMap *attributes = LFileSystemFileManager::fileAttributesAtPath(file);
		LObject *obj = attributes ? attributes->objectForKey(fileAttrCreationDate) : 0;
		if (obj == 0 || !obj->getClass()->isSubclassOf(LDate::classObject()))
			continue;
		LDate *date = (LDate *)obj;
		int index;
		for (index = 0; index < (int)dates->count(); index++)
			if (((LDate *)dates->objectAtIndex(index))->timeIntervalSinceReferenceDate() < date->timeIntervalSinceReferenceDate())
				break;
		LString *path = LFileSystemFileManager::systemPathFromInternalPath(file);
		CZipReader *reader = new CZipReader;
		dates->insertObjectAtIndex(date, index);
		if (!reader->initWithFile(path, 0)) {
			delete reader;
			continue;
		}
		if (index > (int)_readers->count())
			index = _readers->count();
		_readers->insertObjectAtIndex(reader, index);
	}
}

CZipFileManager::~CZipFileManager()
{
	for (int i = 0; i < (int)_readers->count(); i++)
		delete (*_readers)[i];
	_readers->removeAllObjects();
	delete _readers;
	sManager = 0;
}

void CZipFileManager::oneArgFunc(LString *name, LString *key)
{
	if (name == 0 || key == 0)
		return;
	int n = _readers ? _readers->count() : 0;
	for (int i = 0; i < n; i++) {
		LString *path = (*_readers)[i]->_path;
		if (!name->isEqualTo(path->lastPathComponent()))
			continue;
		LAutorelease(LRetain(path));
		delete (*_readers)[i];
		_readers->removeObjectAtIndex(i);
		CZipReader *reader = new CZipReader;
		if (reader->initWithFile(path, key))
			_readers->insertObjectAtIndex(reader, i);
		else
			delete reader;
	}
}

LString *CZipFileManager::systemPathFromInternalPath(const LString *path)
{
	return LFileSystemFileManager::systemPathFromInternalPath(path);
}

bool CZipFileManager::dataFromFile(const LString *path, StPointerBlock **block)
{
	if (LFileSystemFileManager::dataFromFile(path, block))
		return true;
	if (block)
		*block = 0;
	LString *native = UTextUtils::convertPathToNativePath(path);
	for (int i = 0; i < (int)_readers->count(); i++) {
		StPointerBlock *data = (*_readers)[i]->loadFile(native);
		if (data) {
			*block = data;
			return true;
		}
	}
	return false;
}

bool CZipFileManager::loadFileIntoData(const LString *path, LData *data)
{
	if (LFileSystemFileManager::loadFileIntoData(path, data))
		return true;
	LString *native = UTextUtils::convertPathToNativePath(path);
	for (int i = 0; i < (int)_readers->count(); i++)
		if ((*_readers)[i]->loadFileIntoData(native, data))
			return true;
	return false;
}

LStreamID CZipFileManager::openReadStreamForFile(const LString *path)
{
	LStreamID file = LFileSystemFileManager::openReadStreamForFile(path);
	if (file) {
		StreamID *stream = new StreamID();
		stream->inZip = false;
		stream->fileStream = file;
		return (LStreamID)stream;
	}
	LString *native = UTextUtils::convertPathToNativePath(path);
	for (int i = 0; i < (int)_readers->count(); i++) {
		SZipStream *zip = (*_readers)[i]->openStream(native);
		if (zip) {
			StreamID *stream = new StreamID();
			stream->inZip = true;
			stream->zipStream = zip;
			stream->reader = i;
			return (LStreamID)stream;
		}
	}
	return 0;
}

LStreamID CZipFileManager::openWriteStreamForFile(const LString *path)
{
	LStreamID file = LFileSystemFileManager::openWriteStreamForFile(path);
	if (file == 0)
		return 0;
	StreamID *stream = new StreamID();
	stream->inZip = false;
	stream->fileStream = file;
	return (LStreamID)stream;
}

LStreamID CZipFileManager::openAppendStreamForFile(const LString *path)
{
	LStreamID file = LFileSystemFileManager::openAppendStreamForFile(path);
	if (file == 0)
		return 0;
	StreamID *stream = new StreamID();
	stream->inZip = false;
	stream->fileStream = file;
	return (LStreamID)stream;
}

size_t CZipFileManager::loadDataFromStream(LStreamID id, long long offset, void *buffer, unsigned int length)
{
	StreamID *stream = (StreamID *)id;
	if (stream == 0)
		return 0;
	if (!stream->inZip)
		return LFileSystemFileManager::loadDataFromStream(stream->fileStream, offset, buffer, length);
	// A member is read whole; the offset doesn't count.
	if (stream->zipStream == 0)
		return 0;
	return stream->zipStream->loadData((*_readers)[stream->reader]->_stream, buffer, length);
}

size_t CZipFileManager::appendDataToStream(LStreamID id, const void *bytes, unsigned int length)
{
	StreamID *stream = (StreamID *)id;
	return stream ? LFileSystemFileManager::appendDataToStream(stream->fileStream, bytes, length) : 0;
}

bool CZipFileManager::closeStreamWithID(LStreamID id)
{
	StreamID *stream = (StreamID *)id;
	if (stream == 0)
		return false;
	bool ok;
	if (!stream->inZip)
		ok = LFileSystemFileManager::closeStreamWithID(stream->fileStream);
	else
		ok = (*_readers)[stream->reader]->closeStream(stream->zipStream);
	delete stream;
	return ok;
}

long long CZipFileManager::streamSize(LStreamID id)
{
	StreamID *stream = (StreamID *)id;
	if (stream == 0)
		return 0;
	if (!stream->inZip)
		return LFileSystemFileManager::streamSize(stream->fileStream);
	return stream->zipStream->_info ? stream->zipStream->_info->uncompressedSize : 0;
}

long long CZipFileManager::offsetInStream(LStreamID id)
{
	StreamID *stream = (StreamID *)id;
	if (stream == 0)
		return -1;
	if (!stream->inZip)
		return LFileSystemFileManager::offsetInStream(stream->fileStream);
	return stream->zipStream->_position;
}

long long CZipFileManager::seekToStreamOffset(LStreamID id, long long offset)
{
	StreamID *stream = (StreamID *)id;
	if (stream == 0)
		return -1;
	if (stream->inZip)
		return stream->zipStream->seekToStreamOffset(offset);
	return LFileSystemFileManager::seekToStreamOffset(stream->fileStream, offset);
}

bool CZipFileManager::isReadableFileAtPath(const LString *path)
{
	if (LFileSystemFileManager::isReadableFileAtPath(path))
		return true;
	bool found = false;
	LString *native = UTextUtils::convertPathToNativePath(path);
	for (int i = 0; i < (int)_readers->count(); i++)
		if ((*_readers)[i]->isFileExist(native))
			found = true;
	return found;
}

bool CZipFileManager::objectExistsAtPath(const LString *path, unsigned char *isFolder)
{
	if (LFileSystemFileManager::objectExistsAtPath(path, isFolder))
		return true;
	if (isFolder)
		*isFolder = false;
	bool found = false;
	LString *native = UTextUtils::convertPathToNativePath(path);
	for (int i = 0; i < (int)_readers->count(); i++)
		if ((*_readers)[i]->isFileExist(native))
			found = true;
	return found;
}

LArray *CZipFileManager::directoryContentsAtPath(const LString *path)
{
	LArray *contents = LFileSystemFileManager::directoryContentsAtPath(path);
	if (path && path->isAbsolutePath() && !path->hasPrefix(_root))
		return contents;
	LString *native = UTextUtils::convertPathToNativePath(path);
	for (int i = 0; i < (int)_readers->count(); i++)
		(*_readers)[i]->directoryContentsAtPath(&contents, native);
	// Without duplicates.
	LSet *set = LAutorelease(LComparableSet::_alloc()->initWithArray(contents));
	LArray *all = set->allObjects();
	if (all && all->getClass()->isSubclassOf(LArray::classObject()))
		return all;
	return 0;
}

// LZipSingleFileWrapper

LZipSingleFileWrapper *LZipSingleFileWrapper::initWithContentsOfFile(LString *path)
{
	if (LObject::init() == 0)
		return 0;
	_reader = new CZipReader;
	if (_reader->initWithFile(path, 0))
		return this;
	dealloc();
	return 0;
}

void LZipSingleFileWrapper::dealloc()
{
	if (_reader) {
		delete _reader;
		_reader = 0;
	}
	LObject::dealloc();
}

bool LZipSingleFileWrapper::isDirectoryAtPath(LString *path)
{
	return _reader->isDirectoryAtPath(path);
}

LArray *LZipSingleFileWrapper::directoryContentsAtSubpath(const LString *path)
{
	LArray *contents = LArray::array();
	_reader->directoryContentsAtPath(&contents, UTextUtils::convertPathToNativePath(path));
	return contents;
}

LData *LZipSingleFileWrapper::dataWithContentsOfFile(LString *path)
{
	LData *data = LData::dataWithLength(0);
	return _reader->loadFileIntoData(path, data) ? data : 0;
}

LData *LZipSingleFileWrapper::mappedDataWithContentsOfFile(LString *path)
{
	return _reader->mappedDataWithContentsOfFile(path);
}

} // namespace LF
