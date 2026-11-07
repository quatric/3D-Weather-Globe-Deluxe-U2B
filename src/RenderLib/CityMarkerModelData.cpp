// The flat map's route point markers' data.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(CityMarkerModelData, LF_CLASSOF(CParticleSystemData))

const void *CityMarkerModelData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != 'prqd')
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	float size = (Preferences::markersScale() + 0.5f) * 15.0f;
	float *v = (float *)buffer;
	v[0] = size;
	v[1] = size;
	return buffer;
}
