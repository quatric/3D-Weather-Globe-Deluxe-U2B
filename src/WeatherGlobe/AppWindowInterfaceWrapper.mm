// AppWindowInterfaceWrapper: RenderLib's window interface of the main window.

#import "WeatherGlobePrefix.h"
#include "WGAppClasses.h"
#import "MainWindowController.h"
#import "AtlasController.h"
#import "WGNewLocationController.h"
#import "WGCurrentSetController.h"
#import "WGEntryInfoPopupController.h"
#import "NSCursor+NSCursorWGExtensions.h"
#import "AtlasGLView.h"

// The category's zoom cursors predate (and replace) the system's.
#pragma clang diagnostic ignored "-Wunguarded-availability-new"

AppWindowInterfaceWrapper::AppWindowInterfaceWrapper(MainWindowController *owner)
    : BaseWindowInterface([owner glView], 0), _owner(owner)
{
}

bool AppWindowInterfaceWrapper::entryIsMarked(GenericEntry *)
{
	return false;
}

void AppWindowInterfaceWrapper::appendEntryToSet(GenericEntry *entry)
{
	[_owner addEntryToCurrentSet:entry];
}

bool AppWindowInterfaceWrapper::pointEntriesOnMap()
{
	return false;
}

void AppWindowInterfaceWrapper::entryWasClicked(GenericEntry *entry, LPoint, TBillboardKind kind)
{
	[_owner showDetailsForEntry:entry openPane:YES openInfoForBillboard:kind];
}

void AppWindowInterfaceWrapper::locationWasPicked(LPoint geoPoint)
{
	[[[AtlasController atlasController] newLocationController] setGeoCoords:geoPoint];
}

void AppWindowInterfaceWrapper::toogleFullScreen()
{
	[_owner toggleFullWindowMode:nil];
}

void AppWindowInterfaceWrapper::stopAnyFlying()
{
	[[_owner currentSetController] flyStoped];
}

void AppWindowInterfaceWrapper::setScaleValue(float scale)
{
	[_owner setScaleValue:scale];
}

void AppWindowInterfaceWrapper::setMinimapWorldRect(LRect)
{
}

void AppWindowInterfaceWrapper::compassValueChangedTo(TCompassValue)
{
}

void AppWindowInterfaceWrapper::setMouseCursorType(TMouseCursorType type)
{
	NSCursor *cursor;
	switch (type) {
	case 1:
		cursor = [NSCursor openHandCursor];
		break;
	case 2:
		cursor = [NSCursor closedHandCursor];
		break;
	case 3:
		cursor = [NSCursor zoomInCursor];
		break;
	case 4:
		cursor = [NSCursor zoomOutCursor];
		break;
	case 5:
		cursor = [NSCursor pushPinCursor];
		break;
	case 6:
		cursor = [NSCursor coordPickerCursor];
		break;
	default:
		cursor = [NSCursor arrowCursor];
		break;
	}
	[(AtlasGLView *)[_owner glView] setCursor:cursor];
}

void AppWindowInterfaceWrapper::showPopupEntryInfo(GenericEntry *entry)
{
	[WGEntryInfoPopupController showPopupPaletteForEntry:entry];
}

bool AppWindowInterfaceWrapper::isPopupEntryInfoVisible()
{
	return [WGEntryInfoPopupController isVisible];
}

void AppWindowInterfaceWrapper::hidePopupEntryInfo()
{
	[WGEntryInfoPopupController hidePopupPalette];
}
