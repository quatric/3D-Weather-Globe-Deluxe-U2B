// SQLiteDatabase, SQLiteStatement, SQLiteResultsContainer, SQLiteTable,
// SQLiteUtils and the module functions.

#include "SQLiteLib.h"
#include "LCleanupTask.h"
#include "LScanner.h"
#include "UTextUtils.h"

LF_IMPLEMENT_CLASS(SQLiteDatabase, 0)
LF_IMPLEMENT_CLASS(SQLiteStatement, 0)
LF_IMPLEMENT_CLASS(SQLiteResultsContainer, 0)
LF_IMPLEMENT_CLASS(SQLiteTable, 0)

static CFBundleRef _sSQLiteLibBundle = 0;

class _SQLiteLibBundleCleanup : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		if (_sSQLiteLibBundle) {
			CFRelease(_sSQLiteLibBundle);
			_sSQLiteLibBundle = 0;
		}
	}
};
static _SQLiteLibBundleCleanup __SQLiteLibBundleCleanup;

void setSQLiteLibModule(CFBundleRef bundle)
{
	if (_sSQLiteLibBundle)
		CFRelease(_sSQLiteLibBundle);
	_sSQLiteLibBundle = bundle;
	if (bundle)
		CFRetain(bundle);
}

CFBundleRef getSQLiteLibModule()
{
	if (_sSQLiteLibBundle == 0)
		setSQLiteLibModule(CFBundleGetBundleWithIdentifier(CFSTR("com.mackiev.SQLiteLib")));
	return _sSQLiteLibBundle;
}

LObject *SQLiteDatabase::_configMap = 0;

class __SQLDBCleaner : public LCleanupTask {
public:
	virtual void performCleanup()
	{
		if (SQLiteDatabase::_configMap) {
			SQLiteDatabase::_configMap->release();
			SQLiteDatabase::_configMap = 0;
		}
	}
};
static __SQLDBCleaner _sSQLDBCleaner;

static LConstantString __symStr("'");
static LConstantString __doubleSymStr("''");

static int _sqlDummyResultCallback(void *, int, char **, char **)
{
	return 0;
}

// Adds a map of column name to value for the row to the LArray `context`.
static int _sqlResultCallback(void *context, int count, char **values, char **names)
{
	LMap *row = 0;
	for (int i = 0; i < count; i++) {
		if (values[i] == 0)
			continue;
		LString *name = LString::_alloc()->initWithCString(names[i], 0, kLMacRomanEncoding);
		LString *value = LString::_alloc()->initWithCString(values[i], 0, kLUTF8Encoding);
		if (value == 0)
			value = LString::_alloc()->initWithCString(values[i], 0, kLMacRomanEncoding);
		if (name && value) {
			if (row == 0)
				row = LMap::_alloc()->init();
			row->setObjectForKey(value, name);
		}
		LRelease(value);
		LRelease(name);
	}
	if (row) {
		((LArray *)context)->addObject(row);
		row->release();
	}
	return 0;
}

// ---------------------------------------------------------------------------
// SQLiteDatabase

SQLiteDatabase::SQLiteDatabase()
{
	if (_configMap == 0) {
		LObject *config = 0;
		LData *data = LData::dataWithResource(LString::stringWithCString("config.plist"), 0, getSQLiteLibModule());
		if (data) {
			config = data->propertyList();
			if (config && !config->getClass()->isSubclassOf(LF_CLASSOF(LMap)))
				config = 0;
		}
		_configMap = LRetain(config);
	}
	_path = 0;
	_db = 0;
	_tables = 0;
	_showTemporaryTables = false;
	_doLog = false;
	if (_configMap) {
		LObject *doLog = ((LMap *)_configMap)->objectForKey(LString::stringWithCString("DoLog"));
		_doLog = UMapParsingUtils::loadIntFromObject(doLog) != 0;
	}
}

void SQLiteDatabase::dealloc()
{
	_closeDatabase();
	LObject::dealloc();
}

