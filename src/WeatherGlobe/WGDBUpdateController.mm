#import "WGDBUpdateController.h"
#include "AMOnline.h"
#include "AMDatabase.h"

// The keys of an update file's description.
static LConstantString _kUpdateTitleKey("Title");
static LConstantString _kUpdateSizeKey("FileSize");
static LConstantString _kUpdateDescriptionKey("Description");

// The shared controller; -1 once the application terminates.
static WGDBUpdateController *_sWGDBUpdateController = nil;
#define kControllerGone ((WGDBUpdateController *)-1)

// _buttAction: what the download window's button does.
enum {
	kButtonNone = 0,
	kButtonStop = 1,
	kButtonClose = 2,
};

// The database updates: a window listing the update files (each can be
// left out, with the ones after it), then a window following their
// download and the database rescan.
@implementation WGDBUpdateController

+ (void)_appWillTerminate
{
	if (_sWGDBUpdateController && _sWGDBUpdateController != kControllerGone) {
		[_sWGDBUpdateController release];
		_sWGDBUpdateController = kControllerGone;
	}
}

+ (id)sharedInstance
{
	if (!_sWGDBUpdateController) {
		_sWGDBUpdateController = [WGDBUpdateController new];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_appWillTerminate)
			name:NSApplicationWillTerminateNotification object:nil];
	}
	return _sWGDBUpdateController == kControllerGone ? nil : _sWGDBUpdateController;
}

- (void)awakeFromNib
{
	[_downloadProgr setUsesThreadedAnimation:YES];
}

- (void)dealloc
{
	[_currentDownloadedFile release];
	[_downloadWnd release];
	[_mainWindow release];
	if (_descriptions) {
		_descriptions->release();
		_descriptions = 0;
	}
	if (_excludedFiles) {
		_excludedFiles->release();
		_excludedFiles = 0;
	}
	[super dealloc];
}

- (id)nibName
{
	return @"DBUpdateInterface";
}

- (void)updateWithDescriptions:(LMap *)aDescriptions
{
	if (aDescriptions)
		aDescriptions->retain();
	if (_descriptions) {
		_descriptions->release();
		_descriptions = 0;
	}
	_descriptions = aDescriptions;
	if (_excludedFiles) {
		_excludedFiles->release();
		_excludedFiles = 0;
	}
	[self loadNibIfNeeded];
	[_mainWindow center];
	[_mainWindow makeKeyAndOrderFront:nil];
}

- (void)_bgrDBRescanComplete
{
	LNotificationCenter::center()->removeObserver(self, AtlasDatabase::kMainDatabaseDidEndUpdate, 0);
	[_downloadProgr stopAnimation:nil];
	[_downloadProgr setDoubleValue:1.0];
	[_downloadProgr setMaxValue:1.0];
	[_downloadProgr setIndeterminate:NO];
	[_downloadStatus setStringValue:[[NSBundle mainBundle] localizedStringForKey:@"UpdDownloadFinished" value:@"" table:nil]];
	[_downloadButt setTitle:[[NSBundle mainBundle] localizedStringForKey:@"Ok" value:@"" table:nil]];
	[_downloadButt setKeyEquivalent:@"\r"];
	[_downloadButt setEnabled:YES];
	_buttAction = kButtonClose;
}

- (void)updateWillStartWithFilesCount:(NSInteger)aCount
{
	_allUpdateFilesCount = aCount ? (int)aCount : 1;
	_curDwnldFileIdx = 0;
	[self loadNibIfNeeded];
	[_downloadProgr setIndeterminate:NO];
	[_downloadProgr setDoubleValue:0.0];
	[_downloadProgr setMaxValue:100.0];
	[_downloadProgr startAnimation:nil];
	[_downloadStatus setStringValue:@""];
	[_downloadButt setTitle:[[NSBundle mainBundle] localizedStringForKey:@"StopButt" value:@"" table:nil]];
	[_downloadButt setKeyEquivalent:@""];
	[_downloadButt setEnabled:YES];
	_buttAction = kButtonStop;
	[_downloadWnd center];
	[_downloadWnd makeKeyAndOrderFront:nil];
}

