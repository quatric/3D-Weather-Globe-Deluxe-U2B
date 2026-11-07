// EntryInfo: the info items of an entry, merged from three sources: the
// entry's own properties, the atlas database's info tree (read-only) and the
// user database's info tree. Item indices of a key count the property first
// (when the entry has one), then the atlas items, then the user items.
//
// EntryInfoImageWrapper: an image item. Atlas images are BLOBs; user images
// are files in the entry's media data folder, kept in the temporary folder
// until the entry is saved. Thumbnails and textures are made on demand.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(EntryInfo, 0)
LF_IMPLEMENT_CLASS(EntryInfoImageWrapper, 0)

// ---------------------------------------------------------------------------
// EntryInfo

EntryInfo::EntryInfo()
    : _host(0), _imagesChanged(false), _atlasTree(0), _userTree(0), _mapsInited(false), _density(0),
      _densityComputed(false), _imageWrappers(0)
{
}

EntryInfo *EntryInfo::initWithHostEntry(GenericEntry *host)
{
	if (!LObject::init())
		return 0;
	_host = host;
	return this;
}

void EntryInfo::dealloc()
{
	if (_atlasTree) {
		_atlasTree->release();
		_atlasTree = 0;
	}
	if (_userTree) {
		_userTree->release();
		_userTree = 0;
	}
	if (_imageWrappers) {
		_imageWrappers->release();
		_imageWrappers = 0;
	}
	if (_density) {
		_density->release();
		_density = 0;
	}
	LObject::dealloc();
}

bool EntryInfo::shouldCommit() const
{
	return _imagesChanged || (_userTree && _userTree->isDirty());
}

// The wrapper of an image item's subtree (uid, description, aspect ratio).
EntryInfoImageWrapper *EntryInfo::imageWrapperForDBTree(DBTreeElement *tree, unsigned char atlas, unsigned char create)
{
	LString *hostUID = _host ? _host->uid(0) : 0;
	LString *imageUID = 0;
	LString *description = 0;
	float aspectRatio = -1.0f;
	if (tree) {
		imageUID = lobj_cast<LString>(tree->firstValueForKey(EntryInfoImageWrapper::kImageUIDKey));
		description = lobj_cast<LString>(tree->firstValueForKey(EntryInfoImageWrapper::kDescriptionKey));
		LObject *ratio = tree->firstValueForKey(EntryInfoImageWrapper::kAspectRatioKey);
		if (ratio)
			aspectRatio = UMapParsingUtils::loadFloatFromObject(ratio);
	}
	return EntryInfoImageWrapper::wrapperWithParams(hostUID, imageUID, description, aspectRatio, atlas, create);
}

// 1 when the key is one of the entry's own properties.
int EntryInfo::propertyKeysCountForKey(LString *key, TEntryKeySet set, TEntryKeyAccessMode mode) const
{
	LSet *keys = _host ? _host->propertyKeys(set, mode) : 0;
	return (keys && keys->containsObject(key)) ? 1 : 0;
}

bool EntryInfo::canAddItemsForKey(LString *key, TEntryKeySet set)
{
	return propertyKeysCountForKey(key, set, kEntryKeyWrite) == 0;
}

void EntryInfo::initMaps()
{
	if (_mapsInited || _host == 0)
		return;
	LString *uid = _host->uid(0);
	if (uid == 0)
		return;
	_atlasTree = InfoTreeRootElement::_alloc()->initWithGeoObjectUID(uid, 0);
	_userTree = InfoTreeRootElement::_alloc()->initWithGeoObjectUID(uid, 1);
	_mapsInited = true;
}

