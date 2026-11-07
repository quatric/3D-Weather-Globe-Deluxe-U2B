#import "CompanyLinkButton.h"

@implementation CopyrightTextField

// The bundle's copyright, else the nib's text.
- (void)_preInit
{
	if (!_copyright) {
		_copyright = [[[NSBundle mainBundle] copyrightString] retain];
		if (!_copyright)
			_copyright = [[self stringValue] retain];
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

- (void)setBundleValue
{
	if (_copyright)
		[self setStringValue:_copyright];
}

@end
