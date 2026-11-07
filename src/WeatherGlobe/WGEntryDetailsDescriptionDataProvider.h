#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsInfoSimpleDataProvider.h"


@interface WGEntryDetailsDescriptionDataProvider : WGEntryDetailsInfoSimpleDataProvider <WGRebuildLayoutProtocol>
{
}
- (void)controlTextDidEndEditing:(id)aEditing;
- (LObject *)dataObject;
- (void)rebuildLayout;
- (void)rebuildLayout:(id)aLayout;
- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset;
- (id)description;
- (BOOL)isDescriptionEditable;
- (id)descriptionPlaceholder;
- (id)descriptionView;
- (void)updateDescription;
- (id)title;
- (Class)descriptionViewClass;
- (void)dealloc;
@end

