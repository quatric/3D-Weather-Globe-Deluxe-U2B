// NilPtrException
//
// LF::NilPtrException, bases: LF::YAException
//   vtable:
//      0 +0x00 LF::NilPtrException::~NilPtrException()
//      1 +0x04 LF::NilPtrException::~NilPtrException()
//      2 +0x08 std::exception::what() const
//      3 +0x0c LF::NilPtrException::_Doraise() const

#include "LFBase.h"

namespace LF {

void NilPtrException::_Doraise() const
{
	throw NilPtrException(*this);
}

} // namespace LF
