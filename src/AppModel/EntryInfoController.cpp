// EntryInfoController: the info panel's model. The edits go through
// selectors taking the entry's UID (not the entry), so the undo manager can
// replay them after the panel moved to another entry.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(EntryInfoController, 0)
LF_SELECTOR(EntryInfoController, _insertItemForKeyAtIndex);
LF_SELECTOR(EntryInfoController, _addItemForKeyAtIndex);
LF_SELECTOR(EntryInfoController, _replaceItemForKeyAtIndex);
LF_SELECTOR(EntryInfoController, _moveItemForKeyFromIndexToIndex);
LF_SELECTOR(EntryInfoController, _removeItemForKeyAtIndex);
LF_SELECTOR(EntryInfoController, _commitScheduledEntry);

static void _setUndoActionName(const char *name)
{
	EntryInfoController::undoManager()->setActionName(LString::stringFromTable(name, "Undo", getAppModelModule()));
}

// The info of the entry with the UID, or 0.
static EntryInfo *_entryInfoForUID(LString *uid)
{
	GenericEntry *entry = uid ? GenericEntry::entryWithUID(uid) : 0;
	return entry ? entry->entryInfoObject() : 0;
}

static inline int _intValue(LValue *value)
{
	return value ? value->intValue() : 0;
}

EntryInfoController *EntryInfoController::init()
{
	if (LObject::init() == 0)
		return 0;
	return this;
}

void EntryInfoController::dealloc()
{
	if (_entry) {
		_entry->release();
		_entry = 0;
	}
	LObject::dealloc();
}

LUndoManager *EntryInfoController::undoManager()
{
	return LUndoManager::instance();
}

void EntryInfoController::setEntry(GenericEntry *entry)
{
	if (_entry == entry)
		return;
	if (_entry) {
		_entry->release();
		_entry = 0;
	}
	_entry = LRetain(entry);
}

LString *EntryInfoController::parameterTitleFromKey(LString *key)
{
	return LString::stringFromTable(key, "EntryInfoTitle", getAppModelModule());
}

// ---------------------------------------------------------------------------
// Display strings

LString *EntryInfoController::formattedCoordinates(LObject *object)
{
	Vector2 point = { 0, 0 };
	UMapParsingUtilsFH::loadVectorFromObject(&point, object);
	return Utils::shortGeoCoordinatesDescription(LMakePoint(point.x, point.y), Utils::kGeolocFormatFull);
}

bool EntryInfoController::extractURLParamsFromObject(LObject *object, LString **title, LString **url)
{
	DBTreeElement *tree = lobj_cast<DBTreeElement>(object);
	LMap *map = tree ? 0 : lobj_cast<LMap>(object);
	if (!tree && !map) {
		if (title)
			*title = 0;
		if (url)
			*url = 0;
		return false;
	}
	if (title) {
		LObject *value = tree ? tree->firstValueForKey(kEntryInfoItemTitleKey) : map->objectForKey(kEntryInfoItemTitleKey);
		*title = lobj_cast<LString>(value);
	}
	if (url) {
		LObject *value = tree ? tree->firstValueForKey(kEntryInfoItemURLKey) : map->objectForKey(kEntryInfoItemURLKey);
		*url = lobj_cast<LString>(value);
		// Untitled links show their address.
		if (title && !*title)
			*title = *url;
	}
	return true;
}

bool EntryInfoController::extractDescriptionParamsFromObject(LObject *object, LString **title, LString **text)
{
	if (LString *string = lobj_cast<LString>(object)) {
		if (title)
			*title = 0;
		if (text)
			*text = string;
		return true;
	}
	DBTreeElement *tree = lobj_cast<DBTreeElement>(object);
	if (!tree) {
		if (title)
			*title = 0;
		if (text)
			*text = 0;
		return false;
	}
	if (title)
		*title = lobj_cast<LString>(tree->firstValueForKey(kEntryInfoItemTitleKey));
	if (text)
		*text = lobj_cast<LString>(tree->firstValueForKey(kEntryInfoItemStringKey));
	return true;
}

LString *EntryInfoController::formattedHostLocationName(LObject *object)
{
	LString *uid = lobj_cast<LString>(object);
	GenericEntry *entry = uid ? GenericEntry::entryWithUID(uid) : 0;
	return entry ? entry->ownName(0) : 0;
}

