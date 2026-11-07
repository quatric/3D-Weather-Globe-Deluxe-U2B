// LSocket
//
// LF::LSocket and LF::LSocketThread.

#include "LSocket.h"
#include "LValue.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace LF {

LF_IMPLEMENT_CLASS(LSocket, 0)
LF_IMPLEMENT_CLASS(LSocketThread, LF_CLASSOF(LThread))

CSharableResource LSocketThread::_sSocketMutex;

// The original autoreleased the object even when its init had deallocated it.
LSocket *LSocket::socketWithNative(int fd)
{
	LSocket *socket = _alloc();
	if (socket && socket->intitWithNative(fd))
		return LAutorelease(socket);
	return 0;
}

LSocket *LSocket::socketWithDomainType(int domain, int type)
{
	LSocket *socket = _alloc();
	if (socket && socket->initWithDomainType(domain, type))
		return LAutorelease(socket);
	return 0;
}

LSocket *LSocket::intitWithNative(int fd)
{
	_socket = fd;
	if (fd >= 0 && fcntl(fd, F_SETFL, O_NONBLOCK) >= 0)
		return this;
	dealloc();
	return 0;
}

LSocket *LSocket::initWithDomainType(int domain, int type)
{
	_socket = socket(domain, type, IPPROTO_TCP);
	if (_socket >= 0 && fcntl(_socket, F_SETFL, O_NONBLOCK) >= 0)
		return this;
	dealloc();
	return 0;
}

static void _fillAddress(sockaddr_in *addr, LString *address, int port)
{
	memset(addr, 0, sizeof *addr);
	if (address)
		addr->sin_addr.s_addr = inet_addr(address->cString());
	addr->sin_port = htons((uint16_t)port);
	addr->sin_len = sizeof *addr;
	addr->sin_family = AF_INET;
}

int LSocket::connectToAddressPort(LString *address, int port)
{
	sockaddr_in addr;
	_fillAddress(&addr, address, port);
	if (connect(_socket, (sockaddr *)&addr, sizeof addr) < 0)
		return errno;
	return 0;
}

// Tells the running thread to stop.
void LSocket::close()
{
	if (LSocketThread *thread = _thread) {
		thread->retain();
		_thread->_stop = true;
		thread->release();
	}
	_thread = 0;
	::close(_socket);
}

void LSocket::dealloc()
{
	close();
	LObject::dealloc();
}

int LSocket::beginListenAddressPort(LString *address, int port, LObject *delegate, const char *sel)
{
	if (_thread)
		return -103;
	_thread = LSocketThread::_alloc()->initListenThreadWithSocket(this, address, port, delegate, sel);
	_thread->_executeThread();
	_thread->release();
	return 0;
}

void LSocket::beginReadDataWithBytesCount(int count, LObject *delegate, const char *sel)
{
	if (_thread) {
		if (sel && delegate)
			delegate->invokeSelectorInMainThreadWithObjects("didEndSelector", true, false, this,
			                                                LData::dataWithLength(0), LValue::valueWithInt(-103),
			                                                LValue::valueWithInt(0));
		return;
	}
	_thread = LSocketThread::_alloc()->initReadThreadWithSocket(this, delegate, sel, count);
	_thread->_executeThread();
	_thread->release();
}

void LSocket::beginWriteData(LData *data, LObject *delegate, const char *sel)
{
	if (_thread) {
		if (sel && delegate)
			delegate->invokeSelectorInMainThreadWithObjects("didEndSelector", true, false, this,
			                                                LValue::valueWithInt(-103), LValue::valueWithInt(0));
		return;
	}
	_thread = LSocketThread::_alloc()->initWriteThreadWithSocket(this, data, delegate, sel);
	_thread->_executeThread();
	_thread->release();
}

LSocketThread *LSocketThread::initListenThreadWithSocket(LSocket *socket, LString *address, int port,
                                                         LObject *delegate, const char *sel)
{
	if (!LObject::init())
		return 0;
	_socket = LRetain(socket);
	_address = LRetain(address);
	_delegate = LRetain(delegate);
	_stop = false;
	_selector = sel;
	_mode = kListen;
	_port = port;
	return this;
}

LSocketThread *LSocketThread::initReadThreadWithSocket(LSocket *socket, LObject *delegate, const char *sel, int count)
{
	if (!LObject::init())
		return 0;
	_socket = LRetain(socket);
	_delegate = LRetain(delegate);
	_stop = false;
	_selector = sel;
	_mode = kRead;
	_bytesToRead = count;
	return this;
}

LSocketThread *LSocketThread::initWriteThreadWithSocket(LSocket *socket, LData *data, LObject *delegate,
                                                        const char *sel)
{
	if (!LObject::init())
		return 0;
	_socket = LRetain(socket);
	_delegate = LRetain(delegate);
	_stop = false;
	_selector = sel;
	_data = LRetain(data);
	_mode = kWrite;
	return this;
}

