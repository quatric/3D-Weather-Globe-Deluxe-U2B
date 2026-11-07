// LConnection
//
// LF::LConnection. The proxy password comes from the keychain item for the
// proxy host and port (the original used the Keychain Manager's
// KCFindInternetPassword, which has no 64-bit version).

#include "LConnection.h"
#include "LNotification.h"
#include "LValue.h"
#include <Security/Security.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <arpa/inet.h>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace LF {

LF_IMPLEMENT_CLASS(LConnection, 0)

LConnection::ErrorDlgHandler *LConnection::sErrorHandler = 0;

LConstantString LConnection::kConnectionClosedNotification("ConnectionClosedNotification");
LConstantString LConnection::kConnectionLastErrorKey("ConnectionLastErrorKey");
LConstantString LConnection::kConnectionDataKey("ConnectionDataKey");
static LConstantString kConnectionContentLengthUpdatedNotification("ConnectionContentLengthUpdatedNotification");
static LConstantString kConnectionContentLengthKey("ConnectionContentLengthKey");
static LConstantString kConnectionDataDownloadedNotification("connectionDataDownloadedNotification");
static LConstantString kConnectionDownloadedContentSizeKey("ConnectionDownloadedContentSizeKey");

LString *LConnection::connectionClosedNotification = &kConnectionClosedNotification;
LString *LConnection::connectionLastErrorKey = &kConnectionLastErrorKey;
LString *LConnection::connectionDataKey = &kConnectionDataKey;
LString *LConnection::connectionContentLengthUpdatedNotification = &kConnectionContentLengthUpdatedNotification;
LString *LConnection::connectionContentLengthKey = &kConnectionContentLengthKey;
LString *LConnection::connectionDataDownloadedNotification = &kConnectionDataDownloadedNotification;
LString *LConnection::connectionDownloadedContentSizeKey = &kConnectionDownloadedContentSizeKey;

bool LConnection::isOnline()
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_len = sizeof addr;
	addr.sin_family = AF_INET;
	inet_aton("1.1.1.1", &addr.sin_addr);
	SCNetworkReachabilityRef target = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (sockaddr *)&addr);
	SCNetworkReachabilityFlags flags = 0;
	Boolean valid = SCNetworkReachabilityGetFlags(target, &flags);
	CFRelease(target);
	bool online = valid && (flags & kSCNetworkFlagsReachable)
	              && !((flags & kSCNetworkFlagsConnectionRequired) && !(flags & kSCNetworkFlagsConnectionAutomatic));
	LLog("SCNetworkReachabilityGetFlags: flags are valid = %d, flags = %x, isOnline = %d\n", (unsigned)valid, flags,
	     online);
	return online;
}

LConnection *LConnection::initWithURLMethodInfo(LString *url, LString *method, LObject *userInfo)
{
	if (!LObject::init())
		return 0;
	_userInfo = LRetain(userInfo);
	CFURLRef cfURL = CFURLCreateWithString(kCFAllocatorDefault, url ? url->_cf : 0, 0);
	if (cfURL) {
		_request = CFHTTPMessageCreateRequest(kCFAllocatorDefault, method ? method->_cf : CFSTR("GET"), cfURL,
		                                      kCFHTTPVersion1_1);
		CFRelease(cfURL);
	}
	if (_request)
		return this;
	dealloc();
	return 0;
}

void LConnection::setUserInfo(LObject *userInfo)
{
	LRetain(userInfo);
	if (_userInfo) {
		_userInfo->release();
		_userInfo = 0;
	}
	_userInfo = userInfo;
}

void LConnection::setBody(LData *body)
{
	CFDataRef data = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)body->bytes(), body->length());
	CFHTTPMessageSetBody(_request, data);
	CFRelease(data);
}

void LConnection::setHeaders(TLMap<LString *, LString *> *headers)
{
	LMapIterator it(headers, true);
	while (it.next())
		CFHTTPMessageSetHeaderFieldValue(_request, ((const LString *)it.key())->_cf, ((LString *)it.object())->_cf);
}

void LConnection::setUserNameAndPasswd(LString *user, LString *password, unsigned char forProxy)
{
	CFHTTPMessageAddAuthentication(_request, 0, user->_cf, password->_cf, kCFHTTPAuthenticationSchemeBasic, forProxy);
}

