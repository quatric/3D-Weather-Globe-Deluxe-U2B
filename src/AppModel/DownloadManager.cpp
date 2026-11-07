// DownloadManager: the queue of downloads and the conversation with the
// MacKiev server: the authorization (a registration code made from the MAC
// address), the activation of the serial number and the database updates. The
// server answers 250 with a property list of actions to perform.

#include "AppModel.h"

LF_IMPLEMENT_CLASS(DownloadManager, 0)
LF_SELECTOR(DownloadManager, _connectionClosed);
LF_SELECTOR(DownloadManager, _authRequestCompleted);
LF_SELECTOR(DownloadManager, _activateRequestCompleted);
LF_SELECTOR(DownloadManager, _dylibLoadCompleted);
LF_SELECTOR(DownloadManager, _refreshRequestCompleted);
LF_SELECTOR(DownloadManager, _getObjectRequestCompleted);

// The product number the server knows the application by.
static const int kProductID = 42;

// The status the server answers with when it sends actions.
static const int kStatusActions = 250;

void DownloadManager::dealloc()
{
	if (_updateFiles) {
		_updateFiles->release();
		_updateFiles = 0;
	}
	if (_tasks) {
		_tasks->release();
		_tasks = 0;
	}
	if (_connections) {
		_connections->release();
		_connections = 0;
	}
	LObject::dealloc();
}

bool DownloadManager::isInternetAvailable()
{
	return LConnection::isOnline();
}

LString *DownloadManager::_hostName()
{
	return LString::stringFromTable("host", "PathComponents", getAppModelModule());
}

void DownloadManager::terminateUpdating()
{
	if (_updateDelegate == 0)
		return;
	if (_getObjectConnection) {
		LNotificationCenter::center()->removeObserver(this, LConnection::connectionClosedNotification,
		                                              _getObjectConnection);
		_getObjectConnection->terminate();
		_getObjectConnection = 0;
	}
	_updateDelegate->updateWasFinished(0);
}

void DownloadManager::pauseQueue()
{
	_paused = true;
}

bool DownloadManager::authorizationFailed(unsigned char *quietly)
{
	if (quietly)
		*quietly = _offline;
	return _authorizationInvalid || (!_authorized && _authorizationDone);
}

bool DownloadManager::isAuthorizing()
{
	return _authorizing;
}

bool DownloadManager::updatesAreAvailable()
{
	return _updateFiles && _updateFiles->count() > 0;
}

// The machine's own key (its MAC address as twelve hex digits).
static int GetMACAddress(unsigned char *address)
{
	LString *key = SHIFER_ownKey();
	if (key == 0 || key->length() < 12)
		return -1;
	unsigned int bytes[6];
	if (sscanf(key->cString(), "%02X%02X%02X%02X%02X%02X", &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4],
	           &bytes[5]) != 6)
		return -1;
	for (int i = 0; i < 6; i++)
		address[i] = (unsigned char)bytes[i];
	return 0;
}

// The last four bytes of the MAC address, or 0xbadaddec.
unsigned long getMacAdInfo()
{
	unsigned char address[6];
	if (GetMACAddress(address) != 0)
		return 0xbadaddec;
	return ((unsigned long)address[2] << 24) | ((unsigned long)address[3] << 16) | ((unsigned long)address[4] << 8) |
	       address[5];
}

void DownloadManager::invalidate()
{
	LNotificationCenter::center()->removeObserver(this);
	int count = _connections ? _connections->count() : 0;
	for (int i = 0; i < count; i++)
		((LConnection *)_connections->objectAtIndex(i))->terminate();
	if (_refreshConnection) {
		_refreshConnection->terminate();
		_refreshConnection = 0;
	}
	if (_authConnection) {
		_authConnection->terminate();
		_authConnection = 0;
	}
	if (_activationConnection) {
		_activationConnection->terminate();
		_activationConnection = 0;
	}
	if (_connections) {
		_connections->release();
		_connections = 0;
	}
	if (_tasks) {
		_tasks->release();
		_tasks = 0;
	}
	classObject()->killSingleton();
}

void DownloadManager::_taskFinishedWithInfo(LMap *task, LData *data, int status)
{
	LObject *owner = task->objectForKey(&kTaskDelegate);
	LMap *params = (LMap *)task->objectForKey(&kTaskParametersDictionary);
	IDownloadDelegate *delegate = owner ? dynamic_cast<IDownloadDelegate *>(owner) : 0;
	LF_CHECK_PTR(delegate);
	delegate->downloadFinishedForTask(params, data, status);
}

static bool _isServiceTask(LMap *task)
{
	LMap *params = lobj_cast<LMap>(task->objectForKey(&kTaskParametersDictionary));
	LString *url = params ? lobj_cast<LString>(params->objectForKey(DownloadManager::kTaskParameterRequestString)) : 0;
	return url && OnlineServices::handlesURL(url);
}

