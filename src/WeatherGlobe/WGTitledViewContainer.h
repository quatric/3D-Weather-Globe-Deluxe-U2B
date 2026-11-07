#import "WeatherGlobePrefix.h"


@interface WGTitledViewContainer : NSObject
{
    WGSplitView *splitView;
}
- (void)removeSubview:(id)aSubview;
- (void)replaceSubview:(id)aSubview withView:(id)aView withAttributes:(id)aAttributes;
- (void)addSubview:(id)aSubview withAttributes:(id)aAttributes;
- (BOOL)_addSubview:(id)aSubview withAttributes:(id)aAttributes;
- (void)addSubview:(id)aSubview;
- (NSUInteger)indexForWeight:(NSUInteger)aWeight;
- (NSUInteger)indexOfSubview:(id)aSubview;
- (id)splitView;
@end

