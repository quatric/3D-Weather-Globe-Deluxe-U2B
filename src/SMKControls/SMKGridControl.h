#import "SMKControlsPrefix.h"

// additionalInfoForItemAt: key: the item type VoiceOver reads (an NSString)
extern NSString *SMKGridAttributeVoiceOverItemTypeKey;


// the grid control's data source and delegate (all optional but the count)
@interface NSObject (SMKGridControlDataSource)
- (NSInteger)numberOfItemsInGridControl:(SMKGridControl *)aControl;
- (id)gridControl:(SMKGridControl *)aControl titleForItemAt:(NSInteger)aIndex;
- (id)gridControl:(SMKGridControl *)aControl imageForItemAt:(NSInteger)aIndex;
- (id)gridControl:(SMKGridControl *)aControl additionalInfoForItemAt:(NSInteger)aIndex;
- (BOOL)areTitlesCanBePreloadedInGridControl:(SMKGridControl *)aControl;
- (Class)cellClass;
@end

@interface NSObject (SMKGridControlDelegate)
- (id)pasteboardTypesForGridControl:(SMKGridControl *)aControl;
- (id)directPasteboardTypesForGridControl:(SMKGridControl *)aControl;
- (id)registeredDraggedTypesForGridControl:(SMKGridControl *)aControl;
- (void)gridControl:(SMKGridControl *)aControl pasteboard:(id)aPasteboard provideDataForType:(id)aType;
- (BOOL)gridControl:(SMKGridControl *)aControl writeIndexes:(id)aIndexes toPasteboard:(id)aPasteboard;
- (NSDragOperation)gridControl:(SMKGridControl *)aControl validateDrop:(id)aInfo proposedIndex:(NSUInteger)aIndex;
- (BOOL)gridControl:(SMKGridControl *)aControl acceptDrop:(id)aInfo itemIndex:(NSUInteger)aIndex;
- (void)gridControl:(SMKGridControl *)aControl deleteIndexes:(id)aIndexes;
- (void)gridControl:(SMKGridControl *)aControl draggedImage:(id)aImage endedAt:(NSPoint)aPoint operation:(NSDragOperation)aOperation;
- (BOOL)gridControl:(SMKGridControl *)aControl performKeyEquivalent:(id)aEvent;
- (BOOL)canCopyToPasteboardFromGridControl:(SMKGridControl *)aControl;
- (BOOL)canPasteFromPasteboardToGridControl:(SMKGridControl *)aControl;
- (BOOL)canDeleteFromGridControl:(SMKGridControl *)aControl;
- (void)selectionDidChangeInGridControl:(SMKGridControl *)aControl;
@end

