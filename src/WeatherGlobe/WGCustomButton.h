#import "WeatherGlobePrefix.h"


@interface WGCustomButton : NSButton
{
}
+ (Class)cellClass;
@end

@interface WGCustomButtonCell : NSButtonCell
{
    id _data;
}
- (id)accessibilityAttributeValue:(id)aValue;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)drawImageAtIndex:(NSInteger)aIndex withFrame:(NSRect)aFrame;
- (void)addImage:(id)aImage altImage:(id)aImage2 align:(NSInteger)aAlign scale:(NSInteger)aScale;
- (void)setImages:(id)aImages;
- (id)images;
- (void)awakeFromNib;
- (void)dealloc;
@end

