// Regexp: Henry Spencer's regular expressions (regcomp/regexec) behind a
// small class. The compiler and the matcher keep their state in a context
// instead of globals.
//
// Syntax: . ^ $ [] [^] * + ? | () and \c; at most 9 groups.

#include "AppModel.h"
#include <string>

namespace {

enum { NSUBEXP = 10, MAGIC = 0234 };

// Node opcodes; a node is the opcode, a two-byte offset to the next node,
// and the operand.
enum {
	END = 0,     // end of the program
	BOL = 1,     // match "" at the beginning of the line
	EOL = 2,     // match "" at the end of the line
	ANY = 3,     // any character
	ANYOF = 4,   // any character of the string operand
	ANYBUT = 5,  // any character not in the string operand
	BRANCH = 6,  // try the operand, else the next node
	BACK = 7,    // like NOTHING, with the offset pointing back
	EXACTLY = 8, // the string operand
	NOTHING = 9, // the empty string
	STAR = 10,   // the operand node, 0 or more times
	PLUS = 11,   // the operand node, 1 or more times
	OPEN = 20,   // OPEN+n starts group n
	CLOSE = 30   // CLOSE+n ends group n
};

// Flags of compiled pieces.
enum { HASWIDTH = 01, SIMPLE = 02, SPSTART = 04, WORST = 0 };

inline char OP(const char *p)
{
	return *p;
}

inline int NEXT(const char *p)
{
	return ((*(p + 1) & 0377) << 8) + (*(p + 2) & 0377);
}

inline char *OPERAND(char *p)
{
	return p + 3;
}

inline bool ISMULT(char c)
{
	return c == '*' || c == '+' || c == '?';
}

const char META[] = "^$.[()|?+*\\";

void regerror(const char *s)
{
	printf("regexp error: %s", s);
}

} // namespace

struct regexp {
	char *startp[NSUBEXP];
	char *endp[NSUBEXP];
	const char *base;	// the string the offsets are from
	char regstart;	// the character a match must start with, or 0
	char reganch;	// anchored at the beginning of lines
	char *regmust;	// a string every match contains, or 0
	int regmlen;
	char program[1];
};

namespace {

// The compiler's state. The first pass only measures the program (code is
// &dummy) and the second fills it.
struct Compiler {
	const char *parse;
	int npar;
	char dummy;
	char *code;
	long size;

	char *reg(int paren, int *flagp);
	char *branch(int *flagp);
	char *piece(int *flagp);
	char *atom(int *flagp);
	char *node(char op);
	void c(char b);
	void insert(char op, char *opnd);
	void tail(char *p, char *val);
	void optail(char *p, char *val);

