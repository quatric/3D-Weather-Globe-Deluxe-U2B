#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsInfoTitlePopupDataProvider.h"


@interface WGEntryDetailsCoordinatesEditingDataProvider : WGEntryDetailsInfoTitlePopupDataProvider <WGRebuildLayoutProtocol>
{
    NSArray *_ddInputControls;
    NSArray *_dmsInputContols;
    NSNumberFormatter *_ddFormatter;
    NSNumberFormatter *_dmsFormatter;
}
- (void)controlTextDidEndEditing:(id)aEditing;
- (void)popupAction:(id)aAction;
- (void)titlePopupAction:(id)aAction;
- (id)popupTitles;
- (void)validateDataObject:(id)aObject;
- (void)validateDataObject;
- (void)validateControls;
- (void)addInputControlsToDescriptionView;
- (id)popupWithItems:(id)aItems frameXOrigin:(float *)aOrigin;
- (id)controlWithDescription:(id)aDescription editable:(BOOL)aEditable frameXOrigin:(float *)aOrigin formatter:(id)aFormatter;
- (id)dmsFormatter;
- (id)ddFormatter;
- (id)dmsInputControls;
- (id)ddInputControls;
- (void)rebuildLayout;
- (void)rebuildLayout:(id)aLayout;
- (id)objectValueForControl:(id)aControl;
- (void)updateDescription;
- (id)descriptionView;
- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset;
- (BOOL)tryStartDescriptionEditing;
- (BOOL)isDescriptionEditable;
- (void)dealloc;
@end

