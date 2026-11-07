// A planet's place, size, texture and color.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(PlanetDrawableData, LF_CLASSOF(ModelDataWithMatrix))

PlanetDrawableData::PlanetDrawableData()
    : _glows(0), _radius(1.0f), _diffuse(0), _diffuseColor(1.0f, 1.0f, 1.0f, 1.0f), _lighting(true),
      _blendMode(kBlendOpaque)
{
}

void PlanetDrawableData::dealloc()
{
	releaseAndClear(_diffuse);
	releaseAndClear(_glows);
	CDrawableModelData::dealloc();
}

void PlanetDrawableData::setDiffuseTexture(CBaseTexture *texture)
{
	LRetain(texture);
	releaseAndClear(_diffuse);
	_diffuse = texture;
}

const void *PlanetDrawableData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != 'PlCo')
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	return &_diffuseColor;
}

bool PlanetDrawableData::_getTextureForSource(BaseTextureEmu **texture, int source)
{
	if (source != 0)
		return false;
	*texture = _diffuse ? _diffuse->provideTexture() : 0;
	return true;
}

bool PlanetDrawableData::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	bool textured = _diffuse != 0;
	switch (stage->_objectID) {
	case 1:
		*on = !_lighting && textured && _blendMode == kBlendOpaque;
		break;
	case 2:
		*on = _lighting && textured && _blendMode == kBlendOpaque;
		break;
	case 3:
		*on = !_lighting && !textured && _blendMode == kBlendOpaque;
		break;
	case 4:
		*on = _lighting && !textured && _blendMode == kBlendOpaque;
		break;
	case 5:
		*on = !_lighting && _blendMode == kBlendTransparent;
		break;
	case 6:
		*on = _lighting && _blendMode == kBlendTransparent;
		break;
	default:
		return false;
	}
	return true;
}

void PlanetDrawableData::adjustMatrixWithCamera(const Quater &rotation, const Vector3 &position, const Vector3 &camera,
                                                float maxDistance)
{
	Vector3 d = position - camera;
	float distance = d.length();
	float k = maxDistance <= distance ? maxDistance / distance : 1.0f;
	Vector3 p = d * k + camera;
	FHMatrixRotationQuaternion(&_matrix, &rotation);
	k *= _radius;
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			_matrix.m[i][j] *= k;
	_matrix._41 = p.x;
	_matrix._42 = p.y;
	_matrix._43 = p.z;
	// The original walks the glows here and does nothing with them.
}

void PlanetDrawableData::appendGlowModelData(GlowDrawableData *glow)
{
	if (_glows == 0)
		_glows = LArray::_alloc()->init();
	_glows->addObject(glow);
}