void SQLiteDatabase::invalidateTables()
{
	if (_tables) {
		_tables->release();
		_tables = 0;
	}
}

void SQLiteDatabase::setShowTemporaryTables(unsigned char show)
{
	if (_showTemporaryTables != (bool)show) {
		_showTemporaryTables = show;
		invalidateTables();
	}
}

int SQLiteDatabase::checkAndGetInt(int column, int type, sqlite3_stmt *stmt)
{
	if (type == kSQLiteNull)
		return 0;
	return sqlite3_column_int(stmt, column);
}

double SQLiteDatabase::checkAndGetDouble(int column, int type, sqlite3_stmt *stmt)
{
	if (type == kSQLiteNull)
		return 0;
	return sqlite3_column_double(stmt, column);
}

void SQLiteDatabase::_closeDatabase()
{
	StSharableLocker lock(&_lock);
	if (_doLog && _db)
		LLog("Close database at '%@'\n", _path);
	if (_path) {
		_path->release();
		_path = 0;
	}
	invalidateTables();
	if (_db) {
		sqlite3_close(_db);
		_db = 0;
	}
}

void SQLiteDatabase::_openDatabase(const LString *path)
{
	_closeDatabase();
	int result;
	{
		StSharableLocker lock(&_lock);
		result = sqlite3_open(path->UTF8String(), &_db);
		if (_doLog)
			LLog("Open database at '%@'; result='%d'\n", path, result);
	}
	if (result != SQLITE_OK) {
		if (_db)
			sqlite3_close(_db);
		_db = 0;
		LF_RAISE(WrongConditionException());
	}
	processRequest((const LString *)LString::stringWithCString("select * from sqlite_master"));
	if (sqlite3_errcode(_db) != SQLITE_OK)
		LF_RAISE(WrongConditionException());
	_path = LRetain(LString::stringWithString(path));
}

SQLiteDatabase *SQLiteDatabase::initWithDatabasePath(const LString *path)
{
	if (LObject::init() == 0)
		return 0;
	_openDatabase(path);
	return this;
}

bool SQLiteDatabase::processRequestsWithData(LData *data, unsigned char continueOnError)
{
	// 0: CR LF, 1: LF, 2: CR.
	int style = UTextUtils::lineBreakStyleForData(data, false);
	if (style == -1)
		return false;
	const char *line = (const char *)data->bytes();
	const char *end = line + data->length();
	LData *buffer = LData::dataWithLength(0);
	char *error = 0;
	StSharableLocker lock(&_lock);
	int capacity = 0;
	char *request = 0;
	for (;;) {
		const char *p = line, *next;
		int left = (int)(end - line);
		for (;;) {
			if (p >= end) {
				next = end;
				break;
			}
			char c = *p;
			next = p + 1;
			if (c == '\r' && style == 0 && p[1] == '\n') {
				next = p + 2;
				break;
			}
			if ((c == '\n' && style == 1) || (c == '\r' && style == 2) || --left < 0)
				break;
			p++;
		}
		int length = (int)(p - line);
		if (capacity < length) {
			capacity = length + 10;
			buffer->setLength(capacity + 1);
			request = (char *)buffer->bytes();
		}
		if (request) {
			memcpy(request, line, length);
			request[length] = 0;
		}
		if (length != 0) {
			if (_doLog)
				LLog("Processing request from data:\n\t'%s'\n", request);
			int result = sqlite3_exec(_db, request, _sqlDummyResultCallback, 0, &error);
			if (error) {
				sqlite3_free(error);
				error = 0;
			}
			if (result != SQLITE_OK && !continueOnError)
				return false;
		}
		if (end - next <= 0)
			return true;
		line = next;
	}
}

bool SQLiteDatabase::processRequestsFromFile(LString *path, unsigned char continueOnError)
{
	LData *data = LData::dataWithContentsOfFile(path);
	if (data == 0)
		return false;
	return processRequestsWithData(data, continueOnError);
}

