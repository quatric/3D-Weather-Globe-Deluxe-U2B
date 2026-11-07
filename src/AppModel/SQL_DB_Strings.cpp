// SQL_DB_Strings: the SQL the atlas runs against its databases. User objects
// have 21 character UIDs and live in the user tables; the shipped atlas marks
// nothing deleted (DEL is NULL), the user tables mark deletions with DEL=1.

#include "AppModel.h"

static inline bool isUserUID(LString *uid)
{
	return uid->length() == 21;
}

LString *SQL_DB_Strings::selectGeoObjectWithUID(LString *uid)
{
	bool user = isUserUID(uid);
	LString *notDeleted = LString::stringWithFormat(user ? "%@=0" : "%@ ISNULL", kColumnNameDEL);
	return LString::stringWithFormat("SELECT * FROM %@ WHERE %@='%@' AND %@",
	                                 user ? kUserGeoObjectsTableName : kGeoObjectsTableName, kColumnNameUID, uid,
	                                 notDeleted);
}

LString *SQL_DB_Strings::selectGeoObjectWithBondUID(unsigned char user)
{
	LString *notDeleted = LString::stringWithFormat(user ? "%@=0" : "%@ ISNULL", kColumnNameDEL);
	return LString::stringWithFormat("SELECT * FROM %@ WHERE %@=:UID_VAL AND %@",
	                                 user ? kUserGeoObjectsTableName : kGeoObjectsTableName, kColumnNameUID,
	                                 notDeleted);
}

static LString *_selectColumnsForNameSubstring(LString *columns, LString *substring, unsigned char user)
{
	LString *request;
	LString *table = user ? kUserGeoObjectsTableName : kGeoObjectsTableName;
	if (substring == 0 || substring->length() == 0)
		request = LString::stringWithFormat("SELECT %@ FROM %@ WHERE (1)", columns, table);
	else
		request = LString::stringWithFormat("SELECT %@ FROM %@ WHERE (%@ like '%@%%' OR %@ like '%%;%@%%')", columns,
		                                    table, kColumnNameName, substring, kColumnNameAltNames, substring);
	if (user)
		request->appendFormat(" AND %@=0", kColumnNameDEL);
	return request;
}

static LString *_selectGeoObjectOfTypesWithSubstring(LArray *types, LString *substring)
{
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@ IN (%@) AND (%@ LIKE '%@%%' OR %@ LIKE '%%;%@%%')",
	                                 kColumnNameUID, kGeoObjectsTableName, kColumnNameType,
	                                 SQLiteUtils::stringAsSQLValuesList(types), kColumnNameName, substring,
	                                 kColumnNameAltNames, substring);
}

static LString *_columnsListForGenericSearch()
{
	return LString::stringWithFormat(
	    "%@, lower(%@) as %@, %@, %@, %@, %@, 0 as state_name_pointer, %@, 0 as country_name_pointer, %@",
	    kColumnNameUID, kColumnNameName, kColumnNameName, kColumnNamePopulation, kColumnNameType, kColumnNameAltType,
	    kColumnNameHostCountyUID, kColumnNameHostStateUID, kColumnNameHostCountryUID);
}

static LString *_columnsListForPreviewSearch()
{
	return LString::stringWithFormat("%@, %@, %@, %@, %@", kColumnNameUID, kColumnNameName, kColumnNamePopulation,
	                                 kColumnNameType, kColumnNameAltType);
}

static LString *_orderListForPreviewSearch()
{
	return LString::stringWithFormat("%@, %@, %@, %@", kColumnNameType, kColumnNameAltType, kColumnNamePopulation,
	                                 kColumnNameName);
}

LString *SQL_DB_Strings::selectAbbreviations()
{
	return LString::stringWithFormat(
	    "SELECT %@, lower(%@) AS %@ FROM %@ WHERE %@='%@' AND length(%@)<15 UNION SELECT %@, lower(%@) AS %@ FROM %@ "
	    "WHERE %@='%@' AND length(%@)<15 UNION SELECT %@, lower(%@) AS %@ FROM %@",
	    kColumnNameUID, kColumnNameName, kColumnNameAbbr, kGeoObjectsTableName, kColumnNameType, kGeoTypeCountry,
	    kColumnNameName, kColumnNameUID, kColumnNameName, kColumnNameAbbr, kGeoObjectsTableName, kColumnNameType,
	    kGeoTypeState, kColumnNameName, kColumnNameUID, kColumnNameAbbr, kColumnNameAbbr, kAbbrTableName);
}

LString *SQL_DB_Strings::selectTopCitiesForWeatherOnMap()
{
	return LString::stringWithFormat(
	    "SELECT %@, %@, %@ FROM %@ WHERE %@ = '%@' AND (%@>=200000 OR (%@ NOTNULL and %@!='cntyseat')) AND %@ ISNULL "
	    "order by %@ desc, %@",
	    kColumnNameUID, kColumnNameLong, kColumnNameLat, kGeoObjectsTableName, kColumnNameType, kGeoTypeCity,
	    kColumnNamePopulation, kColumnNameAltType, kColumnNameAltType, kColumnNameDEL, kColumnNamePopulation,
	    kColumnNameAltType);
}

LString *SQL_DB_Strings::selectAllCoordsUIDsKindsForPoints()
{
	return LString::stringWithFormat(
	    "SELECT %@, %@, %@, %@, %@ FROM %@ WHERE %@ NOTNULL AND %@ NOTNULL AND %@ ISNULL UNION ALL SELECT %@, %@, %@, "
	    "%@, %@ FROM %@ WHERE %@ NOTNULL AND %@ NOTNULL AND %@=0",
	    kColumnNameLong, kColumnNameLat, kColumnNameUID, kColumnNameType, kColumnNameAltType, kGeoObjectsTableName,
	    kColumnNameLat, kColumnNameLong, kColumnNameDEL, kColumnNameLong, kColumnNameLat, kColumnNameUID,
	    kColumnNameType, kColumnNameAltType, kUserGeoObjectsTableName, kColumnNameLat, kColumnNameLong,
	    kColumnNameDEL);
}

// The kind of point (for the map's point sets) from the first letters of the
// entry's type and alternative type.
int SQL_DB_Strings::pointEntryTypeFromTypeAndAltType(const char *type, const char *altType)
{
	unsigned int key = (unsigned int)(int)type[3] | (unsigned int)(int)type[2] << 8 |
	                   (unsigned int)(int)type[1] << 16 | (unsigned int)(int)type[0] << 24;
	switch (key) {
	case 'appl':
		switch (altType[0]) {
		case '\0':
		case 'r':
			return 8;
		case 'm':
			return 10;
		case 'c':
			return 11;
		}
		return 0;
	case 'city':
		if (altType[0] == '\0')
			return 1;
		if (altType[3] == 'y')
			return 4;
		if (altType[3] == 't')
			return 3;
		if (altType[5] != 'c' && altType[5] != 's')
			return 5;
		return 2;
	case 'lago':
	case 'bay\0':
	case 'fjor':
	case 'gulf':
	case 'stra':
	case 'wate':
	case 'ocea':
	case 'sea\0':
	case 'lake':
		return 5;
	case 'coun':
	case 'cont':
	case 'dese':
	case 'hill':
	case 'stat':
	case 'volc':
	case 'moun':
		return 6;
	case 'user':
		return 7;
	case 'bldn':
	case 'enda':
	case 'zoo\0':
	case 'nat_':
		return 9;
	}
	return 0;
}

LString *SQL_DB_Strings::selectNameForGeoObjectWithUID(LString *uid)
{
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@='%@'", kColumnNameName, kGeoObjectsTableName,
	                                 kColumnNameUID, uid);
}

LString *SQL_DB_Strings::selectCitiesUIDsWithCityName(LString *name)
{
	LString *value = SQLiteUtils::stringByEncodeAsSQLValueWOBracers(name);
	return LString::stringWithFormat(
	    "SELECT %@ FROM %@ WHERE %@='%@' AND (%@ LIKE '%@%%' OR %@ LIKE '%%;%@%%') ORDER BY %@", kColumnNameUID,
	    kGeoObjectsTableName, kColumnNameType, kGeoTypeCity, kColumnNameName, value, kColumnNameAltNames, value,
	    kColumnNameName);
}

