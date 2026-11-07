// CSpriteModel, CSpriteModelData, CGlowModelData and CLensFlareModel.

#include "FHSprites.h"

LF_IMPLEMENT_CLASS(CSpriteModel, LF_CLASSOF(CBuffersOwner))
LF_IMPLEMENT_CLASS(CSpriteModelData, LF_CLASSOF(CDrawableModelData))
LF_IMPLEMENT_CLASS(CGlowModelData, LF_CLASSOF(CSpriteModelData))
LF_IMPLEMENT_CLASS(CLensFlareModel, LF_CLASSOF(CSpriteModel))

// Writes the indices of `quads` quads, two triangles each.
static void _fillQuadIndices(CIndexBuffer *ib, unsigned int quads)
{
	StIndexBufferLocker *locker = ib->detachLocker(0);
	if (locker->_desc.Format == D3DFMT_INDEX32) {
		uint32_t *p = (uint32_t *)locker->data();
		for (unsigned int i = 0; i < quads * 4; i += 4, p += 6) {
			p[0] = i;
			p[1] = i + 3;
			p[2] = i + 1;
			p[3] = i;
			p[4] = i + 2;
			p[5] = i + 3;
		}
	} else {
		uint16_t *p = (uint16_t *)locker->data();
		for (unsigned int i = 0; i < quads; i++, p += 6) {
			uint16_t v = (uint16_t)(i * 4);
			p[0] = v;
			p[1] = v + 3;
			p[2] = v + 1;
			p[3] = v;
			p[4] = v + 2;
			p[5] = v + 3;
		}
	}
	delete locker;
}

void FHFillQuadIndices(CIndexBuffer *ib, unsigned int quads)
{
	_fillQuadIndices(ib, quads);
}

// ---------------------------------------------------------------------------
// CSpriteModel

CSpriteModel::CSpriteModel() : _spritesCount(1), _vb(0), _ib(0) {}

CSpriteModel *CSpriteModel::initWithParams(CRenderProfile *profile)
{
	return initWithDataSource(this, profile, 0) ? this : 0;
}

void CSpriteModel::dealloc()
{
	LRelease(_vb);
	_vb = 0;
	LRelease(_ib);
	_ib = 0;
	CBuffersOwner::dealloc();
}

LClass *CSpriteModel::modelDataClass()
{
	return LF_CLASSOF(CSpriteModelData);
}

bool CSpriteModel::evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance)
{
	CSpriteModelData *sprite = (CSpriteModelData *)data;
	return _context->spriteProbe(&sprite->_position, sprite->_size.x, sprite->_size.y, 0, center, distance);
}

void CSpriteModel::fillVertexBuffer(void *buffer, unsigned int stride, const Vector3 *c, const Vector2 *h,
                                    CSpriteModelData *)
{
	CRenderProfile *profile = renderProfile();
	int position = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	int texcoord = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
	char *p = (char *)buffer;
	for (unsigned int i = 0; i < _spritesCount; i++) {
		if (position >= 0) {
			float *v = (float *)(p + position);
			v[0] = c->x - h->x;
			v[1] = c->y - h->y;
			v[2] = c->z;
			v = (float *)(p + stride + position);
			v[0] = c->x + h->x;
			v[1] = c->y - h->y;
			v[2] = c->z;
			v = (float *)(p + stride * 2 + position);
			v[0] = c->x - h->x;
			v[1] = c->y + h->y;
			v[2] = c->z;
			v = (float *)(p + stride * 3 + position);
			v[0] = c->x + h->x;
			v[1] = c->y + h->y;
			v[2] = c->z;
		}
		if (texcoord >= 0) {
			float *t = (float *)(p + texcoord);
			t[0] = 0;
			t[1] = 0;
			t = (float *)(p + stride + texcoord);
			t[0] = 1;
			t[1] = 0;
			t = (float *)(p + stride * 2 + texcoord);
			t[0] = 0;
			t[1] = 1;
			t = (float *)(p + stride * 3 + texcoord);
			t[0] = 1;
			t[1] = 1;
		}
	}
}

// Fills the quad around the projected center, sized by the distance.
long CSpriteModel::beginDrawing(CDrawableModelData *data)
{
	long stages = CBuffersOwner::beginDrawing(data);
	if (stages) {
		CSpriteModelData *sprite = (CSpriteModelData *)data;
		Vector2 halfSize = sprite->_size * (0.5f * data->_distance);
		StVertexBufferLocker *locker = detachVBLocker(0, D3DLOCK_DISCARD);
		fillVertexBuffer(locker->data(), renderProfile()->vertexStructSize(0), &data->_center, &halfSize, sprite);
		delete locker;
	}
	return stages;
}

void CSpriteModel::fillBuffer(CIndexBuffer *ib)
{
	_fillQuadIndices(ib, _spritesCount);
}

CVertexBuffer *CSpriteModel::vertexBufferForBufOwner(int, CBuffersOwner *)
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

CIndexBuffer *CSpriteModel::indexBufferForBufOwner(CBuffersOwner *)
{
	if (_ib == 0) {
		CHolder *holder = _holder;
		if (holder)
			holder->beginAccumulationBlock();
		_ib = CIndexBuffer::_alloc()->initWithDataSource(this, D3DPT_TRIANGLELIST, _holder, 0, 1);
		if (holder)
			holder->endAccumulationBlock();
	}
	return _ib;
}

// ---------------------------------------------------------------------------
// CSpriteModelData

CSpriteModelData::CSpriteModelData()
{
	_position.x = _position.y = _position.z = 0;
	_size.x = _size.y = 1;
}

void CSpriteModelData::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	CDrawableModelData::storeSceneStateInStream(stream, flags);
	storeVectorInStream(&_position, stream);
	storeVectorInStream(&_size, stream);
}

