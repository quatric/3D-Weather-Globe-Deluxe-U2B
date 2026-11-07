// Movies: MovieExporter (the singleton the app drives) records a scene
// capturer's session into a movie file with MovieRecorder.
//
// The original MovieRecorder was QuickTime's Movie Toolbox (a MovieExport
// component pulling frames through procedures, settings in atom containers,
// the QuickTime export dialog), which doesn't exist for x86_64 or arm64.
// This one keeps its interface and settings (QuickTime codec fourccs and
// codecNormalQuality-style 0..0x400 qualities) and writes with AVFoundation.

#ifndef RLMOVIE_H
#define RLMOVIE_H

class MovieRecorder : public LObject {
	LF_DECLARE_CLASS(MovieRecorder)
public:
	// Where the frames come from.
	class IMovieRecorderDataProvider {
	public:
		virtual ~IMovieRecorderDataProvider() {}
		virtual double getMovieDurationInSeconds() = 0;
		// The RGBA frame (top row first) for `time`, the frame's own time and
		// duration, and its row bytes; 0 ends the movie.
		virtual const void *getSampleImageDataActualTimeDurationBPRForTime(double time, double *actualTime,
		                                                                   double *duration, int *rowBytes) = 0;
		virtual void exportStartedWithNumOfPasses(unsigned int) {}
		virtual void exportFinishedWithError(int) {}
	};

	// The original's QuickTime values.
	enum {
		kCodecH264 = 0x61766331,	// 'avc1'
		kCodecHEVC = 0x68766331,	// 'hvc1'
		kCodecMPEG4 = 0x6d703476,	// 'mp4v' (written as H.264)
		kCodecProRes422 = 0x6170636e,	// 'apcn'
		kCodecProRes4444 = 0x61703468,	// 'ap4h'
		kCodecJPEG = 0x6a706567,	// 'jpeg'
		kQualityMin = 0,
		kQualityLow = 0x100,
		kQualityNormal = 0x200,
		kQualityHigh = 0x300,
		kQualityMax = 0x3ff,
		kQualityLossless = 0x400,
	};

	MovieRecorder();
	virtual MovieRecorder *init();
	virtual void dealloc();
	virtual MovieRecorder *getOwnerLObject() { return this; }

	LSize movieSize() const { return _size; }
	unsigned long movieCodec() const { return _codec; }
	unsigned long movieQuality() const { return _quality; }
	void setMovieSize(LSize size) { _size = size; }
	void setMovieCodec(unsigned long codec) { _codec = codec; }
	void setMovieQuality(unsigned long quality) { _quality = quality; }
	// The original dropped its export component, whose settings come from
	// the fields again.
	void resetToDefaultSettings() { _numPasses = 1; }
	// The settings dialog (size, codec, quality); false when cancelled.
	bool setupCompression();
	// The frame the dialog would preview; kept, not shown.
	bool setTestImageBitmap(const void *bitmap, LSize size, int rowBytes);
	// Writes the provider's frames to `path` (a temporary file without one).
	bool exportDataWithProvider(IMovieRecorderDataProvider *provider, LString *path);
	// Copies the last movie to `path`.
	bool saveToFile(LString *path);

	void _setCurrentPath(LString *path);
	bool _startCreateMovie();

	unsigned int _pixelFormat;	// of the frames: 'RGBA'
	LString *_path;
	LSize _size;
	int _f24;
	unsigned long _codec;
	unsigned long _quality;
	int _frameRate;
	int _keyFrameRate;
	LData *_testImage;
	int _numPasses;
	IMovieRecorderDataProvider *_provider;	// while exporting
};

class _exporterDataSource;

class MovieExporter : public LObject {
	LF_DECLARE_CLASS(MovieExporter)
public:
	// Notifications, object the capturer.
	static LString *movieExportWasStarted;	// {movieDuration: double}
	static LString *movieDuration;
	static LString *movieExportRequestedFrame;	// {requestedFrameTime: double}
	static LString *requestedFrameTime;
	static LString *movieExportUpdateProgress;	// {progressValue: 0..1}
	static LString *progressValue;
	static LString *movieExportWasFinished;	// {movieExportError: int}
	static LString *movieExportError;

	MovieExporter() : _recorder(0), _exporting(false) {}
	virtual void initAsSingleton(void *param);
	virtual void dealloc();
	virtual MovieExporter *getOwnerLObject() { return this; }

	void setTestImageForCompressionOptions(const void *bitmap, LSize size, int rowBytes)
	{
		_recorder->setTestImageBitmap(bitmap, size, rowBytes);
	}
	bool displaySetupDialog() { return _recorder->setupCompression(); }
	LSize movieSize() { return _recorder->movieSize(); }
	unsigned int movieCodec() { return (unsigned int)_recorder->movieCodec(); }
	unsigned int movieQuality() { return (unsigned int)_recorder->movieQuality(); }
	void setMovieSize(LSize size) { _recorder->setMovieSize(size); }
	void setMovieCodec(unsigned int codec) { _recorder->setMovieCodec(codec); }
	void setMovieQuality(unsigned int quality) { _recorder->setMovieQuality(quality); }
	void resetSettingsToDefaults() { _recorder->resetToDefaultSettings(); }
	bool isExporting() const { return _exporting; }
	// The export stops at the next frame.
	void cancelExporting() { _exporting = false; }
	bool exportCapturerSequence(LString *path, CSceneCapturer *capturer);

	MovieRecorder *_recorder;
	bool _exporting;
};

// The capturer's frames for the recorder, in grab mode while it lives.
class _exporterDataSource : public MovieRecorder::IMovieRecorderDataProvider {
public:
	_exporterDataSource(CSceneCapturer *capturer, LSize size, LSize virtualFrameSize, MovieExporter *exporter);
	virtual ~_exporterDataSource();
	virtual double getMovieDurationInSeconds();
	virtual const void *getSampleImageDataActualTimeDurationBPRForTime(double time, double *actualTime,
	                                                                   double *duration, int *rowBytes);
	virtual void exportStartedWithNumOfPasses(unsigned int passes);
	virtual void exportFinishedWithError(int error);

	CSceneCapturer *_capturer;
	MovieExporter *_exporter;
	unsigned int _passes;
	int _pass;
	float _progress;
	double _duration;
};

void initializeRenderLib();
void finalizeRenderLib();

#endif
