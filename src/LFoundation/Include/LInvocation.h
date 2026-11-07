// LInvocation: a target, a selector and a block of arguments, invoked now or
// posted to another thread's run loop.
//
// Arguments are pointer-sized slots (intptr_t); methods reached by selector
// read them with LF_ARG(args, index, Type).
#pragma once

#include "LObject.h"
#include <pthread.h>

#define LF_ARG(args, i, Type) ((Type)(((intptr_t *)(args))[i]))

namespace LF {

class LInvocation : public LObject {
	LF_DECLARE_CLASS(LInvocation)
public:
	LInvocation()
	    : _signature(0), _target(0), _selector(0), _args(0), _argsRetained(false), _cond(0), _mutex(0),
	      _objectArgs(0), _objectArgCount(0), _result(0), _finished(false)
	{
	}
	virtual void dealloc();
	virtual LInvocation *getOwnerLObject() { return this; }

	// Uses `args` as is (it must outlive the invocation).
	LInvocation *initWithGenericArgsNoCopy(LObject *target, const char *sel, char *args);
	// Copies `argCount` slots, unless `wait`: then the caller blocks until the
	// invocation has run, so its arguments stay valid.
	LInvocation *initWithGenericArgs(LObject *target, const char *sel, const intptr_t *args,
	                                 unsigned int argCount, bool wait);
	// A nil-terminated list of LObjects, retained unless `dontRetain`.
	LInvocation *initWithLObjects(LObject *target, const char *sel, LObject *const *objects, bool wait,
	                              bool dontRetain);
	LInvocation *initWithLObjects(LObject *target, const char *sel, ...);

	void invoke();
	void invokeWithTarget(LObject *target);
	void retainArguments();
	bool argumentsRetained() const { return _argsRetained; }
	void setTarget(LObject *target);
	LObject *target() const { return _target; }
	void setSelector(const char *sel) { _selector = sel; }
	const char *selector() const { return _selector; }
	void setIntPtrArgumentAtIndex(intptr_t value, int index);
	void setLArgumentAtIndex(LObject *value, int index);
	intptr_t result() const { return _result; }

	// For a waiting caller: the run loop signals when the invocation has run.
	pthread_cond_t *waitCondition() const { return _cond; }
	pthread_mutex_t *waitMutex() const { return _mutex; }

	LInvocation *_init(LObject *target, const char *sel, char *args, unsigned int bytes, bool wait,
	                   bool lobjects, bool dontRetain);

	LObject *_signature;
	LObject *_target;
	const char *_selector;
	char *_args;
	bool _argsRetained;	// the invocation owns a copy of the arguments
	pthread_cond_t *_cond;
	pthread_mutex_t *_mutex;
	LObject **_objectArgs;
	int _objectArgCount;
	intptr_t _result;
	bool _finished;	// set by the run loop under _mutex once a waited-for invocation has run
};

} // namespace LF