// After a download the database is rescanned with the window modal until
// it is done and closed.
- (void)updateWasFinished:(BOOL)aFinished
{
	LNotificationCenter::center()->removeObserver(self, LConnection::connectionContentLengthUpdatedNotification, 0);
	LNotificationCenter::center()->removeObserver(self, LConnection::connectionDataDownloadedNotification, 0);
	[_currentDownloadedFile release];
	_currentDownloadedFile = nil;
	_buttAction = kButtonNone;
	if (_curDwnldFileIdx) {
		[_downloadProgr setIndeterminate:YES];
		[_downloadProgr startAnimation:nil];
		[_downloadStatus setStringValue:[[NSBundle mainBundle] localizedStringForKey:@"ApplyUpdates" value:@"" table:nil]];
		[_downloadButt setEnabled:NO];
		LNotificationCenter::center()->addObserver(self, @selector(_bgrDBRescanComplete),
			AtlasDatabase::kMainDatabaseDidEndUpdate, 0);
		AtlasDatabase::instance()->rescanForUpdates(YES, YES);
		[NSApp runModalForWindow:_downloadWnd];
	}
	[_downloadWnd orderOut:nil];
}

- (BOOL)fileWillBeDownloadedWithName:(id)aName connection:(LConnection *)aConnection
{
	[aName retain];
	[_currentDownloadedFile release];
	_currentDownloadedFile = aName;
	_curDwnldFileSize = 0;
	[_downloadStatus setStringValue:[NSString stringWithFormat:
		[[NSBundle mainBundle] localizedStringForKey:@"IDS_DOWNLOAD_UPD_FILE" value:@"" table:nil],
		aName, _curDwnldFileIdx + 1, _allUpdateFilesCount]];
	LNotificationCenter::center()->removeObserver(self, LConnection::connectionContentLengthUpdatedNotification, 0);
	LNotificationCenter::center()->removeObserver(self, LConnection::connectionDataDownloadedNotification, 0);
	LNotificationCenter::center()->addObserver(self, @selector(_onContentLengthUpdated:),
		LConnection::connectionContentLengthUpdatedNotification, aConnection);
	LNotificationCenter::center()->addObserver(self, @selector(_onContentDataDownloaded:),
		LConnection::connectionDataDownloadedNotification, aConnection);
	return YES;
}

// 2 goes on, 1 retries (the user's choice after a failure), 0 stops.
- (NSInteger)fileWasDownloadedWithName:(id)aName error:(NSInteger)aError
{
	LNotificationCenter::center()->removeObserver(self, LConnection::connectionContentLengthUpdatedNotification, 0);
	LNotificationCenter::center()->removeObserver(self, LConnection::connectionDataDownloadedNotification, 0);
	[_currentDownloadedFile release];
	_currentDownloadedFile = nil;
	if (aError == 200) {
		_curDwnldFileIdx++;
		[_downloadProgr setDoubleValue:(double)((_curDwnldFileIdx * 100) / _allUpdateFilesCount)];
		return 2;
	}
	NSBundle *bundle = [NSBundle mainBundle];
	return NSRunAlertPanel([bundle localizedStringForKey:@"CannotStoreUpdatesTitle" value:@"" table:nil], @"%@",
		[bundle localizedStringForKey:@"RetryButt" value:@"" table:nil],
		[bundle localizedStringForKey:@"Cancel" value:@"" table:nil], nil,
		[bundle localizedStringForKey:@"CannotStoreUpdatesMessage" value:@"" table:nil]) == NSAlertDefaultReturn;
}

- (NSInteger)errorWithCodeCatched:(NSInteger)aCatched data:(LData *)aData
{
	NSBundle *bundle = [NSBundle mainBundle];
	return NSRunAlertPanel([bundle localizedStringForKey:@"Download error" value:@"" table:nil], @"%@",
		[bundle localizedStringForKey:@"RetryButt" value:@"" table:nil],
		[bundle localizedStringForKey:@"Cancel" value:@"" table:nil], nil,
		[bundle localizedStringForKey:@"Download error body" value:@"" table:nil]) == NSAlertDefaultReturn;
}