int SQLiteDatabase::processRequest(LString *request, TSQLiteRowCallback callback, void *context)
{
	StSharableLocker lock(&_lock);
	SQLiteStatement *statement = LAutorelease(SQLiteStatement::_alloc()->initWithDB(this, request));
	sqlite3_stmt *stmt = statement->statement();
	int count = sqlite3_column_count(stmt);
	int *types = (int *)LData::dataWithLength(count * 4)->bytes();
	if (_doLog)
		LLog("Processing request with callback:\n\t'%@'\n", request);
	int result;
	while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
		for (int i = 0; i < count; i++)
			types[i] = sqlite3_column_type(stmt, i);
		callback(count, types, stmt, context);
	}
	if (result == SQLITE_DONE)
		return 0;
	return sqlite3_errcode(_db);
}

LString *SQLiteDatabase::lastErrorString()
{
	StSharableLocker lock(&_lock);
	if (sqlite3_errcode(_db) == SQLITE_OK)
		return 0;
	return LString::stringWithCString(sqlite3_errmsg(_db), 0, kLUTF8Encoding);
}

LArray *SQLiteDatabase::processRequest(const LString *request)
{
	char *error = 0;
	LArray *rows = LArray::array();
	int result;
	{
		StSharableLocker lock(&_lock);
		if (_doLog)
			LLog("Processing simple request:\n\t'%@'\n", request);
		result = sqlite3_exec(_db, request->UTF8String(), _sqlResultCallback, rows, &error);
	}
	if (error)
		sqlite3_free(error);
	if (result != SQLITE_OK)
		return 0;
	return rows;
}

bool SQLiteDatabase::processRequestsInArray(LArray *requests)
{
	int count = requests ? requests->count() : 0;
	for (int i = 0; i < count; i++)
		if (processRequest((const LString *)requests->objectAtIndex(i)) == 0)
			return false;
	return true;
}

LArray *SQLiteDatabase::tables()
{
	if (_tables == 0) {
		LArray *rows = processRequest(
		    (const LString *)LString::stringWithCString("SELECT name FROM sqlite_master WHERE type IN ('table','view')"));
		int count = rows ? rows->count() : 0;
		_tables = LArray::_alloc()->initWithCapacity(count);
		for (int i = 0; i < count; i++) {
			LMapIterator it((LMap *)rows->objectAtIndex(i), false);
			if (it.next())
				_tables->addObject(SQLiteTable::tableWithDB(this, (LString *)it.object(), false));
		}
		if (_showTemporaryTables) {
			rows = processRequest((const LString *)LString::stringWithCString(
			    "SELECT name FROM sqlite_temp_master WHERE type IN ('table','view')"));
			count = rows ? rows->count() : 0;
			for (int i = 0; i < count; i++) {
				LMapIterator it((LMap *)rows->objectAtIndex(i), false);
				if (it.next())
					_tables->addObject(SQLiteTable::tableWithDB(this, (LString *)it.object(), true));
			}
		}
	}
	return _tables;
}

SQLiteTable *SQLiteDatabase::tableWithName(LString *name)
{
	LArray *all = tables();
	int count = all->count();
	for (int i = 0; i < count; i++) {
		SQLiteTable *table = (SQLiteTable *)all->objectAtIndex(i);
		if (name->compareIgnoringCase(table->_name) == kCFCompareEqualTo)
			return table;
	}
	return 0;
}

bool SQLiteDatabase::attachDatabase(const LString *path, const LString *name)
{
	LString *request = LString::stringWithFormat("ATTACH DATABASE \"%@\" AS '%@'", path, name);
	SQLiteStatement *statement = SQLiteStatement::_alloc()->initWithDB(this, request);
	statement->process();
	LRelease(statement);
	return sqlite3_errcode(_db) == SQLITE_OK;
}

// ---------------------------------------------------------------------------
// SQLiteStatement

