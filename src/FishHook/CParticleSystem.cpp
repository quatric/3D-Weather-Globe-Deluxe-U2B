// Particle systems, their subsystems, data and draw proxies, and the
// multi-instance drawing of CBuffersOwner they use.

#include "FHParticles.h"

#include <memory>

LF_IMPLEMENT_CLASS(CParticleSubsystem, LF_CLASSOF(CBuffersOwner))
LF_IMPLEMENT_CLASS(CParticleSubsystemData, LF_CLASSOF(ModelDataWithMatrix))
LF_IMPLEMENT_CLASS(CParticleSystem, LF_CLASSOF(CDrawableModel))
LF_IMPLEMENT_CLASS(CParticleSystemData, LF_CLASSOF(ModelDataWithMatrix))
LF_IMPLEMENT_CLASS(CParticleSystemDrawProxy, LF_CLASSOF(ModelDataWithMatrix))

typedef CParticleSystemData::SStoredParticle SStoredParticle;

// ---------------------------------------------------------------------------
// CBuffersOwner drawn once a particle of an instancing system.

void CBuffersOwner::doImmediateDrawMultiInstance(CParticleSystemData *data, CFrameDrawer *drawer)
{
	CShader *saved = _shader;
	_shader = _profile->_shader;
	long stages = beginDrawing(data);
	if (stages) {
		for (long stage = 0; stage != stages; stage++) {
			if (!activateStage(stage))
				continue;
			for (unsigned int subset = 0; subset < _subsetsCount; subset++) {
				CShaderStage *shaderStage = _shader ? _shader->_currentStage : 0;
				if (data->applyMaterialForSubset(subset, shaderStage))
					drawSubsetMultiInstance(data, subset, stage);
			}
		}
		endDrawing();
	}
	(void)drawer;
	_shader = saved;
}

// Sets the stage's instance constants of each particle and draws it.
void CBuffersOwner::drawSubsetMultiInstance(CParticleSystemData *data, long, long)
{
	CDrawCache *cache = _context->_drawCache;
	int count = (int)data->_count;
	const SShaderConstDescr *descr = renderProfile()->_shader->_currentStage->instanceVSConstantDescription();
	if (descr == 0 || descr->reg < 0)
		return;
	StPointerBlock buffer(descr->count * 16, false);
	for (int i = 0; i < count; i++) {
		const float *values =
		    (const float *)static_cast<CInstanceConstDataProvider *>(data)->instanceDataPtrForConstant(i, buffer.bytes());
		if (values)
			cache->setVSConstant(descr->reg, values, descr->count);
		if (_ib == 0)
			cache->drawPrimitive(_primType, 0, _primCount);
		else
			cache->drawIndexedPrimitive(_primType, 0, 0, _numVertices, 0, _primCount);
	}
}

// ---------------------------------------------------------------------------
// CParticleSubsystem

CParticleSubsystem::CParticleSubsystem()
    : _count(0), _first(0), _end(0), _mode(kPartSysNone), _ownIB(0), _system(0), _index(0)
{
}

CParticleSubsystem *CParticleSubsystem::initWithParticlesCount(unsigned int count, unsigned int first,
                                                               TPartSysRepresentationMode mode,
                                                               CRenderProfile *profile)
{
	if (initWithDataSource(this, profile, false) == 0)
		return 0;
	_first = first;
	_count = count;
	_end = first + count;
	_mode = mode;
	return this;
}

void CParticleSubsystem::dealloc()
{
	LRelease(_ownIB);
	_ownIB = 0;
	CBuffersOwner::dealloc();
}

LClass *CParticleSubsystem::modelDataClass()
{
	return LF_CLASSOF(CParticleSubsystemData);
}

D3DPRIMITIVETYPE CParticleSubsystem::primitiveTypeForBufOwner(CBuffersOwner *)
{
	return _mode == kPartSysPoints ? D3DPT_POINTLIST : D3DPT_TRIANGLELIST;
}

CVertexBuffer *CParticleSubsystem::vertexBufferForBufOwner(int stream, CBuffersOwner *)
{
	return stream == 0 ? _system->vertexBufferForSubModelAtIndex(_index) : 0;
}

CShader *CParticleSubsystem::renderShaderForBuffer(CIndexBuffer *)
{
	return renderProfile()->_shader;
}

bool CParticleSubsystem::use32BitsForBuffer(CIndexBuffer *)
{
	if (_mode != kPartSysQuads)
		LF_RAISE(WrongConditionException());
	return _count > 0xffff;
}

