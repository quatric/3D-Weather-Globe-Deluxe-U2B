#import "NSObject+SharedInstance.h"

// A shared instance's pointer holds this once the instance is gone.
#define _kTerminatedInstance ((id)-1)

@implementation NSObject (SharedInstance)

// The shared instance is invalidated when the application quits.
+ (void)_terminateSharedInstance
{
	id *instance = [self sharedInstancePtr];
	if (instance && *instance && *instance != _kTerminatedInstance) {
		[*instance invalidate];
		*instance = _kTerminatedInstance;
	}
}

// Made on the first call by classes that return their instance's storage
// from sharedInstancePtr.
+ (id)sharedInstance
{
	id *instance = [self sharedInstancePtr];
	if (!instance)
		return nil;
	if (!*instance) {
		*instance = [[[self class] alloc] init];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_terminateSharedInstance)
		                                             name:NSApplicationWillTerminateNotification object:nil];
		return *instance;
	}
	return *instance != _kTerminatedInstance ? *instance : nil;
}

+ (id *)sharedInstancePtr
{
	return NULL;
}

- (void)invalidate
{
	[self release];
}

@end
