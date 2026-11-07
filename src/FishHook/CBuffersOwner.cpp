// CBuffersOwner: a model drawn from vertex and index buffers its data
// source gives.

#include "FHModels.h"

LF_IMPLEMENT_CLASS(CBuffersOwner, LF_CLASSOF(CDrawableModel))

CBuffersOwner::CBuffersOwner()
    : CDrawableModel(10), _profile(0), _ib(0), _buffersBound(false), _streamsCount(0), _subsetsCount(1), _shader(0),
      _ds(0), _dsOwner(0), _primType(D3DPT_TRIANGLELIST), _primCount(0), _numVertices(0)
{
	for (int i = 0; i < kMaxShaderStreams; i++)
		_vbs[i] = 0;
}

CBuffersOwner *CBuffersOwner::initWithDataSource(IBuffersOwnerDataSource *source, CRenderProfile *profile,
                                                 unsigned char retainSource)
{
	if (profile == 0) {
		release();
		return 0;
	}
	if (initWithHolder(profile->_holder) == 0)
		return 0;
	_ds = source;
	if (retainSource) {
		_dsOwner = source ? dynamic_cast<LObject *>(source) : 0;
		if (_dsOwner)
			_dsOwner->retain();
	}
	setRenderProfile(profile);
	return this;
}

void CBuffersOwner::dealloc()
{
	invalidateBuffers();
	LRelease(_dsOwner);
	_dsOwner = 0;
	LRelease(_profile);
	_profile = 0;
	CContextObject::dealloc();
}

void CBuffersOwner::dataSourceWillDie()
{
	LRelease(_dsOwner);
	_dsOwner = 0;
	_ds = 0;
}

unsigned int CBuffersOwner::numVertsForPrimitive(D3DPRIMITIVETYPE type)
{
	switch (type) {
	case D3DPT_LINELIST:
		return 2;
	case D3DPT_TRIANGLELIST:
		return 3;
	case D3DPT_POINTLIST:
		return 1;
	default:
		return 0;
	}
}

void CBuffersOwner::setRenderProfile(CRenderProfile *profile)
{
	LRetain(profile);
	LRelease(_profile);
	_shader = 0;
	_profile = profile;
	_drawableType = 0;
}

void CBuffersOwner::invalidateBuffers()
{
	for (int i = 0; i < kMaxShaderStreams; i++) {
		LRelease(_vbs[i]);
		_vbs[i] = 0;
	}
	LRelease(_ib);
	_ib = 0;
	_buffersBound = false;
}

void CBuffersOwner::setIndexBuffer(CIndexBuffer *ib)
{
	LRetain(ib);
	LRelease(_ib);
	_ib = ib;
}

void CBuffersOwner::setVertexBufferForStream(CVertexBuffer *vb, int stream)
{
	if ((unsigned int)stream < kMaxShaderStreams) {
		LRetain(vb);
		LRelease(_vbs[stream]);
		_vbs[stream] = vb;
	}
}

StVertexBufferLocker *CBuffersOwner::detachVBLocker(int stream, unsigned int flags)
{
	if ((unsigned int)stream < kMaxShaderStreams && _vbs[stream])
		return _vbs[stream]->detachLocker(0);
	return 0;
}

StIndexBufferLocker *CBuffersOwner::detachIBLocker(unsigned int)
{
	return _ib ? _ib->detachLocker(0) : 0;
}

long CBuffersOwner::beginDrawing(CDrawableModelData *data)
{
	long stages = _profile->activateProfile(data);
	if (stages) {
		_context->setWorldTransform(data ? data->worldMatrix() : sIdentityMatrixPtr);
		bindBuffers();
	}
	return stages;
}

bool CBuffersOwner::activateStage(long stage)
{
	return _profile->activateDrawStage(stage);
}

void CBuffersOwner::drawSubset(CDrawableModelData *, long, long)
{
	simpleDraw(_primCount);
}

void CBuffersOwner::endDrawing()
{
	_profile->deactivateProfile();
}

void CBuffersOwner::simpleDraw(unsigned int primCount)
{
	CDrawCache *cache = _context->_drawCache;
	if (_ib == 0)
		cache->drawPrimitive(_primType, 0, primCount);
	else
		cache->drawIndexedPrimitive(_primType, 0, 0, _numVertices, 0, primCount);
}

// Asks the data source for the buffers the first time, then sets them on
// the device.
void CBuffersOwner::bindBuffers()
{
	if (!_buffersBound) {
		for (int i = 0; i < kMaxShaderStreams; i++) {
			LRelease(_vbs[i]);
			_vbs[i] = 0;
		}
		LRelease(_ib);
		_ib = 0;
		CShader *shader = _profile->_shader;
		_numVertices = 0;
		_streamsCount = shader->_streamsCount;
		for (unsigned int stream = 0; stream < _streamsCount; stream++) {
			if (!shader->_instancingStream[stream & 0xffff]) {
				CVertexBuffer *vb = _ds->vertexBufferForBufOwner(stream, this);
				setVertexBufferForStream(vb, stream);
				if (stream == 0 && vb)
					_numVertices = vb->_count;
			}
		}
		_primType = _ds->primitiveTypeForBufOwner(this);
		if (_primType != D3DPT_POINTLIST)
			setIndexBuffer(_ds->indexBufferForBufOwner(this));
		if (_ib)
			_primCount = _ib->_primCount;
		else if (_primType == D3DPT_POINTLIST)
			_primCount = _numVertices;
		else
			_primCount = 0;
		_buffersBound = true;
	}
	CDrawCache *cache = _context->_drawCache;
	cache->setIndexBuffer(_ib ? _ib->_ib : 0);
	for (int i = 0; i < kMaxShaderStreams; i++)
		if (_vbs[i])
			cache->setStreamSource(i, _vbs[i]->_vb, _vbs[i]->_stride);
}

void CBuffersOwner::doImmediateDrawWithData(CDrawableModelData *data, CFrameDrawer *)
{
	CShader *saved = _shader;
	_shader = _profile->_shader;
	long stages = beginDrawing(data);
	if (stages) {
		long index = 0;
		for (long stage = 0; stage != stages; stage++) {
			if (!activateStage(stage))
				continue;
			for (unsigned int subset = 0; subset < _subsetsCount; subset++) {
				CShaderStage *shaderStage = _shader ? _shader->_currentStage : 0;
				if (data->applyMaterialForSubset(subset, shaderStage))
					drawSubset(data, subset, index++);
			}
		}
		endDrawing();
	}
	_shader = saved;
}

// doImmediateDrawMultiInstance and drawSubsetMultiInstance are with
// CParticleSystem.cpp.
