#import "CompanyLinkButton.h"

// Shows the version; a click switches between the version and the build.
@implementation VersionTextField

- (void)_preInit
{
	[self setEditable:NO];
	if (!_version) {
		_version = [[[NSBundle mainBundle] versionString] retain];
		if (!_version)
			_version = [[self stringValue] retain];
	}
	if (!_build) {
		_build = [[[NSBundle mainBundle] buildString] retain];
		if (!_build)
			_build = [[self stringValue] retain];
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
	[_version release];
	[_build release];
	[super dealloc];
}

- (void)mouseDown:(id)aDown
{
	[self setStringValue:[[self stringValue] isEqualToString:_version] ? _build : _version];
}

- (void)setBundleValue
{
	if (_version)
		[self setStringValue:_version];
}

@end