LString *SQL_DB_Strings::selectFilenameFromUpdatesThatIsLike(LString *pattern)
{
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@ like '%@'", kColumnNameUpdateFileName,
	                                 kUpdatesTableName, kColumnNameUpdateFileName, pattern);
}

LString *SQL_DB_Strings::insertFilenameIntoUpdates(LString *name)
{
	return LString::stringWithFormat("INSERT INTO %@ (%@) VALUES ('%@')", kUpdatesTableName, kColumnNameUpdateFileName,
	                                 name);
}

LString *SQL_DB_Strings::selectAllGeoObjUIDsWithBLOBOfType(LString *repType)
{
	return LString::stringWithFormat("SELECT DISTINCT %@ FROM %@ where %@='%@'", kColumnNameObjectUID,
	                                 kGeoDataTableName, kColumnNameRepType, repType);
}

LString *SQL_DB_Strings::selectBLOBUIDForEntryInfoKeysOrder()
{
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@='%@'", kColumnNameBLOBUid, kGeoDataTableName,
	                                 kColumnNameRepType, kRepTypeEntryInfoKeysOrder);
}

LString *SQL_DB_Strings::selectBLOBUIDForGrid(LString *repType)
{
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@='%@'", kColumnNameBLOBUid, kGeoDataTableName,
	                                 kColumnNameRepType, repType);
}

LString *SQL_DB_Strings::selectBLOBKindWithUID(LString *uid)
{
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@='%@' AND %@=0", kColumnNameKind,
	                                 isUserUID(uid) ? kUserBLOBsTableName : kBLOBsTableName, kColumnNameUID, uid,
	                                 kColumnNameDEL);
}

LString *SQL_DB_Strings::selectBLOBDataWithUID(LString *uid)
{
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@='%@' AND %@=0", kColumnNameBLOBData,
	                                 isUserUID(uid) ? kUserBLOBsTableName : kBLOBsTableName, kColumnNameUID, uid,
	                                 kColumnNameDEL);
}

// User notes always live in the user tables.
LString *SQL_DB_Strings::selectBLOBUIDWithRepresentationOfType(LString *repType, LString *objectUID)
{
	bool user = repType == kRepTypeUserNotes || isUserUID(objectUID);
	LString *notDeleted = LString::stringWithFormat(user ? "%@=0" : "%@ ISNULL", kColumnNameDEL);
	return LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@='%@' AND %@='%@' AND %@", kColumnNameBLOBUid,
	                                 user ? kUserGeoDataTableName : kGeoDataTableName, kColumnNameRepType, repType,
	                                 kColumnNameObjectUID, objectUID, notDeleted);
}

LString *SQL_DB_Strings::selectAllObjectUIDsWithGeotypes(LArray *types, LString *repType)
{
	return LString::stringWithFormat(
	    "SELECT %@.%@ %@ FROM %@, %@ WHERE %@ IN (%@) AND %@.%@=%@.%@ AND %@.%@='%@'", kGeoObjectsTableName,
	    kColumnNameUID, kColumnNameObjectUID, kGeoObjectsTableName, kGeoDataTableName, kColumnNameType,
	    SQLiteUtils::stringAsSQLValuesList(types), kGeoDataTableName, kColumnNameObjectUID, kGeoObjectsTableName,
	    kColumnNameUID, kGeoDataTableName, kColumnNameRepType, repType);
}

LString *SQL_DB_Strings::selectSolarEclipses()
{
	return LString::stringWithFormat("SELECT %@, %@, %@, %@, %@, %@ FROM %@ ORDER BY %@", kStarsColumnNameSolarEclYMD,
	                                 kStarsColumnNameSolarEclHM, kStarsColumnNameSolarEclType,
	                                 kStarsColumnNameSolarEclMag, kStarsColumnNameSolarEclLat,
	                                 kStarsColumnNameSolarEclLong, kStarsTableNameSolarEclipses,
	                                 kStarsColumnNameSolarEclYMD);
}

LString *SQL_DB_Strings::stars_selectConstellationsStars()
{
	return LString::stringWithFormat(
	    "SELECT %@, %@, %@, %@ FROM (SELECT %@, %@, %@, %@, %@ FROM %@ WHERE %@ IN (SELECT %@ FROM %@)) A, %@ B WHERE "
	    "A.%@=B.%@ ORDER BY B.rowid",
	    kStarsColumnNameRA, kStarsColumnNameDEC, kStarsColumnNameBVCOL, kStarsColumnNameMAG, kStarsColumnNameHIP,
	    kStarsColumnNameRA, kStarsColumnNameDEC, kStarsColumnNameBVCOL, kStarsColumnNameMAG, kStarsTableNameStars,
	    kStarsColumnNameHIP, kConstellationColumnNameHipIndex, kConstellationTableNameInds,
	    kConstellationTableNameInds, kStarsColumnNameHIP, kConstellationColumnNameHipIndex);
}

LString *SQL_DB_Strings::stars_selectConstellationsInds()
{
	return LString::stringWithFormat("SELECT %@, %@, %@ FROM %@ A, %@ B WHERE A.%@=B.%@ ORDER BY %@, A.rowid",
	                                 kConstellationColumnNameLineStart, kConstellationColumnNameLineEnd,
	                                 kConstellationColumnNameName, kConstellationTableNameLines,
	                                 kConstellationTableNameNames, kConstellationColumnNameAbbr,
	                                 kConstellationColumnNameAbbr, kConstellationColumnNameName);
}

LString *SQL_DB_Strings::selectEntryTypeRemap(LString *useCase)
{
	return LString::stringWithFormat("SELECT %@, %@, %@, %@, %@ FROM %@ WHERE %@='%@' ORDER BY %@", kColumnNameType,
	                                 kColumnNameAltType, kColumnNameSingular, kColumnNamePlural, kColumnNameWeight,
	                                 kEntryTypeRemapTableName, kColumnNameUseCase, useCase, kColumnNameWeight);
}

LString *SQL_DB_Strings::selectEntryTypeRemapMaxCount()
{
	return LString::stringWithFormat("SELECT DISTINCT %@, %@, %@, %@, %@ FROM %@", kColumnNameType, kColumnNameAltType,
	                                 kColumnNameSingular, kColumnNamePlural, kColumnNameWeight,
	                                 kEntryTypeRemapTableName);
}

LString *SQL_DB_Strings::selectEntryTypeRemapMaxCount_SAFE()
{
	return LString::stringWithFormat("SELECT DISTINCT %@, %@ FROM %@ ORDER BY %@ DESC, %@", kColumnNameType,
	                                 kColumnNameAltType, kGeoObjectsTableName, kColumnNameType, kColumnNameAltType);
}

static LString *_simpleCleanupRequestForTable(LString *table)
{
	return LString::stringWithFormat("DELETE FROM %@ WHERE %@=1", table, kColumnNameDEL);
}

LString *SQL_DB_Strings::selectUserDBRevision()
{
	return LString::stringWithFormat("SELECT * FROM %@", kUserTableNameRevision);
}

// The top level nodes have no parent.
LString *SQL_DB_Strings::selectRouteNodesWithParentUID(LString *parentUID, unsigned char user)
{
	LString *table = user ? kUserRouteNodesTableName : kRouteNodesTableName;
	if (parentUID == 0)
		return LString::stringWithFormat("SELECT * FROM %@ WHERE (%@ ISNULL OR %@='') AND %@=0", table,
		                                 kColumnNameParentUID, kColumnNameParentUID, kColumnNameDEL);
	return LString::stringWithFormat("SELECT * FROM %@ WHERE %@='%@' AND %@=0", table, kColumnNameParentUID, parentUID,
	                                 kColumnNameDEL);
}

static LString *_updateOrReplaceRequest(LString *table, LString *column, LString *value, LString *uid)
{
	return LString::stringWithFormat("UPDATE OR REPLACE %@ SET %@=%@ WHERE %@='%@'", table, column, value,
	                                 kColumnNameUID, uid);
}

static LString *_updateOrReplaceRequest(LString *table, LString *column, LString *value, LArray *uids)
{
	return LString::stringWithFormat("UPDATE OR REPLACE %@ SET %@=%@ WHERE %@ in (%@)", table, column, value,
	                                 kColumnNameUID, SQLiteUtils::stringAsSQLValuesList(uids));
}

