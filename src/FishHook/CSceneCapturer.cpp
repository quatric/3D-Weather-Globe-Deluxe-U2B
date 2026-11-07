// CSceneCapturer and CCaptureSession: recording a scene's frames, playing
// them back and grabbing them as pictures.

#include "FHScene.h"
#include "LCoder.h"
#include "LInvocation.h"

LF_IMPLEMENT_CLASS(CSceneCapturer, 0)
LF_IMPLEMENT_CLASS(CCaptureSession, 0)
LF_SELECTOR(CSceneCapturer, _onPlayerTimer);

static const double kFrameDuration = 1.0 / 30;

// ---------------------------------------------------------------------------
// CCaptureSession

static int __compareDoublesArrayItems(const void *a, const void *b, const void *)
{
	double x = *(const double *)a, y = *(const double *)b;
	return x > y ? 1 : x < y ? -1 : 0;
}

static TArray<double> *_newTimesArray()
{
	TArray<double> *times = new TArray<double>;
	times->setComparator(__compareDoublesArrayItems, __compareDoublesArrayItems);
	return times;
}

CCaptureSession::CCaptureSession() : _capturing(false), _startTime(0), _frames(0), _times(0), _objectsData(0) {}

CCaptureSession *CCaptureSession::init()
{
	return LObject::init() ? this : 0;
}

void CCaptureSession::dealloc()
{
	LRelease(_frames);
	_frames = 0;
	delete _times;
	_times = 0;
	LRelease(_objectsData);
	_objectsData = 0;
	LObject::dealloc();
}

void CCaptureSession::startCapturing()
{
	if (_capturing)
		return;
	_capturing = true;
	_startTime = LTimer::currentTimeValue();
	LRelease(_frames);
	_frames = LArray::_alloc()->init();
	delete _times;
	_times = _newTimesArray();
}

double CCaptureSession::duration() const
{
	if (_times == 0 || (int)_times->count() < 1)
		return 0;
	return _times->lastObject() + kFrameDuration;
}

// Times are kept from the start of the capture.
void CCaptureSession::addFrameForTime(CScenePropertiesSet *frame, double time)
{
	if (_frames && _times) {
		_frames->addObject(frame);
		_times->addObject(time - _startTime);
	}
}

CScenePropertiesSet *CCaptureSession::frameSetForTime(double time, double *frameTime, double *frameDuration)
{
	if (_frames == 0 || _frames->count() == 0)
		return 0;
	int last = (int)_times->count() - 1;
	int index = _times->insertIndexForKey(&time, true);
	if (index < 0)
		index = 0;
	else if (index > last)
		index = last;
	double *t = &(*_times)[index];
	double duration = index < last ? t[1] - t[0] : kFrameDuration;
	if (frameTime)
		*frameTime = *t;
	if (frameDuration)
		*frameDuration = duration;
	return (CScenePropertiesSet *)_frames->objectAtIndex(index);
}

LData *CCaptureSession::dataForObjectWithID(LString *id)
{
	return _objectsData ? (LData *)_objectsData->objectForKey(id) : 0;
}

void CCaptureSession::setDataForObjectWithID(LData *data, LString *id)
{
	if (_objectsData == 0)
		_objectsData = LMap::_alloc()->init();
	_objectsData->setObjectForKey(data, id);
}

void CCaptureSession::encodeWithCoder(LCoder *coder) const
{
	StWriteChunk chunk(coder);
	coder->encodeObject(_frames);
	coder->storeBytes(_times->items(), _times->count() * sizeof(double));
	char hasData = _objectsData != 0;
	coder->storeBytes(&hasData, 1);
	if (hasData)
		coder->encodeObject(_objectsData);
}

