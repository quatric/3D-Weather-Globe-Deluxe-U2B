// LXMLTree
//
// LF::LXMLTree, LF::LXMLNode, LF::__contentHandlerImp and __plistLoaderImp,
// the XML property list reader, with the CFXMLParser callbacks that drive
// them.

#include "LXMLTree.h"
#include "LArray.h"
#include "LData.h"
#include "LDate.h"
#include "LMap.h"
#include "LValue.h"

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace LF {

LF_IMPLEMENT_CLASS(LXMLNode, 0)
LF_IMPLEMENT_CLASS(LXMLTree, LF_CLASSOF(LTreeObject))

LXMLNode *LXMLNode::_nodeWithName(LString *name, LMap *attributes)
{
	LXMLNode *node = _alloc();
	if (node) {
		node->_name = LRetain(name);
		node->_attributes = LRetain(attributes);
	}
	return node;
}

void LXMLNode::dealloc()
{
	if (_name) {
		_name->release();
		_name = 0;
	}
	if (_attributes) {
		_attributes->release();
		_attributes = 0;
	}
	if (_text) {
		_text->release();
		_text = 0;
	}
	LObject::dealloc();
}

LXMLTree *LXMLTree::_treeWithNode(LXMLNode *node)
{
	LXMLTree *tree = _alloc();
	if (tree)
		tree->_node = LRetain(node);
	return tree;
}

void LXMLTree::dealloc()
{
	if (_node) {
		_node->release();
		_node = 0;
	}
	LTreeObject::dealloc();
}

// The predefined entities.
CFStringRef __contentHandlerImp::_stringFromEntity(CFStringRef entity)
{
	if (CFEqual(entity, CFSTR("lt")))
		return CFSTR("<");
	if (CFEqual(entity, CFSTR("gt")))
		return CFSTR(">");
	if (CFEqual(entity, CFSTR("amp")))
		return CFSTR("&");
	if (CFEqual(entity, CFSTR("apos")))
		return CFSTR("'");
	if (CFEqual(entity, CFSTR("quot")))
		return CFSTR("\"");
	return 0;
}

// Elements return the tree made for them (so their end is reported); text
// and entities are appended to the current element's text.
void *__contentHandlerImp::createStructure(CFXMLNodeRef node)
{
	CFXMLNodeTypeCode type = CFXMLNodeGetTypeCode(node);
	if (type == kCFXMLNodeTypeText) {
		CFStringRef text = CFXMLNodeGetString(node);
		if (CFStringGetLength(text) < 1)
			return 0;
		appendElementBody(LString::stringWithCFString(text));
		return 0;
	}
	if (type == kCFXMLNodeTypeEntityReference) {
		CFStringRef text = _stringFromEntity(CFXMLNodeGetString(node));
		if (text == 0)
			return 0;
		appendElementBody(LString::stringWithCFString(text));
		return 0;
	}
	if (type != kCFXMLNodeTypeElement)
		return 0;
	LString *name = LString::stringWithCFString(CFXMLNodeGetString(node));
	const CFXMLElementInfo *info = (const CFXMLElementInfo *)CFXMLNodeGetInfoPtr(node);
	CFIndex n = info->attributeOrder ? CFArrayGetCount(info->attributeOrder) : 0;
	LMap *attributes = 0;
	for (CFIndex i = 0; i < n; i++) {
		CFStringRef key = (CFStringRef)CFArrayGetValueAtIndex(info->attributeOrder, i);
		CFStringRef value = (CFStringRef)CFDictionaryGetValue(info->attributes, key);
		if (attributes == 0)
			attributes = LMap::map();
		attributes->setObjectForKey(LString::stringWithCFString(value), LString::stringWithCFString(key));
	}
	startElement(name, attributes);
	return _current;
}

void __contentHandlerImp::startElement(LString *name, LMap *attributes)
{
	LXMLNode *node = LXMLNode::_nodeWithName(name, attributes);
	LXMLTree *tree = LXMLTree::_treeWithNode(node);
	if (node)
		node->release();
	_current->addChild(tree);
	_current = tree;
	if (tree)
		tree->release();
}

void __contentHandlerImp::endElement(LString *)
{
	_current = (LXMLTree *)_current->superObject();
}

void __contentHandlerImp::appendElementBody(LString *text)
{
	LXMLNode *node = _current->_node;
	if (node == 0)
		return;
	if (node->_text)
		node->_text->appendString(text);
	else
		node->_text = LRetain(text);
}

} // namespace LF

using namespace LF;

// Reads a property list. createStructure returns the element's name
// (retained) for the elements it follows; endStructure makes their values.
// Text is gathered into _key (in <key>), _stringText (in <string>) or _text
// (in any other element).
class __plistLoaderImp : public __contentHandlerImp {
public:
	// A dict or array being filled.
	struct Container {
		LObject *obj;
		int kind;	// 0 = map, 1 = array
		Container *prev;
	};

