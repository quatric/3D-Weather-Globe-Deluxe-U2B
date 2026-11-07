// Shared declarations for the WeatherGlobe sources.
#import "WGSystem.h"
#include "RenderLib.h"
#import "NSString_LStringExtension.h"
#import "C3DViewWrapper.h"
#import "NSObjToLObjWrapper.h"
#import "SMKControlsUmbrella.h"

// The protocols' methods are those of the binary's protocol objects.
@protocol PrintDataSourceProtocol <NSObject>
@end
@protocol EntriesDraggingHelperProtocol
- (void)setDraggedEntries:(TLArray<GenericEntry *> *)aEntries;
- (TLArray<GenericEntry *> *)draggedEntries;
@end
@protocol PrintDataSourceProviderProtocol
- (void)preparePrintingDidEndSelector:(SEL)aSelector target:(id)aTarget;
- (id)printDataSources;
- (id)hostWindowForPrintDialog;
- (id)customPrintDialog;
- (id)printJobTitle;
@end
@protocol WGAutoresizeDelegate
- (void)view:(id)aView didResizeWithOldSize:(NSSize)aSize;
@end
@protocol WGBezierPathCellProtocol <NSObject>
- (BOOL)containsPoint:(NSPoint)aPoint inRect:(NSRect)aRect ofView:(id)aView;
- (void)setDefaultCellBounds:(NSRect)aBounds;
- (void)setHighlightColor:(id)aColor;
- (void)setFillColor:(id)aColor;
- (void)setFrameColor:(id)aColor;
- (void)setBezierPath:(id)aPath;
@end
@protocol WGDragValidationDelegate
- (BOOL)view:(id)aView acceptDrop:(id)aInfo;
- (NSDragOperation)view:(id)aView validateDrop:(id)aInfo;
@end
@protocol WGEditableController
- (BOOL)tryOpenFieldEditor;
- (BOOL)isEditing;
- (void)validateEditing;
- (BOOL)canEdit;
@end
@protocol WGEntryOwner
- (GenericEntry *)currentEntry;
@end
@protocol WGMasterEditController <WGEditableController>
- (void)setIsEditing:(BOOL)aEditing;
@end
@protocol WGMountableControllerContainer
- (void)unmountController:(id)aController;
- (void)mountController:(id)aController;
@end
@protocol WGMountableControllerProtocol
- (BOOL)isActive;
- (BOOL)isMounted;
- (void)didMounted:(BOOL)aMounted;
- (void)setContainer:(id)aContainer;
- (id)container;
- (id)pane;
@end
@protocol WGRebuildLayoutProtocol
- (void)rebuildLayout;
- (void)rebuildLayout:(id)aSender;
@end

