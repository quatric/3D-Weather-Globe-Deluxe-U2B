#import "WeatherGlobePrefix.h"


@interface WGEntryInfoPopupView : NSView
{
    NSTextFieldCell *_nameCell;
    NSTextFieldCell *_headerCell;
    WGCGImageCell *_cgImageCell;
    NSTextFieldCell *_titleCell;
    NSTextFieldCell *_valueCell;
    NSString *_kind;
    NSString *_region;
    NSString *_userDescr;
    NSArray *_entryInfo;
    BOOL _isSizeValid;
    NSSize _sizeForBestFit;
    NSSize _bestSizeForTitles;
    NSSize _bestSizeForValues;
    NSSize _bestSizeForHeader;
}
- (void)drawRect:(NSRect)aRect;
- (BOOL)isFlipped;
- (NSSize)sizeForBestFit;
- (void)setEntryInfo:(id)aInfo;
- (void)setUserDescription:(id)aDescription;
- (void)setEntryRegion:(id)aRegion;
- (void)setEntryKind:(id)aKind;
- (void)setEntryName:(id)aName;
- (void)setEntryIcon:(struct CGImage *)aIcon;
- (void)dealloc;
- (id)initWithFrame:(NSRect)aFrame;
@end