// Fails the queued tasks that wait for the authorization.
void DownloadManager::_clearTasksQueue(int status)
{
	for (int i = 0; _tasks && i < (int)_tasks->count();) {
		LMap *task = (LMap *)_tasks->objectAtIndex(i);
		if (_isServiceTask(task)) {
			i++;
			continue;
		}
		LAutorelease(LRetain(task));
		_tasks->removeObjectAtIndex(i);
		_taskFinishedWithInfo(task, 0, status);
	}
}

// Checks a registration code: "WA" six hex digits "-" eight "-" eight and a
// two digit checksum. Gives the product and the masked MAC address.
static bool digest(const char *code, unsigned long *product, unsigned long *mac)
{
	if (code[0] != 'W' || code[1] != 'A' || code[8] != '-' || code[17] != '-')
		return false;
	auto hex = [](char c) -> int { return c < 'A' ? c - '0' : (c < 'G' ? c - 'A' + 10 : c - 'G'); };
	unsigned int first = 0, second = 0, third = 0, check = 0;
	char sum1 = 0, sum2 = 0, sum3 = 0;
	for (int i = 0; i < 6; i++) {
		sum1 += code[2 + i];
		first = first * 16 + hex(code[2 + i]);
	}
	for (int i = 0; i < 8; i++) {
		sum2 += code[9 + i];
		second = second * 16 + hex(code[9 + i]);
	}
	for (int i = 0; i < 8; i++) {
		sum3 += code[18 + i];
		third = third * 16 + hex(code[18 + i]);
	}
	for (int i = 0; i < 2; i++)
		check = check * 16 + hex(code[26 + i]);
	if (check != (unsigned char)(sum1 + sum2 + sum3))
		return false;
	*product = ((first << 8) ^ third) >> 24 & 0x3f;
	*mac = third ^ second;
	return true;
}

LString *DownloadManager::_registrationCode()
{
	LString *code = LUserDefaults::standardUserDefaults()->stringForKey(&kRegistrationKey);
	return code && code->length() > 27 ? code : 0;
}

bool DownloadManager::_registrationIsValid()
{
	LString *code = _registrationCode();
	const char *chars = code ? code->cString() : 0;
	unsigned long product, mac;
	if (chars == 0 || !digest(chars, &product, &mac) || product != kProductID)
		return false;
	unsigned long own = getMacAdInfo();
	return own != 0 && own == mac;
}

static int _buildNumber()
{
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	return callbacks ? callbacks->_applicationBuildNumber() : 0;
}

LConnection *DownloadManager::_prepareGetObjectRequest(LString *name)
{
	LString *url = LString::stringWithFormat("%@%s/%@", _hostName(), "/wa2k/nph-getobject2", name);
	LConnection *connection = LConnection::_alloc()->initWithURLMethodInfo(url, LString::stringWithCString("POST"), 0);
	LF_CHECK_PTR(connection);
	LAutorelease(connection);
	LString *code = _registrationCode();
	if (!_registrationIsValid() || code == 0)
		code = LString::stringWithCString("");
	int version = DynamicEnvironment::instance()->currentVersion();
	char body[1000];
	snprintf(body, sizeof body, "sid=%s&cfb=%d&dll=%d", code->cString(), _buildNumber(), version);
	connection->setBody(LData::dataWithBytes(body, strlen(body)));
	connection->setUserNameAndPasswd(&sUserName, &sUserPassword, 0);
	LNotificationCenter::center()->addObserver(this, "_getObjectRequestCompleted",
	                                           LConnection::connectionClosedNotification, connection);
	connection->setUserInfo(name);
	if (_getObjectConnection) {
		_getObjectConnection->release();
		_getObjectConnection = 0;
	}
	_getObjectConnection = connection;
	return connection;
}

void DownloadManager::_downloadNextUpdateFile()
{
	if (_updateFiles == 0) {
		terminateUpdating();
		return;
	}
	if (_updateFiles->count() == 0) {
		if (_updateDelegate)
			_updateDelegate->updateWasFinished(1);
		return;
	}
	LString *name = (LString *)LAutorelease(LRetain(_updateFiles->objectAtIndex(0)));
	_updateFiles->removeObjectAtIndex(0);
	LConnection *connection = _prepareGetObjectRequest(name);
	if (_updateDelegate && !_updateDelegate->fileWillBeDownloadedWithName(name, connection) && _updateDelegate)
		return;
	if (connection)
		connection->fetch();
}

bool DownloadManager::updateWithDelegate(IDBUpdateDelegate *delegate, LComparableSet *skip)
{
	if (_updateDelegate)
		return false;
	_updateDelegate = delegate;
	if (skip && skip->count() != 0 && _updateFiles) {
		for (int i = _updateFiles->count() - 1; i >= 0; i--) {
			if (skip->containsObject(_updateFiles->objectAtIndex(i)))
				_updateFiles->removeObjectAtIndex(i);
		}
	}
	delegate->updateWillStartWithFilesCount(_updateFiles ? _updateFiles->count() : 0);
	_downloadNextUpdateFile();
	return true;
}

