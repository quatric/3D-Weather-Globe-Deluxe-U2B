// SearchPreviewEntry: a row of the search results: an entry, a collapsed
// "see all" row for a kind of entries, or a row of plain text.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(SearchPreviewEntry, 0)

SearchPreviewEntry::SearchPreviewEntry()
    : _collapsed(false), _entryType(0), _count(0), _clickable(false), _firstRegionRow(false), _allNamesRow(false),
      _alternative(false), _emphasizeType(false), _showType(false), _entry(0), _title(0), _type(0)
{
}

SearchPreviewEntry *SearchPreviewEntry::initWithCollapsedInfo(int count, int entryType, unsigned char showType,
                                                              unsigned char alternative)
{
	if (!LObject::init())
		return 0;
	_alternative = alternative;
	_collapsed = true;
	_clickable = true;
	_count = count;
	_showType = showType;
	_entryType = entryType;
	return this;
}

void SearchPreviewEntry::dealloc()
{
	if (_entry) {
		_entry->release();
		_entry = 0;
	}
	if (_title) {
		_title->release();
		_title = 0;
	}
	if (_type) {
		_type->release();
		_type = 0;
	}
	LObject::dealloc();
}

GenericEntry *SearchPreviewEntry::geoEntry()
{
	return _entry;
}

bool SearchPreviewEntry::emphasizeTypeString()
{
	return _emphasizeType;
}

bool SearchPreviewEntry::isAlternative()
{
	return _alternative;
}

void SearchPreviewEntry::setFirstRegionRowFlag(unsigned char flag)
{
	_clickable = flag;
	_firstRegionRow = flag;
}

void SearchPreviewEntry::setAllNamesRowFlag(unsigned char flag)
{
	_clickable = flag;
	_allNamesRow = flag;
}

bool SearchPreviewEntry::isClickable()
{
	return _clickable;
}

// The search to run when the row is clicked.
LString *SearchPreviewEntry::customRequestForSubstring(LString *substring)
{
	if (_firstRegionRow)
		return SQL_DB_Strings::selectColumnsByRegion(substring, 0, true);
	if (_allNamesRow)
		return 0;
	LArray *pairs = AtlasDatabase::instance()->typePairsForEntryType(_entryType);
	if (_alternative)
		return SQL_DB_Strings::requestForGenericSearchInRegionsParams(pairs, substring);
	return SQL_DB_Strings::requestForGenericSearchWithNameParams(pairs, substring);
}

SearchPreviewEntry *SearchPreviewEntry::initWithUID(LString *uid, unsigned char showType, unsigned char alternative)
{
	if (!LObject::init())
		return 0;
	_alternative = alternative;
	_showType = showType;
	_entry = GenericEntry::entryWithUID(uid);
	if (_entry)
		_entry->retain();
	return this;
}

SearchPreviewEntry *SearchPreviewEntry::initWithStrings(LString *type, LString *title, unsigned char alternative)
{
	if (!LObject::init())
		return 0;
	_alternative = alternative;
	_type = type;
	if (type)
		type->retain();
	_title = title;
	if (title)
		title->retain();
	_emphasizeType = true;
	return this;
}

LString *SearchPreviewEntry::publicType()
{
	if (_type)
		return _type;
	if (!_showType)
		return 0;
	AtlasDatabase *db = AtlasDatabase::instance();
	if (!_collapsed) {
		if (_entry)
			_type = db->localizedEntryType(kEntryTypeUseCaseSearchPreview, _entry->ownType(0), _entry->altType(0), false);
	} else {
		_type = db->localizedEntryType(_entryType, false);
	}
	if (_type)
		_type->retain();
	return _type;
}

// "Name, State" or "Name, Country" for an entry; "See all N cities" for a
// collapsed row.
LString *SearchPreviewEntry::publicTitle()
{
	if (_entry == 0) {
		if (_title == 0 && _collapsed) {
			LString *format = LString::stringFromTable("See entries", "AppModel", (void *)getAppModelModule());
			LString *kind = AtlasDatabase::instance()->localizedEntryType(_entryType, true);
			_title = LString::_alloc()->initWithFormat(format, kind, _count);
		}
		return _title;
	}
	LString *region = _entry->stateName(0);
	if (region == 0)
		region = _entry->countryName(0);
	if (region == 0)
		return _entry->ownName(0);
	return LString::stringWithFormat("%@, %@", _entry->ownName(0), region);
}

SearchPreviewEntry *SearchPreviewEntry::entryWithStrings(LString *type, LString *title, unsigned char alternative)
{
	return (SearchPreviewEntry *)_alloc()->initWithStrings(type, title, alternative)->l_autorelease();
}

SearchPreviewEntry *SearchPreviewEntry::entryWithUID(LString *uid, unsigned char showType, unsigned char alternative)
{
	return (SearchPreviewEntry *)_alloc()->initWithUID(uid, showType, alternative)->l_autorelease();
}

SearchPreviewEntry *SearchPreviewEntry::entryWithCollapsedInfo(int count, int entryType, unsigned char showType,
                                                               unsigned char alternative)
{
	return (SearchPreviewEntry *)_alloc()->initWithCollapsedInfo(count, entryType, showType, alternative)->l_autorelease();
}
