// AboutMeshData: the logo's animation time, set by PerspectiveScene::_renderAbout.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(AboutMeshData, LF_CLASSOF(ModelDataWithMatrix))

const void *AboutMeshData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != 'pcom')
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	float *v = (float *)buffer;
	v[0] = _time;
	v[1] = 0.0f;
	v[2] = 0.0f;
	v[3] = 0.0f;
	return buffer;
}
