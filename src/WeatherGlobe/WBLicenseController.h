#import "WeatherGlobePrefix.h"


@interface WBLicenseController : NSObject
{
    NSWindow *_licensePanel;
    NSTextView *_licenseTextView;
    BOOL _isModal;
}
+ (id)sharedLicenseController;
- (void)printLicenseAgreement:(id)aAgreement;
- (void)saveLicenseAgreement:(id)aAgreement;
- (void)saveLicenseAgreementSheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo;
- (void)onDisagree:(id)aDisagree;
- (void)onAgree:(id)aAgree;
- (void)runLicense;
- (void)runModeless;
- (BOOL)isAgreed;
- (id)init;
@end

@interface LicenseWindow : NSWindow
{
}
- (BOOL)canBecomeKeyWindow;
- (id)initWithContentRect:(NSRect)aRect styleMask:(NSUInteger)aMask backing:(NSInteger)aBacking defer:(BOOL)aDefer;
@end

@interface LicenseBackgroundImageView : NSImageView
{
}
- (BOOL)mouseDownCanMoveWindow;
@end

@interface PrintableTextView : NSTextView
{
    NSString *_printJobTitle;
}
- (id)printJobTitle;
- (BOOL)isOpaque;
- (void)dealloc;
- (void)setPrintJobTitle:(id)aTitle;
- (NSSize)pageSize;
- (id)_printInfo;
@end

@interface NoMenuTextView : NSTextView
{
}
- (id)menuForEvent:(id)aEvent;
@end

