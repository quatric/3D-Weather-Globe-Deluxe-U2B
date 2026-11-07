// LProtocolArray
//
// LF::LProtocolArray.

#include "LProtocolArray.h"
#include <stdarg.h>
#include <typeinfo>

namespace LF {

LF_IMPLEMENT_CLASS(LProtocolArray, 0)

LProtocolArray *LProtocolArray::init()
{
	return LObject::init() ? this : 0;
}

LProtocolArray *LProtocolArray::initWithArray(const LProtocolArray *other)
{
	_copyDataFromArray(other);
	return this;
}

LProtocolArray *LProtocolArray::initWithProtocolsList(LProtocol *first, va_list rest)
{
	if (first == 0)
		return this;
	unsigned int n = 1;
	va_list ap;
	va_copy(ap, rest);
	while (va_arg(ap, LProtocol *) != 0)
		n++;
	va_end(ap);
	preserveCapacity(n);
	va_copy(ap, rest);
	for (LProtocol *p = first; p != 0; p = va_arg(ap, LProtocol *))
		addObject(p);
	va_end(ap);
	return this;
}

void LProtocolArray::dealloc()
{
	_callRemoveForAllProtocols();
	LObject::dealloc();
}

LString *LProtocolArray::description() const
{
	int n = count();
	LString *str = LString::stringWithFormat("LProtocolArray(0x%08X); count = %d\n( ", (unsigned int)(uintptr_t)this, n);
	// As in the original, each line names the element type.
	for (int i = 0; i < n; i++)
		str->appendFormat("\t%s;\n", typeid(LProtocol *).name());
	str->appendString(LString::stringWithCString("};\n"));
	return str;
}

void LProtocolArray::addObject(const LProtocol *protocol)
{
	LProtocol *p = const_cast<LProtocol *>(protocol);
	p->retain_p();
	CArray::addObject(&p);
}

void LProtocolArray::_copyDataFromArray(const LProtocolArray *other)
{
	CArray::_copyDataFromArray(other);
	for (int i = 0; i < (int)count(); i++)
		protocolAtIndex(i)->retain_p();
}

void LProtocolArray::_callRemoveForAllProtocols()
{
	for (int i = 0; i < (int)count(); i++)
		protocolAtIndex(i)->release_p();
}

} // namespace LF
