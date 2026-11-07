// FailedWithCodeException
//
// LF::FailedWithCodeException, bases: LF::YAException
//   vtable:
//      0 +0x00 LF::FailedWithCodeException::~FailedWithCodeException()
//      1 +0x04 LF::FailedWithCodeException::~FailedWithCodeException()
//      2 +0x08 LF::FailedWithCodeException::what() const
//      3 +0x0c LF::FailedWithCodeException::_Doraise() const
//   this+offset used: 0x4 int/undefined4

#include "LFBase.h"
#include <stdio.h>

namespace LF {

void FailedWithCodeException::_Doraise() const
{
	throw FailedWithCodeException(*this);
}

const char *FailedWithCodeException::what() const noexcept
{
	static char buf[64];
	snprintf(buf, sizeof buf, "operation failed with code: %d", _code);
	return buf;
}

} // namespace LF
