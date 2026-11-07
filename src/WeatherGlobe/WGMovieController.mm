#import "WGMovieController.h"
#import "AtlasController.h"
#import "MainWindowController.h"
#import "WGCustomButton.h"
#import "WGCustomSlider.h"
#import "WGTexturedView.h"
#import "WGTexturedButton.h"
#import "WGExportPanel.h"
#import "WGiMovieExporter.h"
#import "WGHelpBook.h"
#import "NSButton+NSButtonWGExt.h"
#import "NSMutableString+NSStringWGExtensions.h"
#import "NSObjToLObjWrapper.h"
#include "SMKiMovieExport.h"

static NSString *_kMovFileExt = @"mov";
static NSString *_kiMovieHDProjectExtension = @"iMovieProject";
static NSString *_kiPodMovieFileExt = @"m4v";
static NSString *_kTempMovieFileName = @"TempMovieFileName";
static NSString *_kTempMovieExtension = @"mov";

// The export targets (_targetMatrix tags).
enum {
	kTargetIPod = 0,
	kTargetIPhone = 1,
	kTargetIMovie = 2,
	kTargetCustom = 3
};

// The play controls' tags.
enum {
	kControlRecord = 1,
	kControlPlay = 2,
	kControlStop = 3
};

// The size popup's tag of the custom size.
static const NSInteger kCustomSizeTag = 100;
// Clips shorter than this can't be played or exported.
static const float kMinClipDuration = 0.2f;
// Recording stops after 10 minutes.
static const float kMaxClipDuration = 600.0f;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

@implementation WGMovieController

- (void)_setupExportTargetCells
{
	NSArray *cells = [_targetMatrix cells];
	int count = (int)[cells count];
	for (int i = 0; i < count; i++) {
		id cell = [cells objectAtIndex:i];
		if ([cell isKindOfClass:[WGCustomButtonCell class]])
			[cell addImage:nil altImage:[NSImage imageNamed:@"ME_Selection"] align:0 scale:2];
	}
}

- (id)nibName
{
	return @"Movie";
}

- (id)autosaveName
{
	return [self className];
}

- (void)cleanUp:(id)aUp
{
	AppWindowInterfaceWrapper *window = [aUp windowInterface];
	AtlasPanelController *panel = window ? window->currentPanel() : 0;
	if (!panel)
		return;
	if (panel->isSequencePlaying())
		[self _stopPlaying:panel];
	if (panel->isCapturing())
		[self _stopRecording:panel];
}

- (void)dealloc
{
	[self _stop];
	[_moviePlayingTimer invalidate];
	_moviePlayingTimer = nil;
	[_movieRecordingTimer invalidate];
	_movieRecordingTimer = nil;
	[_moviesFolder release];
	[_movieStopper release];
	[_exportMovieAccessory release];
	[_iPodSettingsView release];
	[_iPhoneSettingsView release];
	[_customSettingsView release];
	[_iMovieSettingsView release];
	[super dealloc];
}

- (void)hidePalette
{
	[self _stop];
	[super hidePalette];
}

- (void)controlsAction:(id)aAction
{
	AtlasPanelController *panel = [self _currentPanelController];
	if (!panel)
		return;
	switch ([[aAction keyCell] tag]) {
	case kControlPlay:
		if (panel->isCapturing())
			[self _stopRecording:panel];
		if (panel->isSequencePlaying())
			[self _pause:panel];
		else
			[self _play:panel];
		return;
	case kControlStop:
		if (panel->isCapturing())
			[self _stopRecording:panel];
		[self _stopPlaying:panel];
		return;
	case kControlRecord:
		if (panel->isSequencePlaying())
			[self _stopPlaying:panel];
		if (panel->isCapturing())
			[self _stopRecording:panel];
		else
			[self _record:panel];
		return;
	}
}

- (void)sliderWillStartTracking:(id)aTracking
{
	_isSliderTracking = YES;
	CSceneCapturer *capturer = [self _capturerFromMainView];
	if (capturer)
		capturer->pausePlayback();
}

