// LExpressionParser
//
// LF::LExpressionParser, LF::LExpressionEvaluator, LF::CPrecompiledContainer
// and LF::SExprAtom.

#include "LExpressionParser.h"
#include "LScanner.h"
#include "LCleanupTask.h"
#include <memory>
#include <string.h>

namespace LF {

LF_IMPLEMENT_CLASS(LExpressionEvaluator, 0)
LF_IMPLEMENT_CLASS(LExpressionParser, 0)

// Letters, digits and ":._": what a variable name is made of.
static LCharSet *_kCEquationChars = 0;
static LConstantString _kOpenBracket("(");
static LConstantString _kCloseBracket(")");

static CSharableResource *_staticsLocker()
{
	static CSharableResource sStaticsLocker;
	return &sStaticsLocker;
}

class _exprParserCleaner : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		if (_kCEquationChars) {
			_kCEquationChars->release();
			_kCEquationChars = 0;
		}
	}
};
static _exprParserCleaner sExprParserCleaner;

void SExprAtom::freeAtom()
{
	if (type == kSubexpression && subexpression)
		delete subexpression;
}

int LExpressionEvaluator::priorityOfPostOp(int postOp)
{
	switch (postOp) {
	case kPostOpBitAnd:
	case kPostOpBitOr:
	case kPostOpBitXor:
		return 10;
	case kPostOpAdd:
	case kPostOpSubtract:
		return 6;
	case kPostOpMultiply:
	case kPostOpDivide:
		return 8;
	case kPostOpGreater:
	case kPostOpLess:
	case kPostOpGreaterOrEqual:
	case kPostOpLessOrEqual:
	case kPostOpEqual:
	case kPostOpNotEqual:
		return 4;
	case kPostOpAnd:
	case kPostOpOr:
	case kPostOpXor:
		return 2;
	default:
		return 0;
	}
}

int LExpressionEvaluator::performPreOp(int value, int preOp)
{
	if (preOp == kPreOpNot)
		return value == 0;
	return value;
}

int LExpressionEvaluator::performOperation(int left, int right, int postOp)
{
	switch (postOp) {
	case kPostOpBitAnd: return left & right;
	case kPostOpBitOr: return left | right;
	case kPostOpBitXor: return left ^ right;
	case kPostOpAdd: return left + right;
	case kPostOpSubtract: return left - right;
	case kPostOpMultiply: return left * right;
	case kPostOpDivide: return left / right;
	case kPostOpGreater: return left > right;
	case kPostOpLess: return left < right;
	case kPostOpGreaterOrEqual: return left >= right;
	case kPostOpLessOrEqual: return left <= right;
	case kPostOpEqual: return left == right;
	case kPostOpNotEqual: return left != right;
	case kPostOpAnd: return left != 0 && right != 0;
	case kPostOpOr: return left != 0 || right != 0;
	case kPostOpXor: return (left != 0) != (right != 0);
	default: return left;
	}
}

int LExpressionEvaluator::evaluateLinear(const SPrecompiledEntry *entries, long count)
{
	if (count == 0)
		return 0;
	StPointerBlock block(entries, (unsigned int)(count * sizeof(SPrecompiledEntry)));
	SPrecompiledEntry *e = (SPrecompiledEntry *)block.bytes();
	for (long i = 0; i < count; i++)
		e[i].value = performPreOp(e[i].value, e[i].preOp);

	// Highest priority first; each operation folds its result into the next
	// operand still pending, left to right.
	bool done;
	do {
		int maxPriority = 0;
		for (long i = 0; i < count; i++)
			if (e[i].priority > maxPriority)
				maxPriority = e[i].priority;
		done = true;
		// As in the original, skipping finished operands advances the
		// index but not the entry.
		SPrecompiledEntry *p = e;
		for (long i = 0; i < count - 1; i++, p++) {
			if (p->priority < maxPriority)
				continue;
			SPrecompiledEntry *next = p + 1;
			for (; i + 1 < count && next->priority < 0; next++)
				i++;
			next->value = performOperation(p->value, next->value, p->postOp);
			p->priority = -1;
			done = false;
		}
	} while (!done);
	return e[count - 1].value;
}

CPrecompiledContainer::~CPrecompiledContainer()
{
	int n = _atoms.count();
	for (int i = 0; i < n; i++)
		_atoms[i].freeAtom();
}

void CPrecompiledContainer::appendEntry(int preOp, CPrecompiledContainer *subexpression, int postOp, long priority)
{
	SPrecompiledEntry entry = {preOp, 0, postOp, (int)priority};
	_entries.addObject(entry);
	SExprAtom atom;
	atom.type = SExprAtom::kSubexpression;
	atom.subexpression = subexpression;
	_atoms.addObject(atom);
}

void CPrecompiledContainer::appendEntry(int preOp, int constant, int postOp, long priority)
{
	SPrecompiledEntry entry = {preOp, constant, postOp, (int)priority};
	_entries.addObject(entry);
	SExprAtom atom;
	atom.type = SExprAtom::kConstant;
	atom.constant = constant;
	_atoms.addObject(atom);
}

void CPrecompiledContainer::appendEntry(int preOp, const int *variable, int postOp, long priority)
{
	SPrecompiledEntry entry = {preOp, 0, postOp, (int)priority};
	_entries.addObject(entry);
	SExprAtom atom;
	atom.type = SExprAtom::kVariable;
	atom.variable = variable;
	_atoms.addObject(atom);
}