unsigned int CParticleSubsystem::indsCountForBuffer(CIndexBuffer *)
{
	if (_mode != kPartSysQuads)
		LF_RAISE(WrongConditionException());
	return _count * 6;
}

// The constants, textures, stages and clip planes are the system's.
const void *CParticleSubsystem::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	return _system->_dataPtrForConstant(descr, buffer);
}

bool CParticleSubsystem::_getTextureForSource(BaseTextureEmu **texture, int source)
{
	return static_cast<CDeviceTextureProvider *>(_system)->_getTextureForSource(texture, source);
}

bool CParticleSubsystem::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	return static_cast<CShaderStageStateProvider *>(_system)->_isStageOn(stage, on);
}

bool CParticleSubsystem::_getClipPlanes(CClipPlane ***planes, int *count)
{
	return static_cast<CClipPlanesProvider *>(_system)->_getClipPlanes(planes, count);
}

// With unshared buffers the data's vertex buffers are set before binding.
long CParticleSubsystem::beginDrawing(CDrawableModelData *data)
{
	if (!_system->_sharedBuffers)
		for (unsigned int stream = 0; stream < _streamsCount; stream++)
			setVertexBufferForStream(vertexBufferForBufOwner(stream, this), stream);
	return CBuffersOwner::beginDrawing(data);
}

void CParticleSubsystem::drawSubset(CDrawableModelData *, long, long)
{
	unsigned int count = _end - _first;
	if (_count < count)
		count = _count;
	if ((int)count > 0)
		simpleDraw(_mode == kPartSysQuads ? count * 2 : count);
}

// Two triangles a quad: 0 3 1, 0 2 3.
void CParticleSubsystem::fillBuffer(CIndexBuffer *ib)
{
	if (_mode != kPartSysQuads)
		return;
	std::unique_ptr<StIndexBufferLocker> locker(ib->detachLocker(0));
	unsigned int count = _count;
	if (locker->_desc.Format == D3DFMT_INDEX32) {
		uint32_t *p = (uint32_t *)locker->data();
		for (unsigned int i = 0, v = 0; i < count; i++, v += 4, p += 6) {
			p[0] = v;
			p[1] = v + 3;
			p[2] = v + 1;
			p[3] = v;
			p[4] = v + 2;
			p[5] = v + 3;
		}
	} else {
		uint16_t *p = (uint16_t *)locker->data();
		for (unsigned int i = 0; i < count; i++, p += 6) {
			uint16_t v = (uint16_t)(i * 4);
			p[0] = v;
			p[1] = v + 3;
			p[2] = v + 1;
			p[3] = v;
			p[4] = v + 2;
			p[5] = v + 3;
		}
	}
}

CIndexBuffer *CParticleSubsystem::indexBufferForBufOwner(CBuffersOwner *owner)
{
	if (_ownIB == 0) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_ownIB = CIndexBuffer::_alloc()->initWithDataSource(this, primitiveTypeForBufOwner(owner), _holder, 0, 0);
		if (holder)
			holder->endAccumulationBlock();
	}
	return _ownIB;
}

// Points go 64000 a buffer; quads as many as the device indexes.
LArray *CParticleSubsystem::subsystemsWithMaxParticlesCount(CParticleSystem *system, unsigned int count,
                                                            TPartSysRepresentationMode mode,
                                                            CRenderProfile *profile, LClass *cls)
{
	if (cls == 0)
		cls = LF_CLASSOF(CParticleSubsystem);
	LArray *subsystems = LArray::array();
	unsigned int perBuffer;
	if (mode == kPartSysPoints)
		perBuffer = 64000;
	else if (mode == kPartSysQuads) {
		perBuffer = 0x3ffc;
		CContext *context = profile->_holder->_context;
		if (context)
			perBuffer = context->_caps->GetMaxVertexIndex() >> 2;
	} else
		return 0;
	unsigned int first = 0;
	while (count) {
		unsigned int n = count < perBuffer ? count : perBuffer;
		count -= n;
		CParticleSubsystem *sub = (CParticleSubsystem *)cls->alloc();
		if (sub->initWithDataSource(sub, profile, false) == 0)
			sub = 0;
		else {
			sub->_count = n;
			sub->_first = first;
			sub->_end = first + n;
			sub->_mode = mode;
		}
		first += n;
		if (sub) {
			sub->_system = system;
			sub->_index = subsystems->count();
			subsystems->addObject(sub);
			sub->release();
		}
	}
	return subsystems;
}