@interface SMKGridControl : NSSplitView <NSUserInterfaceValidations>
{
    int _mode;
    NSSize _thumbnailSize;
    NSSize _thumbnailCellSize;
    BOOL _isThreadedPreloadingEnabled;
    id dataSource;
    id delegate;
    NSScrollView *_scrollView;
    SMKGridMatrix *_gridMatrix;
    SMKGridTableView *_tableView;
    SMKGridDrawView *_drawView;
    NSString *_placeholderString;
    NSString *_stringForEmptyTitle;
    BOOL _isDragEnabled;
    BOOL _isExtraDataProvidingAllowed;
    BOOL _isDiscreetScrollingEnabled;
    BOOL _isEmulatingNoOwnScrollView;
    unsigned int _draggingLastProposedIndex;
    unsigned int _draggingLastDragOperation;
}
+ (Class)scrollViewClass;
+ (Class)gridMatrixClass;
- (void)setDelegate:(id)aDelegate;
- (id)delegate;
- (void)notifyOfAppendedItems;
- (void)reloadData;
- (void)_reloadDataInRange:(NSRange)aRange;
- (void)setDataSource:(id)aSource;
- (id)dataSource;
- (id)additionalInfoForItemAt:(NSInteger)aAt;
- (id)titleForItemAt:(NSInteger)aAt;
- (id)imageForItemAt:(NSInteger)aAt;
- (BOOL)performKeyEquivalent:(id)aEquivalent;
- (void)onMouseDown:(id)aDown;
- (void)setTarget:(id)aTarget;
- (void)setDoubleAction:(SEL)aAction;
- (void)setAction:(SEL)aAction;
- (void)setExtraPasteboardDataProvidingAllowed:(BOOL)aAllowed;
- (BOOL)isExtraPasteboardDataProvidingAllowed;
- (void)scrollToVisibleAtIndex:(NSUInteger)aIndex;
- (void)setSelectedItem:(id)aItem byExtendingSelection:(BOOL)aSelection;
- (id)selectedItemIndexes;
- (id)selectedItems;
- (id)selectedItem;
- (void)selectItemAtIndex:(NSUInteger)aIndex byExtendingSelection:(BOOL)aSelection;
- (NSUInteger)selectedItemIndex;
- (NSUInteger)indexOfCell:(id)aCell;
- (id)cellAtIndex:(NSUInteger)aIndex;
- (NSUInteger)count;
- (void)setTableViewAutofitTextDuringLiveResize:(BOOL)aResize;
- (void)setThreadedPreloadingEnabled:(BOOL)aEnabled;
- (BOOL)isThreadedPreloadingEnabled;
- (BOOL)isTitlePreloadingEnabled;
- (void)setCellControlSize:(NSInteger)aSize;
- (void)setScrollerControlSize:(NSInteger)aSize;
- (NSInteger)scrollerControlSize;
- (BOOL)isEmulatingNoOwnScrollView;
- (void)setEmulatingNoOwnScrollView:(BOOL)aView;
- (void)setDiscreetScrollingEnabled:(BOOL)aEnabled;
- (BOOL)isDiscreetScrollingEnabled;
- (void)setGraphicContentShadowEnabled:(BOOL)aEnabled;
- (BOOL)isGraphicContentShadowEnabled;
- (void)setImageFrameColor:(id)aColor;
- (id)imageFrameColor;
- (void)setSelectionStyle:(NSInteger)aStyle;
- (NSInteger)selectionStyle;
- (void)setAllowsMultipleSelection:(BOOL)aSelection;
- (BOOL)allowsMultipleSelection;
- (CGFloat)cellHeightToWidthProportion;
- (void)setCellHeightToWidthProportion:(CGFloat)aProportion;
- (void)setColumnCount:(NSInteger)aCount;
- (void)setThumbnailCachingEnabled:(BOOL)aEnabled;
- (BOOL)isThumbnailCachingEnabled;
- (void)setThumbnailHyphenationEnabled:(BOOL)aEnabled;
- (void)setThumbnailMaxTitleHeight:(CGFloat)aHeight;
- (void)setThumbnailSize:(NSSize)aSize;
- (void)setThumbnailSize:(NSSize)aSize andBorderOffset:(NSSize)aOffset;
- (NSSize)thumbnailSize;
- (void)_updateSelection;
- (void)viewDidEndLiveResize;
- (void)viewWillStartLiveResize;
- (void)tableViewSelectionDidChange:(id)aChange;
- (void)selectionDidChange;
- (void)concludeDragOperation:(id)aOperation;
- (BOOL)performDragOperation:(id)aOperation;
- (BOOL)prepareForDragOperation:(id)aOperation;
- (void)draggingExited:(id)aExited;
- (NSUInteger)draggingUpdated:(id)aUpdated;
- (NSUInteger)draggingEntered:(id)aEntered;
- (void)invalidateDraggedTypes;
- (void)setDragEnabled:(BOOL)aEnabled;
- (BOOL)isDragEnabled;
- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType;
- (void)draggedImage:(id)aImage endedAt:(NSPoint)aAt operation:(NSUInteger)aOperation;
- (void)dragImage:(id)aImage imageOrigin:(NSPoint)aOrigin source:(id)aSource event:(id)aEvent;
- (BOOL)writeSelectionToPasteboard:(id)aPasteboard withImage:(id)aImage;
- (id)imageForMultipleDraggedItemsForImage:(id)aImage;
- (BOOL)validateUserInterfaceItem:(id)aItem;
- (BOOL)canDelete;
- (BOOL)canPaste;
- (BOOL)canCopy;
- (void)cut:(id)aCut;
- (void)delete:(id)aDelete;
- (void)paste:(id)aPaste;
- (void)copy:(id)aCopy;
- (void)setStringForEmptyTitle:(id)aTitle;
- (id)stringForEmptyTitle;
- (void)drawPlaceholderStringInRect:(NSRect)aRect;
- (void)setPlaceholderString:(id)aString;
- (id)placeholderString;
- (NSInteger)mode;
- (void)setMode:(NSInteger)aMode;
- (id)tableView;
- (id)gridMatrix;
- (id)scrollView;
- (void)makeFirstResponder;
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (void)dealloc;
- (void)awakeFromNib;
- (id)initWithFrame:(NSRect)aFrame;
@end