// Stop asks first; Ok closes the window after the rescan.
- (void)downloadWindowAction:(id)aAction
{
	if (_buttAction == kButtonStop) {
		NSBundle *bundle = [NSBundle mainBundle];
		if (NSRunAlertPanel([bundle localizedStringForKey:@"DownloadCancelTitle" value:@"" table:nil], @"%@",
				[bundle localizedStringForKey:@"ContinueDwnldButt" value:@"" table:nil],
				[bundle localizedStringForKey:@"StopDwnldButt" value:@"" table:nil], nil,
				[bundle localizedStringForKey:@"DownloadCancelBody" value:@"" table:nil]) != NSAlertDefaultReturn)
			DownloadManager::instance()->terminateUpdating();
	} else if (_buttAction == kButtonClose) {
		[NSApp stopModalWithCode:0];
	}
}

- (void)cancelAction:(id)aAction
{
	[_mainWindow orderOut:nil];
}

- (void)tableView:(id)aView willDisplayCell:(id)aCell forTableColumn:(id)aColumn row:(NSInteger)aRow
{
}

static unsigned int _intFromNotification(id notification, LString *key)
{
	LNotification *lnotification = lobj_cast<LNotification>([notification getLObject]);
	LObject *value = lnotification && lnotification->userInfo() ? lnotification->userInfo()->objectForKey(key) : 0;
	return UMapParsingUtils::loadIntFromObject(value);
}

- (void)_onContentLengthUpdated:(id)aUpdated
{
	_curDwnldFileSize = _intFromNotification(aUpdated, LConnection::connectionContentLengthKey);
}

// "file (n of m): x of y KB", the bar over all the files.
- (void)_onContentDataDownloaded:(id)aDownloaded
{
	if (!_curDwnldFileSize)
		return;
	unsigned int downloaded = _intFromNotification(aDownloaded, LConnection::connectionDownloadedContentSizeKey);
	[_downloadStatus setStringValue:[NSString stringWithFormat:
		[[NSBundle mainBundle] localizedStringForKey:@"IDS_DOWNLOAD_UPD_FILE_EX" value:@"" table:nil],
		_currentDownloadedFile, _curDwnldFileIdx + 1, _allUpdateFilesCount, downloaded >> 10, (_curDwnldFileSize >> 10) + 1]];
	[_downloadProgr setDoubleValue:(double)((int)(((float)downloaded / (float)_curDwnldFileSize + (float)_curDwnldFileIdx)
		* 100.0f) / _allUpdateFilesCount)];
}

// The check box, or the file description's size, title or description.
- (id)tableView:(id)aView objectValueForTableColumn:(id)aColumn row:(NSInteger)aRow
{
	LArray *files = DownloadManager::instance()->_updateFiles;
	if (!files)
		return nil;
	LObject *file = files->objectAtIndex((unsigned int)aRow);
	id identifier = [aColumn identifier];
	if ([identifier isEqualTo:@"checkbox"])
		return [NSNumber numberWithBool:!_excludedFiles || !_excludedFiles->containsObject(file)];
	LString *key;
	if ([identifier isEqualTo:@"size"])
		key = &_kUpdateSizeKey;
	else if ([identifier isEqualTo:@"title"])
		key = &_kUpdateTitleKey;
	else if ([identifier isEqualTo:@"description"])
		key = &_kUpdateDescriptionKey;
	else
		return nil;
	LString *text = 0;
	if (_descriptions) {
		LMap *description = lobj_cast<LMap>(_descriptions->objectForKey(static_cast<LComparable *>(file)));
		LObject *value = description ? description->objectForKey(key) : 0;
		if (value)
			text = value->description();
	}
	return [NSString stringReferenceFromLString:text];
}

