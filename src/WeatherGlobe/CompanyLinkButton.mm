#import "CompanyLinkButton.h"

@implementation CompanyLinkButton

// The bundle's company link, else the title.
- (void)_preInit
{
	[super _preInit];
	if (!_link) {
		_link = [[[NSBundle mainBundle] companyLink] retain];
		if (!_link)
			_link = [[self title] retain];
	}
	[self setBundleValue];
}

- (void)dealloc
{
	[_link release];
	[super dealloc];
}

- (void)setBundleValue
{
	if (_link) {
		[self setTitle:_link];
		[self setAlternateTitle:_link];
		[self setURL:[NSURL URLWithString:[NSString stringWithFormat:@"http://%@", _link]]];
	}
}

@end
