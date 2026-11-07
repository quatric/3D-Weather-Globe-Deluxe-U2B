// LSingleActionThread
//
// LSingleActionThread: runs one target->selector(param) call on its own
// thread, started by initWithParams().

#include "LRunLoop.h"
#include "LString.h"

using namespace LF;

LF_IMPLEMENT_CLASS(LSingleActionThread, LF_CLASSOF(LThread))

LSingleActionThread *LSingleActionThread::initWithParams(LObject *target, const char *sel, LObject *param)
{
	_target = LRetain(target);
	_selector = sel;
	_param = LRetain(param);
	_executeThread();
	return this;
}

intptr_t LSingleActionThread::run()
{
	LAutoreleasePool::addNewPoolToTop();
	intptr_t result = _target->invokeSelector(_selector, _param);
	LAutoreleasePool::removeTopPool();
	return result;
}

void LSingleActionThread::dealloc()
{
	if (_target) {
		_target->release();
		_target = 0;
	}
	if (_param) {
		_param->release();
		_param = 0;
	}
	LThread::dealloc();
}
