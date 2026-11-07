// MarkerModelData: the marker's color and texture (source 0).

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(MarkerModelData, LF_CLASSOF(ModelDataWithMatrix))

void MarkerModelData::dealloc()
{
	releaseAndClear(_texture);
	CDrawableModelData::dealloc();
}

bool MarkerModelData::_getTextureForSource(BaseTextureEmu **texture, int source)
{
	if (source != 0)
		return CDrawableModelData::_getTextureForSource(texture, source);
	*texture = _texture ? _texture->provideTexture() : 0;
	return true;
}

const void *MarkerModelData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id == 'pcol') {
		memcpy(buffer, &_color, sizeof(_color));
		return buffer;
	}
	if (descr->id == 'pcon')
		return &_params;
	return CDrawableModelData::_dataPtrForConstant(descr, buffer);
}

void MarkerModelData::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	ModelDataWithMatrix::storeSceneStateInStream(stream, flags);
	storeColorInStream(&_color, stream);
}

const void *MarkerModelData::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	data = ModelDataWithMatrix::restoreSceneStateFromData(data, flags);
	return loadColorFromStream(data, &_color);
}