LString *SQL_DB_Strings::deleteRoutesForRouteNodesWithUIDs(LArray *uids)
{
	return LString::stringWithFormat(
	    "UPDATE OR REPLACE %@ SET %@=1 WHERE %@ IN (SELECT %@ FROM %@ WHERE %@ IN (%@) AND %@ NOT NULL)",
	    kUserGeoObjectsTableName, kColumnNameDEL, kColumnNameUID, kColumnNameRouteUID, kUserRouteNodesTableName,
	    kColumnNameUID, SQLiteUtils::stringAsSQLValuesList(uids), kColumnNameRouteUID);
}

LString *SQL_DB_Strings::restoreRoutesForRouteNodesWithUIDs(LArray *uids)
{
	return LString::stringWithFormat(
	    "UPDATE OR REPLACE %@ SET %@=0 WHERE %@ IN (SELECT %@ FROM %@ WHERE %@ IN (%@) AND %@ NOT NULL)",
	    kUserGeoObjectsTableName, kColumnNameDEL, kColumnNameUID, kColumnNameRouteUID, kUserRouteNodesTableName,
	    kColumnNameUID, SQLiteUtils::stringAsSQLValuesList(uids), kColumnNameRouteUID);
}

LString *SQL_DB_Strings::deleteKeywordsForObject(LString *uid)
{
	return LString::stringWithFormat("DELETE FROM %@ WHERE %@='%@'", kUserOcurencesTableName, kColumnNameUID, uid);
}

LString *SQL_DB_Strings::deleteEntryInfoForObjectWithUID(LString *uid)
{
	return LString::stringWithFormat("DELETE FROM %@ WHERE %@='%@'", kUserEntryInfoTableName, kColumnNameObjectUID,
	                                 uid);
}

LString *SQL_DB_Strings::selectAllUserGeoObjects()
{
	return LString::stringWithFormat("SELECT * FROM %@ WHERE %@=0", kUserGeoObjectsTableName, kColumnNameDEL);
}

LString *SQL_DB_Strings::selectAllUserKeywords()
{
	return LString::stringWithFormat(
	    "SELECT uid, kw FROM %@, %@ WHERE kw_rowid=%@.rowid AND uid in (SELECT uid FROM %@ WHERE %@=0)",
	    kUserOcurencesTableName, kUserAllKeywordsTableName, kUserAllKeywordsTableName, kUserGeoObjectsTableName,
	    kColumnNameDEL);
}

LString *SQL_DB_Strings::selectAllUserRouteNodes()
{
	return LString::stringWithFormat("SELECT * FROM %@ WHERE %@=0", kUserRouteNodesTableName, kColumnNameDEL);
}

LString *SQL_DB_Strings::selectAllUserBLOBs()
{
	return LString::stringWithFormat("SELECT * FROM %@ WHERE %@=0 AND %@ in (SELECT %@ FROM %@ WHERE %@=0)",
	                                 kUserBLOBsTableName, kColumnNameDEL, kColumnNameUID, kColumnNameBLOBUid,
	                                 kUserGeoDataTableName, kColumnNameDEL);
}

LString *SQL_DB_Strings::selectAllUserGeoData()
{
	return LString::stringWithFormat("SELECT * FROM %@ WHERE %@=0", kUserGeoDataTableName, kColumnNameDEL);
}

// Rebuilding the user database from a dump: keywords are inserted through a
// temporary table that fills the keyword index.
LString *SQL_DB_Strings::userDBDump(LArray *requests)
{
	return LString::stringWithFormat(
	    "BEGIN TRANSACTION;\r\nCREATE TEMPORARY TABLE _tmp_kw (%@ char(%d), kw varchar(100));\r\nCREATE TEMPORARY "
	    "TRIGGER _kw_importer AFTER INSERT ON _tmp_kw BEGIN INSERT OR IGNORE INTO %@ VALUES (new.kw, "
	    "substr(new.kw||'   ', 1, 3)); INSERT INTO %@ SELECT rowid, new.uid FROM %@ WHERE kw=new.kw; END;\r\n%@DROP "
	    "TRIGGER _kw_importer;\r\nDROP TABLE _tmp_kw;\r\nEND TRANSACTION;\r\n",
	    kColumnNameUID, 21, kUserAllKeywordsTableName, kUserOcurencesTableName, kUserAllKeywordsTableName,
	    requests->componentsJoinedByString(0));
}

LString *SQL_DB_Strings::selectUIDKeyValKindForParent(LString *parentUID, LString *table, LString *parentColumn,
                                                      LString *uidColumn, LString *keyColumn, LString *valueColumn,
                                                      LString *kindColumn, LString *orderColumn)
{
	return LString::stringWithFormat("SELECT %@, %@, %@, %@ FROM %@ WHERE %@='%@' AND %@=0 ORDER BY %@, %@, %@",
	                                 uidColumn, keyColumn, valueColumn, kindColumn, table, parentColumn, parentUID,
	                                 kColumnNameDEL, keyColumn, kindColumn, orderColumn);
}

LString *SQL_DB_Strings::selectAllObjectUIDsWithImages()
{
	LString *images = kEntryInfoImagesKey;
	return LString::stringWithFormat(
	    "SELECT %@ FROM %@ WHERE %@='%@' AND %@=0 UNION SELECT %@ FROM %@ WHERE %@='%@' AND %@=0 AND length(%@)=%d "
	    "UNION SELECT %@ as %@ FROM %@ WHERE %@=0 AND %@ IN (SELECT %@ FROM %@ WHERE %@='%@' AND %@=0 AND "
	    "length(%@)=%d)",
	    kColumnNameObjectUID, kEntryInfoTableName, kColumnNameInfoKey, images, kColumnNameDEL, kColumnNameObjectUID,
	    kUserEntryInfoTableName, kColumnNameInfoKey, images, kColumnNameDEL, kColumnNameObjectUID, 10, kColumnNameUID,
	    kColumnNameObjectUID, kUserGeoObjectsTableName, kColumnNameDEL, kColumnNameUID, kColumnNameObjectUID,
	    kUserEntryInfoTableName, kColumnNameInfoKey, images, kColumnNameDEL, kColumnNameObjectUID, 21);
}

LString *SQL_DB_Strings::selectObjectUIDsWithUserImages()
{
	return LString::stringWithFormat("SELECT DISTINCT %@ FROM %@ WHERE %@='%@'", kColumnNameObjectUID,
	                                 kUserEntryInfoTableName, kColumnNameInfoKey, kEntryInfoImagesKey);
}

LString *SQL_DB_Strings::transactionForReuests(LArray *requests)
{
	return LString::stringWithFormat("BEGIN TRANSACTION;%@;END TRANSACTION;",
	                                 requests->componentsJoinedByString(LString::stringWithCString(";")));
}

LString *SQL_DB_Strings::insertKeywordsForObject(LArray *keywords, LString *uid)
{
	LString *request = LString::stringWithFormat(
	    "CREATE TEMPORARY TABLE _tmp_kw (%@ char(%d), kw varchar(100));CREATE TEMPORARY TRIGGER _kw_importer AFTER "
	    "INSERT ON _tmp_kw BEGIN INSERT OR IGNORE INTO %@ VALUES (new.kw, substr(new.kw||'   ', 1, 3)); INSERT INTO "
	    "%@ SELECT rowid, new.uid FROM %@ WHERE kw=new.kw; END;",
	    kColumnNameUID, 21, kUserAllKeywordsTableName, kUserOcurencesTableName, kUserAllKeywordsTableName);
	int count = keywords ? keywords->count() : 0;
	for (int i = 0; i < count; i++)
		request->appendFormat("INSERT INTO _tmp_kw VALUES ('%@', %@);", uid,
		                      SQLiteUtils::stringByEncodeAsSQLValue((LString *)keywords->objectAtIndex(i)));
	request->appendString(LString::stringWithCString("DROP TRIGGER _kw_importer;DROP TABLE _tmp_kw;"));
	return request;
}

