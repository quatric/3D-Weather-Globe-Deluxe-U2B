#import "WeatherGlobePrefix.h"
#import "WGCustomTableView.h"


@interface DragSourceTableView : WGCustomTableView
{
    BOOL _isBecomingFirstResponder;
}
- (void)accessibilityCellAction:(id)aAction;
- (id)accessibilityAttributeValue:(id)aValue;
- (BOOL)writeSelectionToPasteboard:(id)aPasteboard types:(id)aTypes;
- (id)validRequestorForSendType:(id)aType returnType:(id)aType2;
- (BOOL)becomeFirstResponder;
- (void)deleteForward:(id)aForward;
- (void)keyDown:(id)aDown;
- (void)draggedImage:(id)aImage endedAt:(NSPoint)aAt operation:(NSUInteger)aOperation;
- (NSUInteger)draggingSourceOperationMaskForLocal:(BOOL)aLocal;
@end

@interface NSObject (EntriesDraggingCategory)
- (void)provideEntriesDataForType:(id)aType toPasteboard:(id)aPasteboard;
- (void)declareEntries:(TLArray<GenericEntry*> *)aEntries inPasteboard:(id)aPasteboard withAdditionTypes:(id)aTypes;
@end