	char *next(char *p)
	{
		if (p == &dummy)
			return 0;
		int offset = NEXT(p);
		if (offset == 0)
			return 0;
		return OP(p) == BACK ? p - offset : p + offset;
	}
};

// A parenthesized or top-level expression.
char *Compiler::reg(int paren, int *flagp)
{
	char *ret, *br, *ender;
	int parno = 0;
	int flags;
	*flagp = HASWIDTH;
	if (paren) {
		if (npar >= NSUBEXP) {
			regerror("too many ()");
			return 0;
		}
		parno = npar++;
		ret = node(OPEN + parno);
	} else {
		ret = 0;
	}
	br = branch(&flags);
	if (br == 0)
		return 0;
	if (ret != 0)
		tail(ret, br);
	else
		ret = br;
	if (!(flags & HASWIDTH))
		*flagp &= ~HASWIDTH;
	*flagp |= flags & SPSTART;
	while (*parse == '|') {
		parse++;
		br = branch(&flags);
		if (br == 0)
			return 0;
		tail(ret, br);
		if (!(flags & HASWIDTH))
			*flagp &= ~HASWIDTH;
		*flagp |= flags & SPSTART;
	}
	ender = node(paren ? CLOSE + parno : END);
	tail(ret, ender);
	for (br = ret; br != 0; br = next(br))
		optail(br, ender);
	if (paren && *parse++ != ')') {
		regerror("unmatched ()");
		return 0;
	} else if (!paren && *parse != '\0') {
		if (*parse == ')')
			regerror("unmatched ()");
		else
			regerror("internal error: junk on end");
		return 0;
	}
	return ret;
}

// Concatenated pieces.
char *Compiler::branch(int *flagp)
{
	char *ret, *chain, *latest;
	int flags;
	*flagp = WORST;
	ret = node(BRANCH);
	chain = 0;
	while (*parse != '\0' && *parse != '|' && *parse != ')') {
		latest = piece(&flags);
		if (latest == 0)
			return 0;
		*flagp |= flags & HASWIDTH;
		if (chain == 0)
			*flagp |= flags & SPSTART;
		else
			tail(chain, latest);
		chain = latest;
	}
	if (chain == 0)
		node(NOTHING);
	return ret;
}

// An atom with an optional *, + or ?.
char *Compiler::piece(int *flagp)
{
	char *ret, *nxt;
	char op;
	int flags;
	ret = atom(&flags);
	if (ret == 0)
		return 0;
	op = *parse;
	if (!ISMULT(op)) {
		*flagp = flags;
		return ret;
	}
	if (!(flags & HASWIDTH) && op != '?') {
		regerror("*+ operand could be empty");
		return 0;
	}
	*flagp = op != '+' ? (WORST | SPSTART) : (WORST | HASWIDTH);
	if (op == '*' && (flags & SIMPLE)) {
		insert(STAR, ret);
	} else if (op == '*') {
		// x* as (x&|), with the x looping back to the branch.
		insert(BRANCH, ret);
		optail(ret, node(BACK));
		optail(ret, ret);
		tail(ret, node(BRANCH));
		tail(ret, node(NOTHING));
	} else if (op == '+' && (flags & SIMPLE)) {
		insert(PLUS, ret);
	} else if (op == '+') {
		// x+ as x(&|).
		nxt = node(BRANCH);
		tail(ret, nxt);
		tail(node(BACK), ret);
		tail(nxt, node(BRANCH));
		tail(ret, node(NOTHING));
	} else if (op == '?') {
		// x? as (x|).
		insert(BRANCH, ret);
		tail(ret, node(BRANCH));
		nxt = node(NOTHING);
		tail(ret, nxt);
		optail(ret, nxt);
	}
	parse++;
	if (ISMULT(*parse)) {
		regerror("nested *?+");
		return 0;
	}
	return ret;
}

char *Compiler::atom(int *flagp)
{
	char *ret;
	int flags;
	*flagp = WORST;
	switch (*parse++) {
	case '^':
		ret = node(BOL);
		break;
	case '$':
		ret = node(EOL);
		break;
	case '.':
		ret = node(ANY);
		*flagp |= HASWIDTH | SIMPLE;
		break;
	case '[': {
		if (*parse == '^') {
			ret = node(ANYBUT);
			parse++;
		} else {
			ret = node(ANYOF);
		}
		if (*parse == ']' || *parse == '-')
			c(*parse++);
		while (*parse != '\0' && *parse != ']') {
			if (*parse == '-') {
				parse++;
				if (*parse == ']' || *parse == '\0') {
					c('-');
				} else {
					int from = (unsigned char)*(parse - 2) + 1;
					int to = (unsigned char)*parse;
					if (from > to + 1) {
						regerror("invalid [] range");
						return 0;
					}
					for (; from <= to; from++)
						c((char)from);
					parse++;
				}
			} else {
				c(*parse++);
			}
		}
		c('\0');
		if (*parse != ']') {
			regerror("unmatched []");
			return 0;
		}
		parse++;
		*flagp |= HASWIDTH | SIMPLE;
		break;
	}
	case '(':
		ret = reg(1, &flags);
		if (ret == 0)
			return 0;
		*flagp |= flags & (HASWIDTH | SPSTART);
		break;
	case '\0':
	case '|':
	case ')':
		regerror("internal error: \\0|) unexpected");
		return 0;
	case '?':
	case '+':
	case '*':
		regerror("?+* follows nothing");
		return 0;
	case '\\':
		if (*parse == '\0') {
			regerror("trailing \\");
			return 0;
		}
		ret = node(EXACTLY);
		c(*parse++);
		c('\0');
		*flagp |= HASWIDTH | SIMPLE;
		break;
	default: {
		parse--;
		int len = (int)strcspn(parse, META);
		if (len <= 0) {
			regerror("internal error: strcspn 0");
			return 0;
		}
		// A multiplier applies to the last character only.
		char ender = *(parse + len);
		if (len > 1 && ISMULT(ender))
			len--;
		*flagp |= HASWIDTH;
		if (len == 1)
			*flagp |= SIMPLE;
		ret = node(EXACTLY);
		while (len > 0) {
			c(*parse++);
			len--;
		}
		c('\0');
		break;
	}
	}
	return ret;
}

char *Compiler::node(char op)
{
	char *ret = code;
	if (ret == &dummy) {
		size += 3;
		return ret;
	}
	*code++ = op;
	*code++ = '\0';
	*code++ = '\0';
	return ret;
}

void Compiler::c(char b)
{
	if (code != &dummy)
		*code++ = b;
	else
		size++;
}

// Inserts a node before the operand, moving the operand up.
void Compiler::insert(char op, char *opnd)
{
	if (code == &dummy) {
		size += 3;
		return;
	}
	char *src = code;
	code += 3;
	char *dst = code;
	while (src > opnd)
		*--dst = *--src;
	char *place = opnd;
	*place++ = op;
	*place++ = '\0';
	*place++ = '\0';
}

// Points the last node of the chain at val.
void Compiler::tail(char *p, char *val)
{
	if (p == &dummy)
		return;
	char *scan = p;
	for (;;) {
		char *temp = next(scan);
		if (temp == 0)
			break;
		scan = temp;
	}
	int offset = OP(scan) == BACK ? (int)(scan - val) : (int)(val - scan);
	*(scan + 1) = (offset >> 8) & 0377;
	*(scan + 2) = offset & 0377;
}

// tail() on the operand of a BRANCH.
void Compiler::optail(char *p, char *val)
{
	if (p == 0 || p == &dummy || OP(p) != BRANCH)
		return;
	tail(OPERAND(p), val);
}

regexp *regcomp(const char *exp)
{
	if (exp == 0) {
		regerror("nil argument");
		return 0;
	}
	Compiler cc;
	int flags;
	// First pass: the size.
	cc.parse = exp;
	cc.npar = 1;
	cc.size = 1;
	cc.code = &cc.dummy;
	if (cc.reg(0, &flags) == 0)
		return 0;
	if (cc.size >= 0x7fff) {
		regerror("regexp too big");
		return 0;
	}
	regexp *r = (regexp *)operator new(sizeof(regexp) + cc.size);
	// Second pass: the code.
	cc.parse = exp;
	cc.npar = 1;
	cc.code = r->program;
	cc.c((char)MAGIC);
	if (cc.reg(0, &flags) == 0) {
		operator delete(r);
		return 0;
	}
	r->regstart = '\0';
	r->reganch = 0;
	r->regmust = 0;
	r->regmlen = 0;
	r->base = 0;
	char *scan = r->program + 1;
	if (OP(cc.next(scan)) == END) {
		// One top-level choice.
		scan = OPERAND(scan);
		if (OP(scan) == EXACTLY)
			r->regstart = *OPERAND(scan);
		else if (OP(scan) == BOL)
			r->reganch++;
		// The longest literal string every match has, when the
		// expression starts with a loop.
		if (flags & SPSTART) {
			char *longest = 0;
			size_t len = 0;
			for (; scan != 0; scan = cc.next(scan))
				if (OP(scan) == EXACTLY && strlen(OPERAND(scan)) >= len) {
					longest = OPERAND(scan);
					len = strlen(OPERAND(scan));
				}
			r->regmust = longest;
			r->regmlen = (int)len;
		}
	}
	return r;
}

// The matcher's state.
struct Matcher {
	const char *input;	// the current position
	const char *bol;	// the beginning of the input
	char **startp;
	char **endp;

