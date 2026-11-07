#import "SMKControlsPrefix.h"


@interface SMKEngravedTextFieldCell : NSTextFieldCell
{
    NSColor *_secondaryTextColor;
    float _engraveOffset;
}
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (id)initWithTextFieldCell:(id)aCell;
- (id)init;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)_acceptPropertiesFromTextFieldCell:(id)aCell;
- (void)setEngraveOffset:(CGFloat)aOffset;
- (CGFloat)engraveOffset;
- (id)_secondaryTextColor;
- (id)secondaryTextColor;
- (void)setSecondaryTextColor:(id)aColor;
- (void)dealloc;
@end

