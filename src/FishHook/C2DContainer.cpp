// C2DContainer and its drawable data.

#include "FH2DContainer.h"

#include <memory>

LF_IMPLEMENT_CLASS(C2DContainer, LF_CLASSOF(CBuffersOwner))
LF_IMPLEMENT_CLASS(C2DContainerDrawableData, LF_CLASSOF(ModelDataWithMatrix))

// ---------------------------------------------------------------------------
// C2DContainer::StPatch

C2DContainer::StPatch::StPatch(C2DContainer *container, D3DPRIMITIVETYPE type) : _container(container)
{
	if (container->_patches == 0)
		container->_patches = new TArray<SPatchInfo>;
	if (container->_data == 0)
		container->_data = LData::_alloc()->init();
	SPatchInfo info = {container->_data->length() >> 4, type, 0};
	container->_patches->addObject(info);
	_patch = &(*container->_patches)[container->_patches->count() - 1];
}

void C2DContainer::StPatch::addLine(const SPoint *a, const SPoint *b)
{
	if ((unsigned int)_patch->type - D3DPT_LINELIST < 2) {
		_container->_data->appendBytes(a, sizeof(SPoint));
		_container->_data->appendBytes(b, sizeof(SPoint));
		_patch->count++;
	}
}

void C2DContainer::StPatch::lineTo(const SPoint *p)
{
	if (_patch->type == D3DPT_LINESTRIP) {
		_container->_data->appendBytes(p, sizeof(SPoint));
		_patch->count++;
	}
}

void C2DContainer::StPatch::addTriangle(const SPoint *a, const SPoint *b, const SPoint *c)
{
	if ((unsigned int)_patch->type - D3DPT_TRIANGLELIST < 3) {
		_container->_data->appendBytes(a, sizeof(SPoint));
		_container->_data->appendBytes(b, sizeof(SPoint));
		_container->_data->appendBytes(c, sizeof(SPoint));
		_patch->count++;
	}
}

void C2DContainer::StPatch::triangleTo(const SPoint *p)
{
	if ((unsigned int)_patch->type - D3DPT_TRIANGLESTRIP < 2) {
		_container->_data->appendBytes(p, sizeof(SPoint));
		_patch->count++;
	}
}

// ---------------------------------------------------------------------------
// C2DContainer

C2DContainer::C2DContainer() : _patches(0), _data(0), _vb(0) {}

C2DContainer *C2DContainer::initWithHolder(CHolder *holder)
{
	CRenderProfile *profile = CRenderProfile::renderProfileByFileDesc(&PrimitivesProfile, holder);
	if (profile == 0) {
		dealloc();
		return 0;
	}
	if (CBuffersOwner::initWithDataSource(this, profile, false) == 0)
		return 0;
	return this;
}

void C2DContainer::dealloc()
{
	if (_vb) {
		_vb->dataSourceWillDie();
		LRelease(_vb);
		_vb = 0;
	}
	LRelease(_data);
	_data = 0;
	delete _patches;
	_patches = 0;
	CBuffersOwner::dealloc();
}

LClass *C2DContainer::modelDataClass()
{
	return LF_CLASSOF(C2DContainerDrawableData);
}

unsigned int C2DContainer::vertCountForBuffer(CVertexBuffer *)
{
	return _data ? _data->length() >> 4 : 0;
}

// Each patch as its own draw call.
void C2DContainer::drawSubset(CDrawableModelData *, long, long)
{
	CDrawCache *cache = _context->_drawCache;
	int count = _patches ? _patches->count() : 0;
	for (int i = 0; i < count; i++) {
		const SPatchInfo &info = (*_patches)[i];
		cache->drawPrimitive(info.type, info.start, info.count);
	}
}

void C2DContainer::fillBuffer(CVertexBuffer *vb)
{
	std::unique_ptr<StVertexBufferLocker> locker(vb->detachLocker(0));
	CRenderProfile *profile = renderProfile();
	if ((int)profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0) < 0)
		return;
	profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
	unsigned int stride = profile->vertexStructSize(streamNumberForBuffer(vb));
	memcpy(locker->data(), _data->bytes(), vertCountForBuffer(vb) * stride);
}

CVertexBuffer *C2DContainer::vertexBufferForBufOwner(int, CBuffersOwner *)
{
	if (_vb == 0) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_vb = CVertexBuffer::_alloc()->initWithDataSource(this, _holder, 0, 1);
		if (holder)
			holder->endAccumulationBlock();
	}
	return _vb;
}

// ---------------------------------------------------------------------------
// C2DContainerDrawableData

const void *C2DContainerDrawableData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	SShaderConstDescr d = *descr;
	if (!_flag && d.id == 5)
		d.id = 3;
	return CDrawableModelData::_dataPtrForConstant(&d, buffer);
}

bool C2DContainerDrawableData::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	return CShaderStageStateProvider::_isStageOn(stage, on);
}
