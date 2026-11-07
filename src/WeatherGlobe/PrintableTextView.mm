#import "WBLicenseController.h"

@implementation PrintableTextView

// The shared print info with margins of at least 30 points outside the
// imageable area.
- (id)_printInfo
{
	NSPrintInfo *info = [NSPrintInfo sharedPrintInfo];
	NSRect bounds = [info imageablePageBounds];
	NSSize paper = [info paperSize];
	[info setLeftMargin:MAX(30.0, bounds.origin.x)];
	[info setBottomMargin:MAX(30.0, bounds.origin.y)];
	[info setRightMargin:MAX(30.0, paper.width - (bounds.origin.x + bounds.size.width))];
	[info setTopMargin:MAX(30.0, paper.height - (bounds.origin.y + bounds.size.height))];
	return info;
}

- (NSSize)pageSize
{
	NSPrintInfo *info = [self _printInfo];
	NSSize paper = [info paperSize];
	return NSMakeSize(floor(paper.width - ([info leftMargin] + [info rightMargin])),
	                  floor(paper.height - ([info topMargin] + [info bottomMargin])));
}

- (void)setPrintJobTitle:(id)aTitle
{
	if (_printJobTitle != aTitle) {
		[_printJobTitle release];
		_printJobTitle = [aTitle retain];
	}
}

- (void)dealloc
{
	[_printJobTitle release];
	[super dealloc];
}

- (BOOL)isOpaque
{
	return YES;
}

- (id)printJobTitle
{
	return _printJobTitle;
}

@end
