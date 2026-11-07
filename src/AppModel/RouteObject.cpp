// RouteObject: a sequence of entries and breaks, with a color and a caption
// per entry, and the points, lines and shapes the globe highlights for it.
// MutableRouteObject edits it and posts the change notifications.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(RouteObject, 0)
LF_SELECTOR(RouteObject, onEntryDidChange);
LF_IMPLEMENT_CLASS(MutableRouteObject, LF_CLASSOF(RouteObject))

// ---------------------------------------------------------------------------
// RouteObject

RouteObject::RouteObject()
    : _valid(false), _totalDistance(0), _points(0), _lineIndices(0), _shapes(0), _pointEntries(0),
      _bounds(LZeroRect), _locationsCount(0), _name(0), _items(0), _colors(0), _names(0)
{
}

void RouteObject::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	LObject **fields[] = { (LObject **)&_items,       (LObject **)&_points, (LObject **)&_pointEntries,
		               (LObject **)&_lineIndices, (LObject **)&_shapes, (LObject **)&_colors,
		               (LObject **)&_names,       (LObject **)&_name };
	for (LObject **field : fields)
		if (*field) {
			(*field)->release();
			*field = 0;
		}
	LObject::dealloc();
}

// Drops the given name and the cached points.
void RouteObject::_invalidate()
{
	if (_name) {
		_name->release();
		_name = 0;
	}
	_valid = false;
}

LArray *RouteObject::colorComponentsForItem(GenericEntry *entry) const
{
	return entry ? (LArray *)_colors->objectForKey(entry->uid(0)) : 0;
}

LString *RouteObject::nameForItem(GenericEntry *entry) const
{
	if (entry == 0)
		return 0;
	LString *name = (LString *)_names->objectForKey(entry->uid(0));
	return name ? name : entry->shortCaption(0);
}

intptr_t RouteObject::onEntryDidChange(char *args)
{
	LNotification *ntf = LF_ARG(args, 0, LNotification *);
	RouteSetItem *item = ntf ? lobj_cast<RouteSetItem>(ntf->object()) : 0;
	if (_items && _items->indexOfObject(item) != -1) {
		_invalidate();
		LNotificationCenter::center()->postNotification(kRouteObjectAttributesDidChangeNotification, this, 0);
	}
	return 0;
}

int RouteObject::itemsCount() const
{
	return _items ? _items->count() : 0;
}

float RouteObject::distanceForSubset(int count) const
{
	float distance = 0;
	PointEntry *next = 0;
	while (--count >= 0) {
		RouteSetItem *item = (RouteSetItem *)_items->objectAtIndex(count);
		PointEntry *previous = next;
		if (item->genericEntryInstance())
			if (PointEntry *point = lobj_cast<PointEntry>(item->genericEntryInstance()))
				next = point;
		if (item->routeBreakInstance())
			break;
		if (next && previous)
			distance += previous->distanceToEntry(next);
	}
	return distance;
}

float RouteObject::distanceForItemAtIndex(int index) const
{
	if (index > 0) {
		PointEntry *point =
		    lobj_cast<PointEntry>(((RouteSetItem *)_items->objectAtIndex(index))->genericEntryInstance());
		PointEntry *previous =
		    lobj_cast<PointEntry>(((RouteSetItem *)_items->objectAtIndex(index - 1))->genericEntryInstance());
		if (point && previous)
			return previous->distanceToEntry(point);
	}
	return -1.0f;
}

RouteSetItem *RouteObject::itemAtIndex(int index) const
{
	return (index >= 0 && index < (int)_items->count()) ? (RouteSetItem *)_items->objectAtIndex(index) : 0;
}

float RouteObject::visualWeightForPointAtIndex(unsigned int index)
{
	GenericEntry *entry = _pointEntries ? (GenericEntry *)_pointEntries->objectAtIndex(index) : 0;
	return entry ? entry->visualWeight() : 0;
}

LString *RouteObject::titleForPointAtIndex(unsigned int index)
{
	GenericEntry *entry = _pointEntries ? (GenericEntry *)_pointEntries->objectAtIndex(index) : 0;
	return entry ? nameForItem(entry) : 0;
}

