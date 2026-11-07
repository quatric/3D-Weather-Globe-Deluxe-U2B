// Routes: the route objects (sequences of points with timing), and the tree
// of route sets the route manager shows: the user's library, the sample
// sets, and shared and remote sets.

#ifndef AMROUTES_H
#define AMROUTES_H

#include "AMEntries.h"
#include "LTreeObject.h"

// ---------------------------------------------------------------------------
// Route objects

// What the renderers highlight: points with their colors, the lines between
// them, and shapes.
class IHighlighterDataSource {
public:
	// SRoutePoint records.
	virtual LData *pointsToHighlight() = 0;
	virtual LString *titleForPointAtIndex(unsigned int index) = 0;
	virtual float visualWeightForPointAtIndex(unsigned int index) = 0;
	// Pairs of pointsToHighlight() indices.
	virtual LData *linesIndices() = 0;
	// CShapeContainers.
	virtual LArray *shapesToHighlight() = 0;
};

struct SRoutePoint {
	LPoint geoPoint;
	Color color;
};

// A sequence of entries and breaks, with a color and a caption per entry.
class RouteObject : public LObject, public IHighlighterDataSource {
	LF_DECLARE_CLASS(RouteObject)
public:
	static LString *kEntryColorKey;
	static LString *kEntryCaptionKey;
	static LString *kFlatEntryArrayKey;
	static LString *kBreakIndiciesArrayKey;
	static LString *kRouteObjectItemCountDidChangeNotification;
	static LString *kRouteObjectAttributesDidChangeNotification;

	RouteObject();
	virtual LObject *initWithDecoder(const LCoder *coder);
	virtual void encodeWithCoder(LCoder *coder) const;
	virtual void dealloc();
	virtual RouteObject *getOwnerLObject() { return this; }
	virtual RouteObject *initWithDictionaryRep(LMap *rep, LString *name);
	virtual LMap *dictionaryRepresentation() const;
	virtual LData *pointsToHighlight();
	virtual LString *titleForPointAtIndex(unsigned int index);
	virtual float visualWeightForPointAtIndex(unsigned int index);
	virtual LData *linesIndices();
	virtual LArray *shapesToHighlight();
	virtual intptr_t onEntryDidChange(char *args);

	// A new route (retained) with the same items.
	RouteObject *copy();
	LArray *colorComponentsForItem(GenericEntry *entry) const;
	Color colorForItem(GenericEntry *entry) const;
	LString *nameForItem(GenericEntry *entry) const;
	int itemsCount() const;
	RouteSetItem *itemAtIndex(int index) const;
	bool containsItem(RouteSetItem *item);
	// The distance along the first `count` items, back to the last break.
	float distanceForSubset(int count) const;
	// From the previous item; -1 when either is not a point.
	float distanceForItemAtIndex(int index) const;
	LString *preferredName();
	LRect boundsGeoRect() const;
	float totalDistance() const;
	int locationsCount() const;

	void _reevalPointsDependedVars() const;
	void _invalidate();

	mutable bool _valid;
	mutable float _totalDistance;
	mutable LData *_points;
	mutable LData *_lineIndices;
	mutable LArray *_shapes;
	mutable LArray *_pointEntries;
	mutable LRect _bounds;
	mutable int _locationsCount;
	LString *_name;
	LArray *_items;
	LMap *_colors;
	LMap *_names;
};

class MutableRouteObject : public RouteObject {
	LF_DECLARE_CLASS(MutableRouteObject)
public:
	static LString *kItemColor;
	static LString *kItemIndex;
	static LString *kItemValue;
	static LString *kItemCaption;

	virtual MutableRouteObject *init();
	virtual MutableRouteObject *getOwnerLObject() { return this; }
	virtual MutableRouteObject *initWithDictionaryRep(LMap *rep, LString *name);

