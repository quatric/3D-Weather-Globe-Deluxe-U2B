// LInterfaceObject: the base of user interface elements, which send their
// target an action (or an alternate action) with themselves as the argument.
// LInterfaceModel is the base of the models behind them.
//
// The original declares 16 more pure virtual methods after
// getOwnerLObject(); nothing in the product implements or calls them, so
// they are left out.
#pragma once

#include "LObject.h"

class LInterfaceObject : public LF::LObject {
	LF_DECLARE_ABSTRACT_CLASS(LInterfaceObject)
public:
	LInterfaceObject() : _target(0), _action(0), _altAction(0) {}
	virtual ~LInterfaceObject() {}
	virtual void dealloc() { LObject::dealloc(); }
	virtual LInterfaceObject *getOwnerLObject() = 0;
	virtual void setTargetAction(LF::LObject *target, const char *action)
	{
		_target = target;
		_action = action;
	}
	virtual void performAction();
	virtual void setAltAction(const char *action) { _altAction = action; }
	virtual void performAltAction();

	LF::LObject *target() { return _target; }
	const char *action() { return _action; }
	const char *altAction() { return _altAction; }

	LF::LObject *_target;	// not retained
	const char *_action;
	const char *_altAction;
};

class LInterfaceModel : public LF::LObject {
	LF_DECLARE_CLASS(LInterfaceModel)
public:
	virtual ~LInterfaceModel() {}
	virtual void dealloc() { LObject::dealloc(); }
	virtual LInterfaceModel *getOwnerLObject() { return this; }
	// Subclasses run the named action; the base does nothing.
	virtual intptr_t performAction(LF::LString *, ...) { return 0; }
};
