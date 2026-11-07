#import "CompanyLinkButton.h"

// Info.plist values, localized first.
static id _infoValue(NSBundle *bundle, NSString *key)
{
	id value = [[bundle localizedInfoDictionary] objectForKey:key];
	return value ? value : [[bundle infoDictionary] objectForKey:key];
}

static id _labeledInfoValue(NSBundle *bundle, NSString *key, NSString *label)
{
	id value = _infoValue(bundle, key);
	if (!value)
		return nil;
	return [NSString stringWithFormat:@"%@ %@", [[NSBundle mainBundle] localizedStringForKey:label value:@"" table:nil], value];
}

@implementation NSBundle (BundleInfoStrings)

- (id)bundleName
{
	id name = _infoValue(self, @"CFBundleDisplayName");
	return name ? name : _infoValue(self, @"CFBundleName");
}

- (id)versionString
{
	return _labeledInfoValue(self, @"CFBundleShortVersionString", @"Version");
}

- (id)buildString
{
	return _labeledInfoValue(self, @"CFBundleVersion", @"Build");
}

- (id)copyrightString
{
	return _infoValue(self, @"NSHumanReadableCopyright");
}

- (id)companyLink
{
	return _infoValue(self, @"CompanyLink");
}

@end