// Property keys, the trees' keys (the atlas tree's only when reading), and
// the density when the area and the population are known.
LSet *EntryInfo::allKeys(TEntryKeySet set, TEntryKeyAccessMode mode)
{
	LSet *keys = (LSet *)LComparableSet::_alloc()->init()->l_autorelease();
	initMaps();
	if (mode == kEntryKeyRead && _atlasTree)
		if (LSet *atlasKeys = _atlasTree->allKeys())
			keys->unionSet(atlasKeys);
	if (_userTree)
		if (LSet *userKeys = _userTree->allKeys())
			keys->unionSet(userKeys);
	if (_host) {
		if (LSet *propertyKeys = _host->propertyKeys(set, mode))
			keys->unionSet(propertyKeys);
		if (_host->population(0) > 0 &&
		    (keys->containsObject(kEntryInfoAreaKey) || keys->containsObject(kEntryInfoTotalAreaKey)))
			keys->addObject(kEntryInfoDensityKey);
	}
	return keys;
}

LSet *EntryInfo::imageWrappers()
{
	if (_imageWrappers == 0)
		_imageWrappers = LSet::_alloc()->init();
	return _imageWrappers;
}

// People per area unit, from the atlas area; 0 under 0.01.
LValue *EntryInfo::density()
{
	if (_densityComputed)
		return _density;
	_densityComputed = true;
	LSet *keys = _atlasTree ? _atlasTree->allKeys() : 0;
	if (keys == 0 || _host == 0)
		return 0;
	LString *areaKey = keys->containsObject(kEntryInfoAreaKey)
	                       ? kEntryInfoAreaKey
	                       : (keys->containsObject(kEntryInfoTotalAreaKey) ? kEntryInfoTotalAreaKey : 0);
	LObject *area = areaKey ? _atlasTree->firstValueForKey(areaKey) : 0;
	if (area == 0)
		return 0;
	double value = UMapParsingUtils::loadDoubleFromObject(area);
	if (value > 0.0) {
		double density = (double)_host->population(0) / value;
		if (density >= 0.009999999776482582)
			_density = LRetain(LValue::valueWithDouble(density));
	}
	return _density;
}

int EntryInfo::mapItemsCountForKey(LString *key, TKeySet set)
{
	initMaps();
	if (key->isEqualTo(kEntryInfoDensityKey) && set != kKeySetUser && density())
		return 1;
	int count = 0;
	DBTreeElement *tree = _atlasTree;
	if (set == kKeySetAll)
		count = mapItemsCountForKey(key, kKeySetUser);
	else if (set == kKeySetUser)
		tree = _userTree;
	if (tree)
		if (LArray *values = tree->valuesForKey(key))
			count += values->count();
	return count;
}

LObject *EntryInfo::mapItemForKeyAtIndex(LString *key, int index, TKeySet set)
{
	initMaps();
	if (set == kKeySetAtlas || set == kKeySetAll) {
		int atlasCount = mapItemsCountForKey(key, kKeySetAtlas);
		if (index < atlasCount) {
			if (key->isEqualTo(kEntryInfoDensityKey) && density())
				return _density;
			LArray *values = _atlasTree ? _atlasTree->valuesForKey(key) : 0;
			return values ? values->objectAtIndex(index) : 0;
		}
		index -= atlasCount;
	}
	if ((set == kKeySetAll || set == kKeySetUser) && index < mapItemsCountForKey(key, kKeySetUser) && _userTree)
		if (LArray *values = _userTree->valuesForKey(key))
			return values->objectAtIndex(index);
	return 0;
}

bool EntryInfo::canRemoveItemForKeyAtIndex(LString *key, int index, TEntryKeySet set)
{
	int propertyCount = propertyKeysCountForKey(key, set, kEntryKeyWrite);
	return index >= propertyCount && index - propertyCount < mapItemsCountForKey(key, kKeySetUser);
}

