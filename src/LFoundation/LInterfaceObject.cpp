// LInterfaceObject
//
// LInterfaceObject and LInterfaceModel.

#include "LInterfaceObject.h"

LF_IMPLEMENT_CLASS(LInterfaceObject, 0)
LF_IMPLEMENT_CLASS(LInterfaceModel, 0)

void LInterfaceObject::performAction()
{
	if (_target && _target->respondsToSelector(_action))
		_target->invokeSelector(_action, this);
}

void LInterfaceObject::performAltAction()
{
	if (_target && _target->respondsToSelector(_altAction))
		_target->invokeSelector(_altAction, this);
}