void LConnection::_closeStream()
{
	if (_stream) {
		CFReadStreamClose(_stream);
		CFRelease(_stream);
		_stream = 0;
	}
}

void LConnection::terminate()
{
	if (_fetching) {
		_closeStream();
		_fetching = false;
		release();
	}
}

void LConnection::dealloc()
{
	terminate();
	if (_userInfo) {
		_userInfo->release();
		_userInfo = 0;
	}
	if (_data) {
		_data->release();
		_data = 0;
	}
	_closeStream();
	if (_request)
		CFRelease(_request);
	LLog("Connection dealloc\n");
	LObject::dealloc();
}

// Posts the end with error -50 and lets go of the fetch's reference.
void LConnection::_handleStreamError()
{
	_closeStream();
	LMap *info = LMap::mapWithObjectsAndKeys(LValue::valueWithInt(-50), connectionLastErrorKey, (LObject *)0);
	LNotificationCenter::center()->postNotification(connectionClosedNotification, this, info);
	release();
}

void LConnection::fetch()
{
	if (_fetching)
		return;
	retain();
	LLog("Connection fetch...\n");
	_stream = CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, _request);
	if (_stream) {
		SCDynamicStoreRef store = SCDynamicStoreCreate(0, CFSTR("SIMPLE_GET_"), 0, 0);
		CFDictionaryRef proxies = SCDynamicStoreCopyProxies(store);
		CFRelease(store);
		if (proxies) {
			CFReadStreamSetProperty(_stream, kCFStreamPropertyHTTPProxy, proxies);
			CFRelease(proxies);
		}
		CFStreamClientContext context = {0, this, 0, 0, 0};
		if (CFReadStreamSetClient(_stream,
		                          kCFStreamEventOpenCompleted | kCFStreamEventHasBytesAvailable
		                              | kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered,
		                          ReadStreamClientCallBack, &context)) {
			CFReadStreamScheduleWithRunLoop(_stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
			if (CFReadStreamOpen(_stream)) {
				_fetching = true;
				return;
			}
			LLog("Unable to open an HTTP stream\n");
		}
	}
	_handleStreamError();
}

void LConnection::ReadStreamClientCallBack(CFReadStreamRef, CFStreamEventType event, void *info)
{
	((LConnection *)info)->handleNetworkEvent(event);
}

void LConnection::handleNetworkEvent(CFStreamEventType event)
{
	if (event == kCFStreamEventErrorOccurred) {
		_handleStreamError();
		return;
	}
	if (event == kCFStreamEventHasBytesAvailable) {
		if (_contentLength == 0) {
			CFHTTPMessageRef response =
			    (CFHTTPMessageRef)CFReadStreamCopyProperty(_stream, kCFStreamPropertyHTTPResponseHeader);
			if (response) {
				CFStringRef length = CFHTTPMessageCopyHeaderFieldValue(response, CFSTR("Content-Length"));
				if (length == 0)
					_contentLength = -1;
				else {
					_contentLength = CFStringGetIntValue(length);
					LMap *info = LMap::mapWithObjectsAndKeys(LValue::valueWithInt(_contentLength),
					                                         connectionContentLengthKey, (LObject *)0);
					LNotificationCenter::center()->postNotification(connectionContentLengthUpdatedNotification,
					                                                this, info);
					CFRelease(length);
				}
				CFRelease(response);
			}
		}
		UInt8 buffer[2048];
		do {
			CFIndex n = CFReadStreamRead(_stream, buffer, sizeof buffer);
			if (n < 0) {
				_handleStreamError();
				return;
			}
			if (n == 0)
				break;
			if (_data == 0) {
				_data = LData::_alloc();
				_data->setData(buffer, (unsigned int)n);
			} else
				_data->appendBytes(buffer, (unsigned int)n);
		} while (CFReadStreamHasBytesAvailable(_stream));
		if (_data == 0)
			return;
		LMap *info = LMap::mapWithObjectsAndKeys(LValue::valueWithInt(_data->length()),
		                                         connectionDownloadedContentSizeKey, (LObject *)0);
		LNotificationCenter::center()->postNotification(connectionDataDownloadedNotification, this, info);
		return;
	}
	if (event != kCFStreamEventEndEncountered)
		return;
	CFHTTPMessageRef response =
	    (CFHTTPMessageRef)CFReadStreamCopyProperty(_stream, kCFStreamPropertyHTTPResponseHeader);
	int status = (int)CFHTTPMessageGetResponseStatusCode(response);
	bool again = false;
	if (status == 401 || status == 407) {
		if (status == 407 && !_triedProxyAuth && _setProxyUserNameAndPasswdFromKeyChain())
			again = true;
		else if (sErrorHandler)
			again = sErrorHandler->handleConnectionError(this, status);
	}
	CFRelease(response);
	_closeStream();
	if (!again) {
		LMap *info = LMap::mapWithObjectsAndKeys(LValue::valueWithInt(status), connectionLastErrorKey, _data,
		                                         connectionDataKey, (LObject *)0);
		LNotificationCenter::center()->postNotification(connectionClosedNotification, this, info);
	} else {
		LLog("Connection prepare to re-fetch\n");
		if (_data) {
			_data->release();
			_data = 0;
		}
		_fetching = false;
		fetch();
	}
	release();
}

