// CContext: a GL context's device, caps, draw cache and render targets, the
// world transform and the matrices derived from it. CDrawCache: the device
// calls of a frame.

#include "FHContext.h"
#include "FHRender.h"
#include <math.h>

LF_IMPLEMENT_CLASS(CDrawCache, 0)

// _validMatrices bits.
enum {
	kWorldTranspValid = 0x1,
	kInvWorldValid = 0x2,
	kInvWorldTranspValid = 0x4,
	kWorldViewProjValid = 0x8,
	kWorldViewProjTranspValid = 0x10,
	kViewTranspValid = 0x20,
	kViewProjValid = 0x40,
	kViewProjTranspValid = 0x80,
	kInvViewProjTranspValid = 0x100,
	kWorldViewTranspValid = 0x200,
	kWorldDependent = 0x21f,
};

CContext::CContext(CHolder *holder, CDeviceFormat *)
    : _device(0), _caps(0), _lost(false), _shadowMapType(0), _shadowMapFormat(D3DFMT_UNKNOWN), _drawCallsCount(0),
      _primitivesCount(0), _renderEnvironment(0), _defaultStateBlock(0), _drawCache(0), _currentRenderTarget(0),
      _contextRenderTarget(0), _worldChanged(true), _validMatrices(0), _softwareVP(0xff), _specularTable(0),
      _fresnelTable(0), _holder(holder)
{
	_world.setIdentity();
	_device = DeviceEmu::_alloc()->initWithParams(this);
	_caps = new CContextCaps(this);
	_drawCache = CDrawCache::_alloc()->initWithContext(this);
	_contextRenderTarget = CContextRenderTarget::_alloc()->initWithContext(this);
	_renderEnvironment = new CRenderEnvironment(this);
	_defaultStateBlock = LRetain(CStateBlock::defaultStateWithContext(this));
	FindShadowMapsFormat();
	_drawCallsCount = 0;
	_primitivesCount = 0;
}

CContext::~CContext()
{
	LRelease(_drawCache);
	_drawCache = 0;
	LRelease(_defaultStateBlock);
	_defaultStateBlock = 0;
	LRelease(_contextRenderTarget);
	_contextRenderTarget = 0;
	delete _renderEnvironment;
	_renderEnvironment = 0;
	delete _caps;
	_caps = 0;
	FHSafeRelease(_specularTable);
	FHSafeRelease(_fresnelTable);
	FHSafeRelease(_device);
}

bool CContext::checkDeviceFormat(D3DFORMAT format, unsigned int, D3DRESOURCETYPE type)
{
	if (type == D3DRTYPE_VERTEXBUFFER)
		return format == D3DFMT_VERTEXDATA;
	if (type == D3DRTYPE_INDEXBUFFER)
		return format == D3DFMT_INDEX16 || format == D3DFMT_INDEX32;
	if (type >= D3DRTYPE_SURFACE && type <= D3DRTYPE_CUBETEXTURE) {
		if ((format == D3DFMT_DXT1 || format == D3DFMT_DXT3 || format == D3DFMT_DXT5) &&
		    _caps->CompressedTexturesAreSupported())
			return true;
		switch (format) {
		case D3DFMT_L8:
		case D3DFMT_A8:
		case D3DFMT_A8L8:
		case D3DFMT_R8G8B8:
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
		case D3DFMT_X4R4G4B4:
		case D3DFMT_A4R4G4B4:
			return true;
		default:
			break;
		}
	}
	return false;
}

// Picks the first depth format checkDeviceFormat accepts; it accepts none, so
// there is never a separate depth buffer.
D3DFORMAT CContext::FindZBufferFormat(long minDepth, long maxDepth, long minStencil, long maxStencil)
{
	if (maxDepth < minDepth)
		maxDepth = minDepth;
	TArray<D3DFORMAT> formats;
	if (maxDepth > 31)
		formats.addObject(D3DFMT_D32);
	if (maxDepth > 23 && minDepth < 25) {
		if (maxStencil > 7)
			formats.addObject(D3DFMT_D24S8);
		if (minStencil < 5 && maxStencil > 3)
			formats.addObject(D3DFMT_D24X4S4);
		if (maxStencil == 0 && minStencil == 0)
			formats.addObject(D3DFMT_D24X8);
	}
	if (maxDepth > 15 && minDepth < 17)
		formats.addObject(D3DFMT_D16);
	if (maxDepth > 14 && minDepth < 16 && minStencil < 2 && maxStencil > 0)
		formats.addObject(D3DFMT_D15S1);
	for (unsigned int i = 0; i < formats.count(); i++)
		checkDeviceFormat(formats[i], D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE);
	return D3DFMT_UNKNOWN;
}

