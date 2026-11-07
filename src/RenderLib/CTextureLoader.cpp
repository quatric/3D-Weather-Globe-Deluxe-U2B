// CTextureLoader: reads the earth patches' DDS files on a background queue,
// keeps up to 30 of them decompressed, and copies them into textures reused
// from patches no longer drawn. Also the city lights textures.

#include "RenderLib.h"
#include "UTextUtils.h"

LF_IMPLEMENT_CLASS(CTextureLoader, 0)
LF_SELECTOR(CTextureLoader, _textureWasDecompressed);
LF_SELECTOR(CTextureLoader, _contextWasDetached);

bool sLargeBBGrabbingIsActive = false;

CTextureLoader::CTextureLoader()
    : _maxDecompressedCount(30), _requested(0), _decompressed(0), _textures(0), _unusedTextures(0),
      _glowTextures(0), _glowsUnusedSince(0), _usedGlows(0), _emptyGlows(0), _holder(0), _queue(0),
      _postNotifications(true)
{
}

void CTextureLoader::invalidate()
{
	if (_queue)
		_queue->terminateQueue(1);
	_queue = 0;
	release();
}

void CTextureLoader::holderWillDie(CHolder *)
{
	_holder = 0;
}

LString *CTextureLoader::nightGlowsTexturePathForID(const STextureID &id)
{
	int n = textureRowsAtLevel(id.level);
	const char *side = id.x < n ? "east" : "west";
	LString *path = LString::stringWithFormat("CityGlows\\%s\\%s%d_%02d.dds", side, side, n,
	                                          id.x % n + 1 + (n - id.y - 1) * n);
	return UTextUtils::convertPathToNativePath(path);
}

void CTextureLoader::freeOldestTextureData()
{
	if (_decompressed == 0 || _unusedSince.pairsCount() == 0)
		return;
	unsigned int oldest = 0xffffffff;
	intptr_t key = 0;
	CHashTableIterator it(&_unusedSince);
	while (it.next()) {
		if ((unsigned int)it.value() <= oldest) {
			key = it.key();
			oldest = (unsigned int)it.value();
		}
	}
	_decompressed->removeEntryWithKey(key);
	_unusedSince.removeValueForKey(key);
}

intptr_t CTextureLoader::_contextWasDetached(char *)
{
	if (_textures) {
		delete _textures;
		_textures = 0;
	}
	return 0;
}

void CTextureLoader::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	if (_queue)
		_queue->terminateQueue(1);
	delete _decompressed;
	_decompressed = 0;
	delete _textures;
	_textures = 0;
	delete _requested;
	_requested = 0;
	delete _unusedTextures;
	_unusedTextures = 0;
	delete _glowTextures;
	_glowTextures = 0;
	delete _glowsUnusedSince;
	_glowsUnusedSince = 0;
	delete _usedGlows;
	_usedGlows = 0;
	releaseAndClear(_emptyGlows);
	if (_holder)
		_holder->removeObjectForKey(kTextureLoaderKey);
	LObject::dealloc();
}

// The earth textures: one file for levels 0 and 1, per hemisphere; then
// sheets of 9 or 18 rows; levels 4 and 5 in 30-degree-wide HiRes folders.
LString *CTextureLoader::texturePathForID(const STextureID &id)
{
	int x = id.x;
	int y = id.y;
	int level = id.level;
	int merged = STextureID::columnsMergedInRow(level, y);
	char path[212];
	path[0] = 0;
	switch (level) {
	case 0:
		sprintf(path, "TEXTURES\\EARTH\\%s\\1\\1.%s", x * merged < 1 ? "East" : "West", "dds");
		break;
	case 1:
		sprintf(path, "TEXTURES\\EARTH\\%s\\3\\%d.%s", x * merged < 3 ? "East" : "West",
		        x % 3 + 1 + (short)(2 - y) * 3, "dds");
		break;
	case 2:
		sprintf(path, "TEXTURES\\EARTH\\%s\\9\\1_%02d.%s", x < 9 ? "East" : "West", x % 9 + 1 + (short)(8 - y) * 9,
		        "dds");
		break;
	case 3:
		sprintf(path, "TEXTURES\\EARTH\\%s\\18\\%d_%02d.%s", x < 18 ? "East" : "West",
		        (short)((x % 18) / 9) + 1 + ((short)(17 - y) / 9) * 2, x % 9 + 1 + ((short)(17 - y) % 9) * 9, "dds");
		break;
	case 4:
	case 5: {
		// The 3x3 (level 4) or 9x9 (level 5) sheet of a 10-degree cell.
		int cellX, cellY, file;
		bool polar;
		if (level == 4) {
			cellX = x / 3;
			cellY = y / 3;
			file = x % 9 + 1 + (2 - y % 3) * 9;
			polar = y > 44;
		} else {
			cellX = x / 9;
			cellY = y / 9;
			file = x % 9 + 1 + (8 - y % 9) * 9;
			polar = y > 134;
		}
		int sub = level == 4 ? 1 : cellX % 3 + 1;
		int from = (cellX / 3) * 3;
		int to = from + 3;
		if (polar) {
			from = (from / 3) * 6;
			to = from + 6;
		}
		bool west = false;
		if (from > 17) {
			int f = from;
			from = 36 - f;
			to = (polar ? 30 : 33) - f;
			west = true;
		}
		int lat = cellY < 9 ? 8 - cellY : cellY - 9;
		sprintf(path, "TEXTURES\\EARTH\\HiRes\\%s%03d_%03d-%s%02d_%02d\\%d\\%d_%02d.%s", west ? "w" : "e", from * 10,
		        to * 10, cellY < 9 ? "s" : "n", lat * 10, (lat * 5 + 5) * 2, level, sub, file, "dds");
		break;
	}
	}
	return LString::stringWithCString(path);
}

