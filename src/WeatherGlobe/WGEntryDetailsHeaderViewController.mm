#import "WGEntryDetailsHeaderViewController.h"
#import "WGEntryDetailsController.h"
#import "WGAutoAdjustableTextField.h"
#import "NSImage+NSImageWGExtensions.h"

// The head of the entry details: the entry's icon, name, kind and region,
// stacked to fit their text. While editing, the name and the region can be
// changed; the region is chosen from a combo box that searches the regions
// by the typed prefix.
@implementation WGEntryDetailsHeaderViewController

- (id)initWithOwner:(id)aOwner
{
	self = [super initWithOwner:aOwner];
	if (self) {
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(validateEditing)
			name:_WGEntryDetailsControllerEditModeDidChangeNotification object:[self owner]];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(validateData)
			name:_WGEntryDetailsControllerWillCommitEntryNotification object:[self owner]];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	LNotificationCenter::center()->removeObserver(self);
	if (_regionSearchResults) {
		_regionSearchResults->release();
		_regionSearchResults = 0;
	}
	if (_regionSearchPrefix) {
		_regionSearchPrefix->release();
		_regionSearchPrefix = 0;
	}
	[editStatusView release];
	[gotoButton release];
	[super dealloc];
}

- (id)nibName
{
	return @"EntryDetailsHeaderView";
}

// The edit status and goto views come and go, so they are kept.
- (void)awakeFromNib
{
	_initialHeight = [pane frame].size.height;
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(rebuildLayout)
		name:NSViewFrameDidChangeNotification object:pane];
	[pane setPostsFrameChangedNotifications:YES];
	[regionComboBox setBordered:NO];
	[regionComboBox setButtonBordered:NO];
	[regionComboBox setFocusRingType:NSFocusRingTypeNone];
	[[regionComboBox cell]
		setPlaceholderString:[[NSBundle mainBundle] localizedStringForKey:@"No Region" value:@"" table:nil]];
	[editStatusView retain];
	[gotoButton retain];
	[gotoButton sendActionOn:NSEventMaskLeftMouseDown];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(textDidEndEditing:)
		name:NSControlTextDidEndEditingNotification object:ownName];
	_isNibLoadingFinished = YES;
}

- (id)pane
{
	[self loadNibIfNeeded];
	return pane;
}

- (void)entryWillChangeTo:(GenericEntry *)aTo
{
	if ([ownName currentEditor])
		[[ownName window] endEditingFor:ownName];
	if ([regionComboBox currentEditor])
		[[regionComboBox window] endEditingFor:regionComboBox];
}

- (BOOL)canEdit
{
	return [[self owner] currentEntry] != nil;
}

- (BOOL)isEditing
{
	return [[self owner] isEditing];
}

// The name if editable, else the region box if shown.
- (BOOL)tryOpenFieldEditor
{
	id field = nil;
	if ([ownName isEditable])
		field = ownName;
	if (!field) {
		if (![regionComboBox isHidden])
			field = regionComboBox;
		if (!field)
			return NO;
	}
	return [[ownName window] makeFirstResponder:field];
}

- (void)prepareCommit
{
	[self validateData];
}

// The name (with the goto button after it), the kind, then the region,
// 10 points from the top; the pane grows to fit them.
- (void)rebuildLayout
{
	if (!_isNibLoadingFinished)
		return;
	[self loadNibIfNeeded];
	NSRect frame = [pane frame];
	NSRect statusFrame = [editStatusView frame];
	[editStatusView setFrameOrigin:NSMakePoint(frame.size.width - statusFrame.size.width, 0.0)];
	float y;
	if ([gotoButton superview]) {
		NSRect gotoFrame = [gotoButton frame];
		float height = [self layoutControl:ownName withYOffset:10.0
			withWidth:frame.size.width - gotoFrame.size.width - 1.0];
		y = MAX((float)gotoFrame.size.height + 2.0f, height);
		NSRect nameFrame = [ownName frame];
		float x = floorf(nameFrame.origin.x + nameFrame.size.width + 1.0f);
		float gotoY = floorf((nameFrame.size.height - gotoFrame.size.height) * 0.5f + nameFrame.origin.y);
		[gotoButton setFrame:NSMakeRect(x, gotoY, gotoFrame.size.width, gotoFrame.size.height)];
	} else {
		y = [self layoutControl:ownName withYOffset:10.0 withWidth:frame.size.width];
	}
	y += 10.0f;
	y += [self layoutControl:ownType withYOffset:y withWidth:frame.size.width];
	id region = [self isEditing] && ![regionComboBox isHidden] ? (id)regionComboBox : (id)hostCountry;
	float height = [self layoutControl:region withYOffset:y withWidth:frame.size.width] + 4.0f + y;
	if (height < _initialHeight)
		height = _initialHeight;
	if (height != frame.size.height) {
		[pane setFrameSize:NSMakeSize(frame.size.width, height)];
		[[self owner] rebuildLayout];
	}
	[pane setNeedsDisplay:YES];
}

- (void)rebuildLayout:(id)aLayout
{
	[self rebuildLayout];
}

