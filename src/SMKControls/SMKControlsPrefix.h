// Shared declarations for the SMKControls sources.
#import "WGSystem.h"

@protocol SMKBackgroundRenderer <NSObject>
@end
@protocol SMKBackgroundRendererClient <NSObject>
@end

@class SMKAngleSlider;
@class SMKAngleSliderCell;
@class SMKAttributedStringRenderer;
@class SMKBackgroundAbstractRenderer;
@class SMKBackgroundButtonCell;
@class SMKBackgroundPopUpButton;
@class SMKBackgroundPopUpButtonCell;
@class SMKEngravedTextFieldCell;
@class SMKGridAttributedCell;
@class SMKGridBaseCell;
@class SMKGridClipView;
@class SMKGridControl;
@class SMKGridDrawView;
@class SMKGridMatrix;
@class SMKGridTableView;
@class SMKGridTextFieldCell;
@class SMKScrollView;
@class SMKSearchField;
@class SMKSearchFieldCell;
@class SMKSegmentedCell;
@class SMKSegmentedControl;
@class SMKShadow;
@class SMKSplitView;
@class SMKSplittableScrollView;
@class SMKSplitter;
@class SMKStringFormatter;
@class SMKStringRenderer;
@class SMKTintedView;
@class _SMKBackgroundRenderer;
@class _SMKBackgroundRendererMini;
@class _SMKBackgroundRendererRegular;
@class _SMKBackgroundRendererSmall;

struct _SMKSegCellFlags {
    unsigned int reserved:24;
    unsigned int tracksPopUpMenuOnMouseDown:1;
    unsigned int isPartOfSmkSegmentedControl:1;
    unsigned int frameValid:1;
    unsigned int minWidthValid:1;
    unsigned int widthValid:1;
    unsigned int customWidth:1;
    unsigned int rightCap:1;
    unsigned int leftCap:1;
};

struct _SMKSegControlFlags {
    unsigned int reserved:20;
    unsigned int listensToFrameDidChange:1;
    unsigned int toolTipsSetUp:1;
    unsigned int ignoreProportions:1;
    unsigned int pendingUpdates:1;
    unsigned int updatesDelayed:1;
    unsigned int hasAutosizedSegments:1;
    unsigned int autosizeInfoValid:1;
    unsigned int cellFramesValid:1;
    unsigned int autosizesCells:1;
    unsigned int allowsEmptySelection:1;
    unsigned int trackingMode:2;
};

struct _SMKInset {
    float left;
    float top;
    float right;
    float bottom;
};

struct SMKControls_anon1 {
    unsigned int reserved:23;
    unsigned int isOpaque:1;
    unsigned int backgroundCacheValid:1;
    unsigned int doesntCacheBackground:1;
    unsigned int borderStyle:4;
    unsigned int drawsFill:1;
    unsigned int drawsBorder:1;
};

struct _SMKBackgroundCellFlags {
    unsigned int reserved:31;
    unsigned int dontCacheImages:1;
};

struct _SMKBPopUpFlags {
    unsigned int reserved:11;
    unsigned int dontDrawArrow:1;
    unsigned int showsStateBy:4;
    unsigned int highlightsBy:4;
    unsigned int state:1;
    unsigned int tracksLikeButton:1;
    unsigned int displayedAlignment:3;
    unsigned int displayedImagePosition:3;
    unsigned int dontDrawBackground:1;
    unsigned int reserved0:1;
    unsigned int hasCustomContent:1;
    unsigned int isHighlighted:1;
};

struct _SMKSFFlags {
    unsigned int reserved:18;
    unsigned int registeredForNotif:1;
    unsigned int customSearch:1;
    unsigned int customCancel:1;
    unsigned int menuTracking:1;
    unsigned int cancelVisible:1;
    unsigned int maximumRecents:8;
    unsigned int sendsWholeSearchString:1;
};


extern const struct _SMKInset SMKZeroInset;
float smk_floorf(float aValue);
float smk_ceilf(float aValue);
float smk_roundf(float aValue);

// -[NSImage compositeToPoint:operation:fraction:] (gone from AppKit): the
// image unscaled and upright with its bottom left corner at `point`, in
// flipped views too.
void SMKCompositeToPoint(NSImage *image, NSPoint point, NSCompositingOperation operation, CGFloat fraction);
// the search field cell's standard images; size 0 regular, 1 small, 2 mini
NSImage *SearchImageForSize(NSUInteger aSize, BOOL aPopup);
NSImage *CancelImageForSize(NSUInteger aSize, BOOL aAlternate);

// What SMKSplitView and SMKSplitter tell their split view's delegate.
@interface NSObject (SMKSplitViewDelegate)
- (void)splitView:(NSSplitView *)aSplitView didDoubleClickInDividerAtIndex:(NSInteger)aIndex;
- (void)splitView:(NSSplitView *)aSplitView didDoubleClickInSplitter:(id)aSplitter;
@end
