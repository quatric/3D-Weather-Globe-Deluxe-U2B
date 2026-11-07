// LNotificationCenter
//
// LF::LNotificationCenter: the shared notification center, and the
// non-Objective-C part of LF::ObserverWrapper. Observers are found under the
// lock and called after it is released.

#include "LNotification.h"
#include "LArray.h"

namespace LF {

LF_IMPLEMENT_CLASS(LNotificationCenter, 0)
LF_IMPLEMENT_CLASS(ObserverWrapper, 0)

ObserverWrapper *ObserverWrapper::initWithObserver(id observer)
{
	if (!LObject::init())
		return 0;
	_observer = observer;
	return this;
}

LNotificationCenter *LNotificationCenter::center()
{
	return (LNotificationCenter *)classObject()->singleton(0);
}

void LNotificationCenter::terminateCenter()
{
	classObject()->killSingleton();
}

void LNotificationCenter::initAsSingleton(void *)
{
	_observers = LMap::_alloc()->init();
	_wrappers = new TLIntPtrMap<ObserverWrapper *>;
	_wrapperObservers = new CHashTable;
}

void LNotificationCenter::dealloc()
{
	{
		StSharableLocker locker(&_lock);
		if (_observers) {
			_observers->release();
			_observers = 0;
		}
	}
	if (_wrapperObservers) {
		delete _wrapperObservers;
		_wrapperObservers = 0;
	}
	if (_wrappers) {
		delete _wrappers;
		_wrappers = 0;
	}
	LObject::dealloc();
}

// An observation through an ObserverWrapper ended; the wrapper goes when it
// has none left.
void LNotificationCenter::_releaseWrapperUse(LObject *observer)
{
	intptr_t objcObserver;
	if (_wrapperObservers->getValueForKey(&objcObserver, (intptr_t)observer)
	    && --((ObserverWrapper *)observer)->_useCount == 0) {
		_wrapperObservers->removeValueForKey((intptr_t)observer);
		_wrappers->removeObjectForKey(objcObserver);
	}
}

void LNotificationCenter::removeObserver(LObject *observer, const LString *name, TPerformLockingType locking)
{
	StSharableLocker *locker = 0;
	if (locking == kLock)
		locker = new StSharableLocker(&_lock);
	LArray *entries = (LArray *)_observers->objectForKey(name);
	if (entries) {
		for (int i = 0; i < (int)entries->count(); i++) {
			SObserverEntry *entry = _entry(entries->objectAtIndex(i));
			if (entry->observer == observer) {
				if (entry->objects) {
					delete entry->objects;
					entry->objects = 0;
				}
				_releaseWrapperUse(observer);
				entries->removeObjectAtIndex(i);
				break;
			}
		}
	}
	delete locker;
}

// Forgets the observer's selector for `object`; the observation ends when it
// has no selectors left.
void LNotificationCenter::removeObserver(LObject *observer, const LString *name, LObject *object)
{
	if (object == 0) {
		removeObserver(observer, name, kLock);
		return;
	}
	StSharableLocker locker(&_lock);
	LArray *entries = (LArray *)_observers->objectForKey(name);
	if (entries == 0)
		return;
	for (int i = 0; i < (int)entries->count(); i++) {
		SObserverEntry *entry = _entry(entries->objectAtIndex(i));
		if (entry->observer != observer)
			continue;
		int n = entry->objects ? (int)entry->objects->count() : 0;
		for (int k = n - 1; k >= 0; k--) {
			if ((*entry->objects)[k].object != object)
				continue;
			entry->objects->removeObjectAtIndex(k);
			if (entry->objects->count() == 0) {
				delete entry->objects;
				entry->objects = 0;
				if (entry->sel == 0) {
					_releaseWrapperUse(observer);
					entries->removeObjectAtIndex(i);
				}
				break;
			}
		}
		break;
	}
}

void LNotificationCenter::removeObserver(LObject *observer)
{
	StSharableLocker locker(&_lock);
	LMapIterator it(_observers, true);
	while (it.next())
		removeObserver(observer, (const LString *)it.key(), kDontLock);
}

void LNotificationCenter::addObserver(LObject *observer, const char *sel, const LString *name, LObject *object)
{
	StSharableLocker locker(&_lock);
	SObserverEntry fresh = {observer, 0, 0};
	LArray *entries = (LArray *)_observers->objectForKey(name);
	if (entries == 0) {
		entries = LArray::array();
		_observers->setObjectForKey(entries, name);
	}
	SObserverEntry *entry = &fresh;
	bool isNew = true;
	for (int i = 0; i < (int)entries->count(); i++) {
		SObserverEntry *e = _entry(entries->objectAtIndex(i));
		if (e->observer == observer) {
			entry = e;
			isNew = false;
			break;
		}
	}
	if (object == 0)
		entry->sel = sel;
	else {
		if (entry->objects == 0)
			entry->objects = new TArray<SObjSelPair>;
		SObjSelPair pair = {object, sel};
		entry->objects->addObject(pair);
	}
	if (isNew) {
		entries->addObject(LValue::valueWithBytes(&fresh, sizeof fresh));
		if (observer->getClass()->isSubclassOf(ObserverWrapper::classObject()))
			((ObserverWrapper *)observer)->_useCount++;
	}
}

void LNotificationCenter::postNotification(LNotification *ntf)
{
	StAutoreleasePool pool;
	LArray *observers = LArray::array();
	TArray<const char *> selectors;
	{
		StSharableLocker locker(&_lock);
		LArray *entries = (LArray *)_observers->objectForKey(ntf->_name);
		if (entries == 0)
			return;
		int n = entries->count();
		for (int i = 0; i < n; i++) {
			SObserverEntry *entry = _entry(entries->objectAtIndex(i));
			const char *sel = 0;
			if (ntf->_object == 0 || entry->objects == 0)
				sel = entry->sel;
			else {
				for (int k = 0; k < (int)entry->objects->count(); k++) {
					SObjSelPair &pair = (*entry->objects)[k];
					if (pair.object == ntf->_object) {
						sel = pair.sel;
						break;
					}
				}
			}
			if (sel) {
				observers->addObject(entry->observer);
				selectors.addObject(sel);
			}
		}
	}
	int n = observers->count();
	for (int i = 0; i < n; i++) {
		LObject *observer = observers->objectAtIndex(i);
		if (observer->getClass()->isSubclassOf(ObserverWrapper::classObject()))
			((ObserverWrapper *)observer)->performSelector(selectors[i], ntf);
		else
			observer->invokeSelector(selectors[i], ntf);
	}
}

void LNotificationCenter::postNotification(const LString *name, LObject *object, LMap *userInfo)
{
	StAutoreleasePool pool;
	postNotification(LNotification::notificationWithParams(object, name, userInfo));
}

ObserverWrapper *LNotificationCenter::_wrapperFor(id observer) const
{
	return _wrappers->objectForKey((intptr_t)observer);
}

void LNotificationCenter::removeObserver(id observer)
{
	if (ObserverWrapper *w = _wrapperFor(observer))
		removeObserver(w);
}

void LNotificationCenter::removeObserver(id observer, const LString *name, TPerformLockingType locking)
{
	if (ObserverWrapper *w = _wrapperFor(observer))
		removeObserver(w, name, locking);
}

void LNotificationCenter::removeObserver(id observer, const LString *name, LObject *object)
{
	if (object == 0) {
		removeObserver(observer, name, kLock);
		return;
	}
	if (ObserverWrapper *w = _wrapperFor(observer))
		removeObserver(w, name, object);
}

// The selector is passed on as the observer's -performSelector: argument.
void LNotificationCenter::addObserver(id observer, SEL sel, const LString *name, LObject *object)
{
	ObserverWrapper *w = _wrapperFor(observer);
	if (w == 0) {
		w = ObserverWrapper::_alloc()->initWithObserver(observer);
		_wrappers->setObjectForKey(w, (intptr_t)observer);
		w->release();
		_wrapperObservers->setValueForKey((intptr_t)w, (intptr_t)observer);
	}
	addObserver(w, (const char *)sel, name, object);
}

} // namespace LF
