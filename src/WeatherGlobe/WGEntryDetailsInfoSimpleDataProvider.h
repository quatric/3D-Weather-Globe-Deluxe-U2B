#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsInfoDataProvider.h"


@interface WGEntryDetailsInfoSimpleDataProvider : WGEntryDetailsInfoDataProvider
{
    LObject *_object;
    NSView *_descriptionView;
}
- (void)controlTextDidBeginEditing:(id)aEditing;
- (void)controlTextDidEndEditing:(id)aEditing;
- (id)createDescriptionViewWithDescription:(id)aDescription;
- (id)createTitleViewWithTitleString:(id)aString;
- (id)objectValueForControl:(id)aControl;
- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset;
- (id)descriptionView;
- (BOOL)tryStartDescriptionEditing;
- (void)descriptionDidEndEditing:(id)aEditing;
- (id)descriptionPlaceholder;
- (void)updateDescription;
- (id)description;
- (void)validateDataObject;
- (void)setDataObject:(LObject *)aObject;
- (LObject *)dataObject;
- (void)didMountedToOwner:(id)aOwner;
- (void)dealloc;
- (id)initWithOwner:(id)aOwner key:(LString *)aKey dataObject:(LObject *)aObject;
- (Class)descriptionViewClass;
@end