D3DFORMAT CContext::FindRenderBuffFormat(unsigned int usage, D3DRESOURCETYPE type, long minBits, long maxBits,
                                         bool noAlpha)
{
	TArray<D3DFORMAT> formats;
	if (maxBits > 31) {
		formats.addObject(D3DFMT_A8R8G8B8);
		if (!noAlpha) {
			formats.addObject(D3DFMT_X8R8G8B8);
			formats.addObject(D3DFMT_R8G8B8);
		}
		formats.addObject(D3DFMT_A2B10G10R10);
	}
	if (maxBits > 15 && minBits < 17) {
		formats.addObject(D3DFMT_A4R4G4B4);
		formats.addObject(D3DFMT_A1R5G5B5);
		if (!noAlpha) {
			formats.addObject(D3DFMT_R5G6B5);
			formats.addObject(D3DFMT_X1R5G5B5);
		}
	}
	for (unsigned int i = 0; i < formats.count(); i++)
		if (checkDeviceFormat(formats[i], usage, type))
			return formats[i];
	return D3DFMT_UNKNOWN;
}

void CContext::FindShadowMapsFormat()
{
	_shadowMapType = 0;
	if (_caps->GetMaxSimultTextures() < 4)
		return;
	if (_caps->GetPixelShadersVer() >= 0x200) {
		if (FindZBufferFormat(15, 32, 0, 8) == D3DFMT_UNKNOWN)
			return;
		if (checkDeviceFormat(D3DFMT_R32F, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE)) {
			_shadowMapFormat = D3DFMT_R32F;
			_shadowMapType = 2;
		} else if (checkDeviceFormat(D3DFMT_R16F, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE)) {
			_shadowMapFormat = D3DFMT_R16F;
			_shadowMapType = 2;
			return;
		}
	}
	if (_caps->DepthTexturesAreSupported()) {
		if (FindRenderBuffFormat(D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, 16, 16, false) == D3DFMT_UNKNOWN &&
		    FindRenderBuffFormat(D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, 16, 32, false) == D3DFMT_UNKNOWN)
			return;
		_shadowMapType = 1;
	}
}

bool CContext::resetContext(CDeviceFormat *)
{
	_contextRenderTarget->purgeResources();
	_defaultStateBlock->purgeResources();
	_holder->backBufferSize();
	_contextRenderTarget->restoreResources();
	_defaultStateBlock->restoreResources();
	return true;
}

void CContext::resetRenderProfiles()
{
	if (_renderEnvironment->_activeProfile) {
		_renderEnvironment->_activeProfile->performDeactivation();
		_renderEnvironment->_activeProfile = 0;
	}
}

// ---------------------------------------------------------------------------
// Frames.

HRESULT CContext::beginFrame()
{
	if (_drawCache) {
		_world.setIdentity();
		_worldChanged = true;
		_validMatrices &= ~(unsigned int)kWorldDependent;
	}
	_currentRenderTarget = 0;
	setRenderTarget(0);
	_contextRenderTarget->invalViewProjDepedences();
	if (_drawCache) {
		HRESULT hr = _drawCache->beginDrawing();
		if (hr < 0)
			return hr;
	}
	LSize size = _holder->systemFrameSize();
	SViewPort viewport = { 0, 0, (int)size.width, (int)size.height, 0.0f, 1.0f };
	_drawCache->setViewport(&viewport);
	return D3D_OK;
}

HRESULT CContext::endFrame()
{
	HRESULT hr = D3D_OK;
	if (_drawCache)
		hr = _drawCache->endDrawing();
	if (_currentRenderTarget)
		_currentRenderTarget->detachFromContext();
	_currentRenderTarget = 0;
	return hr;
}

HRESULT CContext::presentFrame()
{
	return _drawCache ? _drawCache->present() : D3D_OK;
}

void CContext::flushBuffer()
{
	_holder->flushBuffer();
}

void CContext::setRenderTarget(CRenderTarget *target)
{
	if (target == 0)
		target = _contextRenderTarget;
	if (target == _currentRenderTarget)
		return;
	if (_drawCache) {
		if (_currentRenderTarget)
			_currentRenderTarget->detachFromContext();
		_currentRenderTarget = target;
		target->attachToContext();
		setZBufferEnable(_currentRenderTarget->usesZBuffer());
		_setTransforms(target);
	}
	_drawCache->setRenderState(D3DRS_FOGCOLOR, (unsigned int)target->fogColor());
}

