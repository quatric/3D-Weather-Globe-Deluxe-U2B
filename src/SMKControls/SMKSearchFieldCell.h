#import "SMKControlsPrefix.h"


@interface SMKSearchFieldCell : NSTextFieldCell
{
    struct _SMKSFFlags _sfFlags;
    NSButtonCell *_searchButtonCell;
    NSButtonCell *_cancelButtonCell;
    NSMenu *_searchMenuTemplate;
    NSMutableArray *_recentSearches;
    NSMenu *_searchMenu;
    NSTimer *_partialStringTimer;
    id _activeEditor;
    unsigned int _reserved1;
    unsigned int _reserved2;
    unsigned int _reserved3;
    unsigned int _reserved4;
}
- (void)resetCursorRect:(NSRect)aRect inView:(id)aView;
- (void)_smkTextDidEndEditing:(id)aEditing;
- (void)endEditing:(id)aEditing;
- (void)selectWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate start:(NSInteger)aStart length:(NSInteger)aLength;
- (void)editWithFrame:(NSRect)aFrame inView:(id)aView editor:(id)aEditor delegate:(id)aDelegate event:(id)aEvent;
- (void)_smkControls_setEditor:(id)aEditor;
- (void)_setRegistered:(BOOL)aRegistered forNotificationFromObject:(id)aObject;
- (void)setStringValue:(id)aValue;
- (void)textDidChange:(id)aChange;
- (BOOL)trackMouse:(id)aMouse inRect:(NSRect)aRect ofView:(id)aView untilMouseUp:(BOOL)aUp;
- (void)highlight:(BOOL)aHighlight withFrame:(NSRect)aFrame inView:(id)aView;
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (NSRect)drawingRectForBounds:(NSRect)aBounds;
- (NSSize)cellSizeForBounds:(NSRect)aBounds;
- (NSRect)cancelButtonRectForBounds:(NSRect)aBounds;
- (NSRect)searchButtonRectForBounds:(NSRect)aBounds;
- (NSRect)searchTextRectForBounds:(NSRect)aBounds;
- (void)setControlSize:(NSControlSize)aSize;
- (void)setEnabled:(BOOL)aEnabled;
- (id)copyWithZone:(NSZone *)aZone;
- (void)encodeWithCoder:(id)aCoder;
- (id)initWithCoder:(id)aCoder;
- (void)dealloc;
- (id)nsSearchFieldCell;
- (id)initWithTextFieldCell:(id)aCell;
- (id)init;
- (id)initTextCell:(id)aCell;
- (id)recentSearches;
- (void)setRecentSearches:(id)aSearches;
- (void)setMaximumRecents:(NSInteger)aRecents;
- (NSInteger)maximumRecents;
- (BOOL)sendsWholeSearchString;
- (void)setSendsWholeSearchString:(BOOL)aString;
- (id)searchMenuTemplate;
- (void)setSearchMenuTemplate:(id)aTemplate;
- (void)resetCancelButtonCell;
- (void)resetSearchButtonCell;
- (void)setCancelButtonCell:(id)aCell;
- (id)cancelButtonCell;
- (void)setSearchButtonCell:(id)aCell;
- (id)searchButtonCell;
@end

@interface SMKSearchFieldCell (SMKSearchFieldCell_Private)
- (void)_setTimerRunning:(BOOL)aRunning;
- (void)_invalidateTimer;
- (void)_sendPartialString;
- (void)_searchFieldClearRecents:(id)aRecents;
- (void)_searchFieldDoRecent:(id)aRecent;
- (void)_searchFieldCancel:(id)aCancel;
- (void)_searchFieldSearch:(id)aSearch;
- (BOOL)_trackButton:(id)aButton forEvent:(id)aEvent inRect:(NSRect)aRect ofView:(id)aView;
- (void)_highlight:(BOOL)aHighlight buttonCell:(id)aCell withFrame:(NSRect)aFrame inView:(id)aView;
- (void)_addRecent:(id)aRecent;
- (unsigned char)_maximumRecents;
- (void)_prepareSearchMenu;
- (void)_syncSearchImage;
- (id)controlViewControl;
@end

