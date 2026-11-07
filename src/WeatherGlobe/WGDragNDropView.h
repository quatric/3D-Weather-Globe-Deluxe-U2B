#import "WeatherGlobePrefix.h"
#import "WGPlaceholderView.h"


@interface WGDragNDropView : WGPlaceholderView
{
    id<WGDragValidationDelegate> delegate;
}
- (BOOL)performDragOperation:(id)aOperation;
- (NSUInteger)draggingUpdated:(id)aUpdated;
- (NSUInteger)draggingEntered:(id)aEntered;
- (void)setDelegate:(id)aDelegate;
- (id)delegate;
- (id)initWithFrame:(NSRect)aFrame;
@end

