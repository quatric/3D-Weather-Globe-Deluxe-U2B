#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsPaneController.h"


@interface WGEntryDetailsWeatherPaneController : WGEntryDetailsPaneController
{
    WGEFView *_extendedForecastfView;
    NSTextField *_genaralDayOfWeek;
    NSTextField *_genaralDateTime;
    NSTextField *_genaralTemperature;
    NSTextField *_genaralWindDirection;
    NSTextField *_genaralWindSpeed;
    NSTextField *_genaralHumidity;
    NSTextField *_genaralPressure;
    NSTextField *_genaralVisibility;
    NSImageView *_genaralSummaryImageView;
    NSProgressIndicator *_progressIndicator;
    WGEFMatrix *_weekdaysMatrix;
    id _weekdaysMatrixSelectedCell;
    NSView *_extendedForecastSubview;
    NSTextField *_extendedPrecipitation;
    NSTextField *_extendedUVIndex;
    NSTextField *_extendedDewPoint;
    NSTextField *_extendedFeelsLike;
    NSTextField *_extendedWindSpeed;
    NSTextField *_extendedHumidity;
    NSImageView *_extendedWindDirectionImageView;
    NSTextField *_extendedDescription;
    NSButton *_minimizeButton;
    BOOL _scheduled;
    BOOL _usesInternet;
    BOOL _minimized;
    PointEntry *_entry;
    PointEntry *_waitingEntry;
    NSTimer *_refreshTimer;
}
- (void)_reportsWereChanged:(id)aChanged;
- (void)_acceptAdjustTime:(id)aTime;
- (void)_clocksMeasureWereChanged:(id)aChanged;
- (void)_unitsOfMeasureWereChanged:(id)aChanged;
- (void)_internetStateWasChanged:(id)aChanged;
- (void)_timeForRefresh:(id)aRefresh;
- (void)_refreshWithCityEntry:(PointEntry *)aEntry;
- (void)_refresh;
- (void)_setExtendedForecast:(LMap *)aForecast;
- (void)_setCurrentConditions:(LMap *)aConditions;
- (id)_windImageForValue:(NSInteger)aValue;
- (id)_weatherImageFromObject:(LObject *)aObject;
- (void)_adjustTime;
- (void)_checkProgressIndicatorAnimation;
- (BOOL)_isWeatherExpired;
- (BOOL)_isExtendedForecastExpired;
- (BOOL)_isCurrentConditionExpired;
- (void)_unschedule;
- (void)_schedule;
- (void)_refreshToolTipRects;
- (void)_checkInternet;
- (void)_setupCellForField:(id)aField;
- (void)toggleMinimizeAction:(id)aAction;
- (id)view:(id)aView stringForToolTip:(NSInteger)aTip point:(NSPoint)aPoint userData:(void *)aData;
- (void)weekdayAction:(id)aAction;
- (void)awakeFromNib;
- (void)update;
- (BOOL)wantsUpdatesWhenInactive;
- (BOOL)isEnabled;
- (id)nibName;
- (id)iconName;
- (id)title;
- (void)dealloc;
- (id)initWithOwner:(id)aOwner;
@end

