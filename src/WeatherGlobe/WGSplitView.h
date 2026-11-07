#import "WeatherGlobePrefix.h"


@interface WGSplitView : NSSplitView
{
    float _dividerThickness;
    NSColor *_dividerFillColor;
}
- (void)setDividerFillColor:(id)aColor;
- (id)dividerFillColor;
- (void)drawDividerInRect:(NSRect)aRect;
- (void)setDividerThickness:(CGFloat)aThickness;
- (CGFloat)dividerThickness;
- (BOOL)isFlipped;
- (id)initWithCoder:(id)aCoder;
@end

@interface NSSplitView (WGSizeConstains)
- (NSSize)wgMaxSizeForView:(id)aView;
- (NSSize)wgMinSizeForView:(id)aView;
- (NSSize)wgMaxSize;
- (NSSize)wgMinSize;
- (CGFloat)wgConstrainMaxCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt;
- (CGFloat)wgConstrainMinCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt;
- (void)wgCustomAdjustSubviews:(BOOL)aSubviews;
- (id)wgFittingSubviewsInContainer;
- (BOOL)wgCanAdjustViews:(id)aViews failedOnMax:(BOOL *)aMax;
@end

