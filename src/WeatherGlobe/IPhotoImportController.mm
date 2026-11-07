#import "IPhotoImportController.h"
#import "IPhotoHelper.h"
#import "IPhotoAlbum.h"
#import "IPhotoPhoto.h"
#import "IAppsPasteboardDataProvider.h"
#import "SMKGridControl.h"
#import "WGSplitView.h"
#import "WGCustomSlider.h"
#import "WGTexturedView.h"
#import "WGCustomTitleView.h"

static NSString *_ipAlbumImageID = @"Album Image";

// The iPhoto palette: iPhoto's albums (from the libraries' AlbumData.xml)
// on the left, the selected album's photos, filtered by the search field,
// in a grid. Photos are dragged or double-clicked out as files.
@implementation IPhotoImportController

- (id)nibName
{
	return @"iPhotoImport";
}

- (id)autosaveName
{
	return [self className];
}

- (void)dealloc
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults setObject:[NSNumber numberWithInt:(int)[_albumsOutlineView selectedRow]] forKey:_ipSelectedAlbumIndexKey];
	[defaults synchronize];
	if ([_photosGrid delegate] == self)
		[_photosGrid setDelegate:nil];
	if ([_photosGrid dataSource] == self)
		[_photosGrid setDataSource:nil];
	[_photosGrid setTarget:nil];
	if ([_albumsOutlineView delegate] == (id)self)
		[_albumsOutlineView setDelegate:nil];
	if ([_albumsOutlineView dataSource] == (id)self)
		[_albumsOutlineView setDataSource:nil];
	[_filteredItems release];
	[_invalidPhotoImage release];
	[_filterString release];
	[_categoryString release];
	[_photos release];
	[_albums release];
	[_modificationTimes release];
	[super dealloc];
}

// Without a library the palette looks for one each time it is shown.
- (void)showPalette
{
	[super showPalette];
	if (!_iPhotoLibraryAbsent)
		return;
	[self initializeWithLibraryFiles:[IPhotoHelper iPhotoRecentDatabases]];
	if (![_albums count]) {
		[_photosGrid setPlaceholderString:[[NSBundle mainBundle]
			localizedStringForKey:@"Launch iPhoto to populate this list." value:@"" table:nil]];
		_iPhotoLibraryAbsent = YES;
	} else {
		[_photosGrid setPlaceholderString:[[NSBundle mainBundle] localizedStringForKey:@"Empty" value:@"" table:nil]];
		[_albumsOutlineView reloadData];
		_iPhotoLibraryAbsent = NO;
	}
}

// Return in the search field moves the focus to it.
- (void)filter:(id)aFilter
{
	NSEvent *event = [[_searchField window] currentEvent];
	if ([event type] == NSEventTypeKeyDown || [event type] == NSEventTypeKeyUp) {
		if ([[event characters] rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"\r\n\x03"]]
			.location != NSNotFound)
			[[_searchField window] makeFirstResponder:_searchField];
	}
	NSString *info = [self filter:[_searchField stringValue] byCategory:@""];
	[_infoText setStringValue:info ? info : @""];
}

// Keeps the photo selection at the first one when the album changes.
- (void)selectItem:(id)aItem
{
	if (_selectedAlbum == aItem)
		return;
	if (aItem && ![aItem isKindOfClass:[IPhotoAlbum class]])
		return;
	NSInteger selected = [[_photosGrid selectedItems] count];
	_selectedAlbum = aItem;
	[self filterPhotos:_filterString byCategory:_categoryString];
	if ([_photosGrid count]) {
		if (selected > 0)
			[_photosGrid selectItemAtIndex:0 byExtendingSelection:NO];
		[_photosGrid scrollToVisibleAtIndex:0];
	}
}

- (NSInteger)numberOfItemsInGridControl:(id)aControl
{
	return [_filteredItems count];
}

- (BOOL)areTitlesCanBePreloadedInGridControl:(id)aControl
{
	return NO;
}

- (id)gridControl:(id)aControl titleForItemAt:(NSInteger)aAt
{
	return [[_filteredItems objectAtIndex:aAt] displayName];
}