bool RouteObject::containsItem(RouteSetItem *item)
{
	return _items && _items->indexOfObject(item) != -1;
}

// A random saturated color.
Color generateRandomColor()
{
	float a = rand() * 4.656613e-10f;
	float b = rand() * 4.656613e-10f;
	float c = rand() * 4.656613e-10f;
	_SVector3 v = { c, b, a };
	FHVec3Normalize(&v, &v);
	return Color(v.x * 0.995f, v.y * 0.995f, v.z * 0.995f, 1.0f);
}

LObject *RouteObject::initWithDecoder(const LCoder *coder)
{
	StReadChunk chunk(coder);
	return initWithDictionaryRep(lobj_cast<LMap>(coder->decodeObject()), 0);
}

void RouteObject::encodeWithCoder(LCoder *coder) const
{
	StWriteChunk chunk(coder);
	coder->encodeObject(dictionaryRepresentation());
}

// The given name, or "first to last" from the captions of the first and the
// last entries that have one.
LString *RouteObject::preferredName()
{
	if (_name)
		return _name;
	int last = (_items ? _items->count() : 0) - 1;
	int first = 0;
	LString *firstName = 0;
	LString *lastName = 0;
	do {
		if (last < first)
			break;
		if (firstName == 0) {
			GenericEntry *entry = itemAtIndex(first)->genericEntryInstance();
			if (entry == 0 || (firstName = nameForItem(entry)) == 0)
				first++;
		}
		if (lastName == 0) {
			if (last <= first)
				break;
			GenericEntry *entry = itemAtIndex(last)->genericEntryInstance();
			if (entry == 0 || (lastName = nameForItem(entry)) == 0)
				last--;
		}
	} while (lastName == 0 || firstName == 0);
	if (firstName && lastName) {
		LString *format = LString::stringFromTable("Route Format", "AppModel", getAppModelModule());
		if (format == 0)
			format = LString::stringWithCString("%@ to %@");
		return LString::stringWithFormat(format, firstName, lastName);
	}
	if (firstName == 0 && lastName == 0)
		return LString::stringFromTable("Untitled Route", "AppModel", getAppModelModule());
	return firstName ? firstName : lastName;
}

static LArray *_colorComponents(const Color &color)
{
	return LArray::arrayWithObjects(LValue::valueWithFloat(color.r), LValue::valueWithFloat(color.g),
	                                LValue::valueWithFloat(color.b), 0);
}

// The entry's color, a new random one when it has none.
Color RouteObject::colorForItem(GenericEntry *entry) const
{
	Color color(0, 0, 0, 0);
	if (entry) {
		LString *uid = entry->uid(0);
		LObject *components = _colors->objectForKey(uid);
		if (components == 0) {
			components = _colorComponents(generateRandomColor());
			_colors->setObjectForKey(components, uid);
		}
		UMapParsingUtilsFH::loadColorFromObject(&color, components);
	}
	return color;
}

static int _compareBreakIndicies(const void *a, const void *b, const void *)
{
	LValue *va = *(LValue *const *)a;
	LValue *vb = *(LValue *const *)b;
	int ia = va ? va->intValue() : 0;
	int ib = vb ? vb->intValue() : 0;
	if (ia == ib)
		return 0;
	return ia < ib ? -1 : 1;
}