LString *SQL_DB_Strings::restoreGeoObjectsWithUIDs(LArray *uids)
{
	return _updateOrReplaceRequest(kUserGeoObjectsTableName, kColumnNameDEL, LString::stringWithCString("0"), uids);
}

LString *SQL_DB_Strings::deleteGeoObjectsWithUIDs(LArray *uids)
{
	return _updateOrReplaceRequest(kUserGeoObjectsTableName, kColumnNameDEL, LString::stringWithCString("1"), uids);
}

LString *SQL_DB_Strings::setParentForRouteNodesWithUIDs(LString *parentUID, LArray *uids)
{
	if (parentUID == 0)
		parentUID = LString::stringWithCString("");
	return _updateOrReplaceRequest(kUserRouteNodesTableName, kColumnNameParentUID,
	                               SQLiteUtils::stringByEncodeAsSQLValue(parentUID), uids);
}

LString *SQL_DB_Strings::restoreRouteNodesWithUIDs(LArray *uids)
{
	return _updateOrReplaceRequest(kUserRouteNodesTableName, kColumnNameDEL, LString::stringWithCString("0"), uids);
}

LString *SQL_DB_Strings::deleteRouteNodesWithUIDs(LArray *uids)
{
	return _updateOrReplaceRequest(kUserRouteNodesTableName, kColumnNameDEL, LString::stringWithCString("1"), uids);
}

// Each element is {type} or {type, alt type}; a lone non-city type matches
// only objects without an alt type.
LString *SQL_DB_Strings::conditionSubstringForGeotypes(LArray *geotypes)
{
	int count = geotypes ? geotypes->count() : 0;
	LString *condition = LString::string();
	for (int i = 0; i < count; i++) {
		LArray *pair = (LArray *)geotypes->objectAtIndex(i);
		LString *type = (LString *)pair->objectAtIndex(0);
		if (pair->count() == 1) {
			const char *format = " OR (%@='%@' AND %@ ISNULL)";
			if (i == 0)
				format = type->isEqualTo(kGeoTypeCity) ? "%@='%@'" : "(%@='%@' AND %@ ISNULL)";
			condition->appendFormat(format, kColumnNameType, type, kColumnNameAltType);
		} else if (pair->count() == 2) {
			condition->appendFormat(i == 0 ? "(%@='%@' AND %@='%@')" : " OR (%@='%@' AND %@='%@')", kColumnNameType,
			                        type, kColumnNameAltType, pair->objectAtIndex(1));
		}
	}
	return LString::stringWithFormat(" AND (%@)", condition);
}

// The keyword tables only help once the last term has three letters, or
// when the other terms name a region.
bool SQL_DB_Strings::fastLookupIsPossibleForTerms(LArray *terms, unsigned char checkHosts)
{
	if (terms == 0 || terms->count() == 0)
		return false;
	if (terms->count() == 1)
		return ((LString *)terms->objectAtIndex(0))->length() > 2;
	if (((LString *)terms->lastObject())->length() > 2)
		return true;
	if (!checkHosts)
		return false;
	LArray *hosts = AtlasDatabase::instance()->hostUIDsFromTerms(terms);
	return hosts != 0 && hosts->count() > 0;
}

LString *SQL_DB_Strings::selectDistinctNameOfRegionsBySubstring(LString *substring)
{
	LString *trimmed = LString::stringWithString(substring);
	trimmed->trimWithCharacters(LCharSet::charSetWithType(10));
	LString *value = SQLiteUtils::stringByEncodeAsSQLValueWOBracers(trimmed);
	return LString::stringWithFormat("SELECT DISTINCT %@ FROM %@ WHERE %@ IN ('%@', '%@', '%@') AND (%@ like "
	                                 "'%%%@%%' OR %@ like '%%%@%%') ORDER BY %@, %@",
	                                 kColumnNameName, kGeoObjectsTableName, kColumnNameType, kGeoTypeCountry,
	                                 kGeoTypeState, kGeoTypeCounty, kColumnNameName, value, kColumnNameAltNames, value,
	                                 kColumnNameType, kColumnNameName);
}

LArray *SQL_DB_Strings::separatedTermsFromSearchString(LString *string)
{
	LString *trimmed = LString::stringWithString(string);
	trimmed->trimWithCharacters(LCharSet::charSetWithType(10));
	if (trimmed->length() == 0)
		return 0;
	return SQLiteUtils::separatedSearchTermsForString(trimmed);
}

bool SQL_DB_Strings::fastLookupIsPossibleForSearchWithSubstring(LString *substring)
{
	return fastLookupIsPossibleForTerms(separatedTermsFromSearchString(substring), 1);
}

// Rows for the _tmp_kw importer of insertKeywordsForObject.
LString *SQL_DB_Strings::insertKeywords(SQLiteResultsContainer *results)
{
	LString *kwKey = LString::stringWithCString("kw");
	LString *request = LString::string();
	int count = results ? results->count() : 0;
	for (int i = 0; i < count; i++) {
		StAutoreleasePool pool;
		LMap *row = results->objectAtIndex(i);
		LString *kw = SQLiteUtils::stringByEncodeAsSQLValue((LString *)row->objectForKey(kwKey));
		request->appendFormat("INSERT INTO _tmp_kw VALUES ('%@', %@);\r\n", row->objectForKey(kColumnNameUID), kw);
	}
	return request;
}

// The UIDs of the objects having a keyword starting with every term.
LString *SQL_DB_Strings::selectUIDStatementForTerms(LArray *terms, unsigned char user)
{
	int count = terms ? terms->count() : 0;
	LString *statement = LString::string();
	for (int i = 0; i < count; i++) {
		LString *term = (LString *)terms->objectAtIndex(i);
		LString *kw3 = LString::stringWithFormat("%@  ", term)->substringWithRange(0, 3)->uppercaseString();
		if (i != 0)
			statement->appendString(LString::stringWithCString(" INTERSECT "));
		statement->appendFormat(
		    "select %@ from %@ where kw_rowid in (select rowid from %@ where kw3='%@' and kw like '%@%%')",
		    kColumnNameUID, user ? kUserOcurencesTableName : kOcurencesTableName,
		    user ? kUserAllKeywordsTableName : kAllKeywordsTableName,
		    SQLiteUtils::stringByEncodeAsSQLValueWOBracers(kw3), SQLiteUtils::stringByEncodeAsSQLValueWOBracers(term));
	}
	return statement;
}

// The keywords completing the last term, of the objects matching all terms.
static LString *_autocompleteBaseRequest(LArray *terms, unsigned char user)
{
	LString *kwTable = user ? kUserAllKeywordsTableName : kAllKeywordsTableName;
	LString *occTable = user ? kUserOcurencesTableName : kOcurencesTableName;
	LString *geoTable = user ? kUserGeoObjectsTableName : kGeoObjectsTableName;
	LString *upper = ((LString *)terms->lastObject())->uppercaseString();
	LString *kw3 = SQLiteUtils::stringByEncodeAsSQLValueWOBracers(upper->substringWithRange(0, 3));
	LString *value = SQLiteUtils::stringByEncodeAsSQLValueWOBracers(upper);
	LString *uids = SQL_DB_Strings::selectUIDStatementForTerms(terms, user);
	LString *notDeleted = LString::stringWithFormat(user ? "%@.%@=0" : "%@.%@ ISNULL", geoTable, kColumnNameDEL);
	return LString::stringWithFormat("SELECT kw FROM %@, %@, %@ WHERE kw3='%@' and kw like '%@%%' and kw!='%@' and "
	                                 "kw_rowid=%@.rowid and %@.uid=%@.uid and %@ and %@.uid in (%@)",
	                                 kwTable, occTable, geoTable, kw3, value, value, kwTable, geoTable, occTable,
	                                 notDeleted, geoTable, uids);
}

LString *SQL_DB_Strings::selectColumnsForTerms(LString *columns, LArray *terms, unsigned char user)
{
	LString *uids = selectUIDStatementForTerms(terms, user);
	LString *request = LString::stringWithFormat("SELECT %@ FROM %@ WHERE (%@ IN (%@))", columns,
	                                             user ? kUserGeoObjectsTableName : kGeoObjectsTableName,
	                                             kColumnNameUID, uids);
	if (user)
		request->appendFormat(" AND %@=0", kColumnNameDEL);
	return request;
}

