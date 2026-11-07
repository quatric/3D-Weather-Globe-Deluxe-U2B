// LTreeObject: an LObject with an ordered list of retained children and a
// (weak) parent. The links are kept by a TLTreeObjectImp node owned by the
// object; changes post kChildrenDidChangeNotification if _postsNotifications
// is set.
#pragma once

#include "LString.h"

namespace LF {

class LTreeObject;

// A tree node: its children nodes and its parent. F::add and F::remove are
// called on a node as it is put into and taken out of a parent.
template <typename T, typename F>
class TTreeObject {
public:
	TTreeObject() : _parent(0) {}
	virtual ~TTreeObject()
	{
		for (int i = 0; i < (int)_children.count(); i++)
			F::remove(_children[i]);
	}

	T *parent() const { return _parent; }
	unsigned int childrenCount() const { return _children.count(); }
	T *childAtIndex(int index) const { return _children[index]; }
	const T *topObject() const
	{
		const T *node = static_cast<const T *>(this);
		while (node->_parent)
			node = node->_parent;
		return node;
	}

	void addChild(T *child)
	{
		F::add(child);
		if (child->_parent)
			child->_parent->removeChild(child);
		_children.addObject(child);
		child->_parent = static_cast<T *>(this);
	}
	void insertChildAtIndex(T *child, long index)
	{
		F::add(child);
		if (child->_parent)
			child->_parent->removeChild(child);
		if (index < 1)
			_children.insertObjectAtIndex(child, 0);
		else if (index >= (long)_children.count())
			_children.addObject(child);
		else
			_children.insertObjectAtIndex(child, (int)index);
		child->_parent = static_cast<T *>(this);
	}
	void removeChild(T *child)
	{
		int i = _children.indexOfObject(child);
		if (i >= 0) {
			_children.removeObjectAtIndex(i);
			child->_parent = 0;
			F::remove(child);
		}
	}
	void removeChildAtIndex(long index)
	{
		T *child = _children[(int)index];
		_children.removeObjectAtIndex((int)index);
		child->_parent = 0;
		F::remove(child);
	}
	void removeAllChildren()
	{
		unsigned int n = _children.count();
		for (unsigned int i = 0; i < n; i++) {
			T *child = _children[i];
			child->_parent = 0;
			F::remove(child);
		}
		_children.removeAllObjects();
	}

	TArray<T *> _children;
	T *_parent;
};

class TLTreeObjectImp;

// A node holds a reference on its LTreeObject while it has a parent.
struct LTreeObjectAddRemoveFunc {
	static void add(TLTreeObjectImp *node);
	static void remove(TLTreeObjectImp *node);
};

class TLTreeObjectImp : public TTreeObject<TLTreeObjectImp, LTreeObjectAddRemoveFunc> {
public:
	TLTreeObjectImp(LTreeObject *owner) : _owner(owner) {}
	virtual ~TLTreeObjectImp() {}

	LTreeObject *_owner;
};

class LTreeObject : public LObject {
	LF_DECLARE_CLASS(LTreeObject)
public:
	static LString *kChildrenDidChangeNotification;

	LTreeObject();
	virtual ~LTreeObject() {}
	virtual LObject *initWithDecoder(const LCoder *coder);
	virtual void encodeWithCoder(LCoder *coder) const;
	virtual void dealloc();
	virtual LTreeObject *getOwnerLObject() { return this; }
	// Called after the children change.
	virtual void _childrenArrayWasChanged() {}
	// Called after the object moves from one parent to another.
	virtual void _parentObjectWasChanged(LTreeObject *oldParent, LTreeObject *newParent) {}

	LTreeObject *superObject() const { return _imp->_parent ? _imp->_parent->_owner : 0; }
	LTreeObject *topObject() const { return _imp->topObject()->_owner; }
	unsigned int childrenCount() const { return _imp->childrenCount(); }
	LTreeObject *childAtIndex(unsigned int index) const { return _imp->childAtIndex(index)->_owner; }
	void addChild(LTreeObject *child);
	void insertChildAtIndex(LTreeObject *child, long index);
	void removeChild(LTreeObject *child);
	void removeChildAtIndex(long index);
	void removeAllChildren();

	void _childrenChanged();

	TLTreeObjectImp *_imp;
	bool _postsNotifications;
};

inline void LTreeObjectAddRemoveFunc::add(TLTreeObjectImp *node)
{
	node->_owner->retain();
}

inline void LTreeObjectAddRemoveFunc::remove(TLTreeObjectImp *node)
{
	node->_owner->release();
}

} // namespace LF