	void setNameForItem(LString *name, GenericEntry *entry);
	void setColorForItem(LArray *components, GenericEntry *entry);
	void setColorForItem(Color color, GenericEntry *entry);
	void insertItemAtIndex(RouteSetItem *item, int index, LString *name, LArray *color);
	void removeItemAtIndex(int index);
	// At `index`, or at the end when -1, after a break.
	void appendRouteObject(RouteObject *route, int index);
	// Maps of kItemValue, kItemIndex, kItemCaption and kItemColor.
	void insertItemsWithOrderedIndexes(TLArray<LMap *> *items);
	void removeItemsWithOrderedIndexes(TLArray<LValue *> *indexes);
	void setColorForItems(LArray *components, TLArray<GenericEntry *> *entries);
	void setColorsForItems(TLArray<LMap *> *items);
	void moveItemsFromIndexesToIndex(TLArray<LValue *> *indexes, int *index);
	void moveItemsStartingAtIndexToIndexes(int *index, TLArray<LValue *> *indexes);
	void removeAllItems();
	void replaceItemsWithRouteObject(RouteObject *route);

protected:
	void _setNameForItem(LString *name, GenericEntry *entry);
	void _setColorForItem(LArray *components, GenericEntry *entry);
	bool _insertItemAtIndex(RouteSetItem *item, int index, LString *name, LArray *color);
	void _removeItemAtIndex(int index);
};

// ---------------------------------------------------------------------------
// Route nodes

// A node of the route set tree: a named folder, or the node of a route entry.
class GenericRouteNode : public LTreeObject {
	LF_DECLARE_CLASS(GenericRouteNode)
public:
	GenericRouteNode();
	virtual void dealloc();
	virtual GenericRouteNode *getOwnerLObject() { return this; }
	virtual GenericRouteNode *initWithName(LString *name, unsigned char editable);
	virtual GenericRouteNode *initWithEntry(RouteObjectEntry *entry);

	// The entry's name for entry nodes.
	LString *name() const;
	RouteObjectEntry *entry() const { return _entry; }
	bool isEditable() const { return _editable; }
	// True when no node below has an entry.
	bool isEmpty();
	bool isDescendantOfNode(GenericRouteNode *node);
	bool isDescendantOfNodeInArray(TLArray<GenericRouteNode *> *nodes);

	LString *_name;
	RouteObjectEntry *_entry;
	bool _editable;
};

// A node stored in the route nodes table.
class DBRouteNode : public GenericRouteNode {
	LF_DECLARE_CLASS(DBRouteNode)
public:
	DBRouteNode();
	virtual void dealloc();
	virtual DBRouteNode *getOwnerLObject() { return this; }
	virtual DBRouteNode *initWithDBMap(LMap *map);
	virtual LClass *_classForChildren() const;

	LString *uid() const { return _uid; }
	LMap *dbMap();
	void _createChildren();

	LString *_uid;
};

// A node of the user's database.
class DBMutableRouteNode : public DBRouteNode {
	LF_DECLARE_CLASS(DBMutableRouteNode)
public:
	virtual DBMutableRouteNode *getOwnerLObject() { return this; }
};

class DBUserRouteNode : public DBMutableRouteNode {
	LF_DECLARE_CLASS(DBUserRouteNode)
public:
	virtual DBUserRouteNode *getOwnerLObject() { return this; }
	using GenericRouteNode::initWithName;
	using GenericRouteNode::initWithEntry;
	virtual DBUserRouteNode *initWithName(LString *name);
	virtual DBUserRouteNode *initWithEntry(UserRouteObjectEntry *entry);
	virtual void setName(LString *name);
};

// The root of the user's library.
class UserLibRouteNode : public DBMutableRouteNode {
	LF_DECLARE_CLASS(UserLibRouteNode)
public:
	virtual UserLibRouteNode *getOwnerLObject() { return this; }
	virtual LClass *_classForChildren() const;
	using GenericRouteNode::initWithName;
	virtual UserLibRouteNode *initWithName(LString *name);
};

// The root of the sample sets of the atlas.
class StaticLibRouteNode : public DBRouteNode {
	LF_DECLARE_CLASS(StaticLibRouteNode)
public:
	virtual StaticLibRouteNode *getOwnerLObject() { return this; }
	virtual LClass *_classForChildren() const;
	using GenericRouteNode::initWithName;
	virtual StaticLibRouteNode *initWithName(LString *name);
};

class SharedLibRouteNode : public GenericRouteNode {
	LF_DECLARE_CLASS(SharedLibRouteNode)
public:
	virtual void dealloc();
	virtual SharedLibRouteNode *getOwnerLObject() { return this; }
	using GenericRouteNode::initWithName;
	virtual SharedLibRouteNode *initWithName(LString *name);
};

class RemoteRouteNode : public GenericRouteNode {
	LF_DECLARE_CLASS(RemoteRouteNode)
public:
	virtual void dealloc();
	virtual RemoteRouteNode *getOwnerLObject() { return this; }
};

#endif
