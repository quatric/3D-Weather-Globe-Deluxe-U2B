// A sphere patch's adaptive mesh data: its trapezoid, its textures and the
// world matrix (z scaled by 5, or by the vertical scale, on the ground).

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(GlobeAdaptiveMeshData, LF_CLASSOF(ModelDataWithMatrix))

GlobeAdaptiveMeshData::GlobeAdaptiveMeshData() : _drawInfo(STextureID(), 1), _fogClip(0.0f), _zOffset(1.0f)
{
	_id.x = 0;
	_id.y = 0;
	_id.level = 0;
}

void GlobeAdaptiveMeshData::dealloc()
{
	CDrawableModelData::dealloc();
}

void GlobeAdaptiveMeshData::transformBoundingBox(Vector3 *min, Vector3 *max, CDrawableModelData *data)
{
	const Vector2 &t = ((GlobeAdaptiveMeshData *)data)->_trapezoid;
	float x0 = min->x;
	float x1 = max->x;
	float minX, maxX;
	if (!(x0 < 0.5f)) {
		minX = x0 * t.y + t.x * max->y * (x0 - 0.5f);
		maxX = t.y * x1 + t.x * min->y * (x1 - 0.5f);
	} else {
		minX = x0 * t.y + t.x * min->y * (x0 - 0.5f);
		maxX = t.y * x1 + t.x * max->y * (x1 - 0.5f);
	}
	min->x = minX;
	max->x = maxX + maxX * max->z;
}

// The diagonal (1, 1, zScale) moved up by _zOffset, then the patch's place.
static void setPatchMatrix(GlobeAdaptiveMeshData *data, float zScale)
{
	Matrix m;
	memset(&m, 0, sizeof(m));
	m.m[0][0] = 1.0f;
	m.m[1][1] = 1.0f;
	m.m[2][2] = zScale;
	m.m[3][2] = data->_zOffset;
	m.m[3][3] = 1.0f;
	const Matrix *patch = 0;
	if (data->_id.level == 5)
		patch = &SphereLOD::sMaxLevelMatrices[data->_id.x + data->_id.y * SphereLOD::sItemsCountForLOD[5] * 2];
	FHMatrixMultiply(&data->_matrix, &m, patch);
}

void GlobeAdaptiveMeshData::setVerticalScaleFactor(float factor)
{
	setPatchMatrix(this, factor);
}

void GlobeAdaptiveMeshData::setTransforms(const STextureID &id, float zOffset)
{
	_id = id;
	_drawInfo.id = id;
	SphereLOD::entryTrapetionParams(_id, &_trapezoid);
	_zOffset = zOffset;
	setPatchMatrix(this, 5.0f);
}

bool GlobeAdaptiveMeshData::_getTextureForSource(BaseTextureEmu **texture, int source)
{
	CTextureLoader *loader = (CTextureLoader *)_model->_holder->objectForKey(kTextureLoaderKey);
	if (source == 0)
		*texture = loader->textureAtLevel(_drawInfo.texture.textureID, 0);
	else if (source == 1)
		*texture = loader->nightGlowsTextureAtLevel(_drawInfo.glows.textureID);
	else
		return CDrawableModelData::_getTextureForSource(texture, source);
	return true;
}

const void *GlobeAdaptiveMeshData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	float *v = (float *)buffer;
	switch (descr->id) {
	case 'GETr':
		v[0] = _trapezoid.x;
		v[1] = _trapezoid.y;
		v[2] = 1.0f;
		v[3] = 0.0f;
		return buffer;
	case 'FPGl':
		if (_model->_context)
			UMathUtils::fillExp2FogParamsConstant(v, _model->_context->worldViewMatrixTransp(), _fogClip * 0.4f,
			                                      _fogClip, 2.1f);
		return buffer;
	case 'TTrn':
		_drawInfo.texture.fillVSScaleOffsConstant((Vector4 *)buffer);
		return buffer;
	case 'TTro':
		_drawInfo.glows.fillVSScaleOffsConstant((Vector4 *)buffer);
		return buffer;
	}
	return CDrawableModelData::_dataPtrForConstant(descr, buffer);
}
