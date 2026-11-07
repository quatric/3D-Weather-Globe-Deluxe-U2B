// Grabbing the back buffer: BackBufferGrabber reads and scales one frame,
// LargeBBEmulator renders a picture bigger than the view in segments and
// StBBGrabbingAsync does that one segment at a time.

#include "FHContext.h"
#include "FHRender.h"
#include "FHTextures.h"
#include "LRunLoop.h"
#include <Accelerate/Accelerate.h>
#include <stdlib.h>
#include <string.h>

LF_IMPLEMENT_CLASS(BackBufferGrabber, LF_CLASSOF(CContextObject))
LF_IMPLEMENT_CLASS(LargeBBEmulator, 0)
LF_SELECTOR(BackBufferGrabber, _rtWasUpdated);
LF_SELECTOR(BackBufferGrabber, _contextWasAttached);
LF_SELECTOR(BackBufferGrabber, _contextWillBeDetached);

BackBufferGrabber::BackBufferGrabber()
    : CContextObject(10), _autoGrabbing(false), _target(0), _width(0x100), _height(0x100), _format(D3DFMT_UNKNOWN),
      _filter(D3DTEXF_NONE), _dataLocker(0), _textureTarget(0), _texture(0), _readWidth(0), _readHeight(0),
      _readBuffer(0), _scaledBuffer(0), _scaleTempBuffer(0), _pboSized(false), _pbo(0)
{
	_managesSelf = true;
}

BackBufferGrabber *BackBufferGrabber::initWithParams(CRenderTarget *target, LSize size, D3DTEXTUREFILTERTYPE filter)
{
	CContext *context = target->context();
	if (initWithHolder(context->holder()) == 0)
		return 0;
	_width = size.width > 0.0f ? (unsigned int)size.width : 0;
	_height = size.height > 0.0f ? (unsigned int)size.height : 0;
	_target = LRetain(target);
	_filter = filter;
	if (target == context->contextRenderTarget()) {
		LNotificationCenter *center = LNotificationCenter::center();
		center->addObserver(this, "_contextWasAttached", &ContextWasAttachedNotification, 0);
		center->addObserver(this, "_contextWillBeDetached", &ContextWillBeDetachedNotification, 0);
	}
	_scaledBuffer = malloc(_width * _height * 4);
	return this;
}

void BackBufferGrabber::dealloc()
{
	unlockGrabbedData();
	LNotificationCenter::center()->removeObserver(this);
	if (_texture) {
		glDeleteTextures(1, &_texture);
		_texture = 0;
	}
	if (_pbo) {
		glDeleteBuffers(1, &_pbo);
		_pbo = 0;
	}
	free(_readBuffer);
	_readBuffer = 0;
	free(_scaledBuffer);
	_scaledBuffer = 0;
	free(_scaleTempBuffer);
	_scaleTempBuffer = 0;
	LRelease(_target);
	_target = 0;
	CContextObject::dealloc();
}

void BackBufferGrabber::setAutoGrabbing(bool autoGrab)
{
	if (_autoGrabbing == autoGrab)
		return;
	_autoGrabbing = autoGrab;
	if (!autoGrab)
		LNotificationCenter::center()->removeObserver(this);
	else
		LNotificationCenter::center()->addObserver(this, "_rtWasUpdated", &targetWasDetached, 0);
}

void BackBufferGrabber::instantiateSelf()
{
	_managesSelf = true;
	_textureTarget = GL_TEXTURE_RECTANGLE_EXT;
	glGenTextures(1, &_texture);
	_format = D3DFMT_A8R8G8B8;
}

void BackBufferGrabber::releaseSelf()
{
	if (_texture) {
		glDeleteTextures(1, &_texture);
		_texture = 0;
	}
	if (_pbo) {
		glDeleteBuffers(1, &_pbo);
		_pbo = 0;
	}
}

// Reads the target's frame and scales it to the grabbed size, top row first.
bool BackBufferGrabber::grab()
{
	unsigned int width = _target->_width;
	unsigned int height = _target->_height;
	bool resized = false;
	if (width != (unsigned int)(_readWidth > 0.0f ? _readWidth : 0.0f) ||
	    height != (unsigned int)(_readHeight > 0.0f ? _readHeight : 0.0f)) {
		resized = true;
		_readWidth = (float)width;
		_readHeight = (float)height;
		free(_readBuffer);
		_readBuffer = 0;
		free(_scaleTempBuffer);
		_scaleTempBuffer = 0;
	}
	if (_readBuffer == 0 && _pbo == 0)
		_readBuffer = malloc(width * height * 4);
	if (_pbo) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbo);
		if (!_pboSized || resized) {
			glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 4, 0, GL_STREAM_READ);
			_pboSized = true;
		}
	}
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, _readBuffer);
	void *pixels = _readBuffer;
	if (_pbo)
		pixels = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

	vImage_Buffer src = { pixels, height, width, width * 4 };
	vImage_Buffer dst = { _scaledBuffer, _height, _width, _width * 4 };
	if (_scaleTempBuffer == 0)
		_scaleTempBuffer = malloc(vImageScale_ARGB8888(&src, &dst, 0, kvImageGetTempBufferSize));
	vImageScale_ARGB8888(&src, &dst, _scaleTempBuffer, kvImageNoFlags);
	vImageVerticalReflect_ARGB8888(&dst, &dst, kvImageDoNotTile);
	if (_pbo) {
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	}
	return true;
}

