// LSocket: a non-blocking TCP socket. Reading, writing and listening each run
// on an LSocketThread that reports to a delegate on the main thread.
//
// As in the original, the delegate is sent "_didEndSelector" (the literal
// name), not the selector it passed.
#pragma once

#include "LRunLoop.h"
#include "LData.h"

namespace LF {

class LSocketThread;

class LSocket : public LObject {
	LF_DECLARE_CLASS(LSocket)
public:
	LSocket() : _socket(-1), _thread(0) {}
	virtual ~LSocket() {}
	virtual void dealloc();
	virtual LSocket *getOwnerLObject() { return this; }

	static LSocket *socketWithNative(int fd);
	static LSocket *socketWithDomainType(int domain, int type);
	// (The original's spelling.) These deallocate the object and return 0
	// if the socket can't be made non-blocking.
	LSocket *intitWithNative(int fd);
	LSocket *initWithDomainType(int domain, int type);

	int nativeSocket() { return _socket; }
	// An IPv4 address; 0 or an errno.
	int connectToAddressPort(LString *address, int port);
	void close();
	// -103 if a thread is already running.
	int beginListenAddressPort(LString *address, int port, LObject *delegate, const char *sel);
	void beginReadDataWithBytesCount(int count, LObject *delegate, const char *sel);
	void beginWriteData(LData *data, LObject *delegate, const char *sel);

	int _socket;
	LSocketThread *_thread;
};

enum TSocketSelectMode {
	kSelectReadWrite = 0,
	kSelectRead = 1,
	kSelectWrite = 2,
};

class LSocketThread : public LThread {
	LF_DECLARE_CLASS(LSocketThread)
public:
	enum { kRead = 1, kWrite = 2, kListen = 3 };

	LSocketThread()
	    : _socket(0), _delegate(0), _selector(0), _stop(false), _bytesToRead(0), _data(0), _address(0), _port(0),
	      _mode(0)
	{
	}
	virtual ~LSocketThread() {}
	virtual void dealloc();
	virtual intptr_t run();
	virtual LSocketThread *getOwnerLObject() { return this; }
	// 0 or an errno; *ready tells whether `fd` is ready in a read or write
	// selection.
	virtual int selectSocket(int fd, TSocketSelectMode mode, unsigned char *ready);
	virtual int read();
	virtual int write();
	virtual int listen();

	LSocketThread *initListenThreadWithSocket(LSocket *socket, LString *address, int port, LObject *delegate,
	                                          const char *sel);
	LSocketThread *initReadThreadWithSocket(LSocket *socket, LObject *delegate, const char *sel, int count);
	LSocketThread *initWriteThreadWithSocket(LSocket *socket, LData *data, LObject *delegate, const char *sel);
	void startFetch() { _executeThread(); }
	void softTerminate() { _stop = true; }

	static CSharableResource _sSocketMutex;

	LSocket *_socket;
	LObject *_delegate;
	const char *_selector;
	volatile bool _stop;
	int _bytesToRead;
	LData *_data;
	LString *_address;
	int _port;
	int _mode;
};

} // namespace LF