int CPrecompiledContainer::evaluateWithEvaluator(LExpressionEvaluator *evaluator)
{
	int n = _atoms.count();
	SPrecompiledEntry *entry = (SPrecompiledEntry *)_entries.bytes();
	for (int i = 0; i < n; i++, entry++) {
		const SExprAtom &atom = _atoms[i];
		if (atom.type == SExprAtom::kVariable)
			entry->value = *atom.variable;
		else if (atom.type == SExprAtom::kSubexpression)
			entry->value = atom.subexpression->evaluateWithEvaluator(evaluator);
	}
	return evaluator->evaluateLinear((const SPrecompiledEntry *)_entries.bytes(), _entries.count());
}

LExpressionParser *LExpressionParser::init()
{
	if (_kCEquationChars == 0) {
		StSharableLocker lock(_staticsLocker(), true);
		if (_kCEquationChars == 0) {
			LCharSet *alnum = LCharSet::charSetWithType(LCharSet::kAlphaNumeric);
			_kCEquationChars = LCharSet::_alloc()->initWithCharSet(alnum);
			_kCEquationChars->addCharactersInCString(":._");
		}
	}
	_evaluator = LExpressionEvaluator::_alloc()->init();
	return this;
}

void LExpressionParser::dealloc()
{
	if (_evaluator) {
		_evaluator->release();
		_evaluator = 0;
	}
	if (_container) {
		delete _container;
		_container = 0;
	}
	LObject::dealloc();
}

int LExpressionParser::postOpCode(LString *op)
{
	if (op == 0 || op->length() == 0)
		return kPostOpNone;
	if (op->length() == 1) {
		switch (op->chars()[0]) {
		case '&': return kPostOpBitAnd;
		case '|': return kPostOpBitOr;
		case '^': return kPostOpBitXor;
		case '+': return kPostOpAdd;
		case '-': return kPostOpSubtract;
		case '*': return kPostOpMultiply;
		case '/': return kPostOpDivide;
		case '>': return kPostOpGreater;
		case '<': return kPostOpLess;
		default: return kPostOpNone;
		}
	}
	if (op->length() != 2)
		return kPostOpNone;
	static const struct { const char *text; int code; } kTwoCharOps[] = {
		{">=", kPostOpGreaterOrEqual}, {"<=", kPostOpLessOrEqual}, {"==", kPostOpEqual},
		{"!=", kPostOpNotEqual}, {"&&", kPostOpAnd}, {"||", kPostOpOr}, {"^^", kPostOpXor},
	};
	const char *s = op->cString();
	for (unsigned i = 0; i < sizeof kTwoCharOps / sizeof kTwoCharOps[0]; i++)
		if (strcmp(kTwoCharOps[i].text, s) == 0)
			return kTwoCharOps[i].code;
	return kPostOpNone;
}

void LExpressionParser::precompileExpression(LString *expression, CVarAddrCallback *callback)
{
	if (_container) {
		delete _container;
		_container = 0;
	}
	_container = new CPrecompiledContainer;
	precompileToContanerWithExpression(_container, expression, callback);
}

void LExpressionParser::precompileToContanerWithExpression(CPrecompiledContainer *container,
    LString *expression, CVarAddrCallback *callback)
{
	LScanner *scanner = LScanner::scannerWithString(expression);
	LCharSet *digits = LCharSet::charSetWithType(LCharSet::kDecimalDigit);
	LCharSet *operators = LCharSet::charSetWithType(LCharSet::kOperators);
	LCharSet *spaces = LCharSet::charSetWithType(LCharSet::kWhitespaceAndNewline);
	LCharSet *alnum = LCharSet::charSetWithType(LCharSet::kAlphaNumeric);
	// Owns a subexpression until it is in the container.
	std::unique_ptr<CPrecompiledContainer> pending;

	do {
		scanner->scanCharSet(spaces, 0);
		int preOp = kPreOpNone;
		CPrecompiledContainer *subexpression = 0;
		const int *variable = 0;
		int constant = 0;
		for (;;) {
			LString *prefix = 0;
			{
				StScannerPosKeeper keeper(scanner);
				scanner->scanToCharSet(alnum, &prefix);
			}
			prefix->trimWithCharacters(spaces);
			if (prefix->length() == 0) {
				LString *token = 0;
				scanner->scanCharSet(digits, &token);
				if (token && token->length()) {
					constant = token->intValue();
					goto haveOperand;
				}
				scanner->scanCharSet(_kCEquationChars, &token);
				if (token == 0 || token->length() == 0)
					return;
				variable = callback->addressOfVariable(token);
				goto haveOperand;
			}
			if (prefix->chars()[0] != '!') {
				if (prefix->chars()[0] != '(')
					return;
				break;
			}
			scanner->movePositionBy(1);
			preOp = kPreOpNot;
		}
		{
			LString *inside = 0;
			if (!scanner->scanInsideBrackets(&_kOpenBracket, &_kCloseBracket, &inside))
				return;
			subexpression = new CPrecompiledContainer;
			pending.reset(subexpression);
			precompileToContanerWithExpression(subexpression, inside, callback);
		}
haveOperand:
		{
			LString *op = 0;
			scanner->scanCharSet(spaces, 0);
			scanner->scanCharSet(operators, &op);
			int postOp = postOpCode(op);
			int priority = _evaluator->priorityOfPostOp(postOp);
			if (subexpression) {
				container->appendEntry(preOp, subexpression, postOp, priority);
				pending.release();
			} else if (variable) {
				container->appendEntry(preOp, variable, postOp, priority);
			} else {
				container->appendEntry(preOp, constant, postOp, priority);
			}
		}
	} while (scanner->position() < expression->length());
}

int LExpressionParser::evaluatePrecompiled()
{
	return _container ? _container->evaluateWithEvaluator(_evaluator) : 0;
}

} // namespace LF
