// CurrentSetController: the editing of the current route set. Each edit
// registers its opposite with the undo manager before it is made.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(CurrentSetController, 0)
LF_SELECTOR(CurrentSetController, _replaceCurrentSet);
LF_SELECTOR(CurrentSetController, _addToCurrentSet);
LF_SELECTOR(CurrentSetController, _deleteItemAtIndex);
LF_SELECTOR(CurrentSetController, _deleteItemsWithOrderedIndicies);
LF_SELECTOR(CurrentSetController, _insertItemIntoRoute);
LF_SELECTOR(CurrentSetController, _insertItemsWithOrderedIndexesIntoRoute);
LF_SELECTOR(CurrentSetController, _moveItemsFromIndexesToIndex);
LF_SELECTOR(CurrentSetController, _moveItemsStartingAtIndexToIndexes);
LF_SELECTOR(CurrentSetController, _setNameForEntry);
LF_SELECTOR(CurrentSetController, _setColorForEntry);
LF_SELECTOR(CurrentSetController, _setColorForEntries);
LF_SELECTOR(CurrentSetController, _setColorsForEntries);

static void _setUndoActionName(const char *name)
{
	CurrentSetController::undoManager()->setActionName(LString::stringFromTable(name, "Undo", getAppModelModule()));
}

CurrentSetController *CurrentSetController::init()
{
	if (LObject::init() == 0)
		return 0;
	if (Preferences::retainLastView())
		_route = MutableRouteObject::_alloc()->initWithDictionaryRep(Preferences::lastRouteDictionaryRep(), 0);
	else
		_route = MutableRouteObject::_alloc()->init();
	return this;
}

void CurrentSetController::dealloc()
{
	if (Preferences::retainLastView())
		Preferences::setLastRouteDictionaryRep(_route->dictionaryRepresentation());
	if (_route) {
		_route->release();
		_route = 0;
	}
	LObject::dealloc();
}

LUndoManager *CurrentSetController::undoManager()
{
	return LUndoManager::instance();
}

void CurrentSetController::replaceCurrentSet(RouteObject *route)
{
	invokeSelector("_replaceCurrentSet", route);
}

void CurrentSetController::addToCurrentSet(RouteObject *route, int index)
{
	invokeSelector("_addToCurrentSet", route, index);
}

void CurrentSetController::deleteItemAtIndex(int index)
{
	invokeSelector("_deleteItemAtIndex", LValue::valueWithInt(index));
}

void CurrentSetController::deleteItemsWithOrderedIndicies(TLArray<LValue *> *indexes)
{
	invokeSelector("_deleteItemsWithOrderedIndicies", indexes);
}

void CurrentSetController::deleteAllItems()
{
	invokeSelector("_replaceCurrentSet", (LObject *)0);
}

void CurrentSetController::insertItemIntoRoute(RouteSetItem *item, int index, LString *name, LArray *color)
{
	LValue *indexValue = LValue::valueWithInt(index);
	invokeSelector("_insertItemIntoRoute", item, LArray::arrayWithObjects(name, color, (LObject *)0), indexValue);
}

int CurrentSetController::moveItemsWithOrderedIndexesToIndex(TLArray<LValue *> *indexes, int index)
{
	return (int)invokeSelector("_moveItemsFromIndexesToIndex", indexes, LValue::valueWithInt(index));
}

void CurrentSetController::setNameForEntry(LString *name, GenericEntry *entry)
{
	invokeSelector("_setNameForEntry", name, entry);
}

void CurrentSetController::setColorForEntry(LArray *components, GenericEntry *entry)
{
	invokeSelector("_setColorForEntry", components, entry);
}

void CurrentSetController::setColorForEntries(LArray *components, TLArray<GenericEntry *> *entries)
{
	invokeSelector("_setColorForEntries", components, entries);
}

int CurrentSetController::insertEntriesFromDescrArray(TLArray<LMap *> *descriptions, int index)
{
	LArray *items = LArray::array();
	unsigned int count = descriptions ? descriptions->count() : 0;
	items->preserveCapacity(count);
	for (unsigned int i = 0; i < count; i++) {
		GenericEntry *entry = GenericEntry::entryWithDictionaryRep(descriptions->objectAtIndex(i));
		if (entry)
			items->addObject(entry);
	}
	return insertItemsIntoRoute((TLArray<RouteSetItem *> *)items, index);
}