void CContext::clearCurrentRenderTarget()
{
	if (_currentRenderTarget && _device)
		_drawCache->clear(_currentRenderTarget->clearColor(),
		                  D3DCLEAR_TARGET | (_currentRenderTarget->usesZBuffer() ? D3DCLEAR_ZBUFFER : 0));
}

CContextCamera *CContext::mainCamera() const
{
	return _contextRenderTarget->camera();
}

void CContext::setMainCamera(CContextCamera *camera)
{
	_contextRenderTarget->setCamera(camera);
	if (_currentRenderTarget == _contextRenderTarget)
		_setTransforms(_currentRenderTarget);
}

void CContext::setZBufferEnable(bool enable)
{
	_drawCache->setRenderState(D3DRS_ZENABLE, _currentRenderTarget->usesZBuffer() && enable ? 1 : 0);
}

void CContext::setSoftwareVertexProcessing(bool software)
{
	if (_softwareVP != (unsigned char)software) {
		_softwareVP = software;
		if (!_lost)
			_drawCache->setSoftwareVertexProcessing(software);
	}
}

void CContext::setDefaultDrawParameters()
{
	if (_defaultStateBlock)
		_drawCache->applyStateBlock(_defaultStateBlock);
	_drawCache->setPixelShader(0);
	_drawCache->setRenderState(D3DRS_CLIPPLANEENABLE, 0);
}

// ---------------------------------------------------------------------------
// Matrices.

void CContext::setWorldTransform(const Matrix *m)
{
	if (_drawCache) {
		_world = *m;
		_worldChanged = true;
		_validMatrices &= ~(unsigned int)kWorldDependent;
	}
}

void CContext::passWorldMatrixToDevIfNeeded()
{
	if (_drawCache && _worldChanged) {
		_drawCache->setTransform(D3DTS_WORLD, &_world);
		_worldChanged = false;
	}
}

void CContext::_setTransforms(CRenderTarget *target)
{
	if (_drawCache && target) {
		_drawCache->setTransform(D3DTS_VIEW, target->viewMatrix());
		_drawCache->setTransform(D3DTS_PROJECTION, target->projectionMatrix());
		invalViewProjDepedences();
	}
}

const Matrix *CContext::worldMatrixTransp()
{
	if (!(_validMatrices & kWorldTranspValid)) {
		_validMatrices |= kWorldTranspValid;
		FHMatrixTranspose(&_worldTransp, &_world);
	}
	return &_worldTransp;
}

const Matrix *CContext::invWorldMatrix()
{
	if (!(_validMatrices & kInvWorldValid)) {
		_validMatrices |= kInvWorldValid;
		FHMatrixInverse(&_invWorld, 0, &_world);
	}
	return &_invWorld;
}

const Matrix *CContext::invWorldMatrixTransp()
{
	if (!(_validMatrices & kInvWorldTranspValid)) {
		_validMatrices |= kInvWorldTranspValid;
		FHMatrixTranspose(&_invWorldTransp, invWorldMatrix());
	}
	return &_invWorldTransp;
}

const Matrix *CContext::worldViewMatrixTransp()
{
	if (!(_validMatrices & kWorldViewTranspValid)) {
		_validMatrices |= kWorldViewTranspValid;
		FHMatrixMultiply(&_worldViewTransp, &_world, _currentRenderTarget->viewMatrix());
		FHMatrixTranspose(&_worldViewTransp, &_worldViewTransp);
	}
	return &_worldViewTransp;
}

const Matrix *CContext::viewMatrixTransp()
{
	if (!(_validMatrices & kViewTranspValid)) {
		_validMatrices |= kViewTranspValid;
		FHMatrixTranspose(&_viewTransp, _currentRenderTarget->viewMatrix());
	}
	return &_viewTransp;
}

const Matrix *CContext::viewProjMatrix()
{
	if (!(_validMatrices & kViewProjValid)) {
		_validMatrices |= kViewProjValid;
		_viewProj = *_currentRenderTarget->viewProjMatrix();
	}
	return &_viewProj;
}

const Matrix *CContext::worldViewProjMatrixTransp()
{
	if (!(_validMatrices & kWorldViewProjTranspValid)) {
		_validMatrices |= kWorldViewProjTranspValid;
		if (!(_validMatrices & kWorldViewProjValid)) {
			_validMatrices |= kWorldViewProjValid;
			FHMatrixMultiply(&_worldViewProj, &_world, viewProjMatrix());
		}
		FHMatrixTranspose(&_worldViewProjTransp, &_worldViewProj);
	}
	return &_worldViewProjTransp;
}

