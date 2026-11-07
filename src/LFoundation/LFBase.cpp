// LFBase
//
// LFoundation's log file and LLog, the LFoundation bundle, and the globals
// they use.

#include "LString.h"
#include "LCleanupTask.h"
#include <CoreServices/CoreServices.h>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace LF {

FILE *sLogFile = 0;
CSharableResource sAlertMutex;
bool sLFoundationIsActive = false;
static bool sIsInExclusiveAssertMode = false;
static CFBundleRef sLFoundationBundle = 0;

void _setLogFile(FILE *file)
{
	if (sLogFile) {
		fwrite("END OF LOG\n", 1, 11, sLogFile);
		fclose(sLogFile);
	}
	sLogFile = file;
}

void _exclusiveShowString(const char *msg, const char *file, int line)
{
	if (sLogFile) {
		fprintf(sLogFile, "%s(%d) : %s\n", file, line, msg);
		fflush(sLogFile);
	}
}

void _FullLogString(const char *msg, const char *file, int line)
{
	if (sLogFile) {
		fprintf(sLogFile, "%s(%d) : %s\n", file, line, msg);
		fflush(sLogFile);
	}
}

bool _isInExclusiveAssertMode()
{
	return sIsInExclusiveAssertMode;
}

// "thread=<id> hh:mm:ss.sss " and the formatted message.
void LLog(const char *format, ...)
{
	if (sLogFile == 0)
		return;
	StSharableLocker locker(&sAlertMutex);
	va_list ap;
	va_start(ap, format);
	LConstantString fmt(format);
	LString *msg = LString::alloc_with_new()->_initWithFormat(&fmt, ap);
	va_end(ap);
	if (msg == 0)
		return;
	CFTimeZoneRef zone = CFTimeZoneCopySystem();
	CFGregorianDate date = CFAbsoluteTimeGetGregorianDate(CFAbsoluteTimeGetCurrent(), zone);
	CFRelease(zone);
	char stamp[100];
	snprintf(stamp, sizeof stamp, "thread=%p %02d:%02d:%s%.03f ", (void *)pthread_self(), date.hour, date.minute,
	         date.second < 10.0 ? "0" : "", date.second);
	if (sLogFile) {
		fputs(stamp, sLogFile);
		fflush(sLogFile);
		if (sLogFile) {
			fprintf(sLogFile, "%s\n", msg->cString());
			fflush(sLogFile);
		}
	}
	msg->release();
}

void setLFoundationModule(CFBundleRef bundle)
{
	if (sLFoundationBundle)
		CFRelease(sLFoundationBundle);
	sLFoundationBundle = bundle;
	if (bundle)
		CFRetain(bundle);
}

CFBundleRef getLFoundationModule()
{
	if (sLFoundationBundle == 0)
		setLFoundationModule(CFBundleGetBundleWithIdentifier(CFSTR("com.mackiev.LFoundation")));
	return sLFoundationBundle;
}

} // namespace LF

using namespace LF;

// Opens ~/Library/Logs/<main bundle id>.LFoundation.log, for apps with a
// bundle identifier.
struct _LogFileInitializer {
	_LogFileInitializer()
	{
		CFStringRef ident = CFBundleGetIdentifier(CFBundleGetMainBundle());
		if (ident == 0)
			return;
		FSRef folder;
		if (FSFindFolder(kUserDomain, kLogsFolderType, true, &folder) != noErr)
			return;
		CFURLRef logs = CFURLCreateFromFSRef(kCFAllocatorDefault, &folder);
		if (logs == 0)
			return;
		CFURLRef base = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, logs, ident, false);
		CFRelease(logs);
		if (base == 0)
			return;
		CFURLRef url = CFURLCreateCopyAppendingPathExtension(kCFAllocatorDefault, base, CFSTR("LFoundation.log"));
		CFRelease(base);
		if (url == 0)
			return;
		char path[1024];
		if (CFURLGetFileSystemRepresentation(url, true, (UInt8 *)path, sizeof path))
			_setLogFile(fopen(path, "wb"));
		CFRelease(url);
	}
	~_LogFileInitializer() { _setLogFile(0); }
};

static _LogFileInitializer sLogFileInitializer;

struct _LFoundationBundleCleanup : public LCleanupTask {
	// The original released the bundle without forgetting it.
	virtual void performCleanup() { setLFoundationModule(0); }
};

static _LFoundationBundleCleanup sLFoundationBundleCleanup;

// Debug reports go to stderr.
int _CrtDbgReport(int, const char *file, int line, const char *, const char *format, ...)
{
	if (file)
		fprintf(stderr, "%s(%d) : ", file, line);
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	return 0;
}