- (void)movieSliderAction:(id)aAction
{
	CSceneCapturer *capturer = [self _capturerFromMainView];
	if (!capturer)
		return;
	float position = [aAction floatValue];
	capturer->setPlaybackPosition(position);
	[_currentPlayingTime setStringValue:[NSString timeFromSecs:position]];
}

- (void)sliderDidEndTracking:(id)aTracking
{
	CSceneCapturer *capturer = [self _capturerFromMainView];
	if (capturer)
		capturer->resumePlayback();
	_isSliderTracking = NO;
}

// The compression dialog, shown with a frame of the clip.
- (void)showExportOptions:(id)aOptions
{
	CSceneCapturer *capturer = [self _capturerFromMainView];
	if (!capturer)
		return;
	MovieExporter *exporter = MovieExporter::instance();
	[self _setupMovieExporter:exporter];
	LSize size = exporter->movieSize();
	if (size.width > 1024.0f || size.height > 1024.0f)
		size = LMakeSize(1024.0f, 768.0f);
	int rowBytes = 0;
	bool grabbing = sLargeBBGrabbingIsActive;
	sLargeBBGrabbingIsActive = true;
	LData *frame = capturer->simpleGrabFrameAtTime(0.0, (int)size.width, (int)size.height, &rowBytes);
	sLargeBBGrabbingIsActive = grabbing;
	if (!frame)
		return;
	exporter->setTestImageForCompressionOptions(frame->bytes(), size, rowBytes);
	if (!exporter->displaySetupDialog())
		return;
	size = exporter->movieSize();
	[_exportMovieOutSizePopUp selectItemAtIndex:[_exportMovieOutSizePopUp indexOfItemWithTag:kCustomSizeTag]];
	[_exportMovieWidthField setStringValue:[NSString stringWithFormat:@"%d", (int)size.width]];
	[_exportMovieHeightField setStringValue:[NSString stringWithFormat:@"%d", (int)size.height]];
	[self exportMovieSizePopupChanged:_exportMovieOutSizePopUp];
}

- (void)helpButton:(id)aButton
{
	[WGHelpBook gotoPage:@"topics/record_play.html" anchor:nil];
}

- (id)_requiredFileType
{
	switch ([_targetMatrix selectedTag]) {
	case kTargetIPod:
	case kTargetIPhone:
		return _kiPodMovieFileExt;
	case kTargetIMovie:
		return [_iMovieMatrix selectedTag] == 0 ? _kiMovieHDProjectExtension : nil;
	case kTargetCustom:
		return _kMovFileExt;
	}
	return nil;
}

- (void)_setFloatingPanel
{
	[(NSPanel *)_palette setFloatingPanel:YES];
}

// Option-clicking a target shows the compression options button.
- (void)selectTarget:(id)aTarget
{
	[self _validateSettingsView];
	[(NSSavePanel *)[aTarget window] setRequiredFileType:[self _requiredFileType]];
	[_advancedOptionsButton setHidden:!([[NSApp currentEvent] modifierFlags] & NSEventModifierFlagOption)];
}

- (void)iMovieMatrixAction:(id)aAction
{
	[(NSSavePanel *)[aAction window] setRequiredFileType:[self _requiredFileType]];
}

- (void)exportMovie:(id)aMovie
{
	if ([self _currentPanelController] == _usedPanelController) {
		[self performSelector:@selector(_exportMovie:) withObject:aMovie afterDelay:0.01];
		return;
	}
	NSBundle *bundle = [NSBundle mainBundle];
	NSRunAlertPanel([bundle localizedStringForKey:@"Cannot export movie" value:@"" table:nil], @"%@",
		[bundle localizedStringForKey:@"Ok" value:@"" table:nil], nil, nil,
		[bundle localizedStringForKey:@"Current view mode was changed" value:@"" table:nil]);
}

- (void)exportMovieSizePopupChanged:(id)aChanged
{
	LSize size = [self _movieSizeFromMenu];
	BOOL custom = [aChanged selectedTag] == kCustomSizeTag;
	[_exportMovieWidthField setEnabled:custom];
	[_exportMovieWidthField setStringValue:[NSString stringWithFormat:@"%d", (int)size.width]];
	[_exportMovieHeightField setEnabled:custom];
	[_exportMovieHeightField setStringValue:[NSString stringWithFormat:@"%d", (int)size.height]];
}

