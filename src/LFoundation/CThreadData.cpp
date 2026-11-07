// CThreadData
//
// LF::CThreadData, LF::CLocalThreadData and LF::CThreadDataCleaner.

#include "CThreadData.h"
#include "LMap.h"

namespace LF {

TArray<CThreadData *> CThreadData::_allDatas;
CSharableResource CThreadData::_locker;
pthread_key_t CThreadData::_threadKey = 0;
bool CThreadData::_wasCleanedUp = false;

static CThreadDataCleaner sThreadDataCleaner;

CThreadData::CThreadData()
{
	StSharableLocker locker(&_locker);
	_allDatas.addObject(this);
	_map = 0;
	_thread = pthread_self();
}

CThreadData::~CThreadData()
{
	if (_map) {
		_map->release();
		_map = 0;
	}
	_allDatas.removeObject(this);
}

LObject *CThreadData::objectForKey(LComparable *key)
{
	return _map ? _map->objectForKey(key) : 0;
}

void CThreadData::setObjectForKey(LObject *obj, LComparable *key)
{
	if (_map == 0)
		_map = LMap::_alloc()->init();
	_map->setObjectForKey(obj, key);
}

// Each deletion takes the data out of _allDatas. (The original walked the
// array as it shrank, deleting some datas twice when there were several.)
void CThreadData::cleanupAll()
{
	_wasCleanedUp = true;
	{
		StSharableLocker locker(&_locker);
		while (_allDatas.count())
			delete _allDatas.lastObject();
	}
	pthread_key_t key = _threadKey;
	_threadKey = 0;
	if (key)
		pthread_key_delete(key);
}

void CThreadData::_pthreadKeyCleanup(void *data)
{
	if (_wasCleanedUp)
		return;
	StSharableLocker locker(&_locker);
	for (int i = 0; i < (int)_allDatas.count(); i++) {
		if (data == _allDatas[i]) {
			// The destructor removes it from _allDatas.
			delete (CThreadData *)data;
			break;
		}
	}
	if (_threadKey)
		pthread_setspecific(_threadKey, 0);
}

CThreadData *CThreadData::threadData()
{
	if (_wasCleanedUp)
		return 0;
	if (_threadKey == 0) {
		StSharableLocker locker(&_locker);
		if (_threadKey == 0) {
			pthread_key_create(&_threadKey, _pthreadKeyCleanup);
			if (_threadKey == 0)
				LF_RAISE(WrongConditionException());
		}
	}
	CThreadData *data = (CThreadData *)pthread_getspecific(_threadKey);
	if (data == 0) {
		data = new CThreadData;
		pthread_setspecific(_threadKey, data);
	}
	return data;
}

CLocalThreadData::CLocalThreadData()
{
	char name[112];
	snprintf(name, sizeof name, "<%p> (CLocalThreadData)", (void *)this);
	_key = LString::_alloc()->initWithCString(name, 0, kLMacRomanEncoding);
}

CLocalThreadData::~CLocalThreadData()
{
	if (_key) {
		_key->release();
		_key = 0;
	}
}

LMap *CLocalThreadData::perThreadMap()
{
	CThreadData *data = CThreadData::threadData();
	LMap *map = data->_map ? (LMap *)data->_map->objectForKey(_key) : 0;
	if (map == 0) {
		map = LMap::map();
		data->setObjectForKey(map, _key);
	}
	return map;
}

} // namespace LF