LObject *CCaptureSession::initWithDecoder(const LCoder *coder)
{
	StReadChunk chunk(coder);
	LObject *obj = coder->decodeObject();
	_frames = obj && obj->getClass()->isSubclassOf(LF_CLASSOF(LArray)) ? (LArray *)obj : 0;
	_times = _newTimesArray();
	_times->preserveCapacity(_frames ? _frames->count() : 0);
	unsigned int count = _frames ? _frames->count() : 0;
	if (count * sizeof(double) != coder->nextBlockSize())
		LF_RAISE(WrongConditionException());
	StPointerBlock times(count * sizeof(double), false);
	coder->readBytes(times.bytes());
	_times->addObjects((const double *)times.bytes(), count);
	if (coder->nextBlockSize() != 1)
		LF_RAISE(WrongConditionException());
	char hasData = 0;
	coder->readBytes(&hasData);
	if (hasData) {
		obj = coder->decodeObject();
		_objectsData = obj && obj->getClass()->isSubclassOf(LF_CLASSOF(LMap)) ? (LMap *)obj : 0;
	}
	return this;
}

// ---------------------------------------------------------------------------
// CSceneCapturer

CSceneCapturer::CSceneCapturer()
    : _scene(0), _session(0), _timer(0), _paused(false), _loop(false), _position(0), _lastTime(0), _grabber(0),
      _grabbing(0), _grabbedData(0), _shownFrame(0)
{
}

CSceneCapturer *CSceneCapturer::initWithScene(CScene *scene)
{
	if (LObject::init() == 0)
		return 0;
	_scene = scene;
	return this;
}

static void _stopGrabbing(LargeBBEmulator **&grabbing)
{
	if (grabbing) {
		if (*grabbing)
			(*grabbing)->stopGrabbing();
		delete grabbing;
		grabbing = 0;
	}
}

void CSceneCapturer::dealloc()
{
	if (_scene)
		_scene->unregisterSceneCapturer(this);
	_stopGrabbing(_grabbing);
	LRelease(_grabber);
	_grabber = 0;
	LRelease(_grabbedData);
	_grabbedData = 0;
	LRelease(_session);
	_session = 0;
	LObject::dealloc();
}

void CSceneCapturer::startCapturing()
{
	LRelease(_session);
	_session = CCaptureSession::_alloc()->init();
	_session->startCapturing();
	_scene->storeCaptureSessionConstants(_session);
	_scene->capturingStartedWithCapturer(this);
}

void CSceneCapturer::stopCapturing()
{
	if (_session)
		_session->stopCapturing();
	_scene->capturingStoppedWithCapturer(this);
}

void CSceneCapturer::clearCapturedData()
{
	if (_session) {
		_session->stopCapturing();
		_session->release();
		_session = 0;
	}
}

// A session can be set while nothing is captured or played.
void CSceneCapturer::setSession(CCaptureSession *session)
{
	if (!isCapturing() && _timer == 0)
		LAssign(_session, session);
}

float CSceneCapturer::playbackDuration() const
{
	return _session ? (float)(_session->duration() + kFrameDuration) : 0;
}

void CSceneCapturer::playInScene(unsigned char loop)
{
	if (_scene == 0 || _timer || isEmpty())
		return;
	_paused = false;
	_timer = LTimer::timerWithParams(30, this, "_onPlayerTimer", true, 0);
	_lastTime = -1;
	_position = 0;
	_shownFrame = 0;
	_loop = loop;
	_scene->playbackStartedWithCapturer(this);
	_scene->setUseConstantsFromSession(_session);
	_scene->setGrabbedFrameSource(_session->frameSetForTime(0, 0, 0));
	_scene->refreshView();
}

// Advances the position by the time since the last tick, unless paused,
// and shows the frame there; stops after the last one.
intptr_t CSceneCapturer::_onPlayerTimer(char *args)
{
	LTimer *timer = LF_ARG(args, 0, LTimer *);
	double now = timer->fireTime();
	if (_lastTime < 0)
		_lastTime = now;
	if (!_paused)
		_position += now - _lastTime;
	_lastTime = now;
	if (_session) {
		CScenePropertiesSet *frame = _session->frameSetForTime(_position, 0, 0);
		if (frame) {
			float duration = (float)(_session->duration() + kFrameDuration);
			if (_position <= (double)duration) {
				if (_shownFrame == frame)
					return 0;
				_scene->setGrabbedFrameSource(frame);
				_scene->refreshView();
				_shownFrame = frame;
				return 0;
			}
		}
	}
	stopPlayback();
	return 0;
}