	__plistLoaderImp(LObject *into)
	    : _sawPlist(false), _into(into), _rootObject(0), _key(0), _inData(false), _inKey(false), _inString(false),
	      _text(0), _stringText(0), _stack(0)
	{
	}
	virtual ~__plistLoaderImp()
	{
		if (_rootObject) {
			_rootObject->release();
			_rootObject = 0;
		}
		// The original freed only the top of the stack.
		if (_stack) {
			delete _stack;
			_stack = 0;
		}
		if (_key) {
			_key->release();
			_key = 0;
		}
		if (_text)
			CFRelease(_text);
		_text = 0;
		if (_stringText)
			CFRelease(_stringText);
		_stringText = 0;
	}
	virtual void *createStructure(CFXMLNodeRef node);
	virtual void addChild(void *parent, void *child) {}
	virtual void endStructure(void *xmlType);

	void _accumulateText(CFStringRef text);
	void _addToContainer(Container *c, LObject *obj)
	{
		if (c->kind == 0)
			((LMap *)c->obj)->setObjectForKey(obj, _key);
		else if (c->kind == 1)
			((LArray *)c->obj)->addObject(obj);
	}

	bool _sawPlist;
	LObject *_into;
	LObject *_rootObject;
	LString *_key;
	bool _inData;
	bool _inKey;
	bool _inString;
	CFMutableStringRef _text;
	CFMutableStringRef _stringText;
	Container *_stack;
};

void __plistLoaderImp::_accumulateText(CFStringRef text)
{
	if (_inKey) {
		if (_key) {
			_key->release();
			_key = 0;
		}
		_key = LString::_alloc();
		_key->setValue(text);
	} else if (_inString) {
		if (_stringText)
			CFStringAppend(_stringText, text);
		else
			_stringText = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, text);
	} else if (_inData) {
		if (_text)
			CFStringAppend(_text, text);
		else
			_text = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, text);
	}
}

void *__plistLoaderImp::createStructure(CFXMLNodeRef node)
{
	CFXMLNodeTypeCode type = CFXMLNodeGetTypeCode(node);
	if (type == kCFXMLNodeTypeText) {
		_accumulateText(CFXMLNodeGetString(node));
		return 0;
	}
	if (type == kCFXMLNodeTypeEntityReference) {
		CFStringRef text = _stringFromEntity(CFXMLNodeGetString(node));
		if (text)
			_accumulateText(text);
		return 0;
	}
	if (type != kCFXMLNodeTypeElement)
		return 0;
	CFStringRef name = CFXMLNodeGetString(node);
	if (!_sawPlist) {
		if (!CFEqual(name, CFSTR("plist")))
			return 0;
		_sawPlist = true;
	} else if (_rootObject == 0) {
		// The top container, read into _into if it is of the right class.
		int kind;
		LClass *cls;
		if (CFEqual(name, CFSTR("dict"))) {
			kind = 0;
			cls = LF_CLASSOF(LMap);
		} else if (CFEqual(name, CFSTR("array"))) {
			kind = 1;
			cls = LF_CLASSOF(LArray);
		} else
			return 0;
		if (_into == 0)
			_rootObject = kind == 0 ? (LObject *)LMap::_alloc()->init() : (LObject *)LArray::_alloc()->init();
		else {
			if (!_into->getClass()->isSubclassOf(cls))
				return 0;
			_rootObject = LRetain(_into);
		}
		Container *c = new Container;
		c->obj = _rootObject;
		c->kind = kind;
		c->prev = 0;
		_stack = c;
	} else {
		_inData = _inKey = _inString = false;
		if (CFEqual(name, CFSTR("key"))) {
			_inKey = true;
			if (_key) {
				_key->release();
				_key = 0;
			}
		} else if (CFEqual(name, CFSTR("string"))) {
			_inString = true;
			if (_stringText)
				CFRelease(_stringText);
			_stringText = 0;
		} else if (CFEqual(name, CFSTR("dict")) || CFEqual(name, CFSTR("array"))) {
			bool isMap = CFEqual(name, CFSTR("dict"));
			LObject *obj = isMap ? (LObject *)LMap::_alloc()->init() : (LObject *)LArray::_alloc()->init();
			Container *c = new Container;
			c->prev = _stack;
			c->obj = obj;
			c->kind = isMap ? 0 : 1;
			_stack = c;
			if (obj) {
				_addToContainer(_stack->prev, obj);
				obj->release();
			}
		} else {
			_inData = true;
			if (_text)
				CFRelease(_text);
			_text = 0;
		}
	}
	return (void *)CFRetain(name);
}

