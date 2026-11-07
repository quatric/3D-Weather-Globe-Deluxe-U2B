// The data of a glow: its colors, cone and texture rotations.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(GlowDrawableData, LF_CLASSOF(ModelDataWithMatrix))

GlowDrawableData::GlowDrawableData()
    : _blendMode(kBlendAlpha), _darkColor(0.03f, 0.03f, 0.03f, 0.03f), _lightColor(1.0f, 1.0f, 1.0f, 1.0f),
      _visibility(1.0f), _texRotation(1.0f, 0.0f, 1.0f, 0.0f), _prolong(0.0f)
{
}

void GlowDrawableData::setTextureRotation(float turns, int index)
{
	if (index != 0 && index != 1)
		return;
	float *v = &_texRotation.x + index * 2;
	v[0] = cosf(turns * 6.2831855f);
	v[1] = sinf(turns * -6.2831855f);
}

bool GlowDrawableData::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	int id = stage->_objectID;
	if (id < 1 || id > 6)
		return CShaderStageStateProvider::_isStageOn(stage, on);
	*on = _blendMode == id - 1;
	return true;
}

void GlowDrawableData::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	ModelDataWithMatrix::storeSceneStateInStream(stream, flags);
	storeMatrixInStream(&_coneRotation, stream);
	storeVectorInStream(&_conePremul, stream);
	storeColorInStream(&_darkColor, stream);
	storeColorInStream(&_lightColor, stream);
	storeFloatInStream(_visibility, stream);
	storeVectorInStream(&_texRotation, stream);
}

const void *GlowDrawableData::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	data = ModelDataWithMatrix::restoreSceneStateFromData(data, flags);
	data = loadMatrixFromStream(data, &_coneRotation);
	data = loadVectorFromStream(data, &_conePremul);
	data = loadColorFromStream(data, &_darkColor);
	data = loadColorFromStream(data, &_lightColor);
	data = loadFloatFromStream(data, &_visibility);
	return loadVectorFromStream(data, &_texRotation);
}

const void *GlowDrawableData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	float *v = (float *)buffer;
	switch (descr->id) {
	case 'CnRt':
		return &_coneRotation;
	case 'CnRu':
		return &_conePremul;
	case 'CnRv':
	case 'CnRw': {
		const Color &c = descr->id == 'CnRv' ? _darkColor : _lightColor;
		v[0] = c.r;
		v[1] = c.g;
		v[2] = c.b;
		v[3] = c.a * _visibility;
		return buffer;
	}
	case 'CnRx':
		v[0] = _prolong;
		v[1] = v[2] = v[3] = 0.0f;
		return buffer;
	// TextureMatrix0Row0..1, TextureMatrix1Row0..1: a turn about the
	// texture's center.
	case 39:
	case 42: {
		float c = descr->id == 39 ? _texRotation.x : _texRotation.z;
		float s = descr->id == 39 ? _texRotation.y : _texRotation.w;
		v[0] = c;
		v[1] = s;
		v[2] = ((1.0f - c) - s) * 0.5f;
		v[3] = 0.0f;
		return buffer;
	}
	case 40:
	case 43: {
		float c = descr->id == 40 ? _texRotation.x : _texRotation.z;
		float s = descr->id == 40 ? _texRotation.y : _texRotation.w;
		v[0] = -s;
		v[1] = c;
		v[2] = ((s + 1.0f) - c) * 0.5f;
		v[3] = 0.0f;
		return buffer;
	}
	}
	return CDrawableModelData::_dataPtrForConstant(descr, buffer);
}

void GlowDrawableData::adjustParameters(const Vector3 *position, float radius, const CContextCamera *camera)
{
	float farClip = camera->_far;
	Vector3 cameraPosition = *camera->position();
	Vector3 d = *position - cameraPosition;
	float distance = d.length();
	Vector3 direction = -d / distance;
	float k = (farClip * 0.95f) / distance;
	if (k < 1.0f) {
		d *= k;
		radius *= k;
		distance *= k;
	}
	float s = radius / distance;
	float c = sqrtf(1.0f - s * s);
	_conePremul = Vector4(c, c, s, 0.0f);
	Vector3 z(0.0f, 0.0f, 1.0f);
	Matrix rotation;
	UMathUtils::matrixRotationVectors(&rotation, &z, &direction);
	FHMatrixTranspose(&_coneRotation, &rotation);
	_matrix.setIdentity();
	_matrix._11 = _matrix._22 = _matrix._33 = radius;
	_matrix._41 = d.x + cameraPosition.x;
	_matrix._42 = d.y + cameraPosition.y;
	_matrix._43 = d.z + cameraPosition.z;
}
