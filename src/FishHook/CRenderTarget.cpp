// Render targets, cameras and the scene state they share.

#include "FHScene.h"

LF_IMPLEMENT_CLASS(CRenderTarget, LF_CLASSOF(CContextObject))
LF_IMPLEMENT_CLASS(CContextRenderTarget, LF_CLASSOF(CRenderTarget))
LF_IMPLEMENT_CLASS(CContextCamera, 0)
LF_IMPLEMENT_CLASS(COrthoCamera, LF_CLASSOF(CContextCamera))

LConstantString targetWasAttached("TargetWasAttached");
LConstantString targetWasDetached("TargetWasDetached");

CRenderTarget::CRenderTarget()
    : CContextObject(9), _width(0x100), _height(0x100), _fogColor(0, 0, 0, 0), _clearColor(0, 0, 0, 0),
      _minColorBits(16), _maxColorBits(32), _minDepthBits(15), _maxDepthBits(32), _minStencilBits(0),
      _maxStencilBits(0), _colorFormat(D3DFMT_A8R8G8B8), _depthFormat(D3DFMT_UNKNOWN), _useZBuffer(false), _camera(0),
      _segCols(1), _segRows(1), _segCol(0), _segRow(0), _viewportValid(false), _frustumValid(false),
      _viewValid(false), _projectionValid(false), _viewProjValid(false)
{
	_managesSelf = true;
}

CRenderTarget *CRenderTarget::initWithRectParameters(long width, long height, bool useZBuffer, CHolder *holder)
{
	initWithHolder(holder);
	_width = width;
	_height = height;
	_useZBuffer = useZBuffer;
	return this;
}

void CRenderTarget::dealloc()
{
	if (_camera) {
		_camera->detachFromRenderTarget();
		LRelease(_camera);
		_camera = 0;
	}
	CContextObject::dealloc();
}

void CRenderTarget::attachToContext()
{
	LNotificationCenter::center()->postNotification(&targetWasAttached, this, 0);
}

void CRenderTarget::detachFromContext()
{
	LNotificationCenter::center()->postNotification(&targetWasDetached, this, 0);
}

void CRenderTarget::getOutSegmentsCount(int *cols, int *rows) const
{
	if (cols)
		*cols = _segCols;
	if (rows)
		*rows = _segRows;
}

void CRenderTarget::getOutSegment(int *col, int *row) const
{
	if (col)
		*col = _segCol;
	if (row)
		*row = _segRow;
}

void CRenderTarget::setFogColor(Color color)
{
	_fogColor = color;
	if (this == _context->currentRenderTarget())
		_context->drawCache()->setRenderState(D3DRS_FOGCOLOR, (unsigned int)_fogColor);
}

const Matrix *CRenderTarget::viewMatrix() const
{
	if (!_viewValid) {
		_viewValid = true;
		recalcViewMatrix(&_view);
	}
	return &_view;
}

const Matrix *CRenderTarget::projectionMatrix() const
{
	if (!_projectionValid) {
		_projectionValid = true;
		recalcProjectionMatrix(&_projection);
	}
	return &_projection;
}

const Matrix *CRenderTarget::viewProjMatrix() const
{
	if (!_viewProjValid) {
		_viewProjValid = true;
		const Matrix *projection = projectionMatrix();
		FHMatrixMultiply(&_viewProj, viewMatrix(), projection);
	}
	return &_viewProj;
}

const SViewFrustum *CRenderTarget::viewFrustum() const
{
	if (!_frustumValid) {
		_frustumValid = true;
		const Matrix *projection = projectionMatrix();
		UMathUtils::calcViewFrustum(&_frustum, viewMatrix(), projection);
	}
	return &_frustum;
}

const SViewPort *CRenderTarget::viewport() const
{
	if (!_viewportValid) {
		_viewportValid = true;
		_viewport.x = 0;
		_viewport.y = 0;
		_viewport.width = _width;
		_viewport.height = _height;
		_viewport.minZ = 0.0f;
		_viewport.maxZ = 1.0f;
	}
	return &_viewport;
}

void CRenderTarget::recalcViewMatrix(Matrix *m) const
{
	*m = _camera ? *_camera->viewMatrix() : Matrix::identity();
}

void CRenderTarget::recalcProjectionMatrix(Matrix *m) const
{
	Matrix projection;
	if (_camera)
		_camera->projectionMatrixForAspect(viewportAspect(), &projection);
	else
		projection.setIdentity();
	adjustProjectionMatrixToCurrentSegment(m, &projection);
}