const Matrix *CContext::viewProjMatrixTransp()
{
	if (!(_validMatrices & kViewProjTranspValid)) {
		_validMatrices |= kViewProjTranspValid;
		FHMatrixTranspose(&_viewProjTransp, viewProjMatrix());
	}
	return &_viewProjTransp;
}

const Matrix *CContext::invViewProjMatrixTransp()
{
	if (!(_validMatrices & kInvViewProjTranspValid)) {
		_validMatrices |= kInvViewProjTranspValid;
		FHMatrixInverse(&_invViewProjTransp, 0, viewProjMatrixTransp());
	}
	return &_invViewProjTransp;
}

// ---------------------------------------------------------------------------
// Visibility tests.

int CContext::boxIntersectionTest(const Vector3 *min, const Vector3 *max, bool useContextTarget)
{
	CRenderTarget *target = useContextTarget ? _contextRenderTarget : _currentRenderTarget;
	return UMathUtils::boxIntersectionTest(min, max, target->viewFrustum());
}

int CContext::boxIntersectionTest(const Vector3 *min, const Vector3 *max, const Matrix *m, bool useContextTarget)
{
	CRenderTarget *target = useContextTarget ? _contextRenderTarget : _currentRenderTarget;
	return UMathUtils::boxIntersectionTest(min, max, m, target->viewFrustum());
}

void CContext::projectVector(Vector3 *out, const Vector3 *v, const Matrix *m, float *invW)
{
	Vector4 p;
	if (m == 0) {
		FHVec3Transform(&p, v, viewProjMatrix());
	} else {
		Vector4 world;
		FHVec3Transform(&world, v, m);
		FHVec4Transform(&p, &world, viewProjMatrix());
	}
	float w = 1.0f / p.w;
	out->x = w * p.x;
	out->y = w * p.y;
	out->z = w * p.z;
	if (invW)
		*invW = w;
}

bool CContext::spriteProbe(const Vector3 *center, float width, float height, const Matrix *m, Vector3 *projected,
                           float *invW)
{
	Vector3 p;
	float w;
	projectVector(&p, center, m, &w);
	float halfW = w * 0.5f * width;
	float halfH = w * 0.5f * height;
	if (projected)
		*projected = p;
	if (invW)
		*invW = w;
	if (p.z < -1.0f || p.z > 1.0f || p.x - halfW >= 1.0f || p.x + halfW <= -1.0f || p.y - halfH >= 1.0f ||
	    p.y + halfH <= -1.0f)
		return false;
	return true;
}

bool CContext::sphereProbe(const Vector3 *center, float radius, const Matrix *m, Vector3 *projected, float *invW)
{
	CRenderTarget *target = _currentRenderTarget;
	Vector4 c;
	bool visible;
	if (m == 0) {
		visible = UMathUtils::sphereIntersectsViewFrustum(center, radius, target->viewFrustum());
		c = Vector4(*center, 1.0f);
	} else {
		FHVec3Transform(&c, center, m);
		Vector3 scaling;
		UMathUtils::scalingFromMatrix(m, &scaling);
		float s = scaling.x;
		if (s <= scaling.y)
			s = scaling.y;
		if (s <= scaling.z)
			s = scaling.z;
		Vector3 c3(c.x, c.y, c.z);
		visible = UMathUtils::sphereIntersectsViewFrustum(&c3, s * radius, target->viewFrustum());
	}
	Vector4 p;
	if (projected == 0) {
		if (invW) {
			FHVec4Transform(&p, &c, target->viewMatrix());
			const Matrix *proj = target->projectionMatrix();
			*invW = 1.0f / (proj->_14 * p.x + proj->_24 * p.y + proj->_34 * p.z + proj->_44);
		}
	} else {
		FHVec4Transform(&p, &c, viewProjMatrix());
		float w = 1.0f / p.w;
		projected->x = w * p.x;
		projected->y = w * p.y;
		projected->z = w * p.z;
		if (invW)
			*invW = w;
	}
	return visible;
}

// ---------------------------------------------------------------------------
// Lookup tables.

static float _clampTableArg(float x)
{
	if (x < 1e-5f)
		return 1e-5f;
	return x <= 1.0f ? x : 1.0f;
}

static void _fillTable(Texture2DEmu *texture, unsigned int (*valueAt)(float))
{
	SurfaceEmu *surface = 0;
	texture->GetSurfaceLevel(0, &surface);
	if (surface == 0)
		return;
	{
		StSurfaceLocker locker(surface, D3DLOCK_DISCARD);
		char *bits = (char *)locker.bits();
		for (int i = 0; i < 0x200; i++) {
			unsigned int v = valueAt(_clampTableArg((float)i / 511.0f));
			unsigned int pixel = 0xff000000 | v | v << 8 | v << 16;
			for (int row = 0; row < 2; row++)
				((unsigned int *)(bits + row * locker.pitch()))[i] = pixel;
		}
	}
	surface->Release();
}