// Image items come back as EntryInfoImageWrappers.
LObject *EntryInfo::itemForKeyAtIndex(LString *key, int index, TEntryKeySet set, TEntryKeyAccessMode mode)
{
	int propertyCount = propertyKeysCountForKey(key, set, mode);
	LObject *item;
	if (index < propertyCount) {
		if (_host == 0)
			return 0;
		item = _host->propertyForKey(key);
	} else {
		index -= propertyCount;
		item = mapItemForKeyAtIndex(key, index, mode == kEntryKeyRead ? kKeySetAll : kKeySetUser);
	}
	if (item == 0 || !kEntryInfoImagesKey->isEqualTo(key))
		return item;
	bool atlas = mode == kEntryKeyRead && index < mapItemsCountForKey(key, kKeySetAtlas);
	EntryInfoImageWrapper *wrapper = imageWrapperForDBTree(lobj_cast<DBTreeElement>(item), atlas, 1);
	if (wrapper)
		imageWrappers()->addObject(wrapper);
	return wrapper;
}

int EntryInfo::itemsCountForKey(LString *key, TEntryKeySet set, TEntryKeyAccessMode mode)
{
	return propertyKeysCountForKey(key, set, mode) +
	       mapItemsCountForKey(key, mode == kEntryKeyRead ? kKeySetAll : kKeySetUser);
}

void EntryInfo::_postChange(LString *key, int index, TEntryKeySet set, LString *operation)
{
	LMap *info = LMap::mapWithObjectsAndKeys(LString::stringWithString(key), kChangedKey, LValue::valueWithInt(index),
	                                         kChangedIndex, LValue::valueWithInt(set), kChangedKeySet, operation,
	                                         kOperationKind, 0);
	LNotificationCenter::center()->postNotification(kDidChangeItemNotification, this, info);
}

// Map items are stored as subtrees of their string, number and data values.
static void _fillSubtreeWithMap(DBTreeElement *subtree, LMap *map)
{
	LMapIterator it(map, true);
	while (it.next()) {
		LObject *value = it.object();
		if (LValue *number = lobj_cast<LValue>(value))
			value = number->description();
		else if (!lobj_cast<LString>(value) && !lobj_cast<LData>(value))
			value = 0;
		if (value)
			subtree->addValueForKey(value, (LString *)it.key());
	}
}

// Puts the value at the user index, as a subtree when it is a map.
void EntryInfo::_setUserItemAtIndex(LObject *value, LString *key, int index, bool replace)
{
	if (LMap *map = lobj_cast<LMap>(value)) {
		if (replace)
			_userTree->removeValueForKeyAtIndex(key, index);
		_fillSubtreeWithMap(_userTree->createSubtreeForKeyAtIndex(key, index), map);
	} else if (replace) {
		_userTree->replaceValueForKeyAtIndex(value, key, index);
	} else {
		_userTree->insertValueForKeyAtIndex(value, key, index);
	}
}

bool EntryInfo::removeItemForKeyAtIndex(LString *key, int index, TEntryKeySet set)
{
	int propertyCount = propertyKeysCountForKey(key, set, kEntryKeyWrite);
	if (index < propertyCount)
		return false;
	index -= propertyCount;
	initMaps();
	LArray *values = _userTree ? _userTree->valuesForKey(key) : 0;
	if (values == 0 || index < 0 || index >= (int)values->count())
		return false;
	_userTree->removeValueForKeyAtIndex(key, index);
	if (kEntryInfoImagesKey->isEqualTo(key))
		_imagesChanged = true;
	_postChange(key, index, set, kOperationKindRemove);
	return true;
}

bool EntryInfo::replaceItemForKeyAtIndex(LObject *value, LString *key, int index, TEntryKeySet set)
{
	int propertyCount = propertyKeysCountForKey(key, set, kEntryKeyWrite);
	initMaps();
	if (kEntryInfoImagesKey->isEqualTo(key)) {
		EntryInfoImageWrapper *wrapper = lobj_cast<EntryInfoImageWrapper>(value);
		if (wrapper == 0 || itemForKeyAtIndex(key, index, set, kEntryKeyWrite) == wrapper)
			return false;
		imageWrappers()->addObject(wrapper);
		value = wrapper->mapRepresentation();
		_imagesChanged = true;
	}
	if (index < propertyCount || (mapItemsCountForKey(key, kKeySetUser) == 0 && propertyCount == 0)) {
		if (_host)
			_host->setPropertyForKey(value, key);
	} else {
		index -= propertyCount;
		if (_userTree == 0)
			return false;
		_setUserItemAtIndex(value, key, index, true);
	}
	_postChange(key, index, set, kOperationKindReplace);
	return true;
}

