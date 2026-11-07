// LAutoreleasePoolInstaller
//
// Ties LAutoreleasePools to Cocoa's autorelease pools: LF::StAutoreleasePool-
// Installer, LF::StNSAutoreleasePool and the Objective-C classes behind them.
// Built without ARC.
//
// The original made LNSAutoreleasePool pose as NSAutoreleasePool, so that
// each NSAutoreleasePool autoreleased an LAutoreleasePoolWrapper whose
// dealloc removed the LAutoreleasePool it had added. Posing no longer exists:
// -[NSAutoreleasePool init] is exchanged with LNSAutoreleasePool's instead
// (once, and for good, as posing was). AppKit no longer makes
// NSAutoreleasePool objects for its events, so a main run loop observer also
// gives every pass of the run loop an LAutoreleasePool.

#include "LString.h"
#import <Foundation/Foundation.h>
#include <objc/runtime.h>

using namespace LF;

@interface LAutoreleasePoolWrapper : NSObject {
	StAutoreleasePool *_lpool;
}
@end

@implementation LAutoreleasePoolWrapper

- (id)init
{
	self = [super init];
	if (self)
		_lpool = new StAutoreleasePool;
	return self;
}

- (void)dealloc
{
	if (_lpool) {
		delete _lpool;
		_lpool = 0;
	}
	[super dealloc];
}

@end

@interface LNSAutoreleasePool : NSAutoreleasePool
@end

static bool sPoolInitExchanged = false;

@implementation LNSAutoreleasePool

// Runs as -[NSAutoreleasePool init] once exchanged; the call to itself is the
// original -init.
- (id)lf_init
{
	id pool = [self lf_init];
	if (pool)
		[[LAutoreleasePoolWrapper new] autorelease];
	return pool;
}

+ (void)lf_exchangeInit
{
	if (sPoolInitExchanged)
		return;
	sPoolInitExchanged = true;
	Method mine = class_getInstanceMethod(self, @selector(lf_init));
	class_addMethod([NSAutoreleasePool class], @selector(lf_init), method_getImplementation(mine),
	                method_getTypeEncoding(mine));
	method_exchangeImplementations(class_getInstanceMethod([NSAutoreleasePool class], @selector(init)),
	                               class_getInstanceMethod([NSAutoreleasePool class], @selector(lf_init)));
}

@end

@interface LAutoreleasePoolInstaller : NSObject
+ (void)install;
+ (void)uninstall;
@end

static id sInstalledLAutoreleasePool = nil;
static CFRunLoopObserverRef sPushObserver = 0;
static CFRunLoopObserverRef sPopObserver = 0;
static int sRunLoopPools = 0;

// The main run loop's pools: one from entry or wake-up to the next sleep or
// exit.
static void _pushPool(CFRunLoopObserverRef, CFRunLoopActivity, void *)
{
	LAutoreleasePool::addNewPoolToTop();
	sRunLoopPools++;
}

static void _popPool(CFRunLoopObserverRef, CFRunLoopActivity, void *)
{
	if (sRunLoopPools > 0) {
		sRunLoopPools--;
		LAutoreleasePool::removeTopPool();
	}
}

@implementation LAutoreleasePoolInstaller

+ (void)install
{
	if (sInstalledLAutoreleasePool)
		return;
	[LNSAutoreleasePool lf_exchangeInit];
	sInstalledLAutoreleasePool = [NSAutoreleasePool new];
	[[LAutoreleasePoolInstaller new] autorelease];
	if (sPushObserver == 0) {
		sPushObserver = CFRunLoopObserverCreate(0, kCFRunLoopEntry | kCFRunLoopAfterWaiting, true, -2147483646,
		                                        _pushPool, 0);
		sPopObserver = CFRunLoopObserverCreate(0, kCFRunLoopBeforeWaiting | kCFRunLoopExit, true, 2147483646,
		                                       _popPool, 0);
		CFRunLoopAddObserver(CFRunLoopGetMain(), sPushObserver, kCFRunLoopCommonModes);
		CFRunLoopAddObserver(CFRunLoopGetMain(), sPopObserver, kCFRunLoopCommonModes);
	}
}

+ (void)uninstall
{
	if (sPushObserver) {
		CFRunLoopRemoveObserver(CFRunLoopGetMain(), sPushObserver, kCFRunLoopCommonModes);
		CFRunLoopRemoveObserver(CFRunLoopGetMain(), sPopObserver, kCFRunLoopCommonModes);
		CFRelease(sPushObserver);
		CFRelease(sPopObserver);
		sPushObserver = sPopObserver = 0;
	}
	[sInstalledLAutoreleasePool release];
	sInstalledLAutoreleasePool = nil;
}

- (void)dealloc
{
	sInstalledLAutoreleasePool = nil;
	[super dealloc];
}

@end

namespace LF {

StAutoreleasePoolInstaller::StAutoreleasePoolInstaller()
{
	[LAutoreleasePoolInstaller install];
}

StAutoreleasePoolInstaller::~StAutoreleasePoolInstaller()
{
	[LAutoreleasePoolInstaller uninstall];
}

StNSAutoreleasePool::StNSAutoreleasePool()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	_nsPool = pool;
	_lPool = sPoolInitExchanged ? 0 : new StAutoreleasePool;
}

StNSAutoreleasePool::~StNSAutoreleasePool()
{
	if (_lPool) {
		delete _lPool;
		_lPool = 0;
	}
	[(NSAutoreleasePool *)_nsPool release];
}

} // namespace LF
