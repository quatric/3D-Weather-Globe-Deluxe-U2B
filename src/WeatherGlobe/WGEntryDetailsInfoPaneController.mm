#import "WGEntryDetailsInfoPaneController.h"
#import "WGEntryDetailsController.h"
#import "WGEntryDetailsInfoSimpleDataProvider.h"
#import "WGEntryDetailsUserInfoImagesController.h"
#import "WGEntryDetailsDateTimeDataProvider.h"
#import "WGEntryDetailsKeyValueURLDataProvider.h"
#import "WGEntryDetailsKeyValueURLEditingDataProvider.h"
#import "WGEntryDetailsWeatherDescriptionDataProvider.h"
#import "WGEntryDetailsComplexDescriptionDataProvider.h"
#import "WGEntryDetailsCoordinatesEditingDataProvider.h"
#import "WGEntryDetailsDescriptionDataProvider.h"
#import "NSWindow+NSWindowWGExt.h"

// Layout of the info rows: the title column width and the margins.
static const CGFloat kTitleWidth = 100.0;
static const CGFloat kOffset = 6.0;
// The most info items an entry may get from the plus buttons.
static const NSUInteger kMaxDataProviders = 25;

static inline NSString *_nsKey(LString *aKey)
{
	return aKey ? (NSString *)aKey->cfString() : nil;
}

@implementation WGEntryDetailsInfoPaneController

- (id)initWithOwner:(id)aOwner
{
	self = [super initWithOwner:aOwner];
	if (self) {
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(validateEditing)
			name:_WGEntryDetailsControllerEditModeDidChangeNotification object:[self owner]];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(prepareCommit)
			name:_WGEntryDetailsControllerWillCommitEntryNotification object:[self owner]];
		_removableViews = [NSMutableArray new];
		[aOwner registerDraggedTypes:[[WGEntryDetailsUserInfoImagesController class] pasteboardTypes] forOwner:self];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	[[self owner] registerDraggedTypes:[[WGEntryDetailsUserInfoImagesController class] pasteboardTypes] forOwner:nil];
	if (_orderedKeys) {
		_orderedKeys->release();
		_orderedKeys = 0;
	}
	[_detailInfoDataProvidersMap release];
	[_detailInfoDataProvidersClassMap release];
	[_imagesContainer release];
	[_removableViews release];
	[_unitsFormatter release];
	[super dealloc];
}

- (id)nibName
{
	return @"EntryDetailsInfoPane";
}

- (void)awakeFromNib
{
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(rebuildLayout)
		name:NSViewFrameDidChangeNotification object:pane];
	[pane setPostsFrameChangedNotifications:YES];
	LNotificationCenter::center()->addObserver(self, @selector(update), Preferences::kUnitsOfMeasureWereChanged, 0);
}

- (id)iconName
{
	return @"icon_info_details";
}

- (id)title
{
	return [[NSBundle mainBundle] localizedStringForKey:@"Details" value:@"" table:nil];
}

// Takes the focus away from the pane's fields and remembers who had it.
- (void)_saveFirstResponderFromSubviews:(NSArray *)aSubviews
{
	id responder = [[[[[self pane] window] firstResponder] retain] autorelease];
	if ([aSubviews containsObject:responder] && [[[self pane] window] makeFirstResponder:nil])
		_savedFirstResponder = [responder retain];
}

- (void)entryWillChangeTo:(GenericEntry *)aTo
{
	LNotificationCenter::center()->removeObserver(self, EntryInfo::kDidChangeItemNotification, [self entryInfo]);
	NSArray *subviews = [[self pane] subviews];
	NSAssert(_savedFirstResponder == nil, @"_savedFirstResponder == nil");
	[self _saveFirstResponderFromSubviews:subviews];
	[subviews makeObjectsPerformSelector:@selector(removeFromSuperviewWithoutNeedingDisplay)];
}

- (void)entryDidChange
{
	EntryInfo *info = [self entryInfo];
	if (info)
		LNotificationCenter::center()->addObserver(self, @selector(entryContentDidChange:),
			EntryInfo::kDidChangeItemNotification, info);
	[self update];
}

