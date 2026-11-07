// LConnection: one HTTP request read through a CFReadStream on the current
// run loop. Progress and the end are posted as notifications from the
// connection; the end's user info has the status (or -50 for a stream error)
// and the data.
#pragma once

#include "LMap.h"
#include "LData.h"
#include <CFNetwork/CFNetwork.h>

namespace LF {

class LConnection : public LObject {
	LF_DECLARE_CLASS(LConnection)
public:
	// Asked about 401 and 407 responses; returns true to fetch again.
	class ErrorDlgHandler {
	public:
		virtual bool handleConnectionError(LConnection *connection, int status) = 0;
		virtual ~ErrorDlgHandler() {}
	};

	LConnection() : _userInfo(0), _data(0), _request(0), _stream(0), _fetching(false), _triedProxyAuth(false), _contentLength(0) {}
	virtual ~LConnection() {}
	virtual LString *description() const;
	virtual void dealloc();
	virtual LConnection *getOwnerLObject() { return this; }

	static void finalizeConnections() {}
	static void setErrorHandler(ErrorDlgHandler *handler) { sErrorHandler = handler; }
	// Whether 1.1.1.1 is reachable without a connection being made first.
	static bool isOnline();

	// `method` defaults to GET.
	LConnection *initWithURLMethodInfo(LString *url, LString *method, LObject *userInfo);
	LObject *userInfo() const { return _userInfo; }
	void setUserInfo(LObject *userInfo);
	void setBody(LData *body);
	void setHeaders(TLMap<LString *, LString *> *headers);
	void setUserNameAndPasswd(LString *user, LString *password, unsigned char forProxy);
	// The connection keeps itself alive while fetching.
	void fetch();
	void terminate();

	void handleNetworkEvent(CFStreamEventType event);
	static void ReadStreamClientCallBack(CFReadStreamRef stream, CFStreamEventType event, void *info);
	void _closeStream();
	void _handleStreamError();
	bool _setProxyUserNameAndPasswdFromKeyChain();

	static ErrorDlgHandler *sErrorHandler;
	static LString *connectionClosedNotification;
	static LString *connectionDataDownloadedNotification;
	static LString *connectionContentLengthUpdatedNotification;
	static LString *connectionLastErrorKey;
	static LString *connectionDataKey;
	static LString *connectionContentLengthKey;
	static LString *connectionDownloadedContentSizeKey;
	static LConstantString kConnectionClosedNotification;
	static LConstantString kConnectionLastErrorKey;
	static LConstantString kConnectionDataKey;

	LObject *_userInfo;
	LData *_data;
	CFHTTPMessageRef _request;
	CFReadStreamRef _stream;
	bool _fetching;
	bool _triedProxyAuth;
	int _contentLength;	// -1 when the response has none
};

} // namespace LF