int CTextureLoader::requestTextureBeforeDraw(const STextureID &requested)
{
	STextureID id = requested;
	// The finest patch with a file.
	do {
		intptr_t exists = 0;
		if (!_fileExists.getValueForKey(&exists, keyForID(id))) {
			exists = LFileManager::manager()->isReadableFileAtPath(texturePathForID(id)) ? 1 : 0;
			_fileExists.setValueForKey(exists, keyForID(id));
		}
		if (exists)
			break;
	} while (id.scaleDownIfPossible());
	_requested->addObject(keyForID(id));
	_unusedTextures->removeObject(keyForID(id));
	if (!sLargeBBGrabbingIsActive) {
		// The finest of them already read.
		do {
			if (_decompressed) {
				TMapEntry<intptr_t, CTextureDecompressionTask *> **e = _decompressed->entryPtrAddrForKey(keyForID(id));
				if (e && (*e)->value)
					break;
			}
		} while (id.scaleDownIfPossible());
	}
	return id.level;
}

BaseTextureEmu *CTextureLoader::nightGlowsTextureAtLevel(const STextureID &id)
{
	if (_holder == 0)
		return 0;
	if (_glowTextures == 0)
		_glowTextures = new TLIntPtrMap<C2DTexture *>;
	intptr_t key = keyForID(id);
	C2DTexture *texture = _glowTextures->objectForKey(key);
	if (texture == 0) {
		LString *path = nightGlowsTexturePathForID(id);
		if (!LFileManager::manager()->objectExistsAtPath(path, 0)) {
			if (_emptyGlows == 0) {
				_holder->beginAccumulationBlock();
				_emptyGlows = C2DTexture::_alloc()->initWithHolderAndPath(
				    _holder, LString::stringWithCString("TEXTURES\\empty_glows.dds"));
				_holder->endAccumulationBlock();
			}
			return _emptyGlows ? _emptyGlows->provideTexture() : 0;
		}
		if (_glowsUnusedSince && _glowsUnusedSince->count() != 0) {
			// Reuse the texture unused the longest.
			double oldest = -1.0;
			intptr_t oldestKey = -1;
			TLIntPtrMap<LValue *>::hashTableIterator it(_glowsUnusedSince);
			while (it.next()) {
				double t = it.entry()->value->doubleValue();
				if (oldest < 0.0 || t < oldest) {
					oldest = t;
					oldestKey = it.entry()->key;
				}
			}
			if (oldestKey != -1) {
				texture = _glowTextures->objectForKey(oldestKey);
				if (texture) {
					texture->retain();
					_glowTextures->removeEntryWithKey(oldestKey);
					_glowsUnusedSince->removeEntryWithKey(oldestKey);
					_glowTextures->setObjectForKey(texture, key);
					texture->release();
					ImageDecompressor *decompressor =
					    ImageDecompressor::decompressorForData(LData::dataWithContentsOfFile(path));
					if (decompressor)
						decompressor->decompressToTexture(texture);
				}
				goto used;
			}
		}
		_holder->beginAccumulationBlock();
		texture = C2DTexture::_alloc()->initWithHolderAndPath(_holder, path);
		_glowTextures->setObjectForKey(texture, key);
		texture->release();
		_holder->endAccumulationBlock();
	}
used:
	if (_usedGlows == 0)
		_usedGlows = new CHashSet;
	_usedGlows->addObject(key);
	if (_glowsUnusedSince)
		_glowsUnusedSince->removeEntryWithKey(key);
	return texture ? texture->provideTexture() : 0;
}

