#import "SMKControlsPrefix.h"


@interface NSBezierPath (SMKControlBorders)
+ (id)smkRoundedBorderPathWithRect:(NSRect)aRect cornerRadius:(CGFloat)aRadius;
+ (id)smkRolloverStyleBorderPathWithRect:(NSRect)aRect aroundRect:(BOOL)aRect2 rightCap:(BOOL)aCap leftCap:(BOOL)aCap2 bounds:(NSRect *)aBounds;
+ (id)smkRolloverStyleBorderPathAroundRect:(NSRect)aRect bounds:(NSRect *)aBounds;
+ (id)smkRolloverStyleBorderPathWithRect:(NSRect)aRect;
+ (id)smkFinderSelectionStyleBorderPathWithRect:(NSRect)aRect;
+ (id)smkRoundedBorderPathWithRect:(NSRect)aRect cornerRadius:(CGFloat)aRadius minXminY:(BOOL)aY minXmaxY:(BOOL)aY2 maxXminY:(BOOL)aY3 maxXmaxY:(BOOL)aY4;
@end