intptr_t BackBufferGrabber::_rtWasUpdated(char *)
{
	grab();
	return 0;
}

intptr_t BackBufferGrabber::_contextWasAttached(char *)
{
	_target = LRetain(_holder->context()->contextRenderTarget());
	return 0;
}

intptr_t BackBufferGrabber::_contextWillBeDetached(char *)
{
	LRelease(_target);
	_target = 0;
	return 0;
}

void BackBufferGrabber::lockGrabbedData(D3DLOCKED_RECT *rect)
{
	_dataLocker = new StSharableLocker(&_dataLock);
	rect->Pitch = _width * 4;
	rect->pBits = _scaledBuffer;
}

void BackBufferGrabber::unlockGrabbedData()
{
	delete _dataLocker;
	_dataLocker = 0;
}

// ---------------------------------------------------------------------------

LargeBBEmulator::LargeBBEmulator()
    : _holder(0), _size(LMakeSize(640, 480)), _virtualFrameSize(LMakeSize(640, 480)),
      _segmentSize(LMakeSize(640, 480)), _cols(1), _rows(1), _bitmapDataSize(0), _rowSize(0),
      _format(D3DFMT_UNKNOWN), _delegate(0), _savedTimerUpdate(false), _grabber(0)
{
}

LargeBBEmulator *LargeBBEmulator::initWithHolder(CHolder *holder)
{
	if (LObject::init() == 0)
		return 0;
	_holder = holder;
	return this;
}

void LargeBBEmulator::dealloc()
{
	LRelease(_grabber);
	_grabber = 0;
	LObject::dealloc();
}

void LargeBBEmulator::setBufferParameters(LSize segmentSize, unsigned int cols, unsigned int rows,
                                          LSize virtualFrameSize)
{
	_cols = cols;
	_segmentSize.width = segmentSize.width >= 1.0f ? segmentSize.width : 1.0f;
	_segmentSize.height = segmentSize.height >= 1.0f ? segmentSize.height : 1.0f;
	_rows = rows;
	_size.height = (float)rows * _segmentSize.height;
	_size.width = (float)cols * _segmentSize.width;
	_virtualFrameSize = virtualFrameSize;
	LRelease(_grabber);
	_grabber = 0;
}

// Segments of at most the maximum texture size (and at most 1024).
void LargeBBEmulator::setBufferParametersSimple(LSize size, LSize virtualFrameSize)
{
	int maxSize = (int)_holder->context()->caps()->GetMaxTextureSize();
	if (maxSize > 0x400)
		maxSize = 0x400;
	int cols = ((int)size.width - 1) / maxSize + 1;
	int rows = ((int)size.height - 1) / maxSize + 1;
	setBufferParameters(LMakeSize((float)((int)size.width / cols), (float)((int)size.height / rows)), cols, rows,
	                    virtualFrameSize);
}

void LargeBBEmulator::onBackbufferReady()
{
	if (_grabber)
		_grabber->grab();
}

static unsigned int _floatToUInt(float f)
{
	return f > 0.0f ? (unsigned int)f : 0;
}

void LargeBBEmulator::startGrabbing()
{
	_holder->setBackBufferEmulator(this);
	CRenderTarget *target = _holder->context()->contextRenderTarget();
	if (_grabber == 0) {
		_grabber = LRetain(target->detachBackBufferGrabber(_segmentSize, D3DTEXF_LINEAR));
		_grabber->setAutoGrabbing(false);
		_format = _grabber->grabbedBufferFormat();
	}
	unsigned int pixelSize = CBaseTexture::sizeOfPixelInFormat(_format);
	_rowSize = pixelSize * _floatToUInt(_segmentSize.width) * _cols;
	_bitmapDataSize = _rowSize * _floatToUInt(_segmentSize.height) * _rows;
	if (_delegate)
		_delegate->renderingWasStarted(this);
	target->setOutSegmentsCount(_cols, _rows);
	_savedTimerUpdate = LTimer::updateCurrentTimeValueOnRequest();
	LTimer::setUpdateCurrentTimeValueOnRequest(true);
}

