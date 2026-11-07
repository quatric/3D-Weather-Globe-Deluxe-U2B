#import "SMKControlsPrefix.h"


@interface SMKBackgroundPopUpButtonCell : NSPopUpButtonCell <SMKBackgroundRendererClient>
{
    id <SMKBackgroundRenderer> _backgroundRenderer;
    id _contentRenderer;
    NSRect _frameRect;
    NSImage *_backgroundImage;
    NSImage *_highlightedBackground;
    int _cachedBackgroundImageWidth;
    int _cachedControlTint;
    NSImage *_arrowImage;
    struct _SMKBPopUpFlags _smkBppFlags;
}
- (void)accessibilityPerformAction:(id)aAction;
- (BOOL)isEnabled;
- (BOOL)isSelected;
- (BOOL)isRightCap;
- (BOOL)isLeftCap;
- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp;
- (void)highlight:(BOOL)aHighlight withFrame:(NSRect)aFrame inView:(id)aView;
- (void)_showMenuWithEvent:(id)aEvent inView:(id)aView cellFrame:(NSRect)aFrame;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (NSSize)cellSizeForBounds:(NSRect)aBounds;
- (id)_backgroundImageForWidth:(CGFloat)aWidth flipped:(BOOL)aFlipped;
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (void)controlTintDidChange:(id)aChange;
- (id)initWithPopUpButtonCell:(id)aCell;
- (void)dealloc;
- (id)initTextCell:(id)aCell pullsDown:(BOOL)aDown;
- (void)_smkCellInit;
- (void)_acceptPropertiesFromPopUpButtonCell:(id)aCell;
- (void)setShowsStateBy:(NSInteger)aBy;
- (NSInteger)showsStateBy;
- (void)setHighlightsBy:(NSInteger)aBy;
- (NSInteger)highlightsBy;
- (void)setState:(NSInteger)aState;
- (NSInteger)state;
- (void)setTracksLikeButton:(BOOL)aButton;
- (BOOL)tracksLikeButton;
- (void)setDisplayedImage:(id)aImage;
- (id)displayedImage;
- (void)setDisplayedTitle:(id)aTitle;
- (id)displayedTitle;
- (void)setDisplayedImagePosition:(NSInteger)aPosition;
- (NSInteger)displayedImagePosition;
- (void)setDisplayedAlignment:(NSInteger)aAlignment;
- (NSInteger)displayedAlignment;
- (void)setHasCustomContent:(BOOL)aContent;
- (BOOL)hasCustomContent;
- (void)setDrawsArrow:(BOOL)aArrow;
- (BOOL)drawsArrow;
- (void)setDrawsBackground:(BOOL)aBackground;
- (BOOL)drawsBackground;
- (void)setControlSize:(NSControlSize)aSize;
- (void)setEnabled:(BOOL)aEnabled;
- (id)arrowImage;
- (id)_arrowImage;
- (void)setBackgroundRenderer:(id)aRenderer;
- (id)backgroundRenderer;
- (id)contentRenderer;
- (void)_setupContentRenderer;
- (void)_invalidateImageCache;
@end