// Scales and shifts the projection so the current segment fills clip space.
void CRenderTarget::adjustProjectionMatrixToCurrentSegment(Matrix *out, const Matrix *m) const
{
	if (_segCols < 2 && _segRows < 2) {
		*out = *m;
		return;
	}
	Matrix copy;
	if (out == m) {
		copy = *m;
		m = &copy;
	}
	float cols = (float)_segCols;
	float rows = (float)_segRows;
	float dx = (float)_segCol * -2.0f + (float)(_segCols - 1);
	float dy = (float)_segRow * 2.0f - (float)(_segRows - 1);
	out->_11 = cols * m->_11 + dx * m->_14;
	out->_21 = cols * m->_21 + dx * m->_24;
	out->_31 = cols * m->_31 + dx * m->_34;
	out->_41 = cols * m->_41 + dx * m->_44;
	out->_12 = rows * m->_12 + dy * m->_14;
	out->_22 = rows * m->_22 + dy * m->_24;
	out->_32 = rows * m->_32 + dy * m->_34;
	out->_42 = rows * m->_42 + dy * m->_44;
	out->_13 = m->_13;
	out->_23 = m->_23;
	out->_33 = m->_33;
	out->_43 = m->_43;
	out->_14 = m->_14;
	out->_24 = m->_24;
	out->_34 = m->_34;
	out->_44 = m->_44;
}

bool CRenderTarget::findBufferFromats(D3DFORMAT &color, D3DFORMAT &depth)
{
	depth = _context->FindZBufferFormat(_minDepthBits, _maxDepthBits, _minStencilBits, _maxStencilBits);
	if (depth == D3DFMT_UNKNOWN)
		return false;
	color = _context->FindRenderBuffFormat(D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, _minColorBits, _maxColorBits,
	                                       false);
	return _colorFormat != D3DFMT_UNKNOWN;
}

void CRenderTarget::instantiateSelf()
{
	if (findBufferFromats(_colorFormat, _depthFormat) && _useZBuffer)
		_context->checkDeviceFormat(_depthFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE);
}

void CRenderTarget::invalViewProjDepedences()
{
	_viewportValid = false;
	_frustumValid = false;
	_viewValid = false;
	_projectionValid = false;
	_viewProjValid = false;
	if (this == _context->currentRenderTarget())
		_context->invalViewProjDepedences();
}

void CRenderTarget::setCamera(CContextCamera *camera)
{
	if (_camera == camera)
		return;
	if (_camera)
		_camera->detachFromRenderTarget();
	LRetain(camera);
	LRelease(_camera);
	_camera = camera;
	if (camera)
		camera->attachToRenderTarget(this);
	invalViewProjDepedences();
}

void CRenderTarget::setOutSegment(unsigned int col, unsigned int row)
{
	_segCol = col;
	_segRow = row;
	invalViewProjDepedences();
}

void CRenderTarget::setOutSegmentsCount(unsigned int cols, unsigned int rows)
{
	_segCols = cols;
	_segRows = rows;
	invalViewProjDepedences();
}

BackBufferGrabber *CRenderTarget::detachBackBufferGrabber(LSize size, D3DTEXTUREFILTERTYPE filter)
{
	CHolder *holder = _context->holder();
	if (holder)
		holder->beginAccumulationBlock();
	BackBufferGrabber *grabber = BackBufferGrabber::_alloc()->initWithParams(this, size, filter);
	if (grabber)
		LAutorelease(grabber);
	if (holder)
		holder->endAccumulationBlock();
	return grabber;
}

// ---------------------------------------------------------------------------

CContextRenderTarget::CContextRenderTarget()
{
	_context = 0;
	_camera = 0;
}

CContextRenderTarget *CContextRenderTarget::initWithContext(CContext *context)
{
	initWithHolder(0);
	_width = 0x80;
	_height = 0x80;
	_useZBuffer = false;
	_context = context;
	restoreSelf();
	return this;
}

void CContextRenderTarget::dealloc()
{
	purgeSelf();
	CRenderTarget::dealloc();
}

float CContextRenderTarget::viewportAspect() const
{
	LSize size = _context->holder()->virtualFrameSize();
	return size.width / size.height;
}

void CContextRenderTarget::restoreSelf()
{
	if (_context->device() == 0)
		LF_RAISE(NilPtrException());
	LSize size = _context->holder()->systemFrameSize();
	_width = (long)size.width;
	_height = (long)size.height;
	invalViewProjDepedences();
	_useZBuffer = true;
}

// ---------------------------------------------------------------------------

CSceneStatePropertyOwner::CSceneStatePropertyOwner() : _stateOwnerName(""), _scenes(0), _storesState(true)
{
}

CSceneStatePropertyOwner::~CSceneStatePropertyOwner()
{
	if (_scenes) {
		for (unsigned int i = 0; i < _scenes->count(); i++) {
			CScene *scene = (*_scenes)[i];
			if (scene)
				FHSceneUnregisterStateOwner(scene, this);
		}
		delete _scenes;
		_scenes = 0;
	}
}

void CSceneStatePropertyOwner::wasRegisteredInScene(CScene *scene)
{
	if (_scenes == 0)
		_scenes = new TArray<CScene *>;
	_scenes->addObject(scene);
}

void CSceneStatePropertyOwner::hostSceneWillDie(CScene *scene)
{
	if (_scenes)
		_scenes->removeObject(scene);
}

bool CSceneStatePropertyOwner::isRegisteredInScene(CScene *scene)
{
	return _scenes && _scenes->containsObject(scene);
}

