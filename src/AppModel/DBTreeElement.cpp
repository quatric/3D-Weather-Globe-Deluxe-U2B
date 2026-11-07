// DBTreeElement: a tree of keyed values stored as rows of a table (uid,
// parent uid, key, value, value type, sort index), loaded on first use.
// DBTreeRootElement names the table and its columns; InfoTreeRootElement is
// the tree of an entry's info items, in the atlas or the user's database.
//
// Value types: 'p' a subtree, 's' a string, 't' a long text kept as a
// TextBLOB, 'd' a data BLOB.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(DBTreeElement, 0)
LF_IMPLEMENT_CLASS(DBTreeRootElement, LF_CLASSOF(DBTreeElement))
LF_IMPLEMENT_CLASS(InfoTreeRootElement, LF_CLASSOF(DBTreeRootElement))

// ---------------------------------------------------------------------------
// DBTreeElement

DBTreeElement::DBTreeElement() : _uid(0), _parent(0), _root(0), _values(0), _keys(0), _loaded(false)
{
}

void DBTreeElement::dealloc()
{
	if (_uid) {
		_uid->release();
		_uid = 0;
	}
	if (_values) {
		_values->release();
		_values = 0;
	}
	if (_keys) {
		_keys->release();
		_keys = 0;
	}
	LObject::dealloc();
}

DBTreeElement *DBTreeElement::initWithUIDParent(LString *uid, DBTreeElement *parent)
{
	if (!LObject::init())
		return 0;
	_uid = LRetain(uid);
	_parent = parent;
	return this;
}

DBTreeRootElement *DBTreeElement::_rootElement()
{
	if (_root == 0) {
		for (DBTreeElement *e = this; e; e = e->_parent)
			if (e->_parent == 0 && lobj_cast<DBTreeRootElement>(e)) {
				_root = (DBTreeRootElement *)e;
				break;
			}
	}
	return _root;
}

static void _markDirty(DBTreeRootElement *root)
{
	if (root)
		root->_dirty = true;
}

// Reads the element's rows: subtrees, strings, and BLOBs by UID.
void DBTreeElement::_loadKeysIfNeeded()
{
	if (_loaded)
		return;
	_loaded = true;
	if (_values) {
		_values->release();
		_values = 0;
	}
	if (_keys) {
		_keys->release();
		_keys = 0;
	}
	DBTreeRootElement *root = _rootElement();
	if (root == 0)
		return;
	LString *request = SQL_DB_Strings::selectUIDKeyValKindForParent(
	    _uid, root->tableName(), root->parentUIDColumnName(), root->uidColumnName(), root->keyColumnName(),
	    root->valColumnName(), root->valTypeColumnName(), root->sortIdxColumnName());
	SQLiteResultsContainer *rows = AtlasDatabase::instance()->searchWithRequest(request, (LMap *)0);
	if (rows == 0)
		return;
	char uid[100], key[100], value[1000], kind[22];
	void *values[4] = { uid, key, value, kind };
	int types[4] = { 3, 3, 3, 3 };
	int count = rows->count();
	for (int i = 0; i < count; i++) {
		value[0] = 0;
		rows->loadValues(values, types, 4, i);
		LString *keyString = LString::_alloc()->initWithCString(key);
		if (keyString == 0)
			continue;
		if (kind[0] == 'p') {
			LString *subtreeUID = LString::_alloc()->initWithCString(uid);
			DBTreeElement *subtree = DBTreeElement::_alloc()->initWithUIDParent(subtreeUID, this);
			if (subtreeUID)
				subtreeUID->release();
			_valuesArrayForKey(keyString, 1)->addObject(subtree);
			subtree->release();
		} else if ((kind[0] == 's' || kind[0] == 't' || kind[0] == 'd') && value[0] != 0) {
			LObject *item = LString::_alloc()->initWithCString(value, (unsigned int)strlen(value), kLUTF8Encoding);
			if (kind[0] == 't' || kind[0] == 'd') {
				BLOBWrapper *blob = BLOBWrapper::wrapperWithBLOBUid((LString *)item);
				if (blob && kind[0] == 't') {
					TextBLOB *text = lobj_cast<TextBLOB>(blob);
					if (text) {
						item->release();
						item = LRetain(text->text());
					}
				} else if (blob) {
					item->release();
					item = LRetain(blob->blobData());
				}
			}
			_valuesArrayForKey(keyString, 1)->addObject(item);
			if (item)
				item->release();
		}
		keyString->release();
	}
}

