// LFoundationInit
//
// LF::initializeLFoundation and LF::finilizeLFoundation.

#include "LUndoManager.h"
#include "LUserDefaults.h"
#include "LCleanupTask.h"
#include "LConnection.h"
#include "LNotification.h"
#include "LRunLoop.h"
#include <math.h>
#include <stdlib.h>

namespace LF {

static StAutoreleasePool *sLFoundationMainPool = 0;
static StAutoreleasePoolInstaller *sAutoreleasePoolInstaller = 0;

void initializeLFoundation(unsigned char analyseLeaks, unsigned char installPoolBridge)
{
	(void)analyseLeaks;
	if (sLFoundationIsActive)
		return;
	sLFoundationMainPool = new StAutoreleasePool;
	sLFoundationIsActive = true;
	// The seed is the fraction of the time over 10000 seconds, in millionths.
	double t = LTimer::currentTimeValue() * 0.0001;
	double seed = (t - floor(t)) * 1000000.0;
	if (seed >= 4294967295.0)
		seed = 4294967295.0;
	if (seed <= 0.0)
		seed = 0.0;
	unsigned int s = (unsigned int)seed;
	srand(s + 100);
	srandom(s);
	if (installPoolBridge)
		sAutoreleasePoolInstaller = new StAutoreleasePoolInstaller;
}

void finilizeLFoundation()
{
	if (!sLFoundationIsActive)
		return;
	if (sAutoreleasePoolInstaller) {
		delete sAutoreleasePoolInstaller;
		sAutoreleasePoolInstaller = 0;
	}
	{
		StNSAutoreleasePool nsPool;
		LMainThread *mainThread = (LMainThread *)LThread::sMainThread;
		mainThread->invalidateRunLoopInvocations();
		LUndoManager::classObject()->killSingleton();
		LCleanupTask::cleanupAllObjects();
		LConnection::finalizeConnections();
		LNotificationCenter::terminateCenter();
		LClass *defaults = LUserDefaults::classObject();
		if (defaults->_singleton) {
			((LUserDefaults *)defaults->singleton(0))->synchronize();
			defaults->killSingleton();
		}
		mainThread->terminateRunLoop();
	}
	if (sLFoundationMainPool) {
		delete sLFoundationMainPool;
		sLFoundationMainPool = 0;
	}
	sLFoundationIsActive = false;
}

} // namespace LF