- (void)_record:(AtlasPanelController *)aRecord
{
	if (!aRecord)
		return;
	[_moviePlayingTimer invalidate];
	_moviePlayingTimer = nil;
	[_movieRecordingTimer invalidate];
	_movieRecordingTimer = nil;
	_movieRecordingTimer = [NSTimer timerWithTimeInterval:0.0 target:self selector:@selector(_onRecordTimer:)
		userInfo:nil repeats:YES];
	NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
	[runLoop addTimer:_movieRecordingTimer forMode:NSDefaultRunLoopMode];
	[runLoop addTimer:_movieRecordingTimer forMode:NSEventTrackingRunLoopMode];
	[_helperTextField setHidden:YES];
	[_fileNameField setHidden:NO];
	[_fileNameField setStringValue:[NSString stringWithFormat:
		[[NSBundle mainBundle] localizedStringForKey:@"Maximum clip duration" value:@"" table:nil], 10]];
	[_timeRecordingField setHidden:NO];
	[_movieSlider setHidden:YES];
	[_movieSlider setEnabled:NO];
	[_movieSlider setFloatValue:0.0f];
	[[_playControls cellWithTag:kControlPlay] setEnabled:NO];
	[[_playControls cellWithTag:kControlStop] setEnabled:NO];
	[_exportButton setEnabled:NO];
	[_movieLength setStringValue:@"00:00"];
	[_movieLength setHidden:YES];
	[_currentPlayingTime setStringValue:@"00:00"];
	[_currentPlayingTime setHidden:YES];
	aRecord->startCapturing();
	_usedPanelController = aRecord;
}

- (void)_stopRecording:(AtlasPanelController *)aRecording
{
	[[_playControls cellWithTag:kControlRecord] setState:NSControlStateValueOff];
	CSceneCapturer *capturer = aRecording ? aRecording->_capturer : 0;
	if (!capturer)
		return;
	aRecording->stopCapturing();
	[_movieRecordingTimer invalidate];
	_movieRecordingTimer = nil;
	float duration = capturer->playbackDuration();
	BOOL tooShort = duration < kMinClipDuration;
	[_fileNameField setHidden:YES];
	NSString *helper = [[NSBundle mainBundle] localizedStringForKey:@"You can export to QT movie" value:@"" table:nil];
	if (![[_helperTextField stringValue] isEqualToString:helper]) {
		// The export hint sits a bit off the recording hint.
		NSPoint origin = [_helperTextField frame].origin;
		origin.y += [[_helperTextField superview] isFlipped] ? -6.0 : 6.0;
		[_helperTextField setFrameOrigin:origin];
		[_helperTextField setStringValue:helper];
	}
	[_helperTextField setHidden:NO];
	[_timeRecordingField setHidden:YES];
	[_movieSlider setHidden:NO];
	[_movieSlider setMaxValue:tooShort ? 1.0 : (double)duration];
	[[_playControls cellWithTag:kControlPlay] setEnabled:!tooShort];
	[_exportButton setEnabled:!tooShort];
	[_movieLength setStringValue:[NSString timeFromSecs:tooShort ? 0.0f : duration]];
	[_movieLength setHidden:NO];
	[_currentPlayingTime setStringValue:@"00:00"];
	[_currentPlayingTime setHidden:NO];
}

- (void)_pause:(AtlasPanelController *)aPause
{
	if (!aPause)
		return;
	[_moviePlayingTimer invalidate];
	_moviePlayingTimer = nil;
	[[_playControls cellWithTag:kControlRecord] setEnabled:YES];
	[_movieSlider setEnabled:NO];
	aPause->stopSequence();
	[[_playControls cellWithTag:kControlPlay] setState:NSControlStateValueOff];
}