// Basic proxy authentication with the keychain's password for the system's
// HTTP proxy.
bool LConnection::_setProxyUserNameAndPasswdFromKeyChain()
{
	SCDynamicStoreRef store = SCDynamicStoreCreate(0, CFSTR("SIMPLE_GET_"), 0, 0);
	CFDictionaryRef proxies = SCDynamicStoreCopyProxies(store);
	CFRelease(store);
	if (proxies == 0)
		return false;
	CFStringRef host = (CFStringRef)CFDictionaryGetValue(proxies, kSCPropNetProxiesHTTPProxy);
	CFNumberRef portNumber = (CFNumberRef)CFDictionaryGetValue(proxies, kSCPropNetProxiesHTTPPort);
	if (host == 0 || portNumber == 0) {
		CFRelease(proxies);
		return false;
	}
	CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(host), kCFStringEncodingMacRoman) + 1;
	char *server = new char[size];
	CFStringGetCString(host, server, size, kCFStringEncodingMacRoman);
	UInt16 port = 0;
	if (CFNumberGetType(portNumber) == kCFNumberSInt32Type) {
		SInt32 value;
		CFNumberGetValue(portNumber, kCFNumberSInt32Type, &value);
		port = (UInt16)value;
	}
	CFRelease(proxies);
	UInt32 passwordLength = 0;
	void *passwordData = 0;
	SecKeychainItemRef item = 0;
	OSStatus err = SecKeychainFindInternetPassword(0, (UInt32)strlen(server), server, 0, 0, 0, 0, 0, 0, port, kSecProtocolTypeAny,
	                                               kSecAuthenticationTypeAny,
	                                               &passwordLength, &passwordData, &item);
	delete[] server;
	if (err != noErr)
		return false;
	CFStringRef password =
	    CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)passwordData, passwordLength, kCFStringEncodingMacRoman, false);
	SecKeychainItemFreeContent(0, passwordData);
	bool ok = false;
	SecKeychainAttribute attr = {kSecAccountItemAttr, 0, 0};
	SecKeychainAttributeList attrs = {1, &attr};
	if (password && SecKeychainItemCopyContent(item, 0, &attrs, 0, 0) == noErr) {
		CFStringRef account = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)attr.data, attr.length,
		                                              kCFStringEncodingMacRoman, false);
		SecKeychainItemFreeContent(&attrs, 0);
		if (account) {
			ok = CFHTTPMessageAddAuthentication(_request, 0, account, password, kCFHTTPAuthenticationSchemeBasic,
			                                    true);
			_triedProxyAuth = true;
			CFRelease(account);
		}
	}
	if (password)
		CFRelease(password);
	if (item)
		CFRelease(item);
	return ok;
}

LString *LConnection::description() const
{
	CFURLRef url = CFHTTPMessageCopyRequestURL(_request);
	CFStringRef host = CFURLCopyNetLocation(url);
	CFStringRef resource = CFURLCopyResourceSpecifier(url);
	LString *text = LString::stringWithFormat("LConnection<0x%08lX>(host = \"%@\", \"%@\")", (unsigned long)this,
	                                          LString::stringWithCFString(host), LString::stringWithCFString(resource));
	if (host)
		CFRelease(host);
	if (resource)
		CFRelease(resource);
	if (url)
		CFRelease(url);
	return text;
}

} // namespace LF