// The entries' dictionary representations, with their colors and captions,
// and the indexes of the breaks among them.
RouteObject *RouteObject::initWithDictionaryRep(LMap *rep, LString *name)
{
	if (!LObject::init())
		return 0;
	_name = LRetain(name);
	_items = LArray::_alloc()->init();
	_colors = LMap::_alloc()->init();
	_names = LMap::_alloc()->init();
	if (rep) {
		LArray *entries = lobj_cast<LArray>(rep->objectForKey(kFlatEntryArrayKey));
		int count = entries ? entries->count() : 0;
		for (int i = 0; i < count; i++) {
			LMap *entryRep = lobj_cast<LMap>(entries->objectAtIndex(i));
			GenericEntry *entry = entryRep ? GenericEntry::entryWithDictionaryRep(entryRep) : 0;
			if (entry == 0)
				continue;
			_items->addObject(entry);
			if (LString *caption = lobj_cast<LString>(entryRep->objectForKey(kEntryCaptionKey)))
				_names->setObjectForKey(caption, entry->uid(0));
			LArray *components = lobj_cast<LArray>(entryRep->objectForKey(kEntryColorKey));
			if (components && components->count() > 2) {
				Color color;
				UMapParsingUtilsFH::loadColorFromObject(&color, components);
				_colors->setObjectForKey(_colorComponents(color), entry->uid(0));
			}
		}
		int itemCount = _items->count();
		LArray *breaks = lobj_cast<LArray>(rep->objectForKey(kBreakIndiciesArrayKey));
		int breakCount = 0;
		if (breaks) {
			breaks->setComparator(_compareBreakIndicies);
			breaks->sort();
			breakCount = breaks->count();
		}
		while (--breakCount >= 0) {
			LValue *index = lobj_cast<LValue>(breaks->objectAtIndex(breakCount));
			if (index && index->intValue() < itemCount)
				_items->insertObjectAtIndex(RouteSetBreak::routeSetBreak(), index->intValue());
		}
	}
	LNotificationCenter::center()->addObserver(this, "onEntryDidChange", GenericEntry::kEntryDidChangeNotification,
	                                           0);
	return this;
}

LMap *RouteObject::dictionaryRepresentation() const
{
	LMap *rep = LMap::map();
	LArray *entries = LArray::array();
	LArray *breaks = LArray::array();
	int count = _items->count();
	int breakCount = 0;
	for (int i = 0; i < count; i++) {
		GenericEntry *entry = ((RouteSetItem *)_items->objectAtIndex(i))->genericEntryInstance();
		if (entry == 0) {
			breaks->addObject(LValue::valueWithInt(i - breakCount++));
			continue;
		}
		LMap *entryRep = entry->entryDictionary();
		entries->addObject(entryRep);
		LString *uid = entry->uid(0);
		if (LObject *color = _colors->objectForKey(uid))
			entryRep->setObjectForKey(color, kEntryColorKey);
		LObject *caption = _names->objectForKey(uid);
		if (caption == 0)
			caption = entry->shortCaption(0);
		if (caption)
			entryRep->setObjectForKey(caption, kEntryCaptionKey);
	}
	rep->setObjectForKey(entries, kFlatEntryArrayKey);
	rep->setObjectForKey(breaks, kBreakIndiciesArrayKey);
	return rep;
}

// The distance, the bounds, the distinct points with their colors, the lines
// between consecutive points (not across breaks), and the shapes.
void RouteObject::_reevalPointsDependedVars() const
{
	_totalDistance = 0;
	_locationsCount = 0;
	LObject **fields[] = { (LObject **)&_points, (LObject **)&_lineIndices, (LObject **)&_shapes,
		               (LObject **)&_pointEntries };
	for (LObject **field : fields)
		if (*field) {
			(*field)->release();
			*field = 0;
		}
	if (_items == 0)
		return;
	int count = _items->count();
	int previousIndex = -1;
	PointEntry *previous = 0;
	float minX = 10000.0f, maxY = -10000.0f, minY = 10000.0f, maxX = -10000.0f;
	LMap *indexesByUID = LMap::map();
	for (int i = 0; i < count; i++) {
		RouteSetItem *item = (RouteSetItem *)_items->objectAtIndex(i);
		PointEntry *point = 0;
		if (item->genericEntryInstance()) {
			_locationsCount++;
			point = lobj_cast<PointEntry>(item);
			if (ShapeEntry *shape = lobj_cast<ShapeEntry>(item)) {
				if (LObject *container = (LObject *)shape->shapeContainer()) {
					if (_shapes == 0)
						_shapes = LArray::_alloc()->init();
					_shapes->addObject(container);
				}
			}
		}
		if (item->routeBreakInstance()) {
			previousIndex = -1;
			previous = 0;
			continue;
		}
		if (point && previous)
			_totalDistance += previous->distanceToEntry(point);
		if (point == 0)
			continue;
		SRoutePoint record;
		record.geoPoint = point->geoPoint(0);
		record.color = colorForItem(point);
		float x = record.geoPoint.x, y = record.geoPoint.y;
		if (_points == 0) {
			minX = maxX = x;
			minY = maxY = y;
			_points = LData::_alloc()->init();
		} else {
			float left = minX - x;
			if (left > 180.0f)
				left = minX - (x + 360.0f);
			float right = x - maxX;
			if (right > 180.0f)
				right = (x - 360.0f) - maxX;
			if (left > 0)
				minX -= left;
			if (right > 0)
				maxX += right;
			if (y < minY)
				minY = y;
			if (y > maxY)
				maxY = y;
		}
		if (_pointEntries == 0)
			_pointEntries = LArray::_alloc()->init();
		int index;
		LString *uid = point->uid(0);
		if (LValue *known = (LValue *)indexesByUID->objectForKey(uid)) {
			index = known->intValue();
		} else {
			_points->appendBytes(&record, sizeof(record));
			_pointEntries->addObject(point);
			index = _points->length() / sizeof(SRoutePoint) - 1;
			indexesByUID->setObjectForKey(LValue::valueWithInt(index), uid);
		}
		if (previousIndex >= 0) {
			if (_lineIndices == 0)
				_lineIndices = LData::_alloc()->init();
			int line[2] = { previousIndex, index };
			_lineIndices->appendBytes(line, sizeof(line));
		}
		previousIndex = index;
		previous = point;
	}
	if (count == 0)
		_bounds = LZeroRect;
	else
		_bounds = LMakeRect(minX, minY, maxX - minX, maxY - minY);
	_valid = true;
}