// Saves the user images: images still listed move out of the temporary
// folder, removed ones move into it, and changed descriptions are stored.
bool EntryInfo::commitImageWrappers()
{
	if (_imageWrappers == 0 || _imageWrappers->count() == 0)
		return true;
	LSet *listedUIDs = LSet::_alloc()->init();
	int atlasCount = mapItemsCountForKey(kEntryInfoImagesKey, kKeySetAtlas);
	int userCount = mapItemsCountForKey(kEntryInfoImagesKey, kKeySetUser);
	for (int i = 0; i < atlasCount + userCount; i++) {
		DBTreeElement *tree = lobj_cast<DBTreeElement>(mapItemForKeyAtIndex(kEntryInfoImagesKey, i, kKeySetAll));
		if (tree == 0)
			continue;
		LString *uid = lobj_cast<LString>(tree->firstValueForKey(EntryInfoImageWrapper::kImageUIDKey));
		if (uid == 0)
			continue;
		listedUIDs->addObject(uid);
		if (i >= atlasCount) {
			EntryInfoImageWrapper *wrapper = imageWrapperForDBTree(tree, 0, 0);
			if (wrapper && wrapper->_changed) {
				_setUserItemAtIndex(wrapper->mapRepresentation(), kEntryInfoImagesKey, i - atlasCount, true);
				wrapper->_changed = false;
			}
		}
	}
	LSet *kept = LSet::_alloc()->init();
	LSetIterator it(_imageWrappers, true);
	while (EntryInfoImageWrapper *wrapper = lobj_cast<EntryInfoImageWrapper>(it.nextObject())) {
		bool listed = listedUIDs->containsObject(wrapper->_imageUID);
		wrapper->commit(!listed);
		if (listed)
			kept->addObject(wrapper);
	}
	_imageWrappers->release();
	_imageWrappers = kept;
	listedUIDs->release();
	return true;
}

bool EntryInfo::commit()
{
	if (_imagesChanged) {
		if (commitImageWrappers())
			_imagesChanged = false;
		if (_imagesChanged)
			return false;
	}
	return _userTree ? _userTree->commitChanges() : true;
}

bool EntryInfo::insertItemForKeyAtIndex(LObject *value, LString *key, int index, TEntryKeySet set)
{
	if (value == 0)
		return false;
	int propertyCount = propertyKeysCountForKey(key, set, kEntryKeyWrite);
	if (index < propertyCount)
		return false;
	index -= propertyCount;
	initMaps();
	if (_userTree == 0)
		return false;
	if (kEntryInfoImagesKey->isEqualTo(key)) {
		EntryInfoImageWrapper *wrapper = lobj_cast<EntryInfoImageWrapper>(value);
		if (wrapper == 0)
			return false;
		imageWrappers()->addObject(wrapper);
		value = wrapper->mapRepresentation();
		_imagesChanged = true;
	}
	_setUserItemAtIndex(value, key, index, false);
	_postChange(key, index, set, kOperationKindInsert);
	return true;
}

bool EntryInfo::addItemForKey(LObject *value, LString *key, TEntryKeySet set)
{
	return insertItemForKeyAtIndex(value, key, itemsCountForKey(key, set, kEntryKeyWrite), set);
}

// ---------------------------------------------------------------------------
// EntryInfoImageWrapper

LStrMap *EntryInfoImageWrapper::__imageWrappersCache = 0;
LSize EntryInfoImageWrapper::kTextureSize = { 512.0f, 512.0f };
LSize EntryInfoImageWrapper::kThumbnailSize = { 128.0f, 96.0f };

