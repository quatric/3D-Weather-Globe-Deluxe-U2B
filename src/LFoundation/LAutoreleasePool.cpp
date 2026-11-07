// LAutoreleasePool
//
// LF::LAutoreleasePool: each thread has a stack of pools and a free list of
// spare ones. Removing the last pool frees them all (and logs, while
// LFoundation is active).

#include "LString.h"
#include "LRunLoop.h"

namespace LF {

extern bool sLFoundationIsActive;

namespace {

struct SPoolThreadData {
	LAutoreleasePool *top;
	LAutoreleasePool *free;	// spare pools, linked by _below
};

pthread_key_t sArPoolKey;
pthread_once_t sArPoolKeyOnce = PTHREAD_ONCE_INIT;

void _createKey()
{
	if (pthread_key_create(&sArPoolKey, 0) != 0)
		LF_RAISE(WrongConditionException());
}

SPoolThreadData *autoreleasePoolThreadData()
{
	pthread_once(&sArPoolKeyOnce, _createKey);
	return (SPoolThreadData *)pthread_getspecific(sArPoolKey);
}

} // namespace

LAutoreleasePool *LAutoreleasePool::currentPool()
{
	SPoolThreadData *data = autoreleasePoolThreadData();
	return data ? data->top : 0;
}

int LAutoreleasePool::releaseAllObjects()
{
	int n = 0;
	if (_objects) {
		for (; n < (int)_objects->count(); n++)
			(*_objects)[n]->release();
		_objects->removeAllObjects(0);
	}
	return n;
}

void LAutoreleasePool::addObject(LObject *obj)
{
	if (_objects == 0)
		_objects = new TArray<LObject *>;
	_objects->addObject(obj);
}

void LAutoreleasePool::addNewPoolToTop()
{
	SPoolThreadData *data = autoreleasePoolThreadData();
	if (data == 0) {
		data = new SPoolThreadData;
		pthread_setspecific(sArPoolKey, data);
		data->free = 0;
		data->top = 0;
	}
	LAutoreleasePool *pool = data->free;
	if (pool == 0) {
		pool = new LAutoreleasePool;
		pool->_objects = 0;
		pool->_below = 0;
		pool->_above = 0;
	} else {
		LAutoreleasePool *next = pool->_below;
		if (next)
			next->_above = 0;
		data->free = next;
	}
	pool->_above = 0;
	pool->_below = data->top;
	if (data->top)
		data->top->_above = pool;
	data->top = pool;
}

void LAutoreleasePool::removeTopPool()
{
	SPoolThreadData *data = autoreleasePoolThreadData();
	if (data == 0 || data->top == 0)
		return;
	LAutoreleasePool *pool = data->top;
	int released = pool->releaseAllObjects();
	LAutoreleasePool *below = pool->_below;
	if (below)
		below->_above = 0;
	data->top = below;
	if (data->free)
		data->free->_above = pool;
	pool->_below = data->free;
	pool->_above = 0;
	data->free = pool;
	if (data->top == 0) {
		int chain = 0;
		do {
			LAutoreleasePool *next = pool->_below;
			pool->releaseAllObjects();
			delete pool->_objects;
			delete pool;
			chain++;
			pool = next;
		} while (pool);
		pthread_setspecific(sArPoolKey, 0);
		delete data;
		if (sLFoundationIsActive) {
			const char *which = pthread_equal(pthread_self(), LThread::sMainThread->_thread) ? "(MAIN THREAD):"
			                                                                                : "(HELPER THREAD):";
			LLog("%s Top Pool released; number of objects in top pool:%d longest chain length:%d\n", which,
			     released, chain);
		}
	}
}

} // namespace LF