- (void)gotoCurrentLocation:(id)aLocation
{
	if ([[self owner] isGotoLocationEnabled])
		[[self owner] gotoCurrentLocation];
}

// Fits a text field to its text across the width (less the edit status
// while editing); an empty one not being edited takes no room. Returns the
// height taken.
- (CGFloat)layoutControl:(id)aControl withYOffset:(CGFloat)aOffset withWidth:(CGFloat)aWidth
{
	NSRect frame = [aControl frame];
	float height = frame.size.height;
	float fieldHeight = WGEntryDetailsInfoDefaultFieldSize.height;
	float width = aWidth - frame.origin.x - 1.0f;
	if ([self isEditing])
		width -= [editStatusView frame].size.width;
	if ([aControl isMemberOfClass:[NSTextField class]] || [aControl isMemberOfClass:[WGAutoAdjustableTextField class]]) {
		NSSize size;
		if ([aControl respondsToSelector:@selector(sizeForProposedSize:)])
			size = [aControl sizeForProposedSize:NSMakeSize(width, fieldHeight)];
		else
			size = [[aControl cell] cellSizeForBounds:NSMakeRect(0.0, 0.0, width, fieldHeight)];
		width = size.width;
		height = size.height;
		if ((![self isEditing] || ![aControl isEditable]) && ![[aControl stringValue] length])
			height = 0.0f;
	}
	[aControl setFrame:NSMakeRect(frame.origin.x, aOffset, width, height)];
	return height != 0.0f ? height + 1.0f : 0.0f;
}

// The typed text up to the ", country" a chosen item shows.
- (id)regionComboBoxSearchPrefix
{
	NSString *string = [regionComboBox stringValue];
	NSUInteger comma = [string rangeOfString:@","].location;
	if (comma != NSNotFound)
		string = [string substringToIndex:comma];
	return string;
}

- (void)regionComboBoxTextDidChange:(id)aChange
{
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(refineRegionSearchIfNeeded) object:nil];
	[self performSelector:@selector(refineRegionSearchIfNeeded) withObject:nil afterDelay:0.5
		inModes:[NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, nil]];
}

- (void)comboBoxWillPopUp:(id)aUp
{
	_isRegionsListPoppedUp = YES;
	[self refineRegionSearchIfNeeded];
}

- (void)comboBoxWillDismiss:(id)aDismiss
{
	_isRegionsListPoppedUp = NO;
}

- (void)textDidEndEditing:(id)aEditing
{
	[self validateData];
}

- (NSInteger)numberOfItemsInComboBox:(id)aBox
{
	return _regionSearchResults ? _regionSearchResults->count() : 0;
}

// A single match is taken when the typing ends.
- (void)regionComboBoxTextDidEndEditing:(id)aEditing
{
	[self refineRegionSearchIfNeeded];
	if (_regionSearchResults && _regionSearchResults->count() == 1) {
		[regionComboBox selectItemAtIndex:0];
		[regionComboBox setStringValue:[self comboBox:regionComboBox objectValueForItemAtIndex:0]];
	}
}

- (void)update
{
	EntryInfoController *info = [[self owner] entryInfoController];
	LString *name = 0, *preferredName = 0, *kind = 0, *region = 0;
	LImage *icon = 0;
	if (info) {
		name = lobj_cast<LString>(info->itemForKeyAtIndex(kEntryOwnNamePropertyKey, 0, (TEntryKeySet)1,
			(TEntryKeyAccessMode)0));
		preferredName = lobj_cast<LString>(info->itemForKeyAtIndex(kEntryPreferredNamePropertyKey, 0,
			(TEntryKeySet)1, (TEntryKeyAccessMode)0));
		kind = lobj_cast<LString>(info->itemForKeyAtIndex(kEntryKindPropertyKey, 0, (TEntryKeySet)1,
			(TEntryKeyAccessMode)0));
		region = lobj_cast<LString>(info->itemForKeyAtIndex(kEntryRegionPropertyKey, 0, (TEntryKeySet)1,
			(TEntryKeyAccessMode)0));
		GenericEntry *entry = info->entry();
		if (entry)
			icon = entry->icon(0);
	}
	[ownName setStringValue:name ? (NSString *)name->cfString() : @""];
	[[ownName cell] setPlaceholderString:preferredName ? (NSString *)preferredName->cfString() : nil];
	[ownType setStringValue:kind ? (NSString *)kind->cfString() : @""];
	[hostCountry setStringValue:region ? (NSString *)region->cfString() : @""];
	[regionComboBox setStringValue:region ? (NSString *)region->cfString() : @""];
	[imageView setImage:[NSImage wgImageWithLImage:icon]];
	[self validateEditing];
}