LString *SQL_DB_Strings::setNameForRouteNodeWithUID(LString *name, LString *uid)
{
	return LString::stringWithFormat("UPDATE OR REPLACE %@ SET %@=%@ WHERE %@='%@'", kUserRouteNodesTableName,
	                                 kColumnNameName, SQLiteUtils::stringByEncodeAsSQLValue(name), kColumnNameUID, uid);
}

LString *SQL_DB_Strings::setParentForRouteNodeWithUID(LString *parentUID, LString *uid)
{
	if (parentUID == 0)
		parentUID = LString::stringWithCString("");
	return LString::stringWithFormat("UPDATE OR REPLACE %@ SET %@=%@ WHERE %@='%@'", kUserRouteNodesTableName,
	                                 kColumnNameParentUID, SQLiteUtils::stringByEncodeAsSQLValue(parentUID),
	                                 kColumnNameUID, uid);
}

LString *SQL_DB_Strings::deleteGeoDataWithUID(LString *uid)
{
	return LString::stringWithFormat("UPDATE OR REPLACE %@ SET %@=%@ WHERE %@='%@'", kUserGeoDataTableName,
	                                 kColumnNameDEL, LString::stringWithCString("1"), kColumnNameUID, uid);
}

LString *SQL_DB_Strings::restoreGeoDataWithUID(LString *uid)
{
	return LString::stringWithFormat("UPDATE OR REPLACE %@ SET %@=%@ WHERE %@='%@'", kUserGeoDataTableName,
	                                 kColumnNameDEL, LString::stringWithCString("0"), kColumnNameUID, uid);
}

static void _columnsAndValues(LMap *map, LString **columns, LString **values)
{
	unsigned int count = map->pairsCount();
	LArray *keys = LArray::arrayWithCapacity(count);
	LArray *objects = LArray::arrayWithCapacity(count);
	LMapIterator it(map, false);
	while (it.next()) {
		keys->addObject((LObject *)it.key());
		objects->addObject(it.object());
	}
	*values = SQLiteUtils::stringAsSQLValuesList(objects);
	*columns = SQLiteUtils::stringAsSQLValuesList(keys);
}

// One INSERT per row, for copying rows between databases.
static LString *_insertRequests(LString *table, SQLiteResultsContainer *results)
{
	LString *request = LString::string();
	int count = results ? results->count() : 0;
	for (int i = 0; i < count; i++) {
		StAutoreleasePool pool;
		LString *columns, *values;
		_columnsAndValues(results->objectAtIndex(i), &columns, &values);
		request->appendFormat("INSERT INTO %@ (%@) VALUES (%@);\r\n", table, columns, values);
	}
	return request;
}

LString *SQL_DB_Strings::insertGeoData(SQLiteResultsContainer *results)
{
	return _insertRequests(kUserGeoDataTableName, results);
}

LString *SQL_DB_Strings::insertBLOBs(SQLiteResultsContainer *results)
{
	return _insertRequests(kUserBLOBsTableName, results);
}

LString *SQL_DB_Strings::insertRouteNodes(SQLiteResultsContainer *results)
{
	return _insertRequests(kUserRouteNodesTableName, results);
}

LString *SQL_DB_Strings::insertGeoObjects(SQLiteResultsContainer *results)
{
	return _insertRequests(kUserGeoObjectsTableName, results);
}

static LString *_insertOrReplaceRequest(LString *table, LMap *map)
{
	LString *columns, *values;
	_columnsAndValues(map, &columns, &values);
	return LString::stringWithFormat("INSERT OR REPLACE INTO %@ (%@) VALUES (%@)", table, columns, values);
}

LString *SQL_DB_Strings::insertOrReplaceEntryInfoElementWithDBMap(LMap *map)
{
	return _insertOrReplaceRequest(kUserEntryInfoTableName, map);
}

LString *SQL_DB_Strings::insertOrReplaceGeoDataWithDBMap(LMap *map)
{
	return _insertOrReplaceRequest(kUserGeoDataTableName, map);
}

LString *SQL_DB_Strings::insertOrReplaceBlobObjectWithDBMap(LMap *map)
{
	return _insertOrReplaceRequest(kUserBLOBsTableName, map);
}

LString *SQL_DB_Strings::insertOrReplaceRouteNodeWithDBMap(LMap *map)
{
	return _insertOrReplaceRequest(kUserRouteNodesTableName, map);
}

// The geotypes of the search filter bits, as a quoted list.
static LString *_stringAsSQLValuesListFromGeotypeFilter(unsigned long long filter)
{
	LString *const types[] = {
		kGeoTypeCity,	    kGeoTypeCounty,	    kGeoTypeState,	   kGeoTypeCountry,
		kGeoTypeTimezone,   kGeoTypeVolcano,	    kGeoTypePeak,	   kGeoTypeHills,
		kGeoTypeDesert,	    kGeoTypeMountain,	    kGeoTypeMountains,	   kGeoTypeContinent,
		kGeoTypeStrait,	    kGeoTypeBay,	    kGeoTypeLagoon,	   kGeoTypeFjord,
		kGeoTypeGulf,	    kGeoTypeSea,	    kGeoTypeLake,	   kGeoTypeOcean,
		kGeoTypeRiver,	    kGeoTypeRoad,	    kGeoTypeRoute,	   kGeoTypeZoo,
		kGeoTypeWaterfall,  kGeoTypeNationalPark,   kGeoTypeEndangeredSite, kGeoTypeDestroyedLandmark,
		kGeoTypeBuildingStructure, kGeoTypeAppleStore, kGeoTypeUserLocation,
	};
	LArray *values = LArray::arrayWithCapacity(64);
	for (unsigned int i = 0; i < sizeof(types) / sizeof(types[0]); i++)
		if (filter & (1ULL << i))
			values->addObject(SQLiteUtils::stringByEncodeAsSQLValue(types[i]));
	return values->componentsJoinedByString(LString::stringWithCString(", "));
}

// A filter with the low and high words all ones does not filter.
static inline bool filterSelectsAll(unsigned long long filter)
{
	return ((unsigned int)filter & (unsigned int)(filter >> 32)) == 0xffffffff;
}

// The most used keyword completing the last term.
LString *SQL_DB_Strings::autocompleteRequestForTerm(LArray *terms, unsigned long long filter)
{
	if (terms == 0 || terms->count() == 0 || ((LString *)terms->lastObject())->length() <= 2)
		return 0;
	LString *atlas = _autocompleteBaseRequest(terms, 0);
	LString *user = _autocompleteBaseRequest(terms, 1);
	if (!filterSelectsAll(filter)) {
		LString *types = _stringAsSQLValuesListFromGeotypeFilter(filter);
		atlas->appendFormat(" AND %@ IN (%@)", kColumnNameType, types);
		user->appendFormat(" AND %@ IN (%@)", kColumnNameType, types);
	}
	return LString::stringWithFormat(
	    "SELECT kw, count(kw) FROM (%@ UNION ALL %@) group by kw order by 2 desc, kw asc limit 1", atlas, user);
}

