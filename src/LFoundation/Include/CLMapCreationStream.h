// CLMapCreationStream: parses old-style property lists ("{ key = value; }",
// "( a, b )", "<data>" and strings) from an LData.
#pragma once

#include "CStream.h"
#include "LString.h"

namespace LF {

class CLMapCreationStream : public CTextBlockStream {
public:
	CLMapCreationStream(const LData *data);
	virtual ~CLMapCreationStream();
	using CTextBlockStream::GetBlockOffsetStart;
	using CTextBlockStream::GetBlockOffsetEnd;
	// Like CTextBlockStream's, with any brackets; also skips comments.
	virtual unsigned int GetBlockOffsetStart(char open, char close);
	virtual unsigned int GetBlockOffsetEnd(char open, char close);

	// The next object: a string, "@..." token, array, map or data.
	LObject *scanObject();
	// Reads "key = value;" pairs up to `end` into `map`.
	void scanMap(unsigned int end, LMap *map);
	void scanArray(unsigned int end, LArray *array);
	LString *loadString();

	TArray<unsigned int> *lineStartMap();
	int _currentLineNumber();
	void _throwOnSyntaxError(const char *message, const char *file, int line);

	LData *_lData;
	TArray<unsigned int> *_lineStarts;
};

} // namespace LF
