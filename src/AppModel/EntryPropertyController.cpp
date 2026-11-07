// EntryPropertyController: the icons of an entry's properties. This version
// of the atlas has none.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(EntryPropertyController, 0)

void EntryPropertyController::dealloc()
{
	if (_icons) {
		_icons->release();
		_icons = 0;
	}
	// The original didn't free the object.
	LObject::dealloc();
}

LImage *EntryPropertyController::iconForEntry(GenericEntry *)
{
	return 0;
}

LImage *EntryPropertyController::smallIconForEntry(GenericEntry *)
{
	return 0;
}
