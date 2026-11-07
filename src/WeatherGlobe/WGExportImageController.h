#import "WeatherGlobePrefix.h"
#import "WGNibOwnerController.h"


@interface WGExportImageController : WGNibOwnerController
{
    NSView *_exportImageAccessory;
    NSPopUpButton *_exportImageOutSizePopUp;
    NSMenu *_exportImagePortraitOutSizeMenu;
    NSMenu *_exportImageLandscapeOutSizeMenu;
    NSPopUpButton *_exportImageFormatPopUp;
    NSMenu *_exportImageFormatMenu;
    NSButtonCell *_exportImagePortrait;
    NSButtonCell *_exportImageLandscape;
    NSTextField *_exportImageWidthField;
    NSTextField *_exportImageHeightField;
    NSString *_exportedImagesFolder;
    NSString *_exportedImageFilename;
    AtlasGLView *_exportedView;
}
- (NSSize)_imageSizeFromMenu;
- (id)_requiredFileType;
- (void)_validateCustomSize;
- (void)_saveScreenShootAtPath:(id)aPath hideExt:(BOOL)aExt;
- (void)_savePanelDidEnd:(id)aEnd returnCode:(NSInteger)aCode contextInfo:(void *)aInfo;
- (void)_imageFormatPopupChanged:(id)aChanged;
- (void)controlTextDidChange:(id)aChange;
- (void)exportImageModeChanged:(id)aChanged;
- (void)imageTypeChanged:(id)aChanged;
- (void)exportGlViewAsImage:(id)aImage;
- (id)glViewAsImageData:(id)aData forSize:(NSSize)aSize;
- (void)setExportedImageFilename:(id)aFilename;
- (id)exportedImageFilename;
- (id)defaultExportFolder;
- (id)nibName;
- (void)awakeFromNib;
- (void)dealloc;
@end