// ---------------------------------------------------------------------------
// CParticleSubsystemData: everything from the system data.

const void *CParticleSubsystemData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	return static_cast<CDrawableModelData *>(_systemData)->_dataPtrForConstant(descr, buffer);
}

bool CParticleSubsystemData::_getTextureForSource(BaseTextureEmu **texture, int source)
{
	return static_cast<CDrawableModelData *>(_systemData)->_getTextureForSource(texture, source);
}

bool CParticleSubsystemData::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	return static_cast<CShaderStageStateProvider *>(_systemData)->_isStageOn(stage, on);
}

bool CParticleSubsystemData::_getClipPlanes(CClipPlane ***planes, int *count)
{
	return static_cast<CClipPlanesProvider *>(_systemData)->_getClipPlanes(planes, count);
}

// ---------------------------------------------------------------------------
// CParticleSystem

CParticleSystem::CParticleSystem()
    : CDrawableModel(10), _profile(0), _subsystems(0), _mode(kPartSysNone), _maxParticles(0), _sharedBuffers(true),
      _vbs(0), _instanceModel(0), _instancingVB(0)
{
}

void CParticleSystem::dealloc()
{
	LRelease(_subsystems);
	_subsystems = 0;
	LRelease(_profile);
	_profile = 0;
	LRelease(_vbs);
	_vbs = 0;
	LRelease(_instanceModel);
	_instanceModel = 0;
	LRelease(_instancingVB);
	_instancingVB = 0;
	CDrawableModel::dealloc();
}

LClass *CParticleSystem::modelDataClass()
{
	return LF_CLASSOF(CParticleSystemData);
}

CParticleSystem *CParticleSystem::initWithParams(unsigned char sharedBuffers, unsigned int maxParticles,
                                                 TPartSysRepresentationMode mode, CRenderProfile *profile,
                                                 CBuffersOwner *instanceModel, LClass *subsystemClass)
{
	if (initWithHolder(profile->_holder) == 0)
		return 0;
	_profile = profile;
	LRetain(profile);
	_mode = mode;
	if (mode <= kPartSysNone)
		return 0;
	if (mode < kPartSysInstances) {
		_subsystems = CParticleSubsystem::subsystemsWithMaxParticlesCount(this, maxParticles, mode, profile,
		                                                                   subsystemClass);
		LRetain(_subsystems);
	} else if (mode == kPartSysInstances) {
		_instanceModel = instanceModel;
		LRetain(instanceModel);
	} else
		return 0;
	_maxParticles = maxParticles;
	_sharedBuffers = sharedBuffers;
	return this;
}

unsigned int CParticleSystem::streamNumberForBuffer(CVertexBuffer *vb)
{
	return _instancingVB == vb;
}

CShader *CParticleSystem::renderShaderForBuffer(CVertexBuffer *)
{
	return renderProfile()->_shader;
}

unsigned int CParticleSystem::vertCountForBuffer(CVertexBuffer *vb)
{
	if (_instancingVB == vb)
		return _maxParticles;
	if (_vbs && _sharedBuffers && _subsystems) {
		int index = _vbs->indexOfObject(vb);
		if (index >= 0) {
			CParticleSubsystem *sub = (CParticleSubsystem *)_subsystems->objectAtIndex(index);
			if (_mode == kPartSysPoints)
				return sub->_count;
			if (_mode == kPartSysQuads)
				return sub->_count * 4;
		}
	}
	return 0;
}

// The current stage of the instance model's profile gives the instance
// constants.
CInstanceConstDataProvider *CParticleSystem::_nextInstanceConstDataProvider()
{
	if (_instanceModel) {
		CRenderProfile *profile = _instanceModel->renderProfile();
		if (profile && profile->_shader && profile->_shader->_currentStage)
			return profile->_shader->_currentStage;
	}
	return 0;
}

// One vertex a particle, or four for quads with the corner 0-3 in the
// position's w.
template <bool kQuads, bool kColor, bool kSize>
static void __t_partSysFillVertexBuffer(char *dst, unsigned int stride, int posOffset, int colorOffset, int sizeOffset,
                                        const char *src, unsigned int srcStride, long count)
{
	for (long i = 0; i < count; i++, src += srcStride) {
		const SStoredParticle *p = (const SStoredParticle *)src;
		for (int corner = 0; corner < (kQuads ? 4 : 1); corner++, dst += stride) {
			memcpy(dst + posOffset, &p->position, sizeof(Vector3));
			if (kColor)
				memcpy(dst + colorOffset, &p->color, 4);
			if (kSize)
				memcpy(dst + sizeOffset, &p->size, 4);
			if (kQuads) {
				float w = (float)corner;
				memcpy(dst + posOffset + 12, &w, 4);
			}
		}
	}
}