// Asks the server which updates there are.
void DownloadManager::refreshDatabase()
{
	if (_refreshConnection)
		return;
	LString *code = _registrationCode();
	if (!_registrationIsValid() || code == 0)
		code = LString::stringWithCString("");
	LString *url = LString::stringWithFormat("%@%s", _hostName(), "/wa2k/nph-refresh2");
	_refreshConnection = LConnection::_alloc()->initWithURLMethodInfo(url, LString::stringWithCString("POST"), 0);
	LF_CHECK_PTR(_refreshConnection);
	LAutorelease(_refreshConnection);
	int version = DynamicEnvironment::instance()->currentVersion();
	char body[1000];
	snprintf(body, sizeof body, "sid=%s&cfb=%d&dll=%d", code->cString(), _buildNumber(), version);
	_refreshConnection->setBody(LData::dataWithBytes(body, strlen(body)));
	_refreshConnection->setUserNameAndPasswd(&sUserName, &sUserPassword, 0);
	LNotificationCenter::center()->addObserver(this, "_refreshRequestCompleted",
	                                           LConnection::connectionClosedNotification, _refreshConnection);
	_refreshConnection->fetch();
}

LConnection *DownloadManager::_prepareActivationRequest(LString *serial)
{
	LString *url = LString::stringWithFormat("%@%s", _hostName(), "/wa2k/nph-activate2");
	LConnection *connection = LConnection::_alloc()->initWithURLMethodInfo(url, LString::stringWithCString("POST"), 0);
	if (connection == 0)
		return 0;
	LAutorelease(connection);
	LString *ownKey = SHIFER_ownKey();
	const char *serialChars = serial ? serial->UTF8String() : "";
	int version = DynamicEnvironment::instance()->currentVersion();
	char body[1008];
	snprintf(body, sizeof body, "ownKey=%s&cfb=%d&dll=%d&prod=%d&serial=%s", ownKey->cString(), _buildNumber(),
	         version, kProductID, serialChars);
	connection->setBody(LData::dataWithBytes(body, strlen(body)));
	return connection;
}

void DownloadManager::postActivationTask(LString *serial)
{
	if (!LConnection::isOnline()) {
		invokeSelector("_activateRequestCompleted", 0);
		return;
	}
	_activationConnection = _prepareActivationRequest(serial);
	if (_activationConnection == 0)
		return;
	_activationConnection->setUserNameAndPasswd(&sUserName, &sUserPassword, 0);
	LNotificationCenter::center()->addObserver(this, "_activateRequestCompleted",
	                                           LConnection::connectionClosedNotification, _activationConnection);
	_activationConnection->fetch();
}

LString *DownloadManager::updatesDownloadFolder()
{
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	LString *root = callbacks ? callbacks->_applicationDataPath(1) : 0;
	if (root == 0)
		return 0;
	LString *folder = root->stringByAppendingPathComponent(
	    LString::stringFromTable("UpdatesFolderName", "PathComponents", getAppModelModule()));
	LFileManager *fm = LFileManager::manager();
	if (fm->objectExistsAtPath(folder, 0))
		return folder;
	fm->createFolderAtPath(folder);
	fm->changeFileAttributesAtPath(
	    folder, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0777), LFileManager::fileAttrPosixPermissions, (LObject *)0));
	return folder;
}

void DownloadManager::setTaskQueueSortCallback(CArrayComparator compare)
{
	if (_tasks == 0)
		_tasks = LArray::_alloc()->init();
	_tasks->setCompareFunc(compare);
	_tasks->setKeepsSorted(compare != 0);
	if (compare)
		_tasks->sort();
}

// The answer to a refresh: the names of the update files, separated by ";",
// "descriptions" first when the server has new descriptions of them. Files
// already downloaded or applied are left out.
void DownloadManager::_processUpdatesFromData(LData *data)
{
	LString *text = LString::stringWithCString((const char *)data->bytes(), data->length())->lowercaseString();
	if (text == 0)
		return;
	LArray *names = text->componentsSeparatedByString(LString::stringWithCString(";"));
	int count = names->count();
	if (count == 0)
		return;
	bool hasDescriptions = names->objectAtIndex(0)->isEqualTo(&kDescriptionsFileName);
	if (_updateFiles) {
		_updateFiles->release();
		_updateFiles = 0;
	}
	LFileManager *fm = LFileManager::manager();
	LString *folder = updatesDownloadFolder();
	for (int i = hasDescriptions ? 1 : 0; i < count; i++) {
		LString *name = (LString *)names->objectAtIndex(i);
		LString *fileName = LString::stringWithCString("upd");
		fileName->appendString(name);
		if (fm->isReadableFileAtPath(folder->stringByAppendingPathComponent(fileName)))
			continue;
		LString *request = SQL_DB_Strings::selectFilenameFromUpdatesThatIsLike(fileName);
		if (AtlasDatabase::instance()->simpleSearchWithRequest(request)->count() != 0)
			continue;
		if (_updateFiles == 0)
			_updateFiles = LArray::_alloc()->init();
		_updateFiles->addObject(name);
	}
	if (!updatesAreAvailable())
		return;
	if (hasDescriptions) {
		LConnection *connection = _prepareGetObjectRequest(&kDescriptionsFileName);
		if (connection)
			connection->fetch();
	} else {
		LNotificationCenter::center()->postNotification(kNewUpdatesAreAvailableNotification, this, 0);
	}
}

