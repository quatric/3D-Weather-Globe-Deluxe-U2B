// YAException
//
// LF::YAException, bases: std::exception
//   vtable:
//      0 +0x00 LF::YAException::~YAException()
//      1 +0x04 LF::YAException::~YAException()
//      2 +0x08 std::exception::what() const
//      3 +0x0c __cxa_pure_virtual

#include "LFBase.h"
#include <stdio.h>

namespace LF {

void YAException::raise(const char *file, int line)
{
	char buf[1012];
	snprintf(buf, sizeof buf, "Exception raised: %s\n", what());
	_exclusiveShowString(buf, file, line);
	_Doraise();
}

} // namespace LF
