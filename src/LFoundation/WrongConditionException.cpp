// WrongConditionException
//
// LF::WrongConditionException, bases: LF::YAException
//   vtable:
//      0 +0x00 LF::WrongConditionException::~WrongConditionException()
//      1 +0x04 LF::WrongConditionException::~WrongConditionException()
//      2 +0x08 std::exception::what() const
//      3 +0x0c LF::WrongConditionException::_Doraise() const

#include "LFBase.h"

namespace LF {

void WrongConditionException::_Doraise() const
{
	throw WrongConditionException(*this);
}

} // namespace LF
