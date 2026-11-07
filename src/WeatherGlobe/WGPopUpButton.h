#import "WeatherGlobePrefix.h"


@interface WGPopUpButton : NSPopUpButton
{
}
+ (Class)cellClass;
@end

@interface WGPopUpButtonCell : NSPopUpButtonCell
{
    float _extraHeight;
    NSDictionary *_attrs;
    float _titleWidth;
}
- (NSRect)titleRectForBounds:(NSRect)aBounds;
- (void)setItemTitlesStringAttributes:(id)aAttributes;
- (void)setTitleWidth:(CGFloat)aWidth;
- (void)setExtraWidthForCellHeight:(CGFloat)aHeight;
- (CGFloat)_extraWidthForCellHeight:(CGFloat)aHeight;
- (void)dealloc;
@end

