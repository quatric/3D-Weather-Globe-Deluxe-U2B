#import "IPhotoAlbum.h"
#import "IPhotoPhoto.h"

// An album of the iPhoto library (AlbumData.xml): its photos, shared with
// the other albums through the photos dictionary, and an icon by kind.
@implementation IPhotoAlbum

- (id)init
{
	return [self initWithDictionaryRepresentation:nil photos:nil version:0];
}

- (id)initWithName:(id)aName dictionaryRepresentation:(id)aRepresentation photos:(id)aPhotos version:(NSInteger)aVersion
{
	self = [self initWithDictionaryRepresentation:aRepresentation photos:aPhotos version:aVersion];
	if (self)
		_name = [aName retain];
	return self;
}

// Version 1 lists the photos themselves (made here when new), version 2
// their keys. The library version is 1.1, 2 or (with album IDs) 3.
- (id)initWithDictionaryRepresentation:(id)aRepresentation photos:(id)aPhotos version:(NSInteger)aVersion
{
	self = [super init];
	if (!self)
		return nil;
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	_name = [[aRepresentation objectForKey:_ipAlbumNameKey] retain];
	_type = [[aRepresentation objectForKey:_ipAlbumTypeKey] retain];
	if (aVersion == 1)
		_libraryVersion = 1.1f;
	else if (aVersion == 2)
		_libraryVersion = [aRepresentation objectForKey:_ipAlbumIDKey] ? 3.0f : 2.0f;
	_photos = [[NSMutableArray allocWithZone:[self zone]] initWithCapacity:0];
	@try {
		_isMaster = [aRepresentation objectForKey:_ipMasterKey] != nil;
		@try {
			_isMaster = [[aRepresentation objectForKey:_ipMasterKey] boolValue];
		} @catch (id exception) {
		}
	} @catch (id exception) {
	}
	if (aVersion == 1) {
		NSDictionary *list = [aRepresentation objectForKey:_ipAlbumPhotosKey];
		NSEnumerator *enumerator = [[list allKeys] objectEnumerator];
		id key;
		while ((key = [enumerator nextObject])) {
			id photo = [aPhotos objectForKey:key];
			if (photo) {
				[_photos addObject:photo];
				continue;
			}
			@try {
				photo = [[[IPhotoPhoto allocWithZone:[self zone]] initWithDictionaryRepresentation:[list objectForKey:key]]
					autorelease];
				[aPhotos setObject:photo forKey:key];
				[_photos addObject:photo];
			} @catch (id exception) {
			}
		}
	} else if (aVersion == 2) {
		NSEnumerator *enumerator = [[aRepresentation objectForKey:_ipAlbumKeysListKey] objectEnumerator];
		id key;
		while ((key = [enumerator nextObject])) {
			@try {
				[_photos addObject:[aPhotos objectForKey:key]];
			} @catch (id exception) {
			}
		}
	} else {
		[self dealloc];
		self = nil;
	}
	[pool release];
	return self;
}

- (void)dealloc
{
	[_photos release];
	[_name release];
	[_type release];
	[_lastImportImage release];
	[_libraryImage release];
	[_library4Image release];
	[_smartAlbumImage release];
	[_albumImage release];
	[_specialMonthImage release];
	[_specialRollImage release];
	[_folderImage release];
	[_bookImage release];
	[_slideshowImage release];
	[super dealloc];
}

// The image named in the class's bundle (a TIFF), loaded once.
- (NSImage *)_image:(NSImage **)aImage named:(NSString *)aName
{
	if (!*aImage) {
		NSString *path = [[NSBundle bundleForClass:[self class]] pathForResource:aName ofType:@"tiff"];
		if (path)
			*aImage = [[NSImage allocWithZone:[self zone]] initWithContentsOfFile:path];
	}
	return *aImage;
}

