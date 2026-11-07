#import "WGPreferencesWrapper.h"

// The preferences as key-value properties, for the bindings of the
// preferences window.
@implementation WGPreferencesWrapper

+ (id)sharedInstance
{
	static id sharedInstance = nil;
	if (!sharedInstance)
		sharedInstance = [[self class] new];
	return sharedInstance;
}

- (NSInteger)retainLastView
{
	return Preferences::retainLastView() & 0xff;
}

- (void)setRetainLastView:(NSInteger)aView
{
	Preferences::setRetainLastView((unsigned char)aView);
}

- (void)setOpenFullScreenByDefault:(BOOL)aDefault
{
	Preferences::setOpenFullScreenByDefault(aDefault);
}

- (BOOL)openFullScreenByDefault
{
	return Preferences::openFullScreenByDefault();
}

- (BOOL)internetState
{
	return Preferences::internetState() == 1;
}

- (void)setInternetState:(BOOL)aState
{
	Preferences::setInternetState((TInternetState)(aState != NO));
}

- (NSInteger)cloudsDetalizationLevel
{
	return Preferences::cloudsDetalizationLevel();
}

- (void)setCloudsDetalizationLevel:(NSInteger)aLevel
{
	Preferences::setCloudsDetalizationLevel((TCloudsDetalizationLevel)aLevel);
}

- (CGFloat)paletteTransparencyLevel
{
	return Preferences::paletteTransparencyLevel();
}

- (void)setPaletteTransparencyLevel:(CGFloat)aLevel
{
	Preferences::setPaletteTransparencyLevel(aLevel);
}

- (BOOL)hidesWFPaletteOnDeactivate
{
	return Preferences::hidesWFPaletteOnDeactivate();
}

- (void)setHidesWFPaletteOnDeactivate:(BOOL)aDeactivate
{
	Preferences::setHidesWFPaletteOnDeactivate(aDeactivate);
}

// The font size popup's tags are the sizes.
- (NSInteger)fontSizeTag
{
	return (int)Preferences::paletteFontSize();
}

- (void)setFontSizeTag:(NSInteger)aTag
{
	Preferences::setPaletteFontSize((float)aTag);
}

- (BOOL)usesMetricSystem
{
	return Preferences::usesMetricSystem();
}

- (void)setUsesMetricSystem:(BOOL)aSystem
{
	Preferences::useMetricSystem(aSystem != NO);
}

- (BOOL)uses24HourClock
{
	return Preferences::uses24HourClock();
}

- (void)setUses24HourClock:(BOOL)aClock
{
	Preferences::use24HourClock(aClock != NO);
}

- (BOOL)isSoundsAvailable
{
	return Preferences::isSoundsAvailable();
}

- (BOOL)playSounds
{
	return [self isSoundsAvailable] && Preferences::playSounds();
}

- (void)setPlaySounds:(BOOL)aSounds
{
	Preferences::setPlaySounds(aSounds);
}

- (CGFloat)soundVolume
{
	return Preferences::soundVolume();
}

- (void)setSoundVolume:(CGFloat)aVolume
{
	Preferences::setSoundVolume(aVolume);
}

- (BOOL)doShowWeatherInDock
{
	return Preferences::doShowWeatherInDock();
}

- (void)setDoShowWeatherInDock:(BOOL)aDock
{
	Preferences::setShowWeatherInDock(aDock != NO);
}

- (BOOL)usesSoftwareUpdating
{
	return Preferences::usesSoftwareUpdating();
}

- (void)setUsesSoftwareUpdating:(BOOL)aUpdating
{
	Preferences::useSoftwareUpdating(aUpdating != NO);
}

- (BOOL)animatedWaterEnabled
{
	return Preferences::animatedWaterEnabled();
}

- (void)setAnimatedWaterEnabled:(BOOL)aEnabled
{
	Preferences::setAnimatedWaterEnabled(aEnabled);
}

- (BOOL)animatedWaterUsesLargeTexture
{
	return Preferences::animatedWaterUsesLargeTexture();
}

- (void)setAnimatedWaterUsesLargeTexture:(BOOL)aTexture
{
	Preferences::setAnimatedWaterUseLargeTexture(aTexture);
}

- (BOOL)drawWithReliefEnabled
{
	return Preferences::drawWithReliefEnabled();
}

- (void)setDrawWithReliefEnabled:(BOOL)aEnabled
{
	Preferences::setDrawWithReliefEnabled(aEnabled);
}

- (CGFloat)terrainDetailsLevel
{
	return Preferences::terrainDetailsLevel();
}

- (void)setTerrainDetailsLevel:(CGFloat)aLevel
{
	Preferences::setTerrainDetailsLevel(aLevel);
}

@end
