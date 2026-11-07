// The entry proxies: several selected entries read as one. A property is the
// common value, or flagged as mixed when the entries differ.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(GenericEntryProxy, 0)
LF_IMPLEMENT_CLASS(PointEntryProxy, LF_CLASSOF(GenericEntryProxy))
LF_IMPLEMENT_CLASS(CityEntryProxy, LF_CLASSOF(PointEntryProxy))
LF_IMPLEMENT_CLASS(FeatureEntryProxy, LF_CLASSOF(PointEntryProxy))
LF_IMPLEMENT_CLASS(ShapeEntryProxy, LF_CLASSOF(GenericEntryProxy))
LF_IMPLEMENT_CLASS(AppleStoreEntryProxy, LF_CLASSOF(PointEntryProxy))
LF_IMPLEMENT_CLASS(UserCustomEntryProxy, LF_CLASSOF(PointEntryProxy))
LF_IMPLEMENT_CLASS(RouteObjectEntryProxy, LF_CLASSOF(GenericEntryProxy))

bool entryPropertyObjectsEqual(LObject *a, LObject *b)
{
	if (a == b)
		return true;
	LComparable *ca = lobj_cast<LComparable>(a);
	LComparable *cb = lobj_cast<LComparable>(b);
	if (ca && ca->isEqualTo(cb))
		return true;
	if (cb && cb->isEqualTo(ca))
		return true;
	return false;
}

static bool arePointsEqual(LPoint a, LPoint b)
{
	return fabsf(a.x - b.x) < 0.0001f && fabsf(a.y - b.y) < 0.0001f;
}

// ---------------------------------------------------------------------------
// GenericEntryProxy

GenericEntryProxy::GenericEntryProxy() : _entries(0)
{
}

void GenericEntryProxy::dealloc()
{
	if (_entries)
		_entries->release();
	LObject::dealloc();
}

void GenericEntryProxy::initWithEntryArray(TLArray<GenericEntry *> *entries)
{
	if (entries) {
		_entries = entries;
		entries->retain();
	}
}

void GenericEntryProxy::interfaceRetain()
{
	retain();
}

void GenericEntryProxy::interfaceRelease()
{
	release();
}

#define PROXY_OBJECT_GETTER(Type, Entry, name)                                                           \
	Type *GenericEntryProxy::name(unsigned char *mixed) const                                       \
	{                                                                                                \
		return _getter<Type *, Entry>::getProperty(_entries, &Entry::name, mixed,                \
		                                           _getter<Type *, Entry>::areObjectsEqual, 0); \
	}

PROXY_OBJECT_GETTER(LMap, GenericEntry, userInfo)
PROXY_OBJECT_GETTER(LArray, GenericEntry, alternativeNames)
PROXY_OBJECT_GETTER(LString, GenericEntry, regionName)
PROXY_OBJECT_GETTER(LString, GenericEntry, countryName)
PROXY_OBJECT_GETTER(LString, GenericEntry, stateName)
PROXY_OBJECT_GETTER(LString, GenericEntry, countyName)
PROXY_OBJECT_GETTER(LString, GenericEntry, timeZoneName)
PROXY_OBJECT_GETTER(LString, GenericEntry, kind)
PROXY_OBJECT_GETTER(LString, GenericEntry, altType)
PROXY_OBJECT_GETTER(LString, GenericEntry, ownType)
PROXY_OBJECT_GETTER(LString, GenericEntry, shortCaption)
PROXY_OBJECT_GETTER(LString, GenericEntry, ownName)
PROXY_OBJECT_GETTER(LString, GenericEntry, uid)

int GenericEntryProxy::population(unsigned char *mixed) const
{
	return _getter<int, GenericEntry>::getProperty(_entries, &GenericEntry::population, mixed,
	                                               _getter<int, GenericEntry>::areEqual, 0);
}

// ---------------------------------------------------------------------------
// PointEntryProxy

void PointEntryProxy::interfaceRetain()
{
	retain();
}

void PointEntryProxy::interfaceRelease()
{
	release();
}

LPoint PointEntryProxy::geoPoint(unsigned char *mixed) const
{
	return _getter<LPoint, PointEntry>::getProperty(_entries, &PointEntry::geoPoint, mixed, arePointsEqual, LZeroPoint);
}

int PointEntryProxy::customWeatherID(unsigned char *mixed) const
{
	return _getter<int, PointEntry>::getProperty(_entries, &PointEntry::customWeatherID, mixed,
	                                             _getter<int, PointEntry>::areEqual, 0);
}

LTimeZone *PointEntryProxy::timeZone(unsigned char *mixed) const
{
	return _getter<LTimeZone *, PointEntry>::getProperty(_entries, &PointEntry::timeZone, mixed,
	                                                     _getter<LTimeZone *, PointEntry>::areObjectsEqual, 0);
}

int PointEntryProxy::currentGMTOffset(unsigned char *mixed) const
{
	return _getter<int, PointEntry>::getProperty(_entries, &PointEntry::currentGMTOffset, mixed,
	                                             _getter<int, PointEntry>::areEqual, 0);
}

// ---------------------------------------------------------------------------
// The kind-specific proxies add only their interface.

#define PROXY_RETAIN_RELEASE(Name)           \
	void Name::interfaceRetain()         \
	{                                    \
		retain();                    \
	}                                    \
	void Name::interfaceRelease()        \
	{                                    \
		release();                   \
	}

PROXY_RETAIN_RELEASE(CityEntryProxy)
PROXY_RETAIN_RELEASE(FeatureEntryProxy)
PROXY_RETAIN_RELEASE(ShapeEntryProxy)
PROXY_RETAIN_RELEASE(AppleStoreEntryProxy)
PROXY_RETAIN_RELEASE(UserCustomEntryProxy)
PROXY_RETAIN_RELEASE(RouteObjectEntryProxy)

LString *RouteObjectEntryProxy::blobUID(unsigned char *mixed) const
{
	return _getter<LString *, RouteObjectEntry>::getProperty(_entries, &RouteObjectEntry::blobUID, mixed,
	                                                         _getter<LString *, RouteObjectEntry>::areObjectsEqual, 0);
}
