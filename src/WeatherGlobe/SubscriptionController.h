#import "WeatherGlobePrefix.h"


@interface SubscriptionController : NSObject
{
    NSWindow *_window;
    NSWindow *_privacyStatementWindow;
    NSTextField *_firstNameField;
    NSTextField *_lastNameField;
    NSTextField *_emailField;
    NSPopUpButton *_countryPopUp;
    LMap *_regInfo;
    BOOL _regInfoIsValid;
    NSWindow *_sheet;
    SEL _sheetDidEndSelector;
}
+ (LMap *)registrationInfo;
+ (BOOL)runModal;
+ (id)_sharedController;
+ (void)_appWillTerminate;
- (void)incompleteRequiredInformationSheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo;
- (void)incompleteEmailSheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo;
- (void)privacyStatementSheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo;
- (void)showHelp:(id)aHelp;
- (void)privacyStatementWindowOK:(id)aOK;
- (void)privacyStatement:(id)aStatement;
- (void)doNotSubscribe:(id)aSubscribe;
- (void)subscribeNow:(id)aNow;
- (LMap *)_registrationInfo;
- (BOOL)_runModal;
- (void)dealloc;
- (void)awakeFromNib;
- (void)_updateCountryPopUpList;
- (void)_loadNibIfNeeded;
- (void)_releaseSheet;
- (void)_setUserInformation;
- (void)_collectUserInformation;
- (LMap *)_addressBookInfo;
@end

@interface SubsribtionTextFormatter : NSFormatter
{
    unsigned int _lastLength;
    unsigned int _maxLength;
}
+ (id)formatterWithMaxLength:(NSUInteger)aLength;
- (BOOL)isPartialStringValid:(id)aValid newEditingString:(id *)aString errorDescription:(id *)aDescription;
- (BOOL)getObjectValue:(id *)aValue forString:(id)aString errorDescription:(id *)aDescription;
- (id)stringForObjectValue:(id)aValue;
- (id)initWithMaxLength:(NSUInteger)aLength;
- (id)init;
@end

