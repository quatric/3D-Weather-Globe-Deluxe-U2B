// LVersionInfo: a bundle's version. Only the build number (CFBundleVersion)
// is known on Mac OS X; the other parts read as 0.
#pragma once

#include "LObject.h"
#include <CoreFoundation/CoreFoundation.h>

namespace LF {

class LVersionInfo : public LObject {
	LF_DECLARE_CLASS(LVersionInfo)
public:
	enum TSubversion {
		kMajorVersion = 0,
		kMinorVersion,
		kBugFixVersion,
		kBuildVersion
	};

	LVersionInfo() : _build(0) {}
	virtual ~LVersionInfo() {}
	virtual void dealloc() { LObject::dealloc(); }
	virtual LVersionInfo *getOwnerLObject() { return this; }

	// Fails (and deallocates) if the bundle has no CFBundleVersion.
	LVersionInfo *initWithBundle(CFBundleRef bundle);
	unsigned short fileVersion(TSubversion) { return 0; }
	unsigned short productVersion(TSubversion part) { return part == kBuildVersion ? (unsigned short)_build : 0; }
	LString *productPublicVersion() { return 0; }

	int _build;
};

} // namespace LF
