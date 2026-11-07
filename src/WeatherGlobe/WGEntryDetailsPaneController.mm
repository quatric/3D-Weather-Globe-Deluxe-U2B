#import "WGEntryDetailsPaneController.h"
#import "WGEntryDetailsController.h"

// A pane of the entry details: follows the owner's current entry and
// updates while mounted (or always, if it wants updates when inactive).
@implementation WGEntryDetailsPaneController

- (id)initWithOwner:(id)aOwner
{
	self = [super init];
	if (self && (owner = aOwner)) {
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onEntryWillChangeTo:)
			name:_WGEntryDetailsCurrentEntryWillChangeNotification object:aOwner];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onEntryChanged)
			name:_WGEntryDetailsCurrentEntryDidChangeNotification object:aOwner];
	}
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}

// The new entry is a pointer value in the user info.
- (void)onEntryWillChangeTo:(id)aTo
{
	[self entryWillChangeTo:(GenericEntry *)[[[aTo userInfo] objectForKey:_WGEntryObjectKey] pointerValue]];
}

- (void)onEntryChanged
{
	if (![self isMounted] && ![self wantsUpdatesWhenInactive])
		return;
	[self loadNibIfNeeded];
	[self entryDidChange];
}

- (void)didMounted:(BOOL)aMounted
{
	[super didMounted:aMounted];
	if (![self wantsUpdatesWhenInactive])
		[self onEntryChanged];
}

- (id)owner
{
	return owner;
}

- (id)title
{
	return @"";
}

- (id)iconName
{
	return @"";
}

- (BOOL)isEnabled
{
	return [owner currentEntry] != 0;
}

- (void)update
{
}

- (BOOL)wantsUpdatesWhenInactive
{
	return NO;
}

- (void)entryWillChangeTo:(GenericEntry *)aTo
{
}

- (void)entryDidChange
{
	[self update];
}

- (EntryInfo *)entryInfo
{
	GenericEntry *entry = [[self owner] currentEntry];
	return entry ? entry->entryInfoObject() : 0;
}

- (GenericEntry *)entry
{
	return [[self owner] currentEntry];
}

- (void)rebuildLayout
{
}

- (void)rebuildLayout:(id)aLayout
{
	[self rebuildLayout];
}

@end
