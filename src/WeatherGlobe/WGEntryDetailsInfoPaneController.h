#import "WeatherGlobePrefix.h"
#import "WGEntryDetailsPaneController.h"


@interface WGEntryDetailsInfoPaneController : WGEntryDetailsPaneController <WGEditableController, WGRebuildLayoutProtocol, WGDragValidationDelegate>
{
    BOOL _isEditing;
    NSDictionary *_detailInfoDataProvidersClassMap;
    NSMutableDictionary *_detailInfoDataProvidersMap;
    TLArray<LF::LString*> *_orderedKeys;
    WGEntryDetailsUserInfoImagesController *_imagesContainer;
    NSMutableArray *_removableViews;
    BOOL _isInObjectValidation;
    id _savedFirstResponder;
    NSNumberFormatter *_unitsFormatter;
}
- (void)entryContentDidChange:(id)aChange;
- (void)validateObjectFromDataProvider:(id)aProvider;
- (id)imagesContainer;
- (NSInteger)indexOfDataProvider:(id)aProvider;
- (NSInteger)keySet;
- (void)prepareCommit;
- (void)_removeItem:(id)aItem;
- (void)_addItem:(id)aItem;
- (NSSize)_addButtonWithYCenterAt:(CGFloat)aAt xOffset:(CGFloat)aOffset title:(id)aTitle imageName:(id)aName providerKey:(id)aKey providerIndex:(NSInteger)aIndex action:(SEL)aAction addToArray:(id)aArray;
- (CGFloat)_insertAddRemoveButtonsAtPosition:(CGFloat)aPosition providerKey:(LString *)aKey providerIndex:(NSInteger)aIndex addToArray:(id)aArray;
- (CGFloat)_insertSeparatorAtPosition:(CGFloat)aPosition addToArray:(id)aArray;
- (id)_allDataProviders;
- (id)_createDataProvidersForKey:(LString *)aKey;
- (id)_createDataProviderWithClass:(Class)aClass forKey:(LString *)aKey index:(NSInteger)aIndex;
- (void)_rebuildEntryDetailsInfoDataProviders;
- (Class)_detailInfoEditingDataProviderClassForKey:(LString *)aKey;
- (Class)_detailInfoDataProviderClassForKey:(LString *)aKey;
- (BOOL)view:(id)aView acceptDrop:(id)aDrop;
- (NSUInteger)view:(id)aView validateDrop:(id)aDrop;
- (BOOL)tryOpenFieldEditor;
- (BOOL)isEditing;
- (void)validateEditing;
- (BOOL)canEdit;
- (void)rebuildLayout:(id)aLayout;
- (void)rebuildLayout;
- (EntryInfoController *)entryInfoController;
- (void)update;
- (void)entryDidChange;
- (void)entryWillChangeTo:(GenericEntry *)aTo;
- (id)title;
- (id)iconName;
- (void)awakeFromNib;
- (id)nibName;
- (void)dealloc;
- (id)initWithOwner:(id)aOwner;
@end