template <bool kQuads>
static void _partSysFill(char *dst, unsigned int stride, int pos, int color, int size, const char *src,
                         unsigned int srcStride, long count)
{
	if (color >= 0 && size >= 0)
		__t_partSysFillVertexBuffer<kQuads, true, true>(dst, stride, pos, color, size, src, srcStride, count);
	else if (color >= 0)
		__t_partSysFillVertexBuffer<kQuads, true, false>(dst, stride, pos, color, size, src, srcStride, count);
	else if (size >= 0)
		__t_partSysFillVertexBuffer<kQuads, false, true>(dst, stride, pos, color, size, src, srcStride, count);
	else
		__t_partSysFillVertexBuffer<kQuads, false, false>(dst, stride, pos, color, size, src, srcStride, count);
}

void CParticleSystem::_fillParticlesModelVB(CVertexBuffer *vb, CParticleSystemData *data, void *particles,
                                            unsigned int count)
{
	unsigned int srcStride = data->particleStructSize();
	CRenderProfile *profile = renderProfile();
	std::unique_ptr<StVertexBufferLocker> locker(vb->detachLocker(0));
	int pos = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	if (pos < 0)
		return;
	int color = profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
	int size = profile->drawableElementOffset(D3DDECLUSAGE_PSIZE, 0, 0);
	char *dst = (char *)locker->data();
	unsigned int stride = profile->vertexStructSize(0);
	if (_mode == kPartSysQuads)
		_partSysFill<true>(dst, stride, pos, color, size, (const char *)particles, srcStride, count);
	else
		_partSysFill<false>(dst, stride, pos, color, size, (const char *)particles, srcStride, count);
}

// The instance stream: each particle's instance constants.
void CParticleSystem::_fillParticlesModelInstanceVB(CVertexBuffer *vb, CParticleSystemData *data)
{
	CRenderProfile *profile = renderProfile();
	std::unique_ptr<StVertexBufferLocker> locker(vb->detachLocker(0));
	char *dst = (char *)locker->data();
	unsigned int stride = profile->vertexStructSize(0);
	for (unsigned int i = 0; i < _maxParticles; i++, dst += stride)
		static_cast<CInstanceConstDataProvider *>(data)->instanceDataPtrForConstant(i, dst);
}

CVertexBuffer *CParticleSystem::instancingVertexBuffer()
{
	if (!_sharedBuffers) {
		CParticleSystemData *data = lobj_cast<CParticleSystemData>(_currentData);
		return data ? data->instancingVertexBuffer() : 0;
	}
	if (_instancingVB == 0) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_instancingVB = CVertexBuffer::_alloc()->initWithDataSource(this, _holder, 0, 1);
		if (holder)
			holder->endAccumulationBlock();
	}
	return _instancingVB;
}

CVertexBuffer *CParticleSystem::vertexBufferForSubModelAtIndex(unsigned int index)
{
	if (!_sharedBuffers) {
		CParticleSystemData *data = lobj_cast<CParticleSystemData>(_currentData);
		return data ? data->vertexBufferForSubModelAtIndex(index) : 0;
	}
	if (_vbs == 0) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_vbs = LArray::_alloc()->init();
		int count = _subsystems ? _subsystems->count() : 0;
		for (int i = 0; i < count; i++) {
			CVertexBuffer *vb = CVertexBuffer::_alloc()->initWithDataSource(this, _holder, 0, 1);
			if (vb) {
				_vbs->addObject(vb);
				vb->release();
			}
		}
		if (holder)
			holder->endAccumulationBlock();
	}
	return (CVertexBuffer *)_vbs->objectAtIndex(index);
}

