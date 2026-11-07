// CoordinateParser: splits a typed coordinate ("40°26'46\"N 79°58'56\"W",
// "-12.5, 130.8") into tokens of digits and symbols, and guesses its
// format. CoordinateParsingResult reads the coordinates from the tokens.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(CoordinateParser, 0)
LF_IMPLEMENT_CLASS(CoordinateParsingResult, 0)

// The character sets of the token types, made on first use.
static LCharSet *_sCoordinateStringSets[kCoordinateCharTypeCount];
static LCharSet *_sDMSNegativeWorldPartSet = 0;

class _coordinateParserCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		for (int i = 0; i < kCoordinateCharTypeCount; i++)
			if (_sCoordinateStringSets[i]) {
				_sCoordinateStringSets[i]->release();
				_sCoordinateStringSets[i] = 0;
			}
		if (_sDMSNegativeWorldPartSet) {
			_sDMSNegativeWorldPartSet->release();
			_sDMSNegativeWorldPartSet = 0;
		}
	}
};

static _coordinateParserCleanup _sCoordinateParserCleanup;

static LCharSet *_charSetWithString(LString *string)
{
	return LCharSet::_alloc()->initWithString(string);
}

static LCharSet *_charSetForType(int type)
{
	switch (type) {
	case kCoordinateCharDigit:
		return LRetain(LCharSet::charSetWithType(LCharSet::kDecimalDigit));
	case kCoordinateCharDegree:
		return _charSetWithString(Utils::geoCoordinatesDegreeDelimiter());
	case kCoordinateCharMinute:
		return _charSetWithString(Utils::geoCoordinatesMinuteDelimiter());
	case kCoordinateCharSecond:
		return _charSetWithString(Utils::geoCoordinatesSecondDelimiter());
	case kCoordinateCharWorldPart:
		return _charSetWithString(Utils::geoCoordinatesWorldPartCharacters(0));
	case kCoordinateCharSeparator:
		return LCharSet::_alloc()->initWithCString(".,");
	case kCoordinateCharMinus:
		return LCharSet::_alloc()->initWithCString("-");
	default:
		return 0;
	}
}

void CoordinateParser::dealloc()
{
	LObject::dealloc();
}

int CoordinateParser::determineCharType(UniChar c)
{
	for (int type = 1; type < kCoordinateCharTypeCount; type++) {
		if (_sCoordinateStringSets[type] == 0)
			_sCoordinateStringSets[type] = _charSetForType(type);
		if (_sCoordinateStringSets[type] && _sCoordinateStringSets[type]->characterIsMember(c))
			return type;
	}
	return kCoordinateCharNone;
}

// Runs of digits become one token (up to 10 digits); every other symbol is
// a token of its own. Degree, minute, second and world part symbols make
// the string DMS; digits alone, decimal.
CoordinateParsingResult *CoordinateParser::Parse(LString *string)
{
	LArray *tokens = LArray::array();
	LArray *types = LArray::array();
	const UniChar *chars = string->lowercaseString()->chars();
	int length = string->length();
	CoordinateStringFormat format = kCoordinateFormatUnknown;
	UniChar token[10];
	int tokenLength = 0;
	int previousType = kCoordinateCharNone;
	for (int i = 0; i < length + 1; i++) {
		UniChar c = i < length ? chars[i] : 0;
		int type = determineCharType(c);
		if (format == kCoordinateFormatDecimal || format == kCoordinateFormatUnknown) {
			if (type >= kCoordinateCharDegree && type <= kCoordinateCharWorldPart)
				format = kCoordinateFormatDMS;
			else if (type == kCoordinateCharDigit)
				format = kCoordinateFormatDecimal;
		}
		if (previousType == type) {
			if (tokenLength < 10 && previousType == kCoordinateCharDigit)
				token[tokenLength++] = c;
		} else {
			if (tokenLength > 0) {
				types->addObject(LValue::valueWithInt(previousType));
				tokens->addObject(LString::stringWithCharacters(token, tokenLength));
			}
			tokenLength = 0;
			if (type != kCoordinateCharNone)
				token[tokenLength++] = c;
		}
		previousType = type;
	}
	return (CoordinateParsingResult *)CoordinateParsingResult::_alloc()
	    ->initWithArraysAndFormat((TLArray<LString *> *)tokens, (TLArray<LValue *> *)types, format)
	    ->l_autorelease();
}

// ---------------------------------------------------------------------------
// CoordinateParsingResult

CoordinateParsingResult::CoordinateParsingResult()
    : _format(kCoordinateFormatUnknown), _index(0), _tokens(0), _types(0)
{
}

void CoordinateParsingResult::reset()
{
	_index = 0;
}