SQLiteStatement::SQLiteStatement()
    : _stmt(0), _sql(0), _db(0), _columnNames(0), _columnTypes(0), _parameterNames(0), _buffer(0)
{
}

void SQLiteStatement::dealloc()
{
	if (_stmt) {
		sqlite3_finalize(_stmt);
		_stmt = 0;
	}
	if (_sql) {
		_sql->release();
		_sql = 0;
	}
	if (_columnNames) {
		_columnNames->release();
		_columnNames = 0;
	}
	if (_columnTypes) {
		_columnTypes->release();
		_columnTypes = 0;
	}
	if (_parameterNames) {
		_parameterNames->release();
		_parameterNames = 0;
	}
	if (_buffer) {
		_buffer->release();
		_buffer = 0;
	}
	LObject::dealloc();
}

SQLiteStatement *SQLiteStatement::initWithDB(SQLiteDatabase *db, LString *sql)
{
	if (LObject::init() == 0)
		return 0;
	_sql = LRetain(sql);
	_db = db;
	return this;
}

bool SQLiteStatement::_compileStatement()
{
	if (_stmt)
		return true;
	if (_db->_doLog)
		LLog("Compile statement:\n\t'%@'\n", _sql);
	return sqlite3_prepare_v2(_db->_db, _sql->UTF8String(), -1, &_stmt, 0) == SQLITE_OK;
}

sqlite3_stmt *SQLiteStatement::statement()
{
	if (_stmt == 0 && !_compileStatement())
		return 0;
	return _stmt;
}

void SQLiteStatement::reset()
{
	if (_stmt)
		sqlite3_reset(_stmt);
}

LArray *SQLiteStatement::columnNames()
{
	if (_columnNames == 0) {
		if (_stmt == 0 && !_compileStatement())
			return 0;
		int count = sqlite3_column_count(_stmt);
		_columnNames = LArray::_alloc()->initWithCapacity(count);
		for (int i = 0; i < count; i++) {
			const char *name = sqlite3_column_name(_stmt, i);
			_columnNames->addObject(LString::stringWithCString(name ? name : "UNKNOWN", 0,
			                                                   name ? kLUTF8Encoding : kLMacRomanEncoding));
		}
	}
	return _columnNames;
}

LArray *SQLiteStatement::columnTypes()
{
	if (_columnTypes == 0) {
		if (_stmt == 0 && !_compileStatement())
			return 0;
		int count = sqlite3_column_count(_stmt);
		_columnTypes = LArray::_alloc()->initWithCapacity(count);
		for (int i = 0; i < count; i++) {
			const char *type = sqlite3_column_decltype(_stmt, i);
			_columnTypes->addObject(LString::stringWithCString(type ? type : "UNKNOWN", 0,
			                                                   type ? kLUTF8Encoding : kLMacRomanEncoding));
		}
	}
	return _columnTypes;
}

