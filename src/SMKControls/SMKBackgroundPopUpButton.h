#import "SMKControlsPrefix.h"


@interface SMKBackgroundPopUpButton : NSPopUpButton
{
}
+ (Class)cellClass;
- (void)setState:(NSInteger)aState;
- (NSInteger)state;
- (BOOL)isFlipped;
@end

