#import "WeatherGlobePrefix.h"


@interface WBAlert : NSObject
{
    NSWindow *_window;
    NSButton *_leftButt;
    NSButton *_midButt;
    NSButton *_rightButt;
    NSButton *_helpButt;
    NSTextField *_title;
    NSTextField *_body;
    NSButton *_dontShowButt;
    NSView *_customBgrView;
    id _delegate;
    int _defButt;
    int _cancelButt;
    int _alertTag;
}
- (void)setDontShowAgain:(BOOL)aAgain;
- (BOOL)dontShowAgain;
- (NSInteger)runModal;
- (void)buttAction:(id)aAction;
- (NSInteger)tag;
- (void)setTag:(NSInteger)aTag;
- (id)delegate;
- (void)setDelegate:(id)aDelegate;
- (void)dealloc;
- (id)initWithTitle:(id)aTitle body:(id)aBody rightButt:(id)aButt middleButt:(id)aButt2 leftButt:(id)aButt3 checkboxTitle:(id)aTitle2 hasHelpButt:(BOOL)aButt4 defButtIndex:(NSInteger)aIndex cancelButtIndex:(NSInteger)aIndex2 hasCustomBgr:(BOOL)aBgr withNibName:(id)aName alertCaption:(id)aCaption;
- (id)initWithTitle:(id)aTitle body:(id)aBody rightButt:(id)aButt middleButt:(id)aButt2 leftButt:(id)aButt3 checkboxTitle:(id)aTitle2 hasHelpButt:(BOOL)aButt4 defButtIndex:(NSInteger)aIndex cancelButtIndex:(NSInteger)aIndex2 hasCustomBgr:(BOOL)aBgr;
@end

@interface WBAlertClearView : NSView
{
}
- (void)drawRect:(NSRect)aRect;
- (BOOL)mouseDownCanMoveWindow;
@end

@interface WBAlertImageView : NSImageView
{
}
- (BOOL)mouseDownCanMoveWindow;
@end

@interface WBAlertWindow : NSPanel
{
}
- (BOOL)canBecomeKeyWindow;
@end