// Rows are packed into _buffer and appended to the stream a chunk at a time.
SQLiteResultsContainer *SQLiteStatement::process()
{
	enum { kChunk = 0x4000 };
	if (_stmt == 0 && !_compileStatement())
		return 0;
	int count = sqlite3_column_count(_stmt);
	SQLiteResultsContainer *results = LAutorelease(SQLiteResultsContainer::_alloc()->initWithStatement(this));
	LData *stream = results->stream();
	LData *offsets = results->streamOffsets();
	unsigned int typesSize = count * 4;
	int *types = (int *)LData::dataWithLength(typesSize)->bytes();
	if (_buffer == 0) {
		_buffer = LData::_alloc()->init();
		_buffer->setLength(kChunk);
	}
	char *buffer = (char *)_buffer->bytes();
	StSharableLocker lock(&_db->_lock);
	if (_db->_doLog)
		LLog("Processing statement:\n\t'%@'\n", _sql);
	int used = 0;
	int result;
	while ((result = sqlite3_step(_stmt)) == SQLITE_ROW) {
		uint32_t offset = stream->length() + used;
		offsets->appendBytes(&offset, 4);
		for (int i = 0; i < count; i++)
			types[i] = sqlite3_column_type(_stmt, i);
		if ((int)(used + typesSize) > kChunk - 4) {
			stream->appendBytes(buffer, used);
			used = 0;
		}
		memcpy(buffer + used, types, typesSize);
		used += typesSize;
		for (int i = 0; i < count; i++) {
			int type = types[i];
			if (type == kSQLiteFloat) {
				float value = (float)sqlite3_column_double(_stmt, i);
				if (used > kChunk - 5) {
					stream->appendBytes(buffer, used);
					used = 0;
				}
				memcpy(buffer + used, &value, 4);
				used += 4;
			} else if (type == kSQLiteInteger) {
				int value = sqlite3_column_int(_stmt, i);
				if (used > kChunk - 5) {
					stream->appendBytes(buffer, used);
					used = 0;
				}
				memcpy(buffer + used, &value, 4);
				used += 4;
			} else if (type == kSQLiteText || type == kSQLiteBlob) {
				const void *bytes =
				    type == kSQLiteText ? (const void *)sqlite3_column_text(_stmt, i) : sqlite3_column_blob(_stmt, i);
				uint32_t length = sqlite3_column_bytes(_stmt, i);
				if ((int)(length + used) > kChunk - 1) {
					stream->appendBytes(buffer, used);
					used = 0;
				}
				if ((int)length < kChunk - 4) {
					memcpy(buffer + used, &length, 4);
					memcpy(buffer + used + 4, bytes, length);
					used += 4 + length;
				} else {
					stream->appendBytes(&length, 4);
					stream->appendBytes(bytes, length);
				}
			}
		}
	}
	if (result != SQLITE_DONE)
		return 0;
	if (used)
		stream->appendBytes(buffer, used);
	return results;
}

SQLiteResultsContainer *SQLiteStatement::processWithParameters(LMap *params)
{
	if (_db->_doLog)
		LLog("\nBinding parameters to statement:'%@'\n", _sql);
	int count;
	if (_parameterNames == 0) {
		if (_stmt == 0 && !_compileStatement())
			return 0;
		count = sqlite3_bind_parameter_count(_stmt);
		_parameterNames = LArray::_alloc()->initWithCapacity(count);
		// Without the ':' (or '@', '$') prefix.
		for (int i = 1; i <= count; i++)
			_parameterNames->addObject(LString::stringWithCString(sqlite3_bind_parameter_name(_stmt, i) + 1));
	} else {
		count = _parameterNames->count();
	}
	StSharableLocker lock(&_db->_lock);
	for (int i = 0; i < count; i++) {
		LObject *value = params->objectForKey((LString *)_parameterNames->objectAtIndex(i));
		if (value == 0)
			LF_RAISE(NilPtrException());
		if (_db->_doLog)
			LLog("\tPARAM_%d = '%@'\n", i, value);
		if (LData *data = lobj_cast<LData>(value)) {
			if (sqlite3_bind_blob(_stmt, i + 1, data->bytes(), data->length(), SQLITE_TRANSIENT) != SQLITE_OK)
				LF_RAISE(WrongConditionException());
		} else if (LString *str = lobj_cast<LString>(value)) {
			const char *utf8 = str->UTF8String();
			if (sqlite3_bind_text(_stmt, i + 1, utf8, (int)strlen(utf8), SQLITE_TRANSIENT) != SQLITE_OK)
				LF_RAISE(WrongConditionException());
		} else if (LValue *number = lobj_cast<LValue>(value)) {
			if (sqlite3_bind_double(_stmt, i + 1, number->doubleValue()) != SQLITE_OK)
				LF_RAISE(WrongConditionException());
		}
	}
	return process();
}

// ---------------------------------------------------------------------------
// SQLiteResultsContainer

