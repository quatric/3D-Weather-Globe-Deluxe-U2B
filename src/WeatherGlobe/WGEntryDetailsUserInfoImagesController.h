#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsInfoDataProvider.h"


@interface WGEntryDetailsUserInfoImagesController : WGEntryDetailsInfoDataProvider <WGAutoresizeDelegate, WGDragValidationDelegate>
{
    SMKGridControl *_userImagesControl;
    NSMapTable *_thumbnailCacheMap;
    BOOL _isEditing;
    BOOL _isScaleImageToCellSize;
    BOOL _doNotPostNotification;
}
+ (id)pasteboardTypes;
- (void)entryContentDidChange:(id)aChange;
- (void)view:(id)aView didResizeWithOldSize:(NSSize)aSize;
- (BOOL)view:(id)aView acceptDrop:(id)aDrop;
- (NSUInteger)view:(id)aView validateDrop:(id)aDrop;
- (BOOL)canDeleteFromGridControl:(id)aControl;
- (BOOL)canPasteFromPasteboardToGridControl:(id)aControl;
- (BOOL)canCopyToPasteboardFromGridControl:(id)aControl;
- (id)pasteboardTypesForGridControl:(id)aControl;
- (id)registeredDraggedTypesForGridControl:(id)aControl;
- (void)selectionDidChangeInGridControl:(id)aControl;
- (void)_selectionDidChangeInGridControl:(id)aControl;
- (void)gridControl:(id)aControl pasteboard:(id)aPasteboard provideDataForType:(id)aType;
- (BOOL)gridControl:(id)aControl writeIndexes:(id)aIndexes toPasteboard:(id)aPasteboard;
- (BOOL)gridControl:(id)aControl acceptDrop:(id)aDrop itemIndex:(NSUInteger)aIndex;
- (NSUInteger)gridControl:(id)aControl validateDrop:(id)aDrop proposedIndex:(NSUInteger)aIndex;
- (void)gridControl:(id)aControl deleteIndexes:(id)aIndexes;
- (id)gridControl:(id)aControl imageForItemAt:(NSInteger)aAt;
- (id)gridControl:(id)aControl titleForItemAt:(NSInteger)aAt;
- (NSInteger)numberOfItemsInGridControl:(id)aControl;
- (id)cellImageAtIndex:(NSInteger)aIndex andCellSize:(NSSize)aSize;
- (id)thumbnailWithLBitmap:(LBitmapImage *)aBitmap ofSize:(NSSize)aSize;
- (void)adjustMatrixAndControl;
- (NSSize)adjustMatrixWithWidth:(CGFloat)aWidth;
- (void)adjustMatrixCentering;
- (void)selectImageAtIndex:(NSInteger)aIndex;
- (void)setScaleImageToCellSize:(BOOL)aSize;
- (BOOL)isScaleImageToCellSize;
- (void)didMountedToOwner:(id)aOwner;
- (BOOL)isEditing;
- (BOOL)tryStartDescriptionEditing;
- (void)validateEditing;
- (void)prepareCommit;
- (void)update;
- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset;
- (id)descriptionView;
- (id)placeholderImage;
- (id)title;
- (void)dealloc;
- (id)initWithOwner:(id)aOwner key:(LString *)aKey;
@end

