// LVersionInfo
//
// LF::LVersionInfo.

#include "LVersionInfo.h"

namespace LF {

LF_IMPLEMENT_CLASS(LVersionInfo, 0)

LVersionInfo *LVersionInfo::initWithBundle(CFBundleRef bundle)
{
	CFTypeRef version = CFBundleGetValueForInfoDictionaryKey(bundle, kCFBundleVersionKey);
	if (version == 0) {
		dealloc();
		return 0;
	}
	_build = CFStringGetIntValue((CFStringRef)version);
	return this;
}

} // namespace LF
