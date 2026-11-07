// LObject
//
// LF::LObject: the retain count, class lookups, key-value coding through the
// class's selectors and outlets, and invocations by selector.
// _invalidateNSWrapper and detachNSWrapper are in NSObjToLObjWrapper.mm.

#include "LObject.h"
#include "LString.h"
#include "LInvocation.h"
#include "LRunLoop.h"
#include "LValue.h"

namespace LF {

char _l_tabs[10000];
static struct _TabsInit {
	_TabsInit() { memset(_l_tabs, '\t', sizeof _l_tabs); }
} sTabsInit;

LClass *LMetaclass::getClass()
{
	return 0;
}

LObject *LMetaclass::alloc()
{
	return 0;
}

void LObject::release()
{
	if (--_retainCount < 1 && !_deallocating) {
		if (_hasPreDealloc)
			pre_dealloc();
		_deallocating = true;
		dealloc();
	}
}

void LObject::initAsSingleton(void *)
{
}

void LObject::initAsMetaclass()
{
}

void LObject::removeFromLeakerAnalyserTable()
{
}

LObject *LObject::init()
{
	return this;
}

LObject *LObject::l_autorelease()
{
	LAutoreleasePool *pool = LAutoreleasePool::currentPool();
	if (pool)
		pool->addObject(this);
	return this;
}

void LObject::pre_dealloc()
{
}

void LObject::dealloc()
{
	_invalidateNSWrapper();
	delete this;
}

const char *LObject::className() const
{
	LClass *cls = getClass();
	return cls ? cls->name() : "LObject";
}

LClass *LObject::getClass() const
{
	return 0;
}

LMetaclass *LObject::getMetaclass() const
{
	LClass *cls = getClass();
	return cls ? cls->getMetaclass() : 0;
}

bool LObject::conformsToProtocol(LProtocolID *protocol)
{
	return getClass()->implementsProtocol(protocol);
}

LProtocol *LObject::findProtocolInterface(LProtocolID *protocol)
{
	if (!conformsToProtocol(protocol))
		return 0;
	return dynamic_cast<LProtocol *>(this);
}

LObject *LObject::copy() const
{
	LObject *self = const_cast<LObject *>(this);
	self->_retainCount++;
	return self;
}

LObject *LObject::valueForUndefinedKey(LString *key)
{
	LLog("Cannot get value for key %@\n", key);
	return 0;
}

void LObject::setValueForUndefinedKey(LObject *, LString *key)
{
	LLog("Cannot set value for key %@\n", key);
}

bool LObject::isEqualTo(const LObject *other) const
{
	return this == other;
}

LObject *LObject::initWithDecoder(const LCoder *)
{
	return this;
}

void LObject::encodeWithCoder(LCoder *) const
{
}

LString *LObject::internalDescription() const
{
	return LString::stringWithFormat("%s <0x%08X>", className(), this);
}

LString *LObject::description() const
{
	return LString::stringWithFormat("%s <0x%08X>", className(), this);
}

LString *LObject::plistRepresentation(unsigned int) const
{
	return LString::stringWithFormat("<plist version=\"1.0\">\n\t%@\n</plist>", _plistRepresentation(1));
}

LString *LObject::_plistRepresentation(unsigned int) const
{
	return 0;
}

LString *LObject::fillTextRepresentation(unsigned int) const
{
	return description();
}

bool LObject::respondsToSelector(const char *sel)
{
	LClass *cls = getClass();
	return cls && cls->_funcPtrForSelector(sel) != 0;
}

void LObject::forwardInvocation(LInvocation *inv)
{
	doesNotRecognizeSelector(inv->selector());
}

void LObject::doesNotRecognizeSelector(const char *sel)
{
	LLog("%@ does not recoginze selector %s\n", this, sel);
}

LObject *LObject::methodSignatureForSelector(const char *sel)
{
	LClass *cls = getClass();
	return cls ? cls->_signatureForSelector(sel) : 0;
}

bool LObject::convertToType(const LTypeID &type, void *out)
{
	LTypeID mine(getClass());
	LObject *self = this;
	return mine.convertValue(&self, type, out);
}

bool LObject::setValueForOutlet(LObject *value, LString *key)
{
	LClass::LOutletInstaller *outlet = getClass()->_outletForKey(key);
	return outlet ? outlet->setValueOfOwner(value, this) : false;
}

bool LObject::getValueForOutlet(LString *key, const LTypeID &type, void *out)
{
	LClass::LOutletInstaller *outlet = getClass()->_outletForKey(key);
	return outlet ? outlet->getValueOfOwner(this, type, out) : false;
}

// When the class has a key-value selector for the key, nothing is set or
// returned: the original looks the selector up and stops there.
void LObject::setValueForKey(LObject *value, LString *key)
{
	const char *sel;
	LObject *signature = 0;
	if (getClass()->_setValueParamsForKey(key, &sel, &signature))
		return;
	if (setValueForOutlet(value, key))
		return;
	if (setValueForOutlet(value, LString::stringWithFormat("_%@", key)))
		return;
	setValueForUndefinedKey(value, key);
}

LObject *LObject::valueForKey(LString *key)
{
	const char *sel;
	LObject *signature = 0;
	if (getClass()->_getValueParamsForKey(key, &sel, &signature))
		return 0;
	LTypeID type(LF_CLASSOF(LValue));
	LObject *value = 0;
	if (getValueForOutlet(key, type, &value))
		return value;
	if (getValueForOutlet(LString::stringWithFormat("_%@", key), type, &value))
		return value;
	return valueForUndefinedKey(key);
}

intptr_t LObject::invokeSelectorWithArgs(const char *sel, char *args)
{
	LInvocation *inv = LInvocation::_alloc()->initWithGenericArgsNoCopy(this, sel, args);
	inv->invoke();
	intptr_t result = inv->result();
	inv->release();
	return result;
}

void LObject::_invokeInMainThread(const char *sel, bool wait, const intptr_t *args, unsigned int argCount)
{
	LRunLoop *loop = LRunLoop::mainThreadRunLoop();
	if (loop == 0)
		LF_RAISE(NilPtrException());
	LInvocation *inv = LInvocation::_alloc()->initWithGenericArgs(this, sel, args, argCount, wait);
	loop->postInvocation(inv);
	inv->release();
}

void LObject::_invokeInMainThreadWithObjects(const char *sel, bool wait, bool dontRetain, LObject *const *objects)
{
	LRunLoop *loop = LRunLoop::mainThreadRunLoop();
	if (loop == 0)
		LF_RAISE(NilPtrException());
	LInvocation *inv = LInvocation::_alloc()->initWithLObjects(this, sel, objects, wait, dontRetain);
	loop->postInvocation(inv);
	inv->release();
}

} // namespace LF
