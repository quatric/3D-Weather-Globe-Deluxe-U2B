// MovieExporter: the app's movie export. A capturer's recorded session is
// replayed frame by frame into MovieRecorder through _exporterDataSource,
// which posts the progress notifications. Also RenderLib's initialization.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(MovieExporter, 0)

static LConstantString sMovieExportWasStarted("MovieExportWasStarted");
static LConstantString sMovieDuration("MovieDuration");
static LConstantString sMovieExportRequestedFrame("MovieExportRequestedFrame");
static LConstantString sRequestedFrameTime("RequestedFrameTime");
static LConstantString sMovieExportUpdateProgress("MovieExportUpdateProgress");
static LConstantString sProgressValue("ProgressValue");
static LConstantString sMovieExportWasFinished("MovieExportWasFinished");
static LConstantString sMovieExportError("MovieExportError");
LString *MovieExporter::movieExportWasStarted = &sMovieExportWasStarted;
LString *MovieExporter::movieDuration = &sMovieDuration;
LString *MovieExporter::movieExportRequestedFrame = &sMovieExportRequestedFrame;
LString *MovieExporter::requestedFrameTime = &sRequestedFrameTime;
LString *MovieExporter::movieExportUpdateProgress = &sMovieExportUpdateProgress;
LString *MovieExporter::progressValue = &sProgressValue;
LString *MovieExporter::movieExportWasFinished = &sMovieExportWasFinished;
LString *MovieExporter::movieExportError = &sMovieExportError;

void MovieExporter::initAsSingleton(void *)
{
	_recorder = MovieRecorder::_alloc()->init();
}

void MovieExporter::dealloc()
{
	releaseAndClear(_recorder);
	LObject::dealloc();
}

bool MovieExporter::exportCapturerSequence(LString *path, CSceneCapturer *capturer)
{
	LSize size = _recorder->movieSize();
	bool exporting = _exporting;
	_exporting = true;
	bool done;
	{
		_exporterDataSource source(capturer, size, size, this);
		done = _recorder->exportDataWithProvider(&source, path);
	}
	_exporting = exporting;
	return done;
}

// ---------------------------------------------------------------------------

_exporterDataSource::_exporterDataSource(CSceneCapturer *capturer, LSize size, LSize virtualFrameSize,
                                         MovieExporter *exporter)
    : _capturer(capturer), _exporter(exporter), _passes(1), _pass(0), _progress(0), _duration(1.0)
{
	_capturer->enterGrabMode(size, virtualFrameSize);
}

_exporterDataSource::~_exporterDataSource()
{
	_capturer->exitGrabMode();
}

double _exporterDataSource::getMovieDurationInSeconds()
{
	return _capturer ? (double)_capturer->playbackDuration() : 0.0;
}

void _exporterDataSource::exportStartedWithNumOfPasses(unsigned int passes)
{
	_passes = passes;
	_progress = 0;
	_pass = 0;
	_duration = getMovieDurationInSeconds();
	LMap *info = LMap::mapWithObjectsAndKeys(LValue::valueWithDouble(_duration), MovieExporter::movieDuration, (LObject *)0);
	LNotificationCenter::center()->postNotification(MovieExporter::movieExportWasStarted, _capturer, info);
}

void _exporterDataSource::exportFinishedWithError(int error)
{
	LMap *info = LMap::mapWithObjectsAndKeys(LValue::valueWithInt(error), MovieExporter::movieExportError, (LObject *)0);
	LNotificationCenter::center()->postNotification(MovieExporter::movieExportWasFinished, _capturer, info);
}

const void *_exporterDataSource::getSampleImageDataActualTimeDurationBPRForTime(double time, double *actualTime,
                                                                                double *duration, int *rowBytes)
{
	if (!_exporter->_exporting)
		return 0;
	double part = time / _duration;
	part = part < 0.0 ? 0.0 : part > 1.0 ? 1.0 : part;
	// Back at the start: the next pass.
	if (part + 0.1 < (double)_progress)
		_pass++;
	_progress = (float)part;
	LNotificationCenter *center = LNotificationCenter::center();
	center->postNotification(
	    MovieExporter::movieExportRequestedFrame, _capturer,
	    LMap::mapWithObjectsAndKeys(LValue::valueWithDouble(time), MovieExporter::requestedFrameTime, (LObject *)0));
	center->postNotification(MovieExporter::movieExportUpdateProgress, _capturer,
	                         LMap::mapWithObjectsAndKeys(LValue::valueWithDouble((part + (unsigned int)_pass) /
	                                                                             (double)_passes),
	                                                     MovieExporter::progressValue, 0));
	_capturer->grabFrameAt(time, actualTime, duration);
	LData *data = _capturer->grabbedData();
	*rowBytes = (int)_capturer->grabbedDataRowSize();
	if (time < *actualTime) {
		*duration = (*actualTime - time) + *duration;
		*actualTime = time;
	}
	return data->bytes();
}

// ---------------------------------------------------------------------------

void initializeRenderLib()
{
	LAutoreleasePool::addNewPoolToTop();
	LAutoreleasePool::removeTopPool();
}

void finalizeRenderLib()
{
	LAutoreleasePool::addNewPoolToTop();
	GroundElement::finalize();
	LF_CLASSOF(MovieExporter)->killSingleton();
	LAutoreleasePool::removeTopPool();
}
