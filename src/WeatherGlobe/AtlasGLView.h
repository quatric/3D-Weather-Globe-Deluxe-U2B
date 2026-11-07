#import "WeatherGlobePrefix.h"


@interface AtlasGLView : C3DViewWrapper
{
    MainWindowController *_controller;
    NSWindow *_progressWnd;
    NSTextField *_progressText;
    NSProgressIndicator *_progressIndicator;
    NSButton *_progressButton;
    int _destImgDataFormat;
    NSData *_destImgData;
    unsigned char _disableRefresh;
    NSCursor *_cursor;
}
- (id)accessibilityAttributeValue:(id)aValue;
- (BOOL)accessibilityIsIgnored;
- (void)onProgressWindowButton:(id)aButton;
- (void)_onImageConversionDone:(id)aDone;
- (NSInteger)_onImageConversion:(id)aConversion;
- (id)grabScreenshotOfApproxSize:(NSSize)aSize virtualSize:(NSSize)aSize2 dataFormat:(NSInteger)aFormat;
- (void)_entriesListDraggingWasEnded:(id)aEnded;
- (void)concludeDragOperation:(id)aOperation;
- (BOOL)performDragOperation:(id)aOperation;
- (BOOL)prepareForDragOperation:(id)aOperation;
- (void)draggingExited:(id)aExited;
- (NSUInteger)draggingUpdated:(id)aUpdated;
- (NSUInteger)draggingEntered:(id)aEntered;
- (void)resetCursorRects;
- (void)setCursor:(id)aCursor;
- (id)cursor;
- (AtlasPanelController *)_currentPanelController;
- (void)setNeedsDisplay:(BOOL)aDisplay;
- (void)dealloc;
- (void)awakeFromNib;
@end

