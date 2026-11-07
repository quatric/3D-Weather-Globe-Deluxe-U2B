// The lightning's sprite.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(LightingMesh, LF_CLASSOF(CMesh))
LF_IMPLEMENT_CLASS(LightingSpriteData, LF_CLASSOF(ModelDataWithMatrix))

LClass *LightingMesh::modelDataClass()
{
	return LF_CLASSOF(LightingSpriteData);
}

bool LightingMesh::evalVisibilityParams(CDrawableModelData *data, Vector3 *center, float *distance)
{
	return CMesh::evalVisibilityParams(data, center, distance) && ((LightingSpriteData *)data)->_color.a >= 0.01f;
}

LightingSpriteData::LightingSpriteData() : _frame(0), _framesCount(1), _color(1.0f, 1.0f, 1.0f, 0.0f), _large(false)
{
}

void LightingSpriteData::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	ModelDataWithMatrix::storeSceneStateInStream(stream, flags);
	storeIntInStream(_frame, stream);
	storeIntInStream(_framesCount, stream);
	storeColorInStream(&_color, stream);
	storeIntInStream(_large, stream);
}

const void *LightingSpriteData::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	data = ModelDataWithMatrix::restoreSceneStateFromData(data, flags);
	data = loadIntFromStream(data, (int *)&_frame);
	data = loadIntFromStream(data, (int *)&_framesCount);
	data = loadColorFromStream(data, &_color);
	int large;
	data = loadIntFromStream(data, &large);
	_large = large != 0;
	return data;
}

const void *LightingSpriteData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id == 'LgDC')
		return &_color;
	if (descr->id != 'LgIn')
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	// The frame's offset and size in the texture: in columns, or in rows when large.
	float *v = (float *)buffer;
	if (!_large) {
		v[0] = (float)_frame;
		v[1] = 1.0f / (float)_framesCount;
		v[2] = 0.0f;
		v[3] = 1.0f;
	} else {
		v[0] = 0.0f;
		v[1] = 1.0f;
		v[2] = (float)_frame;
		v[3] = 1.0f / (float)_framesCount;
	}
	return buffer;
}
