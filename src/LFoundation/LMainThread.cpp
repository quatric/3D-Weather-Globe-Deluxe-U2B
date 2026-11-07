// LMainThread
//
// LF::LMainThread: the LThread for the thread that ran static
// initialisation; its run loop is that thread's CFRunLoop.

#include "LRunLoop.h"

namespace LF {

LMainThread::LMainThread()
{
	_thread = pthread_self();
	if (runLoop())
		_runLoop->removeFromLeakerAnalyserTable();
}

void LMainThread::terminateRunLoop()
{
	if (_runLoop) {
		_runLoop->release();
		_runLoop = 0;
	}
}

void LMainThread::invalidateRunLoopInvocations()
{
	if (_runLoop)
		_runLoop->invalidateAllPostedInvocations();
}

} // namespace LF