- (void)update
{
	NSArray *subviews = [[self pane] subviews];
	if (!_savedFirstResponder)
		[self _saveFirstResponderFromSubviews:subviews];
	[subviews makeObjectsPerformSelector:@selector(removeFromSuperviewWithoutNeedingDisplay)];
	[self _rebuildEntryDetailsInfoDataProviders];
	[self rebuildLayout];
	[[self _allDataProviders] makeObjectsPerformSelector:@selector(didMountedToOwner:) withObject:self];
	if (_savedFirstResponder && [[[self pane] subviews] containsObject:_savedFirstResponder])
		[[[self pane] window] makeFirstResponder:_savedFirstResponder];
	[_savedFirstResponder release];
	_savedFirstResponder = nil;
}

- (EntryInfoController *)entryInfoController
{
	return [[self owner] entryInfoController];
}

- (void)rebuildLayout
{
	NSArray *subviews = [[self pane] subviews];
	NSMutableArray *removable = [NSMutableArray array];
	int keysCount = _orderedKeys ? _orderedKeys->count() : 0;
	CGFloat y = 0.0;
	BOOL prevHasTitle = YES;

	for (int i = 0; i < keysCount; i++) {
		LString *key = _orderedKeys->objectAtIndex(i);
		NSArray *providers = nil;
		NSString *nsKey = _nsKey(key);
		if (nsKey)
			providers = [_detailInfoDataProvidersMap objectForKey:nsKey];
		BOOL hasTitle = YES;
		NSUInteger count = [providers count];
		for (NSUInteger j = 0; j < count; j++) {
			id provider = [providers objectAtIndex:j];
			NSView *titleView = [provider titleView];
			NSView *descView = [provider descriptionView];
			NSRect titleFrame = titleView ? [titleView frame] : NSZeroRect;
			NSRect descFrame = descView ? [descView frame] : NSZeroRect;
			[provider viewSizesForSuperviewBounds:[[self pane] bounds] titleWidth:kTitleWidth offset:kOffset
				outTitleSize:titleView ? &titleFrame.size : NULL
				outDescriptionSize:descView ? &descFrame.size : NULL];

			hasTitle = titleView != nil;
			y += kOffset;
			// A separator goes between the groups with and without titles.
			if (prevHasTitle != hasTitle && j == 0 && i != 0)
				y = [self _insertSeparatorAtPosition:y addToArray:removable];

			if (!hasTitle) {
				if (descView) {
					descFrame.origin = NSMakePoint(kOffset, y);
					[descView setFrame:descFrame];
					if (![subviews containsObject:descView])
						[[self pane] addSubview:descView];
					y += descFrame.size.height;
				}
				continue;
			}

			CGFloat titleY = y;
			CGFloat rowHeight = titleFrame.size.height;
			if (descView) {
				descFrame.origin.x = kTitleWidth + kOffset;
				descFrame.origin.y = y;
				if (descFrame.size.height < titleFrame.size.height)
					descFrame.origin.y += trunc((titleFrame.size.height - descFrame.size.height) * 0.5);
				[descView setFrame:descFrame];
				if (![subviews containsObject:descView])
					[[self pane] addSubview:descView];
				rowHeight = MAX(titleFrame.size.height, descFrame.size.height);
			}
			y += rowHeight;
			titleFrame.origin.y = titleY;
			[titleView setFrame:titleFrame];
			if (![subviews containsObject:titleView])
				[[self pane] addSubview:titleView];
			[self _insertAddRemoveButtonsAtPosition:ceilf(titleFrame.size.height * 0.5) + titleY
				providerKey:key providerIndex:j addToArray:removable];
		}
		prevHasTitle = hasTitle;
	}

	NSRect frame = [[self pane] frame];
	[[self pane] setFrameSize:NSMakeSize(frame.size.width, y + kOffset)];
	[[self owner] rebuildLayout];
	[_removableViews makeObjectsPerformSelector:@selector(removeFromSuperview)];
	[_removableViews release];
	_removableViews = [removable retain];
	[[pane window] wgRecalculateKeyViewLoop];
}

