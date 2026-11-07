#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsKeyValueURLDataProvider.h"


@interface WGEntryDetailsKeyValueURLEditingDataProvider : WGEntryDetailsKeyValueURLDataProvider <WGRebuildLayoutProtocol>
{
    NSTextField *_linkTitle;
    NSTextField *_titleTitle;
    WGAutoAdjustableTextField *_linkDescription;
    WGAutoAdjustableTextField *_titleDescription;
}
- (id)objectValueForControl:(id)aControl;
- (void)controlTextDidEndEditing:(id)aEditing;
- (void)controlTextDidChange:(id)aChange;
- (void)validateDataObject;
- (void)validateDataObject:(id)aObject;
- (void)updateDescription;
- (void)rebuildLayout:(id)aLayout;
- (void)rebuildLayout;
- (void)viewSizesForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset outTitleSize:(NSSize *)aSize outDescriptionSize:(NSSize *)aSize2;
- (id)descriptionView;
- (id)titleView;
- (BOOL)tryStartDescriptionEditing;
- (BOOL)isDescriptionEditable;
- (void)dealloc;
- (Class)descriptionViewClass;
@end

