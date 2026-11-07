// LSet
//
// LF::LSet and LComparableSet: sets of retained objects, by identity or by
// equality; LSetIterator and _LSetEnumerator walk them.

#include "LSet.h"
#include "LArray.h"
#include "LCoder.h"

namespace LF {

LF_IMPLEMENT_CLASS(LSet, 0)
LF_IMPLEMENT_CLASS(LComparableSet, LF_CLASSOF(LSet))
LF_IMPLEMENT_CLASS(_LSetEnumerator, LF_CLASSOF(LEnumerator))

static LConstantString sKeyBegin("object=");
static LConstantString sClosePair(";\n");
static LConstantString sCloseBracket("}");

LSet *LSet::init()
{
	if (_comparable)
		_set = new TLComparableSetImp;
	else
		_set = new TLSetImp;
	return this;
}

void LSet::dealloc()
{
	if (_set) {
		if (_comparable)
			delete _cset();
		else
			delete _oset();
	}
	_set = 0;
	LObject::dealloc();
}

LObject *LSet::anyObject() const
{
	if (_comparable) {
		TLComparableSetImp::Entry *e = _cset()->firstAvailableEntry();
		return e ? (LObject *)e->key : 0;
	}
	TLSetImp::Entry *e = _oset()->firstAvailableEntry();
	return e ? e->key : 0;
}

LObject *LSet::ownObjectForObject(const LObject *obj) const
{
	if (_comparable) {
		TLComparableSetImp::Entry *e = _cset()->entryForKey((const LComparable *)obj);
		return e ? (LObject *)e->key : 0;
	}
	TLSetImp::Entry *e = _oset()->entryForKey(const_cast<LObject *>(obj));
	return e ? e->key : 0;
}

void LSet::removeAllObjects()
{
	if (_comparable)
		_cset()->removeAllObjects();
	else
		_oset()->removeAllObjects();
}

bool LSet::containsObject(const LObject *obj) const
{
	if (_comparable)
		return _cset()->containsObject((const LComparable *)obj);
	return _oset()->containsObject(const_cast<LObject *>(obj));
}

void LSet::removeObject(LObject *obj)
{
	if (_comparable)
		_cset()->removeObject((const LComparable *)obj);
	else
		_oset()->removeObject(obj);
}

void LSet::preserveCapacity(unsigned int capacity)
{
	if (_comparable)
		_cset()->reserve(capacity);
	else
		_oset()->reserve(capacity);
}

void LSet::addObject(LObject *obj)
{
	if (_comparable)
		_cset()->addObject((const LComparable *)obj);
	else
		_oset()->addObject(obj);
}

void LSet::addObjectsFromArray(const LArray *array)
{
	if (array == 0)
		return;
	unsigned int n = array->count();
	for (unsigned int i = 0; i < n; i++)
		addObject(array->objectAtIndex(i));
}

LSet *LSet::initWithArray(LArray *array)
{
	init();
	if (array)
		addObjectsFromArray(array);
	return this;
}

LSet *LSet::initWithObjectsList(LObject *first, va_list rest)
{
	init();
	if (first) {
		unsigned int n = 1;
		va_list count;
		va_copy(count, rest);
		while (va_arg(count, LObject *) != 0)
			n++;
		va_end(count);
		preserveCapacity(n);
		LObject *obj = first;
		do {
			addObject(obj);
			obj = va_arg(rest, LObject *);
		} while (obj);
	}
	return this;
}

LObject *LSet::initWithDecoder(const LCoder *coder)
{
	LCoder *c = const_cast<LCoder *>(coder);
	StReadChunk chunk(c);
	if (c->nextBlockSize() != 4)
		LF_RAISE(WrongConditionException());
	uint32_t n;
	c->readBytes(&n);
	preserveCapacity(n);
	for (unsigned int i = 0; i < n; i++) {
		LObject *obj = c->decodeObject();
		if (obj == 0)
			LF_RAISE(NilPtrException());
		addObject(obj);
	}
	return this;
}

void LSet::encodeWithCoder(LCoder *coder) const
{
	StWriteChunk chunk(coder);
	uint32_t n = count();
	coder->storeBytes(&n, 4);
	LSetIterator it(this, false);
	while (LObject *obj = it.nextObject())
		coder->encodeObject(obj);
}

LSet *LSet::initWithSet(LSet *set)
{
	if (set) {
		init();
		preserveCapacity(set->count());
		LSetIterator it(set, true);
		while (LObject *obj = it.nextObject())
			addObject(obj);
	}
	return this;
}

void LSet::unionSet(const LSet *set)
{
	if (set == 0)
		return;
	LSetIterator it(set, true);
	while (LObject *obj = it.nextObject())
		addObject(obj);
}

void LSet::minusSet(const LSet *set)
{
	if (set == 0)
		return;
	LSetIterator it(set, true);
	while (LObject *obj = it.nextObject())
		removeObject(obj);
}

static LString *_describe(const LSet *set, bool internal)
{
	LSetIterator it(set, true);
	char buf[300];
	snprintf(buf, sizeof buf, "LSet(%p); pairs count = %ld\n{ ", set, (long)it.count());
	LString *str = LString::stringWithCString(buf, 0, kLMacRomanEncoding);
	while (LObject *obj = it.nextObject()) {
		str->appendString(&sKeyBegin);
		str->appendString(internal ? obj->internalDescription() : obj->description());
		str->appendString(&sClosePair);
	}
	str->appendString(&sCloseBracket);
	return str;
}

LString *LSet::description() const
{
	return _describe(this, false);
}

LString *LSet::internalDescription() const
{
	return _describe(this, true);
}

LArray *LSet::allObjects() const
{
	LArray *a = LArray::array();
	a->preserveCapacity(count());
	LSetIterator it(this, true);
	while (LObject *obj = it.nextObject())
		a->addObject(obj);
	return a;
}

LEnumerator *LSet::objectEnumerator() const
{
	_LSetEnumerator *e = _LSetEnumerator::_alloc()->initWithContainer(const_cast<LSet *>(this));
	return e ? LAutorelease(e) : 0;
}

// The members are copied.
LSet *LSet::copy() const
{
	LSet *s = _comparable ? (LSet *)LComparableSet::_alloc() : _alloc();
	s = s->init();
	s->preserveCapacity(count());
	LSetIterator it(this, true);
	while (LObject *obj = it.nextObject()) {
		LObject *c = obj->copy();
		s->addObject(c);
		c->release();
	}
	return s;
}

// LSetIterator

LSetIterator::LSetIterator(const LSet *set, bool retain) : _comparable(set->_comparable)
{
	if (_comparable)
		_it = new LSet::TLComparableSetImp::hashTableIterator(set->_cset());
	else
		_it = new LSet::TLSetImp::hashTableIterator(set->_oset());
	_retained = retain ? LRetain(const_cast<LSet *>(set)) : 0;
	_set = set;
}

LSetIterator::~LSetIterator()
{
	if (_comparable)
		delete (LSet::TLComparableSetImp::hashTableIterator *)_it;
	else
		delete (LSet::TLSetImp::hashTableIterator *)_it;
	if (_retained)
		_retained->release();
}

LObject *LSetIterator::nextObject()
{
	if (_comparable) {
		LSet::TLComparableSetImp::hashTableIterator *it = (LSet::TLComparableSetImp::hashTableIterator *)_it;
		return it->next() ? (LObject *)it->entry()->key : 0;
	}
	LSet::TLSetImp::hashTableIterator *it = (LSet::TLSetImp::hashTableIterator *)_it;
	return it->next() ? it->entry()->key : 0;
}

void LSetIterator::toBegin()
{
	if (_comparable)
		*(LSet::TLComparableSetImp::hashTableIterator *)_it = LSet::TLComparableSetImp::hashTableIterator(_set->_cset());
	else
		*(LSet::TLSetImp::hashTableIterator *)_it = LSet::TLSetImp::hashTableIterator(_set->_oset());
}

unsigned int LSetIterator::count() const
{
	return _set->_set ? _set->count() : 0;
}

// _LSetEnumerator

_LSetEnumerator *_LSetEnumerator::initWithContainer(LObject *set)
{
	if (!set->getClass()->isSubclassOf(LF_CLASSOF(LSet))) {
		dealloc();
		return 0;
	}
	if (LEnumerator::initWithContainer(set) == 0)
		return 0;
	_iterator = new LSetIterator((LSet *)set, true);
	return this;
}

void _LSetEnumerator::dealloc()
{
	if (_iterator) {
		delete _iterator;
		_iterator = 0;
	}
	LEnumerator::dealloc();
}

} // namespace LF
