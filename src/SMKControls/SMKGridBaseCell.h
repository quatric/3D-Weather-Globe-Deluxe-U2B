#import "SMKControlsPrefix.h"


@interface SMKGridBaseCell : NSActionCell
{
    NSSize _contentSize;
    NSString *_textValue;
    NSString *_cachedFittingTextValue;
    NSSize _cachedFittingTextBoundsSize;
    NSImage *_cachedImage;
    unsigned int _isCachedImageValid:1;
    unsigned int _isUpdateNeeded:1;
    int _dataNo;
    NSString *_itemType;
    NSLock *_lock;
    NSLock *_stringRenderingLock;
}
- (id)accessibilityFocusedUIElement;
- (id)accessibilityHitTest:(NSPoint)aTest;
- (BOOL)accessibilityIsIgnored;
- (id)accessibilityAttributeValue:(id)aValue;
- (id)copyWithZone:(NSZone *)aZone;
- (id)_cachedImage;
- (BOOL)hitTestPoint:(NSPoint)aPoint inFrame:(NSRect)aFrame view:(id)aView;
- (NSRect)textPlacementRectForBounds:(NSRect)aBounds;
- (NSRect)graphicContentPlacementRectForBounds:(NSRect)aBounds;
- (NSRect)cachedImageRectForBounds:(NSRect)aBounds;
- (NSSize)cachedImageSizeForSize:(NSSize)aSize bounds:(NSSize)aBounds;
- (NSSize)graphicContentSize;
- (void)updateContentSize;
- (void)emptyTitleCache;
- (void)updateTitleIfNeeded;
- (void)updateContentIfNeeded;
- (void)setNeedsUpdateAsItem:(NSInteger)aItem;
- (void)setNeedsUpdateAsItem:(NSInteger)aItem discardCacheImmediately:(BOOL)aImmediately;
- (void)setImage:(id)aImage;
- (id)image;
- (BOOL)hasGraphicComponent;
- (void)setTitle:(id)aTitle;
- (id)title;
- (void)drawCachingGraphicContentInRect:(NSRect)aRect intoCache:(id)aCache;
- (id)cacheTextImageForBounds:(NSRect)aBounds;
- (id)cachedTextImageForBounds:(NSRect)aBounds;
- (id)cachedImageWithSize:(NSSize)aSize;
- (void)cacheGraphicContentWithSize:(NSSize)aSize;
- (id)cachedGraphicContentWithSize:(NSSize)aSize;
- (void)drawGraphicContentInFrame:(NSRect)aFrame;
- (void)drawShadowAroundRect:(NSRect)aRect;
- (void)drawTextSelectionInRect:(NSRect)aRect cellFrame:(NSRect)aFrame;
- (void)drawImageSelectionInRect:(NSRect)aRect cellFrame:(NSRect)aFrame isGraphicsContentReady:(BOOL)aReady;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView isGraphicsContentReady:(BOOL)aReady;
- (void)drawWithFrame:(NSRect)aFrame inView:(id)aView;
- (NSRect)interiorRectForFrame:(NSRect)aFrame inView:(id)aView;
- (id)loadingStatusImage;
- (id)stringRenderingLock;
- (id)lock;
- (BOOL)isImageSelectionOnTop;
- (id)controlView;
- (void)setState:(NSInteger)aState;
- (void)dealloc;
- (id)init;
@end

