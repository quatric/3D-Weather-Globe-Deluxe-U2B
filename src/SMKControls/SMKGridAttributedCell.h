#import "SMKControlsPrefix.h"
#import "SMKGridBaseCell.h"


@interface SMKGridAttributedCell : SMKGridBaseCell
{
    NSArray *_attributeImages;
    NSLock *_attributeImagesLock;
}
- (id)attributeImagesLock;
- (NSRect)attributeImagesFrameForFrame:(NSRect)aFrame;
- (void)setAttributeImages:(id)aImages;
- (void)updateContentIfNeeded;
- (void)drawGraphicContentInFrame:(NSRect)aFrame;
- (void)dealloc;
@end

