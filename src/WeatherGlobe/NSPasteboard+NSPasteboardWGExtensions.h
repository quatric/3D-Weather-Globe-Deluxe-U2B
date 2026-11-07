#import "WeatherGlobePrefix.h"


@interface NSPasteboard (NSPasteboardWGExtensions)
- (BOOL)wgSetPropertyList:(LObject *)aList forType:(id)aType;
- (LObject *)wgPropertyListForType:(id)aType;
@end