SQLiteResultsContainer::SQLiteResultsContainer()
    : _statement(0), _columnNames(0), _columnTypes(0), _names(0), _stream(0), _offsets(0), _columnsCount(0)
{
}

void SQLiteResultsContainer::dealloc()
{
	if (_names)
		free(_names);
	if (_columnNames) {
		_columnNames->release();
		_columnNames = 0;
	}
	if (_columnTypes) {
		_columnTypes->release();
		_columnTypes = 0;
	}
	if (_stream) {
		_stream->release();
		_stream = 0;
	}
	if (_offsets) {
		_offsets->release();
		_offsets = 0;
	}
	LObject::dealloc();
}

SQLiteResultsContainer *SQLiteResultsContainer::initWithStatement(SQLiteStatement *statement)
{
	if (LObject::init() == 0)
		return 0;
	_statement = statement;
	_columnNames = LRetain(statement->columnNames());
	_columnTypes = LRetain(_statement->columnTypes());
	_columnsCount = _columnNames->count();
	_names = (LString **)malloc(_columnsCount * sizeof(LString *));
	for (unsigned int i = 0; i < _columnsCount; i++)
		_names[i] = (LString *)_columnNames->objectAtIndex(i);
	return this;
}

LData *SQLiteResultsContainer::stream()
{
	if (_stream == 0)
		_stream = LData::_alloc()->init();
	return _stream;
}

LData *SQLiteResultsContainer::streamOffsets()
{
	if (_offsets == 0)
		_offsets = LData::_alloc()->init();
	return _offsets;
}

unsigned int SQLiteResultsContainer::count()
{
	if (_offsets == 0)
		return 0;
	return _offsets->length() / 4;
}

bool SQLiteResultsContainer::loadValues(void **values, int *types, int count, unsigned int index)
{
	const char *row = (const char *)_stream->bytes() + ((const uint32_t *)_offsets->bytes())[index];
	const int *rowTypes = (const int *)row;
	const char *p = row + _columnsCount * 4;
	float parsed = 0;
	for (unsigned int i = 0; i < _columnsCount && (int)i < count; i++) {
		switch (rowTypes[i]) {
		case kSQLiteFloat: {
			float value;
			memcpy(&value, p, 4);
			p += 4;
			if (types[i] == 1)
				*(int *)values[i] = (int)value;
			else if (types[i] == 2)
				*(float *)values[i] = value;
			else
				return false;
			break;
		}
		case kSQLiteInteger: {
			int value;
			memcpy(&value, p, 4);
			p += 4;
			if (types[i] == 1)
				*(int *)values[i] = value;
			else if (types[i] == 2)
				*(float *)values[i] = (float)value;
			else
				return false;
			break;
		}
		case kSQLiteText:
		case kSQLiteBlob: {
			uint32_t length;
			memcpy(&length, p, 4);
			p += 4;
			int type = types[i];
			if (type == 1 || type == 2) {
				char text[20];
				unsigned int n = length < 20 ? length : 19;
				memcpy(text, p, n);
				text[n] = 0;
				sscanf(text, "%f", &parsed);
			}
			if (type == 2)
				*(float *)values[i] = parsed;
			else if (type == 1)
				*(int *)values[i] = (int)parsed;
			else if (type == 3 || type == 4) {
				memcpy(values[i], p, length);
				if (type == 3)
					((char *)values[i])[length] = 0;
			} else
				return false;
			p += length;
			break;
		}
		default:
			break;
		}
	}
	return true;
}

