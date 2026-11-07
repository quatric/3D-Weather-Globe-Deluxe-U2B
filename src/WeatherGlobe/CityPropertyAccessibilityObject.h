#import "WeatherGlobePrefix.h"


@interface CityPropertyAccessibilityObject : NSObject
{
    NSRect _frameRect;
    NSView *_parentView;
    NSString *_description;
}
- (BOOL)accessibilityIsIgnored;
- (id)accessibilityActionNames;
- (BOOL)accessibilityIsAttributeSettable:(id)aSettable;
- (id)accessibilityAttributeValue:(id)aValue;
- (id)accessibilityAttributeNames;
- (void)dealloc;
- (id)initWithParentView:(id)aView frameRect:(NSRect)aRect description:(id)aDescription;
@end

@interface CityPropertyGenericView : NSView
{
    PointEntry *_entry;
}
+ (id)optionsMenuItems;
+ (id)_menuItemsDescriptors;
- (BOOL)accessibilityIsAttributeSettable:(id)aSettable;
- (BOOL)accessibilityIsIgnored;
- (id)accessibilityAttributeValue:(id)aValue;
- (id)accessibilityAttributeNames;
- (id)accessibilityObjects;
- (NSSize)sizeForBestFit;
- (BOOL)enabled;
- (BOOL)acceptsFirstMouse:(id)aMouse;
- (void)dealloc;
- (id)initWithCityEntry:(PointEntry *)aEntry;
@end

