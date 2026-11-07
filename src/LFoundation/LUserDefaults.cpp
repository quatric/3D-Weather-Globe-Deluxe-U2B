// LUserDefaults
//
// LF::LUserDefaults. The property list goes through its XML form both ways.

#include "LUserDefaults.h"
#include "LXMLTree.h"

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace LF {

LF_IMPLEMENT_CLASS(LUserDefaults, 0)

LConstantString LUserDefaults::__strRootKey("LUserDefaults");

void LUserDefaults::setUniqueRootKey(LString *appID)
{
	classObject()->killSingleton();
	classObject()->singleton(appID);
}

void LUserDefaults::initAsSingleton(void *param)
{
	StAutoreleasePool pool;
	_appID = LRetain((LString *)param);
	CFPropertyListRef plist;
	if (_appID == 0)
		plist = CFPreferencesCopyAppValue(__strRootKey._cf, kCFPreferencesCurrentApplication);
	else
		plist = CFPreferencesCopyValue(__strRootKey._cf, _appID->_cf, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
	if (plist) {
		CFDataRef xml = CFPropertyListCreateXMLData(kCFAllocatorDefault, plist);
		if (xml) {
			_values = LMap::_alloc()->init();
			LXMLTree::objectFromXMLData((const char *)CFDataGetBytePtr(xml), CFDataGetLength(xml), _values);
			CFRelease(xml);
		}
		CFRelease(plist);
	}
	if (_values == 0)
		_values = LMap::_alloc()->init();
}

void LUserDefaults::dealloc()
{
	if (_values) {
		_values->release();
		_values = 0;
	}
	if (_defaults) {
		_defaults->release();
		_defaults = 0;
	}
	if (_appID) {
		_appID->release();
		_appID = 0;
	}
	LObject::dealloc();
}

LObject *LUserDefaults::objectForKey(const LComparable *key) const
{
	StSharableLocker locker(&_lock);
	LObject *obj = _values->objectForKey(key);
	if (obj == 0 && _defaults)
		obj = _defaults->objectForKey(key);
	return obj;
}

void LUserDefaults::setObjectForKey(LObject *obj, const LComparable *key)
{
	StSharableLocker locker(&_lock);
	_values->setObjectForKey(obj, key);
}

void LUserDefaults::removeObjectForKey(const LComparable *key)
{
	if (_values)
		_values->removeObjectForKey(key);
}

LString *LUserDefaults::stringForKey(const LComparable *key)
{
	StSharableLocker locker(&_lock);
	LObject *obj = objectForKey(key);
	if (obj && !obj->getClass()->isSubclassOf(LF_CLASSOF(LString)))
		obj = LString::stringWithFormat("%@", obj);
	return (LString *)obj;
}

void LUserDefaults::registerDefaults(LMap *defaults)
{
	StSharableLocker locker(&_lock);
	if (_defaults) {
		_defaults->release();
		_defaults = 0;
	}
	_defaults = LMap::_alloc()->initWithMap(defaults);
}

void LUserDefaults::synchronize()
{
	StAutoreleasePool pool;
	StSharableLocker locker(&_lock);
	LString *xml = _values->plistRepresentation(0);
	CFDataRef data = CFStringCreateExternalRepresentation(kCFAllocatorDefault, xml->_cf, kCFStringEncodingUTF8, 0);
	if (data == 0)
		return;
	CFPropertyListRef plist = CFPropertyListCreateFromXMLData(kCFAllocatorDefault, data, 0, 0);
	if (plist) {
		if (_appID == 0) {
			CFPreferencesSetAppValue(__strRootKey._cf, plist, kCFPreferencesCurrentApplication);
			CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
		} else {
			CFPreferencesSetValue(__strRootKey._cf, plist, _appID->_cf, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
			CFPreferencesSynchronize(_appID->_cf, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost);
		}
		CFRelease(plist);
	}
	CFRelease(data);
}

} // namespace LF