// A new registration code: "WA", the product, the MAC address and a random
// mask, each digit written as 0-9A-F or, at random, G-V; then a checksum.
static void makeRegNo(const unsigned char *mac, unsigned long product, unsigned char *out)
{
	time_t now;
	srand((unsigned int)time(&now));
	unsigned int mask = rand();
	unsigned int sum = 0;
	auto put = [&](unsigned int digit) {
		unsigned int c;
		if (rand() & 1)
			c = digit + 'G';
		else {
			c = digit + '0';
			if (c > '9')
				c = digit + '7';
		}
		*out++ = (unsigned char)c;
		sum += c;
	};
	unsigned int v = ((unsigned int)(product + 0x13) ^ mask) << 24;
	for (int i = 0; i < 2; i++, v <<= 4)
		put(v >> 28);
	v = ((unsigned int)(((mac[0] << 8) | mac[1]) ^ (unsigned short)mask) << 16) ^ mask;
	for (int i = 0; i < 4; i++, v <<= 4)
		put(v >> 28);
	v = (((unsigned int)mac[2] << 24) | ((unsigned int)mac[3] << 16) | ((unsigned int)mac[4] << 8) | mac[5]) ^ mask;
	for (int i = 0; i < 8; i++, v <<= 4)
		put(v >> 28);
	for (int i = 0; i < 8; i++, mask <<= 4)
		put(mask >> 28);
	unsigned int high = (sum & 0xff) >> 4, low = sum & 0xf;
	out[0] = (rand() & 1) ? high + 'G' : (high + '0' > '9' ? high + '7' : high + '0');
	out[1] = (rand() & 1) ? low + 'G' : (low + '0' > '9' ? low + '7' : low + '0');
	out[2] = 0;
}

LConnection *DownloadManager::_prepareRegistrationRequest()
{
	LString *userInfo = LString::stringWithCString("");
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	LMap *registration = callbacks ? callbacks->_registrationInfo() : 0;
	if (registration) {
		userInfo = LString::stringWithFormat(
		    "&fname=%@&lname=%@&email=%@&country=%@", registration->objectForKey(IModelCallbacks::kFirstNameKey),
		    registration->objectForKey(IModelCallbacks::kLastNameKey),
		    registration->objectForKey(IModelCallbacks::kEmailKey),
		    registration->objectForKey(IModelCallbacks::kCountryKey));
	}
	LString *url = LString::stringWithFormat("%@%s", _hostName(), "/wa2k/nph-register2");
	LConnection *connection = LConnection::_alloc()->initWithURLMethodInfo(url, LString::stringWithCString("POST"), 0);
	if (connection == 0)
		return 0;
	LAutorelease(connection);
	unsigned char mac[14];
	if (GetMACAddress(mac) != 0)
		return 0;
	unsigned char code[32];
	makeRegNo(mac, kProductID, code);
	const char *info = userInfo ? userInfo->UTF8String() : 0;
	if (info == 0)
		info = "";
	int version = DynamicEnvironment::instance()->currentVersion();
	char body[10000];
	snprintf(body, sizeof body, "sid=%s&cfb=%d&dll=%d%s", code, _buildNumber(), version, info);
	connection->setBody(LData::dataWithBytes(body, strlen(body)));
	return connection;
}

// Identifies with the registration code, or registers to get one.
void DownloadManager::_startAuthorization()
{
	LLog("Start authorization...\n");
	_offline = false;
	if (!LConnection::isOnline()) {
		_authorized = false;
		_offline = true;
		invokeSelector("_authRequestCompleted", 0);
		LLog("Internet not available!\n");
		return;
	}
	_authorizing = true;
	LString *code = _registrationIsValid() ? _registrationCode() : 0;
	_authConnection = 0;
	if (code == 0) {
		_authConnection = _prepareRegistrationRequest();
		if (_authConnection == 0) {
			_authorizing = false;
			_authorizationDone = true;
			_authorized = false;
			_offline = true;
			LLog("Registration request was not created!\n");
			return;
		}
	} else {
		LString *url = LString::stringWithFormat("%@%s", _hostName(), "/wa2k/nph-identify2");
		_authConnection = LConnection::_alloc()->initWithURLMethodInfo(url, LString::stringWithCString("POST"), 0);
		LAutorelease(_authConnection);
		int version = DynamicEnvironment::instance()->currentVersion();
		char body[1000];
		snprintf(body, sizeof body, "sid=%s&cfb=%d&dll=%d", code->cString(), _buildNumber(), version);
		_authConnection->setBody(LData::dataWithBytes(body, strlen(body)));
	}
	LF_CHECK_PTR(_authConnection);
	_authConnection->setUserNameAndPasswd(&sUserName, &sUserPassword, 0);
	LNotificationCenter::center()->addObserver(this, "_authRequestCompleted",
	                                           LConnection::connectionClosedNotification, _authConnection);
	_authConnection->fetch();
}