LString *EntryInfoController::formattedCurrentDateWithTimeZone(LObject *object)
{
	LDate *date = LDate::date();
	date->setTimeZone(lobj_cast<LTimeZone>(object));
	return date->descriptionWithCalendarFormat(Preferences::uses24HourClock() ? &k24HRsDateFormat : &k12HRsDateFormat);
}

LString *EntryInfoController::formattedAltNames(LObject *object)
{
	LArray *names = lobj_cast<LArray>(object);
	LString *result = LString::string();
	int count = names ? names->count() : 0;
	for (int i = 0; i < count; i++) {
		LString *name = lobj_cast<LString>(names->objectAtIndex(i));
		if (!name)
			continue;
		// Names all in capitals are kept (abbreviations).
		if (!name->isEqualTo(name->uppercaseString()))
			name = name->capitalizedString();
		result->appendString(name);
		if (i < count - 1)
			result->appendString(&kAltNamesSeparatorStr);
	}
	return result;
}

LString *EntryInfoController::formattedGMTOffsetFromTimeZone(LObject *object)
{
	LTimeZone *zone = lobj_cast<LTimeZone>(object);
	if (!zone)
		zone = LTimeZone::defaultTimeZone();
	int minutes = zone->secondsFromGMT() / 60;
	if (minutes == 0)
		return LString::stringWithCString("0:00");
	return LString::stringWithFormat(minutes > 0 ? "+%02d:%02d" : "%02d:%02d", minutes / 60, abs(minutes) % 60);
}

static const struct {
	LString **key;
	EntryInfoController::TEntryPropertyKind kind;
} sKeyKinds[] = {
	{ &kEntryAltNamesPropertyKey, EntryInfoController::kPropertyKindAltNames },
	{ &kEntryPopulationPropertyKey, EntryInfoController::kPropertyKindPopulation },
	{ &kEntryDateTimePropertyKey, EntryInfoController::kPropertyKindDateTime },
	{ &kEntryCoordinatesPropertyKey, EntryInfoController::kPropertyKindCoordinates },
	{ &kEntryGMTOffsetPropertyKey, EntryInfoController::kPropertyKindGMTOffset },
	{ &kEntryInfoURLKey, EntryInfoController::kPropertyKindURL },
	{ &kEntryInfoWebCamKey, EntryInfoController::kPropertyKindURL },
	{ &kEntryInfoItemURLKey, EntryInfoController::kPropertyKindURL },
	{ &kEntryHostCityDescrPropertyKey, EntryInfoController::kPropertyKindHostLocation },
	{ &kEntryInfoDescriptionKey, EntryInfoController::kPropertyKindDescription },
	{ &kEntryInfoDensityKey, EntryInfoController::kPropertyKindDensity },
	{ &kEntryRouteTotalDistancePropertyKey, EntryInfoController::kPropertyKindDistance },
	{ &kEntryInfoCoastlineKey, EntryInfoController::kPropertyKindDistance },
	{ &kEntryInfoHeightKey, EntryInfoController::kPropertyKindSmallDistance },
	{ &kEntryInfoElevationKey, EntryInfoController::kPropertyKindSmallDistance },
	{ &kEntryInfoAverageDepthKey, EntryInfoController::kPropertyKindSmallDistance },
	{ &kEntryInfoGreatestDepthKey, EntryInfoController::kPropertyKindSmallDistance },
	{ &kEntryInfoTotalAreaKey, EntryInfoController::kPropertyKindArea },
	{ &kEntryInfoAreaKey, EntryInfoController::kPropertyKindArea },
	{ &kEntryInfoSurfaceTemperaturesKey, EntryInfoController::kPropertyKindTemperature },
};

EntryInfoController::TEntryPropertyKind EntryInfoController::rowKindFromKey(LString *key)
{
	if (!key)
		return kPropertyKindDefault;
	for (unsigned int i = 0; i < sizeof sKeyKinds / sizeof sKeyKinds[0]; i++)
		if (key->isEqualTo(*sKeyKinds[i].key))
			return sKeyKinds[i].kind;
	return kPropertyKindDefault;
}

