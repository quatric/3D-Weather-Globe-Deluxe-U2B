#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsInfoDataProvider.h"


@interface WGEntryDetailsComplexDescriptionDataProvider : WGEntryDetailsInfoDataProvider
{
    NSTextView *_textView;
}
- (void)viewFrameDidChange:(id)aChange;
- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset;
- (id)descriptionView;
- (id)title;
- (void)validateDescriptionView;
- (void)dealloc;
- (id)initWithOwner:(id)aOwner key:(LString *)aKey;
@end

