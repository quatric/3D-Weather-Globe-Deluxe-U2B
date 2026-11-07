// LTimer
//
// LF::LTimer: calls target->selector(timer) from a run loop after `interval`
// milliseconds, repeatedly unless made one-shot. The time it records is
// LTimer::currentTimeValue(), which can be frozen between updateTimeValue()
// calls.

#include "LRunLoop.h"

namespace LF {

LF_IMPLEMENT_CLASS(LTimer, 0)

static bool sUpdateCurrValOnRequest;
static double sCurrentTimeValue;

bool LTimer::updateCurrentTimeValueOnRequest()
{
	return sUpdateCurrValOnRequest;
}

void LTimer::updateTimeValue()
{
	sCurrentTimeValue = CFAbsoluteTimeGetCurrent();
}

void LTimer::setUpdateCurrentTimeValueOnRequest(bool onRequest)
{
	sUpdateCurrValOnRequest = onRequest;
	sCurrentTimeValue = CFAbsoluteTimeGetCurrent();
}

double LTimer::currentTimeValue()
{
	if (!sUpdateCurrValOnRequest)
		sCurrentTimeValue = CFAbsoluteTimeGetCurrent();
	return sCurrentTimeValue;
}

void LTimer::dealloc()
{
	if (_target) {
		_target->release();
		_target = 0;
	}
	if (_userInfo) {
		_userInfo->release();
		_userInfo = 0;
	}
	LObject::dealloc();
}

void LTimer::scheduleWithRunLoop(LRunLoop *runLoop, unsigned int mode)
{
	retain();
	if (_runLoop)
		_runLoop->uninstallTimer(this);
	_runLoop = runLoop;
	if (runLoop)
		runLoop->installTimer(this, mode);
	release();
}

void LTimer::invalidate()
{
	if (_runLoop)
		scheduleWithRunLoop(0, 0);
}

LTimer *LTimer::initWithParams(unsigned int interval, LObject *target, const char *sel, bool repeats)
{
	_repeats = repeats;
	_interval = interval;
	_target = LRetain(target);
	_userInfo = 0;
	_runLoop = 0;
	_selector = sel;
	return this;
}

void LTimer::setUserInfo(LObject *info)
{
	LAssign(_userInfo, info);
}

// The timer is kept alive by the run loop's timers map; the caller gets it
// without a reference of its own.
LTimer *LTimer::timerWithParams(unsigned int interval, LObject *target, const char *sel, bool repeats,
                                unsigned int mode)
{
	LTimer *timer = _alloc()->initWithParams(interval, target, sel, repeats);
	timer->scheduleWithRunLoop(LRunLoop::mainThreadRunLoop(), mode ? mode : LRunLoop::kTimerModeDefault);
	timer->release();
	return timer;
}

void LTimer::fireImmediately()
{
	_fireTime = currentTimeValue();
	if (_target)
		_target->invokeSelector(_selector, this);
}

} // namespace LF