- (void)rebuildLayout:(id)aLayout
{
	[self rebuildLayout];
}

- (BOOL)canEdit
{
	return [self entryInfo] != 0;
}

- (void)validateEditing
{
	if (_isEditing != [[self owner] isEditing]) {
		[[[self pane] subviews] makeObjectsPerformSelector:@selector(removeFromSuperviewWithoutNeedingDisplay)];
		_isEditing = [[self owner] isEditing];
		[self update];
	}
}

- (BOOL)isEditing
{
	return _isEditing;
}

- (NSUInteger)view:(id)aView validateDrop:(id)aDrop
{
	return [[self imagesContainer] view:aView validateDrop:aDrop];
}

- (BOOL)view:(id)aView acceptDrop:(id)aDrop
{
	BOOL accepted = [[self imagesContainer] view:aView acceptDrop:aDrop];
	if (accepted)
		[[self owner] setIsEditing:YES];
	return accepted;
}

- (Class)_detailInfoEditingDataProviderClassForKey:(LString *)aKey
{
	if (kEntryInfoURLKey->isEqualTo(aKey))
		return [WGEntryDetailsKeyValueURLEditingDataProvider class];
	if (kEntryCoordinatesPropertyKey->isEqualTo(aKey))
		return [WGEntryDetailsCoordinatesEditingDataProvider class];
	if (kEntryInfoDescriptionKey->isEqualTo(aKey))
		return [WGEntryDetailsDescriptionDataProvider class];
	return [self _detailInfoDataProviderClassForKey:aKey];
}

- (id)_createDataProviderWithClass:(Class)aClass forKey:(LString *)aKey index:(NSInteger)aIndex
{
	LObject *item = [self entryInfoController]->itemForKeyAtIndex(aKey, (int)aIndex,
		(TEntryKeySet)[self keySet], (TEntryKeyAccessMode)([self isEditing] != NO));
	return [[aClass alloc] initWithOwner:self key:aKey dataObject:item];
}

// Returns a retained array.
- (id)_createDataProvidersForKey:(LString *)aKey
{
	Class providerClass = [self isEditing] ? [self _detailInfoEditingDataProviderClassForKey:aKey]
		: [self _detailInfoDataProviderClassForKey:aKey];
	int count = [self entryInfoController]->itemsCountForKey(aKey, (TEntryKeySet)[self keySet],
		(TEntryKeyAccessMode)([self isEditing] != NO));
	if (!count)
		return [NSArray new];

	if ([providerClass isSubclassOfClass:[WGEntryDetailsInfoSimpleDataProvider class]]) {
		NSMutableArray *providers = [[NSMutableArray alloc] initWithCapacity:count];
		for (int i = 0; i < count; i++)
			[providers addObject:[[self _createDataProviderWithClass:providerClass forKey:aKey index:i] autorelease]];
		return providers;
	}
	if ([providerClass isSubclassOfClass:[WGEntryDetailsUserInfoImagesController class]]) {
		[[self imagesContainer] update];
		return [[NSArray alloc] initWithObjects:[self imagesContainer], nil];
	}
	id provider = [[[providerClass alloc] initWithOwner:self key:aKey] autorelease];
	return [[NSArray alloc] initWithObjects:provider, nil];
}

- (id)_allDataProviders
{
	NSMutableArray *providers = [NSMutableArray array];
	NSArray *groups = [_detailInfoDataProvidersMap allValues];
	int count = (int)[groups count];
	for (int i = 0; i < count; i++)
		[providers addObjectsFromArray:[groups objectAtIndex:i]];
	return providers;
}

- (CGFloat)_insertSeparatorAtPosition:(CGFloat)aPosition addToArray:(id)aArray
{
	NSRect bounds = [[self pane] bounds];
	NSBox *box = [[NSBox alloc] initWithFrame:NSMakeRect(kOffset, aPosition, bounds.size.width - 12.0, 12.0)];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	[box setBorderType:NSLineBorder];
#pragma clang diagnostic pop
	[box setTitlePosition:NSNoTitle];
	[box setBoxType:NSBoxSeparator];
	[[self pane] addSubview:box];
	[aArray addObject:box];
	[box release];
	return aPosition + [box bounds].size.height;
}

