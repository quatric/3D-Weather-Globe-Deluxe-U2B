// _MovieStopper and _MovieExporterCallbacksHandler: RenderLib's recording
// and MovieExporter's notifications for the movie controller.

#import "WeatherGlobePrefix.h"
#include "WGAppClasses.h"
#import "WGMovieController.h"

LF_IMPLEMENT_CLASS(_MovieStopper, 0)
LF_SELECTOR(_MovieStopper, _onMovieRecordShouldStop);

_MovieStopper *_MovieStopper::initWithController(WGMovieController *controller)
{
	if (!LObject::init())
		return 0;
	LNotificationCenter::center()->addObserver(this, "_onMovieRecordShouldStop", kRecordingMustBeStoppedNotification, 0);
	_controller = controller;
	return this;
}

void _MovieStopper::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	LObject::dealloc();
}

intptr_t _MovieStopper::_onMovieRecordShouldStop(char *)
{
	AtlasPanelController *panel = [_controller _currentPanelController];
	if (panel && panel->isCapturing())
		[_controller _stopRecording:panel];
	return 0;
}

LF_IMPLEMENT_CLASS(_MovieExporterCallbacksHandler, 0)
LF_SELECTOR(_MovieExporterCallbacksHandler, _onExportProgress);
LF_SELECTOR(_MovieExporterCallbacksHandler, _onExportFinished);

_MovieExporterCallbacksHandler *_MovieExporterCallbacksHandler::initWithController(WGMovieController *controller)
{
	if (!LObject::init())
		return 0;
	_unused18 = 0;
	_unused1c = 0;
	_controller = controller;
	LNotificationCenter::center()->addObserver(this, "_onExportProgress", MovieExporter::movieExportUpdateProgress, 0);
	LNotificationCenter::center()->addObserver(this, "_onExportFinished", MovieExporter::movieExportWasFinished, 0);
	return this;
}

void _MovieExporterCallbacksHandler::dealloc()
{
	LNotificationCenter::center()->removeObserver(this);
	LObject::dealloc();
}

intptr_t _MovieExporterCallbacksHandler::_onExportProgress(char *args)
{
	LNotification *notification = LF_ARG(args, 0, LNotification *);
	LMap *info = notification->userInfo();
	if (info) {
		float progress = loadFloatFromObject(info->objectForKey(MovieExporter::progressValue));
		if (progress > 0.0f)
			[_controller _setExportProgress:progress];
	}
	return 0;
}

// An alert for the export's error (an OSStatus); none for success or
// cancellation.
intptr_t _MovieExporterCallbacksHandler::_onExportFinished(char *args)
{
	LNotification *notification = LF_ARG(args, 0, LNotification *);
	LMap *info = notification->userInfo();
	if (!info)
		return 0;
	int error = loadIntFromObject(info->objectForKey(MovieExporter::movieExportError));
	NSString *message, *title;
	switch (error) {
	case 0:
	case -108:	// memFullErr
	case -2008:
		return 0;
	case -34:	// dskFulErr
	case -43:	// fnfErr
		message = @"Free some disk space and try again.";
		title = @"Not enough disk space to export this file";
		break;
	case -47:	// fBsyErr
	case -5010:
	case -5013:
		message = @"Cannot replace file message2";
		title = @"Cannot replace the file";
		break;
	case -35:	// nsvErr
	case -54:	// permErr
	case -61:	// wrPermErr
	case -1402:
	case -5000:	// afpAccessDenied
		message = @"You may not have sufficient privileges to export this file to the folder you have selected. Try exporting it to another location.";
		title = @"Cannot export the file to the selected location";
		break;
	default:
		message = @"There may be file system errors on your hard disk. Try exporting it to another location.";
		title = @"Error while saving file title";
		break;
	}
	NSBundle *bundle = [NSBundle mainBundle];
	NSString *ok = [bundle localizedStringForKey:@"Ok" value:@"" table:nil];
	message = [bundle localizedStringForKey:message value:@"" table:nil];
	title = [bundle localizedStringForKey:title value:@"" table:nil];
	NSRunAlertPanel(title, @"%@", ok, nil, nil, message);
	return 0;
}
