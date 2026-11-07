#import "WeatherGlobePrefix.h"


@interface WGTitledView : NSView
{
    id delegate;
    WGSplitter *_splitter;
    WGImageTextCell *_backgroundCell;
    NSMutableDictionary *_titleAttributes;
    NSView *_contentView;
    NSView *_titleView;
    NSSize _minContentViewSize;
    NSSize _maxContentViewSize;
    unsigned int _orderWeight;
}
- (void)setupSplitter;
- (void)initBackgroundCell;
- (NSRect)titleRect;
- (void)setTitleView:(id)aView;
- (void)setContentView:(id)aView attributes:(id)aAttributes;
- (void)setDelegate:(id)aDelegate;
- (id)titleView;
- (id)backgroundCell;
- (id)splitter;
- (id)contentView;
- (NSUInteger)orderWeight;
- (NSSize)wgMaxSize;
- (NSSize)wgMinSize;
- (void)resetCursorRects;
- (void)mouseDown:(id)aDown;
- (void)willRemoveSubview:(id)aSubview;
- (void)viewDidMoveToSuperview;
- (void)drawRect:(NSRect)aRect;
- (void)setFrameSize:(NSSize)aSize;
- (void)dealloc;
- (id)initWithFrame:(NSRect)aFrame;
@end