LString *EntryInfoController::stringFromInfoObjectForKey(LObject *object, TEntryPropertyKind kind)
{
	// A parametrized text: "...%@...%@..." with its parameters.
	if (kind != kPropertyKindURL && kind != kPropertyKindHostLocation) {
		if (DBTreeElement *tree = lobj_cast<DBTreeElement>(object)) {
			LString *format = lobj_cast<LString>(tree->firstValueForKey(kEntryInfoItemParamStringKey));
			LArray *params = lobj_cast<LArray>(tree->valuesForKey(kEntryInfoItemParamsKey));
			if (params) {
				LString *result = LString::string();
				LArray *parts = format ? format->componentsSeparatedByString(&kLDescrSeparatorStr) : LArray::array();
				int count = params->count();
				int i;
				for (i = 0; i < count; i++) {
					LString *param = stringFromInfoObjectForKey(params->objectAtIndex(i), kind);
					if (i < (int)parts->count())
						result->appendString((LString *)parts->objectAtIndex(i));
					if (param)
						result->appendString(param);
				}
				for (; i < (int)parts->count(); i++)
					result->appendString((LString *)parts->objectAtIndex(i));
				return result;
			}
		}
	}
	switch (kind) {
	case kPropertyKindAltNames:
		return formattedAltNames(object);
	case kPropertyKindPopulation:
		return Utils::commaSeparatedNumericValue(object, 0, 0);
	case kPropertyKindDateTime:
		return formattedCurrentDateWithTimeZone(object);
	case kPropertyKindGMTOffset:
		return formattedGMTOffsetFromTimeZone(object);
	case kPropertyKindCoordinates:
		return formattedCoordinates(object);
	case kPropertyKindHostLocation:
		return formattedHostLocationName(object);
	case kPropertyKindDensity:
		return Utils::densityDescription(object);
	case kPropertyKindDistance:
		return Utils::distanceDescription(object);
	case kPropertyKindSmallDistance:
		return Utils::smallDistanceDescription(object);
	case kPropertyKindArea:
		return Utils::areaDescription(object);
	case kPropertyKindTemperature:
		return Utils::temperatureDescription(object);
	default:
		return object ? object->description() : 0;
	}
}

// ---------------------------------------------------------------------------
// Items

// Keys with an order in `order` (a map of key to LComparable) first, in that
// order; then images and descriptions; then the rest by name.
static int _compareKeys(const void *a, const void *b, const void *context)
{
	LComparable *keyA = *(LComparable *const *)a;
	LComparable *keyB = *(LComparable *const *)b;
	LMap *order = (LMap *)context;
	LComparable *orderA = keyA && order ? (LComparable *)order->objectForKey(keyA) : 0;
	LComparable *orderB = keyB && order ? (LComparable *)order->objectForKey(keyB) : 0;
	if (orderA && orderB)
		return orderA->compareWith(orderB);
	if (keyA && keyB) {
		if (keyB->isEqualTo(kEntryInfoImagesKey) || keyB->isEqualTo(kEntryInfoDescriptionKey))
			return -1;
		if (keyA->isEqualTo(kEntryInfoImagesKey) || keyA->isEqualTo(kEntryInfoDescriptionKey))
			return 1;
	}
	if (keyA && !orderA && !orderB && keyB)
		return keyA->compareWith(keyB);
	if (orderA)
		return -1;
	if (orderB)
		return 1;
	return 0;
}

LArray *EntryInfoController::orderedKeys(TEntryKeySet set, TEntryKeyAccessMode mode)
{
	EntryInfo *info = _entry ? _entry->entryInfoObject() : 0;
	LSet *keys = info ? info->allKeys(set, mode) : 0;
	if (!keys)
		return 0;
	// Items that can be added, though there are none yet.
	if (mode == kEntryKeyWrite) {
		keys->addObject(kEntryInfoImagesKey);
		keys->addObject(kEntryInfoURLKey);
		keys->addObject(kEntryInfoDescriptionKey);
	}
	LArray *sorted = keys->allObjects();
	if (!sorted)
		return 0;
	sorted->setComparator(_compareKeys, 0, AtlasDatabase::instance()->entryInfoKeysOrder(_entry));
	sorted->sort();
	LArray *result = LArray::array();
	result->_addDataFromArray(sorted);
	return result;
}