static unsigned int _specularValue(float x)
{
	static const float kExponents[5] = { 50.0f, 120.0f, 80.0f, 100.0f, 1000.0f };
	static const float kWeights[5] = { 0.015f, 0.04f, 0.06f, 0.03f, 1.0f };
	static const float kCenters[5] = { 0.3f, 0.6f, 0.78f, 0.93f, 1.0f };
	float sum = 0.0f;
	for (int i = 0; i < 5; i++) {
		float d = x - kCenters[i];
		sum += kWeights[i] * powf(1.0f - d * d, kExponents[i] * 2.6f);
	}
	return (int)(_clampTableArg(sum) * 255.5f) & 0xff;
}

static unsigned int _fresnelValue(float x)
{
	return (int)((float)(pow(1.0 - x, 5.0) * 0.8799999952316284 + 0.11999999731779099) * 255.5f) & 0xff;
}

Texture2DEmu *CContext::specularTable()
{
	if (_specularTable == 0 &&
	    _device->CreateTexture(0x200, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &_specularTable) >= 0 &&
	    _specularTable)
		_fillTable(_specularTable, _specularValue);
	return _specularTable;
}

Texture2DEmu *CContext::fresnelTable()
{
	if (_fresnelTable == 0 &&
	    _device->CreateTexture(0x200, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &_fresnelTable) >= 0 &&
	    _fresnelTable)
		_fillTable(_fresnelTable, _fresnelValue);
	return _fresnelTable;
}

// ---------------------------------------------------------------------------

CDrawCache::CDrawCache() : _context(0), _device(0), _texturesCount(0), _texturesSet(0)
{
}

CDrawCache *CDrawCache::initWithContext(CContext *context)
{
	_context = context;
	_device = context->device();
	_device->AddRef();
	_texturesCount = _context->caps()->GetMaxSimultTextures();
	_texturesSet = new bool[_texturesCount];
	memset(_texturesSet, 0, _texturesCount);
	return this;
}

void CDrawCache::dealloc()
{
	FHSafeRelease(_device);
	delete[] _texturesSet;
	_texturesSet = 0;
	LObject::dealloc();
}

void CDrawCache::startTexturesSetup()
{
	memset(_texturesSet, 0, _texturesCount);
}

// Clears the stages not given a texture since startTexturesSetup.
void CDrawCache::endTexturesSetup()
{
	for (unsigned int i = 0; i < _texturesCount; i++) {
		if (!_texturesSet[i])
			setTexture(i, 0);
	}
}

void CDrawCache::setTexture(unsigned int stage, BaseTextureEmu *texture)
{
	_device->SetTexture(stage, texture);
	if (stage < _texturesCount)
		_texturesSet[stage] = true;
}

void CDrawCache::applyStateBlock(CStateBlock *block)
{
	block->applySelf();
}

void CDrawCache::drawPrimitive(D3DPRIMITIVETYPE type, unsigned int startVertex, unsigned int primCount)
{
	_context->_primitivesCount += primCount;
	_device->DrawPrimitive(type, startVertex, primCount);
}

void CDrawCache::drawIndexedPrimitive(D3DPRIMITIVETYPE type, int baseVertexIndex, unsigned int minIndex,
                                      unsigned int numVertices, unsigned int startIndex, unsigned int primCount)
{
	_context->_primitivesCount += primCount;
	_device->DrawIndexedPrimitive(type, baseVertexIndex, minIndex, numVertices, startIndex, primCount);
}

void CDrawCache::clear(const Color &color, unsigned int flags)
{
	GLbitfield mask = (flags & D3DCLEAR_TARGET) ? GL_COLOR_BUFFER_BIT : 0;
	if (flags & D3DCLEAR_ZBUFFER)
		mask |= GL_DEPTH_BUFFER_BIT;
	if (flags & D3DCLEAR_STENCIL)
		mask |= GL_STENCIL_BUFFER_BIT;
	glClearColor(color.r, color.g, color.b, color.a);
	glClear(mask);
}

HRESULT CDrawCache::present()
{
	GLBufferImp::unbindAll(_context->caps()->VBOsAreSupported());
	_context->flushBuffer();
	return D3D_OK;
}

void CDrawCache::setViewport(const SViewPort *viewport)
{
	_device->ValidateContext();
	glViewport(viewport->x, viewport->y, viewport->width, viewport->height);
}