void LSocketThread::dealloc()
{
	{
		StSharableLocker locker(&_sSocketMutex);
		if (_socket) {
			_socket->_thread = 0;
			_socket->release();
		}
		if (_delegate)
			_delegate->release();
		if (_data)
			_data->release();
		if (_address)
			_address->release();
	}
	LThread::dealloc();
}

intptr_t LSocketThread::run()
{
	switch (_mode) {
	case kRead:
		return read();
	case kWrite:
		return write();
	case kListen:
		return listen();
	}
	return 0;
}

int LSocketThread::selectSocket(int fd, TSocketSelectMode mode, unsigned char *ready)
{
	fd_set readSet, writeSet;
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	fd_set *result = 0;
	if (mode == kSelectRead) {
		FD_SET(fd, &readSet);
		result = &readSet;
	} else if (mode == kSelectWrite) {
		FD_SET(fd, &writeSet);
		result = &writeSet;
	} else if (mode == kSelectReadWrite) {
		FD_SET(fd, &readSet);
		FD_SET(fd, &writeSet);
	}
	int err = 0;
	do {
		if (select(fd + 1, &readSet, &writeSet, 0, 0) < 0)
			err = errno;
	} while (err == EINTR);
	if (err == 0 && result && ready)
		*ready = FD_ISSET(fd, result) ? 1 : 0;
	return err;
}

// Accepts connections until stopped, handing each to the delegate.
int LSocketThread::listen()
{
	StAutoreleasePool pool;
	int fd;
	{
		StSharableLocker locker(&_sSocketMutex);
		fd = _socket->_socket;
	}
	sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_port = htons((uint16_t)_port);
	addr.sin_len = sizeof addr;
	addr.sin_family = AF_INET;
	if (_address)
		addr.sin_addr.s_addr = inet_addr(_address->cString());
	if (bind(fd, (sockaddr *)&addr, sizeof addr) < 0 && errno != 0)
		return errno;
	if (::listen(fd, 128) < 0 && errno != 0)
		return errno;
	while (!_stop) {
		sockaddr_in peer;
		memset(&peer, 0, sizeof peer);
		socklen_t length = sizeof peer;
		int client = accept(fd, (sockaddr *)&peer, &length);
		if (_delegate && _selector) {
			StSharableLocker locker(&_sSocketMutex);
			_delegate->invokeSelectorInMainThreadWithObjects("_didEndSelector", true, false, _socket,
			                                                 LSocket::socketWithNative(client));
		}
	}
	return 0;
}

// Sends the data; -102 without data, -101 if the peer closed.
int LSocketThread::write()
{
	StAutoreleasePool pool;
	const char *bytes = 0;
	unsigned int length = 0;
	if (_data) {
		length = _data->length();
		bytes = (const char *)_data->bytes();
	}
	unsigned char ready = 0;
	int err = bytes ? 0 : -102;
	int fd;
	{
		StSharableLocker locker(&_sSocketMutex);
		fd = _socket->_socket;
	}
	unsigned int left = length;
	while (left != 0 && err == 0 && fd != -1 && bytes && !_stop) {
		ssize_t n = send(fd, bytes, left, 0);
		if (n > 0) {
			bytes += n;
			left -= n;
		} else if (n == 0)
			err = -101;
		else {
			err = errno;
			if (err == EAGAIN)
				err = selectSocket(fd, kSelectWrite, &ready);
		}
	}
	if (_delegate && _selector) {
		StSharableLocker locker(&_sSocketMutex);
		_delegate->invokeSelectorInMainThreadWithObjects("_didEndSelector", true, false, _socket,
		                                                 LValue::valueWithInt(err),
		                                                 LValue::valueWithInt(length - left));
	}
	return err;
}

// Receives _bytesToRead bytes; -101 if the peer closed, -100 if stopped.
int LSocketThread::read()
{
	StAutoreleasePool pool;
	LData *data = LData::dataWithLength(0);
	unsigned int left = _bytesToRead;
	char *buffer = (char *)malloc(left);
	unsigned char ready = 0;
	int err = 0;
	if (buffer == 0)
		err = errno;
	int fd;
	{
		StSharableLocker locker(&_sSocketMutex);
		fd = _socket->_socket;
	}
	char *at = buffer;
	while (left != 0 && err == 0 && fd != -1 && at && !_stop) {
		ssize_t n = recv(fd, at, left, 0);
		if (n > 0) {
			if (data)
				data->appendBytes(at, (unsigned int)n);
			left -= n;
			at += n;
		} else if (n == 0)
			err = -101;
		else {
			err = errno;
			if (err == EAGAIN)
				err = selectSocket(fd, kSelectRead, &ready);
		}
	}
	unsigned int total = _bytesToRead;
	if (buffer)
		free(buffer);
	int result = _stop ? -100 : err;
	if (_delegate && _selector) {
		StSharableLocker locker(&_sSocketMutex);
		_delegate->invokeSelectorInMainThreadWithObjects("_didEndSelector", true, false, _socket, data,
		                                                 LValue::valueWithInt(result),
		                                                 LValue::valueWithInt(total - left));
	}
	return result;
}

} // namespace LF
