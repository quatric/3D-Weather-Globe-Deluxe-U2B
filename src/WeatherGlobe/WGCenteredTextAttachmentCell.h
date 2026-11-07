#import "WeatherGlobePrefix.h"


@interface WGCenteredTextAttachmentCell : NSTextAttachmentCell
{
    NSPoint _baselineOffset;
}
- (void)setBaselineOffset:(NSPoint)aOffset;
- (NSPoint)cellBaselineOffset;
@end

@interface NSAttributedString (AppKitWGExtensions)
+ (id)attributedStringWithImageAttachmentWithPath:(id)aPath forFont:(id)aFont;
- (id)wgSetLineBreak:(NSInteger)aBreak;
@end