- (void)_stopPlaying:(AtlasPanelController *)aPlaying
{
	if (!aPlaying)
		return;
	[[_playControls cellWithTag:kControlRecord] setEnabled:YES];
	[[_playControls cellWithTag:kControlPlay] setState:NSControlStateValueOff];
	[[_playControls cellWithTag:kControlStop] setEnabled:NO];
	[_movieSlider setFloatValue:0.0f];
	[_movieSlider setEnabled:NO];
	[_currentPlayingTime setStringValue:@"00:00"];
	[_moviePlayingTimer invalidate];
	_moviePlayingTimer = nil;
	[_movieRecordingTimer invalidate];
	_movieRecordingTimer = nil;
	aPlaying->stopSequence();
}

- (void)_stop
{
	AtlasPanelController *panel = [self _currentPanelController];
	if (!panel)
		return;
	if (panel->isSequencePlaying())
		[self _stopPlaying:panel];
	if (panel->isCapturing())
		[self _stopRecording:panel];
}

// Runs the export's modal session now and then while it goes.
- (void)_setExportProgress:(CGFloat)aProgress
{
	if ([_exportingProgress isIndeterminate]) {
		[_exportingProgress stopAnimation:self];
		[_exportingProgress setIndeterminate:NO];
		[_exportingProgress startAnimation:self];
	}
	[_exportingProgress setDoubleValue:(double)aProgress * 100.0];
	NSTimeInterval now = [NSDate timeIntervalSinceReferenceDate];
	if (_prevUpdateTime + 0.3 <= now) {
		_prevUpdateTime = now;
		[NSApp runModalSession:_exportSession];
	}
}

- (id)_iMovieExporterFromExportParams
{
	if ([_targetMatrix selectedTag] != kTargetIMovie)
		return nil;
	switch ([_iMovieMatrix selectedTag]) {
	case 0:
		return [WGiMovieExporter iMovieExporterWithParams:2];
	case 1:
		return [WGiMovieExporter iMovieExporterWithParams:1];
	}
	return nil;
}

// iMovie projects are made from a temporary movie.
- (id)_exportPathForFilename:(id)aFilename
{
	if ([_targetMatrix selectedTag] != kTargetIMovie)
		return aFilename;
	NSString *folder = (NSString *)iMovieExportGetTemporaryDirectory();
	return [NSString wgCreateUniqueFilename:[folder stringByAppendingPathComponent:_kTempMovieFileName]
		extension:_kTempMovieExtension];
}

- (void)controlTextDidChange:(id)aChange
{
	[self _validateCustomSize];
}

- (void)_setMoviesFolder:(id)aFolder
{
	if (_moviesFolder != aFolder) {
		[_moviesFolder release];
		_moviesFolder = [aFolder retain];
	}
}

- (void)_onTimer:(id)aTimer
{
	if (_isSliderTracking)
		return;
	CSceneCapturer *capturer = (CSceneCapturer *)[[aTimer userInfo] getLObject];
	if (!capturer)
		return;
	float position = capturer->playbackPosition();
	[_currentPlayingTime setStringValue:[NSString timeFromSecs:position]];
	[_movieSlider setFloatValue:position];
	if (capturer->playbackDuration() <= position)
		[self _stopPlaying:[self _currentPanelController]];
}

- (void)_onRecordTimer:(id)aTimer
{
	CSceneCapturer *capturer = [self _capturerFromMainView];
	if (!capturer)
		return;
	float duration = capturer->isCapturing() ? capturer->playbackDuration() : 0.0f;
	[_timeRecordingField setStringValue:[NSString stringWithFormat:
		[[NSBundle mainBundle] localizedStringForKey:@"Time Recorded:" value:@"" table:nil],
		[NSString timeFromSecs:duration]]];
	if (duration > kMaxClipDuration)
		[self _stop];
}

