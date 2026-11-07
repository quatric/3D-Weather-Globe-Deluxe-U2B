// Threads and their run loops: LThread (a pthread with an LRunLoop made on
// first use), LMainThread (the thread that ran static initialisation), the
// LRunLoop that queues invocations and timer firings onto a CFRunLoop, and
// LTimer.
#pragma once

#include "LObject.h"
#include <CoreFoundation/CoreFoundation.h>
#include <pthread.h>

namespace LF {

class LRunLoop;
class LUndoManager;

class LThread : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(LThread)
public:
	// Delivers run-loop messages for threads that have no CFRunLoop source
	// of their own (none do in practice).
	class IMessagePoster {
	public:
		virtual bool postThreadMessage(LThread *thread, unsigned int msg, uintptr_t param1, uintptr_t param2) = 0;
		virtual ~IMessagePoster() {}
	};

	LThread();
	virtual void dealloc();
	virtual void _executeThread();
	virtual intptr_t run() = 0;
	virtual LRunLoop *runLoop();

	static pthread_t currentThreadID() { return pthread_self(); }
	static LThread *currentThread();
	// Runs target->sel(param) on a new thread.
	static void performSingleAction(LObject *target, const char *sel, LObject *param);

	static LThread *sMainThread;

	pthread_t _thread;
	LRunLoop *_runLoop;
	IMessagePoster *_poster;

private:
	static void *_threadEnterFunc(void *param);
};

class LMainThread : public LThread {
public:
	LMainThread();
	virtual void dealloc() {}
	virtual intptr_t run() { return 0; }
	virtual LRunLoop *runLoop() { return LThread::runLoop(); }
	void terminateRunLoop();
	void invalidateRunLoopInvocations();
	void terminate() {}
};

} // namespace LF

// The thread LThread::performSingleAction() starts.
class LSingleActionThread : public LF::LThread {
	LF_DECLARE_CLASS(LSingleActionThread)
public:
	LSingleActionThread() : _target(0), _param(0), _selector(0) {}
	virtual void dealloc();
	virtual intptr_t run();
	virtual LSingleActionThread *getOwnerLObject() { return this; }
	LSingleActionThread *initWithParams(LF::LObject *target, const char *sel, LF::LObject *param);

	LF::LObject *_target;
	LF::LObject *_param;
	const char *_selector;
};

namespace LF {

class LTimer : public LObject {
	LF_DECLARE_CLASS(LTimer)
public:
	LTimer()
	    : _target(0), _selector(0), _userInfo(0), _repeats(true), _interval(1), _runLoop(0), _fireTime(0)
	{
	}
	virtual void dealloc();
	virtual LTimer *getOwnerLObject() { return this; }

	// A timer on the main run loop that calls target->sel(timer) every
	// `interval` milliseconds (once unless `repeats`). `mode` is a set of
	// LRunLoop::kTimerMode* flags (0 means kTimerModeDefault).
	static LTimer *timerWithParams(unsigned int interval, LObject *target, const char *sel, bool repeats,
	                               unsigned int mode);
	LTimer *initWithParams(unsigned int interval, LObject *target, const char *sel, bool repeats);
	void scheduleWithRunLoop(LRunLoop *runLoop, unsigned int mode);
	void invalidate();
	void fireImmediately();
	void fireFromHelperThread() {}
	void setUserInfo(LObject *info);
	LObject *userInfo() const { return _userInfo; }
	bool repeats() const { return _repeats; }
	unsigned int interval() const { return _interval; }
	double fireTime() const { return _fireTime; }

	// The time timers see: the current time, or a value updated only by
	// updateTimeValue() when setUpdateCurrentTimeValueOnRequest(true).
	static double currentTimeValue();
	static void updateTimeValue();
	static void setUpdateCurrentTimeValueOnRequest(bool onRequest);
	static bool updateCurrentTimeValueOnRequest();

	LObject *_target;
	const char *_selector;
	LObject *_userInfo;
	bool _repeats;
	unsigned int _interval;	// milliseconds
	LRunLoop *_runLoop;
	double _fireTime;
};

class LRunLoop : public LObject {
	LF_DECLARE_CLASS(LRunLoop)
public:
	enum {
		kTimerMessage = 0x113,
		kInvocationMessage = 'MTIM',
	};
	enum {
		kTimerModeDefault = 1,	// fires outside tracking loops
		kTimerModeTracking = 2,	// fires inside them too
	};
	struct SMessageInfo {
		unsigned int msg;
		uintptr_t param1;
		uintptr_t param2;
	};
	typedef TArray<SMessageInfo> MessageArray;

	LRunLoop() : _poster(0), _thread(0), _trackLoopMessages(0), _undoManager(0), _posted(0), _source(0), _cfRunLoop(0) {}
	virtual void dealloc();
	virtual LRunLoop *getOwnerLObject() { return this; }
	virtual bool processRunLoopMessage(unsigned int msg, uintptr_t param1, uintptr_t param2);
	virtual void invalidateAllPostedInvocations();
	// Queues `inv` (which must not be released by the run loop's thread
	// before this returns); waits for it to run when it was made to wait.
	virtual void postInvocation(LInvocation *inv);
	virtual void processInvocation(LInvocation *inv);

	static LRunLoop *mainThreadRunLoop() { return LThread::sMainThread->runLoop(); }

	LRunLoop *initWithMessagePoster(LThread::IMessagePoster *poster, LThread *thread);
	bool postThreadMessage(LThread *thread, unsigned int msg, uintptr_t param1, uintptr_t param2);
	void setObservingUndoManager(LUndoManager *manager) { _undoManager = manager; }
	// While tracking (a modal mouse or menu loop), messages are kept until
	// onExitTrackLoop() unless they are timers with kTimerModeTracking.
	void onEnterTrackLoop();
	void onExitTrackLoop();
	void installTimer(LTimer *timer, unsigned int mode);
	void uninstallTimer(LTimer *timer);

	LThread::IMessagePoster *_poster;
	LThread *_thread;
	MessageArray *_trackLoopMessages;
	LUndoManager *_undoManager;
	MessageArray *_posted;
	CSharableResource _postedLock;
	CFRunLoopSourceRef _source;
	CFRunLoopRef _cfRunLoop;

private:
	static void _processSignal(void *info);
	static void _processTimer(CFRunLoopTimerRef timer, void *info);
};

} // namespace LF