	char *next(char *p)
	{
		int offset = NEXT(p);
		if (offset == 0)
			return 0;
		return OP(p) == BACK ? p - offset : p + offset;
	}
	int try_(regexp *prog, const char *string);
	int match(char *prog);
	int repeat(char *p);
};

int Matcher::try_(regexp *prog, const char *string)
{
	input = string;
	startp = prog->startp;
	endp = prog->endp;
	for (int i = 0; i < NSUBEXP; i++) {
		prog->startp[i] = 0;
		prog->endp[i] = 0;
	}
	if (match(prog->program + 1)) {
		prog->startp[0] = (char *)string;
		prog->endp[0] = (char *)input;
		return 1;
	}
	return 0;
}

// Matches the node chain from the current input position.
int Matcher::match(char *prog)
{
	char *scan = prog;
	while (scan != 0) {
		char *nxt = next(scan);
		switch (OP(scan)) {
		case BOL:
			if (input != bol)
				return 0;
			break;
		case EOL:
			if (*input != '\0')
				return 0;
			break;
		case ANY:
			if (*input == '\0')
				return 0;
			input++;
			break;
		case EXACTLY: {
			char *opnd = OPERAND(scan);
			if (*opnd != *input)
				return 0;
			size_t len = strlen(opnd);
			if (len > 1 && strncmp(opnd, input, len) != 0)
				return 0;
			input += len;
			break;
		}
		case ANYOF:
			if (*input == '\0' || strchr(OPERAND(scan), *input) == 0)
				return 0;
			input++;
			break;
		case ANYBUT:
			if (*input == '\0' || strchr(OPERAND(scan), *input) != 0)
				return 0;
			input++;
			break;
		case NOTHING:
		case BACK:
			break;
		case BRANCH: {
			if (OP(nxt) != BRANCH) {
				// One choice: no need to backtrack.
				nxt = OPERAND(scan);
				break;
			}
			do {
				const char *save = input;
				if (match(OPERAND(scan)))
					return 1;
				input = save;
				scan = next(scan);
			} while (scan != 0 && OP(scan) == BRANCH);
			return 0;
		}
		case STAR:
		case PLUS: {
			// Try the longest repetition first, backing off one at a
			// time; skip counts the next character rules out.
			char nextch = OP(nxt) == EXACTLY ? *OPERAND(nxt) : '\0';
			int min = OP(scan) == STAR ? 0 : 1;
			const char *save = input;
			int no = repeat(OPERAND(scan));
			while (no >= min) {
				if (nextch == '\0' || *input == nextch)
					if (match(nxt))
						return 1;
				no--;
				input = save + no;
			}
			return 0;
		}
		case END:
			return 1;
		default:
			if (OP(scan) > OPEN && OP(scan) < OPEN + NSUBEXP) {
				int no = OP(scan) - OPEN;
				const char *save = input;
				if (!match(nxt))
					return 0;
				if (startp[no] == 0)
					startp[no] = (char *)save;
				return 1;
			}
			if (OP(scan) > CLOSE && OP(scan) < CLOSE + NSUBEXP) {
				int no = OP(scan) - CLOSE;
				const char *save = input;
				if (!match(nxt))
					return 0;
				if (endp[no] == 0)
					endp[no] = (char *)save;
				return 1;
			}
			regerror("memory corruption");
			return 0;
		}
		scan = nxt;
	}
	regerror("corrupted pointers");
	return 0;
}

// How many times the simple node matches from the current position; moves
// the position after them.
int Matcher::repeat(char *p)
{
	int count = 0;
	const char *scan = input;
	char *opnd = OPERAND(p);
	switch (OP(p)) {
	case ANY:
		count = (int)strlen(scan);
		scan += count;
		break;
	case EXACTLY:
		while (*opnd == *scan) {
			count++;
			scan++;
		}
		break;
	case ANYOF:
		while (*scan != '\0' && strchr(opnd, *scan) != 0) {
			count++;
			scan++;
		}
		break;
	case ANYBUT:
		while (*scan != '\0' && strchr(opnd, *scan) == 0) {
			count++;
			scan++;
		}
		break;
	default:
		regerror("internal foulup");
		count = 0;
		break;
	}
	input = scan;
	return count;
}

int regexec(regexp *prog, const char *string)
{
	if (prog == 0 || string == 0) {
		regerror("nil parameter");
		return 0;
	}
	if ((unsigned char)*prog->program != MAGIC) {
		regerror("corrupted program");
		return 0;
	}
	if (prog->regmust != 0 && strstr(string, prog->regmust) == 0)
		return 0;
	Matcher m;
	m.bol = string;
	if (prog->reganch)
		return m.try_(prog, string);
	const char *s = string;
	if (prog->regstart != '\0') {
		while ((s = strchr(s, prog->regstart)) != 0) {
			if (m.try_(prog, s))
				return 1;
			s++;
		}
	} else {
		do {
			if (m.try_(prog, s))
				return 1;
		} while (*s++ != '\0');
	}
	return 0;
}

} // namespace

