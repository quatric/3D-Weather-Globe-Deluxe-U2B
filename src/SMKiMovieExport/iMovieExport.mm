// iMovieExport: SMKiMovieExport's C interface on AVFoundation (see
// SMKiMovieExport.h). Errors are OSStatus values like the original's.

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#include "SMKiMovieExport.h"

enum { kErrParam = -50, kErrFileNotFound = -43, kErrDiskFull = -34, kErrIO = -36 };

@interface _iMovieTrack : NSObject {
@public
	AVAsset *_asset;
	NSString *_name;
}
@end

@implementation _iMovieTrack
- (void)dealloc
{
	[_asset release];
	[_name release];
	[super dealloc];
}
@end

@interface _iMovieExporter : NSObject {
@public
	NSURL *_destination;
	int _videoStandard;
	BOOL _hdFormat;
	NSMutableArray *_tracks;
}
- (NSURL *)outputURL;
@end

@implementation _iMovieExporter

- (id)init
{
	if ((self = [super init]) != nil)
		_tracks = [[NSMutableArray alloc] init];
	return self;
}

- (void)dealloc
{
	[_destination release];
	[_tracks release];
	[super dealloc];
}

- (NSURL *)outputURL
{
	if (_destination == nil)
		return nil;
	if (_hdFormat)
		return [[_destination URLByDeletingPathExtension] URLByAppendingPathExtension:@"mov"];
	NSString *name = [_tracks count] ? ((_iMovieTrack *)[_tracks objectAtIndex:0])->_name : nil;
	if ([name length] == 0)
		name = [_destination lastPathComponent];
	return [[_destination URLByAppendingPathComponent:[name stringByDeletingPathExtension]]
	    URLByAppendingPathExtension:@"mov"];
}

- (double)estimatedByteSize
{
	double seconds = 0;
	for (_iMovieTrack *track in _tracks)
		seconds += CMTimeGetSeconds([track->_asset duration]);
	return iMovieExportGetBytesForDuration(seconds);
}

// One track a movie; the second and later ones get a number.
- (OSStatus)flushCheckingFreeDiskSpace:(BOOL)checkSpace
{
	NSURL *output = [self outputURL];
	if (output == nil || [_tracks count] == 0)
		return kErrParam;
	NSFileManager *files = [NSFileManager defaultManager];
	NSURL *folder = [output URLByDeletingLastPathComponent];
	if (![files createDirectoryAtURL:folder withIntermediateDirectories:YES attributes:nil error:nil])
		return kErrIO;
	if (checkSpace && iMovieExportGetFreeDiskBytesAtURL((CFURLRef)folder) < [self estimatedByteSize] / 4)
		return kErrDiskFull;
	OSStatus status = noErr;
	NSUInteger index = 0;
	for (_iMovieTrack *track in _tracks) {
		NSURL *url = output;
		if (index++ > 0) {
			NSString *name = [NSString stringWithFormat:@"%@ %lu.mov",
			                                            [[output lastPathComponent] stringByDeletingPathExtension],
			                                            (unsigned long)index];
			url = [folder URLByAppendingPathComponent:name];
		}
		[files removeItemAtURL:url error:nil];
		AVAssetExportSession *session = [AVAssetExportSession exportSessionWithAsset:track->_asset
		                                                                  presetName:AVAssetExportPresetHighestQuality];
		if (session == nil)
			return kErrIO;
		[session setOutputURL:url];
		[session setOutputFileType:AVFileTypeQuickTimeMovie];
		[session setShouldOptimizeForNetworkUse:NO];
		dispatch_semaphore_t done = dispatch_semaphore_create(0);
		[session exportAsynchronouslyWithCompletionHandler:^{
			dispatch_semaphore_signal(done);
		}];
		dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
		dispatch_release(done);
		if ([session status] != AVAssetExportSessionStatusCompleted) {
			NSLog(@"iMovie export of %@ failed: %@", url, [session error]);
			status = kErrIO;
		}
	}
	return status;
}

@end

iMovieExporterRef iMovieExporterNew(void)
{
	return (iMovieExporterRef)[[_iMovieExporter alloc] init];
}

OSStatus iMovieExporterDispose(iMovieExporterRef exporter)
{
	if (exporter == 0)
		return kErrParam;
	[(_iMovieExporter *)exporter release];
	return noErr;
}