static LMap *_breakAtIndex(LValue *index)
{
	return LMap::mapWithObjectsAndKeys(RouteSetBreak::routeSetBreak(), MutableRouteObject::kItemValue, index,
	                                   MutableRouteObject::kItemIndex, (LObject *)0);
}

int CurrentSetController::insertItemsIntoRoute(TLArray<RouteSetItem *> *items, int index)
{
	unsigned int count = items ? items->count() : 0;
	LArray *inserted = LArray::array();
	inserted->preserveCapacity(count);
	LArray *indexes = LArray::array();
	indexes->preserveCapacity(count);
	// Whether the item before is a break (or the start).
	bool afterBreak = index == 0 || _route->itemAtIndex(index - 1)->genericEntryInstance() == 0;
	for (int i = 0; i < (int)count; i++) {
		RouteSetItem *item = items->objectAtIndex(i);
		RouteObjectEntry *routeEntry = lobj_cast<RouteObjectEntry>(item);
		if (!routeEntry) {
			LValue *at = LValue::valueWithInt(index + inserted->count());
			inserted->addObject(LMap::mapWithObjectsAndKeys(item, MutableRouteObject::kItemValue, at,
			                                                MutableRouteObject::kItemIndex, (LObject *)0));
			indexes->addObject(at);
			afterBreak = false;
			continue;
		}
		// A route goes in as its items, between breaks.
		if (!afterBreak) {
			LValue *at = LValue::valueWithInt(index + inserted->count());
			inserted->addObject(_breakAtIndex(at));
			indexes->addObject(at);
		}
		RouteObject *route = routeEntry->routeObject();
		int routeCount = route ? route->itemsCount() : 0;
		for (int k = 0; k < routeCount; k++) {
			RouteSetItem *routeItem = route->itemAtIndex(k);
			LString *name = 0;
			LArray *color = 0;
			GenericEntry *entry = routeItem ? routeItem->genericEntryInstance() : 0;
			if (entry) {
				name = route->nameForItem(entry);
				color = route->colorComponentsForItem(entry);
			}
			LValue *at = LValue::valueWithInt(index + inserted->count());
			inserted->addObject(LMap::mapWithObjectsAndKeys(
			    routeItem, MutableRouteObject::kItemValue, at, MutableRouteObject::kItemIndex, name,
			    MutableRouteObject::kItemCaption, color, MutableRouteObject::kItemColor, (LObject *)0));
			indexes->addObject(at);
		}
		afterBreak = true;
		if (i + index < (int)(count - 1) + _route->itemsCount()) {
			LValue *at = LValue::valueWithInt(index + inserted->count());
			inserted->addObject(_breakAtIndex(at));
			indexes->addObject(at);
		}
	}
	if (count == 0)
		return 0;
	undoManager()->registerUndo(this, "_deleteItemsWithOrderedIndicies", indexes);
	_setUndoActionName(count < 2 ? "Insert Item" : "Insert Items");
	_route->insertItemsWithOrderedIndexes((TLArray<LMap *> *)inserted);
	return inserted->count();
}

intptr_t CurrentSetController::_replaceCurrentSet(char *args)
{
	RouteObject *route = LF_ARG(args, 0, RouteObject *);
	LObject *previous = _route->copy()->l_autorelease();
	undoManager()->registerUndo(this, "_replaceCurrentSet", previous);
	_setUndoActionName("Replace Route Object");
	_route->replaceItemsWithRouteObject(route);
	return 0;
}

intptr_t CurrentSetController::_addToCurrentSet(char *args)
{
	RouteObject *route = LF_ARG(args, 0, RouteObject *);
	int index = LF_ARG(args, 1, int);
	int count = _route->itemsCount();
	if (index == -1)
		index = count;
	LObject *previous = _route->copy()->l_autorelease();
	undoManager()->registerUndo(this, "_replaceCurrentSet", previous);
	_setUndoActionName("Add To Route Object");
	_route->appendRouteObject(route, index);
	// The original also listed the new indexes, unused.
	return 0;
}