// Starts as many queued tasks as there is room for (urgent ones anyway). Tasks
// of the services fetched by OnlineServices go without the authorization;
// the others wait for it.
void DownloadManager::_openNextConnection()
{
	if (_paused)
		return;
	bool authorized = _authorizationDone && _authorized;
	int queued = _tasks ? _tasks->count() : 0;
	int running = _connections ? _connections->count() : 0;
	int room = queued;
	if (_maxConnections - running < queued)
		room = _maxConnections - running;
	bool waiting = false;
	for (int i = 0; _tasks && i < (int)_tasks->count();) {
		LMap *task = (LMap *)_tasks->objectAtIndex(i);
		LMap *params = (LMap *)task->objectForKey(&kTaskParametersDictionary);
		bool urgent = UMapParsingUtils::loadIntFromObject(task->objectForKey(&kTaskParameterIsUrgent)) != 0;
		LString *method = lobj_cast<LString>(params->objectForKey(kTaskParameterMethod));
		LString *url = lobj_cast<LString>(params->objectForKey(kTaskParameterRequestString));
		bool service = url && OnlineServices::handlesURL(url);
		if ((!service && !authorized) || (room <= 0 && !urgent)) {
			waiting = waiting || !service;
			i++;
			continue;
		}
		room--;
		LAutorelease(LRetain(task));
		_tasks->removeObjectAtIndex(i);
		LConnection *connection = LConnection::_alloc()->initWithURLMethodInfo(url, method, task);
		if (connection == 0) {
			_taskFinishedWithInfo(task, 0, 400);
			continue;
		}
		LAutorelease(connection);
		LString *user = lobj_cast<LString>(params->objectForKey(kTaskParameterUsername));
		LString *password = lobj_cast<LString>(params->objectForKey(kTaskParameterPassword));
		if (user && password)
			connection->setUserNameAndPasswd(user, password, 0);
		if (_connections == 0)
			_connections = LArray::_alloc()->init();
		_connections->addObject(connection);
		LNotificationCenter::center()->addObserver(this, "_connectionClosed",
		                                           LConnection::connectionClosedNotification, connection);
		if (service)
			OnlineServices::fetch(url, connection);
		else
			connection->fetch();
	}
	if (waiting && !authorized && !_authorizing)
		_startAuthorization();
}

// The callback says which tasks to drop, running or queued.
void DownloadManager::enumerateTasksWithCallback(TASK_ACTION (*callback)(LMap *params, void *context), void *context)
{
	bool removed = false;
	int count = _connections ? _connections->count() : 0;
	for (int i = 0; i < count; i++) {
		LConnection *connection = (LConnection *)_connections->objectAtIndex(i);
		LMap *task = lobj_cast<LMap>(connection->userInfo());
		LMap *params = task ? (LMap *)task->objectForKey(&kTaskParametersDictionary) : 0;
		if (callback(params, context) != kTaskActionRemove)
			continue;
		LNotificationCenter::center()->removeObserver(this, LConnection::connectionClosedNotification, connection);
		connection->terminate();
		_connections->removeObjectAtIndex(i);
		i--;
		count--;
		removed = true;
	}
	count = _tasks ? _tasks->count() : 0;
	for (int i = 0; i < count; i++) {
		LMap *task = (LMap *)_tasks->objectAtIndex(i);
		LMap *params = task ? (LMap *)task->objectForKey(&kTaskParametersDictionary) : 0;
		if (callback(params, context) != kTaskActionRemove)
			continue;
		_tasks->removeObjectAtIndex(i);
		i--;
		count--;
		removed = true;
	}
	if (removed && (_connections == 0 || (int)_connections->count() < _maxConnections) && _tasks &&
	    _tasks->count() != 0 && _authorizationDone)
		_openNextConnection();
}

// Cloud maps go after the reports, then the older tasks first.
int _compareWithOtherTaskInfo(const void *a, const void *b, const void *)
{
	LMap *taskA = *(LMap **)a;
	LMap *taskB = *(LMap **)b;
	LMap *paramsA = lobj_cast<LMap>(taskA->objectForKey(&kTaskParametersDictionary));
	LMap *paramsB = lobj_cast<LMap>(taskB->objectForKey(&kTaskParametersDictionary));
	LString *kindA = paramsA ? lobj_cast<LString>(paramsA->objectForKey(CWManager::kRequestKind)) : 0;
	LString *kindB = paramsB ? lobj_cast<LString>(paramsB->objectForKey(CWManager::kRequestKind)) : 0;
	bool mapA = kindA && kindA->isEqualTo(CWManager::kRequestKindMAP);
	bool mapB = kindB && kindB->isEqualTo(CWManager::kRequestKindMAP);
	if (mapA != mapB)
		return mapA ? 1 : -1;
	LValue *numberA = taskA ? lobj_cast<LValue>(taskA->objectForKey(&kTaskNumber)) : 0;
	LValue *numberB = taskB ? lobj_cast<LValue>(taskB->objectForKey(&kTaskNumber)) : 0;
	if (numberA == 0 || numberB == 0)
		return 0;
	return numberA->intValue() - numberB->intValue();
}