int EntryInfoController::itemsCountForKey(LString *key, TEntryKeySet set, TEntryKeyAccessMode mode)
{
	EntryInfo *info = _entry ? _entry->entryInfoObject() : 0;
	int count = info ? info->itemsCountForKey(key, set, mode) : 0;
	// An empty row to add to.
	if (kEntryInfoImagesKey->isEqualTo(key) && mode == kEntryKeyWrite)
		count++;
	else if (set == kEntryKeySetWithWeather || (count < 1 && mode == kEntryKeyWrite))
		count = 1;
	return count;
}

LObject *EntryInfoController::itemForKeyAtIndex(LString *key, int index, TEntryKeySet set, TEntryKeyAccessMode mode)
{
	EntryInfo *info = _entry ? _entry->entryInfoObject() : 0;
	return info ? info->itemForKeyAtIndex(key, index, set, mode) : 0;
}

bool EntryInfoController::canRemoveItemForKeyAtIndex(LString *key, int index, TEntryKeySet set)
{
	EntryInfo *info = _entry ? _entry->entryInfoObject() : 0;
	return info && info->canRemoveItemForKeyAtIndex(key, index, set);
}

bool EntryInfoController::canAddItemsForKey(LString *key, TEntryKeySet set)
{
	EntryInfo *info = _entry ? _entry->entryInfoObject() : 0;
	return info && info->canAddItemsForKey(key, set);
}

intptr_t EntryInfoController::_commitScheduledEntry(char *)
{
	GenericEntry *entry = 0;
	if (_scheduledUID) {
		entry = GenericEntry::entryWithUID(_scheduledUID);
		_scheduledUID->release();
		_scheduledUID = 0;
	}
	_commitTimer = 0;
	return entry ? entry->commit() : false;
}

// Undoing and redoing change entries the panel may not show: they are
// committed on the next run loop pass.
void EntryInfoController::commitIfNeeded(LString *uid)
{
	LString *shownUID = _entry ? _entry->uid(0) : 0;
	if (!uid || (_editing && shownUID == uid))
		return;
	LUndoManager *undo = undoManager();
	if (!undo->isUndoing() && !undo->isRedoing())
		return;
	if (_commitTimer)
		_commitTimer->invalidate();
	if (_scheduledUID && _scheduledUID != uid)
		invokeSelector("_commitScheduledEntry");
	if (_scheduledUID != uid) {
		// The original retained the UID again when it was already scheduled.
		_scheduledUID = LRetain(uid);
	}
	_commitTimer = LTimer::timerWithParams(0, this, "_commitScheduledEntry", false, 0);
}

// args: UID, key, [index, set].
intptr_t EntryInfoController::_removeItemForKeyAtIndex(char *args)
{
	LObject *uidObject = LF_ARG(args, 0, LObject *);
	LString *key = LF_ARG(args, 1, LString *);
	LArray *params = LF_ARG(args, 2, LArray *);
	if (!params || params->count() != 2)
		return false;
	LValue *indexValue = lobj_cast<LValue>(params->objectAtIndex(0));
	LValue *setValue = lobj_cast<LValue>(params->objectAtIndex(1));
	LString *uid = lobj_cast<LString>(uidObject);
	EntryInfo *info = _entryInfoForUID(uid);
	if (!info)
		return false;
	TEntryKeySet set = (TEntryKeySet)_intValue(setValue);
	int index = _intValue(indexValue);
	LObject *item = LRetain(info->itemForKeyAtIndex(key, index, set, kEntryKeyWrite));
	bool removed = info->removeItemForKeyAtIndex(key, index, set);
	if (removed) {
		undoManager()->registerUndo(this, "_insertItemForKeyAtIndex", uidObject, item,
		                            LArray::arrayWithObjects(key, indexValue, setValue, (LObject *)0));
		_setUndoActionName("Delete Item");
		commitIfNeeded(uid);
	}
	if (item)
		item->release();
	return removed;
}