- (NSSize)_addButtonWithYCenterAt:(CGFloat)aAt xOffset:(CGFloat)aOffset title:(id)aTitle imageName:(id)aName providerKey:(id)aKey providerIndex:(NSInteger)aIndex action:(SEL)aAction addToArray:(id)aArray
{
	NSImage *image = [NSImage imageNamed:aName];
	if (!image)
		return NSZeroSize;
	NSSize size = [image size];
	NSButton *button = [[NSButton alloc] initWithFrame:NSMakeRect(aOffset, aAt - floorf(size.height * 0.5), size.width, size.height)];
	[button setTitle:aTitle];
	[button setImage:image];
	[button setButtonType:NSButtonTypeMomentaryLight];
	[button setBezelStyle:NSBezelStyleShadowlessSquare];
	[button setBordered:NO];
	[button setTag:aIndex];
	[[button cell] setRepresentedObject:aKey];
	[button setTarget:self];
	[button setAction:aAction];
	[[self pane] addSubview:button];
	[aArray addObject:button];
	[button release];
	return [button frame].size;
}

- (CGFloat)_insertAddRemoveButtonsAtPosition:(CGFloat)aPosition providerKey:(LString *)aKey providerIndex:(NSInteger)aIndex addToArray:(id)aArray
{
	if (![self isEditing] || !aKey || ![self entryInfo])
		return 0.0;
	NSString *nsKey = _nsKey(aKey);
	int itemsCount = [self entryInfo]->itemsCountForKey(aKey, (TEntryKeySet)[self keySet], (TEntryKeyAccessMode)1);
	NSInteger providersCount = 0;
	if (nsKey)
		providersCount = [[_detailInfoDataProvidersMap objectForKey:nsKey] count];

	BOOL canAdd = providersCount - 1 == aIndex
		&& [self entryInfoController]->canAddItemsForKey(aKey, (TEntryKeySet)[self keySet]);
	// Unsaved items past the stored ones can go while more than one is shown.
	BOOL canRemove = aIndex < itemsCount
		? [self entryInfoController]->canRemoveItemForKeyAtIndex(aKey, (int)aIndex, (TEntryKeySet)[self keySet])
		: providersCount >= 2;

	CGFloat height = 0.0;
	CGFloat x = kOffset;
	if (canRemove) {
		NSString *title = [[NSBundle mainBundle] localizedStringForKey:@"RemoveWebLink Button Title" value:@"" table:nil];
		NSSize size = [self _addButtonWithYCenterAt:aPosition xOffset:kOffset title:title imageName:@"eii_minus"
			providerKey:nsKey providerIndex:aIndex action:@selector(_removeItem:) addToArray:aArray];
		x = size.width + kOffset + kOffset;
		height = size.height;
	}
	if (!canAdd)
		return height;
	NSString *title = [[NSBundle mainBundle] localizedStringForKey:@"AddWebLink Button Title" value:@"" table:nil];
	NSSize size = [self _addButtonWithYCenterAt:aPosition xOffset:x title:title imageName:@"eii_plus"
		providerKey:nsKey providerIndex:aIndex action:@selector(_addItem:) addToArray:aArray];
	return MAX(height, size.height);
}

- (Class)_detailInfoDataProviderClassForKey:(LString *)aKey
{
	if (!_detailInfoDataProvidersClassMap) {
		_detailInfoDataProvidersClassMap = [[NSDictionary dictionaryWithObjectsAndKeys:
			[WGEntryDetailsDateTimeDataProvider class], _nsKey(kEntryDateTimePropertyKey),
			[WGEntryDetailsDateTimeDataProvider class], _nsKey(kEntryGMTOffsetPropertyKey),
			[WGEntryDetailsKeyValueURLDataProvider class], _nsKey(kEntryInfoURLKey),
			[WGEntryDetailsKeyValueURLDataProvider class], _nsKey(kEntryInfoWebCamKey),
			[WGEntryDetailsKeyValueURLDataProvider class], _nsKey(kEntryInfoItemURLKey),
			[WGEntryDetailsWeatherDescriptionDataProvider class], _nsKey(kEntryWeatherForecastPropertyKey),
			[WGEntryDetailsComplexDescriptionDataProvider class], _nsKey(kEntryInfoDescriptionKey),
			[WGEntryDetailsUserInfoImagesController class], _nsKey(kEntryInfoImagesKey),
			nil] retain];
	}
	Class providerClass = [_detailInfoDataProvidersClassMap objectForKey:[NSString stringReferenceFromLString:aKey]];
	return providerClass ? providerClass : [WGEntryDetailsInfoSimpleDataProvider class];
}

