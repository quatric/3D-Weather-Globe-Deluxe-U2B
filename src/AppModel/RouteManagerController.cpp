// RouteManagerController: the route manager's outline. Only the nodes of the
// user's library (DBMutableRouteNode) can be edited; every edit is saved to
// the database at once and registers its opposite with the undo manager.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(RouteManagerController, 0)
LF_SELECTOR(RouteManagerController, _onRefreshDatabase);
LF_SELECTOR(RouteManagerController, _setObjectValueForItem);
LF_SELECTOR(RouteManagerController, _deleteRouteNodes);
LF_SELECTOR(RouteManagerController, _restoreRouteNodes);
LF_SELECTOR(RouteManagerController, _moveRouteNodes);

static void _setUndoActionName(const char *name)
{
	RouteManagerController::undoManager()->setActionName(
	    LString::stringFromTable(name, "Undo", getAppModelModule()));
}

static void _postRoutesWereChanged(RouteManagerController *controller, LObject *root)
{
	LMap *info = LMap::mapWithObjectsAndKeys(root, RouteManagerController::kRootNodeKey, (LObject *)0);
	LNotificationCenter::center()->postNotification(RouteManagerController::kRoutesWereChangedNotification,
	                                                controller, info);
}

RouteManagerController *RouteManagerController::init()
{
	if (LObject::init() == 0)
		return 0;
	_hierarchy = LRetain(AtlasDatabase::instance()->allRoutesHierarchy());
	LNotificationCenter::center()->addObserver(this, "_onRefreshDatabase",
	                                           AtlasDatabase::kMainDatabaseChangedNotification, 0);
	return this;
}

void RouteManagerController::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	if (_hierarchy) {
		_hierarchy->release();
		_hierarchy = 0;
	}
	for (int i = 0; i < kRouteImagesCount; i++) {
		if (_images[i]) {
			_images[i]->release();
			_images[i] = 0;
		}
	}
	LObject::dealloc();
}

LUndoManager *RouteManagerController::undoManager()
{
	return LUndoManager::instance();
}

intptr_t RouteManagerController::_onRefreshDatabase(char *)
{
	if (_hierarchy) {
		_hierarchy->release();
		_hierarchy = 0;
	}
	_hierarchy = LRetain(AtlasDatabase::instance()->allRoutesHierarchy());
	LNotificationCenter::center()->postNotification(kRoutesWereChangedNotification, this, 0);
	return 0;
}

int RouteManagerController::childrenCountForItem(GenericRouteNode *item)
{
	if (item)
		return item->childrenCount();
	return _hierarchy ? _hierarchy->count() : 0;
}

GenericRouteNode *RouteManagerController::childAtIndexForItem(int index, GenericRouteNode *item)
{
	if (item)
		return (GenericRouteNode *)item->childAtIndex(index);
	return _hierarchy ? (GenericRouteNode *)_hierarchy->objectAtIndex(index) : 0;
}

LString *RouteManagerController::objectValueForItem(GenericRouteNode *item)
{
	return item ? item->name() : 0;
}

void RouteManagerController::setObjectValueForItem(LString *name, GenericRouteNode *item)
{
	invokeSelector("_setObjectValueForItem", name, item);
}

bool RouteManagerController::isItemEjectable(GenericRouteNode *item)
{
	return item && item->getClass()->isSubclassOf(LF_CLASSOF(RemoteRouteNode));
}

bool RouteManagerController::isItemCollapsable(GenericRouteNode *item)
{
	return item && item->_editable;
}

bool RouteManagerController::isItemExpandable(GenericRouteNode *item)
{
	return item && item->_entry == 0;
}

bool RouteManagerController::isItemEditable(GenericRouteNode *item)
{
	return item && item->getClass()->isSubclassOf(LF_CLASSOF(DBMutableRouteNode));
}

bool RouteManagerController::isItemObjectValueEditable(GenericRouteNode *item)
{
	return item && item->getClass()->isSubclassOf(LF_CLASSOF(DBUserRouteNode));
}

