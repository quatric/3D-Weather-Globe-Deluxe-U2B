#import "WGLeftAreaController.h"
#import "WGMainEntriesListController.h"
#import "WGCurrentSetController.h"
#import "WGCommonEntryDetailsController.h"
#import "WGCustomButton.h"
#import "WGSplitView.h"
#import "WGTitledView.h"

@interface NSObject (WGLeftAreaControllerDelegate)
- (void)contentDidChangeInController:(id)aController;
@end

// The main window's left area: the entries list over the current route
// set and the entry details panes (each shown as chosen), their titled
// views' attributes from Panes.plist.
@implementation WGLeftAreaController

- (id)init
{
	self = [super init];
	if (self) {
		NSDictionary *prefs = [NSDictionary dictionaryWithContentsOfFile:
			[[NSBundle mainBundle] pathForResource:@"Panes" ofType:@"plist"]];
		_isSettingsMode = YES;
		_panesPrefs = [prefs retain];
	}
	return self;
}

- (void)dealloc
{
	[_entriesListController release];
	[_currentSetController release];
	[_entryDetailsController release];
	[resourcesView release];
	[_panesPrefs release];
	[super dealloc];
}

- (void)awakeFromNib
{
	[self setupTitleButtons];
	[self performLayout];
}

- (void)splitView:(id)aView resizeSubviewsWithOldSize:(NSSize)aSize
{
	[aView wgCustomAdjustSubviews:NO];
}

- (CGFloat)splitView:(id)aView constrainMinCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt
{
	return [aView wgConstrainMinCoordinate:aCoordinate ofSubviewAt:aAt];
}

- (CGFloat)splitView:(id)aView constrainMaxCoordinate:(CGFloat)aCoordinate ofSubviewAt:(NSInteger)aAt
{
	return [aView wgConstrainMaxCoordinate:aCoordinate ofSubviewAt:aAt];
}

// A pane closed by its title bar is hidden.
- (void)removeSubview:(id)aSubview
{
	if ([aSubview contentView] == [_currentSetController pane])
		[self showCurrentSet:NO];
	else if ([aSubview contentView] == [_entryDetailsController pane])
		[self showDetails:NO];
}

- (void)mountController:(id)aController
{
	if (aController == _currentSetController) {
		[self switchToSettingsMode:NO];
		[self showCurrentSet:YES];
	} else if (aController == _entryDetailsController) {
		[self switchToSettingsMode:NO];
		[self showDetails:YES];
	}
}

- (void)unmountController:(id)aController
{
	if (aController == _currentSetController)
		[self showCurrentSet:NO];
	else if (aController == _entryDetailsController)
		[self showDetails:NO];
}

- (void)performLayout
{
	[self switchToSettingsMode:NO];
}

- (void)setupTitleButton:(id)aButton withBackgroundName:(id)aName altBackgroundName:(id)aName2
	foregroundName:(id)aName3 altForegroundName:(id)aName4
{
	[aButton addImage:[NSImage imageNamed:aName] altImage:[NSImage imageNamed:aName2] align:0 scale:1];
	[aButton addImage:[NSImage imageNamed:aName3] altImage:[NSImage imageNamed:aName4] align:0 scale:2];
}

// The panes' attributes become immutable.
- (void)setupTitleButtons
{
	NSMutableDictionary *prefs = [[_panesPrefs mutableCopy] autorelease];
	[_panesPrefs release];
	_panesPrefs = [[NSDictionary dictionaryWithDictionary:prefs] retain];
}

- (void)showPane:(BOOL)aPane forController:(id)aController
{
	if (!aController)
		return;
	NSView *pane = [aController pane];
	if (aPane)
		[super addSubview:pane withAttributes:[_panesPrefs objectForKey:[aController wgIdentifier]]];
	else
		[super removeSubview:pane];
	if ([aController isKindOfClass:[WGMountableController class]])
		[aController didMounted:aPane];
	if ([delegate respondsToSelector:@selector(contentDidChangeInController:)])
		[delegate contentDidChangeInController:self];
}

- (BOOL)isDetailsPaneVisible
{
	return Preferences::isInfoAreaVisible();
}

- (BOOL)isCurrentSetPaneVisible
{
	return Preferences::isCurrentRouteSetVisible();
}

- (BOOL)isSettingsMode
{
	return _isSettingsMode;
}

- (void)showCurrentSet:(BOOL)aSet
{
	if (_isSettingsMode || [self isCurrentSetPaneVisible] == aSet)
		return;
	Preferences::setCurrentRouteSetVisible(aSet);
	if (aSet || _currentSetController) {
		[self showPane:aSet forController:[self currentSetController]];
		[[self splitView] wgCustomAdjustSubviews:NO];
	}
}

- (void)showDetails:(BOOL)aDetails
{
	if (_isSettingsMode || [self isDetailsPaneVisible] == aDetails)
		return;
	Preferences::setInfoAreaVisible(aDetails);
	if (aDetails || _entryDetailsController) {
		[self showPane:aDetails forController:[self entryDetailsController]];
		[[self splitView] wgCustomAdjustSubviews:NO];
	}
}

// Leaving the settings mode shows the entries list and the panes chosen.
- (void)switchToSettingsMode:(BOOL)aMode
{
	if (!_isSettingsMode)
		return;
	_isSettingsMode = NO;
	[self showPane:YES forController:[self entriesListController]];
	if ([self isCurrentSetPaneVisible] || _currentSetController)
		[self showPane:[self isCurrentSetPaneVisible] forController:[self currentSetController]];
	if ([self isDetailsPaneVisible] || _entryDetailsController)
		[self showPane:[self isDetailsPaneVisible] forController:[self entryDetailsController]];
	[[self splitView] wgCustomAdjustSubviews:NO];
	CGFloat height = [titleButtons frame].size.height;
	[titleButtons setFrameSize:NSMakeSize([[self splitView] bounds].size.width, height)];
	[titleButtons selectCellWithTag:0];
}

- (id)entriesListController
{
	if (!_entriesListController) {
		_entriesListController = [WGMainEntriesListController new];
		[_entriesListController setDelegate:delegate];
	}
	return _entriesListController;
}

- (id)currentSetController
{
	if (!_currentSetController) {
		_currentSetController = [WGCurrentSetController new];
		[_currentSetController setDelegate:delegate];
		[_currentSetController setContainer:self];
	}
	return _currentSetController;
}

- (id)entryDetailsController
{
	if (!_entryDetailsController) {
		_entryDetailsController = [WGCommonEntryDetailsController new];
		[_entryDetailsController setDelegate:delegate];
		[_entryDetailsController setContainer:self];
	}
	return _entryDetailsController;
}

@end
