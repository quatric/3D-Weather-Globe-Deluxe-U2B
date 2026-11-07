#import "WeatherGlobePrefix.h"
#import "WGTitledViewContainer.h"


@interface WGLeftAreaController : WGTitledViewContainer <WGMountableControllerContainer>
{
    NSMatrix *titleButtons;
    NSMatrix *currentSetButtons;
    NSView *resourcesView;
    WGMainEntriesListController *_entriesListController;
    WGCurrentSetController *_currentSetController;
    WGCommonEntryDetailsController *_entryDetailsController;
    BOOL _isSettingsMode;
    NSDictionary *_panesPrefs;
    id delegate;
}
- (id)entryDetailsController;
- (id)currentSetController;
- (id)entriesListController;
- (void)switchToSettingsMode:(BOOL)aMode;
- (void)showDetails:(BOOL)aDetails;
- (void)showCurrentSet:(BOOL)aSet;
- (BOOL)isSettingsMode;
- (BOOL)isCurrentSetPaneVisible;
- (BOOL)isDetailsPaneVisible;
- (void)showPane:(BOOL)aPane forController:(id)aController;
- (void)setupTitleButtons;
- (void)setupTitleButton:(id)aButton withBackgroundName:(id)aName altBackgroundName:(id)aName2 foregroundName:(id)aName3 altForegroundName:(id)aName4;
- (void)performLayout;
- (void)unmountController:(id)aController;
- (void)mountController:(id)aController;
- (void)removeSubview:(id)aSubview;
- (CGFloat)splitView:(id)aView constrainMaxCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt;
- (CGFloat)splitView:(id)aView constrainMinCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt;
- (void)splitView:(id)aView resizeSubviewsWithOldSize:(NSSize)aSize;
- (void)awakeFromNib;
- (void)dealloc;
- (id)init;
@end

