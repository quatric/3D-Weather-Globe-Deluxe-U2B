#import "WeatherGlobePrefix.h"


@interface NSView (WGExtensions)
- (id)wgDescendantSearchField;
- (void)wgShiftSibviewsByXOnDelta:(CGFloat)aDelta;
- (id)subviewsSortedByPosition;
- (void)wgRecurseSortSubviewsByPos:(id)aPos;
- (id)wgAdjustedPlacholderStringAttributes:(id)aAttributes;
- (void)wgDrawPlaceholderStringInRect:(NSRect)aRect;
- (void)wgFixFlip;
- (NSSize)wgPreferredSize;
- (NSSize)wgMaxSize;
- (NSSize)wgMinSize;
- (id)wgEnclosingSplitView;
@end

