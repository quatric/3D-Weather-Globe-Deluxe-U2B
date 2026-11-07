#import "WeatherGlobePrefix.h"


@interface WGCloudsPianino : NSControl
{
    id mDelegate;
}
+ (Class)cellClass;
- (id)dateForKeyAtIndex:(NSInteger)aIndex;
- (BOOL)isEnabledKeyAtIndex:(NSInteger)aIndex;
- (NSInteger)keysCount;
- (void)setDelegate:(id)aDelegate;
- (id)delegate;
@end

@interface WGCloudsPianinoCell : NSCell
{
    NSTextFieldCell *_cell;
}
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)_drawTitle:(id)aTitle withFrame:(NSRect)aFrame inView:(id)aView;
- (void)dealloc;
@end


// The pianino's delegate.
@interface NSObject (WGCloudsPianinoDelegate)
- (NSInteger)numberOfKeysInPianino:(id)aPianino;
- (BOOL)pianino:(id)aPianino isEnabledKeyAtIndex:(NSInteger)aIndex;
- (id)pianino:(id)aPianino dateForKeyAtIndex:(NSInteger)aIndex;
@end
