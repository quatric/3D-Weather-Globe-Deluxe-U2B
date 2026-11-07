// The horizon strip's visibility and color.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(HorizontDrawableData, LF_CLASSOF(ModelDataWithMatrix))

HorizontDrawableData::HorizontDrawableData() : _visibility(0.0f), _factor(1.0f), _color(1.0f, 1.0f, 1.0f, 1.0f)
{
}

const void *HorizontDrawableData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id == 'HoPs') {
		float *v = (float *)buffer;
		v[0] = _visibility;
		v[1] = _factor;
		v[2] = v[3] = 0.0f;
		return buffer;
	}
	if (descr->id == 'HoPt')
		return &_color;
	return CDrawableModelData::_dataPtrForConstant(descr, buffer);
}

void HorizontDrawableData::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	ModelDataWithMatrix::storeSceneStateInStream(stream, flags);
	storeFloatInStream(_visibility, stream);
	storeFloatInStream(_factor, stream);
	storeColorInStream(&_color, stream);
}

const void *HorizontDrawableData::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	data = ModelDataWithMatrix::restoreSceneStateFromData(data, flags);
	data = loadFloatFromStream(data, &_visibility);
	data = loadFloatFromStream(data, &_factor);
	return loadColorFromStream(data, &_color);
}
