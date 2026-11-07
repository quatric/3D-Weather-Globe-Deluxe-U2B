#import "WGiMovieExporter.h"
#include "SMKiMovieExport.h"

@implementation WGiMovieExporter

+ (id)iMovieExporterWithParams:(NSInteger)aParams
{
	return [[[WGiMovieExporter alloc] initWithParams:aParams] autorelease];
}

// The export type: 0 an iMovie 4 project for the iPod, 1 an iMovie 4
// project, 2 an iMovie HD project.
- (id)initWithParams:(NSInteger)aParams
{
	self = [super init];
	if (self)
		_exportType = (int)aParams;
	return self;
}

- (BOOL)exportMovieToFileAtPath:(id)aPath fromTmpPath:(id)aPath2
{
	if (![[NSFileManager defaultManager] isReadableFileAtPath:aPath2])
		return NO;
	iMovieExporterRef exporter = iMovieExporterNew();
	if (!exporter)
		return NO;
	iMovieExportDVStreamDescription description = *iMovieExportGetDefaultDVStreamDescription();
	int videoStandard = 100;
	if (_exportType != 0)
		videoStandard = iMovieExportGetDefaultVideoStandardID();
	NSString *folder = [aPath stringByDeletingPathExtension];
	NSURL *destination = [NSURL fileURLWithPath:aPath];
	if (_exportType == 1)
		destination = [NSURL fileURLWithPath:folder];
	iMovieExporterSetDestination(exporter, (CFURLRef)destination, &videoStandard, 0x800, _exportType == 2);
	iMovieMovie movie = 0;
	iMovieLoadFromFile((CFURLRef)[NSURL fileURLWithPath:aPath2], &movie);
	NSString *name = [(_exportType == 0 ? aPath : folder) lastPathComponent];
	iMovieExporterAddTrackFromQTMovie(exporter, movie, (CFStringRef)name, &description);
	iMovieExporterFlush(exporter, 0, 0, 0);
	iMovieExporterDispose(exporter);
	return YES;
}

@end
