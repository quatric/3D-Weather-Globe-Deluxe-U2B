// LNull: the one null object, LNull::null (as NSNull). It isn't a registered
// class and is never deallocated.
#pragma once

#include "LString.h"

namespace LF {

class LNull : public LObject {
public:
	virtual ~LNull() {}
	virtual LNull *copy() const { return &sNull; }
	virtual LString *description() const;
	virtual void dealloc() {}

	static LNull sNull;
	static LNull *null;
};

} // namespace LF
