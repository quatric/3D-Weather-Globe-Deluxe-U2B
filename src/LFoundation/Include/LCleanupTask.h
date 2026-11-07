// LCleanupTask: work done once when LFoundation is finalized. Every task
// registers itself on construction; cleanupAllObjects runs them, newest
// first.
#pragma once

#include "LContainers.h"

namespace LF {

class LCleanupTask {
public:
	LCleanupTask();
	virtual ~LCleanupTask();
	virtual void performCleanup() = 0;

	static CSharableResource *allTasksLocker();
	static void cleanupAllObjects();

	static TArray<LCleanupTask *> *sAllTasks;
	static bool sWasCleanedUp;
};

} // namespace LF