- (id)gridControl:(id)aControl imageForItemAt:(NSInteger)aAt
{
	if (aAt < 0 || (NSUInteger)aAt >= [_filteredItems count])
		return nil;
	IPhotoPhoto *photo = [_filteredItems objectAtIndex:aAt];
	if ([photo hasAccessibleURL]) {
		NSURL *url = [photo thumbURL];
		NSImage *image = url ? [[[NSImage alloc] initByReferencingURL:url] autorelease] : nil;
		if (image)
			return image;
	}
	return [self invalidPhotoImage];
}

- (id)gridControl:(id)aControl additionalInfoForItemAt:(NSInteger)aAt
{
	return [NSDictionary dictionaryWithObject:[[NSBundle mainBundle] localizedStringForKey:@"Photo" value:@"" table:nil]
		forKey:SMKGridAttributeVoiceOverItemTypeKey];
}

- (BOOL)canCopyToPasteboardFromGridControl:(id)aControl
{
	NSEnumerator *enumerator = [[_photosGrid selectedItemIndexes] objectEnumerator];
	NSNumber *index;
	while ((index = [enumerator nextObject])) {
		NSUInteger i = [index unsignedIntValue];
		if (i < [_filteredItems count] && [[_filteredItems objectAtIndex:i] hasAccessibleURL])
			return YES;
	}
	return NO;
}

- (BOOL)gridControl:(id)aControl writeIndexes:(id)aIndexes toPasteboard:(id)aPasteboard
{
	return [self writeIndexes:aIndexes toPasteboard:aPasteboard];
}

// Return or Enter in the grid acts as a double click.
- (BOOL)gridControl:(id)aControl performKeyEquivalent:(id)aEquivalent
{
	if ([(NSEvent *)aEquivalent type] != NSEventTypeKeyDown)
		return NO;
	unsigned short key = [aEquivalent keyCode];
	if (key != 0x34 && key != 0x24 && key != 0x4c)
		return NO;
	if ([[_albumsOutlineView window] firstResponder] != [_photosGrid gridMatrix])
		return NO;
	[self onDoubleClick:_photosGrid];
	return YES;
}

- (NSInteger)outlineView:(id)aView numberOfChildrenOfItem:(id)aItem
{
	return aItem ? [aItem numberOfSubitems] : [_albums count];
}

- (BOOL)outlineView:(id)aView isItemExpandable:(id)aExpandable
{
	return !aExpandable || [aExpandable numberOfSubitems] != 0;
}

- (id)outlineView:(id)aView child:(NSInteger)aChild ofItem:(id)aItem
{
	if (aItem)
		return [aItem subitemAtIndex:aChild];
	if (aChild < 0 || aChild == NSIntegerMax || (NSUInteger)aChild >= [_albums count])
		return nil;
	return [_albums objectAtIndex:aChild];
}

- (id)outlineView:(id)aView objectValueForTableColumn:(id)aColumn byItem:(id)aItem
{
	if ([[aColumn identifier] isEqualToString:_ipAlbumImageID])
		return [aItem icon];
	return [aItem displayName];
}

- (BOOL)outlineView:(id)aView shouldSelectItem:(id)aItem
{
	return YES;
}

- (void)outlineViewSelectionDidChange:(id)aChange
{
	[self selectItem:[_albumsOutlineView itemAtRow:[_albumsOutlineView selectedRow]]];
	[_searchField setStringValue:@""];
	[self filter:nil];
}

- (void)splitView:(id)aView resizeSubviewsWithOldSize:(NSSize)aSize
{
	if (aView == _splitView)
		[aView wgCustomAdjustSubviews:YES];
	[aView adjustSubviews];
}

- (CGFloat)splitView:(id)aView constrainMinCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt
{
	if (aView == _splitView)
		return [aView wgConstrainMinCoordinate:aCoordinate ofSubviewAt:aAt];
	return aCoordinate;
}

- (CGFloat)splitView:(id)aView constrainMaxCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt
{
	if (aView == _splitView)
		return [aView wgConstrainMaxCoordinate:aCoordinate ofSubviewAt:aAt];
	return aCoordinate;
}

- (id)invalidPhotoImage
{
	if (!_invalidPhotoImage) {
		NSString *path = [[NSBundle bundleForClass:[self class]] pathForResource:@"iPhotoInvalidPicture" ofType:@"tiff"];
		if (path)
			_invalidPhotoImage = [[NSImage alloc] initWithContentsOfFile:path];
	}
	return _invalidPhotoImage;
}