// The settings of the chosen target, centered at the top of the container.
- (void)_validateSettingsView
{
	NSView *view;
	switch ([_targetMatrix selectedTag]) {
	case kTargetIPod:
		view = _iPodSettingsView;
		break;
	case kTargetIPhone:
		view = _iPhoneSettingsView;
		break;
	case kTargetIMovie:
		view = _iMovieSettingsView;
		break;
	case kTargetCustom:
		view = _customSettingsView;
		break;
	default:
		view = nil;
		break;
	}
	if (view == _currentSettingsView)
		return;
	[_currentSettingsView removeFromSuperview];
	NSRect frame = [view frame];
	NSRect bounds = [_containerView bounds];
	[view setFrameOrigin:NSMakePoint(ceilf((bounds.size.width - frame.size.width) * 0.5) + bounds.origin.x,
		bounds.origin.y + bounds.size.height - frame.size.height)];
	[_containerView addSubview:view];
	_currentSettingsView = view;
}

// The custom size must be digits, from 96×64 to 1920×1080.
- (void)_validateCustomSize
{
	NSSavePanel *panel = (NSSavePanel *)[_exportMovieAccessory window];
	NSCharacterSet *nonDigits = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
	NSRange widthBad = [[_exportMovieWidthField stringValue] rangeOfCharacterFromSet:nonDigits];
	NSRange heightBad = [[_exportMovieHeightField stringValue] rangeOfCharacterFromSet:nonDigits];
	BOOL bigEnough = (float)[_exportMovieWidthField intValue] >= 96.0f
		&& (float)[_exportMovieHeightField intValue] >= 64.0f;
	BOOL smallEnough = (float)[_exportMovieWidthField intValue] <= 1920.0f
		&& (float)[_exportMovieHeightField intValue] <= 1080.0f;
	BOOL digits = widthBad.location == NSNotFound && widthBad.length == 0
		&& heightBad.location == NSNotFound && heightBad.length == 0;
	[[(WGExportPanel *)panel okButton] setEnabled:digits && bigEnough && smallEnough];
}

- (LSize)_movieSizeFromMenu
{
	static const LSize menuSizes[] = {
		{160.0f, 120.0f}, {176.0f, 144.0f}, {320.0f, 240.0f}, {352.0f, 288.0f}, {640.0f, 480.0f},
		{720.0f, 486.0f}, {768.0f, 576.0f}, {960.0f, 540.0f}, {1280.0f, 720.0f}, {1920.0f, 1080.0f}
	};
	if ([_exportMovieOutSizePopUp selectedTag] == kCustomSizeTag)
		return LMakeSize((float)[_exportMovieWidthField intValue], (float)[_exportMovieHeightField intValue]);
	NSUInteger index = [_exportMovieOutSizePopUp indexOfSelectedItem];
	if (index < sizeof menuSizes / sizeof menuSizes[0])
		return menuSizes[index];
	return LZeroSize;
}

// The devices' sizes and codecs; the custom target keeps the exporter's
// codec and quality.
- (void)_setupMovieExporter:(MovieExporter *)aExporter
{
	LSize size = LZeroSize;
	unsigned int codec = aExporter->movieCodec();
	unsigned int quality = aExporter->movieQuality();
	BOOL reset = YES;
	switch ([_targetMatrix selectedTag]) {
	case kTargetIPhone:
		size.height = 424.0f;
		switch ([_iPhoneMatrix selectedTag]) {
		case 0:
			size.width = 640.0f;
			break;
		case 1:
			size = LMakeSize(480.0f, 320.0f);
			break;
		default:
			size.height = LZeroSize.height;
			break;
		}
		codec = 'mp4v';
		quality = 0x300;
		break;
	case kTargetIPod:
		size.height = 480.0f;
		switch ([_iPodMatrix selectedTag]) {
		case 0:
			size.width = 640.0f;
			break;
		case 1:
			size = LMakeSize(320.0f, 240.0f);
			break;
		default:
			size.height = LZeroSize.height;
			break;
		}
		codec = 'mp4v';
		quality = 0x300;
		break;
	case kTargetIMovie:
		size = LMakeSize(640.0f, 480.0f);
		codec = 'avc1';
		quality = 0x300;
		break;
	case kTargetCustom:
		size = [self _movieSizeFromMenu];
		reset = NO;
		break;
	}
	if (reset)
		aExporter->resetSettingsToDefaults();
	aExporter->setMovieCodec(codec);
	aExporter->setMovieQuality(quality);
	if (size.width != 0.0f || size.height != 0.0f)
		aExporter->setMovieSize(size);
}