// ---------------------------------------------------------------------------
// Regexp

Regexp::Regexp(const char *exp)
{
	size_t length = strlen(exp);
	_exp = new char[length + 1];
	memcpy(_exp, exp, length);
	_exp[length] = '\0';
	_regexp = 0;
}

Regexp::Regexp(const char *exp, int length)
{
	_exp = new char[length + 1];
	strncpy(_exp, exp, length);
	_exp[length] = '\0';
	_regexp = 0;
}

Regexp::~Regexp()
{
	delete[] _exp;
	operator delete(_regexp);
}

// Of group i (0 the whole match) from the start of the string, or -1.
int Regexp::BeginningOfMatch(int i)
{
	if ((unsigned int)i >= NSUBEXP + 1 || _regexp == 0 || _regexp->startp[0] == 0 || _regexp->startp[i] == 0)
		return -1;
	return (int)(_regexp->startp[i] - _regexp->base);
}

int Regexp::EndOfMatch(int i)
{
	if ((unsigned int)i >= NSUBEXP + 1 || _regexp == 0 || _regexp->startp[0] == 0 || _regexp->endp[i] == 0)
		return -1;
	return (int)(_regexp->endp[i] - _regexp->base);
}

bool Regexp::_compile()
{
	operator delete(_regexp);
	_regexp = regcomp(_exp);
	if (_regexp)
		_regexp->startp[0] = 0;
	return _regexp != 0;
}