class _imageWrappersCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		delete EntryInfoImageWrapper::__imageWrappersCache;
		EntryInfoImageWrapper::__imageWrappersCache = 0;
	}
};

static _imageWrappersCleanup _sImageWrappersCleanup;

// The cache key of a wrapper.
LString *EntryInfoImageWrapper::imageKey(LString *hostUID, LString *imageUID)
{
	return hostUID->stringByAppendingString(imageUID);
}

static void _cacheWrapper(EntryInfoImageWrapper *wrapper, LString *key)
{
	if (EntryInfoImageWrapper::__imageWrappersCache == 0)
		EntryInfoImageWrapper::__imageWrappersCache = new LStrMap;
	EntryInfoImageWrapper::__imageWrappersCache->setObjectForKey((intptr_t)wrapper, key);
}

EntryInfoImageWrapper::EntryInfoImageWrapper()
    : _imageUID(0), _hostUID(0), _aspectRatio(0), _description(0), _originalData(0), _originalImage(0),
      _originalDataPath(0), _texturePath(0), _thumbnailPath(0), _changed(false), _atlas(false), _temporary(false),
      _nameCounter(0)
{
}

void EntryInfoImageWrapper::invalidateCachedPaths()
{
	if (_thumbnailPath) {
		_thumbnailPath->release();
		_thumbnailPath = 0;
	}
	if (_texturePath) {
		_texturePath->release();
		_texturePath = 0;
	}
}

void EntryInfoImageWrapper::invalidateAllCachedPaths()
{
	if (__imageWrappersCache == 0)
		return;
	LStrMap::hashTableIterator it(__imageWrappersCache);
	while (it.next())
		((EntryInfoImageWrapper *)it.entry()->value)->invalidateCachedPaths();
}

EntryInfoImageWrapper *EntryInfoImageWrapper::initWithParams(LString *hostUID, LString *imageUID,
                                                             LString *description, float aspectRatio,
                                                             unsigned char atlas)
{
	if (hostUID == 0 || imageUID == 0) {
		dealloc();
		return 0;
	}
	if (!LObject::init())
		return 0;
	_hostUID = LRetain(hostUID);
	_imageUID = LRetain(imageUID);
	_description = LRetain(description);
	_aspectRatio = aspectRatio;
	_atlas = atlas;
	return this;
}

void EntryInfoImageWrapper::setTextDescription(LString *description)
{
	LRetain(description);
	if (_description)
		_description->release();
	_description = description;
	_changed = true;
}

// The name, or the name with " 1", " 2"... before its extension, that no
// file in the entry's media folders has.
LString *EntryInfoImageWrapper::_imageUIDFromImageName(LString *name)
{
	if (name == 0)
		return 0;
	LFileManager *fm = LFileManager::manager();
	AtlasDatabase *db = AtlasDatabase::instance();
	LString *candidate = name;
	for (;;) {
		LString *saved = db->pathForMediaDataFolder(_hostUID, 0);
		saved->appendPathComponent(candidate);
		LString *temporary = db->pathForMediaDataFolder(_hostUID, 1);
		temporary->appendPathComponent(candidate);
		if (!fm->objectExistsAtPath(saved, 0) && !fm->objectExistsAtPath(temporary, 0))
			break;
		LString *extension = name->pathExtension();
		candidate = name->stringByDeletingPathExtension();
		candidate->appendFormat(" %u", ++_nameCounter);
		candidate->appendPathExtension(extension);
	}
	return candidate;
}

void EntryInfoImageWrapper::dealloc()
{
	if (__imageWrappersCache && _hostUID && _imageUID)
		__imageWrappersCache->removeObjectForKey(imageKey(_hostUID, _imageUID));
	LObject **fields[] = { (LObject **)&_hostUID,          (LObject **)&_imageUID,      (LObject **)&_description,
		               (LObject **)&_originalData,     (LObject **)&_originalImage, (LObject **)&_originalDataPath,
		               (LObject **)&_texturePath,      (LObject **)&_thumbnailPath };
	for (LObject **field : fields)
		if (*field) {
			(*field)->release();
			*field = 0;
		}
	LObject::dealloc();
}

