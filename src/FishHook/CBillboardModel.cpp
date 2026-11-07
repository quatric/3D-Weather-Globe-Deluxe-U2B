// CBillboardModel, CBillboardModelData and CBillboardSingleData.

#include "FHSprites.h"

LF_IMPLEMENT_CLASS(CBillboardModel, LF_CLASSOF(CBuffersOwner))
LF_IMPLEMENT_CLASS(CBillboardModelData, LF_CLASSOF(ModelDataWithMatrix))

enum { kBillboardsPerDraw = 0x200 };

// ---------------------------------------------------------------------------
// CBillboardSingleData

CBillboardSingleData::CBillboardSingleData()
    : _rect(LZeroRect), _inViewport(false), _color(0xffffffff), _rotation(0), _trapetion(0), _flip(false)
{
	_texScale.x = _texScale.y = 1;
	_texOffset.x = _texOffset.y = 0;
}

// ---------------------------------------------------------------------------
// CBillboardModelData

void CBillboardModelData::dealloc()
{
	delete _billboards;
	_billboards = 0;
	CDrawableModelData::dealloc();
}

void CBillboardModelData::removeAllBillboards()
{
	delete _billboards;
	_billboards = 0;
}

void CBillboardModelData::addBillboard(const CBillboardSingleData *billboard)
{
	if (_billboards == 0)
		_billboards = new TArray<CBillboardSingleData>;
	_billboards->addObject(*billboard);
}

CBillboardSingleData *CBillboardModelData::billboardAtIndex(int index, unsigned char create)
{
	if (create) {
		if (_billboards == 0)
			_billboards = new TArray<CBillboardSingleData>;
		while ((int)_billboards->count() <= index)
			_billboards->addObject(CBillboardSingleData());
	}
	if (_billboards == 0 || index < 0 || index >= (int)_billboards->count())
		return 0;
	return &(*_billboards)[index];
}

// ---------------------------------------------------------------------------
// CBillboardModel

CBillboardModel::CBillboardModel()
    : _vb(0), _ib(0), _frameSize(LZeroSize), _rendering(false), _count(-1), _locked(0), _vertexSize(0),
      _positionOffset(0), _texcoordOffset(0), _colorOffset(0)
{
}

CBillboardModel *CBillboardModel::initWithRenderProfile(CRenderProfile *profile)
{
	return initWithDataSource(this, profile, 0) ? this : 0;
}

void CBillboardModel::dealloc()
{
	if (_vb) {
		_vb->dataSourceWillDie();
		_vb->release();
		_vb = 0;
	}
	if (_ib) {
		_ib->dataSourceWillDie();
		_ib->release();
		_ib = 0;
	}
	CBuffersOwner::dealloc();
}

LClass *CBillboardModel::modelDataClass()
{
	return LF_CLASSOF(CBillboardModelData);
}

bool CBillboardModel::evalVisibilityParams(CDrawableModelData *, Vector3 *center, float *distance)
{
	if (distance)
		*distance = 1;
	if (center)
		center->x = center->y = center->z = 0;
	return true;
}

CShader *CBillboardModel::renderShaderForBuffer(CVertexBuffer *)
{
	CRenderProfile *profile = renderProfile();
	return profile ? profile->_shader : 0;
}

CShader *CBillboardModel::renderShaderForBuffer(CIndexBuffer *)
{
	CRenderProfile *profile = renderProfile();
	return profile ? profile->_shader : 0;
}

void CBillboardModel::fillBuffer(CIndexBuffer *ib)
{
	FHFillQuadIndices(ib, kBillboardsPerDraw);
}

CVertexBuffer *CBillboardModel::vertexBufferForBufOwner(int stream, CBuffersOwner *)
{
	if (stream != 0)
		return 0;
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

CIndexBuffer *CBillboardModel::indexBufferForBufOwner(CBuffersOwner *owner)
{
	D3DPRIMITIVETYPE type = primitiveTypeForBufOwner(owner);
	if (type != D3DPT_POINTLIST && _ib == 0) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_ib = CIndexBuffer::_alloc()->initWithDataSource(this, type, _holder, 0, 1);
		if (holder)
			holder->endAccumulationBlock();
	}
	return _ib;
}

