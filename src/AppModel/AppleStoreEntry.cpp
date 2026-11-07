// AppleStoreEntry: an Apple retail store.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(AppleStoreEntry, LF_CLASSOF(PointEntry))

void AppleStoreEntry::interfaceRetain()
{
	retain();
}

void AppleStoreEntry::interfaceRelease()
{
	release();
}

LString *AppleStoreEntry::iconKey() const
{
	if (altType(0) == 0)
		return kEntryAppleStoreIconKey;
	if (altType(0)->isEqualTo(kAltTypeMiniRetail))
		return kEntryAppleMiniStoreIconKey;
	if (altType(0)->isEqualTo(kAltTypeComingSoonRetail))
		return kEntryAppleComingSoonStoreIconKey;
	return kEntryAppleStoreIconKey;
}