LArray *SQL_DB_Strings::createUserDBTables(unsigned char temporary)
{
	LString *temp = LString::stringWithCString(temporary ? "TEMP" : "");
	int build = IModelCallbacks::_callbacks ? IModelCallbacks::_callbacks->_applicationBuildNumber() : 0;
	return LArray::arrayWithObjects(
	    LString::stringWithCString("BEGIN TRANSACTION"),
	    LString::stringWithFormat("CREATE %@ TABLE %@ (%@ integer)", temp, kUserTableNameRevision,
	                              kUserColumnNameBuildNumber),
	    LString::stringWithFormat("INSERT OR REPLACE INTO %@ (%@) VALUES (%d)", kUserTableNameRevision,
	                              kUserColumnNameBuildNumber, build),
	    LString::stringWithFormat("CREATE %@ TABLE %@ (%@ char(%d), %@ integer DEFAULT 0, %@ char(10), %@ blob, "
	                              "PRIMARY KEY(\"%@\"))",
	                              temp, kUserBLOBsTableName, kColumnNameUID, 21, kColumnNameDEL, kColumnNameKind,
	                              kColumnNameBLOBData, kColumnNameUID),
	    LString::stringWithFormat("CREATE %@ TABLE %@ (%@ char(%d), %@ integer DEFAULT 0, %@ char(%d), %@ char(%d), "
	                              "%@ varchar(50), PRIMARY KEY(\"%@\"))",
	                              temp, kUserGeoDataTableName, kColumnNameUID, 21, kColumnNameDEL,
	                              kColumnNameObjectUID, 21, kColumnNameBLOBUid, 21, kColumnNameRepType,
	                              kColumnNameUID),
	    LString::stringWithFormat(
	        "CREATE %@ TABLE %@ (%@ char(%d), %@ integer DEFAULT 0, %@ varchar(100), %@ char(12), %@ float, %@ float, "
	        "%@ char(15), %@ varchar(1000), %@ char(%d), %@ varchar(100), %@ char(%d), %@ varchar(100), %@ char(%d), "
	        "%@ varchar(100), %@ varchar(50), %@ float, PRIMARY KEY(\"%@\"))",
	        temp, kUserGeoObjectsTableName, kColumnNameUID, 21, kColumnNameDEL, kColumnNameName, kColumnNameType,
	        kColumnNameLat, kColumnNameLong, kColumnNameAltType, kColumnNameAltNames, kColumnNameHostCountyUID, 10,
	        kUserColumnNameHostCountyName, kColumnNameHostStateUID, 10, kUserColumnNameHostStateName,
	        kColumnNameHostCountryUID, 10, kUserColumnNameHostCountryName, kColumnNameHostTimeZone,
	        kColumnNamePopulation, kColumnNameUID),
	    LString::stringWithFormat("CREATE %@ TABLE %@ (%@ char(%d), %@ integer DEFAULT 0, %@ char(%d), %@ char(%d), "
	                              "%@ varchar(100), PRIMARY KEY(\"%@\"))",
	                              temp, kUserRouteNodesTableName, kColumnNameUID, 21, kColumnNameDEL,
	                              kColumnNameParentUID, 21, kColumnNameRouteUID, 21, kColumnNameName, kColumnNameUID),
	    LString::stringWithFormat("CREATE %@ TABLE %@ (kw varchar(100) UNIQUE NOT NULL, kw3 char(3))", temp,
	                              kUserAllKeywordsTableName),
	    LString::stringWithFormat("CREATE INDEX user_kw3_index on %@ (kw3)", kUserAllKeywordsTableName),
	    LString::stringWithFormat("CREATE %@ TABLE %@ (kw_rowid integer, %@ char(%d))", temp, kUserOcurencesTableName,
	                              kColumnNameUID, 21),
	    LString::stringWithFormat("CREATE INDEX user_kwid_index on %@ (kw_rowid)", kUserOcurencesTableName),
	    LString::stringWithFormat("CREATE INDEX user_uid_index on %@ (%@)", kUserOcurencesTableName, kColumnNameUID),
	    LString::stringWithFormat("CREATE %@ TABLE %@ (%@ char(%d), %@ integer DEFAULT 0, %@ char(%d), %@ char(%d), "
	                              "%@ varchar(30), %@ integer, %@ char(1), %@ varchar(1000), PRIMARY KEY(\"%@\"))",
	                              temp, kUserEntryInfoTableName, kColumnNameUID, 21, kColumnNameDEL,
	                              kColumnNameParentUID, 21, kColumnNameObjectUID, 21, kColumnNameInfoKey,
	                              kColumnNameInfoSortIndex, kColumnNameInfoValueType, kColumnNameInfoValue,
	                              kColumnNameUID),
	    LString::stringWithFormat("CREATE INDEX %@_%@ ON %@ (%@)", kUserEntryInfoTableName, kColumnNameParentUID,
	                              kUserEntryInfoTableName, kColumnNameParentUID),
	    LString::stringWithFormat("CREATE INDEX %@_%@ ON %@ (%@)", kUserEntryInfoTableName, kColumnNameObjectUID,
	                              kUserEntryInfoTableName, kColumnNameObjectUID),
	    LString::stringWithCString("END TRANSACTION"), 0);
}

// Drops what is marked deleted, and whatever only deleted objects used.
LArray *SQL_DB_Strings::cleanUserDB()
{
	return LArray::arrayWithObjects(
	    LString::stringWithCString("BEGIN TRANSACTION"),
	    LString::stringWithFormat("DELETE FROM %@ WHERE %@=1", kUserRouteNodesTableName, kColumnNameDEL),
	    LString::stringWithFormat("DELETE FROM %@ WHERE %@=1 OR %@ IN (SELECT %@ FROM %@ WHERE %@=1)",
	                              kUserEntryInfoTableName, kColumnNameDEL, kColumnNameObjectUID, kColumnNameUID,
	                              kUserGeoObjectsTableName, kColumnNameDEL),
	    LString::stringWithFormat("DELETE FROM %@ WHERE %@=1 OR %@ IN (SELECT %@ FROM %@ WHERE %@=1) OR(%@ IN ('%@', "
	                              "'%@') AND %@ NOT IN (SELECT %@ FROM %@ WHERE %@ IN ('%c', '%c')))",
	                              kUserGeoDataTableName, kColumnNameDEL, kColumnNameObjectUID, kColumnNameUID,
	                              kUserGeoObjectsTableName, kColumnNameDEL, kColumnNameRepType, kRepTypeEntryInfoText,
	                              kRepTypeEntryInfoImage, kColumnNameUID, kColumnNameInfoValue,
	                              kUserEntryInfoTableName, kColumnNameInfoValueType, 't', 'd'),
	    LString::stringWithFormat("DELETE FROM %@ WHERE %@=1", kUserGeoObjectsTableName, kColumnNameDEL),
	    LString::stringWithFormat("DELETE FROM %@ WHERE %@ NOT IN (SELECT %@ FROM %@)", kUserBLOBsTableName,
	                              kColumnNameUID, kColumnNameBLOBUid, kUserGeoDataTableName),
	    LString::stringWithFormat("DELETE FROM %@ WHERE %@ NOT IN (SELECT %@ FROM %@)", kUserOcurencesTableName,
	                              kColumnNameUID, kColumnNameUID, kUserGeoObjectsTableName),
	    LString::stringWithFormat("DELETE FROM %@ WHERE rowid NOT IN (SELECT kw_rowid FROM %@)",
	                              kUserAllKeywordsTableName, kUserOcurencesTableName),
	    LString::stringWithCString("END TRANSACTION"), 0);
}

// The countries listed first in country pickers.
static LArray *_topCountryUIDs()
{
	static LArray *sTopCountryUIDs =
	    LArray::_alloc()->initWithObjects(&kTopCountryUID1, &kTopCountryUID2, &kTopCountryUID3, (LObject *)0);
	return sTopCountryUIDs;
}

// The top countries first, then the others by name.
LString *SQL_DB_Strings::selectAllCountriesNames()
{
	LArray *top = _topCountryUIDs();
	LArray *selects = LArray::array();
	for (unsigned int i = 0; i < top->count(); i++)
		selects->addObject(LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@='%@'", kColumnNameName,
		                                             kGeoObjectsTableName, kColumnNameUID,
		                                             lobj_cast<LString>(top->objectAtIndex(i))));
	return LString::stringWithFormat("SELECT %@ FROM (%@) UNION ALL SELECT %@ FROM (SELECT %@ FROM %@ WHERE %@='%@' "
	                                 "EXCEPT SELECT %@ FROM %@ WHERE %@ IN (%@) ORDER BY %@)",
	                                 kColumnNameName,
	                                 selects->componentsJoinedByString(LString::stringWithCString(" UNION ALL ")),
	                                 kColumnNameName, kColumnNameName, kGeoObjectsTableName, kColumnNameType,
	                                 kGeoTypeCountry, kColumnNameName, kGeoObjectsTableName, kColumnNameUID,
	                                 SQLiteUtils::stringAsSQLValuesList(top), kColumnNameName);
}