// Searches the regions again when the prefix is a new string. Returns
// whether it did.
- (BOOL)searchForRegionWithPrefix:(LString *)aPrefix
{
	if (_regionSearchPrefix == aPrefix && (!aPrefix || aPrefix->isEqualTo(aPrefix)))
		return NO;
	if (_regionSearchResults) {
		_regionSearchResults->release();
		_regionSearchResults = 0;
	}
	if (_regionSearchPrefix) {
		_regionSearchPrefix->release();
		_regionSearchPrefix = 0;
	}
	if (aPrefix)
		aPrefix->retain();
	_regionSearchPrefix = aPrefix;
	LString *request = SQL_DB_Strings::searchForGeoObjectByName(aPrefix, 0xe);
	_regionSearchResults = (TLArray<LMap *> *)AtlasDatabase::instance()->simpleSearchWithRequest(request);
	if (_regionSearchResults)
		_regionSearchResults->retain();
	return YES;
}

// Pops the list up with the new matches.
- (void)refineRegionSearchIfNeeded
{
	LString *prefix = LString::stringWithCFString((CFStringRef)[self regionComboBoxSearchPrefix]);
	if ([self searchForRegionWithPrefix:prefix]) {
		[regionComboBox reloadData];
		if (!_isRegionsListPoppedUp)
			[[[regionComboBox cell] valueForKey:@"buttonCell"] performClick:nil];
	}
}

// "Region, parent region" of the found entry.
- (id)comboBox:(id)aBox objectValueForItemAtIndex:(NSInteger)aIndex
{
	NSString *string = @"";
	LString *parent = 0;
	LMap *map = _regionSearchResults ? _regionSearchResults->objectAtIndex((int)aIndex) : 0;
	if (map) {
		LString *uid = lobj_cast<LString>(map->objectForKey(kColumnNameUID));
		GenericEntry *entry = uid ? GenericEntry::entryWithUID(uid) : 0;
		if (entry) {
			LString *name = entry->ownName(0);
			parent = entry->regionName(0);
			if (name)
				string = (NSString *)name->cfString();
		}
	}
	if (parent && parent->length())
		return [string stringByAppendingFormat:@", %@", (NSString *)parent->cfString()];
	return string;
}

// Name editable and the region box shown while editing an entry that has
// those keys; the region box is watched only then.
- (void)validateEditing
{
	[self validateData];
	BOOL editing = [self isEditing];
	EntryInfoController *info = [[self owner] entryInfoController];
	BOOL nameEditable = NO, regionEditable = NO;
	if (info && editing) {
		LArray *keys = info->orderedKeys((TEntryKeySet)1, (TEntryKeyAccessMode)1);
		if (keys) {
			nameEditable = keys->indexOfObject(kEntryOwnNamePropertyKey) != -1;
			regionEditable = keys->indexOfObject(kEntryRegionPropertyKey) != -1;
		}
	}
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	if (!editing) {
		[editStatusView removeFromSuperview];
		if ([[self owner] isGotoLocationEnabled])
			[pane addSubview:gotoButton];
		else
			[gotoButton removeFromSuperview];
		[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(refineRegionSearchIfNeeded)
			object:nil];
		[center removeObserver:self name:NSControlTextDidChangeNotification object:regionComboBox];
		[center removeObserver:self name:NSControlTextDidEndEditingNotification object:regionComboBox];
	} else {
		[pane addSubview:editStatusView];
		[gotoButton removeFromSuperview];
		[center addObserver:self selector:@selector(regionComboBoxTextDidChange:)
			name:NSControlTextDidChangeNotification object:regionComboBox];
		[center addObserver:self selector:@selector(regionComboBoxTextDidEndEditing:)
			name:NSControlTextDidEndEditingNotification object:regionComboBox];
	}
	if ([ownName currentEditor])
		[[ownName window] endEditingFor:ownName];
	[ownName setEditable:nameEditable];
	[regionComboBox setHidden:!regionEditable];
	[hostCountry setHidden:regionEditable];
	[self rebuildLayout];
}

// Stores the edited name, and the chosen region's UID (or the typed text
// when none was chosen and it changed).
- (void)validateData
{
	EntryInfoController *info = [[self owner] entryInfoController];
	if (!info)
		return;
	LArray *keys = info->orderedKeys((TEntryKeySet)1, (TEntryKeyAccessMode)1);
	BOOL nameEditable = keys && keys->indexOfObject(kEntryOwnNamePropertyKey) != -1;
	BOOL regionEditable = keys && keys->indexOfObject(kEntryRegionPropertyKey) != -1;
	if (nameEditable)
		info->replaceItemForKeyAtIndex(LString::stringWithCFString((CFStringRef)[ownName stringValue]),
			kEntryOwnNamePropertyKey, 0, (TEntryKeySet)1);
	if (!regionEditable)
		return;
	NSInteger index = [regionComboBox indexOfSelectedItem];
	LString *value = 0;
	if (index == -1) {
		if ([[regionComboBox stringValue] isEqual:[hostCountry stringValue]])
			return;
		value = LString::stringWithCFString((CFStringRef)[regionComboBox stringValue]);
	} else if (_regionSearchResults && index < _regionSearchResults->count()) {
		LMap *map = _regionSearchResults->objectAtIndex((int)index);
		if (map)
			value = lobj_cast<LString>(map->objectForKey(kColumnNameUID));
	}
	info->replaceItemForKeyAtIndex(value, kEntryRegionPropertyKey, 0, (TEntryKeySet)1);
}

@end