- (NSInteger)numberOfRowsInTableView:(id)aView
{
	LArray *files = DownloadManager::instance()->_updateFiles;
	return files ? files->count() : 0;
}

- (void)updateAction:(id)aAction
{
	[_mainWindow orderOut:nil];
	AppController *controller = AppController::instance();
	DownloadManager::instance()->updateWithDelegate(controller ? static_cast<DownloadManager::IDBUpdateDelegate *>(controller) : 0,
		_excludedFiles);
}

// Leaving a file out leaves out the ones after it; taking it back takes
// back the ones before it.
- (void)checkboxColumnAction:(id)aAction
{
	LArray *files = DownloadManager::instance()->_updateFiles;
	int count = files ? (int)files->count() : 0;
	int row = (int)[_descriptionsTV clickedRow];
	if (!_excludedFiles)
		_excludedFiles = LComparableSet::_alloc()->init();
	if (!_excludedFiles->containsObject(files->objectAtIndex(row))) {
		for (; row < count; row++)
			_excludedFiles->addObject(files->objectAtIndex(row));
	} else {
		for (int i = 0; i <= row; i++)
			_excludedFiles->removeObject(files->objectAtIndex(i));
	}
	[_descriptionsTV performSelector:@selector(reloadData) withObject:nil afterDelay:0.001];
}

@end

// Sets the main screen's desktop picture. (The original sent the Finder
// a set-desktop-picture Apple event with the file's alias.)
void wgSetDesktopImageAtPath(NSString *path)
{
	NSURL *url = [NSURL fileURLWithPath:path];
	if (!url)
		return;
	[[NSWorkspace sharedWorkspace] setDesktopImageURL:url forScreen:[NSScreen mainScreen] options:@{} error:nil];
}

// A new mail with the file attached, in the mailto: application: Mail is
// scripted (the text as its content), any other gets the file to open.
void wgCreateMailWithAttachment(NSString *path, NSString *text)
{
	if (![[NSFileManager defaultManager] isReadableFileAtPath:path])
		return;
	NSURL *app = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:[NSURL URLWithString:@"mailto:"]];
	NSString *appPath = [app path];
	if (app && [[NSFileManager defaultManager] isReadableFileAtPath:appPath]) {
		if ([[[NSBundle bundleWithPath:appPath] bundleIdentifier] isEqualToString:@"com.apple.mail"]) {
			NSMutableString *name = [[[path lastPathComponent] mutableCopy] autorelease];
			[name replaceOccurrencesOfString:@"\"" withString:@"\\\"" options:0 range:NSMakeRange(0, [name length])];
			NSString *source = [NSString stringWithFormat:
				@"tell application \"Mail\"\n"
				@"activate\n"
				@"set newMessage to make new outgoing message with properties {content:\"%@\"}\n"
				@"tell newMessage\n"
				@"set theAttachment to \"%@\"\n"
				@"if theAttachment is not equal to \"\" then\n"
				@"make new attachment with properties {file name:theAttachment} at before the first paragraph\n"
				@"end if\n"
				@"set visible to true\n"
				@"end tell\n"
				@"end tell\n",
				text, [[path stringByDeletingLastPathComponent] stringByAppendingPathComponent:name]];
			NSAppleScript *script = [[NSAppleScript alloc] initWithSource:source];
			[script executeAndReturnError:nil];
			[script release];
		} else {
			[[NSWorkspace sharedWorkspace] openURLs:[NSArray arrayWithObject:[NSURL fileURLWithPath:path]]
				withApplicationAtURL:app configuration:[NSWorkspaceOpenConfiguration configuration] completionHandler:nil];
		}
		return;
	}
	NSBundle *bundle = [NSBundle mainBundle];
	NSRunAlertPanel([bundle localizedStringForKey:@"Cannot launch Mail" value:@"" table:nil], @"%@",
		[bundle localizedStringForKey:@"Ok" value:@"" table:nil], nil, nil,
		[bundle localizedStringForKey:@"Mail is NA or user have no mail app" value:@"" table:nil]);
}
