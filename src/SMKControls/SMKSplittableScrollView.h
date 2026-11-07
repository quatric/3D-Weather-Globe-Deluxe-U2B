#import "SMKControlsPrefix.h"


@interface SMKSplittableScrollView : NSScrollView <NSCoding>
{
    NSView *_splitterView;
}
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (void)tile;
- (void)setSplitterViewIsVisible:(BOOL)aVisible;
- (BOOL)splitterViewIsVisible;
- (void)setSplitterView:(id)aView;
- (id)splitterView;
- (void)dealloc;
@end

