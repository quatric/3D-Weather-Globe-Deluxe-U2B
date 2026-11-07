// AdaptiveSphereDrawable: the sphere mesh the globe's coarse patches share.
// Each patch's triangles are together in the index buffer, so a patch is
// drawn on its own with its textures.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(AdaptiveSphereDrawable, LF_CLASSOF(CMesh))

void AdaptiveSphereDrawable::dealloc()
{
	CMesh::dealloc();
}

LClass *AdaptiveSphereDrawable::modelDataClass()
{
	return AdaptiveSphereDrawableData::classObject();
}

// Sources 0 and 1 (the patch's texture and city lights) are set per patch.
bool AdaptiveSphereDrawable::_getTextureForSource(BaseTextureEmu **texture, int source)
{
	if ((unsigned int)source < 2) {
		*texture = 0;
		return true;
	}
	return CMesh::_getTextureForSource(texture, source);
}

const void *AdaptiveSphereDrawable::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id == 'TTrn' || descr->id == 'TTro')
		return buffer;
	return CMesh::_dataPtrForConstant(descr, buffer);
}

bool AdaptiveSphereDrawable::activateStage(long stage)
{
	if (stage != 0)
		_context->setWorldTransform(sIdentityMatrixPtr);
	return CBuffersOwner::activateStage(stage);
}

void AdaptiveSphereDrawable::meshWasInstantiated(CMesh *mesh)
{
	if (mesh->_context == 0)
		LF_RAISE(WrongConditionException());
	CRenderProfile *profile = mesh->renderProfile();
	if (profile->_context == 0)
		LF_RAISE(WrongConditionException());
	StVertexBufferLocker *vertices = mesh->detachVBLocker(0, 0);
	if (numVerticesForMesh(mesh) != 5995)
		LF_RAISE(WrongConditionException());
	char *p = (char *)vertices->data();
	unsigned int stride = profile->vertexStructSize(0);
	int pos = profile->drawableElementOffset(D3DDECLUSAGE_POSITION, 0, 0);
	int normal = profile->drawableElementOffset(D3DDECLUSAGE_NORMAL, 0, 0);
	int uv = profile->drawableElementOffset(D3DDECLUSAGE_TEXCOORD, 0, 0);
	for (unsigned int row = 0; row < 55; row++) {
		float v = (float)row / 54.0f;
		float lat = (v - 0.5f) * 3.1415927f;
		float cosLat = cosf(lat), sinLat = sinf(lat);
		for (unsigned int column = 0; column < 109; column++, p += stride) {
			float u = (float)column / 108.0f;
			float lon = u * 6.2831855f;
			float c = cosf(lon), s = sinf(lon);
			if (pos >= 0) {
				float *f = (float *)(p + pos);
				f[2] = sinLat;
				f[1] = cosLat * s;
				f[0] = cosLat * c;
			}
			if (normal >= 0) {
				float *f = (float *)(p + normal);
				f[2] = sinLat;
				f[1] = s * cosLat;
				f[0] = c * cosLat;
			}
			if (uv >= 0) {
				float *f = (float *)(p + uv);
				f[0] = u;
				f[1] = v;
			}
		}
	}
	delete vertices;

	StIndexBufferLocker *indices = mesh->detachIBLocker(0);
	bool wide = indices->_desc.Format == D3DFMT_INDEX32;
	if (indices->_desc.Size / (wide ? 4 : 2) / 3 != 11664)
		LF_RAISE(WrongConditionException());
	// The two hemispheres, each half of the triangles.
	if (wide) {
		_fillIndexBuffersForAdaptiveSphere((uint32_t *)indices->data(), 0, 0, 0);
		_fillIndexBuffersForAdaptiveSphere((uint32_t *)indices->data() + 17496, 1, 0, 0);
	} else {
		_fillIndexBuffersForAdaptiveSphere((uint16_t *)indices->data(), 0, 0, 0);
		_fillIndexBuffersForAdaptiveSphere((uint16_t *)indices->data() + 17496, 1, 0, 0);
	}
	delete indices;
}

// The patch (x, y) at `level`: its children in order, down to the grid's
// quads at level 4.
template <typename T>
void AdaptiveSphereDrawable::_fillIndexBuffersForAdaptiveSphere(T *indices, int x, int y, int level)
{
	if (level >= 4) {
		T v = (T)(y * 109 + x);
		indices[0] = v;
		indices[1] = (T)(v + 1);
		indices[2] = (T)(v + 110);
		indices[3] = v;
		indices[4] = (T)(v + 110);
		indices[5] = (T)(v + 109);
		return;
	}
	int n = 0, size = 0;	// children per side, indices per child
	switch (level) {
	case 0:
		n = 3;
		size = 1944;
		break;
	case 1:
		n = 3;
		size = 216;
		break;
	case 2:
		n = 2;
		size = 54;
		break;
	case 3:
		n = 3;
		size = 6;
		break;
	}
	for (int j = 0; j < n; j++)
		for (int i = 0; i < n; i++)
			_fillIndexBuffersForAdaptiveSphere(indices + (i + n * j) * size, n * x + i, n * y + j, level + 1);
}

template void AdaptiveSphereDrawable::_fillIndexBuffersForAdaptiveSphere<uint16_t>(uint16_t *, int, int, int);
template void AdaptiveSphereDrawable::_fillIndexBuffersForAdaptiveSphere<uint32_t>(uint32_t *, int, int, int);

void AdaptiveSphereDrawable::drawSubset(CDrawableModelData *modelData, long, long)
{
	AdaptiveSphereDrawableData *data = lobj_cast<AdaptiveSphereDrawableData>(modelData);
	if (data == 0)
		return;
	CShaderStage *stage = _shader->_currentStage;
	_textureStage = stage->_textureSources ? stage->_textureSources->indexOfObject(0) : -1;
	_glowsStage = stage->_textureSources ? stage->_textureSources->indexOfObject(1) : -1;
	_textureConst = stage->offsetForConstant('TTrn');
	_glowsConst = stage->offsetForConstant('TTro');
	CDrawCache *cache = _context->_drawCache;
	Color factor(1.0f, 1.0f, 1.0f, data->_alpha);
	cache->setRenderState(D3DRS_TEXTUREFACTOR, (unsigned int)factor);
	CTextureLoader *loader = (CTextureLoader *)_holder->objectForKey(kTextureLoaderKey);
	int count = data->_patches.count();
	for (int i = 0; i < count; i++) {
		AdaptiveLODSphere::SPatchDrawInfo &patch = data->_patches[i];
		cache->setTexture(_textureStage, loader->textureAtLevel(patch.texture.textureID, 0));
		if (_glowsStage >= 0)
			cache->setTexture(_glowsStage, loader->nightGlowsTextureAtLevel(patch.glows.textureID));
		Vector4 v;
		if (_textureConst >= 0) {
			patch.texture.fillVSScaleOffsConstant(&v);
			cache->setVSConstant(_textureConst, &v.x, 1);
		}
		if (_glowsConst >= 0) {
			patch.glows.fillVSScaleOffsConstant(&v);
			cache->setVSConstant(_glowsConst, &v.x, 1);
		}
		int triangles, offset;
		SphereLOD::getPatchIBParameters(patch.id, &triangles, &offset);
		cache->drawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 5995, offset * 6, triangles * 2);
	}
}
