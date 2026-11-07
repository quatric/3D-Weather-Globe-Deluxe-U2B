#import "WeatherGlobePrefix.h"


@interface PreferencesController : NSObject
{
    NSWindow *_mainWindow;
    NSToolbar *_toolbar;
    NSArray *_identifiers;
    NSMutableDictionary *_prefControllers;
    PrefGenericController *_currentController;
}
- (id)toolbar:(id)aToolbar itemForItemIdentifier:(id)aIdentifier willBeInsertedIntoToolbar:(BOOL)aToolbar2;
- (id)toolbarSelectableItemIdentifiers:(id)aIdentifiers;
- (id)toolbarAllowedItemIdentifiers:(id)aIdentifiers;
- (id)toolbarDefaultItemIdentifiers:(id)aIdentifiers;
- (void)showWeatherPane;
- (void)showSelectCityDialog;
- (void)showPreferences;
- (void)_showPreferencesWithIdentifier:(id)aIdentifier;
- (void)_toolbarItemAction:(id)aAction;
- (void)_loadNibIfNeeded;
- (void)dealloc;
- (void)awakeFromNib;
- (id)_prefControllerForKey:(id)aKey;
- (id)_identifiers;
@end

@interface PreferencesToolbarItem : NSToolbarItem
{
}
- (void)validate;
@end

@interface PreferencesWindow : NSWindow
{
}
- (void)keyDown:(id)aDown;
@end