void DownloadManager::postDownloadTaskWithParameters(LMap *params, IDownloadDelegate *delegate, unsigned char urgent)
{
	static int number = 0;
	if (!_sortInstalled) {
		_sortInstalled = true;
		setTaskQueueSortCallback(_compareWithOtherTaskInfo);
	}
	if (_tasks == 0)
		_tasks = LArray::_alloc()->init();
	LObject *owner = delegate ? dynamic_cast<LObject *>(delegate) : 0;
	LMap *task = LMap::mapWithObjectsAndKeys(params, &kTaskParametersDictionary, LValue::valueWithInt(number++),
	                                         &kTaskNumber, LValue::valueWithBoolean(urgent), &kTaskParameterIsUrgent,
	                                         owner, &kTaskDelegate, 0);
	_tasks->addObject(task);
	if (_connections && (int)_connections->count() >= _maxConnections)
		return;
	_openNextConnection();
}

void DownloadManager::setMaxSimultaneousDownloads(int count)
{
	int old = _maxConnections;
	_maxConnections = count;
	if (old < count)
		_openNextConnection();
}

void DownloadManager::resumeQueue()
{
	_paused = false;
	_openNextConnection();
}

// The server sent a new URL creator library.
intptr_t DownloadManager::_dylibLoadCompleted(char *args)
{
	LNotification *ntf = LF_ARG(args, 0, LNotification *);
	LConnection *connection = lobj_cast<LConnection>(ntf->object());
	_refreshConnection = 0;
	LNotificationCenter::center()->removeObserver(this, LConnection::connectionClosedNotification, connection);
	int status = UMapParsingUtils::loadIntFromObject(ntf->userInfo()->objectForKey(LConnection::connectionLastErrorKey));
	LData *data = (LData *)ntf->userInfo()->objectForKey(LConnection::connectionDataKey);
	_authorizing = false;
	if (status == 200) {
		_authorized = true;
		_authorizationDone = true;
		data->expand();
		DynamicEnvironment::instance()->replaceDYLIBWithNew(data);
	}
	_openNextConnection();
	return 0;
}

// The actions are maps of a four-letter type and a value. Returns false when
// one of them stops the processing.
bool DownloadManager::_processActions(LArray *actions)
{
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	int count = actions->count();
	for (int i = 0; i < count; i++) {
		LMap *action = lobj_cast<LMap>(actions->objectAtIndex(i));
		LString *type = lobj_cast<LString>(action->objectForKey(&_typeKey));
		const char *chars = type->cString();
		unsigned int code = ((unsigned char)chars[0] << 24) | ((unsigned char)chars[1] << 16) |
		                    ((unsigned char)chars[2] << 8) | (unsigned char)chars[3];
		switch (code) {
		case 'quit': {
			int value = UMapParsingUtils::loadIntFromObject(action->objectForKey(&_valKey));
			if (value == 0) {
				if (callbacks)
					callbacks->_applicationTerminate(0);
				return false;
			}
			if (value == 1) {
				if (callbacks)
					callbacks->_applicationRestart();
				return false;
			}
			break;
		}
		case 'invl':
			_authorizing = false;
			_authorized = false;
			_authorizationInvalid = true;
			_authorizationDone = true;
			_clearTasksQueue(401);
			break;
		case 'alrt':
			if (!_runAlertWithRep(action))
				return false;
			break;
		case 'brek':
			return false;
		case 'ldll': {
			LString *url = lobj_cast<LString>(action->objectForKey(&_valKey));
			LConnection *connection =
			    LConnection::_alloc()->initWithURLMethodInfo(url, LString::stringWithCString("GET"), 0);
			LNotificationCenter::center()->addObserver(this, "_dylibLoadCompleted",
			                                           LConnection::connectionClosedNotification, connection);
			connection->fetch();
			connection->release();
			break;
		}
		case 'ourl': {
			LString *url = lobj_cast<LString>(action->objectForKey(&_valKey));
			if (url && callbacks)
				callbacks->_openURL(url);
			break;
		}
		case 'reld':
			_authorizing = false;
			_openNextConnection();
			break;
		case 'ract':
			if (callbacks == 0 || !callbacks->_showSerialNumberDialog())
				return false;
			postActivationTask(callbacks ? callbacks->_serialNumber() : 0);
			break;
		case 'regs':
			LLog("show subscription dialog\n");
			if (callbacks == 0 || !callbacks->_showSubscriptionDialog()) {
				LLog("subscription is canceled\n");
				return false;
			}
			_showSubscription = true;
			break;
		case 'spak':
			Preferences::setPublicActivationKey(lobj_cast<LString>(action->objectForKey(&_valKey)));
			LNotificationCenter::center()->postNotification(kActivationCompleteNotification, this, 0);
			break;
		case 'ssid': {
			LString *key = lobj_cast<LString>(action->objectForKey(&_valKey));
			LLog("accept registration key: %@\n", key);
			LUserDefaults::standardUserDefaults()->setObjectForKey(key, &kRegistrationKey);
			LUserDefaults::standardUserDefaults()->synchronize();
			break;
		}
		}
	}
	return true;
}