// Reads the patch now, or queues it (once).
void CTextureLoader::postTextureDecompressionTask(const STextureID &id, unsigned char now)
{
	intptr_t key = keyForID(id);
	if (!now) {
		if (_postedTasks.containsObject(key))
			return;
		_postedTasks.addObject(key);
		if (_queue == 0) {
			_queue = LTaskQueue::detachNewQueue();
			_queue->runQueue();
		}
	}
	CTextureDecompressionTask *task = LAutorelease(CTextureDecompressionTask::_alloc()->initWithParams(
	    texturePathForID(id), D3DFMT_UNKNOWN, this, "_textureWasDecompressed", (void *)key));
	task->_taskID = (int)key;
	if (now) {
		StAutoreleasePool pool;
		bool post = _postNotifications;
		_postNotifications = false;
		task->executeAndInvokeSelector(0);
		_postNotifications = post;
		return;
	}
	_queue->postTaskToQueue(task);
}

BaseTextureEmu *CTextureLoader::textureAtLevel(const STextureID &id, unsigned char force)
{
	intptr_t key = keyForID(id);
	CTextureDecompressionTask *task = _decompressed ? _decompressed->objectForKey(key) : 0;
	if ((id.level == 0 || sLargeBBGrabbingIsActive) && task == 0)
		postTextureDecompressionTask(id, 1);
	if (_textures) {
		C2DTexture *texture = _textures->objectForKey(key);
		if (texture)
			return texture->provideTexture();
	}
	task = _decompressed ? _decompressed->objectForKey(key) : 0;
	if (force && task == 0) {
		postTextureDecompressionTask(id, 1);
		task = _decompressed ? _decompressed->objectForKey(key) : 0;
		if (task == 0)
			return 0;
	}
	if (task == 0)
		return 0;
	D3DFORMAT format = task->_format;
	// A texture of the format no patch drawn uses, or a new one.
	C2DTexture *texture = 0;
	if (_textures && _unusedTextures) {
		CHashSetIterator it(_unusedTextures);
		while (it.next()) {
			intptr_t unusedKey = it.object();
			C2DTexture *unused = _textures->objectForKey(unusedKey);
			if (unused && unused->_info.Format == format) {
				_unusedTextures->removeObject(unusedKey);
				unused->retain();
				_textures->removeEntryWithKey(unusedKey);
				_textures->setObjectForKey(unused, key);
				unused->release();
				texture = unused;
				break;
			}
		}
	}
	if (texture == 0 && _holder) {
		CHolder *holder = _holder;
		holder->beginAccumulationBlock();
		texture = LAutorelease(C2DTexture::_alloc()->initWithHolderAndParams(_holder, 1024, 1024, format, 0, 0));
		if (_textures == 0)
			_textures = new TLIntPtrMap<C2DTexture *>;
		_textures->setObjectForKey(texture, key);
		holder->endAccumulationBlock();
	}
	LArray *levels = task->_results;
	if (texture && levels) {
		int count = levels->count();
		for (int i = 0; i < count; i++) {
			LData *data = lobj_cast<LData>(levels->objectAtIndex(i));
			if (data) {
				C2DTexture::StLocker locker(texture, i, i == 0 ? D3DLOCK_DISCARD : 0);
				memcpy(locker.bits(), data->bytes(), data->length());
			}
		}
	}
	return texture ? texture->provideTexture() : 0;
}

// The water mask files listed in the resource (a property list array).
LComparableSet *CTextureLoader::_waterFilesFromResourceData(LData *data)
{
	if (data == 0)
		return 0;
	LArray *files = lobj_cast<LArray>(data->propertyList());
	if (files == 0)
		return 0;
	LComparableSet *set = LComparableSet::_alloc()->init();
	int count = files->count();
	for (int i = 0; i < count; i++) {
		LString *file = lobj_cast<LString>(files->objectAtIndex(i));
		if (file)
			set->addObject(file);
	}
	return set;
}

intptr_t CTextureLoader::_textureWasDecompressed(char *args)
{
	CTextureDecompressionTask *task = *(CTextureDecompressionTask **)args;
	task->cleanup();
	intptr_t key = (intptr_t)task->_context;
	if (_decompressed == 0)
		_decompressed = new TLIntPtrMap<CTextureDecompressionTask *>;
	if (task->_results)
		_decompressed->setObjectForKey(task, key);
	_postedTasks.removeObject(key);
	if (_postNotifications) {
		LMap *info = LMap::mapWithObjectsAndKeys(LValue::valueWithInt((int)key), kTextureIDAsInt, (LObject *)0);
		LNotificationCenter::center()->postNotification(loadedSetWasUpdatedNotification, this, info);
	}
	return 0;
}