// The cached wrapper of the image, or a new one when `create`.
EntryInfoImageWrapper *EntryInfoImageWrapper::wrapperWithParams(LString *hostUID, LString *imageUID,
                                                                LString *description, float aspectRatio,
                                                                unsigned char atlas, unsigned char create)
{
	if (hostUID == 0 || imageUID == 0)
		return 0;
	LString *key = imageKey(hostUID, imageUID);
	EntryInfoImageWrapper *wrapper =
	    __imageWrappersCache ? (EntryInfoImageWrapper *)__imageWrappersCache->objectForKey(key) : 0;
	if (create && wrapper == 0) {
		wrapper = _alloc()->initWithParams(hostUID, imageUID, description, aspectRatio, atlas);
		if (wrapper) {
			_cacheWrapper(wrapper, key);
			wrapper->l_autorelease();
		}
	}
	return wrapper;
}

// The image file. Atlas images are written out of their BLOB on first use.
LString *EntryInfoImageWrapper::originalDataPath()
{
	if (_originalDataPath)
		return _originalDataPath;
	LString *path;
	if (!_atlas) {
		path = AtlasDatabase::instance()->pathForMediaDataFolder(_hostUID, _temporary);
		if (path)
			path->appendPathComponent(_imageUID);
	} else {
		path = AtlasDatabase::instance()->pathForMediaDataFolder(_hostUID, 1);
		if (path) {
			path->appendPathComponent(_imageUID);
			path->appendPathExtension(LString::stringWithCString(".png"));
			if (!LFileManager::manager()->objectExistsAtPath(path, 0) && originalData())
				_originalData->writeToFile(path);
		}
	}
	_originalDataPath = LRetain(path);
	return _originalDataPath;
}

LData *EntryInfoImageWrapper::originalData()
{
	if (_originalData == 0) {
		if (!_atlas) {
			_originalData = LData::_alloc()->initWithContentsOfFile(originalDataPath());
		} else {
			BLOBWrapper *blob = BLOBWrapper::wrapperWithBLOBUid(_imageUID);
			_originalData = LRetain(blob ? blob->blobData() : 0);
		}
	}
	return _originalData;
}

LBitmapImage *EntryInfoImageWrapper::originalImage()
{
	if (_originalImage == 0) {
		LData *data = originalData();
		if (data)
			_originalImage = LBitmapImage::_alloc()->initWithData(data);
	}
	return _originalImage;
}

// Width over height, from the image when not stored.
float EntryInfoImageWrapper::aspectRatio()
{
	if (_aspectRatio <= 0.0f) {
		LBitmapImage *image = originalImage();
		_aspectRatio = (image && image->height()) ? (float)image->width() / (float)image->height() : 1.0f;
	}
	return _aspectRatio;
}

LMap *EntryInfoImageWrapper::mapRepresentation()
{
	return LMap::mapWithObjectsAndKeys(_imageUID, kImageUIDKey, LValue::valueWithFloat(aspectRatio()),
	                                   kAspectRatioKey, _description, kDescriptionKey, 0);
}

LString *EntryInfoImageWrapper::thumbnailPath()
{
	if (_thumbnailPath)
		return _thumbnailPath;
	LString *path = AtlasDatabase::instance()->pathForMediaThumbnail(_imageUID, _hostUID, _temporary, _atlas);
	if (path) {
		if (!LFileManager::manager()->objectExistsAtPath(path, 0) && originalImage()) {
			LBitmapImage *thumbnail = _originalImage->scaledCloneWithSize(kThumbnailSize, 3);
			if (thumbnail) {
				if (LData *png = thumbnail->representationOfType((TBitmapRepresentationType)0))
					png->writeToFile(path);
				thumbnail->release();
			}
		}
		_thumbnailPath = LRetain(path);
	}
	return _thumbnailPath;
}

