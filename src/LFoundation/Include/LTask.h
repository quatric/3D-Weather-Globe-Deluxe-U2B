// LTask: work for an LTaskQueue, which runs its tasks on a set of
// LQueueThreads in order (optionally kept sorted by a comparator). After a
// task executes, its target is sent its selector with the task.
#pragma once

#include "LRunLoop.h"
#include "LArray.h"

namespace LF {

class LTask : public LObject {
	LF_DECLARE_ABSTRACT_CLASS(LTask)
public:
	LTask() : _priority(1.0f), _taskID(0), _target(0), _selector(0), _waitUntilDone(false) {}
	virtual ~LTask() {}
	virtual void dealloc();
	virtual float priority() { return _priority; }
	virtual void setPriority(float priority) { _priority = priority; }
	virtual void execute() = 0;
	// The target's selector is invoked on the main thread if asked.
	virtual void executeAndInvokeSelector(unsigned char inMainThread);

	LTask *initWithTargetAndSelector(LObject *target, const char *sel);

	float _priority;
	int _taskID;
	LObject *_target;
	const char *_selector;
	bool _waitUntilDone;
};

class LTaskQueue;

class LQueueThread : public LThread {
	LF_DECLARE_CLASS(LQueueThread)
public:
	LQueueThread() : _queue(0), _reserved(0) {}
	virtual ~LQueueThread() {}
	virtual intptr_t run();
	virtual LQueueThread *getOwnerLObject() { return this; }

	LTaskQueue *_queue;
	int _reserved;
};

class LTaskQueue : public LObject {
	LF_DECLARE_CLASS(LTaskQueue)
public:
	LTaskQueue();
	virtual ~LTaskQueue() {}
	virtual LTaskQueue *init() { return this; }
	static LTaskQueue *detachNewQueue()
	{
		LTaskQueue *q = _alloc();
		if (q && q->init())
			return LAutorelease(q);
		return 0;
	}
	virtual void dealloc();
	virtual LTaskQueue *getOwnerLObject() { return this; }
	// Starts the threads; the queue then keeps itself until terminateQueue.
	virtual void runQueue();
	virtual void setTaskQueueComparator(CArrayComparator compare, const void *context);
	virtual void sortQueue();
	virtual void setQueueSorted(unsigned char sorted);
	virtual void postTaskToQueue(LTask *task);
	virtual void removeTaskFromQueue(LTask *task);
	// All tasks with the ID, or the first one only.
	virtual void removeTaskWithID(int taskID, unsigned char firstOnly);
	// Runs the first task; returns whether more are queued.
	virtual bool executeTaskFromQueue();

	unsigned int threadsCount() { return _threadsCount; }
	// Only before runQueue.
	void setThreadsCount(unsigned int count);
	unsigned int queuedTasksCount() const { return _tasks->count(); }
	// Stops the threads (waiting for them if asked) and releases the queue.
	void terminateQueue(unsigned char wait);

	LArray *_tasks;
	CSharableResource _lock;
	volatile bool _terminate;
	volatile unsigned int _finishedThreads;
	unsigned int _threadsCount;
	TArray<LQueueThread *> *_threads;
	pthread_cond_t _cond;
	pthread_mutex_t _mutex;
};

} // namespace LF
