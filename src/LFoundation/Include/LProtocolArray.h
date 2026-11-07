// LProtocolArray: a list of retained LProtocol interfaces.
#pragma once

#include "LString.h"
#include <stdarg.h>

namespace LF {

class LProtocolArray : public LObject, public CArray {
	LF_DECLARE_CLASS(LProtocolArray)
public:
	LProtocolArray() : CArray(sizeof(LProtocol *)) {}
	virtual ~LProtocolArray() {}
	virtual LProtocolArray *init();
	virtual LString *description() const;
	virtual void dealloc();
	virtual LProtocolArray *getOwnerLObject() { return this; }

	LProtocolArray *initWithArray(const LProtocolArray *other);
	// rest holds the protocols after the first, ending with a 0.
	LProtocolArray *initWithProtocolsList(LProtocol *first, va_list rest);
	void addObject(const LProtocol *protocol);
	LProtocol *protocolAtIndex(int index) const { return *(LProtocol **)objectAtIndex(index); }

	void _copyDataFromArray(const LProtocolArray *other);
	void _callRemoveForAllProtocols();
};

} // namespace LF