// Refilters when the filter changed and returns the "N photos" info.
- (id)filter:(id)aFilter byCategory:(id)aCategory
{
	if (!([_filterString isEqualToString:aFilter] && [_categoryString isEqualToString:aCategory])
		&& ([aFilter length] || [_filterString length]))
		[self filterPhotos:aFilter byCategory:aCategory];
	NSInteger count = [_photosGrid count];
	NSBundle *bundle = [NSBundle mainBundle];
	if (count == 1)
		return [bundle localizedStringForKey:@"iAppsFilterOne" value:@"" table:nil];
	return [NSString stringWithFormat:[bundle localizedStringForKey:@"iAppsFilter" value:@"" table:nil], (int)count];
}

- (void)filterPhotos:(id)aPhotos byCategory:(id)aCategory
{
	if (![_filterString isEqualToString:aPhotos]) {
		[_filterString release];
		_filterString = [aPhotos copy];
	}
	if (![_categoryString isEqualToString:aCategory]) {
		[_categoryString release];
		_categoryString = [aCategory copy];
	}
	[_filteredItems release];
	_filteredItems = [[_selectedAlbum filterPhotos:_filterString byCategory:_categoryString] retain];
	[_photosGrid reloadData];
}

// Reloads the libraries when one of the files changed, appeared or
// disappeared.
- (void)initializeWithLibraryFiles:(id)aFiles
{
	@try {
		NSMutableArray *libraries = [NSMutableArray arrayWithCapacity:[aFiles count]];
		NSFileManager *manager = [NSFileManager defaultManager];
		BOOL changed = NO;
		NSEnumerator *enumerator = [aFiles objectEnumerator];
		NSURL *url;
		while ((url = [enumerator nextObject])) {
			if (![url isFileURL])
				continue;
			NSString *path = [url path];
			NSString *name = [path lastPathComponent];
			NSDate *modified = [[manager attributesOfItemAtPath:[path stringByResolvingSymlinksInPath] error:NULL]
				objectForKey:NSFileModificationDate];
			if (![manager isReadableFileAtPath:path])
				modified = nil;
			if (![[[self modificationTimes] objectForKey:name] isEqualToDate:modified]) {
				[[self modificationTimes] removeObjectForKey:name];
				changed = YES;
				if (modified)
					[[self modificationTimes] setObject:modified forKey:name];
			}
			NSDictionary *library = [NSDictionary dictionaryWithContentsOfFile:[url path]];
			if (library)
				[libraries addObject:library];
		}
		if (changed || (![libraries count] && [_photos count]) || ([libraries count] && ![_photos count]))
			[self initializeWithLibraries:libraries];
	} @catch (id exception) {
	}
}

// Version 1 libraries list the albums with their photos; version 2 lists
// the photos once and the albums by photo keys. The master album ("Photo
// Library") goes first.
- (void)initializeWithLibraries:(id)aLibraries
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSEnumerator *libraries = [aLibraries objectEnumerator];
	[_photos release];
	_photos = nil;
	[_albums release];
	_albums = nil;
	NSDictionary *library;
	while ((library = [libraries nextObject])) {
		@try {
			int version = [[library objectForKey:_ipMajorVersionKey] intValue];
			library = [self updateLibrary:library withMajorVersion:version];
			NSMutableArray *found = [NSMutableArray array];
			if (version == 1) {
				NSDictionary *albums = [library objectForKey:_ipAlbumsListKey];
				NSEnumerator *names = [[albums allKeys] objectEnumerator];
				NSString *name;
				while ((name = [names nextObject])) {
					@try {
						if (!_photos)
							_photos = [[NSMutableDictionary alloc] initWithCapacity:0];
						[found addObject:[[[IPhotoAlbum alloc] initWithName:name
							dictionaryRepresentation:[albums objectForKey:name] photos:_photos version:1] autorelease]];
					} @catch (id exception) {
					}
				}
			} else if (version == 2) {
				NSDictionary *photos = [library objectForKey:_ipPhotosListKey];
				NSEnumerator *keys = [[photos allKeys] objectEnumerator];
				NSString *key;
				while ((key = [keys nextObject])) {
					@try {
						IPhotoPhoto *photo = [[[IPhotoPhoto alloc]
							initWithDictionaryRepresentation:[photos objectForKey:key]] autorelease];
						if (!_photos)
							_photos = [[NSMutableDictionary alloc] initWithCapacity:0];
						[_photos setObject:photo forKey:key];
					} @catch (id exception) {
					}
				}
				NSEnumerator *albums = [[library objectForKey:_ipAlbumsListKey] objectEnumerator];
				NSDictionary *album;
				while ((album = [albums nextObject])) {
					@try {
						[found addObject:[[[IPhotoAlbum alloc] initWithDictionaryRepresentation:album photos:_photos
							version:2] autorelease]];
					} @catch (id exception) {
					}
				}
			}
			for (IPhotoAlbum *album in found) {
				if (!_albums)
					_albums = [[NSMutableArray alloc] initWithCapacity:0];
				if ([album isMaster] && [_albums count])
					[_albums insertObject:album atIndex:0];
				else
					[_albums addObject:album];
			}
			[_albums sortUsingSelector:@selector(compareIncrease:)];
		} @catch (id exception) {
		}
	}
	[pool release];
}