const void *CSpriteModelData::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	data = CDrawableModelData::restoreSceneStateFromData(data, flags);
	data = loadVectorFromStream(data, &_position);
	return loadVectorFromStream(data, &_size);
}

// ---------------------------------------------------------------------------
// CGlowModelData

CGlowModelData::CGlowModelData() : _brightness(1)
{
	_color.r = _color.g = _color.b = _color.a = 1;
}

void CGlowModelData::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	CSpriteModelData::storeSceneStateInStream(stream, flags);
	storeColorInStream(&_color, stream);
	storeFloatInStream(_brightness, stream);
}

const void *CGlowModelData::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	data = CSpriteModelData::restoreSceneStateFromData(data, flags);
	data = loadColorFromStream(data, &_color);
	return loadFloatFromStream(data, &_brightness);
}

// ---------------------------------------------------------------------------
// CLensFlareModel

static const float kFlareScales[9] = {0.6f, 0.55f, 0.4f, 0.2f, 0.1f, 0.25f, 1.0f, 1.0f, 1.5f};
static const float kFlarePositions[9] = {1.6f, 1.0f, 1.4f, 0.3f, 0.1f, -0.2f, -0.4f, -0.7f, -0.9f};
static const float kFlareColors[9][4] = {
    {0.4f, 0.3f, 0.1f, 1.0f}, {0.3f, 0.4f, 0.5f, 1.0f},  {0.2f, 0.3f, 0.05f, 1.0f},
    {0.1f, 0.2f, 0.4f, 1.0f}, {0.2f, 0.15f, 0.05f, 1.0f}, {0.1f, 0.2f, 0.6f, 1.0f},
    {0.3f, 0.3f, 0.1f, 1.0f}, {0.2f, 0.4f, 0.1f, 1.0f},  {0.1f, 0.2f, 0.5f, 1.0f},
};

CLensFlareModel::CLensFlareModel()
{
	_spritesCount = 9;
}

LClass *CLensFlareModel::modelDataClass()
{
	return LF_CLASSOF(CGlowModelData);
}

// Visible while the glow shows and its center is within the depth range.
bool CLensFlareModel::evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance)
{
	Vector3 local;
	if (center == 0)
		center = &local;
	CSpriteModel::evalVisibilityParams(data, center, distance);
	return ((CGlowModelData *)data)->currentGlowBrightness() > 0.01f && center->z >= -1 && center->z <= 1;
}

static unsigned int _colorByte(float f)
{
	return f <= 0 ? 0 : (unsigned int)(int64_t)f;
}

void CLensFlareModel::fillVertexBuffer(void *buffer, unsigned int stride, const Vector3 *center, const Vector2 *,
                                       CSpriteModelData *data)
{
	CRenderTarget *target = _context->_currentRenderTarget;
	int cols, rows, col, row;
	target->getOutSegmentsCount(&cols, &rows);
	target->getOutSegment(&col, &row);
	LSize full = _holder->fullFrameSize();
	LSize bb = _holder->backBufferSize();
	float py = (center->y * 0.5f + 0.5f) * bb.height + (float)((rows - row) - 1) * bb.height;
	py = (py + py) / full.height - 1;
	float sy = full.height / bb.height;
	float px = (center->x * 0.5f + 0.5f) * bb.width + (float)col * bb.width;
	px = (px + px) / full.width - 1;
	float sx = full.width / bb.width;
	LSize virtualSize = _holder->virtualFrameSize();
	int colorOffset = renderProfile()->drawableElementOffset(D3DDECLUSAGE_COLOR, 0, 0);
	unsigned int *color = colorOffset >= 0 ? (unsigned int *)((char *)buffer + colorOffset) : 0;
	int texOffset = renderProfile()->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
	float *tex = texOffset >= 0 ? (float *)((char *)buffer + texOffset) : 0;
	CGlowModelData *glow = (CGlowModelData *)data;
	float brightness = glow->currentGlowBrightness();
	float fade = (float)(1.0 / pow(2.0, (double)((px * px + py * py) * 0.4f))) * brightness;
	char *p = (char *)buffer;
	for (int i = 0; i < 9; i++) {
		float y = py * kFlarePositions[i] * sy + (float)((row * 2 - rows) + 1);
		float x = px * kFlarePositions[i] * sx + (float)(~(col * 2) + cols);
		Vector2 h = glow->_size * kFlareScales[i];
		h.y = sy * h.y;
		h.x = (virtualSize.height / virtualSize.width) * sx * h.x;
		float tu = (float)(i % 3) * 0.33333334f;
		float tv = (float)(i / 3) * 0.33333334f;
		unsigned int r = _colorByte(fade * glow->_color.r * kFlareColors[i][0] * 255.0f);
		unsigned int g = _colorByte(fade * glow->_color.g * kFlareColors[i][1] * 255.0f);
		unsigned int b = _colorByte(fade * glow->_color.b * kFlareColors[i][2] * 255.0f);
		unsigned int a = _colorByte(fade * glow->_color.a * kFlareColors[i][3] * 255.0f);
		for (int k = 0; k < 4; k++) {
			float *v = (float *)p;
			v[0] = k & 1 ? x + h.x : x - h.x;
			v[1] = k & 2 ? y + h.y : y - h.y;
			v[2] = 0.1f;
			if (color)
				*color = (r & 0xff) | (g & 0xff) << 8 | (b & 0xff) << 16 | a << 24;
			if (tex) {
				tex[0] = k & 1 ? tu + 0.33333334f : tu;
				tex[1] = k & 2 ? tv + 0.33333334f : tv;
			}
			p += stride;
			if (color)
				color = (unsigned int *)((char *)color + stride);
			if (tex)
				tex = (float *)((char *)tex + stride);
		}
	}
}
