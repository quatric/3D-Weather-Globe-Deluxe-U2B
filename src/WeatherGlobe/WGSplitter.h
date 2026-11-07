#import "WeatherGlobePrefix.h"


@interface WGSplitter : SMKSplitter
{
    NSSplitView *splitView;
}
- (void)mouseDown:(id)aDown;
- (void)setEnclosingSplitView:(id)aView;
- (id)enclosingSplitView;
- (BOOL)acceptsFirstMouse:(id)aMouse;
@end