- (CSceneCapturer *)_capturerFromMainView
{
	AtlasPanelController *panel = [self _currentPanelController];
	return panel ? panel->_capturer : 0;
}

- (void)_play:(AtlasPanelController *)aPlay
{
	CSceneCapturer *capturer = aPlay ? aPlay->_capturer : 0;
	if (!capturer)
		return;
	[_movieRecordingTimer invalidate];
	_movieRecordingTimer = nil;
	if (capturer->playbackDuration() <= 0.0f) {
		[[_playControls cellWithTag:kControlPlay] setState:NSControlStateValueOff];
		return;
	}
	_moviePlayingTimer = [NSTimer timerWithTimeInterval:0.0 target:self selector:@selector(_onTimer:)
		userInfo:(id)capturer->detachNSWrapper() repeats:YES];
	NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
	[runLoop addTimer:_moviePlayingTimer forMode:NSDefaultRunLoopMode];
	[runLoop addTimer:_moviePlayingTimer forMode:NSEventTrackingRunLoopMode];
	[_movieLength setStringValue:[NSString timeFromSecs:capturer->playbackDuration()]];
	[_movieSlider setEnabled:YES];
	[_movieSlider setMaxValue:(double)capturer->playbackDuration()];
	aPlay->playSequence();
	capturer->setPlaybackPosition([_movieSlider floatValue]);
	[[_playControls cellWithTag:kControlRecord] setEnabled:NO];
	[[_playControls cellWithTag:kControlStop] setEnabled:YES];
}

- (AtlasPanelController *)_currentPanelController
{
	AppWindowInterfaceWrapper *window = [[[AtlasController atlasController] mainWindowController] windowInterface];
	return window ? window->currentPanel() : 0;
}

- (void)cancelExport:(id)aExport
{
	if (!_exportSession)
		return;
	MovieExporter *exporter = MovieExporter::instance();
	if (exporter->isExporting()) {
		exporter->cancelExporting();
		_isExportCanceled = YES;
	}
}

- (BOOL)wgShouldDoExportInWGExportPanel:(id)aPanel
{
	if ([_exportMovieOutSizePopUp selectedTag] != kCustomSizeTag)
		return YES;
	LSize size = MovieExporter::instance()->movieSize();
	BOOL bigEnough = size.width >= 96.0f && size.height >= 64.0f;
	BOOL smallEnough = size.width <= 1920.0f && size.height <= 1080.0f;
	NSBundle *bundle = [NSBundle mainBundle];
	if (!bigEnough)
		NSRunAlertPanel([bundle localizedStringForKey:@"Cannot export movie" value:@"" table:nil], @"%@",
			[bundle localizedStringForKey:@"Ok" value:@"" table:nil], nil, nil,
			[bundle localizedStringForKey:@"Resolution is too small" value:@"" table:nil]);
	if (!smallEnough)
		NSRunAlertPanel([bundle localizedStringForKey:@"Cannot export movie" value:@"" table:nil], @"%@",
			[bundle localizedStringForKey:@"Ok" value:@"" table:nil], nil, nil,
			[bundle localizedStringForKey:@"Resolution is too big" value:@"" table:nil]);
	return bigEnough && smallEnough;
}

