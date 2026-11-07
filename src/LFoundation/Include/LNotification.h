// LNotification, LNotificationCenter: named notifications sent to observers'
// selectors. An Objective-C observer is represented by an ObserverWrapper,
// which forwards the notification with -performSelector:withObject:.
#pragma once

#include "LMap.h"
#include "LValue.h"
#include <objc/objc.h>

namespace LF {

class LNotification : public LObject {
	LF_DECLARE_CLASS(LNotification)
public:
	LNotification() : _object(0), _name(0), _userInfo(0) {}
	virtual ~LNotification() {}
	virtual LString *internalDescription() const;
	virtual void dealloc();
	virtual LNotification *getOwnerLObject() { return this; }

	static LNotification *notificationWithParams(LObject *object, const LString *name, LMap *userInfo);
	LNotification *initWithNtfParams(LObject *object, const LString *name, LMap *userInfo);

	LObject *object() const { return _object; }
	LString *name() const { return _name; }
	LMap *userInfo() const { return _userInfo; }

	LObject *_object;
	LString *_name;
	LMap *_userInfo;
};

enum TPerformLockingType {
	kDontLock = 0,
	kLock = 1,
};

// An Objective-C observer.
class ObserverWrapper : public LObject {
	LF_DECLARE_CLASS(ObserverWrapper)
public:
	ObserverWrapper() : _useCount(0), _observer(0) {}
	virtual ~ObserverWrapper() {}
	virtual ObserverWrapper *getOwnerLObject() { return this; }

	ObserverWrapper *initWithObserver(id observer);
	// In NSObjToLObjWrapper.mm.
	void performSelector(const char *sel, LNotification *ntf);

	int _useCount;	// the observations made through this wrapper
	id _observer;
};

class LNotificationCenter : public LObject {
	LF_DECLARE_CLASS(LNotificationCenter)
public:
	// One observer of one notification name: its selector for any object,
	// and its selectors for particular objects. Kept in an LValue.
	struct SObjSelPair {
		LObject *object;
		const char *sel;
	};
	struct SObserverEntry {
		LObject *observer;
		const char *sel;
		TArray<SObjSelPair> *objects;
	};

	LNotificationCenter() : _observers(0), _wrappers(0), _wrapperObservers(0) {}
	virtual ~LNotificationCenter() {}
	virtual void initAsSingleton(void *param);
	virtual void dealloc();
	virtual LNotificationCenter *getOwnerLObject() { return this; }

	static LNotificationCenter *center();
	static void terminateCenter();

	// `sel` is called with the LNotification; a null `object` observes the
	// notification from any object.
	void addObserver(LObject *observer, const char *sel, const LString *name, LObject *object);
	void removeObserver(LObject *observer);
	void removeObserver(LObject *observer, const LString *name, TPerformLockingType locking);
	void removeObserver(LObject *observer, const LString *name, LObject *object);
	void addObserver(id observer, SEL sel, const LString *name, LObject *object);
	void removeObserver(id observer);
	void removeObserver(id observer, const LString *name, TPerformLockingType locking);
	void removeObserver(id observer, const LString *name, LObject *object);

	void postNotification(LNotification *ntf);
	void postNotification(const LString *name, LObject *object, LMap *userInfo);

	static SObserverEntry *_entry(LObject *value) { return (SObserverEntry *)((LValue *)value)->bytes(); }
	void _releaseWrapperUse(LObject *observer);
	ObserverWrapper *_wrapperFor(id observer) const;

	LMap *_observers;	// name -> LArray of LValue(SObserverEntry)
	CSharableResource _lock;
	TLIntPtrMap<ObserverWrapper *> *_wrappers;	// Objective-C observer -> its wrapper
	CHashTable *_wrapperObservers;	// wrapper -> Objective-C observer
};

} // namespace LF
