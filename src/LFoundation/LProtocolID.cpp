// LProtocolID
//
// LF::LProtocolID: a protocol's name (compared on its first 64 characters),
// its hash, and its parent and child protocols.

#include "LObject.h"

namespace LF {

LProtocolID::LProtocolID(const char *name)
    : _name(name), _parentsList(true), _childrenList(false), _hash(_LCStringHash(name, 64))
{
}

// Shorter names order first; equal lengths compare as strings.
int LProtocolID::compareWith(const LProtocolID &o) const
{
	int a = (int)strlen(_name), b = (int)strlen(o._name);
	if (a < b)
		return -1;
	if (a > b)
		return 1;
	return strncmp(_name, o._name, a);
}

void LProtocolID::addChild(LProtocolID *child)
{
	_childrenList.addID(child);
}

void LProtocolID::addParent(LProtocolID *parent)
{
	_parentsList.addID(parent);
	parent->_childrenList.addID(this);
}

bool LProtocolID::isSubprotocolOf(LProtocolID *other)
{
	if (other == 0)
		return false;
	return strncmp(_name, other->_name, 64) == 0 || _parentsList.containsID(other);
}

LProtocolIDInstaller::LProtocolIDInstaller(LClass *cls, LProtocolID *protocol)
{
	cls->_addProtocolWithID(protocol);
}

} // namespace LF
