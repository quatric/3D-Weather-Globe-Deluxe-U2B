#import "WGMovieController.h"

@implementation NSString (TimeFromSecs)

// mm:ss
+ (id)timeFromSecs:(CGFloat)aSecs
{
	float secs = aSecs;
	return [NSString stringWithFormat:@"%.2d:%.2d", (int)(secs / 60.0f), (int)floorf(secs) % 60];
}

@end