@class AboutController;
@class AppNameTextField;
@class AtlasApplication;
@class AtlasController;
@class AtlasGLView;
@class AtlasStartupDelegate;
@class BackgroundImageView;
@class BaseWeatherPalettePrintDataSource;
@class CIBackgroundImageView;
@class CIPaletteController;
@class CIPaletteTitleView;
@class CIPaletteView;
@class CIPaletteWindow;
@class CPCurrentConditionsView;
@class CPExpandedForecastView;
@class CPMainPropertyView;
@class CPStatusBarView;
@class CPWeatherDescriptionView;
@class CPWeatherGenericView;
@class CitiesListController;
@class CityInfoController;
@class CityInfoSelectionView;
@class CityInfoView;
@class CityPropertyAccessibilityObject;
@class CityPropertyGenericView;
@class CompanyLinkButton;
@class CompassAccessibilityObject;
@class CopyrightTextField;
@class DragSourceTableView;
@class EFOneDayController;
@class EntriesListController;
@class HSScrollView;
@class IAppsPasteboardDataProvider;
@class IPhotoAlbum;
@class IPhotoHelper;
@class IPhotoImportController;
@class IPhotoPhoto;
@class ImageViewAccessibilityIgnored;
@class LicenseBackgroundImageView;
@class LicenseWindow;
@class LinkButton;
@class LocationPrintDataSource;
@class MainPrintDataSourceProvider;
@class MainViewPrintDataSource;
@class MainWindowController;
@class MapDrawView;
@class NoMenuTextView;
@class PrefApearanceController;
@class PrefCloudsController;
@class PrefGeneralController;
@class PrefGenericController;
@class PrefGraphicsController;
@class PrefMyLocationController;
@class PrefOnlineController;
@class PrefScreenSaverController;
@class PreferencesController;
@class PreferencesToolbarItem;
@class PreferencesWindow;
@class PrintController;
@class PrintableTextView;
@class PrintingView;
@class RouteNodeTextFormatter;
@class RouteSetPrintDataSource;
@class SFOneDayController;
@class SFOneLocationController;
@class SecurityTextView;
@class SelectCityDialogController;
@class SelectCityWindow;
@class SnapshotEditorController;
@class SubscriptionController;
@class SubsribtionTextFormatter;
@class VersionTextField;
@class WBAlert;
@class WBAlertClearView;
@class WBAlertImageView;
@class WBAlertWindow;
@class WBHelpButton;
@class WBLicenseController;
@class WBTableRowsLayout;
@class WBTableView;
@class WGAccessibilityCustomToolbarButtonCell;
@class WGAccessibilityCustomToolbarOptionsButtonCell;
@class WGAccessibilityTableImageCell;
@class WGAccessibilityTableTextCell;
@class WGAutoAdjustableTextField;
@class WGBalloonPaletteController;
@class WGBalloonTexturedVew;
@class WGBezierPathCell;
@class WGBezierPathMatrix;
@class WGBorderedSplitView;
@class WGButtonCell;
@class WGCGImageCell;
@class WGCGImageRep;
@class WGCenteredTextAttachmentCell;
@class WGCenteredTextFieldCell;
@class WGCloudsPianino;
@class WGCloudsPianinoCell;
@class WGColorSelectionCell;
@class WGCommonEntryDetailsController;
@class WGCompassButton;
@class WGCompassCell;
@class WGCoordinateTextField;
@class WGCurrentSetBaloonTableView;
@class WGCurrentSetController;
@class WGCurrentSetTableView;
@class WGCustomButton;
@class WGCustomButtonCell;
@class WGCustomFieldEditor;
@class WGCustomImageView;
@class WGCustomLocationEditor;
@class WGCustomLocationFormatter;
@class WGCustomPaletteController;
@class WGCustomPopUpButtonCell;
@class WGCustomSlider;
@class WGCustomSliderCell;
@class WGCustomTableView;
@class WGCustomTitleView;
@class WGCustomToolbarButtonCell;
@class WGCustomToolbarController;
@class WGCustomToolbarMatrix;
@class WGDBUpdateController;
@class WGDragNDropView;
@class WGEFMatrix;
@class WGEFMatrixCell;
@class WGEFView;
@class WGEarthView;
@class WGEarthViewPlaceholder;
@class WGEntryDetailsComplexDescriptionDataProvider;
@class WGEntryDetailsController;
@class WGEntryDetailsCoordinatesEditingDataProvider;
@class WGEntryDetailsDateTimeDataProvider;
@class WGEntryDetailsDescriptionDataProvider;
@class WGEntryDetailsHeaderViewController;
@class WGEntryDetailsInfoDataProvider;
@class WGEntryDetailsInfoPaneController;
@class WGEntryDetailsInfoPaneView;
@class WGEntryDetailsInfoSimpleDataProvider;
@class WGEntryDetailsInfoTitlePopupDataProvider;
@class WGEntryDetailsKeyValueURLDataProvider;
@class WGEntryDetailsKeyValueURLEditingDataProvider;
@class WGEntryDetailsPaneController;
@class WGEntryDetailsUserInfoImagesController;
@class WGEntryDetailsWeatherDescriptionDataProvider;
@class WGEntryDetailsWeatherPaneController;
@class WGEntryInfoPopupController;
@class WGEntryInfoPopupView;
@class WGExportImageController;
@class WGExportPanel;
@class WGExtWeatherForecastPrintView;
@class WGFilledView;
@class WGGridControl;
@class WGHelpBook;
@class WGIPhotoImportActiveTexturedView;
@class WGImageAndTextCell;
@class WGImageTextCell;
@class WGImageTextTableCell;
@class WGLeftAreaController;
@class WGLeftPlaceholder;
@class WGLocationInfoPrintView;
@class WGLocationNameTextFieldCell;
@class WGMainEntriesListController;
@class WGMainWindow;
@class WGMountableController;
@class WGMovieController;
@class WGNewLocationController;
@class WGNibOwnerController;
@class WGPaletteBaseController;
@class WGPlaceholderView;
@class WGPopUpButton;
@class WGPopUpButtonCell;
@class WGPreferencesWrapper;
@class WGPrintRouteSetTableView;
@class WGPrintTableViewCell;
@class WGPrintTableViewPlaceholder;
@class WGPrintingEntryDetailsController;
@class WGPrintingEntryDetailsInfoPaneController;
@class WGRightPlaceholder;
@class WGRouteEntryDetailsController;
@class WGRouteImportExport;
@class WGRouteManagerActiveTexturedView;
@class WGRouteManagerController;
@class WGRouteManagerFlippedDocumentView;
@class WGRouteManagerOutlineView;
@class WGRouteManagerPanel;
@class WGRouteManagerTexturedView;
@class WGRouteSetPrintView;
@class WGSFWeatherPrintView;
@class WGSearchedEntriesTableView;
@class WGSerialNumberController;
@class WGSettingsPaletteController;
@class WGSplitView;
@class WGSplitter;
@class WGTextFieldCell;
@class WGTexturedButton;
@class WGTexturedView;
@class WGTimeLineCell;
@class WGTimeLineControl;
@class WGTimeLineController;
@class WGTitledView;
@class WGTitledViewContainer;
@class WGiMovieExporter;
@class WeatherPaletteExtPrintDataSource;
@class WeatherPalettePrintDataSourceProvider;
@class WeatherPaletteShortPrintDataSource;
@class _FolderName;
@class _WGBalloonPanel;
@class _WGBorderlessPanel;
@class _WGCITexturedView;
@class _WGFullScreenWindow;

class AppWindowInterfaceWrapper;

struct TParamsHolder {
    float f0;
    float f1;
    float f2;
    float f3;
    float f4;
    float f5;
};

// A cell of a matrix; -1, -1 for none.
struct _WGMatrixPosition {
    int row;
    int column;
};

struct _layout_info_t {
    int f0;
    int f1;
    int f2;
    unsigned short f3;
    unsigned char f4;
    unsigned char f5;
};
#include "WGAppClasses.h"