// Matches at `start` of the first `length` characters; the match length, or
// -1.
int Regexp::Match(const char *string, int length, int start)
{
	if (!_compile())
		return -1;
	std::string text(string, length);
	_regexp->base = text.c_str();
	regexec(_regexp, text.c_str() + start);
	int result = _regexp->startp[0] ? (int)(_regexp->endp[0] - _regexp->startp[0]) : -1;
	_rebase(text.c_str(), string);
	return result;
}

// Searches forward (range >= 0) or backward from `start` within `range`
// characters of the first `length`; the position of the match, or -1. ^ and
// $ match at line boundaries.
int Regexp::Search(const char *string, int length, int start, int range)
{
	if (start + range > length && (range = length - start) < 0)
		return -1;
	if (!_compile())
		return -1;
	std::string text(string, length);
	char *buffer = &text[0];
	bool forward = range >= 0;
	char *from, *end;
	if (forward) {
		from = buffer + start;
		end = from + range;
	} else {
		end = buffer + start;
		from = end + range;
	}
	char saved = *end;
	*end = '\0';
	bool bol = _exp[0] == '^';
	bool eol = _exp[0] != '\0' && _exp[strlen(_exp) - 1] == '$';
	if (bol && from != buffer) {
		from = strchr(from, '\n');
		if (from)
			from++;
	}
	char *lineEnd = 0;
	char lineEndChar = 0;
	char *lastMatch = 0;
	while (from != 0 && from < end) {
		if (eol && (lineEnd = strchr(from, '\n')) != 0) {
			lineEndChar = *lineEnd;
			*lineEnd = '\0';
		}
		int found = regexec(_regexp, from);
		if (lineEnd)
			*lineEnd = lineEndChar;
		if (!found) {
			if (!bol && !eol)
				break;
			from = strchr(from, '\n');
			if (from)
				from++;
		} else {
			if (forward)
				goto done;
			lastMatch = _regexp->startp[0];
			from = _regexp->endp[0];
			if (bol && (from = strchr(from, '\n')) != 0)
				from++;
		}
	}
	if (!forward && lastMatch) {
		if (eol && (lineEnd = strchr(lastMatch, '\n')) != 0) {
			lineEndChar = *lineEnd;
			*lineEnd = '\0';
		}
		regexec(_regexp, lastMatch);
		if (lineEnd)
			*lineEnd = lineEndChar;
	}
done:
	*end = saved;
	_regexp->base = buffer;
	int result = _regexp->startp[0] ? (int)(_regexp->startp[0] - buffer) : -1;
	_rebase(buffer, string);
	return result;
}

// Moves the match pointers from the working copy to the caller's string.
void Regexp::_rebase(const char *copy, const char *string)
{
	for (int i = 0; i < NSUBEXP; i++) {
		if (_regexp->startp[i])
			_regexp->startp[i] = (char *)string + (_regexp->startp[i] - copy);
		if (_regexp->endp[i])
			_regexp->endp[i] = (char *)string + (_regexp->endp[i] - copy);
	}
	_regexp->base = string;
}
