// LUndoManager: undo and redo stacks of recorded invocations, optionally
// grouped. A group is a pair of markers (UndoItems with the group's level)
// around its items; with groupsByEvent, everything registered while the main
// run loop handles one event is one group.
#pragma once

#include "LString.h"
#include <objc/objc.h>

namespace LF {

class LUndoManager : public LObject {
	LF_DECLARE_CLASS(LUndoManager)
public:
	// A stack entry; a plain UndoItem is a group marker.
	class UndoItem {
	public:
		UndoItem() : _next(0), _actionName(0), _isGroupMarker(false), _level(0) {}
		UndoItem(int level) : _next(0), _actionName(0), _isGroupMarker(true), _level(level) {}
		virtual ~UndoItem()
		{
			if (_actionName) {
				_actionName->release();
				_actionName = 0;
			}
		}
		virtual void *target() { return 0; }
		virtual void invoke() {}
		virtual bool isGroupMarker() { return _isGroupMarker; }

		LString *actionName() { return _actionName; }
		void setActionName(LString *name);

		UndoItem *_next;
		LString *_actionName;
		bool _isGroupMarker;
		int _level;
	};

	// target->sel(args...), with up to three LObject arguments.
	class UndoInvocationRecord : public UndoItem {
	public:
		UndoInvocationRecord(LObject *target, const char *sel, LArray *args);
		virtual ~UndoInvocationRecord();
		virtual void *target() { return _target; }
		virtual void invoke();

		LObject *_target;
		const char *_sel;
		LArray *_args;
	};

	// [target sel...] with LObject arguments.
	class UndoObjCInvocationRecord : public UndoItem {
	public:
		UndoObjCInvocationRecord(id target, SEL sel, LArray *args);
		virtual ~UndoObjCInvocationRecord();
		virtual void *target() { return _target; }
		virtual void invoke();

		id _target;
		SEL _sel;
		LArray *_args;
	};

	// A singly linked list of UndoItems, newest first.
	class CUndoStack {
	public:
		CUndoStack() : _top(0) {}
		void push(UndoItem *item)
		{
			if (_top)
				item->_next = _top;
			_top = item;
		}
		UndoItem *pop()
		{
			UndoItem *item = _top;
			if (item) {
				_top = item->_next;
				item->_next = 0;
			}
			return item;
		}
		UndoItem *top() { return _top; }
		void removeInvocationsWithTarget(void *target);
		void removeGroupIfEmpty(UndoItem *&head);
		void deleteAll()
		{
			while (UndoItem *item = pop())
				delete item;
		}

		UndoItem *_top;
	};

	static LString *kWillChangeNotificationName;
	static LString *kDidChangeNotificationName;

	LUndoManager()
	    : _thread(0), _isUndoing(false), _isRedoing(false), _groupsByEvent(false), _groupingByEvent(false),
	      _groupLevel(0), _undoStack(0), _redoStack(0)
	{
	}
	virtual ~LUndoManager() {}
	virtual void initAsSingleton(void *param) { init(); }
	virtual LUndoManager *init();
	virtual void dealloc();
	virtual LUndoManager *getOwnerLObject() { return this; }

	bool isUndoing() const { return _isUndoing; }
	bool isRedoing() const { return _isRedoing; }
	LObject *prepareWithInvocationTarget(LObject *target) { return target; }
	bool groupsByEvent() const { return _groupsByEvent; }
	void setGroupsByEvent(unsigned char groups) { _groupsByEvent = groups; }
	bool canUndo() const { return _undoStack->top() != 0; }
	bool canRedo() const { return _redoStack->top() != 0; }
	LString *undoActionName() const { return _undoStack->top() ? _undoStack->top()->_actionName : 0; }
	LString *redoActionName() const { return _redoStack->top() ? _redoStack->top()->_actionName : 0; }

	void beginUndoGrouping();
	void endUndoGrouping();
	void endGroupingByEvent();
	void runLoopProcessedEvent() { endGroupingByEvent(); }
	void removeAllActionsWithTarget(void *target);
	void removeAllActions();
	void setActionName(LString *name);
	void undo() { _undoRedo(1); }
	void redo() { _undoRedo(0); }
	// Up to three arguments, ending at the first null.
	void registerUndo(LObject *target, const char *sel, LObject *a1 = 0, LObject *a2 = 0, LObject *a3 = 0);
	// Arguments ending with a null.
	void registerUndo(id target, SEL sel, LObject *first, ...);

	void _registerUndoItem(UndoItem *item);
	void _undoRedo(signed char undo);
	CUndoStack *_activeStack() const { return _isUndoing ? _redoStack : _undoStack; }
	void _nameGroupStart(UndoItem *top);

	pthread_t _thread;
	bool _isUndoing;
	bool _isRedoing;
	bool _groupsByEvent;
	bool _groupingByEvent;
	int _groupLevel;
	CUndoStack *_undoStack;
	CUndoStack *_redoStack;
};

} // namespace LF
