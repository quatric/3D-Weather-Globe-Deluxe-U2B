#import "SMKControlsPrefix.h"


@interface SMKBackgroundButtonCell : NSButtonCell <SMKBackgroundRendererClient>
{
    id <SMKBackgroundRenderer> _backgroundRenderer;
    id _contentRenderer;
    NSImage *_backgroundImage;
    NSImage *_highlightedBackground;
    int _cachedBackgroundImageWidth;
    int _cachedControlTint;
    struct _SMKBackgroundCellFlags _smkBCFlags;
    NSRect _frameRect;
}
- (BOOL)accessibilityIsIgnored;
- (id)accessibilityHitTest:(NSPoint)aTest;
- (id)accessibilityAttributeValue:(id)aValue;
- (void)setControlSize:(NSControlSize)aSize;
- (NSInteger)displayedImagePosition;
- (void)setEnabled:(BOOL)aEnabled;
- (BOOL)isEnabled;
- (void)setHighlighted:(BOOL)aHighlighted;
- (void)setState:(NSInteger)aState;
- (BOOL)isSelected;
- (BOOL)isRightCap;
- (BOOL)isLeftCap;
- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp;
- (void)performClick:(id)aClick;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (NSSize)cellSizeForBounds:(NSRect)aBounds;
- (struct _SMKInset)contentInset;
- (NSSize)contentSizeForBounds:(NSRect)aBounds;
- (id)_backgroundImageForWidth:(CGFloat)aWidth flipped:(BOOL)aFlipped;
- (void)setBackgroundRenderer:(id)aRenderer;
- (id)backgroundRenderer;
- (id)initWithButtonCell:(id)aCell;
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (void)controlTintDidChange:(id)aChange;
- (id)copyWithZone:(NSZone *)aZone;
- (void)_setContentRenderer:(id)aRenderer;
- (void)_finishCopying;
- (void)dealloc;
- (id)initImageCell:(id)aCell;
- (id)initTextCell:(id)aCell;
- (void)_smkCellInit;
- (void)_acceptPropertiesFromButtonCell:(id)aCell;
- (void)setBordered:(BOOL)aBordered;
- (void)setBezeled:(BOOL)aBezeled;
- (id)contentRenderer;
- (void)_setupContentRenderer;
- (void)_invalidateImageCache;
@end

