#import "WGTimeLineControl.h"

@implementation WGTimeLineControl

+ (Class)cellClass
{
	return [WGTimeLineCell class];
}

- (LDate *)dateValue
{
	id cell = [self cell];
	if ([cell isKindOfClass:[WGTimeLineCell class]])
		return [(WGTimeLineCell *)cell dateValue];
	return 0;
}

- (void)setDateValue:(LDate *)aValue
{
	id cell = [self cell];
	if ([cell isKindOfClass:[WGTimeLineCell class]])
		[(WGTimeLineCell *)cell setDateValue:aValue];
}

- (BOOL)acceptsFirstMouse:(id)aMouse
{
	return YES;
}

@end
