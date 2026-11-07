// MovieRecorder: writes a provider's frames into a QuickTime movie with
// AVFoundation (the original drove a QuickTime MovieExport component; see
// RLMovie.h). The frames are pulled in order: each request is for the end
// of the previous frame, in 1/600 s units like the original's procedures.

#import <AVFoundation/AVFoundation.h>
#import <Accelerate/Accelerate.h>
#import <Cocoa/Cocoa.h>
#include "RenderLib.h"

LF_IMPLEMENT_CLASS(MovieRecorder, 0)

enum { kTimeScale = 600 };

// QuickTime's error codes, as the provider gets them.
enum { kErrFileBusy = -47, kErrWritePermission = -61 };

MovieRecorder::MovieRecorder()
    : _pixelFormat('RGBA'), _path(0), _size(LMakeSize(320.0f, 240.0f)), _f24(0), _codec(kCodecH264),
      _quality(kQualityHigh), _frameRate(30), _keyFrameRate(24), _testImage(0), _numPasses(1), _provider(0)
{
}

MovieRecorder *MovieRecorder::init()
{
	LObject::init();
	return this;
}

void MovieRecorder::dealloc()
{
	releaseAndClear(_path);
	releaseAndClear(_testImage);
	LObject::dealloc();
}

bool MovieRecorder::setTestImageBitmap(const void *bitmap, LSize size, int rowBytes)
{
	releaseAndClear(_testImage);
	if (bitmap == 0 || size.height <= 0)
		return false;
	_testImage = LRetain(LData::dataWithBytes(bitmap, (unsigned int)(rowBytes * (int)size.height)));
	return true;
}

// ---------------------------------------------------------------------------
// The settings dialog: the movie size, the codec and the quality.

static const struct {
	unsigned long codec;
	NSString *title;
} kCodecs[] = {
	{ MovieRecorder::kCodecH264, @"H.264" },
	{ MovieRecorder::kCodecHEVC, @"HEVC (H.265)" },
	{ MovieRecorder::kCodecProRes422, @"Apple ProRes 422" },
	{ MovieRecorder::kCodecProRes4444, @"Apple ProRes 4444" },
	{ MovieRecorder::kCodecJPEG, @"Photo JPEG" },
};

static const struct {
	unsigned long quality;
	NSString *title;
} kQualities[] = {
	{ MovieRecorder::kQualityLow, @"Low" },
	{ MovieRecorder::kQualityNormal, @"Medium" },
	{ MovieRecorder::kQualityHigh, @"High" },
	{ MovieRecorder::kQualityMax, @"Best" },
};

static NSTextField *_label(NSString *text, NSRect frame)
{
	NSTextField *label = [NSTextField labelWithString:text];
	[label setFrame:frame];
	[label setAlignment:NSTextAlignmentRight];
	return label;
}

bool MovieRecorder::setupCompression()
{
	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setMessageText:@"Movie Settings"];
	[alert setInformativeText:@"Choose the size and the compression of the movie."];
	[alert addButtonWithTitle:@"OK"];
	[alert addButtonWithTitle:@"Cancel"];
	NSView *view = [[[NSView alloc] initWithFrame:NSMakeRect(0, 0, 320, 96)] autorelease];
	[view addSubview:_label(@"Size:", NSMakeRect(0, 70, 90, 20))];
	NSTextField *width = [[[NSTextField alloc] initWithFrame:NSMakeRect(96, 70, 70, 22)] autorelease];
	[width setIntegerValue:(NSInteger)_size.width];
	NSTextField *height = [[[NSTextField alloc] initWithFrame:NSMakeRect(190, 70, 70, 22)] autorelease];
	[height setIntegerValue:(NSInteger)_size.height];
	[view addSubview:width];
	[view addSubview:_label(@"x", NSMakeRect(166, 70, 20, 20))];
	[view addSubview:height];
	[view addSubview:_label(@"Codec:", NSMakeRect(0, 38, 90, 20))];
	NSPopUpButton *codec = [[[NSPopUpButton alloc] initWithFrame:NSMakeRect(94, 34, 220, 26) pullsDown:NO] autorelease];
	for (size_t i = 0; i < sizeof(kCodecs) / sizeof(kCodecs[0]); i++) {
		[codec addItemWithTitle:kCodecs[i].title];
		if (kCodecs[i].codec == _codec)
			[codec selectItemAtIndex:(NSInteger)i];
	}
	[view addSubview:codec];
	[view addSubview:_label(@"Quality:", NSMakeRect(0, 6, 90, 20))];
	NSPopUpButton *quality = [[[NSPopUpButton alloc] initWithFrame:NSMakeRect(94, 2, 220, 26) pullsDown:NO] autorelease];
	for (size_t i = 0; i < sizeof(kQualities) / sizeof(kQualities[0]); i++) {
		[quality addItemWithTitle:kQualities[i].title];
		if (kQualities[i].quality <= _quality)
			[quality selectItemAtIndex:(NSInteger)i];
	}
	[view addSubview:quality];
	[alert setAccessoryView:view];
	if ([alert runModal] != NSAlertFirstButtonReturn)
		return false;
	// Even sizes, as the encoders want them.
	NSInteger w = [width integerValue] & ~1, h = [height integerValue] & ~1;
	if (w >= 16 && h >= 16)
		_size = LMakeSize((float)w, (float)h);
	_codec = kCodecs[[codec indexOfSelectedItem]].codec;
	_quality = kQualities[[quality indexOfSelectedItem]].quality;
	return true;
}

