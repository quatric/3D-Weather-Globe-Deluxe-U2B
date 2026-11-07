#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface WGMountableController : WGNibOwnerController <WGMountableControllerProtocol>
{
    NSView *pane;
    BOOL _isMounted;
    id<WGMountableControllerContainer> _container;
}
- (void)setContainer:(id)aContainer;
- (id)container;
- (BOOL)isMounted;
- (void)didMounted:(BOOL)aMounted;
- (void)dealloc;
- (BOOL)isActive;
- (id)pane;
@end