LSet *DBTreeElement::allKeys()
{
	_loadKeysIfNeeded();
	return _keys;
}

LArray *DBTreeElement::valuesForKey(LString *key)
{
	_loadKeysIfNeeded();
	return _values ? (LArray *)_values->objectForKey(key) : 0;
}

LObject *DBTreeElement::firstValueForKey(LString *key)
{
	LArray *values = valuesForKey(key);
	return (values && values->count() > 0) ? values->objectAtIndex(0) : 0;
}

// The values of the key, made when `create`.
LArray *DBTreeElement::_valuesArrayForKey(LString *key, unsigned char create)
{
	if (create && _values == 0)
		_values = LMap::_alloc()->init();
	LArray *values = _values ? (LArray *)_values->objectForKey(key) : 0;
	if (create && values == 0) {
		values = LArray::_alloc()->init();
		_values->setObjectForKey(values, key);
		values->release();
		_loadKeysIfNeeded();
		if (_keys == 0)
			_keys = LComparableSet::_alloc()->init();
		_keys->addObject(key);
	}
	return values;
}

void DBTreeElement::removeValuesForKey(LString *key)
{
	if (_values == 0)
		return;
	_valuesArrayForKey(key, 1);
	_values->removeObjectForKey(key);
	_markDirty(_rootElement());
}

void DBTreeElement::removeValueForKeyAtIndex(LString *key, int index)
{
	_valuesArrayForKey(key, 1)->removeObjectAtIndex(index);
	_markDirty(_rootElement());
}

void DBTreeElement::replaceValueForKeyAtIndex(LObject *value, LString *key, int index)
{
	_valuesArrayForKey(key, 1)->replaceObjectAtIndex(value, index);
	_markDirty(_rootElement());
}

void DBTreeElement::insertValueForKeyAtIndex(LObject *value, LString *key, int index)
{
	_valuesArrayForKey(key, 1)->insertObjectAtIndex(value, index);
	_markDirty(_rootElement());
}

void DBTreeElement::addValueForKey(LObject *value, LString *key)
{
	_valuesArrayForKey(key, 1)->addObject(value);
	_markDirty(_rootElement());
}

// A new empty subtree in the key's values.
DBTreeElement *DBTreeElement::createSubtreeForKeyAtIndex(LString *key, int index)
{
	DBTreeElement *subtree = DBTreeElement::_alloc()->initWithUIDParent(AtlasDatabase::generateUID(), this);
	subtree->_loaded = true;
	insertValueForKeyAtIndex(subtree, key, index);
	subtree->release();
	return subtree;
}