// ---------------------------------------------------------------------------

void MovieRecorder::_setCurrentPath(LString *path)
{
	releaseAndClear(_path);
	if (path) {
		_path = LString::_alloc()->init();
		_path->setValue(path->cfString());
	} else {
		NSString *name = [NSString stringWithFormat:@"WeatherGlobeMovie-%@.mov", [[NSUUID UUID] UUIDString]];
		NSString *tmp = [NSTemporaryDirectory() stringByAppendingPathComponent:name];
		_path = LString::_alloc()->init();
		_path->setValue((CFStringRef)tmp);
	}
}

// The writer's settings for the codec and the quality.
static NSDictionary *_videoSettings(unsigned long codec, unsigned long quality, LSize size, int frameRate,
                                    int keyFrameRate)
{
	int width = (int)size.width, height = (int)size.height;
	float q = quality >= MovieRecorder::kQualityLossless ? 1.0f : (float)quality / MovieRecorder::kQualityLossless;
	NSMutableDictionary *settings = [NSMutableDictionary dictionary];
	[settings setObject:@(width) forKey:AVVideoWidthKey];
	[settings setObject:@(height) forKey:AVVideoHeightKey];
	switch (codec) {
	case MovieRecorder::kCodecProRes422:
		[settings setObject:AVVideoCodecTypeAppleProRes422 forKey:AVVideoCodecKey];
		break;
	case MovieRecorder::kCodecProRes4444:
		[settings setObject:AVVideoCodecTypeAppleProRes4444 forKey:AVVideoCodecKey];
		break;
	case MovieRecorder::kCodecJPEG:
		[settings setObject:AVVideoCodecTypeJPEG forKey:AVVideoCodecKey];
		[settings setObject:@{ AVVideoQualityKey: @(q < 0.1f ? 0.1f : q) } forKey:AVVideoCompressionPropertiesKey];
		break;
	default: {
		bool hevc = codec == MovieRecorder::kCodecHEVC;
		[settings setObject:hevc ? AVVideoCodecTypeHEVC : AVVideoCodecTypeH264 forKey:AVVideoCodecKey];
		// About 0.03 to 0.3 bits a pixel.
		double bitsPerPixel = 0.03 + 0.27 * q * q;
		if (hevc)
			bitsPerPixel *= 0.6;
		double bitRate = bitsPerPixel * width * height * (frameRate > 0 ? frameRate : 30);
		[settings setObject:@{
			AVVideoAverageBitRateKey: @(bitRate),
			AVVideoMaxKeyFrameIntervalKey: @(keyFrameRate > 0 ? keyFrameRate : 24),
			AVVideoExpectedSourceFrameRateKey: @(frameRate > 0 ? frameRate : 30),
		}
		             forKey:AVVideoCompressionPropertiesKey];
		break;
	}
	}
	return settings;
}

