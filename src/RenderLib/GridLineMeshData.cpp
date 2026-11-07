// The grid line matrices.

#include "RenderLib.h"

LF_IMPLEMENT_CLASS(GridLineMeshData, LF_CLASSOF(CDrawableModelData))

GridLineMeshData::GridLineMeshData() : _linesRenderCount(1)
{
	for (int i = 0; i < 8; i++)
		FHMatrixIdentity(&_lineMatrices[i]);
}

const void *GridLineMeshData::_dataPtrForConstant(const SShaderConstDescr *descr, Matrix *buffer)
{
	if (descr->id != 'GMMa')
		return CDrawableModelData::_dataPtrForConstant(descr, buffer);
	return _lineMatrices;
}