bool RouteManagerController::isItemSelectable(GenericRouteNode *item)
{
	return item && item != item->topObject();
}

void RouteManagerController::_addUIDsOfNodeDescendantsToUIDsArray(DBRouteNode *node, TLArray<LString *> *uids)
{
	if (!uids)
		return;
	int count = node ? node->childrenCount() : 0;
	for (int i = 0; i < count; i++) {
		DBRouteNode *child = lobj_cast<DBRouteNode>(node->childAtIndex(i));
		if (child) {
			uids->addObject(child->_uid);
			_addUIDsOfNodeDescendantsToUIDsArray(child, uids);
		}
	}
}

void RouteManagerController::addRouteObjectNodesContentToRouteObject(TLArray<GenericRouteNode *> *nodes,
                                                                     MutableRouteObject *route)
{
	int count = nodes ? nodes->count() : 0;
	for (int i = 0; i < count; i++) {
		GenericRouteNode *node = nodes->objectAtIndex(i);
		if (node->_entry) {
			RouteObject *nodeRoute = node->_entry->routeObject();
			if (nodeRoute)
				route->appendRouteObject(nodeRoute, -1);
		}
	}
}

bool RouteManagerController::validateDrag(TLArray<GenericRouteNode *> *items)
{
	int count = items ? items->count() : 0;
	for (int i = 0; i < count; i++) {
		GenericRouteNode *item = items->objectAtIndex(i);
		// Not the roots, nor the sample folders.
		if (!lobj_cast<DBRouteNode>(item->superObject()))
			return false;
		if (!lobj_cast<DBMutableRouteNode>(item) && item && item->_entry == 0)
			return false;
	}
	return true;
}

bool RouteManagerController::validateDrop(TLArray<GenericRouteNode *> *items, GenericRouteNode *item, int)
{
	if (!item || item->_entry != 0)
		return false;
	if (!lobj_cast<DBMutableRouteNode>(item->superObject()) && item != item->topObject())
		return false;
	return items->indexOfObject(item) == -1 && !item->isDescendantOfNodeInArray(items);
}

bool RouteManagerController::acceptDrop(TLArray<GenericRouteNode *> *items, GenericRouteNode *item, int)
{
	if (!item || !items)
		return false;
	// Onto an entry: beside it.
	if (item->_entry)
		item = (GenericRouteNode *)item->superObject();
	GenericRouteNode *first = items->count() > 0 ? items->objectAtIndex(0) : 0;
	if (lobj_cast<DBMutableRouteNode>(first)) {
		addChildrenForItem(items, item);
	} else {
		// From the sample sets: copies.
		for (int i = 0; i < (int)items->count(); i++) {
			DBRouteNode *node = lobj_cast<DBRouteNode>(items->objectAtIndex(i));
			if (node) {
				RouteObject *route = (RouteObject *)node->_entry->routeObject()->copy()->l_autorelease();
				createNewChildForItem(item, UserRouteObjectEntry::entryWithRouteObject(route), node->name());
			}
		}
	}
	return true;
}

void RouteManagerController::removeItems(TLArray<GenericRouteNode *> *items)
{
	int count = items ? items->count() : 0;
	LArray *nodes = 0;
	for (int i = 0; i < count; i++) {
		DBMutableRouteNode *node = lobj_cast<DBMutableRouteNode>(items->objectAtIndex(i));
		if (node && !node->isDescendantOfNodeInArray(items)) {
			if (!nodes)
				nodes = LArray::array();
			nodes->addObject(node);
		}
	}
	invokeSelector("_deleteRouteNodes", nodes);
}

void RouteManagerController::addChildrenForItem(TLArray<GenericRouteNode *> *items, GenericRouteNode *item)
{
	if (!lobj_cast<DBMutableRouteNode>(item))
		return;
	LMap *parents = LMap::map();
	LArray *nodes = LArray::array();
	int count = items ? items->count() : 0;
	for (int i = 0; i < count; i++) {
		DBMutableRouteNode *node = lobj_cast<DBMutableRouteNode>(items->objectAtIndex(i));
		if (node) {
			parents->setObjectForKey(item, node->_uid);
			nodes->addObject(node);
		}
	}
	invokeSelector("_moveRouteNodes", nodes, parents);
}