- (id)modificationTimes
{
	if (!_modificationTimes)
		_modificationTimes = [[NSMutableDictionary alloc] initWithCapacity:0];
	return _modificationTimes;
}

- (void)onDoubleClick:(id)aClick
{
	[self writeIndexes:[_photosGrid selectedItemIndexes] toPasteboard:[NSPasteboard generalPasteboard]];
}

// The selected photos that have files, as file names.
- (BOOL)writeIndexes:(id)aIndexes toPasteboard:(id)aPasteboard
{
	NSMutableIndexSet *indexes = [[NSMutableIndexSet new] autorelease];
	NSEnumerator *enumerator = [aIndexes objectEnumerator];
	NSNumber *number;
	while ((number = [enumerator nextObject]))
		[indexes addIndex:[number unsignedIntValue]];
	if (![indexes count])
		return NO;
	NSMutableArray *items = nil;
	// The original didn't advance past an index beyond the photos.
	for (NSUInteger i = [indexes firstIndex]; i != NSNotFound; i = [indexes indexGreaterThanIndex:i]) {
		if (i >= [_filteredItems count])
			continue;
		IPhotoPhoto *photo = [_filteredItems objectAtIndex:i];
		if (photo && [photo hasAccessibleURL]) {
			if (!items)
				items = [NSMutableArray arrayWithCapacity:0];
			[items addObject:photo];
		}
	}
	if (![items count])
		return NO;
	IAppsPasteboardDataProvider *provider = [IAppsPasteboardDataProvider dataProviderWithItems:items];
	[aPasteboard declareTypes:[NSArray arrayWithObject:NSFilenamesPboardType] owner:nil];
	[provider pasteboard:aPasteboard provideDataForType:NSFilenamesPboardType];
	return YES;
}

- (NSSize)splitView:(id)aView minSizeForView:(id)aView2
{
	if (aView != _splitView)
		return NSZeroSize;
	return NSMakeSize(0.0, aView2 == [_albumsOutlineView enclosingScrollView] ? 72.0 : 150.0);
}

// Thumbnails from 64 to 192 points, in steps of 2.
- (void)sliderAction:(id)aAction
{
	float size = truncf(((float)[aAction floatValue] * (128.0f / (float)[aAction maxValue]) + 64.0f) * 0.5f);
	size += size;
	[_photosGrid setThumbnailSize:NSMakeSize(size, size)];
	[_photosGrid setNeedsDisplay:YES];
}

