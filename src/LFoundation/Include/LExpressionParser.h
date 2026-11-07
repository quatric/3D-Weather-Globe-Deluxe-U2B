// LExpressionParser: compiles a condition such as "!a.b && (c >= 2)" once
// into a CPrecompiledContainer, then evaluates it as often as needed with
// LExpressionEvaluator. Names are turned into int addresses by a
// CVarAddrCallback, so evaluation reads the variables' current values.
#pragma once

#include "LString.h"

namespace LF {

enum TPreOpCode {
	kPreOpNone = 0,
	kPreOpNot	// "!"
};

enum TPostOpCode {
	kPostOpNone = 0,
	kPostOpBitAnd,	// "&"
	kPostOpBitOr,	// "|"
	kPostOpBitXor,	// "^"
	kPostOpAdd,	// "+"
	kPostOpSubtract,	// "-"
	kPostOpMultiply,	// "*"
	kPostOpDivide,	// "/"
	kPostOpGreater,	// ">"
	kPostOpLess,	// "<"
	kPostOpGreaterOrEqual,	// ">="
	kPostOpLessOrEqual,	// "<="
	kPostOpEqual,	// "=="
	kPostOpNotEqual,	// "!="
	kPostOpAnd,	// "&&"
	kPostOpOr,	// "||"
	kPostOpXor	// "^^"
};

// An operand, its operators and the precedence of the operator after it.
struct SPrecompiledEntry {
	int preOp;	// a TPreOpCode
	int value;
	int postOp;	// a TPostOpCode
	int priority;	// -1 once the operation is done
};

class CPrecompiledContainer;

// Where an operand's value comes from at evaluation time.
struct SExprAtom {
	enum { kConstant = 0, kVariable, kSubexpression };
	void freeAtom();

	int type;
	union {
		int constant;
		const int *variable;
		CPrecompiledContainer *subexpression;
	};
};

// Resolves a variable name to the address of its value.
class CVarAddrCallback {
public:
	virtual const int *addressOfVariable(LString *name) = 0;
};

class LExpressionEvaluator : public LObject {
	LF_DECLARE_CLASS(LExpressionEvaluator)
public:
	virtual ~LExpressionEvaluator() {}
	virtual LExpressionEvaluator *init() { return this; }
	virtual LExpressionEvaluator *getOwnerLObject() { return this; }
	// Works on a copy of the entries; returns the last entry's value.
	virtual int evaluateLinear(const SPrecompiledEntry *entries, long count);
	virtual int priorityOfPostOp(int postOp);
	virtual int performPreOp(int value, int preOp);
	virtual int performOperation(int left, int right, int postOp);
};

class CPrecompiledContainer {
public:
	CPrecompiledContainer() {}
	~CPrecompiledContainer();
	void appendEntry(int preOp, CPrecompiledContainer *subexpression, int postOp, long priority);
	void appendEntry(int preOp, int constant, int postOp, long priority);
	void appendEntry(int preOp, const int *variable, int postOp, long priority);
	int evaluateWithEvaluator(LExpressionEvaluator *evaluator);

	TArray<SPrecompiledEntry> _entries;
	TArray<SExprAtom> _atoms;
};

class LExpressionParser : public LObject {
	LF_DECLARE_CLASS(LExpressionParser)
public:
	LExpressionParser() : _evaluator(0), _container(0) {}
	virtual ~LExpressionParser() {}
	virtual LExpressionParser *init();
	virtual void dealloc();
	virtual LExpressionParser *getOwnerLObject() { return this; }

	int postOpCode(LString *op);
	void precompileExpression(LString *expression, CVarAddrCallback *callback);
	void precompileToContanerWithExpression(CPrecompiledContainer *container, LString *expression,
	    CVarAddrCallback *callback);
	int evaluatePrecompiled();

	LExpressionEvaluator *_evaluator;
	CPrecompiledContainer *_container;
};

} // namespace LF