intptr_t CurrentSetController::_deleteItemAtIndex(char *args)
{
	LValue *indexValue = LF_ARG(args, 0, LValue *);
	int index = indexValue ? indexValue->intValue() : 0;
	RouteSetItem *item = index < _route->itemsCount() ? _route->itemAtIndex(index) : 0;
	if (!item)
		return 0;
	GenericEntry *entry = item->genericEntryInstance();
	LArray *color = _route->colorComponentsForItem(entry);
	LArray *attributes = LArray::arrayWithObjects(_route->nameForItem(entry), color, (LObject *)0);
	undoManager()->registerUndo(this, "_insertItemIntoRoute", item, attributes, LValue::valueWithInt(index));
	_setUndoActionName("Delete Item");
	_route->removeItemAtIndex(index);
	return 0;
}

intptr_t CurrentSetController::_deleteItemsWithOrderedIndicies(char *args)
{
	TLArray<LValue *> *indexes = LF_ARG(args, 0, TLArray<LValue *> *);
	LArray *removed = LArray::array();
	int count = indexes ? indexes->count() : 0;
	for (int i = 0; i < count; i++) {
		LValue *index = indexes->objectAtIndex(i);
		RouteSetItem *item = _route->itemAtIndex(index->intValue());
		GenericEntry *entry = item->genericEntryInstance();
		LArray *color = _route->colorComponentsForItem(entry);
		LString *name = _route->nameForItem(entry);
		removed->addObject(LMap::mapWithObjectsAndKeys(item, MutableRouteObject::kItemValue, index,
		                                               MutableRouteObject::kItemIndex, name,
		                                               MutableRouteObject::kItemCaption, color,
		                                               MutableRouteObject::kItemColor, (LObject *)0));
	}
	undoManager()->registerUndo(this, "_insertItemsWithOrderedIndexesIntoRoute", removed);
	_setUndoActionName("Delete Items");
	_route->removeItemsWithOrderedIndexes(indexes);
	return 0;
}

intptr_t CurrentSetController::_insertItemIntoRoute(char *args)
{
	RouteSetItem *item = LF_ARG(args, 0, RouteSetItem *);
	LArray *attributes = LF_ARG(args, 1, LArray *);
	LValue *indexValue = LF_ARG(args, 2, LValue *);
	int index = indexValue ? indexValue->intValue() : 0;
	if (item) {
		if (RouteObjectEntry *routeEntry = lobj_cast<RouteObjectEntry>(item->genericEntryInstance())) {
			invokeSelector("_addToCurrentSet", routeEntry->routeObject(), index);
			return 0;
		}
	}
	LString *name = 0;
	LArray *color = 0;
	if (attributes) {
		if (attributes->count() != 0)
			name = lobj_cast<LString>(attributes->objectAtIndex(0));
		if (attributes->count() > 1)
			color = lobj_cast<LArray>(attributes->objectAtIndex(1));
	}
	_route->insertItemAtIndex(item, index, name, color);
	undoManager()->registerUndo(this, "_deleteItemAtIndex", indexValue);
	_setUndoActionName("Insert Item");
	return 0;
}

intptr_t CurrentSetController::_insertItemsWithOrderedIndexesIntoRoute(char *args)
{
	TLArray<LMap *> *items = LF_ARG(args, 0, TLArray<LMap *> *);
	LArray *indexes = LArray::array();
	int count = items->count();
	for (int i = 0; i < count; i++) {
		LValue *index = lobj_cast<LValue>(items->objectAtIndex(i)->objectForKey(MutableRouteObject::kItemIndex));
		if (index)
			indexes->addObject(index);
	}
	if (count == 0)
		return 0;
	undoManager()->registerUndo(this, "_deleteItemsWithOrderedIndicies", indexes);
	_setUndoActionName(count < 2 ? "Insert Item" : "Insert Items");
	_route->insertItemsWithOrderedIndexes(items);
	return 0;
}

