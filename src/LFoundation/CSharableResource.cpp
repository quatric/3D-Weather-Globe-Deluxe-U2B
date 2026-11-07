// CSharableResource
//
// LF::CSharableResource
//   vtable:
//      0 +0x00 LF::CSharableResource::~CSharableResource()
//      1 +0x04 LF::CSharableResource::~CSharableResource()
//      2 +0x08 LF::CSharableResource::lock()
//      3 +0x0c LF::CSharableResource::tryLock()
//      4 +0x10 LF::CSharableResource::unlock()
//   this+offset used: 0x4 int/undefined4, 0x34 int/pthread_t/undefined4

#include "LFBase.h"

namespace LF {

CSharableResource::CSharableResource()
	: _lockCount(0), _owner(0)
{
	pthread_mutex_init(&_mutex, NULL);
}

CSharableResource::~CSharableResource()
{
	pthread_mutex_destroy(&_mutex);
}

void CSharableResource::lock()
{
	if (_owner == 0 || _owner != pthread_self()) {
		pthread_mutex_lock(&_mutex);
		if (_owner == 0)
			_owner = pthread_self();
	}
	_lockCount++;
}

bool CSharableResource::tryLock()
{
	if (_owner == 0 || _owner != pthread_self()) {
		if (pthread_mutex_trylock(&_mutex) != 0)
			return false;
		if (_owner == 0)
			_owner = pthread_self();
	}
	_lockCount++;
	return true;
}

void CSharableResource::unlock()
{
	if (_owner == pthread_self() && --_lockCount == 0) {
		_owner = 0;
		pthread_mutex_unlock(&_mutex);
	}
}

} // namespace LF
