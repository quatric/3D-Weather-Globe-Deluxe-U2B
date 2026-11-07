// AdvancedEntriesListDataSource: the main entries list, whose preview panel
// is the advanced entries tab.

#import "WeatherGlobePrefix.h"
#include "WGAppClasses.h"
#import "WGMainEntriesListController.h"

NSString *_kEntriesTableViewID = @"_entriesTableView";
NSString *_kAdvancedEntriesTableViewID = @"_advancedEntriesTableView";

LF_IMPLEMENT_CLASS(AdvancedEntriesListDataSource, LF_CLASSOF(EntriesListDataSource))

void AdvancedEntriesListDataSource::setEntriesListController(WGMainEntriesListController *controller)
{
	_entriesListController = controller;
}

bool AdvancedEntriesListDataSource::isPreviewPanelShown()
{
	if (!_entriesListController)
		return false;
	return ![[[[_entriesListController tabView] selectedTabViewItem] identifier] isEqualToString:_kEntriesTableViewID];
}

void AdvancedEntriesListDataSource::showPreviewPanel()
{
	[_entriesListController switchTabViewID:_kAdvancedEntriesTableViewID];
}

void AdvancedEntriesListDataSource::hidePreviewPanel()
{
	[_entriesListController switchTabViewID:_kEntriesTableViewID];
}

void AdvancedEntriesListDataSource::reloadGenericList()
{
	[_entriesListController reloadGenericList];
}
