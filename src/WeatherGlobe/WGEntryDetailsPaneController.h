#import "WeatherGlobePrefix.h"
#import "WGMountableController.h"


@interface WGEntryDetailsPaneController : WGMountableController <WGRebuildLayoutProtocol>
{
    WGEntryDetailsController *owner;
}
- (void)rebuildLayout:(id)aLayout;
- (void)rebuildLayout;
- (GenericEntry *)entry;
- (EntryInfo *)entryInfo;
- (void)entryDidChange;
- (void)entryWillChangeTo:(GenericEntry *)aTo;
- (BOOL)wantsUpdatesWhenInactive;
- (void)update;
- (BOOL)isEnabled;
- (id)iconName;
- (id)title;
- (id)owner;
- (void)didMounted:(BOOL)aMounted;
- (void)onEntryChanged;
- (void)onEntryWillChangeTo:(id)aTo;
- (void)dealloc;
- (id)initWithOwner:(id)aOwner;
@end

