#import "WeatherGlobePrefix.h"


@interface WGSerialNumberController : NSObject
{
    BOOL _nibWasLoaded;
    NSTextView *_securityTextView;
    NSWindow *_window;
    NSTextField *_serial;
    NSButton *_acceptButton;
}
+ (LString *)serialNumber;
+ (BOOL)runModal;
+ (id)_sharedController;
+ (void)_appWillTerminate;
- (BOOL)validateMenuItem:(id)aItem;
- (void)controlTextDidChange:(id)aChange;
- (id)windowWillReturnFieldEditor:(id)aEditor toObject:(id)aObject;
- (void)acceptAction:(id)aAction;
- (BOOL)_runModal;
- (LString *)_serialNumber;
- (void)_loadNibIfNeeded;
- (void)_appDidBecomeActive:(id)aActive;
- (void)dealloc;
- (void)awakeFromNib;
- (void)_setupCustomTitle;
- (void)_validateButtons;
@end

@interface SecurityTextView : NSTextView
{
}
- (BOOL)validateMenuItem:(id)aItem;
- (id)menuForEvent:(id)aEvent;
@end

