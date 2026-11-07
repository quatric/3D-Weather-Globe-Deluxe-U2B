// LXMLTree: an XML document as a tree of LXMLNodes (name, attributes, text),
// and the XML property list reader. Both are driven by CFXMLParser through
// a __contentHandlerImp.
#pragma once

#include "LTreeObject.h"
#include <CoreFoundation/CoreFoundation.h>

namespace LF {

class LMap;
class LData;

class LXMLNode : public LObject {
	LF_DECLARE_CLASS(LXMLNode)
public:
	LXMLNode() : _name(0), _attributes(0), _text(0) {}
	virtual ~LXMLNode() {}
	virtual void dealloc();
	virtual LXMLNode *getOwnerLObject() { return this; }

	// Retained (not autoreleased).
	static LXMLNode *_nodeWithName(LString *name, LMap *attributes);

	LString *name() const { return _name; }
	LMap *attributes() const { return _attributes; }
	LString *text() const { return _text; }

	LString *_name;
	LMap *_attributes;
	LString *_text;
};

class LXMLTree : public LTreeObject {
	LF_DECLARE_CLASS(LXMLTree)
public:
	LXMLTree() : _node(0) {}
	virtual ~LXMLTree() {}
	virtual void dealloc();
	virtual LXMLTree *getOwnerLObject() { return this; }

	// The document's tree: an LXMLTree without a node, whose children are
	// the top elements. Autoreleased; 0 if the XML is malformed.
	static LXMLTree *treeFromData(const LData *data);
	// The value of an XML property list (autoreleased), or 0. The top dict or
	// array is read into `into` if it is an LMap or LArray respectively.
	static LObject *objectFromXMLData(const char *bytes, unsigned int length, LObject *into);
	// Retained (not autoreleased).
	static LXMLTree *_treeWithNode(LXMLNode *node);

	LXMLNode *node() const { return _node; }

	LXMLNode *_node;
};

// Receives CFXMLParser's callbacks. The base class builds an LXMLTree under
// _root; _current is the tree of the element being read.
class __contentHandlerImp {
public:
	__contentHandlerImp(LXMLTree *root = 0) : _root(root), _current(root) {}
	virtual ~__contentHandlerImp() {}
	virtual void *createStructure(CFXMLNodeRef node);
	virtual void addChild(void *parent, void *child) {}
	virtual void endStructure(void *xmlType) { endElement((LString *)xmlType); }
	virtual void startElement(LString *name, LMap *attributes);
	virtual void endElement(LString *name);
	virtual void appendElementBody(LString *text);

	CFStringRef _stringFromEntity(CFStringRef entity);

	LXMLTree *_root;
	LXMLTree *_current;
};

} // namespace LF