- (BOOL)tryOpenFieldEditor
{
	int keysCount = _orderedKeys ? _orderedKeys->count() : 0;
	BOOL opened = NO;
	for (int i = 0; !opened && i < keysCount; i++) {
		NSArray *providers = [_detailInfoDataProvidersMap objectForKey:
			[NSString stringReferenceFromLString:_orderedKeys->objectAtIndex(i)]];
		int count = (int)[providers count];
		for (int j = 0; !opened && j < count; j++)
			opened = [[providers objectAtIndex:j] tryStartDescriptionEditing];
	}
	return opened;
}

- (void)_rebuildEntryDetailsInfoDataProviders
{
	if (!_detailInfoDataProvidersMap)
		_detailInfoDataProvidersMap = [NSMutableDictionary new];
	else {
		[[self _allDataProviders] makeObjectsPerformSelector:@selector(didMountedToOwner:) withObject:nil];
		[_detailInfoDataProvidersMap removeAllObjects];
	}

	TLArray<LString *> *keys = 0;
	EntryInfoController *controller = [self entryInfoController];
	if (controller)
		keys = (TLArray<LString *> *)controller->orderedKeys((TEntryKeySet)[self keySet],
			(TEntryKeyAccessMode)([self isEditing] != NO));
	if (_orderedKeys) {
		_orderedKeys->release();
		_orderedKeys = 0;
	}
	if (keys)
		keys->retain();
	_orderedKeys = keys;

	int count = keys ? keys->count() : 0;
	for (int i = 0; i < count; i++) {
		LString *key = _orderedKeys->objectAtIndex(i);
		NSString *nsKey = _nsKey(key);
		if (nsKey)
			[_detailInfoDataProvidersMap setObject:[[self _createDataProvidersForKey:key] autorelease] forKey:nsKey];
	}
}

- (void)_removeItem:(id)aItem
{
	if (![self isEditing])
		return;
	NSInteger index = [aItem tag];
	NSString *nsKey = [[aItem cell] representedObject];
	LString *key = nsKey ? LString::stringWithCFString((CFStringRef)nsKey) : 0;
	NSMutableArray *providers = [_detailInfoDataProvidersMap objectForKey:nsKey];
	id provider = [providers objectAtIndex:index];
	[provider validateDataObject];
	// An item that never reached the entry has to go by hand.
	if (![[self owner] entryInfoController]->removeItemForKeyAtIndex(key, (int)index, (TEntryKeySet)[self keySet])) {
		[[provider titleView] removeFromSuperviewWithoutNeedingDisplay];
		[[provider descriptionView] removeFromSuperviewWithoutNeedingDisplay];
		[providers removeObjectAtIndex:index];
		[self rebuildLayout];
	}
}

- (void)_addItem:(id)aItem
{
	if (![self isEditing] || [[self _allDataProviders] count] >= kMaxDataProviders)
		return;
	NSInteger index = [aItem tag];
	NSString *nsKey = [[aItem cell] representedObject];
	Class providerClass = [self _detailInfoEditingDataProviderClassForKey:[nsKey LString]];
	id provider = [[providerClass alloc] initWithOwner:self key:[nsKey LString] dataObject:0];
	[[_detailInfoDataProvidersMap objectForKey:nsKey] insertObject:provider atIndex:index + 1];
	[provider release];
	[self rebuildLayout];
}

- (void)prepareCommit
{
	[self entryWillChangeTo:[self entry]];
}

