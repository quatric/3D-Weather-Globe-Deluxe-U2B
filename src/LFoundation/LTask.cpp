// LTask
//
// LF::LTask, LF::LTaskQueue and LF::LQueueThread.

#include "LTask.h"
#include <unistd.h>

namespace LF {

LF_IMPLEMENT_CLASS(LTask, 0)
LF_IMPLEMENT_CLASS(LTaskQueue, 0)
LF_IMPLEMENT_CLASS(LQueueThread, LF_CLASSOF(LThread))

void LTask::dealloc()
{
	if (_target) {
		_target->release();
		_target = 0;
	}
	LObject::dealloc();
}

LTask *LTask::initWithTargetAndSelector(LObject *target, const char *sel)
{
	_target = LRetain(target);
	_waitUntilDone = false;
	_selector = sel;
	return this;
}

void LTask::executeAndInvokeSelector(unsigned char inMainThread)
{
	execute();
	if (_target == 0 || _selector == 0)
		return;
	if (!inMainThread)
		_target->invokeSelector(_selector, this);
	else
		_target->invokeSelectorInMainThreadWithObjects(_selector, _waitUntilDone, true, this);
}

LTaskQueue::LTaskQueue() : _tasks(0), _terminate(false), _finishedThreads(0), _threadsCount(1), _threads(0)
{
	pthread_cond_init(&_cond, 0);
	pthread_mutex_init(&_mutex, 0);
}

void LTaskQueue::dealloc()
{
	pthread_mutex_destroy(&_mutex);
	pthread_cond_destroy(&_cond);
	{
		StSharableLocker locker(&_lock);
		if (_tasks) {
			_tasks->release();
			_tasks = 0;
		}
	}
	if (_threads) {
		delete _threads;
		_threads = 0;
	}
	LObject::dealloc();
}

void LTaskQueue::setThreadsCount(unsigned int count)
{
	if (_threads)
		LF_RAISE(WrongConditionException());
	_threadsCount = count;
}

void LTaskQueue::terminateQueue(unsigned char wait)
{
	unsigned int n = _threads->count();
	pthread_mutex_lock(&_mutex);
	_terminate = true;
	_finishedThreads = 0;
	pthread_cond_broadcast(&_cond);
	pthread_mutex_unlock(&_mutex);
	if (wait) {
		while (_finishedThreads < n)
			usleep(50000);
	}
	if (_threads) {
		delete _threads;
		_threads = 0;
	}
	release();
}

void LTaskQueue::sortQueue()
{
	StSharableLocker locker(&_lock);
	_tasks->sort();
}

void LTaskQueue::setTaskQueueComparator(CArrayComparator compare, const void *context)
{
	if (_tasks == 0)
		_tasks = LArray::_alloc()->init();
	StSharableLocker locker(&_lock);
	_tasks->setCompareFunc(compare);
	_tasks->setCompareContext(context);
}

void LTaskQueue::setQueueSorted(unsigned char sorted)
{
	if (_tasks == 0)
		_tasks = LArray::_alloc()->init();
	StSharableLocker locker(&_lock);
	_tasks->setKeepsSorted(sorted);
	if (sorted)
		_tasks->sort();
}

void LTaskQueue::postTaskToQueue(LTask *task)
{
	{
		StSharableLocker locker(&_lock);
		if (_tasks == 0)
			_tasks = LRetain(LArray::array());
		_tasks->addObject(task);
	}
	pthread_mutex_lock(&_mutex);
	pthread_cond_signal(&_cond);
	pthread_mutex_unlock(&_mutex);
}

void LTaskQueue::removeTaskFromQueue(LTask *task)
{
	StSharableLocker locker(&_lock);
	_tasks->removeObject(task);
}

// (As in the original, the element after a removed one is skipped.)
void LTaskQueue::removeTaskWithID(int taskID, unsigned char firstOnly)
{
	if (_tasks == 0)
		return;
	StSharableLocker locker(&_lock);
	int n = _tasks->count();
	for (int i = 0; i < n; i++) {
		if (((LTask *)_tasks->objectAtIndex(i))->_taskID == taskID) {
			_tasks->removeObjectAtIndex(i);
			if (firstOnly)
				break;
			n--;
		}
	}
}

bool LTaskQueue::executeTaskFromQueue()
{
	LTask *task;
	{
		StSharableLocker locker(&_lock);
		if (_tasks == 0 || _tasks->count() < 1 || (task = (LTask *)_tasks->objectAtIndex(0)) == 0)
			return false;
		task->retain();
		_tasks->removeObjectAtIndex(0);
	}
	// The reference goes to the main-thread invocation, which releases its
	// objects when it's done (posted with dontRetain). Without one, release it
	// here (the original leaked it). Read before: the main thread may free the
	// task as soon as the invocation is posted.
	bool posted = task->_target != 0 && task->_selector != 0;
	task->executeAndInvokeSelector(1);
	if (!posted)
		task->release();
	StSharableLocker locker(&_lock);
	return _tasks->count() > 0;
}

void LTaskQueue::runQueue()
{
	if (_threads)
		return;
	_threads = new TArray<LQueueThread *>;
	for (unsigned int i = 0; i < _threadsCount; i++) {
		LQueueThread *thread = LQueueThread::_alloc();
		if (thread->LObject::init()) {
			thread->_queue = this;
			thread->_reserved = 0;
		} else
			thread = 0;
		if (thread) {
			_threads->addObject(thread);
			thread->_executeThread();
			thread->release();
		}
	}
	retain();
}

// Waits for tasks and runs them until the queue terminates.
intptr_t LQueueThread::run()
{
	LTaskQueue *queue = _queue;
	if (queue == 0)
		return 0;
	StAutoreleasePool pool;
	unsigned int n;
	{
		StSharableLocker locker(&queue->_lock);
		n = queue->_tasks ? queue->_tasks->count() : 0;
	}
	pthread_mutex_lock(&queue->_mutex);
	do {
		if (n == 0)
			pthread_cond_wait(&queue->_cond, &queue->_mutex);
		if (queue->_terminate)
			break;
		for (;;) {
			pthread_mutex_unlock(&queue->_mutex);
			{
				StNSAutoreleasePool nsPool;
				queue->executeTaskFromQueue();
			}
			pthread_mutex_lock(&queue->_mutex);
			StSharableLocker locker(&queue->_lock);
			n = queue->_tasks ? queue->_tasks->count() : 0;
			if (queue->_terminate || n == 0)
				break;
		}
	} while (!queue->_terminate);
	queue->_finishedThreads++;
	pthread_mutex_unlock(&queue->_mutex);
	return 0;
}

} // namespace LF
