#import "WGExportImageController.h"
#import "WGExportPanel.h"
#import "AtlasGLView.h"
#import "AtlasController.h"
#import "MainWindowController.h"

// The output size menus' tag for a custom size.
enum { kCustomSizeTag = 100 };

// The largest custom side.
enum { kMaxImageSide = 12288 };

// The format popup's items: the old NSSavePanel file types, and the
// extensions and bitmap file types they stand for.
static NSString *const _fileTypes[] = {@"TIFF", @"JPEG", @"PNG"};

static NSString *_extensionForFileType(NSString *type)
{
	if ([type isEqualToString:@"JPEG"])
		return @"jpg";
	if ([type isEqualToString:@"PNG"])
		return @"png";
	return @"tiff";
}

// Landscape sizes; the portrait menu swaps them.
static const NSSize _menuSizes[] = {
	{320, 240}, {512, 384}, {640, 480}, {800, 600}, {1024, 768}, {1280, 1024}, {1600, 1200},
	{2048, 1600}, {3072, 2304}, {4096, 3072}, {6144, 4608}, {8192, 6144}, {12288, 9216},
};

// Saves a picture of the globe view: a save panel with an accessory for
// the format and the size (from the menus, or custom up to 12288).
@implementation WGExportImageController

- (void)dealloc
{
	[_exportedImagesFolder release];
	[_exportImageAccessory release];
	[_exportImagePortraitOutSizeMenu release];
	[_exportImageLandscapeOutSizeMenu release];
	[_exportImageFormatMenu release];
	[super dealloc];
}

// JPEG, landscape 1024×768 by default.
- (void)awakeFromNib
{
	[_exportImageFormatPopUp setMenu:_exportImageFormatMenu];
	[_exportImageFormatPopUp selectItemAtIndex:1];
	[self exportImageModeChanged:nil];
	[_exportImageOutSizePopUp setTarget:self];
	[_exportImageOutSizePopUp setAction:@selector(_imageFormatPopupChanged:)];
	_exportedImagesFolder = @"~/Pictures";
}

- (id)nibName
{
	return @"ExportImageAccessory";
}

- (id)defaultExportFolder
{
	return @"~/Pictures";
}

- (id)exportedImageFilename
{
	return _exportedImageFilename;
}

- (void)setExportedImageFilename:(id)aFilename
{
	if (_exportedImageFilename != aFilename) {
		[_exportedImageFilename release];
		_exportedImageFilename = [aFilename retain];
	}
}

- (id)glViewAsImageData:(id)aData forSize:(NSSize)aSize
{
	return [aData grabScreenshotOfApproxSize:aSize virtualSize:aSize dataFormat:NSBitmapImageFileTypeJPEG];
}

- (void)exportGlViewAsImage:(id)aImage
{
	[self loadNibIfNeeded];
	_exportedView = aImage;
	WGExportPanel *panel = (WGExportPanel *)[WGExportPanel savePanel];
	[panel setExtensionHidden:YES];
	[panel setCanSelectHiddenExtension:YES];
	[panel setAllowedFileTypes:[NSArray arrayWithObject:_extensionForFileType([self _requiredFileType])]];
	[[panel okButton] setTitle:[[NSBundle mainBundle] localizedStringForKey:@"Export" value:@"" table:nil]];
	[panel setDelegate:(id)self];
	[panel setAccessoryView:_exportImageAccessory];
	NSWindow *window = [[[AtlasController atlasController] mainWindowController] mainWindow];
	[panel setDirectoryURL:[NSURL fileURLWithPath:[_exportedImagesFolder stringByExpandingTildeInPath]]];
	[panel setNameFieldStringValue:[[NSBundle mainBundle] localizedStringForKey:@"Untitled" value:@"" table:nil]];
	[panel beginSheetModalForWindow:window completionHandler:^(NSModalResponse result) {
		[self _savePanelDidEnd:panel returnCode:result contextInfo:0];
	}];
	[self _validateCustomSize];
}

- (void)imageTypeChanged:(id)aChanged
{
	[(NSSavePanel *)[_exportImageAccessory window] setAllowedFileTypes:
		[NSArray arrayWithObject:_extensionForFileType([self _requiredFileType])]];
}

// Swaps the size menu for the orientation, keeping the selected size
// (1024×768 at first).
- (void)exportImageModeChanged:(id)aChanged
{
	NSInteger index = aChanged ? [_exportImageOutSizePopUp indexOfSelectedItem] : 4;
	NSMenu *menu = [aChanged keyCell] == _exportImagePortrait ? _exportImagePortraitOutSizeMenu
		: _exportImageLandscapeOutSizeMenu;
	if (menu == [_exportImageOutSizePopUp menu])
		return;
	[_exportImageOutSizePopUp setMenu:menu];
	[_exportImageOutSizePopUp selectItemAtIndex:index];
	[self _imageFormatPopupChanged:nil];
}

- (void)controlTextDidChange:(id)aChange
{
	[self _validateCustomSize];
}