// The SQL saving the element's values and subtrees, joined by ';'. Strings
// with line breaks or over 1000 bytes go to TextBLOBs; data values are not
// saved.
LString *DBTreeElement::commitRequest()
{
	DBTreeRootElement *root = _rootElement();
	if (root == 0)
		return 0;
	LString *objectUID = root->_uid;
	LArray *requests = LArray::array();
	LCharSet *lineBreaks = LCharSet::characterSetWithCharactersInCString("\r\n");
	_loadKeysIfNeeded();
	LSetIterator it(_keys, false);
	while (LString *key = (LString *)it.nextObject()) {
		LArray *values = valuesForKey(key);
		int count = values->count();
		for (int i = 0; i < count; i++) {
			LMap *row = LMap::mapWithObjectsAndKeys(_uid, kColumnNameParentUID, objectUID, kColumnNameObjectUID, key,
			                                        kColumnNameInfoKey, LValue::valueWithInt(i),
			                                        kColumnNameInfoSortIndex, 0);
			char kind = 'p';
			LObject *value = values->objectAtIndex(i);
			LString *rowUID = 0;
			LString *extraRequest = 0;
			if (LString *string = lobj_cast<LString>(value)) {
				kind = 's';
				rowUID = AtlasDatabase::generateUID();
				if (string->findFirstCharFromSet(lineBreaks, 0, 1) != (unsigned int)-1 || strlen(string->UTF8String()) > 1000) {
					kind = 't';
					TextBLOB *blob = (TextBLOB *)TextBLOB::_alloc()
					                     ->initWithParams(AtlasDatabase::generateUID(), string)
					                     ->l_autorelease();
					extraRequest = SQL_DB_Strings::commitBLOB(blob, kRepTypeEntryInfoText, objectUID);
					value = blob->uid();
				}
				row->setObjectForKey(value, kColumnNameInfoValue);
			} else if (lobj_cast<LData>(value)) {
				continue;
			} else if (DBTreeElement *subtree = lobj_cast<DBTreeElement>(value)) {
				extraRequest = subtree->commitRequest();
				if (extraRequest == 0)
					return 0;
				if (extraRequest->length() == 0)
					continue;
				rowUID = subtree->_uid;
			} else {
				continue;
			}
			if (rowUID == 0)
				continue;
			row->setObjectForKey(rowUID, kColumnNameUID);
			row->setObjectForKey(LString::stringWithCString(&kind, 1, kLMacRomanEncoding), kColumnNameInfoValueType);
			requests->addObject(SQL_DB_Strings::insertOrReplaceEntryInfoElementWithDBMap(row));
			if (extraRequest)
				requests->addObject(extraRequest);
		}
	}
	return requests->componentsJoinedByString(LString::stringWithCString(";"));
}

// ---------------------------------------------------------------------------
// DBTreeRootElement

DBTreeRootElement::DBTreeRootElement() : _dirty(false)
{
}

void DBTreeRootElement::dealloc()
{
	DBTreeElement::dealloc();
}

bool DBTreeRootElement::isDirty()
{
	return _dirty;
}

// Replaces the stored tree with the edited one.
bool DBTreeRootElement::commitChanges()
{
	if (!_dirty)
		return true;
	LString *request = commitRequest();
	if (request) {
		request = LString::stringWithFormat("%@;%@", SQL_DB_Strings::deleteEntryInfoForObjectWithUID(_uid), request);
		if (AtlasDatabase::instance()->simpleSearchWithRequest(request)) {
			_dirty = false;
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// InfoTreeRootElement

InfoTreeRootElement::InfoTreeRootElement() : _user(false)
{
}

InfoTreeRootElement *InfoTreeRootElement::initWithGeoObjectUID(LString *uid, unsigned char user)
{
	if (!initWithUIDParent(uid, 0))
		return 0;
	_user = user;
	return this;
}

// Only the user's tree is saved.
bool InfoTreeRootElement::commitChanges()
{
	return _user ? DBTreeRootElement::commitChanges() : false;
}

LString *InfoTreeRootElement::tableName()
{
	return _user ? kUserEntryInfoTableName : kEntryInfoTableName;
}

LString *InfoTreeRootElement::parentUIDColumnName()
{
	return kColumnNameParentUID;
}

LString *InfoTreeRootElement::uidColumnName()
{
	return kColumnNameUID;
}

LString *InfoTreeRootElement::keyColumnName()
{
	return kColumnNameInfoKey;
}

LString *InfoTreeRootElement::valColumnName()
{
	return kColumnNameInfoValue;
}

LString *InfoTreeRootElement::valTypeColumnName()
{
	return kColumnNameInfoValueType;
}

LString *InfoTreeRootElement::sortIdxColumnName()
{
	return kColumnNameInfoSortIndex;
}