void __plistLoaderImp::endStructure(void *xmlType)
{
	CFStringRef name = (CFStringRef)xmlType;
	LObject *value = 0;
	if (CFEqual(name, CFSTR("key"))) {
	} else if (CFEqual(name, CFSTR("string"))) {
		if (_stringText == 0)
			value = LString::_alloc()->init();
		else {
			LString *str = LString::_alloc();
			str->setValue(_stringText);
			value = str;
			CFRelease(_stringText);
			_stringText = 0;
		}
	} else if (CFEqual(name, CFSTR("integer"))) {
		LValue *v = LValue::_alloc();
		v->setValue(_text ? (int)CFStringGetIntValue(_text) : 0);
		value = v;
	} else if (CFEqual(name, CFSTR("real"))) {
		LValue *v = LValue::_alloc();
		v->setValue(_text ? (float)CFStringGetDoubleValue(_text) : 0.0f);
		value = v;
	} else if (CFEqual(name, CFSTR("true"))) {
		LValue *v = LValue::_alloc();
		v->setValue((unsigned char)1);
		value = v;
	} else if (CFEqual(name, CFSTR("false"))) {
		LValue *v = LValue::_alloc();
		v->setValue((unsigned char)0);
		value = v;
	} else if (CFEqual(name, CFSTR("data"))) {
		// Base64 text, narrowed from UTF-16 in place.
		unsigned int n = _text ? CFStringGetLength(_text) : 0;
		LData *base64 = LData::dataWithLength(n * 2);
		if (n)
			CFStringGetCharacters(_text, CFRangeMake(0, n), (UniChar *)base64->bytes());
		const UniChar *chars = (const UniChar *)base64->bytes();
		char *bytes = (char *)base64->bytes();
		for (int i = 0; i < (int)n; i++)
			bytes[i] = (char)chars[i];
		base64->setLength(n);
		value = LData::_alloc()->initWithBase64Data(base64);
	} else if (CFEqual(name, CFSTR("date"))) {
		if (_text) {
			CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(_text) + 1, kCFStringEncodingMacRoman);
			char *text = new char[size];
			int year, month, day, hour, minute, second;
			if (CFStringGetCString(_text, text, size, kCFStringEncodingMacRoman)
			    && sscanf(text, "%d-%d-%dT%d:%d:%dZ", &year, &month, &day, &hour, &minute, &second) == 6)
				value = LDate::_alloc()->initWithTime(year, month, day, hour, minute, second, 0);
			delete[] text;
		}
	} else if (CFEqual(name, CFSTR("dict")) || CFEqual(name, CFSTR("array"))) {
		Container *c = _stack;
		_stack = c->prev;
		delete c;
	}
	if (value) {
		_addToContainer(_stack, value);
		value->release();
	}
	CFRelease(name);
	if (_text)
		CFRelease(_text);
	_text = 0;
	_inKey = false;
	_inData = false;
}

static void *createStructureCallBack(CFXMLParserRef, CFXMLNodeRef node, void *info)
{
	return ((__contentHandlerImp *)info)->createStructure(node);
}

static void addChildCallBack(CFXMLParserRef, void *parent, void *child, void *info)
{
	((__contentHandlerImp *)info)->addChild(parent, child);
}

static void endStructureCallBack(CFXMLParserRef, void *xmlType, void *info)
{
	((__contentHandlerImp *)info)->endStructure(xmlType);
}

static CFDataRef resolveEntityCallBack(CFXMLParserRef, CFXMLExternalID *, void *)
{
	return 0;
}

static Boolean handleErrorCallBack(CFXMLParserRef, CFXMLParserStatusCode, void *)
{
	return true;
}

static CFXMLParserRef _createParser(CFDataRef data, __contentHandlerImp *handler)
{
	CFXMLParserCallBacks callbacks = {0, createStructureCallBack, addChildCallBack, endStructureCallBack,
	                                  resolveEntityCallBack, handleErrorCallBack};
	CFXMLParserContext context = {0, handler, 0, 0, 0};
	return CFXMLParserCreate(kCFAllocatorDefault, data, 0, kCFXMLParserAllOptions, kCFXMLNodeCurrentVersion,
	                         &callbacks, &context);
}

namespace LF {

LObject *LXMLTree::objectFromXMLData(const char *bytes, unsigned int length, LObject *into)
{
	CFDataRef data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)bytes, length, kCFAllocatorNull);
	if (data == 0)
		return 0;
	__plistLoaderImp loader(into);
	CFXMLParserRef parser = _createParser(data, &loader);
	CFRelease(data);
	if (parser) {
		Boolean ok = CFXMLParserParse(parser);
		CFRelease(parser);
		if (ok && loader._rootObject)
			return LAutorelease(LRetain(loader._rootObject));
	}
	return 0;
}

LXMLTree *LXMLTree::treeFromData(const LData *data)
{
	LXMLTree *tree = LAutorelease(_alloc());
	StAutoreleasePool pool;
	CFDataRef cfData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)data->bytes(), data->length(),
	                                               kCFAllocatorNull);
	if (cfData == 0)
		return 0;
	__contentHandlerImp handler(tree);
	CFXMLParserRef parser = _createParser(cfData, &handler);
	CFRelease(cfData);
	if (parser == 0)
		return 0;
	Boolean ok = CFXMLParserParse(parser);
	CFRelease(parser);
	return ok ? tree : 0;
}

} // namespace LF