void CTextureLoader::startNextFrame()
{
	// A frame stamp that grows with time (wrapping).
	double t = LTimer::currentTimeValue() * 100.0 / 4294967295.0;
	double stamp = (t - floor(t)) * 4294967295.0;
	if (stamp > 4294967295.0)
		stamp = 4294967295.0;
	if (stamp < 0.0)
		stamp = 0.0;
	if (_requested && _decompressed) {
		// Read patches not drawn in the last frame start aging.
		TLIntPtrMap<CTextureDecompressionTask *>::hashTableIterator it(_decompressed);
		while (it.next()) {
			intptr_t key = it.entry()->key;
			if ((char)(key >> 24) > 1 && !_requested->containsObject(key) && !_unusedSince.getValueForKey(0, key))
				_unusedSince.setValueForKey((intptr_t)(unsigned int)stamp, key);
		}
	}
	int room = _maxDecompressedCount - (_decompressed ? (int)_decompressed->count() : 0);
	for (int i = 0; i < 5 && room < 0 && _unusedSince.pairsCount() != 0; i++) {
		freeOldestTextureData();
		room = _maxDecompressedCount - (_decompressed ? (int)_decompressed->count() : 0);
	}
	if (_requested) {
		// Read what was drawn and isn't read yet; the top level at once.
		CHashSetIterator it(_requested);
		while (it.next()) {
			intptr_t key = it.object();
			if (_decompressed == 0 || _decompressed->objectForKey(key) == 0) {
				if (room < 0 && _unusedSince.pairsCount() != 0)
					freeOldestTextureData();
				STextureID id;
				id.x = (short)(key & 0xfff);
				id.y = (short)((key >> 12) & 0xfff);
				id.level = (char)(key >> 24);
				postTextureDecompressionTask(id, id.level == 0);
				room--;
			}
		}
		if (_unusedTextures) {
			// Patches no longer drawn needn't be read.
			TArray<intptr_t> dropped;
			CHashSetIterator posted(&_postedTasks);
			while (posted.next()) {
				intptr_t key = posted.object();
				if (!_requested->containsObject(key)) {
					if (_queue)
						_queue->removeTaskWithID((int)key, 0);
					dropped.addObject(key);
				}
			}
			for (unsigned int i = 0; i < dropped.count(); i++)
				_postedTasks.removeObject(dropped[i]);
		}
		delete _requested;
		_requested = 0;
	}
	_requested = new CHashSet;
	delete _unusedTextures;
	_unusedTextures = new CHashSet;
	if (_textures) {
		TLIntPtrMap<C2DTexture *>::hashTableIterator it(_textures);
		while (it.next())
			_unusedTextures->addObject(it.entry()->key);
	}
	if (_glowsUnusedSince == 0)
		_glowsUnusedSince = new TLIntPtrMap<LValue *>;
	if (_glowTextures) {
		TLIntPtrMap<C2DTexture *>::hashTableIterator it(_glowTextures);
		while (it.next()) {
			intptr_t key = it.entry()->key;
			if (_usedGlows && !_usedGlows->containsObject(key) && _glowsUnusedSince->entryPtrAddrForKey(key) == 0)
				_glowsUnusedSince->setObjectForKey(LValue::valueWithDouble(LTimer::currentTimeValue()), key);
		}
	}
	if (_usedGlows) {
		delete _usedGlows;
		_usedGlows = 0;
	}
}

CTextureLoader *CTextureLoader::initWithHolder(CHolder *holder)
{
	_holder = holder;
	holder->setObjectForKey(this, kTextureLoaderKey);
	LNotificationCenter::center()->addObserver(this, "_contextWasDetached", &ContextWillBeDetachedNotification,
	                                           _holder);
	startNextFrame();
	// The coarse levels, to draw something at once.
	STextureID id;
	id.y = 0;
	id.level = 0;
	for (id.x = 0; id.x < 2; id.x++)
		requestTextureBeforeDraw(id);
	id.level = 1;
	for (int x = 0; x < 6; x += 3)
		for (id.y = 0; id.y < 3; id.y++)
			for (id.x = (short)x; id.x < x + 3; id.x++)
				requestTextureBeforeDraw(id);
	return this;
}