- (id)icon
{
	if ([self isMaster])
		return _libraryVersion < 3.0f ? [self _image:&_libraryImage named:@"iPhotoLibrary"]
			: [self _image:&_library4Image named:@"iPhotoLibrary4"];
	if ([self isSpecialRoll])
		return [self _image:&_specialRollImage named:@"iPhotoSpecialRoll"];
	if ([self isSpecialMonth])
		return [self _image:&_specialMonthImage named:@"iPhotoSpecialMonth"];
	if ([self isSmart])
		return [self _image:&_smartAlbumImage named:@"iPhotoSmartAlbum"];
	if ([self isLastImport])
		return [self _image:&_lastImportImage named:@"iPhotoLastImport"];
	if ([self isFolder])
		return [self _image:&_folderImage named:@"iPhotoFolder"];
	if ([self isBook])
		return [self _image:&_bookImage named:@"iPhotoBook"];
	if ([self isSlideshow])
		return [self _image:&_slideshowImage named:@"iPhotoSlideshow"];
	return [self _image:&_albumImage named:@"iPhotoAlbum"];
}

- (id)displayName
{
	return _name;
}

- (NSUInteger)numberOfSubitems
{
	return 0;
}

- (id)subitemAtIndex:(NSUInteger)aIndex
{
	return nil;
}

- (NSUInteger)numberOfPhotos
{
	return [_photos count];
}

- (id)photoAtIndex:(NSUInteger)aIndex
{
	return aIndex < [_photos count] ? [_photos objectAtIndex:aIndex] : nil;
}

// The library first, then the last import, the rolls, the months, the
// folders, the smart albums, the albums, the slideshows and the books.
- (NSInteger)sortWeight
{
	if ([self isMaster])
		return 8;
	if ([self isLastImport])
		return 7;
	if ([self isSpecialRoll])
		return 6;
	if ([self isSpecialMonth])
		return 5;
	if ([self isFolder])
		return 4;
	if ([self isSmart])
		return 3;
	if ([self isBook])
		return 1;
	if ([self isSlideshow])
		return 0;
	return 2;
}

- (BOOL)isMaster
{
	return _isMaster;
}

// The photos whose names contain the text (any case).
- (id)filterPhotos:(id)aPhotos byCategory:(id)aCategory
{
	if (![aPhotos length])
		return _photos;
	NSMutableArray *photos = [NSMutableArray arrayWithCapacity:0];
	NSEnumerator *enumerator = [_photos objectEnumerator];
	id photo;
	while ((photo = [enumerator nextObject]))
		if ([[photo displayName] rangeOfString:aPhotos options:NSCaseInsensitiveSearch].location != NSNotFound)
			[photos addObject:photo];
	return photos;
}

// Heavier first, then by name.
- (NSInteger)compareIncrease:(id)aIncrease
{
	if ([self sortWeight] == [aIncrease sortWeight])
		return [[self displayName] compare:[aIncrease displayName] options:NSCaseInsensitiveSearch | NSNumericSearch];
	return [self sortWeight] <= [aIncrease sortWeight] ? NSOrderedDescending : NSOrderedAscending;
}

- (NSInteger)compareDecrease:(id)aDecrease
{
	return [aDecrease compareIncrease:self];
}

- (BOOL)isLastImport
{
	return [[self displayName] isEqualToString:[[NSBundle mainBundle] localizedStringForKey:@"iPhotoLastImport" value:@"" table:nil]];
}

- (BOOL)_isType:(NSString *)aKey
{
	return [_type isEqualToString:[[NSBundle mainBundle] localizedStringForKey:aKey value:@"" table:nil]];
}

- (BOOL)isSpecialRoll
{
	return [self _isType:@"iPhotoSpecialRoll"];
}

- (BOOL)isSpecialMonth
{
	return [self _isType:@"iPhotoSpecialMonth"];
}

- (BOOL)isSmart
{
	return [self _isType:@"iPhotoSmart"];
}

- (BOOL)isFolder
{
	return [self _isType:@"iPhotoFolder"];
}

- (BOOL)isBook
{
	return [self _isType:@"iPhotoBook"];
}

- (BOOL)isSlideshow
{
	return [self _isType:@"iPhotoSlideshow"];
}

@end