intptr_t DownloadManager::_getObjectRequestCompleted(char *args)
{
	LNotification *ntf = LF_ARG(args, 0, LNotification *);
	LConnection *connection = lobj_cast<LConnection>(ntf->object());
	LString *name = lobj_cast<LString>(connection->userInfo());
	if (name)
		LAutorelease(LRetain(name));
	_getObjectConnection = 0;
	LNotificationCenter::center()->removeObserver(this, LConnection::connectionClosedNotification, connection);
	int status = UMapParsingUtils::loadIntFromObject(ntf->userInfo()->objectForKey(LConnection::connectionLastErrorKey));
	LData *data = (LData *)ntf->userInfo()->objectForKey(LConnection::connectionDataKey);
	if (status == 200) {
		if (name && name->isEqualTo(&kDescriptionsFileName)) {
			data->expand();
			LMap *descriptions = LMap::mapWithTextRepresentation(data);
			LNotificationCenter::center()->postNotification(kNewUpdatesAreAvailableNotification, this, descriptions);
			return 0;
		}
		LString *fileName = LString::stringWithCString("upd");
		fileName->appendString(name);
		LString *path = updatesDownloadFolder()->stringByAppendingPathComponent(fileName);
		int action;
		do {
			int result = -50;
			if (data->writeToFile(path)) {
				LFileManager::manager()->changeFileAttributesAtPath(
				    path, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0664),
				                                      LFileManager::fileAttrPosixPermissions, 0));
				result = 200;
			}
			action = _updateDelegate ? _updateDelegate->fileWasDownloadedWithName(name, result) : kUpdateActionRetry;
		} while (action == kUpdateActionRetry);
		if (action == kUpdateActionNext) {
			_downloadNextUpdateFile();
			return 0;
		}
		if (action != kUpdateActionStop)
			return 0;
	} else {
		if (status == kStatusActions) {
			LString *text = LString::stringWithCString((const char *)data->bytes(), data->length());
			LArray *actions = lobj_cast<LArray>(text->propertyList());
			if (actions)
				_processActions(actions);
			return 0;
		}
		if ((unsigned int)(status - 200) < 100)
			return 0;
		if (name && name->isEqualTo(&kDescriptionsFileName))
			return 0;
		if (_updateDelegate == 0)
			return 0;
		int action = _updateDelegate->errorWithCodeCatched(status, data);
		if (action != kUpdateActionStop) {
			if (action != kUpdateActionRetry)
				return 0;
			LConnection *retry = _prepareGetObjectRequest(name);
			if (_updateDelegate->fileWillBeDownloadedWithName(name, retry) && retry)
				retry->fetch();
			return 0;
		}
		if (_updateDelegate == 0)
			return 0;
	}
	terminateUpdating();
	return 0;
}

// 201 with the list of updates, 250 with actions; other errors retry a few times.
intptr_t DownloadManager::_refreshRequestCompleted(char *args)
{
	LNotification *ntf = LF_ARG(args, 0, LNotification *);
	LConnection *connection = lobj_cast<LConnection>(ntf->object());
	_refreshConnection = 0;
	LNotificationCenter::center()->removeObserver(this, LConnection::connectionClosedNotification, connection);
	int status = UMapParsingUtils::loadIntFromObject(ntf->userInfo()->objectForKey(LConnection::connectionLastErrorKey));
	LData *data = (LData *)ntf->userInfo()->objectForKey(LConnection::connectionDataKey);
	if (status == 201) {
		if (data && data->length() != 0)
			_processUpdatesFromData(data);
	} else if (status == kStatusActions) {
		if (Preferences::usesSoftwareUpdating()) {
			LString *text = LString::stringWithCString((const char *)data->bytes(), data->length());
			LArray *actions = lobj_cast<LArray>(text->propertyList());
			if (actions)
				_processActions(actions);
		}
	} else if ((unsigned int)(status - 200) >= 100) {
		if (++_refreshAttempts < 10)
			refreshDatabase();
	}
	return 0;
}