LMap *SQLiteResultsContainer::objectAtIndex(unsigned int index)
{
	const char *row = (const char *)_stream->bytes() + ((const uint32_t *)_offsets->bytes())[index];
	const int *rowTypes = (const int *)row;
	const char *p = row + _columnsCount * 4;
	LMap *map = LMap::map();
	for (unsigned int i = 0; i < _columnsCount; i++) {
		LObject *value = 0;
		switch (rowTypes[i]) {
		case kSQLiteFloat: {
			float f;
			memcpy(&f, p, 4);
			p += 4;
			value = LValue::valueWithFloat(f);
			break;
		}
		case kSQLiteInteger: {
			int n;
			memcpy(&n, p, 4);
			p += 4;
			value = LValue::valueWithInt(n);
			break;
		}
		case kSQLiteText: {
			uint32_t length;
			memcpy(&length, p, 4);
			p += 4;
			if (length == 0) {
				value = LString::stringWithCString("");
			} else {
				value = LString::stringWithCString(p, length, kLUTF8Encoding);
				if (value == 0)
					value = LString::stringWithCString(p, length, kLMacRomanEncoding);
			}
			p += length;
			break;
		}
		case kSQLiteBlob: {
			uint32_t length;
			memcpy(&length, p, 4);
			p += 4;
			value = LData::dataWithBytes(p, length);
			p += length;
			break;
		}
		default:
			break;
		}
		if (value)
			map->setObjectForKey(value, _names[i]);
	}
	return map;
}

struct SSortContext {
	SQLiteResultsContainer::TSortCallback callback;
	const void *context;
	const char *stream;
	unsigned int columnsCount;
};

int SQLiteResultsContainer::_resultsSortCallback(const void *a, const void *b, const void *context)
{
	const SSortContext *sort = (const SSortContext *)context;
	const char *row1 = sort->stream + *(const uint32_t *)a;
	const char *row2 = sort->stream + *(const uint32_t *)b;
	unsigned int typesSize = sort->columnsCount * 4;
	return sort->callback(sort->columnsCount, (const int *)row1, (const int *)row2, row1 + typesSize,
	                      row2 + typesSize, sort->context);
}

void SQLiteResultsContainer::sortWithCallback(TSortCallback callback, const void *context)
{
	SSortContext sort = {callback, context, (const char *)stream()->bytes(), _columnsCount};
	QSORT::sortInContext(streamOffsets()->bytes(), count(), 4, &sort, _resultsSortCallback);
}

// ---------------------------------------------------------------------------
// SQLiteTable

void SQLiteTable::dealloc()
{
	if (_sql) {
		_sql->release();
		_sql = 0;
	}
	if (_name) {
		_name->release();
		_name = 0;
	}
	if (_columnNames) {
		_columnNames->release();
		_columnNames = 0;
	}
	if (_columnTypes) {
		_columnTypes->release();
		_columnTypes = 0;
	}
	LObject::dealloc();
}

SQLiteTable *SQLiteTable::initWithDB(SQLiteDatabase *db, LString *name, unsigned char temporary)
{
	if (LObject::init() == 0)
		return 0;
	_temporary = temporary;
	_db = db;
	_name = LRetain(name);
	const char *format = temporary
	                         ? "SELECT sql FROM sqlite_temp_master WHERE type IN ('table','view') AND tbl_name=\"%@\""
	                         : "SELECT sql FROM sqlite_master WHERE type IN ('table','view') AND tbl_name=\"%@\"";
	LArray *rows = _db->processRequest((const LString *)LString::stringWithFormat(format, name));
	LMap *row = rows && rows->count() == 1 ? (LMap *)rows->objectAtIndex(0) : 0;
	_sql = 0;
	if (row) {
		LMapIterator it(row, false);
		_sql = it.next() ? (LString *)it.object() : 0;
	}
	LRetain(_sql);
	SQLiteStatement *statement =
	    SQLiteStatement::_alloc()->initWithDB(_db, LString::stringWithFormat("SELECT * FROM %@", name));
	_columnNames = LRetain(statement->columnNames());
	_columnTypes = LRetain(statement->columnTypes());
	LRelease(statement);
	return this;
}

SQLiteTable *SQLiteTable::tableWithDB(SQLiteDatabase *db, LString *name, unsigned char temporary)
{
	return LAutorelease(_alloc()->initWithDB(db, name, temporary));
}

// ---------------------------------------------------------------------------
// SQLiteUtils