void CSceneStatePropertyOwner::storeStringInStream(LString *s, LData *stream)
{
	int length = s->length();
	storeIntInStream(length, stream);
	stream->appendBytes(s->chars(), length * 2);
}

const void *CSceneStatePropertyOwner::loadStringFromStream(const void *data, LString **s)
{
	unsigned int length = *(const unsigned int *)data;
	const char *p = (const char *)data + 4;
	if (length <= 100000000) {
		StPointerBlock chars(p, length * 2);
		*s = LString::stringWithCharacters((const UniChar *)chars.bytes(), length);
		p += length * 2;
	}
	return p;
}

// ---------------------------------------------------------------------------

CContextCamera::CContextCamera()
    : _position(0, 0, 0), _direction(0, 1, 0), _up(0, 0, 1), _near(0.01f), _far(10.0f), _fov(1.04719758f),
      _viewValid(false), _target(0)
{
}

CContextCamera *CContextCamera::init()
{
	return this;
}

void CContextCamera::dealloc()
{
	LObject::dealloc();
}

void CContextCamera::attachToRenderTarget(CRenderTarget *target)
{
	if (_target)
		LF_RAISE(WrongConditionException());
	_target = target;
}

void CContextCamera::setClipDistances(float nearZ, float farZ)
{
	_near = nearZ;
	_far = farZ;
	if (_target) {
		_target->_projectionValid = false;
		_target->_viewportValid = false;
		_target->_frustumValid = false;
	}
}

void CContextCamera::setFOV(float fov)
{
	_fov = fov;
	invalProjMatrixDependences();
}

void CContextCamera::setPosition(const Vector3 *v)
{
	_position = *v;
	_viewValid = false;
	if (_target) {
		_target->_frustumValid = false;
		_target->_viewValid = false;
	}
}

void CContextCamera::setViewDirection(const Vector3 *v)
{
	_direction = *v;
	_viewValid = false;
	if (_target) {
		_target->_frustumValid = false;
		_target->_viewValid = false;
	}
}

void CContextCamera::setViewUpVector(const Vector3 *v)
{
	_up = *v;
	_viewValid = false;
	if (_target) {
		_target->_frustumValid = false;
		_target->_viewValid = false;
	}
}

const Matrix *CContextCamera::viewMatrix() const
{
	if (!_viewValid) {
		recalcViewMatrix();
		_viewValid = true;
	}
	return &_view;
}

void CContextCamera::projectionMatrixForAspect(float aspect, Matrix *m) const
{
	if (m)
		FHMatrixPerspectiveFovRH(m, _fov, aspect, _near, _far);
}

void CContextCamera::invalProjMatrixDependences() const
{
	if (_target) {
		_target->_projectionValid = false;
		_target->_frustumValid = false;
	}
}

void CContextCamera::recalcViewMatrix() const
{
	Vector3 at = _position + _direction;
	FHMatrixLookAtRH(&_view, &_position, &at, &_up);
}

void CContextCamera::storeSceneStateInStream(LData *stream, unsigned int)
{
	float params[12] = { _position.x, _position.y, _position.z, _direction.x, _direction.y, _direction.z,
		             _up.x,       _up.y,       _up.z,       _near,        _far,         _fov };
	stream->appendBytes(params, sizeof(params));
}

const void *CContextCamera::restoreSceneStateFromData(const void *data, unsigned int)
{
	const float *params = (const float *)data;
	_position = Vector3(params[0], params[1], params[2]);
	_direction = Vector3(params[3], params[4], params[5]);
	_up = Vector3(params[6], params[7], params[8]);
	_near = params[9];
	_far = params[10];
	_fov = params[11];
	if (_target) {
		_target->_viewValid = false;
		_target->_projectionValid = false;
		_target->_frustumValid = false;
	}
	_viewValid = false;
	return params + 12;
}

// ---------------------------------------------------------------------------

COrthoCamera::COrthoCamera() : _frustumSize(0, 0)
{
}

COrthoCamera *COrthoCamera::init()
{
	return CContextCamera::init() ? this : 0;
}

void COrthoCamera::setFrustumSize(LSize size)
{
	_frustumSize = Vector2(size.width, size.height);
	invalProjMatrixDependences();
}

void COrthoCamera::projectionMatrixForAspect(float, Matrix *m) const
{
	if (m)
		FHMatrixOrthoRH(m, _frustumSize.x, _frustumSize.y, _near, _far);
}

void COrthoCamera::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	CContextCamera::storeSceneStateInStream(stream, flags);
	storeVectorInStream(&_frustumSize, stream);
}

const void *COrthoCamera::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	data = CContextCamera::restoreSceneStateFromData(data, flags);
	return loadVectorFromStream(data, &_frustumSize);
}

// A camera recorded in a frame becomes the main camera again.
void CContextCamera::renderSelfHolderInScene(CScene *scene, unsigned int)
{
	scene->holder()->_context->setMainCamera(this);
}
