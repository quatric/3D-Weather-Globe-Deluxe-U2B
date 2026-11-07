// A clouds layer's parameters: its visibility and the blend to the next frame.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CCloudsMeshData, LF_CLASSOF(ModelDataWithMatrix))

CCloudsMeshData::CCloudsMeshData() : _blendFactor(0.0f), _visibility(1.0f)
{
}

void CCloudsMeshData::adjustWithBlendFactor(float factor)
{
	CCloudsShaderStage::cloudsAnimationQuality = Preferences::cloudsAnimationQuality();
	_blendFactor = clamp01(factor);
}

void CCloudsMeshData::setVisibility(float visibility)
{
	_visibility = clamp01(visibility);
}

const void *CCloudsMeshData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != 'blfc')
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	float *v = (float *)buffer;
	float blend = _visibility * _blendFactor;
	v[0] = blend;
	v[1] = 1.0f - (1.0f - _visibility) / (1.0001f - blend);
	v[2] = 0.0f;
	v[3] = 0.0f;
	return buffer;
}

void CCloudsMeshData::storeSceneStateInStream(LData *stream, unsigned int flags)
{
	ModelDataWithMatrix::storeSceneStateInStream(stream, flags);
	storeFloatInStream(_blendFactor, stream);
}

const void *CCloudsMeshData::restoreSceneStateFromData(const void *data, unsigned int flags)
{
	data = ModelDataWithMatrix::restoreSceneStateFromData(data, flags);
	return loadFloatFromStream(data, &_blendFactor);
}
