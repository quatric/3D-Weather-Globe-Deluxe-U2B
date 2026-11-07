// CThreadData: per-thread storage of LObjects by key. CLocalThreadData is a
// key whose value is an LMap of its own on each thread.
#pragma once

#include "LCleanupTask.h"
#include "LString.h"

namespace LF {

class CThreadData {
public:
	CThreadData();
	virtual ~CThreadData();

	// The calling thread's data; 0 once cleanupAll has run.
	static CThreadData *threadData();
	static void cleanupAll();
	static void _pthreadKeyCleanup(void *data);

	LObject *objectForKey(LComparable *key);
	void setObjectForKey(LObject *obj, LComparable *key);

	LMap *_map;
	pthread_t _thread;

	static TArray<CThreadData *> _allDatas;
	static CSharableResource _locker;
	static pthread_key_t _threadKey;
	static bool _wasCleanedUp;
};

class CLocalThreadData {
public:
	CLocalThreadData();
	~CLocalThreadData();
	LMap *perThreadMap();

	LString *_key;
};

class CThreadDataCleaner : public LCleanupTask {
public:
	virtual void performCleanup() { CThreadData::cleanupAll(); }
};

} // namespace LF
