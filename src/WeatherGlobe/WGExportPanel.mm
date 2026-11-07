#import "WGExportPanel.h"
#import "WGMovieController.h"

// The panel's default button.
static NSButton *_defaultButtonIn(NSView *view)
{
	if ([view isKindOfClass:[NSButton class]] && [[(NSButton *)view keyEquivalent] isEqualToString:@"\r"])
		return (NSButton *)view;
	for (NSView *subview in [view subviews]) {
		NSButton *button = _defaultButtonIn(subview);
		if (button)
			return button;
	}
	return nil;
}

@implementation WGExportPanel

// NSSavePanel's private _okButton; found by its key equivalent now.
- (id)okButton
{
	return _defaultButtonIn([[self contentView] superview] ? [[self contentView] superview] : [self contentView]);
}

// The delegate can refuse the export.
- (void)ok:(id)aOk
{
	id delegate = [self delegate];
	if ([delegate respondsToSelector:@selector(wgShouldDoExportInWGExportPanel:)] &&
	    ![delegate wgShouldDoExportInWGExportPanel:self])
		return;
	[super ok:aOk];
}

@end
