#import "PrefScreenSaverController.h"

// The screen saver preferences: a tips sheet and a button to the system's
// screen saver settings.
@implementation PrefScreenSaverController

- (void)dealloc
{
	[_tipsSheet release];
	[super dealloc];
}

- (id)_nibName
{
	return @"PrefScreenSaver";
}

- (id)toolbarLabel
{
	return [[NSBundle mainBundle] localizedStringForKey:@"ScreenSaverPrefLabel" value:@"" table:nil];
}

- (id)toolbarImage
{
	return [NSImage imageNamed:@"pref_scrsaver"];
}

// The tips come from ScreenSaverTips.rtf (loaded once).
- (void)_showTips
{
	if (!_tipsSet) {
		NSString *path = [[NSBundle bundleForClass:[self class]] pathForResource:@"ScreenSaverTips" ofType:@"rtf"];
		NSData *data = [[NSData alloc] initWithContentsOfFile:path];
		if (data) {
			NSAttributedString *tips = [[NSAttributedString alloc] initWithRTF:data documentAttributes:nil];
			if (tips) {
				[_tips setAttributedStringValue:tips];
				_tipsSet = YES;
			}
			[tips release];
			[data release];
		}
		if (!_tipsSet)
			return;
	}
	[[[self mainView] window] beginSheet:_tipsSheet completionHandler:^(NSModalResponse returnCode) {
		[self sheetDidEnd:_tipsSheet returnCode:returnCode contextInfo:0];
	}];
}

- (void)sheetDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo
{
	[_tipsSheet orderOut:nil];
}

- (void)_closeTipsSheet
{
	[[[self mainView] window] endSheet:_tipsSheet returnCode:1];
}

// 100 shows the tips, 300 closes them, 200 opens the system settings (the
// original tried ScreenSaver.prefPane, then the older
// DesktopScreenEffectsPref.prefPane).
- (void)controlsAction:(id)aAction
{
	switch ([aAction tag]) {
	case 100:
		[self _showTips];
		break;
	case 300:
		[self _closeTipsSheet];
		break;
	case 200: {
		NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
		if (![workspace openFile:@"/System/Library/PreferencePanes/ScreenSaver.prefPane"]
		    && ![workspace openFile:@"/System/Library/PreferencePanes/DesktopScreenEffectsPref.prefPane"])
			[workspace openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.ScreenSaver-Settings.extension"]];
		break;
	}
	}
}

@end