LArray *RouteObject::shapesToHighlight()
{
	if (!_valid)
		_reevalPointsDependedVars();
	return _shapes;
}

LData *RouteObject::linesIndices()
{
	if (!_valid)
		_reevalPointsDependedVars();
	return _lineIndices;
}

LData *RouteObject::pointsToHighlight()
{
	if (!_valid)
		_reevalPointsDependedVars();
	return _points;
}

LRect RouteObject::boundsGeoRect() const
{
	if (!_valid)
		_reevalPointsDependedVars();
	return _bounds;
}

float RouteObject::totalDistance() const
{
	if (!_valid)
		_reevalPointsDependedVars();
	return _totalDistance;
}

int RouteObject::locationsCount() const
{
	if (!_valid)
		_reevalPointsDependedVars();
	return _locationsCount;
}

RouteObject *RouteObject::copy()
{
	RouteObject *route = (RouteObject *)_alloc()->init();
	route->_name = LRetain(_name);
	route->_items = LArray::_alloc()->initWithArray(_items);
	route->_colors = LMap::_alloc()->initWithMap(_colors);
	route->_names = LMap::_alloc()->initWithMap(_names);
	return route;
}

// ---------------------------------------------------------------------------
// MutableRouteObject

MutableRouteObject *MutableRouteObject::init()
{
	return initWithDictionaryRep(0, 0);
}

MutableRouteObject *MutableRouteObject::initWithDictionaryRep(LMap *rep, LString *name)
{
	return (MutableRouteObject *)RouteObject::initWithDictionaryRep(rep, name);
}

void MutableRouteObject::_setNameForItem(LString *name, GenericEntry *entry)
{
	if (entry == 0)
		return;
	LString *uid = entry->uid(0);
	if (name == 0 || name->length() == 0)
		_names->removeObjectForKey(uid);
	else
		_names->setObjectForKey(name, uid);
	_invalidate();
}

void MutableRouteObject::setNameForItem(LString *name, GenericEntry *entry)
{
	if (entry == 0)
		return;
	_setNameForItem(name, entry);
	LNotificationCenter::center()->postNotification(kRouteObjectAttributesDidChangeNotification, this, 0);
}

void MutableRouteObject::_setColorForItem(LArray *components, GenericEntry *entry)
{
	if (entry == 0)
		return;
	LString *uid = entry->uid(0);
	if (components == 0)
		_colors->removeObjectForKey(uid);
	else
		_colors->setObjectForKey(components, uid);
	_invalidate();
}

void MutableRouteObject::setColorForItem(LArray *components, GenericEntry *entry)
{
	if (entry == 0)
		return;
	_setColorForItem(components, entry);
	LNotificationCenter::center()->postNotification(kRouteObjectAttributesDidChangeNotification, this, 0);
}

