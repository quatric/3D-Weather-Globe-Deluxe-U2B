// The nodes of the route set tree. Database nodes load their children from
// the route nodes table when made.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(GenericRouteNode, LF_CLASSOF(LTreeObject))
LF_IMPLEMENT_CLASS(DBRouteNode, LF_CLASSOF(GenericRouteNode))
LF_IMPLEMENT_CLASS(DBMutableRouteNode, LF_CLASSOF(DBRouteNode))
LF_IMPLEMENT_CLASS(DBUserRouteNode, LF_CLASSOF(DBMutableRouteNode))
LF_IMPLEMENT_CLASS(UserLibRouteNode, LF_CLASSOF(DBMutableRouteNode))
LF_IMPLEMENT_CLASS(StaticLibRouteNode, LF_CLASSOF(DBRouteNode))
LF_IMPLEMENT_CLASS(SharedLibRouteNode, LF_CLASSOF(GenericRouteNode))
LF_IMPLEMENT_CLASS(RemoteRouteNode, LF_CLASSOF(GenericRouteNode))

// ---------------------------------------------------------------------------
// GenericRouteNode

GenericRouteNode::GenericRouteNode() : _name(0), _entry(0), _editable(false)
{
}

GenericRouteNode *GenericRouteNode::initWithEntry(RouteObjectEntry *entry)
{
	if (!LObject::init())
		return 0;
	_entry = LRetain(entry);
	return this;
}

void GenericRouteNode::dealloc()
{
	if (_name) {
		_name->release();
		_name = 0;
	}
	if (_entry) {
		_entry->release();
		_entry = 0;
	}
	LTreeObject::dealloc();
}

LString *GenericRouteNode::name() const
{
	return _entry ? _entry->ownName(0) : _name;
}

bool GenericRouteNode::isEmpty()
{
	if (_entry)
		return false;
	int count = childrenCount();
	for (int i = 0; i < count; i++)
		if (!((GenericRouteNode *)childAtIndex(i))->isEmpty())
			return false;
	return true;
}

GenericRouteNode *GenericRouteNode::initWithName(LString *name, unsigned char editable)
{
	if (!LObject::init())
		return 0;
	_name = LRetain(name);
	_editable = editable;
	return this;
}

bool GenericRouteNode::isDescendantOfNode(GenericRouteNode *node)
{
	for (LTreeObject *p = superObject(); p; p = p->superObject())
		if (p == node)
			return true;
	return false;
}

bool GenericRouteNode::isDescendantOfNodeInArray(TLArray<GenericRouteNode *> *nodes)
{
	int count = nodes ? nodes->count() : 0;
	for (int i = 0; i < count; i++)
		if (isDescendantOfNode(nodes->objectAtIndex(i)))
			return true;
	return false;
}

// ---------------------------------------------------------------------------
// DBRouteNode

DBRouteNode::DBRouteNode() : _uid(0)
{
}

void DBRouteNode::dealloc()
{
	if (_uid) {
		_uid->release();
		_uid = 0;
	}
	GenericRouteNode::dealloc();
}

// Makes a node of _classForChildren() for each row whose parent is this one.
void DBRouteNode::_createChildren()
{
	removeAllChildren();
	bool user = getClass()->isSubclassOf(LF_CLASSOF(DBMutableRouteNode));
	LArray *rows =
	    AtlasDatabase::instance()->simpleSearchWithRequest(SQL_DB_Strings::selectRouteNodesWithParentUID(_uid, user));
	int count = rows ? rows->count() : 0;
	if (count == 0)
		return;
	LClass *childClass = _classForChildren();
	if (childClass == 0 || !childClass->isSubclassOf(LF_CLASSOF(DBRouteNode)))
		return;
	for (int i = 0; i < count; i++) {
		DBRouteNode *child = ((DBRouteNode *)childClass->alloc())->initWithDBMap((LMap *)rows->objectAtIndex(i));
		if (child) {
			addChild(child);
			child->release();
		}
	}
}

// A folder row has a name; an entry row the UID of a route entry.
DBRouteNode *DBRouteNode::initWithDBMap(LMap *map)
{
	LString *routeUID = (LString *)map->objectForKey(kColumnNameRouteUID);
	GenericRouteNode *node;
	if (routeUID == 0) {
		node = initWithName((LString *)map->objectForKey(kColumnNameName), 1);
	} else {
		RouteObjectEntry *entry = lobj_cast<RouteObjectEntry>(GenericEntry::entryWithUID(routeUID));
		if (entry == 0) {
			dealloc();
			return 0;
		}
		node = initWithEntry(entry);
	}
	if (node == 0)
		return 0;
	_uid = LRetain((LString *)map->objectForKey(kColumnNameUID));
	_createChildren();
	return this;
}

LClass *DBRouteNode::_classForChildren() const
{
	return const_cast<DBRouteNode *>(this)->getClass();
}

LMap *DBRouteNode::dbMap()
{
	LMap *map = LMap::mapWithObjectsAndKeys(_uid, kColumnNameUID, (LObject *)0);
	if (_name)
		map->setObjectForKey(_name, kColumnNameName);
	if (_entry)
		map->setObjectForKey(_entry->uid(0), kColumnNameRouteUID);
	DBRouteNode *parent = lobj_cast<DBRouteNode>(superObject());
	if (parent && parent->_uid)
		map->setObjectForKey(parent->_uid, kColumnNameParentUID);
	return map;
}

// ---------------------------------------------------------------------------
// DBUserRouteNode

// Renames the entry, or the folder in the database.
void DBUserRouteNode::setName(LString *name)
{
	if (name == 0)
		return;
	if (UserRouteObjectEntry *entry = lobj_cast<UserRouteObjectEntry>(_entry)) {
		entry->setOwnName(name);
		entry->commit();
		return;
	}
	name->retain();
	if (_name)
		_name->release();
	_name = name;
	AtlasDatabase::instance()->simpleSearchWithRequest(SQL_DB_Strings::setNameForRouteNodeWithUID(name, _uid));
}

DBUserRouteNode *DBUserRouteNode::initWithName(LString *name)
{
	if (!GenericRouteNode::initWithName(name, 1))
		return 0;
	_uid = LRetain(AtlasDatabase::generateUID());
	return this;
}

DBUserRouteNode *DBUserRouteNode::initWithEntry(UserRouteObjectEntry *entry)
{
	if (!GenericRouteNode::initWithEntry(entry))
		return 0;
	_uid = LRetain(AtlasDatabase::generateUID());
	return this;
}

// ---------------------------------------------------------------------------
// Library roots

UserLibRouteNode *UserLibRouteNode::initWithName(LString *name)
{
	if (!GenericRouteNode::initWithName(name, 0))
		return 0;
	_createChildren();
	return this;
}

LClass *UserLibRouteNode::_classForChildren() const
{
	return LF_CLASSOF(DBUserRouteNode);
}

StaticLibRouteNode *StaticLibRouteNode::initWithName(LString *name)
{
	if (!GenericRouteNode::initWithName(name, 0))
		return 0;
	_createChildren();
	return this;
}

LClass *StaticLibRouteNode::_classForChildren() const
{
	return LF_CLASSOF(DBRouteNode);
}

SharedLibRouteNode *SharedLibRouteNode::initWithName(LString *name)
{
	return (SharedLibRouteNode *)GenericRouteNode::initWithName(name, 1);
}

void SharedLibRouteNode::dealloc()
{
	GenericRouteNode::dealloc();
}

void RemoteRouteNode::dealloc()
{
	GenericRouteNode::dealloc();
}