bool MovieRecorder::_startCreateMovie()
{
	NSString *path = (NSString *)_path->cfString();
	NSFileManager *files = [NSFileManager defaultManager];
	if ([files fileExistsAtPath:path] && ![files removeItemAtPath:path error:nil]) {
		if (_provider) {
			BOOL writable = [files isWritableFileAtPath:[path stringByDeletingLastPathComponent]];
			_provider->exportFinishedWithError(writable ? kErrFileBusy : kErrWritePermission);
		}
		return false;
	}
	int width = (int)_size.width, height = (int)_size.height;
	NSError *error = nil;
	AVAssetWriter *writer = [AVAssetWriter assetWriterWithURL:[NSURL fileURLWithPath:path]
	                                                 fileType:AVFileTypeQuickTimeMovie
	                                                    error:&error];
	AVAssetWriterInput *input = nil;
	AVAssetWriterInputPixelBufferAdaptor *adaptor = nil;
	if (writer) {
		input = [AVAssetWriterInput
		    assetWriterInputWithMediaType:AVMediaTypeVideo
		                   outputSettings:_videoSettings(_codec, _quality, _size, _frameRate, _keyFrameRate)];
		[input setExpectsMediaDataInRealTime:NO];
		NSDictionary *attributes = @{
			(id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
			(id)kCVPixelBufferWidthKey: @(width),
			(id)kCVPixelBufferHeightKey: @(height),
		};
		adaptor = [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:input
		                                                                         sourcePixelBufferAttributes:attributes];
		if ([writer canAddInput:input])
			[writer addInput:input];
		else
			writer = nil;
	}
	if (writer == nil || ![writer startWriting]) {
		int code = error ? (int)[error code] : (writer ? (int)[[writer error] code] : -1);
		if (_provider)
			_provider->exportFinishedWithError(code ? code : -1);
		return false;
	}
	[writer startSessionAtSourceTime:kCMTimeZero];
	if (_provider)
		_provider->exportStartedWithNumOfPasses(1);
	double duration = _provider ? _provider->getMovieDurationInSeconds() : 0.0;
	int end = (int)(duration * kTimeScale);
	int time = 0;
	bool failed = false;
	const uint8_t kRGBAToBGRA[4] = { 2, 1, 0, 3 };
	while (_provider && time < end && !failed) {
		double frameTime = 0, frameDuration = 0;
		int rowBytes = 0;
		const void *pixels = _provider->getSampleImageDataActualTimeDurationBPRForTime(
		    (double)time / kTimeScale, &frameTime, &frameDuration, &rowBytes);
		if (pixels == 0)
			break;	// cancelled: the movie ends here
		while (![input isReadyForMoreMediaData])
			[NSThread sleepForTimeInterval:0.002];
		CVPixelBufferRef buffer = 0;
		if (CVPixelBufferPoolCreatePixelBuffer(0, [adaptor pixelBufferPool], &buffer) != kCVReturnSuccess) {
			failed = true;
			break;
		}
		CVPixelBufferLockBaseAddress(buffer, 0);
		vImage_Buffer from = { (void *)pixels, (vImagePixelCount)height, (vImagePixelCount)width, (size_t)rowBytes };
		vImage_Buffer to = { CVPixelBufferGetBaseAddress(buffer), (vImagePixelCount)height, (vImagePixelCount)width,
			             CVPixelBufferGetBytesPerRow(buffer) };
		vImagePermuteChannels_ARGB8888(&from, &to, kRGBAToBGRA, kvImageNoFlags);
		CVPixelBufferUnlockBaseAddress(buffer, 0);
		int sampleTime = (int)(frameTime * kTimeScale + 0.5);
		if (![adaptor appendPixelBuffer:buffer withPresentationTime:CMTimeMake(sampleTime, kTimeScale)])
			failed = true;
		CVPixelBufferRelease(buffer);
		int next = (int)((frameTime + frameDuration) * kTimeScale + 0.5);
		time = next > time ? next : time + kTimeScale / (_frameRate > 0 ? _frameRate : 30);
	}
	[input markAsFinished];
	int code = 0;
	if (failed) {
		[writer cancelWriting];
		code = [writer error] ? (int)[[writer error] code] : -1;
	} else {
		[writer endSessionAtSourceTime:CMTimeMake(time, kTimeScale)];
		dispatch_semaphore_t done = dispatch_semaphore_create(0);
		[writer finishWritingWithCompletionHandler:^{
			dispatch_semaphore_signal(done);
		}];
		dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
		dispatch_release(done);
		if ([writer status] != AVAssetWriterStatusCompleted)
			code = [writer error] ? (int)[[writer error] code] : -1;
	}
	if (_provider)
		_provider->exportFinishedWithError(code);
	return code == 0;
}

bool MovieRecorder::exportDataWithProvider(IMovieRecorderDataProvider *provider, LString *path)
{
	_setCurrentPath(path);
	IMovieRecorderDataProvider *previous = _provider;
	_provider = provider;
	bool done = _startCreateMovie();
	_provider = previous;
	return done;
}

bool MovieRecorder::saveToFile(LString *path)
{
	if (_path == 0 || path == 0)
		return false;
	return [[NSFileManager defaultManager] copyItemAtPath:(NSString *)_path->cfString()
	                                               toPath:(NSString *)path->cfString()
	                                                error:nil];
}
