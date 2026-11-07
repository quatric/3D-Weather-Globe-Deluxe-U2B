// TextStreamException
//
// LF::TextStreamException, bases: LF::YAException
//   vtable:
//      0 +0x00 LF::TextStreamException::~TextStreamException()
//      1 +0x04 LF::TextStreamException::~TextStreamException()
//      2 +0x08 std::exception::what() const
//      3 +0x0c LF::TextStreamException::_Doraise() const
//   this+offset used: 0x4 int/undefined4

#include "LFBase.h"

namespace LF {

void TextStreamException::_Doraise() const
{
	throw TextStreamException(*this);
}

} // namespace LF