// Saves a user object and indexes the words of its name and of its regions'
// names.
LString *SQL_DB_Strings::insertOrReplaceGeoObjectWithDBMap(LMap *map)
{
	static LString *const sAllUserColumnsWithNames[] = { kColumnNameName, kUserColumnNameHostCountryName,
		                                             kUserColumnNameHostStateName,
		                                             kUserColumnNameHostCountyName };
	LString *uid = (LString *)map->objectForKey(kColumnNameUID);
	LSet *keywords = (LSet *)LComparableSet::_alloc()->init()->l_autorelease();
	for (unsigned int i = 0; i < sizeof(sAllUserColumnsWithNames) / sizeof(sAllUserColumnsWithNames[0]); i++) {
		LString *value = (LString *)map->objectForKey(sAllUserColumnsWithNames[i]);
		if (value == 0)
			continue;
		LArray *terms = SQLiteUtils::separatedSearchTermsForString(value);
		for (unsigned int j = 0; j < terms->count(); j++)
			keywords->addObject(terms->objectAtIndex(j));
	}
	LString *insertKeywords = insertKeywordsForObject(keywords->allObjects(), uid);
	LString *deleteKeywords = LString::stringWithFormat("DELETE FROM %@ WHERE %@='%@'", kUserOcurencesTableName,
	                                                    kColumnNameUID, uid);
	return LString::stringWithFormat("%@;%@;%@", _insertOrReplaceRequest(kUserGeoObjectsTableName, map),
	                                 deleteKeywords, insertKeywords);
}

// Saves the BLOB and its link to the object.
LString *SQL_DB_Strings::commitBLOB(BLOBWrapper *blob, const LString *repType, const LString *objectUID)
{
	LMap *blobMap = LMap::mapWithObjectsAndKeys(blob->uid(), kColumnNameUID, blob->kind(), kColumnNameKind,
	                                            blob->blobData(), kColumnNameBLOBData, 0);
	LMap *geoDataMap = LMap::mapWithObjectsAndKeys(blob->uid(), kColumnNameUID, objectUID, kColumnNameObjectUID,
	                                               blob->uid(), kColumnNameBLOBUid, repType, kColumnNameRepType, 0);
	return LString::stringWithFormat("%@;%@", _insertOrReplaceRequest(kUserGeoDataTableName, geoDataMap),
	                                 _insertOrReplaceRequest(kUserBLOBsTableName, blobMap));
}

// Objects whose name starts with the substring, or lying in a region it
// names. The atlas and user tables are searched apart and joined.
LString *SQL_DB_Strings::searchForGeoObjectWithSubstring(LString *substring, unsigned long long filter)
{
	LArray *selects = LArray::array();
	LString *columns = _columnsListForGenericSearch();
	LString *atlas =
	    LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@ ISNULL", columns, kGeoObjectsTableName, kColumnNameDEL);
	LString *user =
	    LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@=0", columns, kUserGeoObjectsTableName, kColumnNameDEL);
	if (!filterSelectsAll(filter)) {
		LString *types = _stringAsSQLValuesListFromGeotypeFilter(filter);
		atlas->appendFormat(" AND %@ IN (%@)", kColumnNameType, types);
		user->appendFormat(" AND %@ IN (%@)", kColumnNameType, types);
	}
	LString *trimmed = LString::stringWithString(substring);
	LCharSet *spaces = LCharSet::_alloc()->initWithCString(" \n");
	trimmed->trimWithCharacters(spaces);
	spaces->release();
	if (trimmed->length() == 0) {
		selects->addObject(atlas);
		selects->addObject(user);
		return selects->componentsJoinedByString(LString::stringWithCString(" UNION ALL "));
	}

	LArray *terms = separatedTermsFromSearchString(trimmed);
	LArray *hosts = AtlasDatabase::instance()->hostUIDsFromTerms(terms);
	LString *inHosts = 0;
	if (hosts != 0 && hosts->count() != 0) {
		LString *list = SQLiteUtils::stringAsSQLValuesList(hosts);
		inHosts = LString::stringWithFormat(" AND (%@ IN (%@) OR %@ IN (%@))", kColumnNameHostCountryUID, list,
		                                    kColumnNameHostStateUID, list);
	}
	LString *value = SQLiteUtils::stringByEncodeAsSQLValueWOBracers(trimmed);

	if (fastLookupIsPossibleForTerms(terms, 0)) {
		LString *atlasUIDs = selectUIDStatementForTerms(terms, 0);
		LString *userUIDs = selectUIDStatementForTerms(terms, 1);
		selects->addObject(LString::stringWithFormat("%@ AND %@ IN (%@)", atlas, kColumnNameUID, atlasUIDs));
		selects->addObject(LString::stringWithFormat("%@ AND %@ IN (%@)", atlas, kColumnNameHostStateUID, atlasUIDs));
		selects->addObject(
		    LString::stringWithFormat("%@ AND %@ IN (%@)", atlas, kColumnNameHostCountryUID, atlasUIDs));
		selects->addObject(LString::stringWithFormat("%@ AND %@ IN (%@)", user, kColumnNameUID, userUIDs));
		selects->addObject(LString::stringWithFormat("%@ AND %@ IN (%@)", user, kColumnNameHostStateUID, atlasUIDs));
		selects->addObject(LString::stringWithFormat("%@ AND %@ IN (%@)", user, kColumnNameHostCountryUID, atlasUIDs));
		if (inHosts) {
			// The last term named a region: the others are searched in it.
			terms->removeObjectAtIndex(terms->count() - 1);
			selects->addObject(LString::stringWithFormat("%@ AND %@ IN (%@) %@", atlas, kColumnNameUID,
			                                             selectUIDStatementForTerms(terms, 0), inHosts));
			selects->addObject(LString::stringWithFormat("%@ AND %@ IN (%@) %@", user, kColumnNameUID,
			                                             selectUIDStatementForTerms(terms, 1), inHosts));
		}
		return selects->componentsJoinedByString(LString::stringWithCString(" UNION "));
	}

	LString *atlasInHosts = 0, *userInHosts = 0;
	if (inHosts) {
		terms->removeObjectAtIndex(terms->count() - 1);
		atlasInHosts = LString::stringWithFormat(" UNION %@ AND %@ IN (%@) %@", atlas, kColumnNameUID,
		                                         selectUIDStatementForTerms(terms, 0), inHosts);
		userInHosts = LString::stringWithFormat(" UNION %@ AND %@ IN (%@) %@", user, kColumnNameUID,
		                                        selectUIDStatementForTerms(terms, 1), inHosts);
	}
	LString *states = _selectGeoObjectOfTypesWithSubstring(LArray::arrayWithObjects(kGeoTypeState, (LObject *)0), value);
	LString *countries = _selectGeoObjectOfTypesWithSubstring(LArray::arrayWithObjects(kGeoTypeCountry, (LObject *)0), value);
	if (atlasInHosts == 0)
		atlasInHosts = LString::stringWithCString("");
	selects->addObject(
	    LString::stringWithFormat("%@ AND (%@ LIKE '%@%%' OR %@ LIKE '%%;%@%%' OR %@ IN (%@) OR %@ IN (%@)) %@", atlas,
	                              kColumnNameName, value, kColumnNameAltNames, value, kColumnNameHostStateUID, states,
	                              kColumnNameHostCountryUID, countries, atlasInHosts));
	if (userInHosts == 0)
		userInHosts = LString::stringWithCString("");
	selects->addObject(LString::stringWithFormat(
	    "%@ AND (%@ LIKE '%@%%' OR %@ IN (%@) OR %@ IN (%@) OR %@ LIKE '%@%%' OR %@ LIKE '%@%%') %@", user,
	    kColumnNameName, value, kColumnNameHostStateUID, states, kColumnNameHostCountryUID, countries,
	    kUserColumnNameHostStateName, value, kUserColumnNameHostCountryName, value, userInHosts));
	return selects->componentsJoinedByString(LString::stringWithCString(" UNION ALL "));
}

