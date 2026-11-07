#import "WeatherGlobePrefix.h"


@interface WGPreferencesWrapper : NSObject
{
}
+ (id)sharedInstance;
- (void)setTerrainDetailsLevel:(CGFloat)aLevel;
- (CGFloat)terrainDetailsLevel;
- (void)setDrawWithReliefEnabled:(BOOL)aEnabled;
- (BOOL)drawWithReliefEnabled;
- (void)setAnimatedWaterUsesLargeTexture:(BOOL)aTexture;
- (BOOL)animatedWaterUsesLargeTexture;
- (void)setAnimatedWaterEnabled:(BOOL)aEnabled;
- (BOOL)animatedWaterEnabled;
- (void)setUsesSoftwareUpdating:(BOOL)aUpdating;
- (BOOL)usesSoftwareUpdating;
- (void)setDoShowWeatherInDock:(BOOL)aDock;
- (BOOL)doShowWeatherInDock;
- (void)setSoundVolume:(CGFloat)aVolume;
- (CGFloat)soundVolume;
- (void)setPlaySounds:(BOOL)aSounds;
- (BOOL)playSounds;
- (BOOL)isSoundsAvailable;
- (void)setUses24HourClock:(BOOL)aClock;
- (BOOL)uses24HourClock;
- (void)setUsesMetricSystem:(BOOL)aSystem;
- (BOOL)usesMetricSystem;
- (void)setFontSizeTag:(NSInteger)aTag;
- (NSInteger)fontSizeTag;
- (void)setHidesWFPaletteOnDeactivate:(BOOL)aDeactivate;
- (BOOL)hidesWFPaletteOnDeactivate;
- (void)setPaletteTransparencyLevel:(CGFloat)aLevel;
- (CGFloat)paletteTransparencyLevel;
- (void)setCloudsDetalizationLevel:(NSInteger)aLevel;
- (NSInteger)cloudsDetalizationLevel;
- (void)setInternetState:(BOOL)aState;
- (BOOL)internetState;
- (BOOL)openFullScreenByDefault;
- (void)setOpenFullScreenByDefault:(BOOL)aDefault;
- (void)setRetainLastView:(NSInteger)aView;
- (NSInteger)retainLastView;
@end