LString *_stringByEncodeAsSQLValue(LString *str, unsigned char quote)
{
	if (str == 0)
		return 0;
	unsigned int length = str->length();
	const UniChar *chars = str->chars();
	for (unsigned int i = 0; i < length; i++) {
		if (chars[i] == '\'') {
			str = LString::stringWithString(str);
			str->replaceOccurrencesOfString(&__symStr, &__doubleSymStr);
			break;
		}
	}
	if (!quote)
		return str;
	LString *result = LString::stringWithCString("'");
	result->appendString(str);
	result->appendString(LString::stringWithCString("'"));
	return result;
}

LString *SQLiteUtils::stringByEncodeAsSQLValue(LValue *value)
{
	if (value == 0)
		return 0;
	return value->fillTextRepresentation(0);
}

LString *SQLiteUtils::stringByEncodeAsSQLValueWOBracers(LString *str)
{
	return _stringByEncodeAsSQLValue(str, false);
}

LString *SQLiteUtils::stringByEncodeAsSQLValue(LString *str)
{
	return _stringByEncodeAsSQLValue(str, true);
}

LString *SQLiteUtils::stringByEncodeAsSQLValue(LData *data)
{
	static const char remap[] = "0123456789ABCDEF";
	if (data == 0)
		return 0;
	unsigned int length = data->length();
	const unsigned char *bytes = (const unsigned char *)data->bytes();
	LData *text = LData::_alloc()->init();
	text->setLength(length * 2 + 3);
	char *p = (char *)text->bytes();
	p[0] = 'X';
	p[1] = '\'';
	p[length * 2 + 2] = '\'';
	for (unsigned int i = 0; i < length; i++) {
		p[2 + i * 2] = remap[bytes[i] >> 4];
		p[3 + i * 2] = remap[bytes[i] & 0xf];
	}
	LString *result = LAutorelease(
	    LString::_alloc()->initWithCString((const char *)text->bytes(), length * 2 + 3, kLMacRomanEncoding));
	text->release();
	return result;
}

LArray *SQLiteUtils::arrayByEncodingAllAsSQLValues(LArray *values)
{
	if (values == 0)
		return 0;
	unsigned int count = values->count();
	LArray *result = LArray::arrayWithCapacity(count);
	for (unsigned int i = 0; i < count; i++) {
		LObject *value = values->objectAtIndex(i);
		LObject *encoded = 0;
		if (LString *str = lobj_cast<LString>(value))
			encoded = _stringByEncodeAsSQLValue(str, true);
		else if (LValue *number = lobj_cast<LValue>(value))
			encoded = number->fillTextRepresentation(0);
		else if (LData *data = lobj_cast<LData>(value))
			encoded = stringByEncodeAsSQLValue(data);
		if (encoded)
			result->addObject(encoded);
	}
	return result;
}

LString *SQLiteUtils::stringAsSQLValuesList(LArray *values)
{
	LArray *encoded = arrayByEncodingAllAsSQLValues(values);
	if (encoded == 0)
		return 0;
	return encoded->componentsJoinedByString(LString::stringWithCString(", "));
}

LArray *SQLiteUtils::separatedSearchTermsForString(LString *str)
{
	// Words are runs of letters, digits and quotes.
	LCharSet *word = LAutorelease(LCharSet::_alloc()->initWithCharSet(LCharSet::charSetWithType(LCharSet::kAlphaNumeric)));
	word->formUnionWithCharacterSet(LCharSet::characterSetWithCharactersInCString("'\"`"));
	LCharSet *separators = word->invertedSet();
	LScanner *scanner = LScanner::scannerWithString(str);
	LArray *terms = LArray::array();
	scanner->scanToCharSet(word, 0);
	LString *term;
	while (scanner->scanToCharSet(separators, &term)) {
		terms->addObject(term->uppercaseString());
		scanner->scanToCharSet(word, 0);
	}
	return terms;
}