// Exports the recorded clip in a modal session over the palette.
- (void)_exportMovie:(id)aMovie
{
	[self _stop];
	CSceneCapturer *capturer = [self _capturerFromMainView];
	if (!capturer)
		return;
	[self _validateSettingsView];
	WGExportPanel *panel = (WGExportPanel *)[WGExportPanel savePanel];
	[panel setExtensionHidden:YES];
	[panel setCanSelectHiddenExtension:YES];
	[[panel okButton] setTitle:[[NSBundle mainBundle] localizedStringForKey:@"Export" value:@"" table:nil]];
	[panel setRequiredFileType:[self _requiredFileType]];
	[panel setDelegate:(id)self];
	[panel setAccessoryView:_exportMovieAccessory];
	NSString *untitled = [[NSBundle mainBundle] localizedStringForKey:@"Untitled" value:@"" table:nil];
	if ([panel runModalForDirectory:_moviesFolder file:untitled] != NSModalResponseOK) {
		_isExportCanceled = NO;
		return;
	}
	[panel makeFirstResponder:nil];
	NSString *exportPath = [self _exportPathForFilename:[panel filename]];
	NSString *path = [panel filename];
	[self _setMoviesFolder:[path stringByDeletingLastPathComponent]];
	LString *exportLPath = LString::stringWithCFString((CFStringRef)exportPath);
	BOOL existed = [[NSFileManager defaultManager] fileExistsAtPath:path];
	MovieExporter *exporter = MovieExporter::instance();
	bool grabbing = sLargeBBGrabbingIsActive;
	sLargeBBGrabbingIsActive = true;
	[self _setupMovieExporter:exporter];

	[_fileNameField setStringValue:[[NSBundle mainBundle] localizedStringForKey:@"Exporting..." value:@"" table:nil]];
	[_helperTextField setHidden:YES];
	NSString *caption = [_windowCaption stringValue];
	[_windowCaption setStringValue:[[NSBundle mainBundle] localizedStringForKey:@"Export to QuickTime window caption"
		value:@"" table:nil]];
	[_fileNameField setHidden:NO];
	[_exportingProgress setHidden:NO];
	[_exportingProgress setIndeterminate:YES];
	[_exportingProgress setUsesThreadedAnimation:YES];
	[_exportingProgress startAnimation:self];
	[_movieSlider setHidden:YES];
	[_movieLength setHidden:YES];
	[_currentPlayingTime setHidden:YES];
	[_exportButton setHidden:YES];
	[_playControls setHidden:YES];
	[_cancelButton setHidden:NO];
	[[_exportingProgress window] display];

	LObject *callbacks = ((_MovieExporterCallbacksHandler *)_MovieExporterCallbacksHandler::_alloc())->initWithController(self);
	_exportSession = [NSApp beginModalSessionForWindow:[_exportingProgress window]];
	bool exported = exporter->exportCapturerSequence(exportLPath, capturer);
	WGiMovieExporter *iMovieExporter = [self _iMovieExporterFromExportParams];
	if (iMovieExporter) {
		[[NSFileManager defaultManager] removeItemAtPath:path error:nil];
		[iMovieExporter exportMovieToFileAtPath:path fromTmpPath:exportPath];
		[[NSFileManager defaultManager] removeItemAtPath:exportPath error:nil];
	}
	[NSApp endModalSession:_exportSession];
	_exportSession = 0;
	if (callbacks)
		callbacks->release();

	[self performSelector:@selector(_setFloatingPanel) withObject:nil afterDelay:0.0];
	[self showPalette];
	[_windowCaption setStringValue:caption];
	[_exportButton setHidden:NO];
	[_playControls setHidden:NO];
	[_cancelButton setHidden:YES];
	[_exportingProgress stopAnimation:self];
	[_exportingProgress setUsesThreadedAnimation:NO];
	[_exportingProgress setHidden:YES];
	[_movieSlider setHidden:NO];
	[_movieLength setHidden:NO];
	[_currentPlayingTime setHidden:NO];
	[_fileNameField setStringValue:[path lastPathComponent]];
	[_fileNameField setHidden:YES];

	if (!exported || _isExportCanceled) {
		// Leaves a file that was there before.
		if (!existed)
			[[NSFileManager defaultManager] removeItemAtPath:path error:nil];
	}
	else {
		if ([panel isExtensionHidden])
			[[NSFileManager defaultManager] setAttributes:[NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
				forKey:NSFileExtensionHidden] ofItemAtPath:path error:nil];
		if ([_openExportedMovieInQuickTime state] == NSControlStateValueOn) {
			// The iMovie project opens by its project file inside.
			if ([_targetMatrix selectedTag] == kTargetIMovie && [_iMovieMatrix selectedTag] == 1) {
				NSString *folder = [path stringByDeletingPathExtension];
				path = [folder stringByAppendingPathComponent:[folder lastPathComponent]];
			}
			[[NSWorkspace sharedWorkspace] openFile:path];
		}
	}
	sLargeBBGrabbingIsActive = grabbing;
	_isExportCanceled = NO;
}

