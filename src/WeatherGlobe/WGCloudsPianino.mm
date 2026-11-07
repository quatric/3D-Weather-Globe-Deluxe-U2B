#import "WGCloudsPianino.h"

// The keys come from the delegate (numberOfKeysInPianino:,
// pianino:isEnabledKeyAtIndex:, pianino:dateForKeyAtIndex:).
@implementation WGCloudsPianino

+ (Class)cellClass
{
	return [WGCloudsPianinoCell class];
}

- (id)delegate
{
	return mDelegate;
}

- (void)setDelegate:(id)aDelegate
{
	mDelegate = aDelegate;
}

- (NSInteger)keysCount
{
	if ([mDelegate respondsToSelector:@selector(numberOfKeysInPianino:)])
		return (int)[mDelegate numberOfKeysInPianino:self];
	return 0;
}

- (BOOL)isEnabledKeyAtIndex:(NSInteger)aIndex
{
	if ([mDelegate respondsToSelector:@selector(pianino:isEnabledKeyAtIndex:)])
		return [mDelegate pianino:self isEnabledKeyAtIndex:aIndex];
	return YES;
}

- (id)dateForKeyAtIndex:(NSInteger)aIndex
{
	if ([mDelegate respondsToSelector:@selector(pianino:dateForKeyAtIndex:)])
		return [mDelegate pianino:self dateForKeyAtIndex:aIndex];
	return nil;
}

@end
