#import "WeatherGlobePrefix.h"
#import "WGPaletteBaseController.h"
#import "WGTexturedView.h"


@interface IPhotoImportController : WGPaletteBaseController
{
    WGIPhotoImportActiveTexturedView *_texturedBottomView;
    WGSplitView *_splitView;
    NSOutlineView *_albumsOutlineView;
    SMKGridControl *_photosGrid;
    NSSearchField *_searchField;
    NSTextField *_infoText;
    WGCustomSlider *_slider;
    NSMutableDictionary *_photos;
    NSMutableArray *_albums;
    IPhotoAlbum *_selectedAlbum;
    NSArray *_filteredItems;
    NSImage *_invalidPhotoImage;
    NSString *_filterString;
    NSString *_categoryString;
    NSMutableDictionary *_modificationTimes;
    BOOL _iPhotoLibraryAbsent;
}
- (BOOL)writeIndexes:(id)aIndexes toPasteboard:(id)aPasteboard;
- (void)onDoubleClick:(id)aClick;
- (id)modificationTimes;
- (void)initializeWithLibraries:(id)aLibraries;
- (void)initializeWithLibraryFiles:(id)aFiles;
- (id)updateLibrary:(id)aLibrary withMajorVersion:(NSInteger)aVersion;
- (void)filterPhotos:(id)aPhotos byCategory:(id)aCategory;
- (id)filter:(id)aFilter byCategory:(id)aCategory;
- (id)invalidPhotoImage;
- (NSSize)splitView:(id)aView minSizeForView:(id)aView2;
- (CGFloat)splitView:(id)aView constrainMaxCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt;
- (CGFloat)splitView:(id)aView constrainMinCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt;
- (void)splitView:(id)aView resizeSubviewsWithOldSize:(NSSize)aSize;
- (void)outlineViewSelectionDidChange:(id)aChange;
- (BOOL)outlineView:(id)aView shouldSelectItem:(id)aItem;
- (id)outlineView:(id)aView objectValueForTableColumn:(id)aColumn byItem:(id)aItem;
- (id)outlineView:(id)aView child:(NSInteger)aChild ofItem:(id)aItem;
- (BOOL)outlineView:(id)aView isItemExpandable:(id)aExpandable;
- (NSInteger)outlineView:(id)aView numberOfChildrenOfItem:(id)aItem;
- (BOOL)gridControl:(id)aControl performKeyEquivalent:(id)aEquivalent;
- (BOOL)gridControl:(id)aControl writeIndexes:(id)aIndexes toPasteboard:(id)aPasteboard;
- (BOOL)canCopyToPasteboardFromGridControl:(id)aControl;
- (id)gridControl:(id)aControl additionalInfoForItemAt:(NSInteger)aAt;
- (id)gridControl:(id)aControl imageForItemAt:(NSInteger)aAt;
- (id)gridControl:(id)aControl titleForItemAt:(NSInteger)aAt;
- (BOOL)areTitlesCanBePreloadedInGridControl:(id)aControl;
- (NSInteger)numberOfItemsInGridControl:(id)aControl;
- (void)selectItem:(id)aItem;
- (void)sliderAction:(id)aAction;
- (void)filter:(id)aFilter;
- (void)showPalette;
- (void)dealloc;
- (void)awakeFromNib;
- (id)autosaveName;
- (id)nibName;
@end

@interface WGIPhotoImportActiveTexturedView : WGTexturedView
{
}
- (BOOL)mouseDownCanMoveWindow;
@end

