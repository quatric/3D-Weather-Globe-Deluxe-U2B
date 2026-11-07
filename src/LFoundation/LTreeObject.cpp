// LTreeObject
//
// LF::LTreeObject: the LObject side of the tree; TTreeObject (in the header)
// keeps the links.

#include "LTreeObject.h"
#include "LArray.h"
#include "LCoder.h"
#include "LNotification.h"

namespace LF {

LF_IMPLEMENT_CLASS(LTreeObject, 0)

static LConstantString sChildrenDidChangeNotification("ChildrenDidChangeNotification");
LString *LTreeObject::kChildrenDidChangeNotification = &sChildrenDidChangeNotification;

LTreeObject::LTreeObject() : _postsNotifications(false)
{
	_imp = new TLTreeObjectImp(this);
}

// The children outlive the object only if retained elsewhere; they are
// detached first so they don't point back at it.
void LTreeObject::dealloc()
{
	TLTreeObjectImp *imp = _imp;
	int n = imp ? imp->childrenCount() : 0;
	for (int i = 0; i < n; i++)
		imp->_children[i]->_parent = 0;
	if (imp) {
		delete imp;
		_imp = 0;
	}
	LObject::dealloc();
}

void LTreeObject::_childrenChanged()
{
	if (_postsNotifications)
		LNotificationCenter::center()->postNotification(kChildrenDidChangeNotification, this, 0);
	_childrenArrayWasChanged();
}

void LTreeObject::addChild(LTreeObject *child)
{
	LTreeObject *oldParent = child->superObject();
	_imp->addChild(child->_imp);
	child->_parentObjectWasChanged(oldParent, this);
	_childrenChanged();
}

void LTreeObject::insertChildAtIndex(LTreeObject *child, long index)
{
	LTreeObject *oldParent = child->superObject();
	_imp->insertChildAtIndex(child->_imp, index);
	child->_parentObjectWasChanged(oldParent, this);
	_childrenChanged();
}

void LTreeObject::removeChild(LTreeObject *child)
{
	LRetain(child);
	_imp->removeChild(child->_imp);
	child->_parentObjectWasChanged(this, 0);
	child->release();
	_childrenChanged();
}

void LTreeObject::removeChildAtIndex(long index)
{
	LTreeObject *child = LRetain(childAtIndex(index));
	_imp->removeChildAtIndex(index);
	child->_parentObjectWasChanged(this, 0);
	child->release();
	_childrenChanged();
}

void LTreeObject::removeAllChildren()
{
	unsigned int n = childrenCount();
	LArray *children = LArray::_alloc()->initWithCapacity(n);
	for (int i = 0; i < (int)n; i++)
		children->addObject(childAtIndex(i));
	_imp->removeAllChildren();
	for (int i = 0; i < (int)n; i++)
		((LTreeObject *)children->objectAtIndex(i))->_parentObjectWasChanged(this, 0);
	if (children)
		children->release();
	_childrenChanged();
}

LObject *LTreeObject::initWithDecoder(const LCoder *coder)
{
	LCoder *c = const_cast<LCoder *>(coder);
	StReadChunk chunk(c);
	if (c->nextBlockSize() != 4)
		LF_RAISE(WrongConditionException());
	uint32_t n;
	c->readBytes(&n);
	_imp->_children.preserveCapacity(n);
	for (unsigned int i = 0; i < n; i++) {
		LTreeObject *child = (LTreeObject *)c->decodeObject();
		if (child == 0 || !child->getClass()->isSubclassOf(classObject())) {
			LF_RAISE(NilPtrException());
			child = 0;
		}
		addChild(child);
	}
	return this;
}

void LTreeObject::encodeWithCoder(LCoder *coder) const
{
	StWriteChunk chunk(coder);
	uint32_t n = _imp->childrenCount();
	coder->storeBytes(&n, 4);
	for (unsigned int i = 0; i < _imp->childrenCount(); i++)
		coder->encodeObject(childAtIndex(i));
}

} // namespace LF