void MutableRouteObject::setColorForItem(Color color, GenericEntry *entry)
{
	setColorForItem(_colorComponents(color), entry);
}

bool MutableRouteObject::_insertItemAtIndex(RouteSetItem *item, int index, LString *name, LArray *color)
{
	if (item == 0 || index > itemsCount())
		return false;
	_items->insertObjectAtIndex(item, index);
	GenericEntry *entry = item->genericEntryInstance();
	if (color && entry)
		_setColorForItem(color, entry);
	if (name && entry)
		_setNameForItem(name, entry);
	_invalidate();
	return true;
}

void MutableRouteObject::insertItemAtIndex(RouteSetItem *item, int index, LString *name, LArray *color)
{
	if (item == 0 || index > itemsCount())
		return;
	if (_insertItemAtIndex(item, index, name, color))
		LNotificationCenter::center()->postNotification(kRouteObjectItemCountDidChangeNotification, this, 0);
}

// Removes the item, and the entry's color and caption when no other item
// has the entry.
void MutableRouteObject::_removeItemAtIndex(int index)
{
	if (index >= itemsCount())
		return;
	GenericEntry *entry = ((RouteSetItem *)_items->objectAtIndex(index))->genericEntryInstance();
	LString *uid = entry ? LRetain(entry->uid(0)) : 0;
	_items->removeObjectAtIndex(index);
	if (uid) {
		bool used = false;
		int count = _items->count();
		for (int i = 0; i < count && !used; i++) {
			GenericEntry *other = ((RouteSetItem *)_items->objectAtIndex(i))->genericEntryInstance();
			used = other && uid->isEqualTo(other->uid(0));
		}
		if (!used) {
			_colors->removeObjectForKey(uid);
			_names->removeObjectForKey(uid);
		}
		uid->release();
	}
	_invalidate();
}

void MutableRouteObject::removeItemAtIndex(int index)
{
	if (index >= itemsCount())
		return;
	_removeItemAtIndex(index);
	LNotificationCenter::center()->postNotification(kRouteObjectItemCountDidChangeNotification, this, 0);
}

void MutableRouteObject::appendRouteObject(RouteObject *route, int index)
{
	if (index == -1)
		index = itemsCount();
	if (route == 0)
		return;
	bool changed = false;
	if (index > 0 && itemAtIndex(index - 1)->genericEntryInstance()) {
		changed = _insertItemAtIndex(RouteSetBreak::routeSetBreak(), index, 0, 0);
		index++;
	}
	int count = route->itemsCount();
	for (int i = 0; i < count; i++) {
		RouteSetItem *item = route->itemAtIndex(i);
		changed |= _insertItemAtIndex(item, index + i, 0, 0);
		if (GenericEntry *entry = item->genericEntryInstance()) {
			if (LArray *color = route->colorComponentsForItem(entry))
				_setColorForItem(color, entry);
			_setNameForItem(route->nameForItem(entry), entry);
		}
	}
	if (changed)
		LNotificationCenter::center()->postNotification(kRouteObjectItemCountDidChangeNotification, this, 0);
}

void MutableRouteObject::insertItemsWithOrderedIndexes(TLArray<LMap *> *items)
{
	int count = items ? items->count() : 0;
	bool changed = false;
	for (int i = 0; i < count; i++) {
		LMap *map = items->objectAtIndex(i);
		RouteSetItem *item = lobj_cast<RouteSetItem>(map->objectForKey(kItemValue));
		LValue *index = lobj_cast<LValue>(map->objectForKey(kItemIndex));
		LString *caption = lobj_cast<LString>(map->objectForKey(kItemCaption));
		LArray *color = lobj_cast<LArray>(map->objectForKey(kItemColor));
		changed |= _insertItemAtIndex(item, index ? index->intValue() : 0, caption, color);
	}
	if (changed)
		LNotificationCenter::center()->postNotification(kRouteObjectItemCountDidChangeNotification, this, 0);
}

