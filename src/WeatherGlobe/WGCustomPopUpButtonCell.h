#import "WeatherGlobePrefix.h"


@interface WGCustomPopUpButtonCell : NSPopUpButtonCell
{
    NSImage * _images[3];
    NSDictionary *_attribsDict;
}
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)setImage:(id)aImage forPosition:(NSInteger)aPosition;
- (id)imageForPosition:(NSInteger)aPosition;
- (id)attributesDict;
- (void)dealloc;
- (id)initWithPopUpButtonCell:(id)aCell;
@end