// An alert with up to three buttons, each with the actions to perform.
bool DownloadManager::_runAlertWithRep(LMap *rep)
{
	if (rep == 0)
		return false;
	LMap *button1 = lobj_cast<LMap>(rep->objectForKey(LString::stringWithCString("button1")));
	LMap *button2 = lobj_cast<LMap>(rep->objectForKey(LString::stringWithCString("button2")));
	LMap *button3 = lobj_cast<LMap>(rep->objectForKey(LString::stringWithCString("button3")));
	LString *text = LString::stringWithCString("text");
	LString *title3 = button3 ? lobj_cast<LString>(button3->objectForKey(text)) : 0;
	LString *title2 = button2 ? lobj_cast<LString>(button2->objectForKey(text)) : 0;
	LString *title1 = button1 ? lobj_cast<LString>(button1->objectForKey(text)) : 0;
	LString *message = lobj_cast<LString>(rep->objectForKey(LString::stringWithCString("text2")));
	LString *title = lobj_cast<LString>(rep->objectForKey(LString::stringWithCString("text1")));
	int result = IAlertCallback::showAlert(title, message, title1, title2, title3, (IAlertCallback::TAlertResult)0, 0, 0);
	LMap *chosen;
	if (result == 0)
		chosen = button1;
	else if (result == 1)
		chosen = button2;
	else if (result == 2)
		chosen = button3;
	else
		return true;
	// The original only looks at the first button's presence.
	if (button1 == 0 || chosen == 0)
		return true;
	return _processActions(lobj_cast<LArray>(chosen->objectForKey(LString::stringWithCString("actions"))));
}

intptr_t DownloadManager::_activateRequestCompleted(char *args)
{
	LNotification *ntf = args ? LF_ARG(args, 0, LNotification *) : 0;
	LConnection *connection = ntf ? lobj_cast<LConnection>(ntf->object()) : 0;
	IModelCallbacks *callbacks = IModelCallbacks::_callbacks;
	_activationConnection = 0;
	if (connection)
		LNotificationCenter::center()->removeObserver(this, LConnection::connectionClosedNotification, connection);
	if (ntf) {
		int status =
		    UMapParsingUtils::loadIntFromObject(ntf->userInfo()->objectForKey(LConnection::connectionLastErrorKey));
		LData *data = (LData *)ntf->userInfo()->objectForKey(LConnection::connectionDataKey);
		if (status == kStatusActions) {
			LString *text = LString::stringWithCString((const char *)data->bytes(), data->length());
			LArray *actions = lobj_cast<LArray>(text->propertyList());
			if (actions) {
				if (_processActions(actions))
					return 0;
				if (callbacks)
					callbacks->_applicationTerminate(0);
				return 0;
			}
		}
	}
	if (IAlertCallback::showAlertWithCode(0x1000001) == 0 && callbacks && callbacks->_showSerialNumberDialog()) {
		postActivationTask(callbacks->_serialNumber());
		return 0;
	}
	if (callbacks)
		callbacks->_applicationTerminate(0);
	return 0;
}

intptr_t DownloadManager::_authRequestCompleted(char *args)
{
	LNotification *ntf = args ? LF_ARG(args, 0, LNotification *) : 0;
	LConnection *connection = ntf ? lobj_cast<LConnection>(ntf->object()) : 0;
	_authConnection = 0;
	if (connection)
		LNotificationCenter::center()->removeObserver(this, LConnection::connectionClosedNotification, connection);
	LData *data = 0;
	bool askedSubscription = _showSubscription;
	_showSubscription = false;
	int status = 0;
	if (ntf) {
		status = UMapParsingUtils::loadIntFromObject(ntf->userInfo()->objectForKey(LConnection::connectionLastErrorKey));
		data = (LData *)ntf->userInfo()->objectForKey(LConnection::connectionDataKey);
	}
	LLog("authorization completed with code = %d\n", status);
	if (status == 200) {
		_authorized = true;
		_authorizing = false;
		_authorizationDone = true;
	} else {
		if (status == kStatusActions) {
			LString *text = LString::stringWithCString((const char *)data->bytes(), data->length());
			LArray *actions = lobj_cast<LArray>(text->propertyList());
			if (actions && _processActions(actions))
				return 0;
		}
		_authorizing = false;
		_authorizationDone = true;
		if (!askedSubscription || IAlertCallback::showAlertWithCode(0x1000000) != 0) {
			_clearTasksQueue(status);
			return 0;
		}
		_showSubscription = true;
	}
	_openNextConnection();
	return 0;
}

intptr_t DownloadManager::_connectionClosed(char *args)
{
	LNotification *ntf = LF_ARG(args, 0, LNotification *);
	LConnection *connection = lobj_cast<LConnection>(ntf->object());
	LNotificationCenter::center()->removeObserver(this, LConnection::connectionClosedNotification, connection);
	if (_connections == 0)
		return -1;
	int index = _connections->indexOfObject(connection);
	if (index == -1)
		return -1;
	LRetain(connection);
	_connections->removeObjectAtIndex(index);
	LMap *task = (LMap *)connection->userInfo();
	int status = UMapParsingUtils::loadIntFromObject(ntf->userInfo()->objectForKey(LConnection::connectionLastErrorKey));
	LData *data = (LData *)ntf->userInfo()->objectForKey(LConnection::connectionDataKey);
	if (status != 200 || data == 0)
		data = 0;
	_taskFinishedWithInfo(task, data, status);
	connection->release();
	if ((int)_connections->count() < _maxConnections)
		_openNextConnection();
	return 0;
}
