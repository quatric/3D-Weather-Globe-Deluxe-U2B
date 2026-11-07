#import "WGSerialNumberController.h"
#import "WGCustomTitleView.h"

// The shared controller; (id)-1 once the app terminated.
static id _sSerialNumberController = nil;

// The serial number dialog: a modal window whose field uses a
// SecurityTextView field editor; serials are cut to 50 characters.
@implementation WGSerialNumberController

- (void)_validateButtons
{
	[_acceptButton setEnabled:[[_serial stringValue] length] != 0];
}

- (void)_setupCustomTitle
{
	WGCustomTitleView *title = [_window customTitleView];
	[title setImage:[NSImage imageNamed:@"MW_title_left"] forTexture:0];
	[title setImage:[NSImage imageNamed:@"MW_title_center"] forTexture:1];
	[title setImage:[NSImage imageNamed:@"MW_title_right"] forTexture:2];
	[title setTitleShadowColor:[[NSColor blackColor] colorWithAlphaComponent:0.6]];
	[title setTitleShadowOffset:NSMakeSize(0.0, 1.0)];
	[title setDrawsTitle:YES];
}

- (void)awakeFromNib
{
	[_window setBackgroundColor:[NSColor colorWithDeviceWhite:0.93 alpha:1.0]];
	[self _setupCustomTitle];
	[self _validateButtons];
	[_window center];
	[_window setExcludedFromWindowsMenu:YES];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_appDidBecomeActive:)
		name:NSApplicationDidBecomeActiveNotification object:nil];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_window release];
	[_securityTextView release];
	[super dealloc];
}

- (void)_appDidBecomeActive:(id)aActive
{
	if ([NSApp modalWindow] != _window)
		[_window setLevel:CGWindowLevelForKey(kCGNormalWindowLevelKey)];
}

- (void)_loadNibIfNeeded
{
	if (!_nibWasLoaded && [NSBundle loadNibNamed:@"SerialNumberDialog" owner:self])
		_nibWasLoaded = YES;
}

- (LString *)_serialNumber
{
	return [[_serial stringValue] LString];
}

- (BOOL)_runModal
{
	[self _loadNibIfNeeded];
	[NSApp runModalForWindow:_window];
	[_window orderOut:self];
	return YES;
}

- (void)acceptAction:(id)aAction
{
	[NSApp stopModal];
}

- (id)windowWillReturnFieldEditor:(id)aEditor toObject:(id)aObject
{
	if (_serial != aObject)
		return nil;
	if (!_securityTextView) {
		_securityTextView = [[SecurityTextView alloc] init];
		[_securityTextView setFieldEditor:YES];
	}
	return _securityTextView;
}

- (void)controlTextDidChange:(id)aChange
{
	NSString *serial = [_serial stringValue];
	if ([serial length] > 50)
		[_serial setStringValue:[serial substringToIndex:50]];
	[self _validateButtons];
}

- (BOOL)validateMenuItem:(id)aItem
{
	return NO;
}

+ (void)_appWillTerminate
{
	if (_sSerialNumberController && _sSerialNumberController != (id)-1) {
		[_sSerialNumberController release];
		_sSerialNumberController = (id)-1;
	}
}

+ (id)_sharedController
{
	if (!_sSerialNumberController) {
		_sSerialNumberController = [WGSerialNumberController new];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_appWillTerminate)
			name:NSApplicationWillTerminateNotification object:nil];
	}
	return _sSerialNumberController == (id)-1 ? nil : _sSerialNumberController;
}

+ (BOOL)runModal
{
	return [[self _sharedController] _runModal];
}

+ (LString *)serialNumber
{
	return [[self _sharedController] _serialNumber];
}

@end
