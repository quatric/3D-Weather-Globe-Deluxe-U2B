// LUserDefaults: the app's preferences, kept as one property list under the
// key "LUserDefaults" in CFPreferences (of the app, or of the application ID
// given to setUniqueRootKey). Registered defaults are looked up second.
#pragma once

#include "LMap.h"

namespace LF {

class LUserDefaults : public LObject {
	LF_DECLARE_CLASS(LUserDefaults)
public:
	LUserDefaults() : _values(0), _defaults(0), _appID(0) {}
	virtual ~LUserDefaults() {}
	// `param` is the application ID (an LString) or 0 for the app's own.
	virtual void initAsSingleton(void *param);
	virtual void dealloc();
	virtual LUserDefaults *getOwnerLObject() { return this; }

	static LUserDefaults *standardUserDefaults() { return (LUserDefaults *)classObject()->singleton(0); }
	// Re-creates the shared defaults for another application ID.
	static void setUniqueRootKey(LString *appID);

	LObject *objectForKey(const LComparable *key) const;
	void setObjectForKey(LObject *obj, const LComparable *key);
	void removeObjectForKey(const LComparable *key);
	// The value, or its description if it isn't a string.
	LString *stringForKey(const LComparable *key);
	void registerDefaults(LMap *defaults);
	void synchronize();

	static LConstantString __strRootKey;

	LMap *_values;
	LMap *_defaults;
	LString *_appID;
	mutable CSharableResource _lock;
};

} // namespace LF