- (NSInteger)keySet
{
	return 0;
}

- (NSInteger)indexOfDataProvider:(id)aProvider
{
	NSString *nsKey = [NSString stringReferenceFromLString:[(WGEntryDetailsInfoDataProvider *)aProvider key]];
	return (int)[[_detailInfoDataProvidersMap objectForKey:nsKey] indexOfObject:aProvider];
}

- (id)imagesContainer
{
	if (!_imagesContainer)
		_imagesContainer = [[WGEntryDetailsUserInfoImagesController alloc] initWithOwner:self key:kEntryInfoImagesKey];
	return _imagesContainer;
}

// Follows the entry's item changes while editing, so the rows the user
// is typing in stay put.
- (void)entryContentDidChange:(id)aChange
{
	if (![self isEditing]) {
		[self update];
		return;
	}
	if (_isInObjectValidation)
		return;
	LNotification *notification = lobj_cast<LNotification>([aChange getLObject]);
	if (!notification)
		return;
	LMap *userInfo = notification->userInfo();
	if (!userInfo)
		return;
	LValue *keySet = lobj_cast<LValue>(userInfo->objectForKey(EntryInfo::kChangedKeySet));
	if (!keySet || keySet->intValue() != 0)
		return;
	LValue *indexValue = lobj_cast<LValue>(userInfo->objectForKey(EntryInfo::kChangedIndex));
	int index = indexValue ? indexValue->intValue() : 0;
	LString *key = lobj_cast<LString>(userInfo->objectForKey(EntryInfo::kChangedKey));
	LString *kind = lobj_cast<LString>(userInfo->objectForKey(EntryInfo::kOperationKind));

	Class providerClass = [self isEditing] ? [self _detailInfoEditingDataProviderClassForKey:key]
		: [self _detailInfoDataProviderClassForKey:key];
	if (![providerClass isSubclassOfClass:[WGEntryDetailsInfoSimpleDataProvider class]])
		return;
	NSMutableArray *providers = [_detailInfoDataProvidersMap objectForKey:[NSString stringReferenceFromLString:key]];

	BOOL replace = EntryInfo::kOperationKindReplace->isEqualTo(kind)
		|| (NSInteger)[providers count] == [self entryInfoController]->itemsCountForKey(key,
			(TEntryKeySet)[self keySet], (TEntryKeyAccessMode)1);
	if (replace) {
		id provider = [providers objectAtIndex:index];
		[provider setDataObject:[self entryInfoController]->itemForKeyAtIndex(key, index,
			(TEntryKeySet)[self keySet], (TEntryKeyAccessMode)([self isEditing] != NO))];
	}
	else if (EntryInfo::kOperationKindInsert->isEqualTo(kind)) {
		id provider = [self _createDataProviderWithClass:providerClass forKey:key index:index];
		[providers insertObject:provider atIndex:index];
		[provider release];
	}
	else if (EntryInfo::kOperationKindRemove->isEqualTo(kind)) {
		id provider = [providers objectAtIndex:index];
		[[provider titleView] removeFromSuperviewWithoutNeedingDisplay];
		[[provider descriptionView] removeFromSuperviewWithoutNeedingDisplay];
		[providers removeObjectAtIndex:index];
	}
	else
		return;
	[self rebuildLayout];
}

- (void)validateObjectFromDataProvider:(id)aProvider
{
	LUndoManager *undoManager = LUndoManager::instance();
	if (undoManager->isUndoing() || undoManager->isRedoing())
		return;
	_isInObjectValidation = YES;
	EntryInfoController *controller = [self entryInfoController];
	LString *key = [(WGEntryDetailsInfoDataProvider *)aProvider key];
	LObject *object = [aProvider dataObject];
	if (!object)
		controller->removeItemForKeyAtIndex(key, (int)[self indexOfDataProvider:aProvider], (TEntryKeySet)0);
	else
		controller->replaceItemForKeyAtIndex(object, key, (int)[self indexOfDataProvider:aProvider], (TEntryKeySet)0);
	_isInObjectValidation = NO;
}

@end
