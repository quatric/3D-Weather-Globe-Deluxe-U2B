// SMKiMovieExport: hands a movie the application recorded to iMovie.
//
// The original turned QuickTime movies into DV streams and wrote iMovie 4/5
// and iMovie HD project files around them (QuickTime and iMovie HD's
// formats, gone from 64-bit macOS and from today's iMovie). This one keeps
// the C interface and writes what iMovie imports now: a QuickTime movie
// (H.264, the source's size) where the project would have gone. The
// application opens it with iMovie.

#ifndef SMKIMOVIEEXPORT_H
#define SMKIMOVIEEXPORT_H

#include <CoreFoundation/CoreFoundation.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iMovieExporterOpaque *iMovieExporterRef;
// An AVAsset (retained) where the original had a QuickTime Movie.
typedef void *iMovieMovie;

#pragma pack(push, 4)
typedef struct _iMovieExportDVStreamDescription {
	double audioSampleRate;	// 44100
	unsigned char lockedAudio;
} iMovieExportDVStreamDescription;
#pragma pack(pop)

iMovieExporterRef iMovieExporterNew(void);
OSStatus iMovieExporterDispose(iMovieExporterRef exporter);
// `url`: the project (HD: the .iMovieProject; else the project's folder).
// The video standard (the original's IDs: 100 for the iPod, 1 the
// default) and the mode are kept, not used: the movie keeps its size.
OSStatus iMovieExporterSetDestination(iMovieExporterRef exporter, CFURLRef url, const int *videoStandardID,
                                      int mode, unsigned char iMovieHDFileFormat);
OSStatus iMovieExporterAddTrackFromQTMovie(iMovieExporterRef exporter, iMovieMovie movie, CFStringRef name,
                                           const iMovieExportDVStreamDescription *description);
// Writes the tracks: <project without extension>.mov for HD, else
// <folder>/<name>.mov (the first track's name).
OSStatus iMovieExporterFlush(iMovieExporterRef exporter, unsigned char atomically, unsigned char uniqueDestination,
                             unsigned char checkFreeDiskSpace);
// The movie the exporter writes for the destination it has.
CFURLRef iMovieExporterCopyOutputURL(iMovieExporterRef exporter);
double iMovieExporterGetEstimatedByteSize(iMovieExporterRef exporter);

// *movie gets a retained movie, released by iMovieExporterDispose's tracks
// or iMovieRelease.
OSStatus iMovieLoadFromFile(CFURLRef url, iMovieMovie *movie);
void iMovieRelease(iMovieMovie movie);

CFStringRef iMovieExportGetTemporaryDirectory(void);
double iMovieExportGetFreeDiskBytesAtURL(CFURLRef url);
// DV's 3.6 MB a second.
double iMovieExportGetBytesForDuration(double seconds);
const iMovieExportDVStreamDescription *iMovieExportGetDefaultDVStreamDescription(void);
int iMovieExportGetDefaultVideoStandardID(void);
double iMovieExportGetProgressPercentMin(void);
double iMovieExportGetProgressPercentMax(void);

#ifdef __cplusplus
}
#endif

#endif