// Draws the subsystems holding the data's particles; shared buffers are
// refilled with them first. Instancing systems draw the instance model once
// a particle.
void CParticleSystem::doImmediateDrawWithData(CDrawableModelData *modelData, CFrameDrawer *drawer)
{
	CParticleSystemData *data = lobj_cast<CParticleSystemData>(modelData);
	if (data == 0)
		return;
	unsigned int count = data->_count;
	if (_mode == kPartSysInstances) {
		if (_instanceModel)
			_instanceModel->doImmediateDrawMultiInstance(data, drawer);
		return;
	}
	int subsCount = data->_subDatas ? data->_subDatas->count() : 0;
	for (int i = 0; i < subsCount; i++) {
		CParticleSubsystem *sub = (CParticleSubsystem *)_subsystems->objectAtIndex(i);
		CDrawableModelData *subData = (CDrawableModelData *)data->_subDatas->objectAtIndex(i);
		sub->_end = count;
		if (sub->_first >= count)
			continue;
		if (subData && subData->getClass()->isSubclassOf(LF_CLASSOF(ModelDataWithMatrix)))
			((ModelDataWithMatrix *)subData)->_matrix = *modelData->worldMatrix();
		if (_sharedBuffers) {
			unsigned int first = sub->_first;
			int n = (int)(count - first);
			CVertexBuffer *vb = vertexBufferForSubModelAtIndex(i);
			if (n > 0)
				_fillParticlesModelVB(vb, data, (char *)data->_particles + first * data->particleStructSize(), n);
		}
		sub->doImmediateDrawWithData(subData, drawer);
	}
}

// ---------------------------------------------------------------------------
// CParticleSystemData

CParticleSystemData::CParticleSystemData()
    : _subDatas(0), _particles(0), _count(0), _vbsDirty(true), _instancingDirty(true), _vbs(0), _instancingVB(0)
{
}

void CParticleSystemData::dealloc()
{
	LRelease(_subDatas);
	_subDatas = 0;
	LRelease(_vbs);
	_vbs = 0;
	LRelease(_instancingVB);
	_instancingVB = 0;
	if (_particles) {
		free(_particles);
		_particles = 0;
	}
	CDrawableModelData::dealloc();
}

// A subsystem data for each subsystem, and room for the most particles.
CDrawableModelData *CParticleSystemData::initWithHostModel(CDrawableModel *model)
{
	if (CDrawableModelData::initWithHostModel(model) == 0)
		return 0;
	CParticleSystem *system = lobj_cast<CParticleSystem>(model);
	if (system == 0)
		return this;
	_subDatas = LArray::_alloc()->init();
	TPartSysRepresentationMode mode = system->_mode;
	if (mode <= kPartSysNone)
		return 0;
	if (mode < kPartSysInstances) {
		int count = system->_subsystems ? system->_subsystems->count() : 0;
		for (int i = 0; i < count; i++) {
			CParticleSubsystem *sub = (CParticleSubsystem *)system->_subsystems->objectAtIndex(i);
			CDrawableModelData *subData = sub->detachModelDataInstance();
			if (subData && subData->getClass()->isSubclassOf(LF_CLASSOF(CParticleSubsystemData))) {
				_subDatas->addObject(subData);
				((CParticleSubsystemData *)subData)->_systemData = this;
			}
		}
	} else if (mode != kPartSysInstances)
		return 0;
	_count = system->_maxParticles;
	_particles = malloc(system->_maxParticles * particleStructSize());
	memset(_particles, 0, _count * sizeof(SStoredParticle));
	return this;
}

LClass *CParticleSystemData::drawProxyClass()
{
	return LF_CLASSOF(CParticleSystemDrawProxy);
}

CDrawableModelData *CParticleSystemData::detachDrawProxy()
{
	LObject *obj = LAutorelease(drawProxyClass()->alloc());
	if (obj && obj->getClass()->isSubclassOf(LF_CLASSOF(CParticleSystemDrawProxy))) {
		((CParticleSystemDrawProxy *)obj)->initWithPSystemData(this);
		return (CParticleSystemDrawProxy *)obj;
	}
	return 0;
}

// At most the system's maximum; more particles than before need refilling.
unsigned int CParticleSystemData::setParticlesCount(unsigned int count)
{
	CParticleSystem *system = lobj_cast<CParticleSystem>(_model);
	if (system && count > system->_maxParticles)
		count = system->_maxParticles;
	if (count > _count)
		_vbsDirty = _instancingDirty = true;
	_count = count;
	return count;
}

CShader *CParticleSystemData::renderShaderForBuffer(CVertexBuffer *)
{
	return _model->renderProfile()->_shader;
}

unsigned int CParticleSystemData::streamNumberForBuffer(CVertexBuffer *)
{
	CParticleSystem *system = lobj_cast<CParticleSystem>(_model);
	return system && system->_mode == kPartSysInstances ? 1 : 0;
}