void LargeBBEmulator::stopGrabbing()
{
	CRenderTarget *target = _holder->context()->contextRenderTarget();
	if (_delegate)
		_delegate->renderingWasFinished(this);
	LTimer::setUpdateCurrentTimeValueOnRequest(_savedTimerUpdate);
	target->setOutSegmentsCount(1, 1);
	target->setOutSegment(0, 0);
	_holder->setBackBufferEmulator(0);
}

// Renders segment (col, row) and copies it into `buffer`, which has
// `rowBytes` (at least bufferRowSize) per row.
void LargeBBEmulator::grabSnapshotSubimage(void *buffer, int rowBytes, int col, int row)
{
	CRenderTarget *target = _holder->context()->contextRenderTarget();
	unsigned int pixelSize = CBaseTexture::sizeOfPixelInFormat(_format);
	unsigned int width = _floatToUInt(_segmentSize.width);
	unsigned int height = _floatToUInt(_segmentSize.height);
	target->setOutSegment(col, row);
	bool last = row == (int)_rows - 1 && col == (int)_cols - 1;
	_holder->doAcceleratedRendering(col == 0 && row == 0, last, false);
	D3DLOCKED_RECT rect;
	_grabber->lockGrabbedData(&rect);
	if (_delegate)
		_delegate->processBackBufferSegment(this, &rect, col, row);
	unsigned int stride = (unsigned int)rowBytes > _rowSize ? (unsigned int)rowBytes : _rowSize;
	char *out = (char *)buffer;
	for (unsigned int y = 0; y < height; y++) {
		memcpy(out, (char *)rect.pBits + y * rect.Pitch, width * pixelSize);
		out += stride;
	}
	_grabber->unlockGrabbedData();
}

void LargeBBEmulator::grabSnapshot(void *buffer, int rowBytes)
{
	if (buffer == 0)
		return;
	unsigned int pixelSize = CBaseTexture::sizeOfPixelInFormat(_format);
	unsigned int width = _floatToUInt(_segmentSize.width);
	unsigned int height = _floatToUInt(_segmentSize.height);
	unsigned int stride = (unsigned int)rowBytes > _rowSize ? (unsigned int)rowBytes : _rowSize;
	for (unsigned int row = 0; row < _rows; row++)
		for (unsigned int col = 0; col < _cols; col++)
			grabSnapshotSubimage((char *)buffer + width * pixelSize * col + row * stride * height, rowBytes, col,
			                     row);
}

LData *LargeBBEmulator::createSingleSnapshot()
{
	startGrabbing();
	LData *data = LData::dataWithLength(_bitmapDataSize);
	grabSnapshot(data->bytes(), _rowSize);
	stopGrabbing();
	return data;
}

// ---------------------------------------------------------------------------

StBBGrabbingAsync::StBBGrabbingAsync(LargeBBEmulator *emulator, LSize size, LSize virtualFrameSize, IGrabCB *callback)
    : _emulator(emulator), _callback(callback), _rowsData(0), _rowBytes(0), _segmentRowBytes(0), _segment(0),
      _cols(0), _rows(0), _segmentWidth(0), _segmentHeight(0)
{
	if (_emulator == 0)
		return;
	_emulator->setBufferParametersSimple(size, virtualFrameSize);
	if (_callback)
		_callback->setOutSize(_emulator->size());
	_cols = _emulator->_cols;
	_rows = _emulator->_rows;
	_segmentWidth = (int)_emulator->_segmentSize.width;
	_segmentHeight = (int)_emulator->_segmentSize.height;
	_emulator->startGrabbing();
	_rowBytes = _emulator->bufferRowSize();
	_segmentRowBytes = CBaseTexture::sizeOfPixelInFormat(_emulator->bufferFormat()) * _segmentWidth;
	_rowsData = LData::_alloc()->init();
	_rowsData->setLength(_rowBytes * _segmentHeight);
}

StBBGrabbingAsync::~StBBGrabbingAsync()
{
	if (_emulator)
		_emulator->stopGrabbing();
	LRelease(_rowsData);
	_rowsData = 0;
}

float StBBGrabbingAsync::renderProgress() const
{
	unsigned int total = _cols * _rows;
	return total == 0 ? 1.0f : (float)_segment / (float)total;
}

// Renders the next segment; hands a finished row of segments to the callback.
bool StBBGrabbingAsync::tryRender()
{
	if (_segment >= _cols * _rows)
		return false;
	int row = _segment / _cols;
	int col = _segment % _cols;
	if (_emulator)
		_emulator->grabSnapshotSubimage((char *)_rowsData->bytes() + col * _segmentRowBytes, _rowBytes, col, row);
	if (col == _cols - 1 && _callback)
		_callback->dataIsReady(_rowsData->bytes(), _segmentHeight * row, _segmentHeight, _rowBytes);
	_segment++;
	return true;
}
