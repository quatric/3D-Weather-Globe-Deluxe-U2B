// SmartBillboardSystem: lays billboards out over the view so they don't
// pile up.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(SmartBillboardSystem, 0)
LF_SELECTOR(SmartBillboardSystem, _visChangedDelayedSelector);

SmartBillboardSystem::SmartBillboardSystem()
    : _layoutValid(false), _entries(0), _entriesByKey(0), _maxCoverage(0.25f), _visibilityChanged(false),
      _viewSize(LZeroSize), _visibleKinds(0xffffffff), _autohideKinds(0), _visTimer(0)
{
}

SmartBillboardSystem *SmartBillboardSystem::init()
{
	return LObject::init() ? this : 0;
}

void SmartBillboardSystem::dealloc()
{
	releaseAndClear(_entries);
	releaseAndClear(_entriesByKey);
	LObject::dealloc();
}

void SmartBillboardSystem::invalidate()
{
	if (_visTimer)
		_visTimer->invalidate();
	_visTimer = 0;
	release();
}

void SmartBillboardSystem::removeAllEntries()
{
	releaseAndClear(_entries);
	releaseAndClear(_entriesByKey);
	invalLayout();
}

intptr_t SmartBillboardSystem::_visChangedDelayedSelector(char *)
{
	_visTimer = 0;
	_visibilityChanged = true;
	return 0;
}

SmartBillboardData *SmartBillboardSystem::entryForKey(LString *key)
{
	return _entriesByKey ? (SmartBillboardData *)_entriesByKey->objectForKey(key) : 0;
}

void SmartBillboardSystem::enumEntries(ISmartBBCallback *callback, unsigned char skipIfLaidOut, unsigned char forward,
                                       unsigned char notify)
{
	if (callback == 0)
		return;
	if (_visibilityChanged) {
		int count = _entries ? _entries->count() : 0;
		int shown = 0;
		for (int i = 0; i < count; i++) {
			bool wasShown = false;
			if (shown < (int)_shownIndices.count() && (unsigned int)i == _shownIndices[shown]) {
				shown++;
				wasShown = true;
			}
			SmartBillboardData *entry = (SmartBillboardData *)_entries->objectAtIndex(i);
			if (notify && wasShown != entry->_visible) {
				if (!entry->_visible)
					callback->entryBecomesInvisible(entry);
				else
					callback->entryBecomesVisible(entry);
			}
		}
	}
	_visibilityChanged = false;
	if (!_layoutValid)
		_performFastLayout(callback, notify);
	else if (skipIfLaidOut)
		return;
	int count = _visibleIndices.count();
	callback->enumWasStarted(this);
	for (int i = 0; i < count; i++) {
		unsigned int index = forward ? _visibleIndices[i] : _visibleIndices[count - 1 - i];
		SmartBillboardData *entry = (SmartBillboardData *)_entries->objectAtIndex(index);
		callback->enumEntryWithRect(this, entry, &entry->_rect);
	}
	callback->enumWasFinished(this);
}