- (void)awakeFromNib
{
	if (![_palette setFrameUsingName:@"iPhoto Import Palette"])
		[_palette center];
	[_palette setFrameAutosaveName:@"iPhoto Import Palette"];
	[self initializeWithLibraryFiles:[IPhotoHelper iPhotoRecentDatabases]];
	id titleView = [(id)_palette customTitleView];
	[titleView setImage:[NSImage imageNamed:@"RouteManager_title"] forTexture:1];
	[titleView setTitleShadowColor:[[NSColor blackColor] colorWithAlphaComponent:0.6]];
	[titleView setTitleShadowOffset:NSMakeSize(0.0, 1.0)];
	[titleView setDrawsTitle:YES];
	[_texturedBottomView setImage:[NSImage imageNamed:@"RouteManager_bottom"] forTexture:7];
	[_palette setMovableByWindowBackground:YES];
	[_palette setBackgroundColor:[NSColor colorWithDeviceRed:0.9 green:0.9 blue:0.9 alpha:1.0]];
	[_splitView setDividerThickness:10.0];
	[_splitView setDividerFillColor:nil];
	[_slider setKnobImage:[NSImage imageNamed:@"s_sliderknob"]];
	[_slider setKnobAlternateImage:[NSImage imageNamed:@"s_sliderknob_on"]];
	[_slider setBarFirstEdgeImage:[NSImage imageNamed:@"s_sliderback_left"]];
	[_slider setBarMiddleImage:[NSImage imageNamed:@"s_sliderback_center"]];
	[_slider setBarLastEdgeImage:[NSImage imageNamed:@"s_sliderback_right"]];
	[_photosGrid setDelegate:self];
	[_photosGrid setDataSource:self];
	[_photosGrid setTarget:self];
	[_photosGrid setDoubleAction:@selector(onDoubleClick:)];
	[_photosGrid setAllowsMultipleSelection:YES];
	[_photosGrid setSelectionStyle:3];
	[_photosGrid setEmulatingNoOwnScrollView:YES];
	[_photosGrid setCellControlSize:NSControlSizeSmall];
	[_photosGrid setThumbnailCachingEnabled:NO];
	[_photosGrid setThumbnailHyphenationEnabled:NO];
	[_photosGrid setExtraPasteboardDataProvidingAllowed:NO];
	[[_photosGrid scrollView] setBorderType:NSNoBorder];
	[_photosGrid setMode:0];
	[_photosGrid setThumbnailMaxTitleHeight:17.0];
	float size = (128.0f / (float)[_slider maxValue]) * (float)[_slider floatValue] + 64.0f;
	[_photosGrid setThumbnailSize:NSMakeSize(size, size)];
	if (![_albums count]) {
		[_photosGrid setPlaceholderString:[[NSBundle mainBundle]
			localizedStringForKey:@"Launch iPhoto to populate this list." value:@"" table:nil]];
		_iPhotoLibraryAbsent = YES;
	} else {
		[_photosGrid setPlaceholderString:[[NSBundle mainBundle] localizedStringForKey:@"Empty" value:@"" table:nil]];
		_iPhotoLibraryAbsent = NO;
	}
	[_albumsOutlineView setDelegate:(id)self];
	[_albumsOutlineView setDataSource:(id)self];
	[_albumsOutlineView setAllowsMultipleSelection:NO];
	[_albumsOutlineView setAllowsColumnSelection:NO];
	[_albumsOutlineView setIntercellSpacing:NSMakeSize(1.0, 2.0)];
	[_albumsOutlineView setAllowsColumnReordering:NO];
	[_albumsOutlineView setAllowsColumnResizing:NO];
	if (!_iPhotoLibraryAbsent) {
		int row = [[[NSUserDefaults standardUserDefaults] objectForKey:_ipSelectedAlbumIndexKey] intValue];
		[_albumsOutlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
		[_albumsOutlineView scrollRowToVisible:row];
	}
}

// Rewrites a path prefix in a mutable copy of a record's string under key;
// returns whether anything changed.
static BOOL _replacePathPrefix(NSMutableDictionary *record, NSString *key, NSString *from, NSString *to)
{
	NSMutableString *path = [NSMutableString stringWithString:[record objectForKey:key]];
	if (![path replaceOccurrencesOfString:from withString:to options:NSLiteralSearch range:NSMakeRange(0, [path length])])
		return NO;
	[record setObject:path forKey:key];
	return YES;
}

// A library whose archive (folder) isn't readable any more is moved to
// the library folder iPhoto now reports: the paths in it are rewritten.
// Version 1 libraries don't record the archive path; it is taken from the
// first photo path inside a folder named like the iPhoto library.
- (id)updateLibrary:(id)aLibrary withMajorVersion:(NSInteger)aVersion
{
	NSMutableDictionary *library = [NSMutableDictionary dictionaryWithDictionary:aLibrary];
	NSString *archive = nil;
	if (aVersion == 2) {
		NSString *path = [aLibrary objectForKey:_ipArchivePathKey];
		archive = path ? [NSString stringWithString:path] : @"";
	}
	NSFileManager *manager = [NSFileManager defaultManager];
	if (archive && [manager isReadableFileAtPath:archive])
		return library;
	NSString *folder = nil;
	NSEnumerator *keys = [[NSArray arrayWithObjects:_iPhotoRecentDatabasesKey, _iPhotoRecentDatabases1Key, nil]
		objectEnumerator];
	NSString *key;
	while (!folder && (key = [keys nextObject]))
		folder = [[[[NSBundle mainBundle] localizedStringForKey:key value:@"" table:nil] stringByExpandingTildeInPath]
			stringByDeletingLastPathComponent];
	if (!folder)
		return library;
	NSURL *url = [NSURL fileURLWithPath:folder];
	NSURL *resolved = [IPhotoHelper resolveAliasesInURL:url];
	NSString *current = [(resolved ? resolved : url) path];
	if ([current isEqual:archive] || ![manager isReadableFileAtPath:current])
		return library;
	if (aVersion == 1) {
		NSMutableDictionary *albums = [NSMutableDictionary dictionaryWithDictionary:[library objectForKey:_ipAlbumsListKey]];
		NSString *folderName = [[NSBundle mainBundle] localizedStringForKey:@"iPhotoLibraryFolderName" value:@""
			table:nil];
		NSEnumerator *names = [[albums allKeys] objectEnumerator];
		NSString *name;
		while ((name = [names nextObject])) {
			@try {
				NSMutableDictionary *album = [NSMutableDictionary dictionaryWithDictionary:[albums objectForKey:name]];
				NSMutableDictionary *photos = [NSMutableDictionary
					dictionaryWithDictionary:[album objectForKey:_ipAlbumPhotosKey]];
				NSEnumerator *photoKeys = [[photos allKeys] objectEnumerator];
				NSString *photoKey;
				while ((photoKey = [photoKeys nextObject])) {
					NSMutableDictionary *photo = [NSMutableDictionary dictionaryWithDictionary:[photos objectForKey:photoKey]];
					if (!archive || [archive isEqual:@""]) {
						for (NSString *path = [NSString stringWithString:[photo objectForKey:_ipPhotoPathKey]]; path;
							path = [path stringByDeletingLastPathComponent]) {
							if ([path isEqual:@"/"])
								break;
							if ([[path lastPathComponent] isEqual:folderName]) {
								archive = path;
								break;
							}
						}
					}
					if (archive && ![archive isEqual:@""] && ![archive isEqual:current]) {
						_replacePathPrefix(photo, _ipPhotoPathKey, archive, current);
						_replacePathPrefix(photo, _ipThumbPathKey, archive, current);
					}
					[photos setObject:photo forKey:photoKey];
				}
				[album setObject:photos forKey:_ipAlbumPhotosKey];
				[albums setObject:album forKey:name];
			} @catch (id exception) {
			}
		}
		[library setObject:albums forKey:_ipAlbumsListKey];
	} else if (aVersion == 2) {
		[library setObject:current forKey:_ipArchivePathKey];
		NSMutableDictionary *photos = [NSMutableDictionary dictionaryWithDictionary:[library objectForKey:_ipPhotosListKey]];
		NSEnumerator *photoKeys = [[photos allKeys] objectEnumerator];
		NSString *photoKey;
		while ((photoKey = [photoKeys nextObject])) {
			@try {
				NSMutableDictionary *photo = [NSMutableDictionary dictionaryWithDictionary:[photos objectForKey:photoKey]];
				_replacePathPrefix(photo, _ipPhotoPathKey, archive, current);
				_replacePathPrefix(photo, _ipThumbPathKey, archive, current);
				[photos setObject:photo forKey:photoKey];
			} @catch (id exception) {
			}
		}
		[library setObject:photos forKey:_ipPhotosListKey];
		NSMutableArray *albums = [NSMutableArray arrayWithArray:[library objectForKey:_ipAlbumsListKey]];
		for (NSUInteger i = 0; i < [albums count]; i++) {
			@try {
				NSMutableDictionary *album = [NSMutableDictionary dictionaryWithDictionary:[albums objectAtIndex:i]];
				if (_replacePathPrefix(album, _ipSongPathKey, archive, current))
					[albums replaceObjectAtIndex:i withObject:album];
			} @catch (id exception) {
			}
		}
		[library setObject:albums forKey:_ipAlbumsListKey];
	}
	return library;
}

@end
