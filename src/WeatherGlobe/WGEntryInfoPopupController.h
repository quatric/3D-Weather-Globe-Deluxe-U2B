#import "WeatherGlobePrefix.h"


@interface WGEntryInfoPopupController : NSObject
{
    NSPanel *_palette;
    NSTimer *_fadeOutTimer;
    double _fadeOutStartTime;
    EntryInfoController *_entryInfoController;
    WGEntryInfoPopupView *_entryInfoView;
}
+ (unsigned char)isVisible;
+ (void)hidePopupPalette;
+ (void)showPopupPaletteForEntry:(GenericEntry *)aEntry;
+ (id *)sharedInstancePtr;
- (unsigned char)_isVisible;
- (void)_showPopupPaletteForEntry:(GenericEntry *)aEntry;
- (void)_hidePopupPalette;
- (void)_onFadeOutTimer:(id)aTimer;
- (void)_hideImmediately;
- (id)_userDescription;
- (id)_entryInfo;
- (void)_createPaletteIfNeeded;
- (void)invalidate;
- (void)dealloc;
@end