CoordinateStringFormat CoordinateParsingResult::getCoordinateFormat()
{
	return _format;
}

void CoordinateParsingResult::dealloc()
{
	if (_tokens) {
		_tokens->release();
		_tokens = 0;
	}
	if (_types) {
		_types->release();
		_types = 0;
	}
	LObject::dealloc();
}

bool CoordinateParsingResult::hasNextCoordinate()
{
	return _tokens && _index < (int)_tokens->count() - 1 && _format != kCoordinateFormatUnknown;
}

CoordinateParsingResult *CoordinateParsingResult::initWithArraysAndFormat(TLArray<LString *> *tokens,
                                                                          TLArray<LValue *> *types,
                                                                          CoordinateStringFormat format)
{
	_format = format;
	_tokens = LRetain(tokens);
	_types = LRetain(types);
	_index = 0;
	return this;
}

int CoordinateParsingResult::_typeAt(int index)
{
	return _types->objectAtIndex(index)->intValue();
}

// [-]digits[.digits]
TDegrees CoordinateParsingResult::getNextDecimalCoordinate()
{
	TDegrees result = { 0, 0, 0, false };
	if (_tokens == 0 || _types == 0)
		return result;
	bool negative = _typeAt(_index) == kCoordinateCharMinus;
	if (negative)
		_index++;
	int count = _tokens->count();
	LString *value = 0;
	bool separator = false;
	bool done = false;
	int i = _index;
	for (; !done && i < count; i++) {
		switch (_typeAt(i)) {
		case kCoordinateCharSeparator:
			done = separator;
			separator = !separator;
			break;
		case kCoordinateCharMinus:
			done = true;
			break;
		case kCoordinateCharDigit:
			if (value == 0)
				value = separator ? LString::stringWithFormat("0.%@", _tokens->objectAtIndex(i))
				                  : _tokens->objectAtIndex(i);
			else if (separator)
				value = LString::stringWithFormat("%@.%@", value, _tokens->objectAtIndex(i));
			else
				done = true;
			break;
		}
	}
	_index = i - 1;
	if (value)
		result = degreesFromFloat(value->floatValue());
	result.isNegative = negative;
	return result;
}

// Degrees, minutes and seconds by their symbols, and a world part letter.
TDegrees CoordinateParsingResult::getNextDMSCoordinate()
{
	TDegrees result = { 0, 0, 0, false };
	if (_tokens == 0 || _types == 0)
		return result;
	int count = _tokens->count();
	LString *value = 0;
	bool done = false;
	bool haveDegrees = false, haveMinutes = false, haveSeconds = false, haveWorldPart = false;
	int i = _index;
	for (; !done && i < count; i++) {
		int type = _typeAt(i);
		if (type == kCoordinateCharDigit) {
			if (value == 0) {
				value = _tokens->objectAtIndex(i);
				continue;
			}
			// A second number without a symbol: the first was degrees.
			i--;
			type = kCoordinateCharDegree;
		}
		switch (type) {
		case kCoordinateCharDegree:
			done = haveDegrees;
			if (!haveDegrees) {
				result.degrees = value ? (float)value->intValue() : 0.0f;
				haveDegrees = true;
				value = 0;
			}
			break;
		case kCoordinateCharMinute:
			done = haveMinutes;
			if (!haveMinutes) {
				result.minutes = value ? (float)value->intValue() : 0.0f;
				haveMinutes = true;
				value = 0;
			}
			break;
		case kCoordinateCharSecond:
			done = haveSeconds;
			if (!haveSeconds) {
				result.seconds = value ? (float)value->intValue() : 0.0f;
				haveSeconds = true;
				value = 0;
			}
			break;
		case kCoordinateCharWorldPart:
			done = haveWorldPart;
			if (!haveWorldPart) {
				if (_sDMSNegativeWorldPartSet == 0)
					_sDMSNegativeWorldPartSet = _charSetWithString(Utils::geoCoordinatesWorldPartCharacters(1));
				result.isNegative = _sDMSNegativeWorldPartSet->characterIsMember(_tokens->objectAtIndex(i)->chars()[0]);
				haveWorldPart = true;
				value = 0;
			}
			break;
		default:
			value = 0;
			break;
		}
	}
	_index = i - 1 - (value ? 1 : 0);
	return result;
}

TDegrees CoordinateParsingResult::getNextCoordinate()
{
	if (_format == kCoordinateFormatDMS)
		return getNextDMSCoordinate();
	if (_format == kCoordinateFormatDecimal)
		return getNextDecimalCoordinate();
	TDegrees none = { 0, 0, 0, false };
	return none;
}
