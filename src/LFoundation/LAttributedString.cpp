// LAttributedString
//
// LF::LAttributedString.

#include "LAttributedString.h"
#include "LNull.h"
#include <string.h>

namespace LF {

LF_IMPLEMENT_CLASS(LAttributedString, 0)

int LAttributedString::_runSearchCallback(const void *charIndex, const void *run, const void *)
{
	uintptr_t index = (uintptr_t)charIndex;
	const LRange *r = (const LRange *)run;
	if (index < r->location)
		return -1;
	return index >= r->location + r->length ? 1 : 0;
}

static TArray<LRange> *_newRuns()
{
	TArray<LRange> *runs = new TArray<LRange>;
	runs->setComparator(0, LAttributedString::_runSearchCallback);
	return runs;
}

void LAttributedString::_invalidatePlatformObject()
{
	if (_platformObject) {
		_platformObject->release();
		_platformObject = 0;
	}
	_platformKind = 0;
}

LAttributedString *LAttributedString::init()
{
	if (LObject::init() == 0)
		return 0;
	_string = LString::_alloc()->init();
	_runs = _newRuns();
	_runs->addObject(LMakeRange(0, 0));
	LMap *attributes = LMap::_alloc()->init();
	_attributes = LArray::_alloc()->initWithObjects(attributes, (LObject *)0);
	attributes->release();
	return this;
}

LAttributedString *LAttributedString::initWithString(LString *string, LMap *attributes)
{
	if (LObject::init() == 0)
		return 0;
	_string = LString::_alloc();
	_string->setValue(string);
	_runs = _newRuns();
	_runs->addObject(LMakeRange(0, _string->length()));
	bool created = attributes == 0;
	if (created)
		attributes = LMap::_alloc()->init();
	_attributes = LArray::_alloc()->initWithObjects(attributes, (LObject *)0);
	if (created)
		attributes->release();
	return this;
}

LAttributedString *LAttributedString::initWithAttributedString(const LAttributedString *other)
{
	if (LObject::init() == 0)
		return 0;
	setAttributedString(other);
	return this;
}

LAttributedString *LAttributedString::copy() const
{
	return _alloc()->initWithAttributedString(this);
}

void LAttributedString::setAttributedString(const LAttributedString *other)
{
	if (_string) {
		_string->release();
		_string = 0;
	}
	if (_attributes) {
		_attributes->release();
		_attributes = 0;
	}
	delete _runs;
	_runs = 0;
	if (other && other->_string)
		_string = other->_string->copy();
	else
		_string = LString::_alloc()->init();
	if (other && other->_runs)
		_runs = new TArray<LRange>(*other->_runs);
	else
		_runs = new TArray<LRange>;
	_runs->setComparator(0, _runSearchCallback);
	if (other && other->_attributes)
		_attributes = other->_attributes->copy();
	else
		_attributes = LArray::_alloc()->init();
	_invalidatePlatformObject();
}

void LAttributedString::dealloc()
{
	if (_string) {
		_string->release();
		_string = 0;
	}
	delete _runs;
	_runs = 0;
	if (_attributes) {
		_attributes->release();
		_attributes = 0;
	}
	if (_platformObject) {
		_platformObject->release();
		_platformObject = 0;
	}
	LObject::dealloc();
}

LString *LAttributedString::description() const
{
	int n = _attributes->count();
	LString *str = LString::stringWithFormat("LAttributedString <0x%08X>; '%@' runs count='%d', length='%d'\n",
	    (unsigned int)(uintptr_t)this, _string, n, _string->length());
	for (int i = 0; i < n; i++) {
		const LRange &r = _run(i);
		str->appendFormat("(%d..%d) - %@\n", r.location, r.location + r.length, _attributesOfRun(i));
	}
	return str;
}

bool LAttributedString::isEqualTo(LObject *other)
{
	if (other == 0 || !other->getClass()->isSubclassOf(classObject()))
		return LObject::isEqualTo(other);
	LAttributedString *o = (LAttributedString *)other;
	if (o->_runs->count() != _runs->count() ||
	    memcmp(o->_runs->bytes(), _runs->bytes(), _runs->count() * _runs->elementSize()) != 0)
		return false;
	return _string->isEqualTo(o->_string) && _attributes->isEqualTo(o->_attributes);
}

LObject *LAttributedString::platformDependedObject(int kind, TPlatformObjectFactory factory)
{
	if (_platformObject) {
		if (kind == _platformKind)
			return _platformObject;
		_platformObject->release();
		_platformObject = 0;
	}
	_platformKind = kind;
	_platformObject = factory(this);
	return _platformObject;
}

int LAttributedString::_runIndexFromCharIndex(unsigned int index) const
{
	return _runs->insertIndexForKey((const void *)(uintptr_t)index, true);
}

LRange LAttributedString::_runsRangeFromCharsRange(LRange range) const
{
	if (range.length == 0 && range.location == 0)
		return LMakeRange(0, 0);
	int first = _runIndexFromCharIndex(range.location);
	int last = _runIndexFromCharIndex(range.location + range.length - 1);
	if (last >= (int)_runs->count())
		last = _runs->count() - 1;
	if (last < first)
		last = first;
	return LMakeRange(first, last - first + 1);
}

void LAttributedString::_mergeRuns(unsigned int first, unsigned int last)
{
	unsigned int start = _run(first).location;
	unsigned int end = _run(last).location + _run(last).length;
	_runs->removeObjectsInRange(LMakeRange(first + 1, last - first));
	_attributes->removeObjectsInRange(LMakeRange(first + 1, last - first));
	_run(first) = LMakeRange(start, end - start);
}

void LAttributedString::_optimize(LRange runs)
{
	LMap *current = 0;
	int groupStart = runs.location;
	int groupEnd = runs.location;
	int count = runs.length;
	int end = runs.location + count;
	for (int i = runs.location; i < end; i++) {
		int next = groupEnd;
		if (_run(i).length == 0) {
			_runs->removeObjectAtIndex(i);
			_attributes->removeObjectAtIndex(i);
			count--;
			end = runs.location + count;
			i--;
		} else if (current == 0) {
			current = _attributesOfRun(i);
		} else {
			next = i;
			if (!current->isEqualTo(_attributesOfRun(i))) {
				if (groupStart < groupEnd) {
					int merged = groupEnd - groupStart;
					_mergeRuns(groupStart, groupEnd);
					count -= merged;
					end = runs.location + count;
					i = groupStart;
				}
				current = _attributesOfRun(i);
				groupStart = i;
				next = i;
			}
		}
		groupEnd = next;
	}
	if (groupStart < groupEnd)
		_mergeRuns(groupStart, groupEnd);
}

LRange LAttributedString::_splitRunsToCharsRange(LRange range, bool copyAttributes)
{
	LRange runs = _runsRangeFromCharsRange(range);
	if (runs.length == 0)
		return LMakeRange(0, 0);
	int first = runs.location;
	LMap *leftAttributes = 0;
	if (_run(first).location < range.location) {
		unsigned int cut = range.location - _run(first).location;
		LRange right = LMakeRange(range.location, _run(first).length - cut);
		_run(first).length = cut;
		leftAttributes = _attributesOfRun(first);
		first++;
		_runs->insertObjectAtIndex(right, first);
		LObject *attributes = copyAttributes ? LAutorelease(leftAttributes->copy()) : (LObject *)LNull::null;
		_attributes->insertObjectAtIndex(attributes, first);
	}
	int endIndex = first + runs.length;
	unsigned int endPos = range.location + range.length;
	LRange &lastRun = _run(endIndex - 1);
	unsigned int runEnd = lastRun.location + lastRun.length;
	if (runEnd > endPos) {
		lastRun.length = endPos - lastRun.location;
		_runs->insertObjectAtIndex(LMakeRange(endPos, runEnd - endPos), endIndex);
		LObject *attributes = _attributesOfRun(endIndex - 1);
		if (copyAttributes)
			attributes = LAutorelease(((LMap *)attributes)->copy());
		else if (attributes == LNull::null)
			attributes = LAutorelease(leftAttributes->copy());
		_attributes->insertObjectAtIndex(attributes, endIndex);
	}
	return LMakeRange(first, runs.length);
}

// The runs to look at for merging after an edit of `runs`: one more on each
// side where there is one.
static LRange _neighborhood(LRange runs, unsigned int count)
{
	if (runs.location != 0) {
		runs.location--;
		runs.length++;
	}
	if (runs.location + runs.length < count)
		runs.length++;
	return runs;
}

LMap *LAttributedString::attributesAtIndex(unsigned int index, LRange *range)
{
	int i = _runIndexFromCharIndex(index);
	if (i < 0 || i >= (int)_attributes->count())
		LF_RAISE(WrongConditionException());
	if (range)
		*range = _run(i);
	return _attributesOfRun(i);
}

LObject *LAttributedString::attributeAtIndex(LString *name, unsigned int index, LRange *effectiveRange)
{
	int i = _runIndexFromCharIndex(index);
	if (i < 0 || i >= (int)_attributes->count())
		LF_RAISE(WrongConditionException());
	LMap *attributes = _attributesOfRun(i);
	LObject *value = attributes ? attributes->objectForKey(name) : 0;
	if (effectiveRange == 0)
		return value;
	if (value == 0) {
		*effectiveRange = LZeroRange;
		return 0;
	}
	int first = i;
	while (first > 0) {
		LObject *v = _attributesOfRun(first - 1)->objectForKey(name);
		if (v == 0 || !v->isEqualTo(value))
			break;
		first--;
	}
	int last = i;
	while (last < (int)_attributes->count() - 1) {
		LObject *v = _attributesOfRun(last + 1)->objectForKey(name);
		if (v == 0 || !v->isEqualTo(value))
			break;
		last++;
	}
	effectiveRange->location = _run(first).location;
	effectiveRange->length = _run(last).location + _run(last).length - _run(first).location;
	return value;
}

LMap *LAttributedString::attributesAtIndexWithLongestRange(unsigned int index, LRange *range)
{
	int i = _runIndexFromCharIndex(index);
	if (range)
		*range = _run(i);
	return _attributesOfRun(i);
}

LMap *LAttributedString::attributesAtIndexWithLongestRange(unsigned int index, LRange *range, LRange limit)
{
	int i = _runIndexFromCharIndex(index);
	if (range) {
		*range = _run(i);
		if (range->location < limit.location) {
			unsigned int cut = limit.location - range->location;
			range->length = range->length < cut ? 0 : range->length - cut;
			range->location = limit.location;
		}
		int over = (int)(range->location + range->length - (limit.location + limit.length));
		if (over > 0) {
			if (range->length < (unsigned int)over) {
				range->length = 0;
				range->location = limit.location + limit.length;
			} else {
				range->length -= over;
			}
		}
	}
	return _attributesOfRun(i);
}

LAttributedString *LAttributedString::attributedSubstringFromRange(LRange range)
{
	LAttributedString *result = LAutorelease(_alloc()->init());
	if (result->_string) {
		result->_string->release();
		result->_string = 0;
	}
	result->_runs->removeAllObjects();
	result->_attributes->removeAllObjects();
	result->_string = LRetain(_string->substringWithRange(range.location, range.length));

	LRange runs = _runsRangeFromCharsRange(range);
	int last = runs.location;
	if (runs.length <= 1) {
		result->_runs->addObject(LMakeRange(0, range.length));
	} else {
		const LRange &firstRun = _run(runs.location);
		unsigned int pos = firstRun.length - (range.location - firstRun.location);
		result->_runs->addObject(LMakeRange(0, pos));
		LMap *attributes = _attributesOfRun(runs.location)->copy();
		result->_attributes->addObject(attributes);
		attributes->release();
		last = runs.location + runs.length - 1;
		for (int i = runs.location + 1; i < last; i++) {
			result->_runs->addObject(LMakeRange(pos, _run(i).length));
			pos += _run(i).length;
			attributes = _attributesOfRun(i)->copy();
			result->_attributes->addObject(attributes);
			attributes->release();
		}
		result->_runs->addObject(LMakeRange(pos, range.length - pos));
	}
	LMap *attributes = _attributesOfRun(last)->copy();
	result->_attributes->addObject(attributes);
	attributes->release();
	return result;
}

void LAttributedString::setAttributes(LMap *attributes, LRange range)
{
	LRange runs = _splitRunsToCharsRange(range, false);
	if (attributes == 0)
		attributes = LMap::map();
	for (unsigned int i = runs.location; i < runs.location + runs.length; i++)
		_attributes->replaceObjectAtIndex(attributes, i);
	_optimize(_neighborhood(runs, _runs->count()));
	_invalidatePlatformObject();
}

void LAttributedString::addAttribute(LString *name, LObject *value, LRange range)
{
	LRange runs = _splitRunsToCharsRange(range, true);
	for (unsigned int i = runs.location; i < runs.location + runs.length; i++)
		_attributesOfRun(i)->setObjectForKey(value, name);
	_optimize(_neighborhood(runs, _runs->count()));
	_invalidatePlatformObject();
}

void LAttributedString::addAttributes(LMap *attributes, LRange range)
{
	LRange runs = _splitRunsToCharsRange(range, true);
	LMapIterator it(attributes, true);
	while (it.next())
		for (unsigned int i = runs.location; i < runs.location + runs.length; i++)
			_attributesOfRun(i)->setObjectForKey(it.object(), it.key());
	_optimize(_neighborhood(runs, _runs->count()));
	_invalidatePlatformObject();
}

void LAttributedString::removeAttribute(LString *name, LRange range)
{
	LRange runs = _splitRunsToCharsRange(range, true);
	for (unsigned int i = runs.location; i < runs.location + runs.length; i++)
		_attributesOfRun(i)->removeObjectForKey(name);
	_optimize(_neighborhood(runs, _runs->count()));
	_invalidatePlatformObject();
}

void LAttributedString::replaceCharactersInRange(LRange range, const LString *str)
{
	int delta = (int)str->length() - (int)range.length;
	unsigned int end = range.location + range.length;
	unsigned int newEnd = end + delta;
	int last = _runIndexFromCharIndex(range.length == 0 ? end : end - 1);
	int first = last;
	if (delta < 0)
		first = _runIndexFromCharIndex(newEnd);
	bool merge = false;
	if (first == last || delta >= 0) {
		_run(last).length += delta;
	} else {
		// The text left of the edit ends in the first run, the rest of the
		// last run follows it.
		_run(first).length = newEnd - _run(first).location;
		LRange &lastRun = _run(last);
		lastRun.length = lastRun.location + lastRun.length - end;
		lastRun.location = newEnd;
		merge = true;
		int between = last - first - 1;
		if (between != 0) {
			_runs->removeObjectsInRange(LMakeRange(first + 1, between));
			_attributes->removeObjectsInRange(LMakeRange(first + 1, between));
			last -= between;
		}
	}
	if (delta != 0)
		for (int i = last + 1; i < (int)_runs->count(); i++)
			_run(i).location += delta;
	if (merge)
		_optimize(LMakeRange(first, last - first + 1));
	_string->replaceCharactersInRange(range.location, range.length, str);
	_invalidatePlatformObject();
}

void LAttributedString::replaceCharactersInRangeWithAttributedString(LRange range, const LAttributedString *str)
{
	LRange runs = _splitRunsToCharsRange(range, false);
	if (runs.length) {
		_runs->removeObjectsInRange(runs);
		_attributes->removeObjectsInRange(runs);
	}
	int n = str->_attributes->count();
	for (int i = 0; i < n; i++) {
		LRange r = str->_run(i);
		r.location += range.location;
		_runs->insertObjectAtIndex(r, runs.location + i);
		_attributes->insertObjectAtIndex(str->_attributesOfRun(i), runs.location + i);
	}
	// The runs after the inserted ones move by the change in length. (The
	// original started counting at n + 1 rather than after the inserted runs.)
	int delta = (int)str->_string->length() - (int)range.length;
	for (int i = runs.location + n; i < (int)_runs->count(); i++)
		_run(i).location += delta;
	_optimize(_neighborhood(LMakeRange(runs.location, n), _runs->count()));
	_string->replaceCharactersInRange(range.location, range.length, str->_string);
	_invalidatePlatformObject();
}

void LAttributedString::insertAttributedStringAtIndex(LAttributedString *str, unsigned int index)
{
	if (index < _string->length())
		replaceCharactersInRangeWithAttributedString(LMakeRange(index, 0), str);
	else
		appendAttributedString(str);
	_invalidatePlatformObject();
}

void LAttributedString::appendAttributedString(LAttributedString *str)
{
	int oldCount = _attributes->count();
	int n = str->_attributes->count();
	unsigned int length = _string->length();
	for (int i = 0; i < n; i++) {
		_attributes->addObject(str->_attributesOfRun(i));
		LRange r = str->_run(i);
		r.location += length;
		_runs->addObject(r);
	}
	_optimize(LMakeRange(oldCount - 1, 2));
	_string->appendString(str->_string);
	_invalidatePlatformObject();
}

void LAttributedString::deleteCharactersInRange(LRange range)
{
	if (range.location == 0 && range.length == _string->length()) {
		if (_attributes->count() > 1) {
			_attributes->removeAllObjectsAfter(0);
			_runs->removeAllObjectsAfter(0);
		}
		_run(0) = LZeroRange;
		_string->setValue(LString::emptyString());
		return;
	}
	LRange runs = _splitRunsToCharsRange(range, false);
	if (runs.length) {
		_runs->removeObjectsInRange(runs);
		_attributes->removeObjectsInRange(runs);
	}
	for (int i = runs.location; i < (int)_runs->count(); i++)
		_run(i).location -= range.length;
	_optimize(_neighborhood(LMakeRange(runs.location, 1), _runs->count()));
	_string->eraseSubstringInRange(range.location, range.length);
	_invalidatePlatformObject();
}

} // namespace LF