void CBillboardModel::_startBillboardsRendering()
{
	if (_rendering)
		return;
	_count = -1;
	_rendering = true;
	_frameSize = _holder->virtualFrameSize();
	CRenderProfile *profile = renderProfile();
	_vertexSize = profile->vertexStructSize(0);
	_positionOffset = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	_texcoordOffset = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
	_colorOffset = profile->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
}

void CBillboardModel::_finishBillboardsRendering()
{
	if (!_rendering)
		return;
	if (_count >= 0 && _locked) {
		vertexBufferForBufOwner(0, this)->_vb->Unlock();
		_locked = 0;
		_context->_drawCache->drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, _count * 4, 0, _count * 2);
		_count = -1;
	}
	_rendering = false;
}

// Writes the four corners of a billboard, in clip space, to the locked
// buffer, drawing when it is full.
void CBillboardModel::_renderSingleBB(const CBillboardSingleData &bb)
{
	if (_count < 0) {
		vertexBufferForBufOwner(0, this)->_vb->Lock(0, 0, (void **)&_locked, D3DLOCK_DISCARD);
		_count = 0;
	}
	char *p = _locked + _count * 4 * _vertexSize;
	float x = bb._rect.origin.x;
	float y = bb._rect.origin.y;
	float w = bb._rect.size.width;
	float h = bb._rect.size.height;
	if (bb._inViewport) {
		y = ((1 - y) - h) * _frameSize.height * 0.5f;
		x = _frameSize.width * (x + 1) * 0.5f;
		w = w * _frameSize.width * 0.5f;
		h = h * _frameSize.height * 0.5f;
	}
	float s = (float)sin(bb._rotation);
	float c = (float)cos(bb._rotation);
	for (unsigned int i = 0; i < 4; i++) {
		float u = (float)(i & 1) - 0.5f;
		float v = (float)(i >> 1) - 0.5f;
		float hv = h * v;
		float px = c * w * u + x + 0 + w * 0.5f + s * hv;
		float *pos = (float *)(p + _positionOffset);
		pos[3] = 1;
		pos[2] = 0;
		pos[1] = ((((h * 0.5f + y + 0) - w * u * s) + hv * c) * -2) / _frameSize.height + 1;
		pos[0] = (px + px) / _frameSize.width - 1;
		float tu = bb._flip ? 1 - (u + 0.5f) : u + 0.5f;
		float *tex = (float *)(p + _texcoordOffset);
		tex[1] = (v + 0.5f) * bb._texScale.y + bb._texOffset.y;
		tex[0] = tu * bb._texScale.x + bb._texOffset.x;
		if (_colorOffset >= 0)
			*(unsigned int *)(p + _colorOffset) = bb._color;
		p += _vertexSize;
	}
	if (++_count >= kBillboardsPerDraw) {
		_count = -1;
		vertexBufferForBufOwner(0, this)->_vb->Unlock();
		_context->_drawCache->drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, kBillboardsPerDraw * 4, 0,
		                                           kBillboardsPerDraw * 2);
	}
}

void CBillboardModel::drawSubset(CDrawableModelData *data, long, long)
{
	if (data == 0 || !data->getClass()->isSubclassOf(LF_CLASSOF(CBillboardModelData)))
		return;
	CBillboardModelData *billboards = (CBillboardModelData *)data;
	_startBillboardsRendering();
	DeviceEmu *device = _context->_device;
	D3DTEXTUREADDRESS address = billboards->_wrapTexture ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, address);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, address);
	int count = billboards->billboardsCount();
	for (int i = 0; i < count; i++)
		_renderSingleBB((*billboards->_billboards)[i]);
	_finishBillboardsRendering();
}
