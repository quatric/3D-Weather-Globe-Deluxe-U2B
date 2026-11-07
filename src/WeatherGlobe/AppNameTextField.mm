#import "CompanyLinkButton.h"

@implementation AppNameTextField

// Shows the bundle's name; the nib's text is the fallback.
- (void)_preInit
{
	if (!_appName) {
		_appName = [[[NSBundle mainBundle] bundleName] retain];
		if (!_appName)
			_appName = [[self stringValue] retain];
	}
	[self setBundleValue];
}

- (id)init
{
	self = [super init];
	if (self)
		[self _preInit];
	return self;
}

- (void)awakeFromNib
{
	[self _preInit];
}

- (void)dealloc
{
	[_appName release];
	[super dealloc];
}

- (void)setBundleValue
{
	if (_appName)
		[self setStringValue:_appName];
}

@end
