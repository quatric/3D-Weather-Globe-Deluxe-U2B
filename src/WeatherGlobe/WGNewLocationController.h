#import "WeatherGlobePrefix.h"
#import "WGCustomPaletteController.h"


@interface WGNewLocationController : WGCustomPaletteController
{
    NSTextField *_locationNameField;
    SMKTintedView *_locationNameView;
    WGCoordinateTextField *_latitudeField;
    WGCoordinateTextField *_longitudeField;
    NSTextField *_timeZoneText;
    NSPopUpButton *_timeZonesPopup;
    NSPopUpButton *_coordsInputModePopup;
    WGTexturedView *_boxTexturedView;
    WGTexturedButton *_addLocationButton;
    WGCompassButton *_compassButton;
    WGFilledView *_separatorView;
    WGCustomLocationEditor *_customLocationEditor;
    BOOL _isLocationNameEntered;
    float _latitude;
    float _longitude;
    CoordinateParser *_coordsParser;
}
- (void)selectOnCompassLatitude:(NSInteger)aLatitude longitude:(NSInteger)aLongitude;
- (void)_adjustTime;
- (void)nullWasEntered;
- (void)parseNullCoordinate:(id)aCoordinate;
- (BOOL)isCoordsModeDMS;
- (BOOL)wgCustomLocationEditor:(id)aEditor pasteString:(id)aString;
- (void)textFieldDidChange:(id)aChange;
- (id)windowWillReturnFieldEditor:(id)aEditor toObject:(id)aObject;
- (BOOL)validateCoordinates;
- (void)parseCoordinate:(id)aCoordinate forTextField:(id)aField doUIRefresh:(BOOL)aRefresh;
- (void)parseCoordIfTextBoxBeingEdited;
- (void)refreshWorldPartButtons;
- (void)refreshEditFields;
- (id)timeZonePopupMenu;
- (void)setupCellForPopupButton:(id)aButton;
- (void)onHelpButton:(id)aButton;
- (void)coordEditorAction:(id)aAction;
- (void)degreesModeChanged:(id)aChanged;
- (void)timeZoneChanged:(id)aChanged;
- (void)compassButton:(id)aButton;
- (void)addLocationButton:(id)aButton;
- (void)setGeoCoords:(LPoint)aCoords;
- (void)setCoordsPickerEnabled:(BOOL)aEnabled;
- (BOOL)isCoordsPickerEnabled;
- (void)windowWillClose:(id)aClose;
- (void)dealloc;
- (void)awakeFromNib;
- (id)nibName;
@end

@interface WGLocationNameTextFieldCell : NSTextFieldCell
{
    NSAttributedString *_placeholderString;
}
- (void)drawInteriorWithFrame:(NSRect)aFrame inView:(id)aView;
- (void)dealloc;
- (id)initWithCell:(id)aCell;
@end

@interface WGCoordinateTextField : NSTextField
{
}
- (BOOL)becomeFirstResponder;
@end

@interface WGCustomLocationFormatter : NSFormatter
{
    id _delegate;
}
- (BOOL)isPartialStringValid:(id *)aValid proposedSelectedRange:(NSRange *)aRange originalString:(id)aString originalSelectedRange:(NSRange)aRange2 errorDescription:(id *)aDescription;
- (id)editingStringForObjectValue:(id)aValue;
- (id)attributedStringForObjectValue:(id)aValue withDefaultAttributes:(id)aAttributes;
- (BOOL)getObjectValue:(id *)aValue forString:(id)aString errorDescription:(id *)aDescription;
- (id)stringForObjectValue:(id)aValue;
- (void)setDelegate:(id)aDelegate;
- (id)removeAllNonDecimalsFromString:(id)aString excludeChars:(id)aChars;
@end

@interface WGCustomLocationEditor : NSTextView
{
    id _pasteDelegate;
}
- (void)setPasteDelegate:(id)aDelegate;
- (id)pasteDelegate;
- (void)paste:(id)aPaste;
@end

@interface NSMenuItem (WGTimeZoneNamesComparsion)
- (NSInteger)timeZoneComparisonFunc:(id)aFunc;
@end