// Moves the nodes to the parents mapped from their UIDs.
intptr_t RouteManagerController::_moveRouteNodes(char *args)
{
	TLArray<DBRouteNode *> *nodes = LF_ARG(args, 0, TLArray<DBRouteNode *> *);
	LMap *parents = LF_ARG(args, 1, LMap *);
	LMap *previousParents = LMap::map();
	int count = nodes ? nodes->count() : 0;
	LObject *root = 0;
	for (int i = 0; i < count; i++) {
		DBRouteNode *node = nodes->objectAtIndex(i);
		DBMutableRouteNode *parent = lobj_cast<DBMutableRouteNode>(node->superObject());
		DBMutableRouteNode *newParent = parents ? lobj_cast<DBMutableRouteNode>(parents->objectForKey(node->_uid)) : 0;
		if (!root)
			root = node->topObject();
		if (parent) {
			parent->removeChild(node);
			previousParents->setObjectForKey(parent, node->_uid);
		}
		if (newParent)
			newParent->addChild(node);
		AtlasDatabase::instance()->simpleSearchWithRequest(
		    SQL_DB_Strings::setParentForRouteNodeWithUID(newParent ? newParent->_uid : 0, node->_uid));
	}
	undoManager()->registerUndo(this, "_moveRouteNodes", nodes, previousParents);
	_setUndoActionName(nodes->count() != 1 ? "Move Items" : "Move Item");
	if (root)
		_postRoutesWereChanged(this, root);
	return 0;
}

// Puts deleted nodes back under the parents mapped from their UIDs.
intptr_t RouteManagerController::_restoreRouteNodes(char *args)
{
	TLArray<DBRouteNode *> *nodes = LF_ARG(args, 0, TLArray<DBRouteNode *> *);
	LMap *parents = LF_ARG(args, 1, LMap *);
	LArray *uids = LArray::array();
	int count = nodes ? nodes->count() : 0;
	LObject *root = 0;
	for (int i = 0; i < count; i++) {
		DBRouteNode *node = nodes->objectAtIndex(i);
		uids->addObject(node->_uid);
		_addUIDsOfNodeDescendantsToUIDsArray(node, (TLArray<LString *> *)uids);
		if (!root)
			root = node->topObject();
		if (parents) {
			DBMutableRouteNode *parent = lobj_cast<DBMutableRouteNode>(parents->objectForKey(node->_uid));
			if (parent)
				parent->addChild(node);
		}
	}
	if (uids->count() == 0)
		return 0;
	undoManager()->registerUndo(this, "_deleteRouteNodes", nodes);
	_setUndoActionName(nodes->count() != 1 ? "Restore Items" : "Restore Item");
	AtlasDatabase *db = AtlasDatabase::instance();
	db->simpleSearchWithRequest(SQL_DB_Strings::restoreRouteNodesWithUIDs(uids));
	db->simpleSearchWithRequest(SQL_DB_Strings::restoreRoutesForRouteNodesWithUIDs(uids));
	if (root)
		_postRoutesWereChanged(this, root);
	return 0;
}

// The nodes and their descendants are marked deleted in the database, so
// they can be restored.
intptr_t RouteManagerController::_deleteRouteNodes(char *args)
{
	TLArray<DBRouteNode *> *nodes = LF_ARG(args, 0, TLArray<DBRouteNode *> *);
	LArray *uids = LArray::array();
	LMap *parents = LMap::map();
	int count = nodes ? nodes->count() : 0;
	LObject *root = 0;
	for (int i = 0; i < count; i++) {
		DBRouteNode *node = nodes->objectAtIndex(i);
		DBMutableRouteNode *parent = lobj_cast<DBMutableRouteNode>(node->superObject());
		if (!parent)
			continue;
		uids->addObject(node->_uid);
		_addUIDsOfNodeDescendantsToUIDsArray(node, (TLArray<LString *> *)uids);
		if (!root)
			root = node->topObject();
		parent->removeChild(node);
		parents->setObjectForKey(parent, node->_uid);
	}
	if (uids->count() == 0)
		return 0;
	undoManager()->registerUndo(this, "_restoreRouteNodes", nodes, parents);
	_setUndoActionName(nodes->count() != 1 ? "Delete Items" : "Delete Item");
	AtlasDatabase *db = AtlasDatabase::instance();
	db->simpleSearchWithRequest(SQL_DB_Strings::deleteRouteNodesWithUIDs(uids));
	db->simpleSearchWithRequest(SQL_DB_Strings::deleteRoutesForRouteNodesWithUIDs(uids));
	if (root)
		_postRoutesWereChanged(this, root);
	return 0;
}

