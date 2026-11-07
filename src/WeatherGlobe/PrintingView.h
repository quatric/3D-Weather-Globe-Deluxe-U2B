#import "WeatherGlobePrefix.h"


@interface PrintingView : NSView
{
    PrintController *_controller;
}
- (void)drawRect:(NSRect)aRect;
- (NSRect)rectForPage:(NSInteger)aPage;
- (BOOL)knowsPageRange:(NSRange *)aRange;
- (id)printJobTitle;
- (BOOL)isOpaque;
- (void)setPrintController:(id)aController;
- (id)printController;
@end

