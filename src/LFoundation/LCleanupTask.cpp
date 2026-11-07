// LCleanupTask
//
// LF::LCleanupTask: the list of cleanup tasks.

#include "LCleanupTask.h"

namespace LF {

TArray<LCleanupTask *> *LCleanupTask::sAllTasks = 0;
bool LCleanupTask::sWasCleanedUp = false;

CSharableResource *LCleanupTask::allTasksLocker()
{
	static CSharableResource rsrc;
	return &rsrc;
}

LCleanupTask::LCleanupTask()
{
	StSharableLocker locker(allTasksLocker());
	if (sAllTasks == 0)
		sAllTasks = new TArray<LCleanupTask *>;
	sAllTasks->addObject(this);
}

LCleanupTask::~LCleanupTask()
{
	StSharableLocker locker(allTasksLocker());
	sAllTasks->removeObject(this);
}

void LCleanupTask::cleanupAllObjects()
{
	StSharableLocker locker(allTasksLocker());
	if (sWasCleanedUp)
		return;
	sWasCleanedUp = true;
	int n = sAllTasks ? sAllTasks->count() : 0;
	for (int i = n - 1; i >= 0; i--)
		(*sAllTasks)[i]->performCleanup();
}

} // namespace LF
