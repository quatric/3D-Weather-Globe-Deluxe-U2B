// LNull
//
// LF::LNull.

#include "LNull.h"

namespace LF {

LNull LNull::sNull;
LNull *LNull::null = &LNull::sNull;

LString *LNull::description() const
{
	return LString::stringWithCString("<LNull>", 0, kLMacRomanEncoding);
}

} // namespace LF
