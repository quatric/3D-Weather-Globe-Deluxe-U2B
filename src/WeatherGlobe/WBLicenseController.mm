#import "WBLicenseController.h"

static NSString *const _WBLicenseAgreeKey = @"LicenseAgreed";
static WBLicenseController *_sController = nil;

// The license agreement panel, shown modally until the user agrees.
@implementation WBLicenseController

+ (id)sharedLicenseController
{
	if (!_sController)
		_sController = [[WBLicenseController alloc] init];
	return _sController;
}

- (id)init
{
	if (_sController) {
		[super dealloc];
		return _sController;
	}
	return [super init];
}

- (BOOL)isAgreed
{
	return [[NSUserDefaults standardUserDefaults] boolForKey:_WBLicenseAgreeKey];
}

- (void)runModeless
{
	_isModal = NO;
	if (!_licensePanel) {
		[NSBundle loadNibNamed:@"LicensePanel" owner:self];
		[_licenseTextView readRTFDFromFile:[[NSBundle mainBundle] pathForResource:@"LicenseAgreement" ofType:@"rtf"]];
		[_licenseTextView setDrawsBackground:NO];
		[[_licenseTextView enclosingScrollView] setDrawsBackground:NO];
	}
	[_licensePanel center];
	[_licensePanel orderFront:self];
}

- (void)runLicense
{
	[self runModeless];
	_isModal = YES;
	[NSApp runModalForWindow:_licensePanel];
}

- (void)onAgree:(id)aAgree
{
	[[NSUserDefaults standardUserDefaults] setBool:YES forKey:_WBLicenseAgreeKey];
	if (_isModal)
		[NSApp stopModalWithCode:NSModalResponseOK];
	[_licensePanel orderOut:aAgree];
}

- (void)onDisagree:(id)aDisagree
{
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:_WBLicenseAgreeKey];
	[NSApp terminate:aDisagree];
}

- (void)saveLicenseAgreementSheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo
{
	if (aCode != NSModalResponseOK)
		return;
	NSData *rtf = [_licenseTextView RTFFromRange:NSMakeRange(0, [[_licenseTextView textStorage] length])];
	[rtf writeToFile:[[aEnd URL] path] atomically:YES];
}

// Saves the text as RTF, by default to the desktop under the panel's title.
- (void)saveLicenseAgreement:(id)aAgreement
{
	NSSavePanel *panel = [NSSavePanel savePanel];
	[panel setAllowedFileTypes:[NSArray arrayWithObject:@"rtf"]];
	NSArray *desktops = NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES);
	NSString *directory = [desktops count] ? [desktops objectAtIndex:0] : NSHomeDirectory();
	[panel setDirectoryURL:[NSURL fileURLWithPath:directory isDirectory:YES]];
	[panel setNameFieldStringValue:[_licensePanel title]];
	[panel beginSheetModalForWindow:_licensePanel completionHandler:^(NSModalResponse result) {
		[self saveLicenseAgreementSheetDidEnd:panel returnCode:result contextInfo:NULL];
	}];
}

- (void)printLicenseAgreement:(id)aAgreement
{
	PrintableTextView *view = [PrintableTextView new];
	[view setFrameSize:[view pageSize]];
	[view setPrintJobTitle:[_licensePanel title]];
	[[view layoutManager] replaceTextStorage:[_licenseTextView textStorage]];
	[[NSPrintInfo sharedPrintInfo] setVerticallyCentered:NO];
	NSPrintOperation *operation = [NSPrintOperation printOperationWithView:[view autorelease]];
	[operation runOperationModalForWindow:_licensePanel delegate:nil didRunSelector:NULL contextInfo:NULL];
}

@end
