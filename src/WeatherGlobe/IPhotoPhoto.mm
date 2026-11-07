#import "IPhotoPhoto.h"
#import "IPhotoHelper.h"

NSString *_ipSelectedAlbumIndexKey = @"iPhoto Import Palette Selected Album Index";
NSString *_ipMajorVersionKey = @"Major Version";
NSString *_ipSongPathKey = @"SongPath";
NSString *_ipPhotosListKey = @"Master Image List";
NSString *_ipAlbumsListKey = @"List of Albums";
NSString *_ipArchivePathKey = @"Archive Path";
NSString *_ipPhotoNameKey = @"Caption";
NSString *_ipThumbPathKey = @"ThumbPath";
NSString *_ipPhotoPathKey = @"ImagePath";
NSString *_ipAlbumKeysListKey = @"KeyList";
NSString *_ipAlbumPhotosKey = @"PhotoList";
NSString *_ipMasterKey = @"Master";
NSString *_ipAlbumIDKey = @"AlbumId";
NSString *_ipAlbumTypeKey = @"Album Type";
NSString *_ipAlbumNameKey = @"AlbumName";

// One photo of an iPhoto library: its AlbumData.xml dictionary.
@implementation IPhotoPhoto

- (id)init
{
	return [self initWithDictionaryRepresentation:nil];
}

- (id)initWithDictionaryRepresentation:(id)aRepresentation
{
	self = [super init];
	if (self)
		_info = [aRepresentation retain];
	return self;
}

- (void)dealloc
{
	[_info release];
	[super dealloc];
}

// A file URL from the path under key, with aliases resolved.
- (NSURL *)_resolvedURLForKey:(NSString *)key
{
	NSString *path = [[self info] objectForKey:key];
	if (!path)
		return nil;
	NSURL *url = [NSURL fileURLWithPath:path];
	NSURL *resolved = [IPhotoHelper resolveAliasesInURL:url];
	return resolved ? resolved : url;
}

- (id)URL
{
	return [self _resolvedURLForKey:_ipPhotoPathKey];
}

- (id)thumbURL
{
	return [self _resolvedURLForKey:_ipThumbPathKey];
}

// The caption, else the image's file name without extension.
- (id)displayName
{
	NSString *name = [[self info] objectForKey:_ipPhotoNameKey];
	if (name)
		return name;
	NSURL *url = [self URL];
	if (!url && !(url = [self thumbURL]))
		return nil;
	if ([url isFileURL])
		return [[[url path] lastPathComponent] stringByDeletingPathExtension];
	return nil;
}

- (BOOL)hasAccessibleURL
{
	if (!_urlAccessTested) {
		NSURL *url = [self URL];
		if ([url isFileURL])
			_urlAccessOk = [[NSFileManager defaultManager] isReadableFileAtPath:[url path]];
		_urlAccessTested = YES;
	}
	return _urlAccessOk;
}

- (id)info
{
	return _info;
}

// Lazy pasteboard data: the image, its name (plain, RTF, RTFD) or its URL.
- (void)pasteboard:(id)aPasteboard provideDataForType:(id)aType
{
	NSData *data = nil;
	NSString *string = nil;
	if ([aType isEqualToString:NSPasteboardTypeTIFF]) {
		NSImage *image = [[[NSImage allocWithZone:[self zone]] initWithContentsOfURL:[self URL]] autorelease];
		data = [image TIFFRepresentation];
	} else if ([aType isEqualToString:NSPasteboardTypeString]) {
		string = [self displayName];
	} else if ([aType isEqualToString:NSPasteboardTypeURL]) {
		[[self URL] writeToPasteboard:aPasteboard];
	} else if ([aType isEqualToString:NSPasteboardTypeRTF] || [aType isEqualToString:NSPasteboardTypeRTFD]) {
		NSAttributedString *text = [[NSAttributedString allocWithZone:[self zone]] initWithString:[self displayName]];
		NSRange range = NSMakeRange(0, [text length]);
		if ([aType isEqualToString:NSPasteboardTypeRTF])
			data = [text RTFFromRange:range documentAttributes:@{}];
		else
			data = [text RTFDFromRange:range documentAttributes:@{}];
		[text release];
	}
	if (data)
		[aPasteboard setData:data forType:aType];
	if (string)
		[aPasteboard setString:string forType:aType];
}

@end
