#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsPaneController.h"


@interface WGEntryDetailsHeaderViewController : WGEntryDetailsPaneController <WGRebuildLayoutProtocol, WGEditableController>
{
    NSImageView *imageView;
    NSTextField *ownName;
    NSTextField *ownType;
    NSTextField *hostCountry;
    NSImageView *editStatusView;
    NSButton *gotoButton;
    NSComboBox *regionComboBox;
    BOOL _isNibLoadingFinished;
    float _initialHeight;
    TLArray<LF::LMap*> *_regionSearchResults;
    LString *_regionSearchPrefix;
    BOOL _isRegionsListPoppedUp;
}
- (void)textDidEndEditing:(id)aEditing;
- (void)comboBoxWillDismiss:(id)aDismiss;
- (void)comboBoxWillPopUp:(id)aUp;
- (id)comboBox:(id)aBox objectValueForItemAtIndex:(NSInteger)aIndex;
- (NSInteger)numberOfItemsInComboBox:(id)aBox;
- (void)regionComboBoxTextDidEndEditing:(id)aEditing;
- (void)regionComboBoxTextDidChange:(id)aChange;
- (id)regionComboBoxSearchPrefix;
- (void)refineRegionSearchIfNeeded;
- (BOOL)searchForRegionWithPrefix:(LString *)aPrefix;
- (CGFloat)layoutControl:(id)aControl withYOffset:(CGFloat)aOffset withWidth:(CGFloat)aWidth;
- (void)gotoCurrentLocation:(id)aLocation;
- (void)rebuildLayout:(id)aLayout;
- (void)rebuildLayout;
- (void)validateData;
- (void)prepareCommit;
- (BOOL)tryOpenFieldEditor;
- (BOOL)isEditing;
- (void)validateEditing;
- (BOOL)canEdit;
- (void)update;
- (void)entryWillChangeTo:(GenericEntry *)aTo;
- (id)pane;
- (void)awakeFromNib;
- (id)nibName;
- (void)dealloc;
- (id)initWithOwner:(id)aOwner;
@end

