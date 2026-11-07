#import "SubscriptionController.h"
#import "WGHelpBook.h"
#import <AddressBook/AddressBook.h>
#include "AMDatabase.h"

// The shared controller; -1 once the application terminates.
static SubscriptionController *_sSubscriptionController = nil;
#define kSubscriptionControllerGone ((SubscriptionController *)-1)

// The weather subscription dialog: the user's name, e-mail and country
// (from the stored registration, else the address book's "me" card),
// checked before the dialog closes.
@implementation SubscriptionController

- (void)_releaseSheet
{
	if (_sheet)
		NSReleaseAlertPanel(_sheet);
	_sheet = nil;
	_sheetDidEndSelector = 0;
}

- (void)_loadNibIfNeeded
{
	if (_window)
		return;
	LLog("SubscriptionController load nib\n");
	[NSBundle loadNibNamed:@"Subscribe" owner:self];
}

- (void)awakeFromNib
{
	LLog("SubscriptionController nib was loaded\n");
	[_firstNameField setFormatter:[SubsribtionTextFormatter formatterWithMaxLength:250]];
	[_lastNameField setFormatter:[SubsribtionTextFormatter formatterWithMaxLength:250]];
	_regInfo = Preferences::registrationInfo();
	if (!_regInfo)
		_regInfo = [self _addressBookInfo];
	if (_regInfo)
		_regInfo->retain();
	[_window setExcludedFromWindowsMenu:YES];
}

- (void)dealloc
{
	if (_sheet)
		NSReleaseAlertPanel(_sheet);
	[_privacyStatementWindow release];
	[_window release];
	if (_regInfo) {
		_regInfo->release();
		_regInfo = 0;
	}
	[super dealloc];
}

// The information entered is stored when the user subscribes.
- (BOOL)_runModal
{
	[self _loadNibIfNeeded];
	[self _updateCountryPopUpList];
	[self _setUserInformation];
	[NSApp activateIgnoringOtherApps:YES];
	[_window center];
	NSModalResponse response = [NSApp runModalForWindow:_window];
	[_window orderOut:self];
	_regInfoIsValid = response == 1;
	if (_regInfoIsValid)
		Preferences::storeRegistrationInfo(_regInfo);
	return _regInfoIsValid;
}

- (LMap *)_registrationInfo
{
	return _regInfoIsValid ? _regInfo : 0;
}

// Something before the @, one @. The local part takes letters, digits and !#$%'*+-./=?^_`~&, the domain
// letters, digits, dots and hyphens, with a dot inside and none doubled.
static BOOL _isValidEmail(NSString *email)
{
	if (![email canBeConvertedToEncoding:NSASCIIStringEncoding])
		return NO;
	NSRange at = [email rangeOfString:@"@"];
	if (at.length != 1 || at.location == 0)
		return NO;
	if ([email rangeOfString:@"@" options:NSBackwardsSearch].location != at.location)
		return NO;
	if (at.location == [email length] - 1)
		return NO;
	NSString *local = [email substringToIndex:at.location];
	NSString *domain = [email substringFromIndex:at.location + 1];
	NSMutableCharacterSet *localChars = [NSMutableCharacterSet alphanumericCharacterSet];
	[localChars addCharactersInString:@"!#$%'*+-./=?^_`~&"];
	NSMutableCharacterSet *domainChars = [NSMutableCharacterSet alphanumericCharacterSet];
	[domainChars addCharactersInString:@".-"];
	if (!local || !domain)
		return NO;
	if ([local rangeOfCharacterFromSet:[localChars invertedSet]].location != NSNotFound)
		return NO;
	if ([domain rangeOfCharacterFromSet:[domainChars invertedSet]].location != NSNotFound)
		return NO;
	if ([domain hasSuffix:@"."] || [domain hasPrefix:@"."])
		return NO;
	if ([domain rangeOfString:@".."].location != NSNotFound)
		return NO;
	return [domain rangeOfString:@"."].location != NSNotFound;
}

