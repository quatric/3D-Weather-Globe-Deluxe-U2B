// IWindowInterface: the window side of a panel controller; all pure but
// entryWasClicked.

#include "RenderLib.h"

void IWindowInterface::entryWasClicked(GenericEntry *entry, LPoint, TBillboardKind)
{
	LLog("Entry action %@\n", entry);
}
