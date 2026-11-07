#import "WeatherGlobePrefix.h"


@interface WGEntryDetailsInfoDataProvider : NSObject
{
    WGEntryDetailsInfoPaneController *_owner;
    LString *_key;
    NSString *_titleString;
    NSView *_titleView;
}
- (id)createDescriptionViewWithDescription:(id)aDescription;
- (id)createTitleViewWithTitleString:(id)aString;
- (NSSize)descriptionSizeForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset;
- (NSSize)titleSizeForTitleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset;
- (void)prepareCommit;
- (void)didMountedToOwner:(id)aOwner;
- (void)viewSizesForSuperviewBounds:(NSRect)aBounds titleWidth:(CGFloat)aWidth offset:(CGFloat)aOffset outTitleSize:(NSSize *)aSize outDescriptionSize:(NSSize *)aSize2;
- (BOOL)tryStartDescriptionEditing;
- (BOOL)isDescriptionEditable;
- (id)descriptionView;
- (id)titleView;
- (id)title;
- (LString *)key;
- (id)owner;
- (void)dealloc;
- (id)initWithOwner:(id)aOwner key:(LString *)aKey;
@end