// All the fields are required (the first empty one gets the focus after
// the alert), and the e-mail address must look complete.
- (void)subscribeNow:(id)aNow
{
	NSString *email = [_emailField stringValue];
	id empty = nil;
	if (![_countryPopUp indexOfSelectedItem])
		empty = _countryPopUp;
	if (![email length])
		empty = _emailField;
	if (![[_lastNameField stringValue] length])
		empty = _lastNameField;
	if (![[_firstNameField stringValue] length])
		empty = _firstNameField;
	NSBundle *bundle = [NSBundle mainBundle];
	NSString *ok = [bundle localizedStringForKey:@"OK" value:@"" table:nil];
	if (!empty) {
		if (_isValidEmail(email)) {
			[self _collectUserInformation];
			[NSApp stopModalWithCode:1];
			return;
		}
		_sheet = NSGetAlertPanel([bundle localizedStringForKey:@"Email address not complete" value:@"" table:nil], @"%@",
			ok, nil, nil, [bundle localizedStringForKey:@"Email address not complete..." value:@"" table:nil]);
		_sheetDidEndSelector = @selector(incompleteEmailSheetDidEnd:returnCode:contextInfo:);
	} else {
		_sheet = NSGetAlertPanel([bundle localizedStringForKey:@"Required information incomplete" value:@"" table:nil], @"%@",
			ok, nil, nil, [bundle localizedStringForKey:@"Required information incomplete..." value:@"" table:nil]);
		_sheetDidEndSelector = @selector(incompleteRequiredInformationSheetDidEnd:returnCode:contextInfo:);
	}
	[_sheet setExcludedFromWindowsMenu:YES];
	NSWindow *sheet = _sheet;
	SEL didEnd = _sheetDidEndSelector;
	[_window beginSheet:sheet completionHandler:^(NSModalResponse returnCode) {
		if (didEnd == @selector(incompleteEmailSheetDidEnd:returnCode:contextInfo:))
			[self incompleteEmailSheetDidEnd:sheet returnCode:returnCode contextInfo:0];
		else
			[self incompleteRequiredInformationSheetDidEnd:sheet returnCode:returnCode contextInfo:empty];
	}];
}

- (void)doNotSubscribe:(id)aSubscribe
{
	[NSApp stopModalWithCode:0];
}

- (void)privacyStatement:(id)aStatement
{
	_sheet = _privacyStatementWindow;
	_sheetDidEndSelector = @selector(privacyStatementSheetDidEnd:returnCode:contextInfo:);
	NSWindow *sheet = _privacyStatementWindow;
	[_window beginSheet:sheet completionHandler:^(NSModalResponse returnCode) {
		[self privacyStatementSheetDidEnd:sheet returnCode:returnCode contextInfo:0];
	}];
}

- (void)privacyStatementWindowOK:(id)aOK
{
	[[_sheet sheetParent] endSheet:_sheet returnCode:1];
}

- (void)showHelp:(id)aHelp
{
	[WGHelpBook gotoPage:@"topics/weather.html" anchor:nil];
}

- (void)privacyStatementSheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo
{
	[aEnd orderOut:self];
	if (aCode == 1) {
		_sheet = nil;
		_sheetDidEndSelector = 0;
	}
}

- (void)incompleteEmailSheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo
{
	[aEnd orderOut:self];
	if (aCode == 1)
		[self _releaseSheet];
}

// The empty field gets the focus.
- (void)incompleteRequiredInformationSheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo
{
	[aEnd orderOut:self];
	if (aCode != 1)
		return;
	[self _releaseSheet];
	id field = (id)aInfo;
	if (field && [field isKindOfClass:[NSResponder class]])
		[_window makeFirstResponder:field];
}

+ (void)_appWillTerminate
{
	if (_sSubscriptionController && _sSubscriptionController != kSubscriptionControllerGone) {
		[_sSubscriptionController release];
		_sSubscriptionController = kSubscriptionControllerGone;
	}
}

+ (id)_sharedController
{
	if (!_sSubscriptionController) {
		_sSubscriptionController = [SubscriptionController new];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_appWillTerminate)
			name:NSApplicationWillTerminateNotification object:nil];
	}
	return _sSubscriptionController == kSubscriptionControllerGone ? nil : _sSubscriptionController;
}

+ (BOOL)runModal
{
	return [[self _sharedController] _runModal];
}

+ (LMap *)registrationInfo
{
	return [[self _sharedController] _registrationInfo];
}

