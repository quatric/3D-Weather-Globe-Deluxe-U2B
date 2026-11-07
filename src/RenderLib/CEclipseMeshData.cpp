// The moon's shadow on the Earth (stage 1) and the water's specular spot
// (stage 2): a plane facing the light, with its axes for the shader.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CEclipseMeshData, LF_CLASSOF(ModelDataWithMatrix))

// The Earth radius in kilometers.
static const float kEarthRadiusKm = 6371.1f;

CEclipseMeshData::CEclipseMeshData() : _shadowStage(false), _specularStage(false), _visibility(1.0f), _inArea(false)
{
}

bool CEclipseMeshData::_isStageOn(CShaderStage *stage, unsigned char *on)
{
	if (stage->_objectID == 1)
		*on = _shadowStage;
	else if (stage->_objectID == 2)
		*on = _specularStage;
	else
		return CShaderStageStateProvider::_isStageOn(stage, on);
	return true;
}

void CEclipseMeshData::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	ModelDataWithMatrix::storeSceneStateInStream(stream, flags);
	storeVectorInStream(&_sunDirection, stream);
	storeVectorInStream(&_offset, stream);
	storeVectorInStream(&_txt, stream);
	storeVectorInStream(&_worldX, stream);
	storeVectorInStream(&_worldY, stream);
	storeIntInStream(_inArea, stream);
	storeIntInStream(_shadowStage, stream);
	storeIntInStream(_specularStage, stream);
	storeFloatInStream(_visibility, stream);
}

const void *CEclipseMeshData::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	data = ModelDataWithMatrix::restoreSceneStateFromData(data, flags);
	data = loadVectorFromStream(data, &_sunDirection);
	data = loadVectorFromStream(data, &_offset);
	data = loadVectorFromStream(data, &_txt);
	data = loadVectorFromStream(data, &_worldX);
	data = loadVectorFromStream(data, &_worldY);
	int i;
	data = loadIntFromStream(data, &i);
	_inArea = (unsigned char)i;
	data = loadIntFromStream(data, &i);
	_shadowStage = (unsigned char)i;
	data = loadIntFromStream(data, &i);
	_specularStage = (unsigned char)i;
	return loadFloatFromStream(data, &_visibility);
}

const void *CEclipseMeshData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	float *v = (float *)buffer;
	const Vector3 *vector;
	switch (descr->id) {
	case 'ETxM':
		v[0] = 2.0f;
		v[1] = 2.0f;
		v[2] = _txt.x - 0.5f;
		v[3] = _txt.y - 0.5f;
		return buffer;
	case 'ETxN':
		vector = &_sunDirection;
		break;
	case 'ETxO':
		vector = &_offset;
		break;
	case 'ETxP':
		vector = &_worldX;
		break;
	case 'ETxQ':
		vector = &_worldY;
		break;
	case 'ETxR':
		v[0] = _visibility;
		v[1] = v[2] = v[3] = 0.0f;
		return buffer;
	default:
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	}
	v[0] = vector->x;
	v[1] = vector->y;
	v[2] = vector->z;
	v[3] = 0.0f;
	return buffer;
}

void CEclipseMeshData::_setupRotation(const _SVector3 *v)
{
	Vector3 axis(0.0f, 1.0f, 0.0f);
	Vector3 y(0.0f, 1.0f, 0.0f);
	float c = v->x * 0.0f + v->z * 0.0f + v->y;
	if (c < 0.9999f) {
		FHVec3Cross(&axis, &y, v);
		FHVec3Normalize(&axis, &axis);
	}
	FHMatrixRotationAxis(&_matrix, &axis, acosf(c));
}

void CEclipseMeshData::adjustToLightSpecular(CContextLight *light, const Vector3 *view)
{
	Vector3 z(0.0f, 0.0f, 1.0f);
	Vector3 l;
	FHVec3Normalize(&l, view);
	l += Vector3(light->_params.Direction.x, light->_params.Direction.y, light->_params.Direction.z);
	FHVec3Normalize(&l, &l);
	_setupRotation(&l);
	_inArea = true;
	FHVec3Cross(&_worldY, &z, &l);
	FHVec3Normalize(&_worldY, &_worldY);
	FHVec3Cross(&_worldX, &_worldY, &l);
	FHVec3Normalize(&_worldX, &_worldX);
	Matrix inverse;
	FHMatrixInverse(&inverse, 0, worldMatrix());
	Vector4 t;
	FHVec3Transform(&t, &_worldX, &inverse);
	_worldX = Vector3(t.x, t.y, t.z);
	FHVec3Transform(&t, &_worldY, &inverse);
	_worldY = Vector3(t.x, t.y, t.z);
	_shadowStage = false;
	_specularStage = true;
}

void CEclipseMeshData::adjustToSunMoonParams(const Vector3 *sun, const Vector3 *moon)
{
	_sunDirection = -*sun;
	_setupRotation(&_sunDirection);
	float d = _sunDirection.dot(*moon);
	_offset = _sunDirection * d - *moon;
	_offset /= kEarthRadiusKm;
	float r2 = _offset.lengthSq();
	_inArea = r2 < 4.0f && d > 0.0f;
	Matrix inverse;
	FHMatrixInverse(&inverse, 0, &_matrix);
	Vector4 t;
	FHVec3Transform(&t, &_offset, &inverse);
	_txt = Vector2(t.x, t.z);
	_worldY = _offset;
	FHVec3Normalize(&_worldY, &_worldY);
	FHVec3Cross(&_worldX, &_worldY, &_sunDirection);
	FHVec3Normalize(&_worldX, &_worldX);
	_shadowStage = true;
	_specularStage = false;
}