// A DXT5 texture of the image with a transparent one-pixel border.
LString *EntryInfoImageWrapper::texturePath()
{
	if (_texturePath)
		return _texturePath;
	LString *path = AtlasDatabase::instance()->pathForMediaTexture(_imageUID, _hostUID, _temporary, _atlas);
	if (path) {
		if (!LFileManager::manager()->objectExistsAtPath(path, 0) && originalImage()) {
			LBitmapImage *texture = _originalImage->scaledCloneWithSize(kTextureSize, 2);
			if (texture) {
				LBitmapImage::StLocker lock(texture, LBitmapImage::StLocker::kRead, kPixelFormatBGRA);
				unsigned int bytesPerRow = lock.bytesPerRow();
				int width = (int)kTextureSize.width;
				int height = (int)kTextureSize.height;
				unsigned char *bits = (unsigned char *)lock.bitmapData();
				memset(bits, 0, bytesPerRow);
				memset(bits + bytesPerRow * (height - 1), 0, bytesPerRow);
				for (int y = 1; y < height - 1; y++) {
					uint32_t *row = (uint32_t *)(bits + bytesPerRow * y);
					row[0] = 0;
					row[width - 1] = 0;
				}
				if (LData *dds = Utils::ddsDataWithFormatFromBitmap(D3DFMT_DXT5, bits, kTextureSize, bytesPerRow))
					dds->writeToFile(path);
				texture->release();
			}
		}
		_texturePath = LRetain(path);
	}
	return _texturePath;
}

// Moves a user image and its caches into or out of the temporary folder.
bool EntryInfoImageWrapper::commit(unsigned char temporary)
{
	if (_atlas || _temporary == (bool)temporary)
		return true;
	AtlasDatabase *db = AtlasDatabase::instance();
	LFileManager *fm = LFileManager::manager();
	LString *path = db->pathForMediaDataFolder(_hostUID, temporary);
	path->appendPathComponent(_imageUID);
	if (!fm->movePathToPath(originalDataPath(), path))
		return false;
	if (_originalDataPath)
		_originalDataPath->release();
	_originalDataPath = LRetain(path);
	if (_texturePath) {
		LString *texture = db->pathForMediaTexture(_imageUID, _hostUID, temporary, _atlas);
		fm->movePathToPath(_texturePath, texture);
		_texturePath->release();
		_texturePath = LRetain(texture);
	}
	if (_thumbnailPath) {
		LString *thumbnail = db->pathForMediaThumbnail(_imageUID, _hostUID, temporary, _atlas);
		fm->movePathToPath(_thumbnailPath, thumbnail);
		_thumbnailPath->release();
		_thumbnailPath = LRetain(thumbnail);
	}
	_temporary = temporary;
	return true;
}

// A new user image, written to the temporary folder under a free name.
EntryInfoImageWrapper *EntryInfoImageWrapper::initWithImageData(LData *data, LString *hostUID, LString *name)
{
	if (data && hostUID && name && LObject::init()) {
		_originalData = LRetain(data);
		_hostUID = LRetain(hostUID);
		_temporary = true;
		_imageUID = LRetain(_imageUIDFromImageName(name));
		if (originalDataPath() && _originalData->writeToFile(_originalDataPath))
			return this;
	}
	dealloc();
	return 0;
}

EntryInfoImageWrapper *EntryInfoImageWrapper::wrapperWithImageData(LData *data, LString *hostUID, LString *name)
{
	if (data == 0)
		return 0;
	EntryInfoImageWrapper *wrapper = _alloc()->initWithImageData(data, hostUID, name);
	if (wrapper) {
		_cacheWrapper(wrapper, imageKey(wrapper->_hostUID, wrapper->_imageUID));
		wrapper->l_autorelease();
	}
	return wrapper;
}
