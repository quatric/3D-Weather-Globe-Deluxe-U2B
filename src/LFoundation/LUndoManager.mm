// LUndoManager
//
// LF::LUndoManager and its undo records. Objective-C++ for the records of
// Objective-C targets; built without ARC.

#include "LUndoManager.h"
#include "LArray.h"
#include "LNotification.h"
#include "LRunLoop.h"
#import <Foundation/Foundation.h>

namespace LF {

LF_IMPLEMENT_CLASS(LUndoManager, 0)

static LConstantString sWillChangeNotificationName("UndoManagerWillChangeNotification");
static LConstantString sDidChangeNotificationName("UndoManagerDidChangeNotification");
LString *LUndoManager::kWillChangeNotificationName = &sWillChangeNotificationName;
LString *LUndoManager::kDidChangeNotificationName = &sDidChangeNotificationName;

void LUndoManager::UndoItem::setActionName(LString *name)
{
	if (_actionName != name) {
		if (_actionName) {
			_actionName->release();
			_actionName = 0;
		}
		_actionName = LRetain(name);
	}
}

LUndoManager::UndoInvocationRecord::UndoInvocationRecord(LObject *target, const char *sel, LArray *args)
    : _target(target), _sel(sel), _args(LRetain(args))
{
}

LUndoManager::UndoInvocationRecord::~UndoInvocationRecord()
{
	if (_args) {
		_args->release();
		_args = 0;
	}
}

void LUndoManager::UndoInvocationRecord::invoke()
{
	if (_target == 0 || _sel == 0)
		return;
	switch (_args->count()) {
	case 0:
		_target->invokeSelector(_sel, (LObject *)0);
		break;
	case 1:
		_target->invokeSelector(_sel, _args->objectAtIndex(0));
		break;
	case 2:
		_target->invokeSelector(_sel, _args->objectAtIndex(0), _args->objectAtIndex(1), (LObject *)0);
		break;
	case 3:
		_target->invokeSelector(_sel, _args->objectAtIndex(0), _args->objectAtIndex(1), _args->objectAtIndex(2));
		break;
	}
}

LUndoManager::UndoObjCInvocationRecord::UndoObjCInvocationRecord(id target, SEL sel, LArray *args)
    : _target(target), _sel(sel), _args(LRetain(args))
{
}

LUndoManager::UndoObjCInvocationRecord::~UndoObjCInvocationRecord()
{
	if (_args) {
		_args->release();
		_args = 0;
	}
}

// One argument goes through -performSelector:withObject:, others through an
// NSInvocation. The arguments are the LObject pointers themselves.
void LUndoManager::UndoObjCInvocationRecord::invoke()
{
	int n = _args->count();
	if (n == 1) {
		LObject *arg = _args->lastObject();
		[(NSObject *)_target performSelector:_sel withObject:(id)arg];
		return;
	}
	NSInvocation *inv =
	    [NSInvocation invocationWithMethodSignature:[(NSObject *)_target methodSignatureForSelector:_sel]];
	for (int i = 0; i < n; i++) {
		LObject *arg = _args->objectAtIndex(i);
		[inv setArgument:&arg atIndex:i + 2];
	}
	[inv setSelector:_sel];
	[inv setTarget:_target];
	[inv invoke];
}

// Removes the empty groups (a marker followed by markers only up to its
// match) from `head` on. As in the original, once a group with items is
// seen, later groups count as non-empty too.
void LUndoManager::CUndoStack::removeGroupIfEmpty(UndoItem *&head)
{
	UndoItem *item = head;
	UndoItem *prev = 0;
	bool empty = true;
	while (item) {
		UndoItem *last = item;
		if (item->isGroupMarker()) {
			UndoItem *before = item;
			UndoItem *match;
			for (;;) {
				match = before ? before->_next : 0;
				if (match == 0)
					break;
				if (!match->isGroupMarker()) {
					empty = false;
					before = match;
					continue;
				}
				if (match->_level == item->_level)
					break;
				// A nested group (the original didn't relink after removing
				// the nested group's start).
				removeGroupIfEmpty(match);
				before->_next = match;
				before = match;
			}
			if (match) {
				last = match;
				if (empty) {
					last = item->_next == match ? match->_next : item->_next;
					before->_next = match->_next;
					if (prev)
						prev->_next = last;
					if (item == head)
						head = last;
					delete item;
					delete match;
				}
			}
		}
		// The original crashed here when an empty group ended the stack.
		if (last == 0)
			return;
		item = last->_next;
		prev = last;
	}
}

void LUndoManager::CUndoStack::removeInvocationsWithTarget(void *target)
{
	UndoItem *item = _top;
	UndoItem *prev = 0;
	while (item) {
		UndoItem *next = item->_next;
		if (!item->isGroupMarker() && item->target() == target) {
			if (prev)
				prev->_next = next;
			if (item == _top)
				_top = next;
			delete item;
		} else
			prev = item;
		item = next;
	}
	removeGroupIfEmpty(_top);
}

LUndoManager *LUndoManager::init()
{
	if (LObject::init()) {
		_thread = LThread::currentThreadID();
		_undoStack = new CUndoStack;
		_redoStack = new CUndoStack;
	}
	return this;
}

void LUndoManager::dealloc()
{
	removeAllActions();
	delete _undoStack;
	delete _redoStack;
	LObject::dealloc();
}

void LUndoManager::beginUndoGrouping()
{
	_groupLevel++;
	_activeStack()->push(new UndoItem(_groupLevel));
}

// The marker at the start of the group of the current level gets the action
// name of `top`.
void LUndoManager::_nameGroupStart(UndoItem *top)
{
	for (UndoItem *item = top->_next; item; item = item->_next) {
		if (item->isGroupMarker() && item->_level == _groupLevel) {
			item->setActionName(top->_actionName);
			return;
		}
	}
}

// An empty group is dropped; otherwise its end marker is pushed, and both
// markers take the name of the group's last action.
void LUndoManager::endUndoGrouping()
{
	CUndoStack *stack = _activeStack();
	if (_groupLevel <= 0)
		return;
	UndoItem *top = stack->top();
	if (top && top->isGroupMarker() && top->_level == _groupLevel)
		stack->pop();
	else if (top) {
		UndoItem *marker = new UndoItem(_groupLevel);
		marker->setActionName(top->_actionName);
		stack->push(marker);
		_nameGroupStart(marker);
	}
	_groupLevel--;
}

void LUndoManager::endGroupingByEvent()
{
	if (_groupingByEvent) {
		_groupingByEvent = false;
		LRunLoop::mainThreadRunLoop()->setObservingUndoManager(0);
		endUndoGrouping();
		_isUndoing = false;
		_isRedoing = false;
	}
}

void LUndoManager::removeAllActionsWithTarget(void *target)
{
	endGroupingByEvent();
	_undoStack->removeInvocationsWithTarget(target);
	_redoStack->removeInvocationsWithTarget(target);
}

void LUndoManager::removeAllActions()
{
	endGroupingByEvent();
	_undoStack->deleteAll();
	_redoStack->deleteAll();
	delete _undoStack;
	delete _redoStack;
	_undoStack = new CUndoStack;
	_redoStack = new CUndoStack;
}

// Actions registered while undoing go on the redo stack; others go on the
// undo stack, and a new action (neither undoing nor redoing) clears redo.
void LUndoManager::_registerUndoItem(UndoItem *item)
{
	LNotificationCenter::center()->postNotification(kWillChangeNotificationName, this, 0);
	if (_groupsByEvent && !_groupingByEvent) {
		_groupingByEvent = true;
		beginUndoGrouping();
		LRunLoop::mainThreadRunLoop()->setObservingUndoManager(this);
	}
	CUndoStack *stack;
	if (_isUndoing) {
		item->setActionName(undoActionName());
		stack = _redoStack;
	} else {
		if (_isRedoing)
			item->setActionName(redoActionName());
		stack = _undoStack;
	}
	stack->push(item);
	if (!_isUndoing && !_isRedoing) {
		_redoStack->deleteAll();
		delete _redoStack;
		_redoStack = new CUndoStack;
	}
	LNotificationCenter::center()->postNotification(kDidChangeNotificationName, this, 0);
}

void LUndoManager::setActionName(LString *name)
{
	if (_isUndoing || _isRedoing)
		return;
	UndoItem *top = _undoStack->top();
	if (top == 0)
		return;
	top->setActionName(name);
	if (top->isGroupMarker() && _groupLevel < top->_level)
		_nameGroupStart(top);
}

// Undoes (or redoes) the top action, or the whole group on top. A group's
// markers move to the other stack around the actions its invocations
// register there.
void LUndoManager::_undoRedo(signed char undo)
{
	endGroupingByEvent();
	CUndoStack *from = undo ? _undoStack : _redoStack;
	CUndoStack *to = undo ? _redoStack : _undoStack;
	if (from->top() == 0)
		return;
	if (undo)
		_isUndoing = true;
	else
		_isRedoing = true;
	UndoItem *item = from->top();
	if (!item->isGroupMarker()) {
		item->invoke();
		from->pop();
		delete item;
	} else {
		int level = item->_level;
		LString *name = LRetain(item->_actionName);
		for (;;) {
			from->pop();
			if (!item->isGroupMarker()) {
				item->invoke();
				delete item;
			} else
				to->push(item);
			item = from->top();
			if (item == 0)
				break;
			if (item->isGroupMarker() && item->_level == level) {
				from->pop();
				to->push(item);
				break;
			}
		}
		UndoItem *top = to->top();
		if (top) {
			top->setActionName(name);
			if (top->isGroupMarker())
				_nameGroupStart(top);
		}
		if (name)
			name->release();
	}
	if (!_groupingByEvent) {
		_isUndoing = false;
		_isRedoing = false;
	}
}

void LUndoManager::registerUndo(LObject *target, const char *sel, LObject *a1, LObject *a2, LObject *a3)
{
	LArray *args = LArray::arrayWithObjects(a1, a2, a3, (LObject *)0);
	_registerUndoItem(new UndoInvocationRecord(target, sel, args));
}

void LUndoManager::registerUndo(id target, SEL sel, LObject *first, ...)
{
	va_list ap;
	va_start(ap, first);
	LArray *args = LAutorelease(LArray::_alloc()->initWithObjectsList(first, ap));
	va_end(ap);
	_registerUndoItem(new UndoObjCInvocationRecord(target, sel, args));
}

} // namespace LF
