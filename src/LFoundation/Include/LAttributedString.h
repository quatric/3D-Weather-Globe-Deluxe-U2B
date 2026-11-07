// LAttributedString: a string with attribute maps over runs of characters.
// _runs and _attributes are parallel: run i (a character range) has the map
// _attributes[i]. Runs are kept contiguous; edits split runs and then merge
// neighbors whose maps are equal. A platform object (such as an
// NSAttributedString) made from it can be cached until it changes.
#pragma once

#include "LMap.h"
#include "LArray.h"

namespace LF {

class LAttributedString : public LObject {
	LF_DECLARE_CLASS(LAttributedString)
public:
	typedef LObject *(*TPlatformObjectFactory)(LAttributedString *);

	LAttributedString() : _string(0), _runs(0), _attributes(0), _platformKind(0), _platformObject(0) {}
	virtual ~LAttributedString() {}
	virtual LAttributedString *init();
	virtual LAttributedString *copy() const;
	virtual LString *description() const;
	virtual void dealloc();
	virtual LAttributedString *getOwnerLObject() { return this; }
	using LObject::isEqualTo;
	virtual bool isEqualTo(LObject *other);

	LAttributedString *initWithString(LString *string, LMap *attributes);
	LAttributedString *initWithAttributedString(const LAttributedString *other);
	void setAttributedString(const LAttributedString *other);

	LString *string() const { return _string; }
	unsigned int length() const { return _string->length(); }

	LMap *attributesAtIndex(unsigned int index, LRange *range);
	LObject *attributeAtIndex(LString *name, unsigned int index, LRange *effectiveRange);
	// The run's attributes; the range is only the run's.
	LMap *attributesAtIndexWithLongestRange(unsigned int index, LRange *range);
	LMap *attributesAtIndexWithLongestRange(unsigned int index, LRange *range, LRange limit);
	LAttributedString *attributedSubstringFromRange(LRange range);

	void setAttributes(LMap *attributes, LRange range);
	void addAttribute(LString *name, LObject *value, LRange range);
	void addAttributes(LMap *attributes, LRange range);
	void removeAttribute(LString *name, LRange range);

	void replaceCharactersInRange(LRange range, const LString *str);
	void replaceCharactersInRangeWithAttributedString(LRange range, const LAttributedString *str);
	void insertAttributedStringAtIndex(LAttributedString *str, unsigned int index);
	void appendAttributedString(LAttributedString *str);
	void deleteCharactersInRange(LRange range);

	// The cached object of the kind, made by factory when missing or of
	// another kind.
	LObject *platformDependedObject(int kind, TPlatformObjectFactory factory);

	static int _runSearchCallback(const void *charIndex, const void *run, const void *context);
	int _runIndexFromCharIndex(unsigned int index) const;
	LRange _runsRangeFromCharsRange(LRange range) const;
	// Splits the runs at the range's ends; returns the runs inside. The new
	// runs get copies of the maps if asked, else placeholders.
	LRange _splitRunsToCharsRange(LRange range, bool copyAttributes);
	// Drops empty runs and merges equal neighbors among the runs given.
	void _optimize(LRange runs);
	void _mergeRuns(unsigned int first, unsigned int last);
	void _invalidatePlatformObject();

	LRange &_run(int i) const { return *(LRange *)_runs->CArray::objectAtIndex(i); }
	LMap *_attributesOfRun(int i) const { return (LMap *)_attributes->objectAtIndex(i); }

	LString *_string;
	TArray<LRange> *_runs;
	LArray *_attributes;	// of LMap
	int _platformKind;
	LObject *_platformObject;
};

} // namespace LF
