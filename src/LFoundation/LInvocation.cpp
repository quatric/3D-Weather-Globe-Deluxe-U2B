// LInvocation
//
// LF::LInvocation: a target, a selector and its argument slots. Posted
// invocations own a copy of the slots and retain the target (and object
// arguments); waiting ones borrow the caller's and get a condition and mutex
// the run loop signals once they have run.

#include "LInvocation.h"

namespace LF {

LF_IMPLEMENT_CLASS(LInvocation, 0)

LInvocation *LInvocation::initWithGenericArgsNoCopy(LObject *target, const char *sel, char *args)
{
	if (init() == 0)
		return 0;
	_target = target;
	_selector = sel;
	_args = args;
	return this;
}

void LInvocation::dealloc()
{
	if (_argsRetained) {
		if (_target) {
			_target->release();
			_target = 0;
		}
		// Object arguments are released whether or not _init retained them.
		if (_signature == 0 && _objectArgs != 0 && _cond == 0)
			for (LObject **p = _objectArgs; *p; p++)
				(*p)->release();
		if (_args)
			free(_args);
	}
	if (_signature) {
		_signature->release();
		_signature = 0;
	}
	LObject::dealloc();
}

void LInvocation::setIntPtrArgumentAtIndex(intptr_t, int)
{
	if (_signature == 0)
		LF_RAISE(NilPtrException());
}

void LInvocation::setLArgumentAtIndex(LObject *, int)
{
	if (_signature == 0)
		LF_RAISE(NilPtrException());
}

void LInvocation::invokeWithTarget(LObject *target)
{
	if (target == 0)
		return;
	LMethodPtr method = target->getClass()->_funcPtrForSelector(_selector);
	if (method == 0) {
		target->forwardInvocation(this);
		return;
	}
	_result = (target->*method)(_args);
}

void LInvocation::invoke()
{
	invokeWithTarget(_target);
}

// `bytes` is the size of the argument block, or with `lobjects` it is counted
// from the nil-terminated list (terminator included). A waiting invocation
// uses the caller's block as is.
LInvocation *LInvocation::_init(LObject *target, const char *sel, char *args, unsigned int bytes, bool wait,
                                bool lobjects, bool dontRetain)
{
	if (init() == 0)
		return 0;
	if (!wait) {
		if (lobjects) {
			LObject **objects = (LObject **)args;
			for (bytes = sizeof(LObject *); objects[bytes / sizeof(LObject *) - 1] != 0; bytes += sizeof(LObject *)) {
				if (!dontRetain)
					objects[bytes / sizeof(LObject *) - 1]->retain();
				_objectArgCount++;
			}
		}
		char *copy = bytes ? (char *)malloc(bytes) : 0;
		_args = copy;
		if (copy) {
			_argsRetained = true;
			memcpy(copy, args, bytes);
			if (_objectArgCount)
				_objectArgs = (LObject **)_args;
		}
	} else {
		_args = args;
	}
	_target = target;
	_selector = sel;
	if (_argsRetained && target)
		target->retain();
	_cond = 0;
	_mutex = 0;
	if (wait) {
		_cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
		pthread_cond_init(_cond, 0);
		_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(_mutex, 0);
	}
	return this;
}

LInvocation *LInvocation::initWithLObjects(LObject *target, const char *sel, LObject *const *objects, bool wait,
                                           bool dontRetain)
{
	return _init(target, sel, (char *)objects, 0, wait, true, dontRetain);
}

LInvocation *LInvocation::initWithGenericArgs(LObject *target, const char *sel, const intptr_t *args,
                                              unsigned int argCount, bool wait)
{
	// The slots plus the 0 slot after them.
	return _init(target, sel, (char *)args, (argCount + 1) * sizeof(intptr_t), wait, false, false);
}

void LInvocation::setTarget(LObject *target)
{
	if (_argsRetained) {
		if (target)
			target->retain();
		if (_target) {
			_target->release();
			_target = 0;
		}
	}
	_target = target;
}

void LInvocation::retainArguments()
{
	if (_argsRetained)
		return;
	_argsRetained = true;
	if (_target)
		_target->retain();
	if (_signature == 0)
		for (int i = 0; i < _objectArgCount; i++)
			if (_objectArgs[i])
				_objectArgs[i]->retain();
}

} // namespace LF