void CSceneCapturer::stopPlayback()
{
	if (_timer == 0)
		return;
	if (_scene) {
		_scene->playbackStoppedWithCapturer(this);
		_scene->setGrabbedFrameSource(0);
		_scene->setUseConstantsFromSession(0);
	}
	_timer->invalidate();
	_timer = 0;
}

bool CSceneCapturer::enterGrabMode(LSize size, LSize virtualFrameSize)
{
	_stopGrabbing(_grabbing);
	LRelease(_grabber);
	_grabber = 0;
	LRelease(_grabbedData);
	_grabbedData = 0;
	_grabber = LargeBBEmulator::_alloc()->initWithHolder(_scene->_holder);
	if (_grabber == 0)
		return false;
	_grabber->setBufferParametersSimple(size, virtualFrameSize);
	_grabbing = new LargeBBEmulator *(_grabber);
	_grabber->startGrabbing();
	_grabbedData = LData::_alloc()->init();
	_grabbedData->setLength(_grabber->bitmapDataSize());
	_scene->setUseConstantsFromSession(_session);
	return true;
}

void CSceneCapturer::exitGrabMode()
{
	if (_scene) {
		_scene->setGrabbedFrameSource(0);
		_scene->setUseConstantsFromSession(0);
	}
	_stopGrabbing(_grabbing);
	LRelease(_grabber);
	_grabber = 0;
	LRelease(_grabbedData);
	_grabbedData = 0;
}

LSize CSceneCapturer::grabbingFrameSize() const
{
	return _grabber ? _grabber->size() : LZeroSize;
}

unsigned int CSceneCapturer::grabbedDataRowSize() const
{
	return _grabber ? _grabber->bufferRowSize() : 0;
}

// Draws the frame at the time into the grabbed data.
bool CSceneCapturer::grabFrameAt(double time, double *frameTime, double *frameDuration)
{
	if (_session == 0)
		return false;
	CScenePropertiesSet *frame = _session->frameSetForTime(time, frameTime, frameDuration);
	if (frame == 0)
		return false;
	LAutoreleasePool::addNewPoolToTop();
	_scene->setGrabbedFrameSource(frame);
	int rowSize = _grabber->bufferRowSize();
	void *bytes = _grabbedData->bytes();
	if (*_grabbing)
		(*_grabbing)->grabSnapshot(bytes, rowSize);
	LAutoreleasePool::removeTopPool();
	return true;
}

// Grabs one frame at a size without entering grab mode. Autoreleased.
LData *CSceneCapturer::simpleGrabFrameAtTime(double time, int width, int height, int *rowBytes)
{
	LargeBBEmulator *grabber = LargeBBEmulator::_alloc()->initWithHolder(_scene->_holder);
	LSize size = {(float)width, (float)height};
	grabber->setBufferParameters(size, 1, 1, size);
	grabber->startGrabbing();
	unsigned int length = grabber->bitmapDataSize();
	int rowSize = grabber->bufferRowSize();
	if (rowBytes)
		*rowBytes = rowSize;
	LData *data = LData::dataWithLength(length);
	LAutoreleasePool::addNewPoolToTop();
	_scene->setUseConstantsFromSession(_session);
	_scene->setGrabbedFrameSource(_session ? _session->frameSetForTime(time, 0, 0) : 0);
	grabber->grabSnapshot(data->bytes(), rowSize);
	_scene->setGrabbedFrameSource(0);
	_scene->setUseConstantsFromSession(0);
	LAutoreleasePool::removeTopPool();
	grabber->stopGrabbing();
	grabber->release();
	return data;
}