void MutableRouteObject::removeItemsWithOrderedIndexes(TLArray<LValue *> *indexes)
{
	int count = indexes ? indexes->count() : 0;
	for (int i = count - 1; i >= 0; i--)
		_removeItemAtIndex(indexes->objectAtIndex(i)->intValue());
	if (count)
		LNotificationCenter::center()->postNotification(kRouteObjectItemCountDidChangeNotification, this, 0);
}

void MutableRouteObject::setColorForItems(LArray *components, TLArray<GenericEntry *> *entries)
{
	int count = entries ? entries->count() : 0;
	for (int i = 0; i < count; i++)
		_setColorForItem(components, entries->objectAtIndex(i));
	if (count)
		LNotificationCenter::center()->postNotification(kRouteObjectAttributesDidChangeNotification, this, 0);
}

void MutableRouteObject::setColorsForItems(TLArray<LMap *> *items)
{
	int count = items ? items->count() : 0;
	for (int i = 0; i < count; i++) {
		LMap *map = items->objectAtIndex(i);
		RouteSetItem *item = lobj_cast<RouteSetItem>(map->objectForKey(kItemValue));
		GenericEntry *entry = item ? item->genericEntryInstance() : 0;
		LArray *color = lobj_cast<LArray>(map->objectForKey(kItemColor));
		if (entry)
			_setColorForItem(color, entry);
	}
	if (count)
		LNotificationCenter::center()->postNotification(kRouteObjectAttributesDidChangeNotification, this, 0);
}

// Moves the items at the (ascending) indexes to *index, which is updated
// for the items removed before it.
void MutableRouteObject::moveItemsFromIndexesToIndex(TLArray<LValue *> *indexes, int *index)
{
	if (index == 0)
		return;
	int count = indexes ? indexes->count() : 0;
	int moved = 0;
	int removedBefore = 0;
	for (int i = 0; i < count; i++) {
		LValue *value = lobj_cast<LValue>(indexes->objectAtIndex(i));
		int from = value ? value->intValue() : 0;
		if (from < moved + *index) {
			(*index)--;
			from -= removedBefore++;
		}
		if (from != moved + *index) {
			LObject *item = LRetain(_items->objectAtIndex(from));
			_items->removeObjectAtIndex(from);
			_items->insertObjectAtIndex(item, moved++ + *index);
			if (item)
				item->release();
		}
	}
	if (count) {
		_invalidate();
		LNotificationCenter::center()->postNotification(kRouteObjectItemCountDidChangeNotification, this, 0);
	}
}

// Moves the items starting at *index to the indexes.
void MutableRouteObject::moveItemsStartingAtIndexToIndexes(int *index, TLArray<LValue *> *indexes)
{
	if (index == 0)
		return;
	int count = indexes ? indexes->count() : 0;
	for (int i = 0; i < count; i++) {
		LValue *value = lobj_cast<LValue>(indexes->objectAtIndex(i));
		int to = value ? value->intValue() : 0;
		LObject *item = LRetain(_items->objectAtIndex(*index));
		_items->removeObjectAtIndex(*index);
		if (to < *index)
			(*index)++;
		else
			to += count - i;
		_items->insertObjectAtIndex(item, to);
		if (item)
			item->release();
	}
	if (count) {
		_invalidate();
		LNotificationCenter::center()->postNotification(kRouteObjectItemCountDidChangeNotification, this, 0);
	}
}

void MutableRouteObject::removeAllItems()
{
	_items->removeAllObjects();
	if (_colors)
		_colors->release();
	if (_names)
		_names->release();
	_colors = LMap::_alloc()->init();
	_names = LMap::_alloc()->init();
	_invalidate();
	LNotificationCenter::center()->postNotification(kRouteObjectItemCountDidChangeNotification, this, 0);
}

void MutableRouteObject::replaceItemsWithRouteObject(RouteObject *route)
{
	if (route == 0) {
		removeAllItems();
		return;
	}
	if (_colors)
		_colors->release();
	if (_names)
		_names->release();
	if (_items)
		_items->release();
	_colors = LMap::_alloc()->initWithMap(route->_colors);
	_names = LMap::_alloc()->initWithMap(route->_names);
	_items = LArray::_alloc()->initWithArray(route->_items);
	_invalidate();
	LNotificationCenter::center()->postNotification(kRouteObjectItemCountDidChangeNotification, this, 0);
}