void CParticleSystemData::fillBuffer(CVertexBuffer *vb)
{
	CParticleSystem *system = lobj_cast<CParticleSystem>(_model);
	if (system == 0 || system->_mode <= kPartSysNone)
		return;
	if (system->_mode < kPartSysInstances) {
		int count = _vbs ? _vbs->count() : 0;
		for (int i = 0; i < count; i++) {
			if (_vbs->objectAtIndex(i) != vb)
				continue;
			CParticleSubsystem *sub = (CParticleSubsystem *)system->_subsystems->objectAtIndex(i);
			system->_fillParticlesModelVB(vb, this, (char *)_particles + particleStructSize() * sub->_first,
			                              sub->_count);
		}
	} else if (system->_mode == kPartSysInstances)
		system->_fillParticlesModelInstanceVB(_instancingVB, this);
}

unsigned int CParticleSystemData::vertCountForBuffer(CVertexBuffer *vb)
{
	CParticleSystem *system = lobj_cast<CParticleSystem>(_model);
	if (system == 0)
		return 0;
	if (system->_mode == kPartSysInstances)
		return system->_maxParticles;
	if (_vbs && _subDatas) {
		int index = _vbs->indexOfObject(vb);
		if (index >= 0) {
			CParticleSubsystem *sub = (CParticleSubsystem *)system->_subsystems->objectAtIndex(index);
			if (system->_mode == kPartSysPoints)
				return sub->_count;
			if (system->_mode == kPartSysQuads)
				return sub->_count * 4;
		}
	}
	return 0;
}

CVertexBuffer *CParticleSystemData::instancingVertexBuffer()
{
	CHolder *holder = _model->_holder;
	if (_instancingVB == 0) {
		if (holder)
			holder->beginAccumulationBlock();
		_instancingDirty = false;
		CParticleSystem *system = lobj_cast<CParticleSystem>(_model);
		_instancingVB = CVertexBuffer::_alloc()->initWithDataSource(this, system->_holder, 0, 1);
		if (holder)
			holder->endAccumulationBlock();
		if (_instancingVB == 0)
			return 0;
	}
	if (_instancingDirty) {
		CParticleSystem *system = lobj_cast<CParticleSystem>(_model);
		if (system) {
			system->_fillParticlesModelInstanceVB(_instancingVB, this);
			_instancingDirty = false;
		}
	}
	return _instancingVB;
}

CVertexBuffer *CParticleSystemData::vertexBufferForSubModelAtIndex(unsigned int index)
{
	if (_vbs == 0) {
		CHolder *holder = _model->_holder;
		if (holder)
			holder->beginAccumulationBlock();
		_vbsDirty = true;
		CParticleSystem *system = lobj_cast<CParticleSystem>(_model);
		_vbs = LArray::_alloc()->init();
		int count = _subDatas ? _subDatas->count() : 0;
		for (int i = 0; i < count; i++) {
			CVertexBuffer *vb = CVertexBuffer::_alloc()->initWithDataSource(this, system->_holder, 0, 1);
			if (vb) {
				_vbs->addObject(vb);
				vb->release();
			}
		}
		if (holder)
			holder->endAccumulationBlock();
	}
	CVertexBuffer *vb = (CVertexBuffer *)_vbs->objectAtIndex(index);
	if (vb && _vbsDirty) {
		CParticleSystem *system = lobj_cast<CParticleSystem>(_model);
		if (system) {
			CParticleSubsystem *sub = (CParticleSubsystem *)system->_subsystems->objectAtIndex(index);
			system->_fillParticlesModelVB(vb, this, (char *)_particles + sub->_first * particleStructSize(),
			                              sub->_count);
			_vbsDirty = false;
		}
	}
	return vb;
}

// ---------------------------------------------------------------------------
// CParticleSystemDrawProxy

CParticleSystemDrawProxy *CParticleSystemDrawProxy::initWithPSystemData(CParticleSystemData *data)
{
	if (CDrawableModelData::initWithHostModel(data->_model) == 0)
		return 0;
	_systemData = data;
	LRetain(data);
	return this;
}

void CParticleSystemDrawProxy::dealloc()
{
	LRelease(_systemData);
	_systemData = 0;
	CDrawableModelData::dealloc();
}

// Draws the system data with this proxy's matrix.
void CParticleSystemDrawProxy::immediateDraw(CFrameDrawer *drawer)
{
	if (_systemData == 0) {
		CDrawableModelData::immediateDraw(drawer);
		return;
	}
	_copyAttributesToHost();
	_systemData->immediateDraw(drawer);
}

void CParticleSystemDrawProxy::_copyAttributesToHost()
{
	_systemData->_matrix = *worldMatrix();
}
