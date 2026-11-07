#import "IPhotoHelper.h"

NSString *_iPhotoRecentDatabasesKey = @"iPhotoRecentDatabases";
NSString *_iPhotoRecentDatabases1Key = @"iPhotoRecentDatabases1";
static NSString *const _shortVersionKey = @"CFBundleShortVersionString";

// The original's CFArrayApplyFunction callback: each string becomes a URL
// (exceptions ignored).
static void _ArrayConvertNSStringToNSURL(NSArray *strings, NSMutableArray *urls)
{
	for (id string in strings) {
		@try {
			NSURL *url = [NSURL URLWithString:string];
			if (url)
				[urls addObject:url];
		} @catch (NSException *exception) {
		}
	}
}

// The alias's target, or nil when the URL isn't an alias.
static NSURL *_resolvedAlias(NSURL *url)
{
	NSNumber *isAlias = nil;
	if (![url getResourceValue:&isAlias forKey:NSURLIsAliasFileKey error:nil] || ![isAlias boolValue])
		return nil;
	return [NSURL URLByResolvingAliasFileAtURL:url options:NSURLBookmarkResolutionWithoutUI error:nil];
}

@implementation IPhotoHelper

+ (id)iPhotoRecentDatabases
{
	return [self iAppsRecentDatabasesForKeys:[NSArray arrayWithObjects:_iPhotoRecentDatabasesKey, _iPhotoRecentDatabases1Key, nil]];
}

+ (CGFloat)iPhotoApplicationVersion
{
	NSString *bundleID = [[NSBundle mainBundle] localizedStringForKey:@"iPhotoBundleId" value:@"" table:nil];
	NSBundle *bundle = [self applicationBundleForBundleID:bundleID creator:'iPho'];
	return bundle ? [[[bundle infoDictionary] objectForKey:_shortVersionKey] floatValue] : 0;
}

+ (BOOL)iPhotoApplicationVersionSupported
{
	return [self iPhotoApplicationVersion] >= 1.1f;
}

// The URL with aliases resolved (FSResolveAliasFile in the original): the
// URL itself if it's an alias, else (when it doesn't exist) its folder's,
// with the last component resolved again. Nil when nothing was an alias.
+ (id)resolveAliasesInURL:(id)aURL
{
	if (!aURL)
		return nil;
	if ([aURL checkResourceIsReachableAndReturnError:nil])
		return _resolvedAlias(aURL);
	NSURL *parent = [aURL URLByDeletingLastPathComponent];
	NSURL *resolvedParent = parent ? [self resolveAliasesInURL:parent] : nil;
	if (!resolvedParent)
		return nil;
	NSURL *candidate = [resolvedParent URLByAppendingPathComponent:[aURL lastPathComponent]];
	NSURL *resolved = _resolvedAlias(candidate);
	return resolved ? resolved : candidate;
}

// The readable file URLs, aliases resolved.
+ (id)arrayByRemovingNonExistedPaths:(id)aPaths
{
	NSMutableArray *result = [NSMutableArray array];
	for (NSURL *url in aPaths) {
		NSURL *resolved = [self resolveAliasesInURL:url];
		if (!resolved)
			resolved = url;
		if ([resolved isFileURL] && [[NSFileManager defaultManager] isReadableFileAtPath:[resolved path]])
			[result addObject:resolved];
	}
	return result;
}

// LSFindApplicationForInfo in the original; creator codes are gone, so only
// the bundle id is used.
+ (id)applicationBundleForBundleID:(id)aID creator:(NSUInteger)aCreator
{
	NSURL *url = [aID length] ? [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:aID] : nil;
	if (![url isFileURL])
		return nil;
	return [NSBundle bundleWithPath:[[url absoluteURL] path]];
}

// The first key with existing databases in the iApps preferences; failing
// that, the first key whose localized default path exists.
+ (id)iAppsRecentDatabasesForKeys:(id)aKeys
{
	NSArray *databases = nil;
	NSString *domain = [[NSBundle mainBundle] localizedStringForKey:@"iAppsSignature" value:@"" table:nil];
	NSDictionary *preferences = [[NSUserDefaults standardUserDefaults] persistentDomainForName:domain];
	if (preferences) {
		for (NSString *key in aKeys) {
			NSArray *strings = [preferences objectForKey:key];
			if ([strings count]) {
				NSMutableArray *urls = [NSMutableArray arrayWithCapacity:0];
				_ArrayConvertNSStringToNSURL(strings, urls);
				databases = [self arrayByRemovingNonExistedPaths:urls];
			}
			if ([databases count])
				break;
		}
	}
	if ([databases count])
		return databases;
	for (NSString *key in aKeys) {
		NSString *path = [[[NSBundle mainBundle] localizedStringForKey:key value:@"" table:nil] stringByExpandingTildeInPath];
		NSURL *url = [NSURL fileURLWithPath:path];
		if (url)
			databases = [self arrayByRemovingNonExistedPaths:[NSArray arrayWithObject:url]];
		if ([databases count])
			break;
	}
	return databases;
}

@end