void SmartBillboardSystem::_performFastLayout(ISmartBBCallback *callback, unsigned char notify)
{
	if (callback == 0)
		return;
	int count = _entries ? _entries->count() : 0;
	_visibleIndices.removeAllObjects();
	float viewArea = _viewSize.width * _viewSize.height;
	float covered = 0;
	unsigned int shownCount = 0;
	bool crowded = false;
	for (int i = 0; i < count; i++) {
		SmartBillboardData *entry = (SmartBillboardData *)_entries->objectAtIndex(i);
		LPoint position = entry->_position;
		bool show = !(crowded && !entry->_showAlways) && (entry->_kind & _visibleKinds) != 0;
		if (show) {
			LSize size = entry->bbSizeForVisibleMask(_visibleKinds);
			LSize offset = size;
			switch (entry->_orientation) {
			case 2:
				offset.width = -size.width;
				offset.height = -size.height;
				break;
			case 3:
				offset.width = -size.width;
				break;
			case 0:
				offset.height = -size.height;
				break;
			}
			LPoint viewPoint;
			show = callback->geoPointToViewPoint(&viewPoint, &position, &offset);
			if (show) {
				entry->_rect = LMakeRect(viewPoint.x, viewPoint.y, size.width, size.height);
				switch (entry->_orientation) {
				case 2:
					entry->_rect.origin.y -= size.height;
					// fall through
				case 3:
					entry->_rect.origin.x -= offset.width;
					break;
				case 0:
					entry->_rect.origin.y -= size.height;
					break;
				}
				if ((entry->_kind & _autohideKinds) != 0 && !entry->_showAlways) {
					int visible = _visibleIndices.count();
					for (int j = 0; j < visible; j++) {
						SmartBillboardData *other =
						    (SmartBillboardData *)_entries->objectAtIndex(_visibleIndices[j]);
						LRect common = LIntersectionRect(other->_rect, entry->_rect);
						if (common.size.width >= 0.5f && common.size.height >= 0.5f) {
							float part = common.size.width / size.width;
							if (common.size.height / size.height <= part)
								part = common.size.height / size.height;
							// Hysteresis: shown ones stay a little longer.
							float factor = entry->_visible ? 0.9f : 1.1f;
							if (part * factor > 0.3f) {
								show = false;
								break;
							}
						}
					}
				}
			}
		}
		if (notify && show != entry->_visible) {
			if (_visTimer == 0) {
				_shownIndices.removeAllObjects();
				for (int j = 0; j < count; j++)
					if (((SmartBillboardData *)_entries->objectAtIndex(j))->_visible)
						_shownIndices.addObject(j);
			} else {
				_visTimer->invalidate();
				_visTimer = 0;
			}
			_visTimer = LTimer::timerWithParams(300, this, "_visChangedDelayedSelector", false, 0);
		}
		entry->_visible = show;
		if (show) {
			_visibleIndices.addObject(i);
			shownCount++;
			covered += entry->_rect.size.width * entry->_rect.size.height;
			crowded = shownCount >= 3 && covered / viewArea > _maxCoverage;
		}
	}
	_layoutValid = true;
}

SmartBillboardData *SmartBillboardSystem::findBillboard(ISmartBBCallback *callback, LPoint viewPoint)
{
	if (!_layoutValid)
		_performFastLayout(callback, 0);
	int count = _visibleIndices.count();
	for (int i = 0; i < count; i++) {
		SmartBillboardData *entry = (SmartBillboardData *)_entries->objectAtIndex(_visibleIndices[i]);
		const LRect &r = entry->_rect;
		if (r.origin.x <= viewPoint.x && r.origin.y <= viewPoint.y && viewPoint.x <= r.origin.x + r.size.width &&
		    viewPoint.y <= r.size.height + r.origin.y)
			return entry;
	}
	return 0;
}

void SmartBillboardSystem::removeAllEntriesWithKind(unsigned int kind)
{
	int count = _entries ? _entries->count() : 0;
	for (int i = 0; i < count; i++) {
		SmartBillboardData *entry = (SmartBillboardData *)_entries->objectAtIndex(i);
		if ((kind & entry->_kind) == 0)
			continue;
		entry->_kind &= ~kind;
		if (entry->_kind != 0)
			continue;
		if (_entriesByKey)
			_entriesByKey->removeObjectForKey(entry->uniqueKey());
		_entries->removeObjectAtIndex(i);
		i--;
		count--;
	}
	invalLayout();
}

void SmartBillboardSystem::addEntry(SmartBillboardData *entry)
{
	if (entry == 0)
		return;
	if (_entries == 0)
		_entries = LArray::_alloc()->init();
	if (_entries) {
		_entries->addObject(entry);
		LComparable *key = entry->uniqueKey();
		if (key) {
			if (_entriesByKey == 0)
				_entriesByKey = LMap::_alloc()->init();
			if (_entriesByKey)
				_entriesByKey->setObjectForKey(entry, key);
		}
	}
	invalLayout();
}
