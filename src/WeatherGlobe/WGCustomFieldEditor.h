#import "WeatherGlobePrefix.h"


@interface WGCustomFieldEditor : NSTextView
{
    LUndoManager *_undoManager;
}
+ (id)sharedFieldEditor;
- (void)setUndoManager:(LUndoManager *)aManager;
- (void)redo:(id)aRedo;
- (void)undo:(id)aUndo;
- (BOOL)validateMenuItem:(id)aItem;
- (BOOL)isDataModified;
- (void)drawRect:(NSRect)aRect;
@end