// Objects whose name starts with the string.
LString *SQL_DB_Strings::searchForGeoObjectByName(LString *name, unsigned long long filter)
{
	LArray *selects = LArray::array();
	LString *columns = _columnsListForGenericSearch();
	LString *atlas =
	    LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@ ISNULL", columns, kGeoObjectsTableName, kColumnNameDEL);
	LString *user =
	    LString::stringWithFormat("SELECT %@ FROM %@ WHERE %@=0", columns, kUserGeoObjectsTableName, kColumnNameDEL);
	if (!filterSelectsAll(filter)) {
		LString *types = _stringAsSQLValuesListFromGeotypeFilter(filter);
		atlas->appendFormat(" AND %@ IN (%@)", kColumnNameType, types);
		user->appendFormat(" AND %@ IN (%@)", kColumnNameType, types);
	}
	LString *trimmed = LString::stringWithString(name);
	LCharSet *spaces = LCharSet::_alloc()->initWithCString(" \n");
	trimmed->trimWithCharacters(spaces);
	spaces->release();
	if (trimmed->length() == 0) {
		selects->addObject(atlas);
	} else {
		selects->addObject(LString::stringWithFormat("%@ AND (%@ LIKE '%@%%' OR %@ LIKE '%%;%@%%')", atlas,
		                                             kColumnNameName, trimmed, kColumnNameAltNames, trimmed));
		user = LString::stringWithFormat("%@ AND (%@ LIKE '%@%%')", user, kColumnNameName, trimmed);
	}
	selects->addObject(user);
	return selects->componentsJoinedByString(LString::stringWithCString(" UNION ALL "));
}

// The objects matching the words of the name; `hostsFound` tells whether
// the last word named a region, whose objects are then included.
LString *SQL_DB_Strings::selectColumnsByName(LString *name, LString *condition, unsigned char *hostsFound,
                                             unsigned char forGenericSearch)
{
	LString *trimmed = name ? LString::stringWithString(name) : LString::stringWithCString("");
	trimmed->trimWithCharacters(LCharSet::charSetWithType(10));
	LString *columns = forGenericSearch ? _columnsListForGenericSearch()
	                                    : LString::stringWithFormat("%@, %@, %@, %@, %@", kColumnNameUID,
	                                                                kColumnNameName, kColumnNamePopulation,
	                                                                kColumnNameType, kColumnNameAltType);
	LArray *terms = separatedTermsFromSearchString(trimmed);
	LArray *hosts = AtlasDatabase::instance()->hostUIDsFromTerms(terms);
	int hostCount = hosts ? hosts->count() : 0;
	if (hostsFound)
		*hostsFound = hostCount > 0;

	LString *atlas, *user;
	if (fastLookupIsPossibleForTerms(terms, 0)) {
		atlas = selectColumnsForTerms(columns, terms, 0);
		user = selectColumnsForTerms(columns, terms, 1);
	} else {
		LString *value = SQLiteUtils::stringByEncodeAsSQLValueWOBracers(trimmed);
		atlas = _selectColumnsForNameSubstring(columns, value, 0);
		user = _selectColumnsForNameSubstring(columns, value, 1);
	}
	if (condition) {
		atlas->appendString(condition);
		user->appendString(condition);
	}
	if (hostCount != 0) {
		terms->removeObjectAtIndex(terms->count() - 1);
		LString *list = SQLiteUtils::stringAsSQLValuesList(hosts);
		LString *inHosts = LString::stringWithFormat(" AND (%@ IN (%@) OR %@ IN (%@))", kColumnNameHostCountryUID,
		                                             list, kColumnNameHostStateUID, list);
		atlas->appendFormat(" UNION %@ %@", selectColumnsForTerms(columns, terms, 0), inHosts);
		user->appendFormat(" UNION %@ %@", selectColumnsForTerms(columns, terms, 1), inHosts);
		if (condition) {
			atlas->appendString(condition);
			user->appendString(condition);
		}
	}
	LString *request = LString::stringWithFormat("%@ UNION ALL %@", atlas, user);
	if (!forGenericSearch)
		request->appendFormat(" ORDER BY %@", LString::stringWithFormat("%@, %@, %@, %@", kColumnNameType,
		                                                                kColumnNameAltType, kColumnNamePopulation,
		                                                                kColumnNameName));
	return request;
}

LString *SQL_DB_Strings::requestForGenericSearchWithNameParams(LArray *geotypes, LString *name)
{
	return selectColumnsByName(name, conditionSubstringForGeotypes(geotypes), 0, 1);
}

// The objects in the countries, states and counties whose name contains
// the string, and the user objects whose region names contain it.
LString *SQL_DB_Strings::selectColumnsByRegion(LString *region, LString *condition, unsigned char forGenericSearch)
{
	LString *trimmed = LString::stringWithString(region);
	trimmed->trimWithCharacters(LCharSet::charSetWithType(10));
	if (condition == 0)
		condition = LString::stringWithCString("");
	LString *v = SQLiteUtils::stringByEncodeAsSQLValueWOBracers(trimmed);
	LString *c = forGenericSearch ? _columnsListForGenericSearch()
	                              : LString::stringWithFormat("%@, %@, %@, %@, %@", kColumnNameUID, kColumnNameName,
	                                                          kColumnNamePopulation, kColumnNameType,
	                                                          kColumnNameAltType);
	LString *request = LString::stringWithCString("");
	LString *const hostColumns[] = { kColumnNameHostCountryUID, kColumnNameHostStateUID, kColumnNameHostCountyUID };
	LString *const regionTypes[] = { kGeoTypeCountry, kGeoTypeState, kGeoTypeCounty };
	for (int user = 0; user < 2; user++)
		for (int i = 0; i < 3; i++) {
			if (user || i)
				request->appendString(LString::stringWithCString(" UNION "));
			request->appendFormat("SELECT %@ FROM %@ WHERE %@ IN (SELECT %@ FROM %@ WHERE %@='%@' AND (%@ like "
			                      "'%%%@%%' OR %@ like '%%%@%%'))",
			                      c, user ? kUserGeoObjectsTableName : kGeoObjectsTableName, hostColumns[i],
			                      kColumnNameUID, kGeoObjectsTableName, kColumnNameType, regionTypes[i],
			                      kColumnNameName, v, kColumnNameAltNames, v);
			if (user)
				request->appendFormat(" AND %@=0", kColumnNameDEL);
			request->appendFormat(" %@", condition);
		}
	request->appendFormat(" UNION SELECT %@ FROM %@ WHERE (%@ like '%%%@%%' OR %@ like '%%%@%%' OR %@ like '%%%@%%') "
	                      "AND %@=0 %@",
	                      c, kUserGeoObjectsTableName, kUserColumnNameHostCountryName, v, kUserColumnNameHostStateName,
	                      v, kUserColumnNameHostCountyName, v, kColumnNameDEL, condition);
	if (!forGenericSearch)
		request->appendFormat(" ORDER BY %@", LString::stringWithFormat("%@, %@, %@, %@", kColumnNameType,
		                                                                kColumnNameAltType, kColumnNamePopulation,
		                                                                kColumnNameName));
	return request;
}

LString *SQL_DB_Strings::requestForGenericSearchInRegionsParams(LArray *geotypes, LString *region)
{
	return selectColumnsByRegion(region, conditionSubstringForGeotypes(geotypes), 1);
}

// One power of one series of the VSOP87 theory for a planet.
LString *SQL_DB_Strings::stars_selectVSOPBlock(int planet, int param, int power)
{
	static const char *const sPlanetNames[] = { "C", "V", "E", "M", "J", "S", "U", "N" };
	static const char *const sParamNames[] = { "L", "B", "R" };
	if ((unsigned int)planet > 7 || (unsigned int)param > 2)
		return 0;
	return LString::stringWithFormat("SELECT %@, %@, %@ FROM %@ WHERE %@='%s' and %@='%s' and %@='%d'",
	                                 kStarsColumnNamePlanetAmp, kStarsColumnNamePlanetPhi0,
	                                 kStarsColumnNamePlanetOmega, kStarsTableNameVSOP, kStarsColumnNamePlanetName,
	                                 sPlanetNames[planet], kStarsColumnNamePlanetParam, sParamNames[param],
	                                 kStarsColumnNamePlanetPower, power);
}

LString *SQL_DB_Strings::stars_selectSStarInfo()
{
	return LString::stringWithFormat("SELECT %@, %@, %@, %@, %@, %@ FROM %@", kStarsColumnNameHIP,
	                                 kStarsColumnNameRA, kStarsColumnNameDEC, kStarsColumnNameMAG,
	                                 kStarsColumnNamePLX, kStarsColumnNameBVCOL, kStarsTableNameStars);
}
