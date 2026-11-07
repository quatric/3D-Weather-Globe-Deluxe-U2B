#import "WGEntryDetailsKeyValueURLDataProvider.h"
#import "CompanyLinkButton.h"

@implementation WGEntryDetailsKeyValueURLDataProvider

// The link shows its title (else the URL) and goes to the URL (http://
// when it has no scheme).
- (void)updateDescription
{
	LinkButton *button = (LinkButton *)_descriptionView;
	NSString *title = [self urlTitle];
	NSString *url = [self urlValue];
	[button setTitle:[title length] ? title : url];
	[button setAlternateTitle:[title length] ? title : url];
	if ([url rangeOfString:@"://"].location == NSNotFound)
		url = [@"http://" stringByAppendingString:url];
	[button setToolTip:url];
	[button setURL:[NSURL URLWithString:url]];
}

- (id)descriptionView
{
	if (!_descriptionView) {
		NSFont *font = [NSFont systemFontOfSize:11.0];
		LinkButton *button = [[LinkButton alloc] initWithFrame:NSMakeRect(0, 0, WGEntryDetailsInfoDefaultFieldSize.width, 14.0)];
		[button setBordered:NO];
		[button setFont:font];
		[button setAlignment:NSTextAlignmentLeft];
		[button setButtonType:NSButtonTypeMomentaryChange];
		if ([button respondsToSelector:@selector(setLineBreakMode:)])
			[button setLineBreakMode:NSLineBreakByTruncatingTail];
		_descriptionView = button;
		[self updateDescription];
	}
	return _descriptionView;
}

- (id)urlTitle
{
	LString *title = 0;
	EntryInfoController::extractURLParamsFromObject([self dataObject], &title, 0);
	return title ? [NSString stringReferenceFromLString:title] : @"";
}

- (id)urlValue
{
	LString *url = 0;
	EntryInfoController::extractURLParamsFromObject([self dataObject], 0, &url);
	return url ? [NSString stringReferenceFromLString:url] : @"";
}

@end
