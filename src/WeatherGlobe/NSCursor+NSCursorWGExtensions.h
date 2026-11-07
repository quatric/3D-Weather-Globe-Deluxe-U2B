#import "WeatherGlobePrefix.h"


@interface NSCursor (NSCursorWGExtensions)
+ (id)coordPickerCursor;
+ (id)pushPinCursor;
+ (id)zoomOutCursor;
+ (id)zoomInCursor;
+ (id)pointingHandBigCursor;
+ (id)_cursorWithImageName:(id)aName hotSpot:(NSPoint)aSpot;
@end