OSStatus iMovieExporterSetDestination(iMovieExporterRef exporter, CFURLRef url, const int *videoStandardID, int,
                                      unsigned char iMovieHDFileFormat)
{
	if (exporter == 0 || url == 0)
		return kErrParam;
	_iMovieExporter *e = (_iMovieExporter *)exporter;
	[e->_destination release];
	e->_destination = [(NSURL *)url copy];
	e->_videoStandard = videoStandardID ? *videoStandardID : iMovieExportGetDefaultVideoStandardID();
	e->_hdFormat = iMovieHDFileFormat != 0;
	return noErr;
}

OSStatus iMovieExporterAddTrackFromQTMovie(iMovieExporterRef exporter, iMovieMovie movie, CFStringRef name,
                                           const iMovieExportDVStreamDescription *)
{
	if (exporter == 0 || movie == 0)
		return kErrParam;
	_iMovieTrack *track = [[[_iMovieTrack alloc] init] autorelease];
	track->_asset = [(AVAsset *)movie retain];
	track->_name = [(NSString *)name copy];
	[((_iMovieExporter *)exporter)->_tracks addObject:track];
	return noErr;
}

OSStatus iMovieExporterFlush(iMovieExporterRef exporter, unsigned char, unsigned char, unsigned char checkFreeDiskSpace)
{
	if (exporter == 0)
		return kErrParam;
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	OSStatus status = [(_iMovieExporter *)exporter flushCheckingFreeDiskSpace:checkFreeDiskSpace != 0];
	[pool release];
	return status;
}

CFURLRef iMovieExporterCopyOutputURL(iMovieExporterRef exporter)
{
	return exporter ? (CFURLRef)[[(_iMovieExporter *)exporter outputURL] retain] : 0;
}

double iMovieExporterGetEstimatedByteSize(iMovieExporterRef exporter)
{
	return exporter ? [(_iMovieExporter *)exporter estimatedByteSize] : 0.0;
}

OSStatus iMovieLoadFromFile(CFURLRef url, iMovieMovie *movie)
{
	if (movie)
		*movie = 0;
	if (url == 0 || movie == 0)
		return kErrParam;
	if (![[NSFileManager defaultManager] isReadableFileAtPath:[(NSURL *)url path]])
		return kErrFileNotFound;
	*movie = [[AVURLAsset alloc] initWithURL:(NSURL *)url options:nil];
	return noErr;
}

void iMovieRelease(iMovieMovie movie)
{
	[(AVAsset *)movie release];
}

// <tmp>/<application>.<SMKiMovieExport's identifier>, created (replacing a
// file of that name) when missing.
CFStringRef iMovieExportGetTemporaryDirectory(void)
{
	static NSString *sDirectory = nil;
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	if (sDirectory == nil) {
		NSString *identifier = [[NSBundle bundleForClass:[_iMovieExporter class]] bundleIdentifier];
		NSString *name = [NSString stringWithFormat:@"%@.%@", [[[NSBundle mainBundle] bundlePath] lastPathComponent],
		                                            identifier ? identifier : @"SMKiMovieExport"];
		sDirectory = [[NSTemporaryDirectory() stringByAppendingPathComponent:name] retain];
	}
	NSFileManager *files = [NSFileManager defaultManager];
	BOOL isFolder = NO;
	if ([files fileExistsAtPath:sDirectory isDirectory:&isFolder] && !isFolder)
		[files removeItemAtPath:sDirectory error:nil];
	if (!isFolder)
		[files createDirectoryAtPath:sDirectory
		    withIntermediateDirectories:YES
		                     attributes:@{ NSFilePosixPermissions: @0777 }
		                          error:nil];
	[pool release];
	return (CFStringRef)sDirectory;
}

// Of the nearest existing folder up from `url`.
double iMovieExportGetFreeDiskBytesAtURL(CFURLRef url)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSFileManager *files = [NSFileManager defaultManager];
	NSString *path = [(NSURL *)url path];
	while ([path length] > 1 && ![files fileExistsAtPath:path])
		path = [path stringByDeletingLastPathComponent];
	double bytes = [[[files attributesOfFileSystemForPath:path error:nil] objectForKey:NSFileSystemFreeSize] doubleValue];
	[pool release];
	return bytes;
}

double iMovieExportGetBytesForDuration(double seconds)
{
	return seconds * 3685771.0;
}

const iMovieExportDVStreamDescription *iMovieExportGetDefaultDVStreamDescription(void)
{
	static const iMovieExportDVStreamDescription sDescription = { 44100.0, 0 };
	return &sDescription;
}

int iMovieExportGetDefaultVideoStandardID(void)
{
	return 1;
}

double iMovieExportGetProgressPercentMin(void)
{
	return 0.0;
}

double iMovieExportGetProgressPercentMax(void)
{
	return 100.0;
}
