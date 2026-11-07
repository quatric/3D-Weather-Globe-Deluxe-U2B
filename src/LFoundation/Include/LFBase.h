// LFoundation basics: exceptions, locks, logging.
#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <exception>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

namespace LF {

class YAException : public std::exception {
public:
	YAException(const char *const &) {}
	YAException(const YAException &) {}
	virtual ~YAException() {}
	// logs the exception with the source position, then throws a copy
	void raise(const char *file, int line);
	virtual void _Doraise() const = 0;
};

class NilPtrException : public YAException {
public:
	NilPtrException() : YAException("NULL pointer exception") {}
	NilPtrException(const NilPtrException &e) : YAException(e) {}
	virtual ~NilPtrException() {}
	virtual void _Doraise() const;
};

class WrongConditionException : public YAException {
public:
	WrongConditionException() : YAException("wrong condition exception") {}
	WrongConditionException(const WrongConditionException &e) : YAException(e) {}
	virtual ~WrongConditionException() {}
	virtual void _Doraise() const;
};

class FailedWithCodeException : public YAException {
public:
	FailedWithCodeException(int code) : YAException("operation failed"), _code(code) {}
	FailedWithCodeException(const FailedWithCodeException &e) : YAException(e), _code(e._code) {}
	virtual ~FailedWithCodeException() {}
	virtual const char *what() const noexcept;
	virtual void _Doraise() const;
	int _code;
};

class TextStreamException : public YAException {
public:
	TextStreamException(const char *msg, int pos) : YAException(msg), _pos(pos) {}
	TextStreamException(const TextStreamException &e) : YAException(e), _pos(e._pos) {}
	virtual ~TextStreamException() {}
	virtual void _Doraise() const;
	int _pos;
};

// The original's checks: raise the exception, tagged with the position.
#define LF_RAISE(e) (e).raise(__FILE__, __LINE__)
#define LF_CHECK_PTR(p) do { if ((p) == 0) LF_RAISE(LF::NilPtrException()); } while (0)
#define LF_CHECK(c) do { if (!(c)) LF_RAISE(LF::WrongConditionException()); } while (0)
#define LF_CHECK_CODE(err) do { int _e = (err); if (_e != 0) LF_RAISE(LF::FailedWithCodeException(_e)); } while (0)

// A recursive mutex that remembers its owner thread.
class CSharableResource {
public:
	CSharableResource();
	virtual ~CSharableResource();
	virtual void lock();
	virtual bool tryLock();
	virtual void unlock();

	int _lockCount;
	pthread_mutex_t _mutex;
	pthread_t _owner;
};

// Holds a CSharableResource locked for its scope.
class StSharableLocker {
public:
	StSharableLocker(CSharableResource *res, bool wait = true);
	~StSharableLocker() { if (_locked && _res) _res->unlock(); }
	bool isLocked() const { return _locked; }

	CSharableResource *_res;
	bool _locked;
};

// LFoundation.framework's bundle (resources such as LCharSet.bitmaps).
CFBundleRef getLFoundationModule();
void setLFoundationModule(CFBundleRef bundle);

// The log file (LFoundation's console), set up by _LogFileInitializer.
extern FILE *sLogFile;
extern CSharableResource sAlertMutex;

extern bool sLFoundationIsActive;

// Sets up LFoundation for the process: the main autorelease pool, the random
// seeds and, if asked, the NSAutoreleasePool bridge. (The leak analyser of
// the original did nothing; its flag is accepted and ignored.)
void initializeLFoundation(unsigned char analyseLeaks, unsigned char installPoolBridge);
// Tears down the shared objects and the main pool.
void finilizeLFoundation();

void LLog(const char *format, ...);
void _setLogFile(FILE *file);
void _exclusiveShowString(const char *msg, const char *file, int line);
void _FullLogString(const char *msg, const char *file, int line);
bool _isInExclusiveAssertMode();

} // namespace LF