// ~/Movies/<localized folder>, made when missing.
- (id)defaultMoviesFolder
{
	LString *path = LString::stringWithCFString((CFStringRef)NSHomeDirectoryForUser(NSUserName()));
	path->appendPathComponent(LString::stringWithCString("Movies"));
	path->appendPathComponent(LString::stringFromTable("MoviesFolderName", "PathComponents", getAppModelModule()));
	LFileManager *manager = LFileManager::manager();
	if (!manager->objectExistsAtPath(path, 0)) {
		if (!manager->createFolderAtPath(path))
			return nil;
		manager->changeFileAttributesAtPath(path, LMap::mapWithObjectsAndKeys(LValue::valueWithIntPtr(0777),
			LFileManager::fileAttrPosixPermissions, 0));
	}
	return (NSString *)path->cfString();
}

- (void)awakeFromNib
{
	[super awakeFromNib];
	_moviesFolder = [[self defaultMoviesFolder] retain];
	[_movieSlider setKnobImage:[NSImage imageNamed:@"MWB_slider_knob"]];
	[_movieSlider setKnobAlternateImage:[NSImage imageNamed:@"MWB_slider_knob_on"]];
	[_movieSlider setKnobDisabledImage:[NSImage imageNamed:@"MWB_slider_knob_Disable"]];
	[_movieSlider setBarFirstEdgeImage:[NSImage imageNamed:@"MWT_sliderback_left"]];
	[_movieSlider setBarMiddleImage:[NSImage imageNamed:@"MWT_sliderback_center"]];
	[_movieSlider setBarLastEdgeImage:[NSImage imageNamed:@"MWT_sliderback_right"]];
	[_movieSlider setBarDisabledFirstEdgeImage:[NSImage imageNamed:@"MWT_sliderback_left_disabl"]];
	[_movieSlider setBarDisabledMiddleImage:[NSImage imageNamed:@"MWT_sliderback_center_disabl"]];
	[_movieSlider setBarDisabledLastEdgeImage:[NSImage imageNamed:@"MWT_sliderback_right_disabl"]];
	[_boxTexturedView setImage:[NSImage imageNamed:@"MBG_TextBlock_center"] forTexture:4];
	[_boxTexturedView setImage:[NSImage imageNamed:@"MBG_TextBlock_Left"] forTexture:3];
	[_boxTexturedView setImage:[NSImage imageNamed:@"MBG_TextBlock_Right"] forTexture:5];
	[_exportButton setImagePosition:NSImageOverlaps];
	[_exportButton setImage:[NSImage imageNamed:@"BlackButton_Left"] forPosition:0];
	[_exportButton setImage:[NSImage imageNamed:@"BlackButton_Center"] forPosition:1];
	[_exportButton setImage:[NSImage imageNamed:@"BlackButton_Right"] forPosition:2];
	[_exportButton setTitleColor:[NSColor whiteColor]];
	[_cancelButton setImagePosition:NSImageOverlaps];
	[_cancelButton setImage:[NSImage imageNamed:@"BlackButton_Left"] forPosition:0];
	[_cancelButton setImage:[NSImage imageNamed:@"BlackButton_Center"] forPosition:1];
	[_cancelButton setImage:[NSImage imageNamed:@"BlackButton_Right"] forPosition:2];
	[_cancelButton setTitleColor:[NSColor whiteColor]];
	[self _setupExportTargetCells];
	[self exportMovieSizePopupChanged:_exportMovieOutSizePopUp];
	if (!_movieStopper) {
		_MovieStopper *stopper = ((_MovieStopper *)_MovieStopper::_alloc())->initWithController(self);
		_movieStopper = [[NSObjToLObjWrapper alloc] initWithLObject:stopper->l_autorelease()];
	}
}

@end

#pragma clang diagnostic pop
