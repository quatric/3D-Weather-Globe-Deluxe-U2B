// SQLiteLib: a thin LFoundation wrapper around SQLite. The original framework
// embedded SQLite 3.6.11; this one links the system libsqlite3.

#ifndef SQLITELIB_H
#define SQLITELIB_H

#include <sqlite3.h>
#include "LObject.h"
#include "LString.h"
#include "LData.h"
#include "LArray.h"
#include "LMap.h"
#include "LValue.h"

using namespace LF;

CFBundleRef getSQLiteLibModule();
void setSQLiteLibModule(CFBundleRef bundle);

class SQLiteTable;
class SQLiteStatement;
class SQLiteResultsContainer;

// Column types as sqlite3_column_type returns them.
enum {
	kSQLiteInteger = SQLITE_INTEGER,	// 1
	kSQLiteFloat = SQLITE_FLOAT,		// 2
	kSQLiteText = SQLITE_TEXT,		// 3
	kSQLiteBlob = SQLITE_BLOB,		// 4
	kSQLiteNull = SQLITE_NULL		// 5
};

// Called for every row: the column count, their types and the statement to
// read the values from.
typedef int (*TSQLiteRowCallback)(int count, int *types, sqlite3_stmt *stmt, void *context);

// One database file. All access goes through its lock.
class SQLiteDatabase : public LObject {
	LF_DECLARE_CLASS(SQLiteDatabase)
public:
	SQLiteDatabase();
	virtual void dealloc();
	virtual SQLiteDatabase *getOwnerLObject() { return this; }

	SQLiteDatabase *initWithDatabasePath(const LString *path);

	// Runs every line of the data (or the file) as a request; stops at the
	// first failure unless `continueOnError`.
	bool processRequestsWithData(LData *data, unsigned char continueOnError);
	bool processRequestsFromFile(LString *path, unsigned char continueOnError);
	// The rows as LMaps of column name to text; 0 on error.
	LArray *processRequest(const LString *request);
	bool processRequestsInArray(LArray *requests);
	// 0, or the sqlite error code.
	int processRequest(LString *request, TSQLiteRowCallback callback, void *context);
	LString *lastErrorString();

	LArray *tables();
	SQLiteTable *tableWithName(LString *name);
	void invalidateTables();
	void setShowTemporaryTables(unsigned char show);
	bool attachDatabase(const LString *path, const LString *name);

	// Read a column of the current row; NULL reads as 0.
	static int checkAndGetInt(int column, int type, sqlite3_stmt *stmt);
	static double checkAndGetDouble(int column, int type, sqlite3_stmt *stmt);

	void _openDatabase(const LString *path);
	void _closeDatabase();

	static LObject *_configMap;

	CSharableResource _lock;
	sqlite3 *_db;
	LString *_path;
	LArray *_tables;
	bool _showTemporaryTables;
	bool _doLog;
};

// A compiled request. Its results come back as an SQLiteResultsContainer.
class SQLiteStatement : public LObject {
	LF_DECLARE_CLASS(SQLiteStatement)
public:
	SQLiteStatement();
	virtual void dealloc();
	virtual SQLiteStatement *getOwnerLObject() { return this; }

	SQLiteStatement *initWithDB(SQLiteDatabase *db, LString *sql);
	sqlite3_stmt *statement();
	void reset();
	LArray *columnNames();
	LArray *columnTypes();
	SQLiteResultsContainer *process();
	// Binds the values of `params` (keyed by parameter name) then processes.
	SQLiteResultsContainer *processWithParameters(LMap *params);

	bool _compileStatement();

	sqlite3_stmt *_stmt;
	LString *_sql;
	SQLiteDatabase *_db;
	LArray *_columnNames;
	LArray *_columnTypes;
	LArray *_parameterNames;
	LData *_buffer;
};

// The rows of a processed statement, packed in one stream: for every row, the
// column types (4 bytes each), then the values: 4 bytes for an int or a float,
// a 4-byte length and the bytes for text and blobs, nothing for NULL.
class SQLiteResultsContainer : public LObject {
	LF_DECLARE_CLASS(SQLiteResultsContainer)
public:
	SQLiteResultsContainer();
	virtual void dealloc();
	virtual SQLiteResultsContainer *getOwnerLObject() { return this; }

	SQLiteResultsContainer *initWithStatement(SQLiteStatement *statement);
	unsigned int count();
	// The row as a map of column name to LValue, LString or LData.
	LMap *objectAtIndex(unsigned int index);
	// Reads the first `count` columns of the row into `values`, converting to
	// `types`: 1 int, 2 float, 3 C string, 4 raw bytes.
	bool loadValues(void **values, int *types, int count, unsigned int index);
	// Sorts the rows; the callback gets the column count, both rows' types and
	// both rows' values.
	typedef int (*TSortCallback)(int count, const int *types1, const int *types2, const char *values1,
	                             const char *values2, const void *context);
	void sortWithCallback(TSortCallback callback, const void *context);

	LData *stream();
	LData *streamOffsets();

	static int _resultsSortCallback(const void *a, const void *b, const void *context);

	SQLiteStatement *_statement;
	LArray *_columnNames;
	LArray *_columnTypes;
	LString **_names;
	LData *_stream;
	LData *_offsets;
	unsigned int _columnsCount;
};

// A table (or view) of a database: its creation SQL and its columns.
class SQLiteTable : public LObject {
	LF_DECLARE_CLASS(SQLiteTable)
public:
	SQLiteTable() {}
	virtual void dealloc();
	virtual SQLiteTable *getOwnerLObject() { return this; }

	static SQLiteTable *tableWithDB(SQLiteDatabase *db, LString *name, unsigned char temporary);
	SQLiteTable *initWithDB(SQLiteDatabase *db, LString *name, unsigned char temporary);

	SQLiteDatabase *_db;
	LString *_name;
	LString *_sql;
	LArray *_columnNames;
	LArray *_columnTypes;
	bool _temporary;
};

namespace SQLiteUtils {
LString *stringByEncodeAsSQLValue(LValue *value);
// Quoted, with quotes doubled.
LString *stringByEncodeAsSQLValue(LString *str);
LString *stringByEncodeAsSQLValueWOBracers(LString *str);
// As a blob literal: X'0123...'.
LString *stringByEncodeAsSQLValue(LData *data);
LArray *arrayByEncodingAllAsSQLValues(LArray *values);
// "v1, v2, ..."
LString *stringAsSQLValuesList(LArray *values);
// The words of the string, upper-cased.
LArray *separatedSearchTermsForString(LString *str);
} // namespace SQLiteUtils

LString *_stringByEncodeAsSQLValue(LString *str, unsigned char quote);

#endif
