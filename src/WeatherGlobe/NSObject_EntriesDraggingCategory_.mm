#import "DragSourceTableView.h"
#import "NSPasteboard+NSPasteboardWGExtensions.h"

NSString *_kEntriesListPasteboardType = @"com.mackiev.3dweatherglobe2.EntriesListPasteboardType";
NSString *_kPointsPasteboardType = @"com.mackiev.3dweatherglobe2.PointsPasteboardType";

@implementation NSObject (EntriesDraggingCategory)

// Promises the entries as strings plus the additional types; the dragging
// helper provides them.
- (void)declareEntries:(TLArray<GenericEntry *> *)aEntries inPasteboard:(id)aPasteboard withAdditionTypes:(id)aTypes
{
	if (![self conformsToProtocol:@protocol(EntriesDraggingHelperProtocol)])
		return;
	[aPasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:self];
	[aPasteboard addTypes:aTypes owner:self];
	[(id<EntriesDraggingHelperProtocol>)self setDraggedEntries:aEntries];
}

// The dragged entries as dictionaries, as their captions (a line each) or
// as the point entries' geo points.
- (void)provideEntriesDataForType:(id)aType toPasteboard:(id)aPasteboard
{
	if (![self conformsToProtocol:@protocol(EntriesDraggingHelperProtocol)])
		return;
	TLArray<GenericEntry *> *entries = [(id<EntriesDraggingHelperProtocol>)self draggedEntries];
	int count = entries ? (int)entries->count() : 0;
	if ([aType isEqual:_kEntriesListPasteboardType]) {
		LArray *list = LArray::array();
		for (int i = 0; i < count; i++)
			list->addObject(entries->objectAtIndex(i)->entryDictionary());
		[aPasteboard wgSetPropertyList:list forType:_kEntriesListPasteboardType];
	} else if ([aType isEqual:NSStringPboardType]) {
		NSMutableString *string = nil;
		for (int i = 0; i < count; i++) {
			NSString *caption = (NSString *)entries->objectAtIndex(i)->shortCaption(0)->cfString();
			if (!string) {
				string = [NSMutableString stringWithString:caption];
			} else {
				[string appendString:@"\n"];
				[string appendString:caption];
			}
		}
		[aPasteboard setString:string forType:NSStringPboardType];
	} else if ([aType isEqual:_kPointsPasteboardType]) {
		NSMutableData *points = [NSMutableData dataWithCapacity:count * sizeof(LPoint)];
		for (int i = 0; i < count; i++) {
			GenericEntry *entry = entries->objectAtIndex(i);
			if (entry && entry->getClass()->isSubclassOf(LF_CLASSOF(PointEntry))) {
				LPoint point = static_cast<PointEntry *>(entry)->geoPoint(0);
				[points appendBytes:&point length:sizeof(LPoint)];
			}
		}
		[aPasteboard setData:points forType:_kPointsPasteboardType];
	}
}

@end