intptr_t RouteManagerController::_setObjectValueForItem(char *args)
{
	LString *name = LF_ARG(args, 0, LString *);
	DBUserRouteNode *node = lobj_cast<DBUserRouteNode>(LF_ARG(args, 1, LObject *));
	if (!node || !name)
		return 0;
	undoManager()->registerUndo(this, "_setObjectValueForItem", node->name(), node);
	_setUndoActionName(node->_entry ? "Change Route Name" : "Change Folder Name");
	node->setName(name);
	_postRoutesWereChanged(this, node->topObject());
	return 0;
}

DBUserRouteNode *RouteManagerController::createNewChildForItem(GenericRouteNode *item, UserRouteObjectEntry *entry,
                                                               LString *name)
{
	DBMutableRouteNode *parent = lobj_cast<DBMutableRouteNode>(item);
	if (!parent || parent->_entry != 0)
		return 0;
	DBUserRouteNode *node = DBUserRouteNode::_alloc();
	if (entry)
		node = node->initWithEntry(entry);
	else
		node = node->initWithName(LString::stringFromTable("USER FOLDER", "AppModel", getAppModelModule()));
	parent->addChild(node);
	node->release();
	if (!AtlasDatabase::instance()->commitRouteNode(node)) {
		parent->removeChild(node);
		IAlertCallback::showAlertWithCode(0x1040000);
		return 0;
	}
	undoManager()->registerUndo(this, "_deleteRouteNodes",
	                            TLArray<DBMutableRouteNode *>::arrayWithObjects(node, (DBMutableRouteNode *)0));
	_setUndoActionName("Create New Item");
	if (name)
		node->setName(name);
	_postRoutesWereChanged(this, parent->topObject());
	return node;
}

LString *RouteManagerController::_imagePathForTRouteImageID(TRouteImageID id)
{
	const char *path;
	switch (id) {
	case kUserSetImage:
		path = "INTERFACE\\RouteManagerPalette\\User_Set.png";
		break;
	case kSampleSetImage:
		path = "INTERFACE\\RouteManagerPalette\\Sample_Set.png";
		break;
	case kUserFolderOpenImage:
	case kUserFolderClosedImage:
		path = "INTERFACE\\RouteManagerPalette\\User_Folder.png";
		break;
	case kSampleFolderOpenImage:
	case kSampleFolderClosedImage:
		path = "INTERFACE\\RouteManagerPalette\\Sample_Folder.png";
		break;
	default:
		return 0;
	}
	return LString::stringWithCString(path);
}

LImage *RouteManagerController::iconForItem(GenericRouteNode *item, unsigned char expanded)
{
	if (!item || item == item->topObject())
		return 0;
	bool editable = item->getClass()->isSubclassOf(LF_CLASSOF(DBMutableRouteNode));
	TRouteImageID id;
	if (item->_entry)
		id = editable ? kUserSetImage : kSampleSetImage;
	else if (editable)
		id = expanded ? kUserFolderOpenImage : kUserFolderClosedImage;
	else
		id = expanded ? kSampleFolderOpenImage : kSampleFolderClosedImage;
	if (_images[id] == 0) {
		LString *path = _imagePathForTRouteImageID(id);
		_images[id] = path ? LImage::_alloc()->initWithContentsOfFile(path) : 0;
	}
	return _images[id];
}