// "--Select Your Country--", the database's countries, "Other".
- (void)_updateCountryPopUpList
{
	LArray *countries = AtlasDatabase::instance()->simpleSearchWithRequest(SQL_DB_Strings::selectAllCountriesNames());
	unsigned int count = countries ? countries->count() : 0;
	NSMutableArray *titles = [NSMutableArray arrayWithCapacity:count];
	for (unsigned int i = 0; i < count; i++) {
		LString *name = lobj_cast<LString>(static_cast<LMap *>(countries->objectAtIndex(i))->objectForKey(kColumnNameName));
		if (name)
			[titles addObject:[NSString stringReferenceFromLString:name]];
	}
	[_countryPopUp removeAllItems];
	[_countryPopUp addItemWithTitle:[[NSBundle mainBundle] localizedStringForKey:@"--Select Your Country--" value:@"" table:nil]];
	[_countryPopUp addItemsWithTitles:titles];
	[_countryPopUp addItemWithTitle:[[NSBundle mainBundle] localizedStringForKey:@"Other" value:@"" table:nil]];
}

// A field without a stored value is cleared.
static void _setField(NSTextField *field, LMap *info, LString *key)
{
	LString *value = lobj_cast<LString>(info->objectForKey(key));
	if (value)
		[field setObjectValue:(NSString *)value->cfString()];
	else if ([[field stringValue] length])
		[field setObjectValue:@""];
}

- (void)_setUserInformation
{
	if (!_regInfo)
		return;
	_setField(_firstNameField, _regInfo, IModelCallbacks::kFirstNameKey);
	_setField(_lastNameField, _regInfo, IModelCallbacks::kLastNameKey);
	_setField(_emailField, _regInfo, IModelCallbacks::kEmailKey);
	LString *country = lobj_cast<LString>(_regInfo->objectForKey(IModelCallbacks::kCountryKey));
	if (country) {
		[_countryPopUp selectItemWithTitle:(NSString *)country->cfString()];
		if ([_countryPopUp indexOfSelectedItem] >= 0)
			return;
	} else if (![_countryPopUp selectedItem]) {
		return;
	}
	[_countryPopUp selectItemAtIndex:0];
}

// The "me" card's names, primary e-mail and primary address's country.
- (LMap *)_addressBookInfo
{
	LMap *info = LMap::map();
	ABPerson *me = [[ABAddressBook sharedAddressBook] me];
	NSString *value = [me valueForProperty:kABFirstNameProperty];
	if (value)
		info->setObjectForKey(LString::stringWithCFString((CFStringRef)value), IModelCallbacks::kFirstNameKey);
	value = [me valueForProperty:kABLastNameProperty];
	if (value)
		info->setObjectForKey(LString::stringWithCFString((CFStringRef)value), IModelCallbacks::kLastNameKey);
	ABMultiValue *emails = [me valueForProperty:kABEmailProperty];
	value = [emails valueForIdentifier:[emails primaryIdentifier]];
	if (value)
		info->setObjectForKey(LString::stringWithCFString((CFStringRef)value), IModelCallbacks::kEmailKey);
	ABMultiValue *addresses = [me valueForProperty:kABAddressProperty];
	value = [[addresses valueForIdentifier:[addresses primaryIdentifier]] objectForKey:kABAddressCountryKey];
	if (value)
		info->setObjectForKey(LString::stringWithCFString((CFStringRef)value), IModelCallbacks::kCountryKey);
	return info;
}

- (void)_collectUserInformation
{
	if (!_regInfo)
		_regInfo = LMap::_alloc()->init();
	_regInfo->setObjectForKey(LString::stringWithCFString((CFStringRef)[_firstNameField stringValue]), IModelCallbacks::kFirstNameKey);
	_regInfo->setObjectForKey(LString::stringWithCFString((CFStringRef)[_lastNameField stringValue]), IModelCallbacks::kLastNameKey);
	_regInfo->setObjectForKey(LString::stringWithCFString((CFStringRef)[_emailField stringValue]), IModelCallbacks::kEmailKey);
	if ([_countryPopUp indexOfSelectedItem] > 0)
		_regInfo->setObjectForKey(LString::stringWithCFString((CFStringRef)[[_countryPopUp selectedItem] title]),
			IModelCallbacks::kCountryKey);
}

@end
