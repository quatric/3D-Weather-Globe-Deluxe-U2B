// StSharableLocker
//
// LF::StSharableLocker
//   this+offset used: 0x4 byte

#include "LFBase.h"

namespace LF {

StSharableLocker::StSharableLocker(CSharableResource *res, bool wait)
	: _res(res), _locked(false)
{
	if (res) {
		if (wait) {
			res->lock();
			_locked = true;
		} else {
			_locked = res->tryLock();
		}
	}
}

} // namespace LF