// A custom size enables its fields (and disables the orientation); a menu
// size fills them in.
- (void)_imageFormatPopupChanged:(id)aChanged
{
	if ([_exportImageOutSizePopUp selectedTag] == kCustomSizeTag) {
		[_exportImageLandscape setEnabled:NO];
		[_exportImagePortrait setEnabled:NO];
		[_exportImageWidthField setEnabled:YES];
		[_exportImageHeightField setEnabled:YES];
		return;
	}
	[[(WGExportPanel *)[_exportImageAccessory window] okButton] setEnabled:YES];
	[_exportImageLandscape setEnabled:YES];
	[_exportImagePortrait setEnabled:YES];
	[_exportImageWidthField setEnabled:NO];
	[_exportImageHeightField setEnabled:NO];
	NSSize size = [self _imageSizeFromMenu];
	if (NSEqualSizes(NSZeroSize, size))
		return;
	[_exportImageWidthField setStringValue:[NSString stringWithFormat:@"%d", (int)size.width]];
	[_exportImageHeightField setStringValue:[NSString stringWithFormat:@"%d", (int)size.height]];
}

// The sheet has ended already (the original closed it and called
// -endSheet: here).
- (void)_savePanelDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo
{
	NSSavePanel *panel = aEnd;
	NSString *path = [[panel URL] path];
	[panel makeFirstResponder:nil];
	if (aCode != NSModalResponseOK || !path)
		return;
	[panel close];
	[_exportedImagesFolder release];
	_exportedImagesFolder = [[path stringByDeletingLastPathComponent] retain];
	[self _saveScreenShootAtPath:path hideExt:[panel isExtensionHidden]];
}

// Grabs the view at the size and writes it, replacing the file unless it
// is locked; failures are reported.
- (void)_saveScreenShootAtPath:(id)aPath hideExt:(BOOL)aExt
{
	NSFileManager *manager = [NSFileManager defaultManager];
	NSString *name = [aPath lastPathComponent];
	NSString *folder = [aPath stringByDeletingLastPathComponent];
	NSBundle *bundle = [NSBundle mainBundle];
	NSString *title, *message;
	if (![manager isWritableFileAtPath:folder]) {
		title = [bundle localizedStringForKey:@"Cannot save to folder title" value:@"" table:nil];
		message = [NSString stringWithFormat:[bundle localizedStringForKey:@"Cannot save to folder message" value:@"" table:nil], folder];
	} else if ([manager fileExistsAtPath:aPath] && LFileManager::manager()->isLockedFileAtPath([aPath LString])) {
		title = [NSString stringWithFormat:[bundle localizedStringForKey:@"Cannot replace file title" value:@"" table:nil],
			[name stringByDeletingPathExtension]];
		message = [bundle localizedStringForKey:@"Cannot replace file message" value:@"" table:nil];
	} else {
		NSString *type = [self _requiredFileType];
		_exportedImageFilename = name;
		NSBitmapImageFileType format = NSBitmapImageFileTypeJPEG;
		if (![type isEqualToString:@"JPEG"])
			format = [type isEqualToString:@"PNG"] ? NSBitmapImageFileTypePNG : NSBitmapImageFileTypeTIFF;
		NSSize size = [self _imageSizeFromMenu];
		NSData *data = [_exportedView grabScreenshotOfApproxSize:size virtualSize:size dataFormat:format];
		if (!data)
			return;
		[manager removeItemAtPath:aPath error:nil];
		if ([data writeToFile:aPath atomically:YES]) {
			if (aExt)
				[manager setAttributes:[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:YES],
					NSFileExtensionHidden, nil] ofItemAtPath:aPath error:nil];
			_exportedImageFilename = nil;
			return;
		}
		title = [bundle localizedStringForKey:@"Not enough disk space to export this file" value:@"" table:nil];
		message = [bundle localizedStringForKey:@"Free some disk space and try again." value:@"" table:nil];
	}
	NSRunAlertPanel(title, @"%@", [bundle localizedStringForKey:@"Ok" value:@"" table:nil], nil, nil, message);
}

- (id)_requiredFileType
{
	NSInteger index = [_exportImageFormatPopUp indexOfSelectedItem];
	return index >= 0 && index < 3 ? _fileTypes[index] : @"";
}

// The custom fields' size, or the menu's (swapped for portrait).
- (NSSize)_imageSizeFromMenu
{
	NSSize size = NSZeroSize;
	if ([_exportImageOutSizePopUp selectedTag] == kCustomSizeTag)
		size = NSMakeSize([_exportImageWidthField intValue], [_exportImageHeightField intValue]);
	NSInteger index = [_exportImageOutSizePopUp indexOfSelectedItem];
	if (index >= 0 && index < (NSInteger)(sizeof(_menuSizes) / sizeof(_menuSizes[0])))
		size = _menuSizes[index];
	if ([_exportImageOutSizePopUp menu] == _exportImagePortraitOutSizeMenu)
		size = NSMakeSize(size.height, size.width);
	return size;
}

// Export needs digits only, both sides non-zero and at most 12288.
- (void)_validateCustomSize
{
	NSCharacterSet *nonDigits = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
	NSRange widthRange = [[_exportImageWidthField stringValue] rangeOfCharacterFromSet:nonDigits];
	NSRange heightRange = [[_exportImageHeightField stringValue] rangeOfCharacterFromSet:nonDigits];
	BOOL nonZero = [_exportImageHeightField intValue] && [_exportImageWidthField intValue];
	BOOL small = [_exportImageHeightField intValue] <= kMaxImageSide && [_exportImageWidthField intValue] <= kMaxImageSide;
	BOOL digits = widthRange.location == NSNotFound && widthRange.length == 0
		&& heightRange.location == NSNotFound && heightRange.length == 0;
	[[(WGExportPanel *)[_exportImageAccessory window] okButton] setEnabled:digits && nonZero && small];
}

@end