// args: UID, [key, from, to, set].
intptr_t EntryInfoController::_moveItemForKeyFromIndexToIndex(char *args)
{
	LObject *uidObject = LF_ARG(args, 0, LObject *);
	LArray *params = LF_ARG(args, 1, LArray *);
	if (!params || params->count() != 4)
		return false;
	LString *key = lobj_cast<LString>(params->objectAtIndex(0));
	LValue *fromValue = lobj_cast<LValue>(params->objectAtIndex(1));
	LValue *toValue = lobj_cast<LValue>(params->objectAtIndex(2));
	LValue *setValue = lobj_cast<LValue>(params->objectAtIndex(3));
	LString *uid = lobj_cast<LString>(uidObject);
	EntryInfo *info = _entryInfoForUID(uid);
	if (!info)
		return false;
	TEntryKeySet set = (TEntryKeySet)_intValue(setValue);
	int from = _intValue(fromValue);
	int to = _intValue(toValue);
	if (from == to)
		return false;
	LObject *item = LRetain(info->itemForKeyAtIndex(key, from, set, kEntryKeyWrite));
	// `to` is the position before the item is removed.
	if (from < to)
		to--;
	bool moved = false;
	if (!info->removeItemForKeyAtIndex(key, from, set)) {
		info->insertItemForKeyAtIndex(item, key, from, set);
	} else {
		moved = info->insertItemForKeyAtIndex(item, key, to, set);
		if (moved) {
			if (to <= from)
				fromValue = LValue::valueWithInt(from + 1);
			LArray *undoParams = LArray::arrayWithObjects(key, LValue::valueWithInt(to), fromValue, setValue, (LObject *)0);
			undoManager()->registerUndo(this, "_moveItemForKeyFromIndexToIndex", uidObject, undoParams);
			_setUndoActionName("Move Item");
			commitIfNeeded(uid);
		}
	}
	if (item)
		item->release();
	return moved;
}

// args: UID, value, [key, index, set]. Past the last item, adds it.
intptr_t EntryInfoController::_replaceItemForKeyAtIndex(char *args)
{
	LObject *uidObject = LF_ARG(args, 0, LObject *);
	LObject *value = LF_ARG(args, 1, LObject *);
	LArray *params = LF_ARG(args, 2, LArray *);
	if (!params || params->count() != 3)
		return false;
	LString *key = lobj_cast<LString>(params->objectAtIndex(0));
	LValue *indexValue = lobj_cast<LValue>(params->objectAtIndex(1));
	LValue *setValue = lobj_cast<LValue>(params->objectAtIndex(2));
	LString *uid = lobj_cast<LString>(uidObject);
	EntryInfo *info = _entryInfoForUID(uid);
	if (!info)
		return false;
	TEntryKeySet set = (TEntryKeySet)_intValue(setValue);
	int index = _intValue(indexValue);
	if (index < 0 || index >= info->itemsCountForKey(key, set, kEntryKeyWrite))
		return invokeSelector("_addItemForKeyAtIndex", uidObject, value,
		                      LArray::arrayWithObjects(key, setValue, (LObject *)0));
	LObject *item = LRetain(info->itemForKeyAtIndex(key, index, set, kEntryKeyWrite));
	LComparable *newValue = lobj_cast<LComparable>(value);
	LComparable *oldValue = lobj_cast<LComparable>(item);
	bool replaced = false;
	if (value != item && !(newValue && oldValue && newValue->isEqualTo(oldValue))) {
		replaced = info->replaceItemForKeyAtIndex(value, key, index, set);
		if (replaced) {
			undoManager()->registerUndo(this, "_replaceItemForKeyAtIndex", uidObject, item,
			                            LArray::arrayWithObjects(key, indexValue, setValue, (LObject *)0));
			_setUndoActionName("Change Item");
			commitIfNeeded(uid);
		}
	}
	if (item)
		item->release();
	return replaced;
}

