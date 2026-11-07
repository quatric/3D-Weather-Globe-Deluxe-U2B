// LMap
//
// LF::LMap: a hash table from retained LComparable keys to retained
// LObjects; also LMapIterator's users and _LMapEnumerator.

#include "LMap.h"
#include "LCoder.h"
#include "LData.h"
#include "LValue.h"
#include "LXMLTree.h"
#include "CLMapCreationStream.h"

namespace LF {

LF_IMPLEMENT_CLASS(LMap, 0)
LF_IMPLEMENT_CLASS(_LMapEnumerator, LF_CLASSOF(LEnumerator))

static LConstantString sKeyBegin("key=\"");
static LConstantString sValBegin("\";\nvalue=\"");
static LConstantString sClosePair(";\n");
static LConstantString sCloseBracket("}");
static LConstantString sEqualString(" = ");
static LConstantString sNewLine("\n");

static LString *_tabs(unsigned int n)
{
	return LString::stringWithCString(_l_tabs, n, kLMacRomanEncoding);
}

LMap *LMap::map()
{
	return LAutorelease(_alloc()->init());
}

LMap *LMap::mapWithObjectsAndKeys(LObject *object, const LComparable *key, ...)
{
	va_list ap;
	va_start(ap, key);
	LMap *m = _alloc()->initWithObjectsAndKeysList(object, key, ap);
	va_end(ap);
	return LAutorelease(m);
}

LMap *LMap::init()
{
	return LObject::init() ? this : 0;
}

// An XML plist (when a '<' block comes first) or an old-style "{ ... }" map.
LMap *LMap::initWithTextRepresentation(const LData *data)
{
	if (data && data->length() != 0) {
		CLMapCreationStream stream(data);
		unsigned int xml = stream.GetBlockOffsetStart('<', '>');
		unsigned int map = stream.GetBlockOffsetStart('{', '}');
		if (map != (unsigned int)-1 || xml != (unsigned int)-1) {
			if (map == (unsigned int)-1 || (xml != (unsigned int)-1 && xml <= map)) {
				LXMLTree::objectFromXMLData((const char *)data->bytes(), data->length(), this);
				return this;
			}
			int start = stream.GetBlockOffsetStart('{', '}');
			if (start != -1) {
				stream.SetMarker(start + 1);
				unsigned int end = stream.GetBlockOffsetEnd('{', '}');
				if (end != (unsigned int)-1) {
					stream.scanMap(end, this);
					return this;
				}
			}
		}
	}
	dealloc();
	return 0;
}

void LMap::dealloc()
{
	LObject::dealloc();
}

bool LMap::writeToFile(const LString *path)
{
	LString *plist = plistRepresentation(0);
	return plist && plist->writeToFile(path);
}

LObject *LMap::objectForKey(const LComparable *key) const
{
	return _imp.objectForKey(key);
}

void LMap::removeObjectForKey(const LComparable *key)
{
	if (key)
		_imp.removeEntryWithKey(key);
}

// Keys first, then values, as objects.
void LMap::encodeWithCoder(LCoder *coder) const
{
	StWriteChunk chunk(coder);
	uint32_t n = _imp.count();
	coder->storeBytes(&n, 4);
	_TLMapImp::hashTableIterator it(&_imp);
	while (it.next()) {
		coder->encodeObject(it.entry()->key);
		coder->encodeObject(it.entry()->value);
	}
}

void LMap::addEntriesFromMap(const LMap *other)
{
	if (other == 0)
		return;
	_imp.reserve(other->_imp.count() + _imp.count());
	_TLMapImp::hashTableIterator it(&other->_imp);
	while (it.next())
		_imp.setObjectForKey(it.entry()->value, it.entry()->key);
}

LMap *LMap::initWithMap(const LMap *other)
{
	addEntriesFromMap(other);
	return this;
}

LMap *LMap::initWithObjectsAndKeysList(LObject *object, const LComparable *key, va_list rest)
{
	if (object == 0)
		return this;
	unsigned int n = 1;
	va_list count;
	va_copy(count, rest);
	for (;;) {
		LObject *o = va_arg(count, LObject *);
		if (o == 0 || va_arg(count, const LComparable *) == 0)
			break;
		n++;
	}
	va_end(count);
	_imp.reserve(n);
	for (;;) {
		_imp.setObjectForKey(object, key);
		object = va_arg(rest, LObject *);
		if (object == 0)
			return this;
		key = va_arg(rest, const LComparable *);
		if (key == 0)
			return this;
	}
}

LMap *LMap::initWithObjectsAndKeys(LObject **objects, const LComparable **keys, unsigned int count)
{
	_imp.reserve(count);
	for (unsigned int i = 0; i < count; i++)
		_imp.setObjectForKey(objects[i], keys[i]);
	return this;
}

// Pairs whose key isn't an LComparable, or whose value is missing, are
// dropped.
LObject *LMap::initWithDecoder(const LCoder *coder)
{
	LCoder *c = const_cast<LCoder *>(coder);
	StReadChunk chunk(c);
	if (c->nextBlockSize() != 4)
		LF_RAISE(WrongConditionException());
	uint32_t n;
	c->readBytes(&n);
	_imp.reserve(n);
	for (unsigned int i = 0; i < n; i++) {
		LObject *key = c->decodeObject();
		if (key && !key->getClass()->isSubclassOf(LF_CLASSOF(LComparable)))
			key = 0;
		LObject *value = c->decodeObject();
		if (key && value)
			_imp.setObjectForKey(value, (const LComparable *)key);
	}
	return this;
}

bool LMap::isEqualTo(const LObject *other) const
{
	if (other == 0 || !other->getClass()->isSubclassOf(classObject()))
		return LObject::isEqualTo(other);
	const LMap *m = (const LMap *)other;
	if (_imp.count() != m->_imp.count())
		return false;
	LMapIterator it(this, true);
	while (it.next()) {
		LObject *value = m->objectForKey(it.key());
		if (value == 0 || !it.object()->isEqualTo(value))
			return false;
	}
	return true;
}

// The values are copied; the keys are shared.
LMap *LMap::copy() const
{
	LMap *m = _alloc();
	m->_imp.reserve(_imp.count());
	LMapIterator it(this, true);
	while (it.next()) {
		LObject *value = it.object()->copy();
		m->_imp.setObjectForKey(value, it.key());
		value->release();
	}
	return m;
}

LMap *LMap::initWithContentsOfFile(const LString *path)
{
	return initWithTextRepresentation(LData::dataWithContentsOfFile(path));
}

LString *LMap::_plistRepresentation(unsigned int indent) const
{
	LString *str = LString::stringWithFormat("<dict>%@", &sNewLine);
	LString *outer = _tabs(indent);
	LString *inner = _tabs(indent + 1);
	_TLMapImp::hashTableIterator it(&_imp);
	while (it.next()) {
		LString *value = it.entry()->value->_plistRepresentation(indent + 1);
		str->appendFormat("%@<key>%@</key>%@%@%@%@", inner, it.entry()->key, &sNewLine, inner, value, &sNewLine);
	}
	str->appendFormat("%@</dict>", outer);
	return str;
}

LString *LMap::description() const
{
	char buf[300];
	snprintf(buf, sizeof buf, "LMap(%p); pairs count = %ld\n{\n ", this, (long)_imp.count());
	LString *str = LString::stringWithCString(buf, 0, kLMacRomanEncoding);
	_TLMapImp::hashTableIterator it(&_imp);
	while (it.next()) {
		LString *value = it.entry()->value->description();
		str->appendFormat("\t%@ = %@;\n", it.entry()->key->description(), value);
	}
	str->appendString(&sCloseBracket);
	return str;
}

LString *LMap::fillTextRepresentation(unsigned int indent) const
{
	unsigned int inner = indent + 1;
	LString *str = LString::stringWithCString("{", 0, kLMacRomanEncoding);
	if (_imp.count() != 0)
		str->appendString(&sNewLine);
	_TLMapImp::hashTableIterator it(&_imp);
	while (it.next()) {
		str->appendString(_tabs(inner));
		str->appendString(it.entry()->key->fillTextRepresentation(inner));
		str->appendString(&sEqualString);
		str->appendString(it.entry()->value->fillTextRepresentation(inner));
		str->appendString(&sClosePair);
	}
	if (inner > 1)
		str->appendString(_tabs(indent));
	str->appendString(&sCloseBracket);
	return str;
}

LString *LMap::internalDescription() const
{
	char buf[300];
	snprintf(buf, sizeof buf, "LMap(%p); pairs count = %ld\n{ ", this, (long)_imp.count());
	LString *str = LString::stringWithCString(buf, 0, kLMacRomanEncoding);
	_TLMapImp::hashTableIterator it(&_imp);
	while (it.next()) {
		str->appendString(&sKeyBegin);
		str->appendString(it.entry()->key->internalDescription());
		str->appendString(&sValBegin);
		str->appendString(it.entry()->value->internalDescription());
		str->appendString(&sClosePair);
	}
	str->appendString(&sCloseBracket);
	return str;
}

void InsertIntValueForLSTRKey(LMap *map, int value, LString *key)
{
	map->setObjectForKey(LValue::valueWithInt(value), key);
}

void InsertIntValueForKey(LMap *map, int value, const char *key)
{
	LString *k = LString::stringWithCString(key, 0, kLMacRomanEncoding);
	map->setObjectForKey(LValue::valueWithInt(value), k);
}

LEnumerator *LMap::objectEnumerator() const
{
	_LMapEnumerator *e = _LMapEnumerator::_alloc()->initWithContainer(const_cast<LMap *>(this), false);
	return e ? LAutorelease(e) : 0;
}

LEnumerator *LMap::keyEnumerator() const
{
	_LMapEnumerator *e = _LMapEnumerator::_alloc()->initWithContainer(const_cast<LMap *>(this), true);
	return e ? LAutorelease(e) : 0;
}

// _LMapEnumerator

_LMapEnumerator *_LMapEnumerator::initWithContainer(LObject *map, bool keys)
{
	if (!map->getClass()->isSubclassOf(LF_CLASSOF(LMap))) {
		dealloc();
		return 0;
	}
	if (LEnumerator::initWithContainer(map) == 0)
		return 0;
	_iterator = new LMapIterator((LMap *)map, true);
	_keys = keys;
	return this;
}

void _LMapEnumerator::reset()
{
	_iterator->reset();
}

LObject *_LMapEnumerator::next()
{
	if (!_iterator->next())
		return 0;
	return _keys ? (LObject *)_iterator->key() : _iterator->object();
}

void _LMapEnumerator::dealloc()
{
	if (_iterator) {
		delete _iterator;
		_iterator = 0;
	}
	LEnumerator::dealloc();
}

} // namespace LF