intptr_t CurrentSetController::_moveItemsFromIndexesToIndex(char *args)
{
	TLArray<LValue *> *indexes = LF_ARG(args, 0, TLArray<LValue *> *);
	LValue *indexValue = LF_ARG(args, 1, LValue *);
	int count = indexes ? indexes->count() : 0;
	int index = indexValue ? indexValue->intValue() : 0;
	if (count == 0)
		return index;
	_route->moveItemsFromIndexesToIndex(indexes, &index);
	undoManager()->registerUndo(this, "_moveItemsStartingAtIndexToIndexes", LValue::valueWithInt(index), indexes);
	_setUndoActionName(count != 1 ? "Move Items" : "Move Item");
	return index;
}

intptr_t CurrentSetController::_moveItemsStartingAtIndexToIndexes(char *args)
{
	LValue *indexValue = LF_ARG(args, 0, LValue *);
	TLArray<LValue *> *indexes = LF_ARG(args, 1, TLArray<LValue *> *);
	int count = indexes ? indexes->count() : 0;
	if (count == 0)
		return 0;
	int index = indexValue ? indexValue->intValue() : 0;
	_route->moveItemsStartingAtIndexToIndexes(&index, indexes);
	undoManager()->registerUndo(this, "_moveItemsFromIndexesToIndex", indexes, LValue::valueWithInt(index));
	_setUndoActionName(count != 1 ? "Move Items" : "Move Item");
	return 0;
}

intptr_t CurrentSetController::_setNameForEntry(char *args)
{
	LString *name = LF_ARG(args, 0, LString *);
	GenericEntry *entry = LF_ARG(args, 1, GenericEntry *);
	undoManager()->registerUndo(this, "_setNameForEntry", _route->nameForItem(entry), entry);
	_setUndoActionName("Change Item Caption");
	_route->setNameForItem(name, entry);
	return 0;
}

intptr_t CurrentSetController::_setColorForEntry(char *args)
{
	LArray *components = LF_ARG(args, 0, LArray *);
	GenericEntry *entry = LF_ARG(args, 1, GenericEntry *);
	undoManager()->registerUndo(this, "_setColorForEntry", _route->colorComponentsForItem(entry), entry);
	_setUndoActionName("Change Item Color");
	_route->setColorForItem(components, entry);
	return 0;
}

intptr_t CurrentSetController::_setColorForEntries(char *args)
{
	LArray *components = LF_ARG(args, 0, LArray *);
	TLArray<GenericEntry *> *entries = LF_ARG(args, 1, TLArray<GenericEntry *> *);
	LArray *previous = LArray::array();
	int count = entries ? entries->count() : 0;
	for (int i = 0; i < count; i++) {
		GenericEntry *entry = entries->objectAtIndex(i);
		previous->addObject(LMap::mapWithObjectsAndKeys(entry, MutableRouteObject::kItemValue,
		                                                _route->colorComponentsForItem(entry),
		                                                MutableRouteObject::kItemColor, (LObject *)0));
	}
	if (count == 0)
		return 0;
	undoManager()->registerUndo(this, "_setColorsForEntries", previous);
	_setUndoActionName(count < 2 ? "Change Item Color" : "Change Items Color");
	_route->setColorForItems(components, entries);
	return 0;
}

intptr_t CurrentSetController::_setColorsForEntries(char *args)
{
	TLArray<LMap *> *items = LF_ARG(args, 0, TLArray<LMap *> *);
	LArray *previous = LArray::array();
	int count = items ? items->count() : 0;
	for (int i = 0; i < count; i++) {
		RouteSetItem *item = lobj_cast<RouteSetItem>(items->objectAtIndex(i)->objectForKey(MutableRouteObject::kItemValue));
		GenericEntry *entry = item ? item->genericEntryInstance() : 0;
		previous->addObject(LMap::mapWithObjectsAndKeys(entry, MutableRouteObject::kItemValue,
		                                                _route->colorComponentsForItem(entry),
		                                                MutableRouteObject::kItemColor, (LObject *)0));
	}
	if (count == 0)
		return 0;
	undoManager()->registerUndo(this, "_setColorsForEntries", previous);
	_setUndoActionName(count < 2 ? "Change Item Color" : "Change Items Color");
	_route->setColorsForItems(items);
	return 0;
}