// args: UID, value, [key, set].
intptr_t EntryInfoController::_addItemForKeyAtIndex(char *args)
{
	LObject *uidObject = LF_ARG(args, 0, LObject *);
	LObject *value = LF_ARG(args, 1, LObject *);
	LArray *params = LF_ARG(args, 2, LArray *);
	if (!params || params->count() != 2)
		return false;
	LString *key = lobj_cast<LString>(params->objectAtIndex(0));
	LValue *setValue = lobj_cast<LValue>(params->objectAtIndex(1));
	LString *uid = lobj_cast<LString>(uidObject);
	EntryInfo *info = _entryInfoForUID(uid);
	if (!info)
		return false;
	TEntryKeySet set = (TEntryKeySet)_intValue(setValue);
	bool added = info->addItemForKey(value, key, set);
	int count = info->itemsCountForKey(key, set, kEntryKeyWrite);
	if (added) {
		LArray *undoParams = LArray::arrayWithObjects(LValue::valueWithInt(count - 1), setValue, (LObject *)0);
		undoManager()->registerUndo(this, "_removeItemForKeyAtIndex", uidObject, key, undoParams);
		_setUndoActionName("Insert Item");
		commitIfNeeded(uid);
	}
	return added;
}

// args: UID, value, [key, index, set].
intptr_t EntryInfoController::_insertItemForKeyAtIndex(char *args)
{
	LObject *uidObject = LF_ARG(args, 0, LObject *);
	LObject *value = LF_ARG(args, 1, LObject *);
	LArray *params = LF_ARG(args, 2, LArray *);
	if (!params || params->count() != 3)
		return false;
	LString *key = lobj_cast<LString>(params->objectAtIndex(0));
	LValue *indexValue = lobj_cast<LValue>(params->objectAtIndex(1));
	LValue *setValue = lobj_cast<LValue>(params->objectAtIndex(2));
	LString *uid = lobj_cast<LString>(uidObject);
	EntryInfo *info = _entryInfoForUID(uid);
	if (!info)
		return false;
	bool inserted =
	    info->insertItemForKeyAtIndex(value, key, _intValue(indexValue), (TEntryKeySet)_intValue(setValue));
	if (inserted) {
		undoManager()->registerUndo(this, "_removeItemForKeyAtIndex", uidObject, key,
		                            LArray::arrayWithObjects(indexValue, setValue, (LObject *)0));
		_setUndoActionName("Insert Item");
		commitIfNeeded(uid);
	}
	return inserted;
}

bool EntryInfoController::removeItemForKeyAtIndex(LString *key, int index, TEntryKeySet set)
{
	LArray *params = LArray::arrayWithObjects(LValue::valueWithInt(index), LValue::valueWithInt(set), (LObject *)0);
	return invokeSelector("_removeItemForKeyAtIndex", _entry->uid(0), key, params);
}

bool EntryInfoController::moveItemForKeyFromIndexToIndex(LString *key, int from, int to, TEntryKeySet set)
{
	LArray *params = LArray::arrayWithObjects(key, LValue::valueWithInt(from), LValue::valueWithInt(to),
	                                          LValue::valueWithInt(set), (LObject *)0);
	return invokeSelector("_moveItemForKeyFromIndexToIndex", _entry->uid(0), params);
}

bool EntryInfoController::replaceItemForKeyAtIndex(LObject *value, LString *key, int index, TEntryKeySet set)
{
	LArray *params = LArray::arrayWithObjects(key, LValue::valueWithInt(index), LValue::valueWithInt(set), (LObject *)0);
	return invokeSelector("_replaceItemForKeyAtIndex", _entry->uid(0), value, params);
}

bool EntryInfoController::addItemForKey(LObject *value, LString *key, TEntryKeySet set)
{
	LArray *params = LArray::arrayWithObjects(key, LValue::valueWithInt(set), (LObject *)0);
	return invokeSelector("_addItemForKeyAtIndex", _entry->uid(0), value, params);
}

bool EntryInfoController::insertItemForKeyAtIndex(LObject *value, LString *key, int index, TEntryKeySet set)
{
	LArray *params = LArray::arrayWithObjects(key, LValue::valueWithInt(index), LValue::valueWithInt(set), (LObject *)0);
	return invokeSelector("_insertItemForKeyAtIndex", _entry->uid(0), value, params);
}

EntryInfoImageWrapper *EntryInfoController::insertImageWrapperWithData(LData *data, LString *name, int index)
{
	if (!data)
		return 0;
	EntryInfoImageWrapper *wrapper = EntryInfoImageWrapper::wrapperWithImageData(data, _entry->uid(0), name);
	if (wrapper && insertItemForKeyAtIndex(wrapper, kEntryInfoImagesKey, index, kEntryKeySetShort))
		_setUndoActionName("Insert Image");
	return wrapper;
}
